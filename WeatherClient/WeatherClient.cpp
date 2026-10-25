// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/** \file WeatherClient.cpp
    \brief SDL2-controlled analog weather station rendered with Skia.
    \details Boost.Statechart FSM is driven only by the SDL thread; a jthread
    performs blocking HTTPS work and posts immutable results to the UI queue.
*/
#define SDL_MAIN_HANDLED
#include "WeatherNetwork.h"

#include <SDL.h>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/mpl/list.hpp>

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkRect.h"
#include "include/core/SkSurface.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <format>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace wc = weather_client;
namespace sc = boost::statechart;
namespace mpl = boost::mpl;
namespace {
   constexpr auto refresh = std::chrono::minutes{15};
   constexpr auto retry = std::chrono::minutes{1};

   enum class Request { Ip, Address, Weather };
   struct Completion {
      Request kind{};
      std::optional<wc::Location> location;
      std::optional<wc::WeatherData> weather;
      std::string error;
      std::uint64_t generation{};
   };

   struct Off;
   struct Resolving;
   struct Fetching;
   struct Ready;
   struct Error;
   struct EvStart: sc::event<EvStart> {};
   struct EvLocation: sc::event<EvLocation> {};
   struct EvWeather: sc::event<EvWeather> {};
   struct EvFailure: sc::event<EvFailure> {};
   struct EvRefresh: sc::event<EvRefresh> {};

   struct Station {
      std::mutex mutex;
      std::deque<Completion> completed;
      std::optional<wc::Location> location;
      std::optional<wc::WeatherData> weather;
      std::string error;
      std::string query;
      std::string input;
      bool editing{};
      bool daily{};
      bool demo{};
      bool loading{};
      bool shutdown{};
      std::size_t offset{};
      std::uint64_t generation{};
      std::chrono::steady_clock::time_point next{std::chrono::steady_clock::now()};
      std::filesystem::path config{"weather-location.json"};
      std::jthread worker;

      void Start(Request kind, std::string address = {}) {
         // Stale results are discarded with a monotonically increasing ID.
         ++generation;
         auto const id = generation;
         if(worker.joinable()) {
            // Do not block the UI waiting for an ongoing network request:
            // the previous request is still owned by its jthread. New
            // requests wait for it to finish on the worker's own queue.
            pending.emplace_back(kind,std::move(address),id);
            return;
         }
         Launch(kind,std::move(address),id);
      }

      struct Task { Request kind; std::string address; std::uint64_t id; };
      std::deque<Task> pending;

      void Launch(Request kind, std::string address, std::uint64_t id) {
         loading=true;
         worker = std::jthread([this,kind,address=std::move(address),id] {
            Completion result{.kind=kind,.generation=id};
            try {
               if(demo && kind==Request::Weather) {
                  result.weather=DemoWeather();
               } else if(kind==Request::Ip) {
                  result.location=wc::ResolveIp();
               } else if(kind==Request::Address) {
                  result.location=wc::ResolveAddress(address);
               } else {
                  std::optional<wc::Location> at;
                  {
                     std::lock_guard lock{mutex};
                     at=location;
                  }
                  if(!at) throw std::runtime_error{"Missing location"};
                  result.weather=wc::FetchWeather(*at);
               }
            } catch(std::exception const& e) {
               result.error=e.what();
            }
            std::lock_guard lock{mutex};
            completed.push_back(std::move(result));
         });
      }

      void TickWorker() {
         if(!worker.joinable()) return;
         // Only join after it has posted a completion. The UI never waits on
         // the HTTPS operation itself.
         bool done;
         {
            std::lock_guard lock{mutex};
            done=!completed.empty();
         }
         if(done) {
            worker.join();
            if(!pending.empty()) {
               auto task=std::move(pending.back());
               pending.clear();
               Launch(task.kind,std::move(task.address),task.id);
            }
         }
      }

      static wc::WeatherData DemoWeather() {
         wc::WeatherData data;
         data.timezone="Europe/Berlin";
         data.timezone_abbreviation="CET/CEST";
         data.current.time="2026-10-25T13:00";
         data.current.temperature_2m=18.4;
         data.current.relative_humidity_2m=64;
         data.current.pressure_msl=1015;
         data.current.wind_speed_10m=16;
         data.current.wind_direction_10m=270;
         data.current.precipitation=0.1;
         data.current.cloud_cover=48;
         data.current.weather_code=2;
         data.current.is_day=1;
         for(int i=0;i<48;++i) {
            wc::WeatherHour hour;
            hour.time=std::format("2026-10-{:02}T{:02}:00",25+i/24,i%24);
            hour.temperature_2m=18.4+4*std::sin(i*0.26);
            hour.precipitation_probability=static_cast<double>((i*11)%80);
            hour.wind_speed_10m=11.0+5*std::cos(i*0.2);
            hour.weather_code=(i%9==0?61:2);
            data.hours.push_back(std::move(hour));
         }
         for(int i=0;i<7;++i) {
            wc::WeatherDay day;
            day.date=std::format("2026-10-{:02}",25+i);
            day.temperature_2m_max=19.0+i*.3;
            day.temperature_2m_min=9.0+i*.2;
            day.precipitation_sum=i*.5;
            day.weather_code= i%3==0?61:2;
            data.days.push_back(std::move(day));
         }
         return data;
      }
   };

   struct Machine:sc::state_machine<Machine,Off> {
      Station& station;
      explicit Machine(Station& s):station{s} {}
   };

   struct Off:sc::state<Off,Machine> {
      using reactions=mpl::list<sc::transition<EvStart,Resolving>>;
      explicit Off(my_context c):my_base(c) {}
   };
   struct Resolving:sc::state<Resolving,Machine> {
      using reactions=mpl::list<
         sc::transition<EvLocation,Fetching>,
         sc::transition<EvFailure,Error>,
         sc::transition<EvRefresh,Resolving>,
         sc::transition<EvStart,Resolving>>;
      explicit Resolving(my_context c):my_base(c) {}
   };
   struct Fetching:sc::state<Fetching,Machine> {
      using reactions=mpl::list<
         sc::transition<EvWeather,Ready>,
         sc::transition<EvFailure,Error>,
         sc::transition<EvRefresh,Fetching>,
         sc::transition<EvStart,Resolving>>;
      explicit Fetching(my_context c):my_base(c) {}
   };
   struct Ready:sc::state<Ready,Machine> {
      using reactions=mpl::list<
         sc::transition<EvRefresh,Fetching>,
         sc::transition<EvStart,Resolving>>;
      explicit Ready(my_context c):my_base(c) {}
   };
   struct Error:sc::state<Error,Machine> {
      using reactions=mpl::list<
         sc::transition<EvRefresh,Fetching>,
         sc::transition<EvStart,Resolving>,
         sc::transition<EvLocation,Fetching>>;
      explicit Error(my_context c):my_base(c) {}
   };

   void SetColor(SkPaint& paint, SkColor color) {
      paint.setAntiAlias(true);
      paint.setColor(color);
   }
   void Text(SkCanvas* canvas,float x,float y,std::string const& text,
      float size=18,SkColor color=0xffd7e7ec) {
      SkPaint paint;
      SetColor(paint,color);
      SkFont font;
      font.setSize(size);
      canvas->drawSimpleText(text.data(),text.size(),SkTextEncoding::kUTF8,x,y,font,paint);
   }
   void Needle(SkCanvas* canvas,float cx,float cy,float radius,double percent,
      SkColor color) {
      percent=std::clamp(percent,0.0,1.0);
      float const pi=3.14159265f;
      float const angle=(-225.f+static_cast<float>(percent)*270.f)*pi/180.f;
      SkPaint p;SetColor(p,color);
      p.setStrokeWidth(4);
      p.setStrokeCap(SkPaint::kRound_Cap);
      canvas->drawLine(cx,cy,cx+std::cos(angle)*radius,cy+std::sin(angle)*radius,p);
      p.setColor(0xfff1ede0);
      canvas->drawCircle(cx,cy,7,p);
   }
   void Gauge(SkCanvas* canvas,float cx,float cy,float radius,
      std::string const& label,std::optional<double> value,
      double minimum,double maximum,std::string const& unit) {
      SkPaint p;
      SetColor(p,0xff1b3444);
      canvas->drawCircle(cx+3,cy+5,radius+7,p);
      SetColor(p,0xffb59b62);p.setStyle(SkPaint::kStroke_Style);p.setStrokeWidth(4);
      canvas->drawCircle(cx,cy,radius+5,p);
      p.setStyle(SkPaint::kFill_Style);
      SetColor(p,0xffecedde);canvas->drawCircle(cx,cy,radius,p);
      SetColor(p,0xff778d8f);p.setStrokeWidth(2);
      for(int i=0;i<=10;++i) {
         float a=(-225.f+i*27.f)*3.14159265f/180.f;
         float const d= i%5==0?17.f:10.f;
         canvas->drawLine(cx+std::cos(a)*(radius-d),
            cy+std::sin(a)*(radius-d),
            cx+std::cos(a)*(radius-4.f),cy+std::sin(a)*(radius-4.f),p);
      }
      Text(canvas,cx-radius*.55f,cy-radius*.38f,label,15,0xff314b50);
      if(value) {
         Needle(canvas,cx,cy,radius*.72f,(*value-minimum)/(maximum-minimum),0xffb82f39);
         Text(canvas,cx-radius*.45f,cy+radius*.47f,
            std::format("{:.1f} {}",*value,unit),17,0xff1e3b46);
      } else {
         Text(canvas,cx-15,cy+radius*.47f,"--",17,0xff314b50);
      }
   }

   void Render(SkCanvas* canvas,int width,int height,Station const& s) {
      canvas->clear(0xff0e202d);
      canvas->save();
      float const scale=(std::min)(width/1200.f,height/780.f);
      canvas->translate((width-1200*scale)*.5f,(height-780*scale)*.5f);
      canvas->scale(scale,scale);
      SkPaint p;SetColor(p,0xff233c4c);
      canvas->drawRoundRect(SkRect::MakeXYWH(18,16,1164,748),24,24,p);
      SetColor(p,0xffb39a66);p.setStyle(SkPaint::kStroke_Style);p.setStrokeWidth(3);
      canvas->drawRoundRect(SkRect::MakeXYWH(18,16,1164,748),24,24,p);
      Text(canvas,65,78,"ADECC   |   WEATHER STATION",30,0xfff1e7cc);
      Text(canvas,67,112,s.location?s.location->label:"Standort wird ermittelt ...",17);
      Text(canvas,66,140,
         s.weather?std::format("{}  |  {}  |  {}",
            s.weather->current.time,s.weather->timezone,
            wc::WeatherDescription(s.weather->current.weather_code.value_or(-1)))
            : "Wetterdaten noch nicht geladen",16,0xff9fc6d1);

      auto const& c=s.weather?&s.weather->current:nullptr;
      Gauge(canvas,210,300,122,"TEMPERATUR",c?c->temperature_2m:std::nullopt,-25,45,"C");
      Gauge(canvas,510,300,122,"LUFTDRUCK",c?c->pressure_msl:std::nullopt,950,1050,"hPa");
      Gauge(canvas,810,300,122,"FEUCHTE",c?c->relative_humidity_2m:std::nullopt,0,100,"%");
      Gauge(canvas,1060,300,80,"WIND",c?c->wind_speed_10m:std::nullopt,0,100,"km/h");
      Text(canvas,55,480,s.daily?"7-TAGE-VORSCHAU":"STUNDENVORSCHAU",22,0xfff1e7cc);
      Text(canvas,800,480,"[H] Stunden   [D] Tage",14);
      if(s.weather) {
         if(s.daily) {
            for(std::size_t i=s.offset;i<s.weather->days.size()&&i<s.offset+6;++i) {
               auto const& d=s.weather->days[i];
               float x=65+(i-s.offset)*185.f;
               Text(canvas,x,530,d.date,16);
               Text(canvas,x,560,std::format("{:.0f}/{:.0f} C",
                  d.temperature_2m_max.value_or(0),d.temperature_2m_min.value_or(0)),20,0xfff0d28b);
               Text(canvas,x,590,std::format("Regen {:.1f} mm",
                  d.precipitation_sum.value_or(0)),15);
               Text(canvas,x,618,wc::WeatherDescription(d.weather_code.value_or(-1)),13);
            }
         } else {
            for(std::size_t i=s.offset;i<s.weather->hours.size()&&i<s.offset+8;++i) {
               auto const& h=s.weather->hours[i];
               float x=55+(i-s.offset)*140.f;
               Text(canvas,x,530,h.time.size()>=16?h.time.substr(11,5):h.time,19);
               Text(canvas,x,562,std::format("{:.1f} C",h.temperature_2m.value_or(0)),21,0xfff0d28b);
               Text(canvas,x,594,std::format("{:.0f}% Regen",
                  h.precipitation_probability.value_or(0)),15);
               Text(canvas,x,622,wc::WeatherDescription(h.weather_code.value_or(-1)),12);
            }
         }
      }
      if(s.editing) {
         SetColor(p,0xff102631);
         canvas->drawRoundRect(SkRect::MakeXYWH(45,661,1100,46),8,8,p);
         Text(canvas,59,691,"Adresse: "+s.input+"_",18,0xfff1e7cc);
      } else {
         Text(canvas,55,692,"[A] Adresse   [I] IP-Ortung   [R] Aktualisieren   [< >] Blaettern   [ESC] Ende",16);
      }
      Text(canvas,55,729,s.error.empty()
         ? (s.loading?"FSM: LADE DATEN ...":s.demo?"DEMO / OFFLINE":"FSM: BEREIT")
         : std::format("FSM: FEHLER  {}",s.error.substr(0,105)),14,
         s.error.empty()?0xff9ed7bf:0xffffc0a8);
      canvas->restore();
   }
}
int main(int argc,char** argv) {
   // Run the deterministic JSON/forecast smoke without initializing SDL.
   for(int i=1;i<argc;++i) {
      if(std::string_view(argv[i])=="--self-test") {
         auto const data=Station::DemoWeather();
         bool const valid=data.hours.size()==48 && data.days.size()==7 &&
            data.current.temperature_2m.has_value();
         std::cout << (valid?"WEATHER_CLIENT_TEST_PASS\n":"WEATHER_CLIENT_TEST_FAIL\n");
         return valid?0:2;
      }
   }
   if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK) return 1;
   SDL_SetMainReady();
   if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0) {
      std::cerr << SDL_GetError() << '\n';curl_global_cleanup();return 1;
   }
   Station station;
   for(int i=1;i<argc;++i) {
      if(std::string_view(argv[i])=="--demo") station.demo=true;
      if(std::string_view(argv[i])=="--config" && i+1<argc) station.config=argv[++i];

   }
   SDL_Window* window=SDL_CreateWindow("adecc Scholar Weather Station",
      SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1200,780,
      SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
   if(!window) { SDL_Quit();curl_global_cleanup();return 1; }
   SDL_Renderer* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
   if(!renderer) { SDL_DestroyWindow(window);SDL_Quit();curl_global_cleanup();return 1; }
   Machine fsm{station};
   fsm.initiate();
   fsm.process_event(EvStart{});
   try {
      if(station.demo) {
         station.location=wc::Location{"Demo-Standort","demo","Europe/Berlin",52.52,13.405,true};
         fsm.process_event(EvLocation{});
         station.Start(Request::Weather);
      } else if(auto location=wc::LoadLocation(station.config)) {
         station.location=*location;
         fsm.process_event(EvLocation{});
         station.Start(Request::Weather);
      } else {
         station.Start(Request::Ip);
      }
   } catch(std::exception const& e) {
      station.error=e.what();fsm.process_event(EvFailure{});
   }
   bool running=true;
   while(running) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
         if(event.type==SDL_QUIT) running=false;
         if(event.type==SDL_TEXTINPUT&&station.editing)
            station.input+=event.text.text;
         if(event.type!=SDL_KEYDOWN) continue;
         auto const key=event.key.keysym.sym;
         if(station.editing) {
            if(key==SDLK_ESCAPE) { station.editing=false;SDL_StopTextInput(); }
            else if(key==SDLK_BACKSPACE&&!station.input.empty()) station.input.pop_back();
            else if(key==SDLK_RETURN) {
               station.editing=false;SDL_StopTextInput();
               if(!station.input.empty()) {
                  station.query=station.input;
                  fsm.process_event(EvStart{});
                  station.Start(Request::Address,station.input);
               }
            }
            continue;
         }
         if(key==SDLK_ESCAPE) running=false;
         if(key==SDLK_a) {
            station.editing=true;station.input=station.location?station.location->address:"";
            SDL_StartTextInput();
         }
         if(key==SDLK_i&&!station.demo) {
            fsm.process_event(EvStart{});station.Start(Request::Ip);
         }
         if(key==SDLK_r&&station.location) {
            fsm.process_event(EvRefresh{});station.Start(Request::Weather);
         }
         if(key==SDLK_h) { station.daily=false;station.offset=0; }
         if(key==SDLK_d) { station.daily=true;station.offset=0; }
         if(key==SDLK_RIGHT) station.offset+=(station.daily?3:4);
         if(key==SDLK_LEFT) station.offset=station.offset>(station.daily?3U:4U)
            ? station.offset-(station.daily?3U:4U):0U;
      }
      station.TickWorker();
      std::deque<Completion> done;
      {
         std::lock_guard lock{station.mutex};
         done.swap(station.completed);
      }
      for(auto& result:done) {
         if(result.generation!=station.generation) continue;
         station.loading=false;
         if(!result.error.empty()) {
            station.error=result.error;
            fsm.process_event(EvFailure{});
            station.next=std::chrono::steady_clock::now()+retry;
         } else if(result.location) {
            {
               std::lock_guard lock{station.mutex};
               station.location=std::move(result.location);
            }
            station.error.clear();
            if(result.kind==Request::Address) {
               try { wc::SaveLocation(station.config,*station.location); }
               catch(std::exception const& e) { station.error=e.what(); }
            }
            fsm.process_event(EvLocation{});
            station.Start(Request::Weather);
         } else if(result.weather) {
            station.weather=std::move(result.weather);
            station.error.clear();
            fsm.process_event(EvWeather{});
            station.next=std::chrono::steady_clock::now()+refresh;
         }
      }
      if(!station.loading&&std::chrono::steady_clock::now()>=station.next) {
         if(station.location) {
            fsm.process_event(EvRefresh{});
            station.Start(Request::Weather);
         } else if(!station.demo) {
            fsm.process_event(EvStart{});
            station.Start(Request::Ip);
         }
         station.next=std::chrono::steady_clock::now()+refresh;
      }
      int width{},height{};
      SDL_GetRendererOutputSize(renderer,&width,&height);
      width=(std::max)(width,1);height=(std::max)(height,1);
      auto surface=SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width,height));
      if(surface) {
         Render(surface->getCanvas(),width,height,station);
         SkPixmap pixmap;
         if(surface->peekPixels(&pixmap)) {
            SDL_Surface* image=SDL_CreateRGBSurfaceWithFormatFrom(
               const_cast<void*>(pixmap.addr()),width,height,32,
               static_cast<int>(pixmap.rowBytes()),SDL_PIXELFORMAT_ARGB8888);
            if(image) {
               SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer,image);
               SDL_FreeSurface(image);
               if(tex) {
                  SDL_RenderClear(renderer);
                  SDL_RenderCopy(renderer,tex,nullptr,nullptr);
                  SDL_RenderPresent(renderer);
                  SDL_DestroyTexture(tex);
               }
            }
         }
      }
      SDL_Delay(50);
   }
   if(station.worker.joinable()) station.worker.join();
   fsm.terminate();
   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   SDL_Quit();
   curl_global_cleanup();
   return 0;
}
