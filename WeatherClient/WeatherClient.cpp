// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/** \file WeatherClient.cpp
    \brief SDL2-controlled analog weather station rendered with Skia.
    \details Boost.Statechart FSM is driven only by the SDL thread; a jthread
    performs blocking HTTPS work and posts immutable results to the UI queue.
*/
#include "WeatherNetwork.h"
#include "WeatherView.h"
#include "WeatherArtwork.h"
#include "WeatherNarrative.h"
#include "WeatherText.h"

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
// Boost.Statechart transition<Event,...> needs Event types with external linkage.
// Keep the states and events in this named namespace for simple FSM extensions.
namespace weather_client::application {
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
      bool fullscreen{};
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
      lettering::Draw(canvas,x,y,text,size,color);
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

   void Render(SkCanvas* canvas,int width,int height,Station const& s,
      float animationTime) {
      canvas->clear(0xff091923);
      canvas->save();
      float const scale=(std::min)(width/1200.f,height/780.f);
      canvas->translate((width-1200*scale)*.5f,(height-780*scale)*.5f);
      canvas->scale(scale,scale);

      SkPaint p;
      SetColor(p,0xff182e3d);
      canvas->drawRoundRect(SkRect::MakeXYWH(18,14,1164,750),24,24,p);
      p.setStyle(SkPaint::kStroke_Style);
      p.setStrokeWidth(2.f);
      p.setColor(0xff5a8090);
      canvas->drawRoundRect(SkRect::MakeXYWH(18,14,1164,750),24,24,p);

      auto const* current=s.weather?&s.weather->current:nullptr;
      int const code=current?current->weather_code.value_or(-1):-1;
      bool const daytime=!current||current->is_day.value_or(1)!=0;
      artwork::Atmosphere(canvas,code,daytime,animationTime);
      Text(canvas,57,59,"ADECC  /  WEATHER OBSERVATORY",25,0xfff5f1e8);
      Text(canvas,58,84,
         s.location?view::ClipText(s.location->label,65):"Detecting approximate location...",
         17,0xffb3d9e6);
      if(s.weather) {
         auto const brief=wc::ComposeWeatherBrief(*s.weather);
         Text(canvas,58,114,brief.headline,25,0xfff9d19c);
         Text(canvas,58,139,view::ClipText(brief.conditions,86),
            15,0xffdeedf2);
         Text(canvas,58,164,view::ClipText(brief.outlook,105),
            15,0xffe0e8e8);
      } else {
         Text(canvas,58,116,"Waiting for weather observations...",21,0xffe5d2b9);
         Text(canvas,58,147,
            s.loading?"Resolving location and loading the forecast.":
               "Press ADDRESS to choose a location.",15,0xffb9d4dc);
      }

      artwork::Dial(canvas,208,305,104,"TEMPERATURE",
         current?current->temperature_2m:std::nullopt,
         -25,45,"C",0xfff7b76a,animationTime);
      artwork::Dial(canvas,486,305,104,"BAROMETER",
         current?current->pressure_msl:std::nullopt,
         950,1050,"hPa",0xff83d2ed,animationTime);
      artwork::Dial(canvas,764,305,104,"HUMIDITY",
         current?current->relative_humidity_2m:std::nullopt,
         0,100,"%",0xff80d5be,animationTime);
      artwork::WindRose(canvas,1041,305,86,
         current?current->wind_direction_10m:std::nullopt,animationTime);
      if(current&&current->wind_speed_10m) {
         Text(canvas,994,415,std::format("WIND {:.0f} km/h",
            *current->wind_speed_10m),15,0xffdbeaf0);
      } else {
         Text(canvas,1010,415,"WIND --",15,0xffdbeaf0);
      }
      view::DrawConditionDetails(canvas,current);
      Text(canvas,55,478,s.daily?"7-DAY FORECAST":"HOURLY FORECAST",
         18,0xfff0e3ce);
      view::DrawForecast(canvas,s.weather?&*s.weather:nullptr,s.daily,s.offset);
      view::DrawControls(canvas,s.editing,s.daily,s.demo,s.input);

      if(!s.error.empty()) {
         Text(canvas,55,733,"ERROR  "+view::ClipText(s.error,95),
            14,0xffffb9ae);
      } else if(s.loading) {
         Text(canvas,55,733,"SYNCING WEATHER DATA  /  PLEASE WAIT",
            14,0xffa8dfed);
      } else {
         Text(canvas,55,733,
            s.demo?"OFFLINE DEMO  /  SYNTHETIC DATA":
               "LIVE WEATHER  /  OPEN-METEO  /  OSM GEOCODING",
            14,0xff9bcbb7);
      }
      Text(canvas,949,733,
         s.weather?view::ClipText(s.weather->timezone,24):"TIMEZONE PENDING",
         13,0xff91aebb);
      canvas->restore();
   }

} // namespace weather_client::application
int main(int argc,char** argv) {
   using namespace weather_client::application;
   namespace view = weather_client::view;
   // Run the deterministic JSON/forecast smoke without initializing SDL.
   for(int i=1;i<argc;++i) {
      if(std::string_view(argv[i])=="--self-test") {
         auto const data=Station::DemoWeather();
         std::string text{"Müller"};
         view::BackspaceUtf8(text);
         auto const brief=wc::ComposeWeatherBrief(data);
         auto const glyphSurface=SkSurfaces::Raster(SkImageInfo::MakeN32Premul(72,24));
         bool glyphVisible=false;
         if(glyphSurface) {
            glyphSurface->getCanvas()->clear(SK_ColorTRANSPARENT);
            weather_client::lettering::Draw(glyphSurface->getCanvas(),2,18,"TEST",15,SK_ColorWHITE);
            SkPixmap glyphPixels;
            if(glyphSurface->peekPixels(&glyphPixels)) {
               for(int y=0;y<24&&!glyphVisible;++y) {
                  for(int x=0;x<72;++x) {
                     if(SkColorGetA(*glyphPixels.addr32(x,y))!=0) {
                        glyphVisible=true;
                        break;
                     }
                  }
               }
            }
         }
         bool const valid=glyphVisible &&
            data.hours.size()==48 && data.days.size()==7 &&
            data.current.temperature_2m.has_value() &&
            text=="Mülle" &&
            !brief.headline.empty() &&
            brief.outlook.starts_with("Next 6h:") &&
            wc::CardinalDirection(270)=="W" &&
            wc::WeatherDescription(61)=="Rain" &&
            view::HitTest(75,680,1200,780,false)==view::Action::Address &&
            view::HitTest(1080,680,1200,780,true)==view::Action::Apply &&
            view::HitTest(75,680,1200,780,true)==view::Action::None &&
            view::MoveOffset(&data,false,0,true)==4 &&
            view::MoveOffset(&data,true,0,true)==1 &&
            view::ClampOffset(&data,false,999)==40 &&
            view::ClampOffset(&data,true,999)==1;
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
   auto submitAddress = [&] {
      station.editing=false;
      SDL_StopTextInput();
      if(!station.input.empty()) {
         station.query=station.input;
         fsm.process_event(EvStart{});
         station.Start(Request::Address,station.input);
      }
   };
   auto execute = [&](view::Action action) {
      switch(action) {
         case view::Action::Address:
            station.editing=true;
            station.input=station.location?station.location->address:"";
            SDL_StartTextInput();
            break;
         case view::Action::Ip:
            if(!station.demo) {
               fsm.process_event(EvStart{});
               station.Start(Request::Ip);
            }
            break;
         case view::Action::Refresh:
            if(station.location) {
               fsm.process_event(EvRefresh{});
               station.Start(Request::Weather);
            }
            break;
         case view::Action::Hours:
            station.daily=false;station.offset=0;
            break;
         case view::Action::Days:
            station.daily=true;station.offset=0;
            break;
         case view::Action::Previous:
            station.offset=view::MoveOffset(station.weather?&*station.weather:nullptr,
               station.daily,station.offset,false);
            break;
         case view::Action::Next:
            station.offset=view::MoveOffset(station.weather?&*station.weather:nullptr,
               station.daily,station.offset,true);
            break;
         case view::Action::Fullscreen:
            station.fullscreen=!station.fullscreen;
            SDL_SetWindowFullscreen(window,station.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
            break;
         case view::Action::Apply:
            submitAddress();
            break;
         case view::Action::Cancel:
            station.editing=false;SDL_StopTextInput();
            break;
         default: break;
      }
   };
   while(running) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
         if(event.type==SDL_QUIT) running=false;
         if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT) {
            int winWidth{},winHeight{};
            SDL_GetWindowSize(window,&winWidth,&winHeight);
            execute(view::HitTest(static_cast<float>(event.button.x),
               static_cast<float>(event.button.y),winWidth,winHeight,station.editing));
         }
         if(event.type==SDL_MOUSEWHEEL&&!station.editing) {
            execute(event.wheel.y<0?view::Action::Next:view::Action::Previous);
         }
         if(event.type==SDL_TEXTINPUT&&station.editing &&
            station.input.size()+SDL_strlen(event.text.text)<301U) {
            station.input+=event.text.text;
         }
         if(event.type!=SDL_KEYDOWN) continue;
         auto const key=event.key.keysym.sym;
         if(station.editing) {
            if(key==SDLK_ESCAPE) execute(view::Action::Cancel);
            else if(key==SDLK_BACKSPACE) view::BackspaceUtf8(station.input);
            else if(key==SDLK_RETURN||key==SDLK_KP_ENTER) execute(view::Action::Apply);
            continue;
         }
         if(key==SDLK_ESCAPE) running=false;
         if(key==SDLK_a) execute(view::Action::Address);
         if(key==SDLK_i) execute(view::Action::Ip);
         if(key==SDLK_r) execute(view::Action::Refresh);
         if(key==SDLK_h) execute(view::Action::Hours);
         if(key==SDLK_d) execute(view::Action::Days);
         if(key==SDLK_f||key==SDLK_F11) execute(view::Action::Fullscreen);
         if(key==SDLK_RIGHT) execute(view::Action::Next);
         if(key==SDLK_LEFT) execute(view::Action::Previous);
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
         Render(surface->getCanvas(),width,height,station,SDL_GetTicks()/1000.f);
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
