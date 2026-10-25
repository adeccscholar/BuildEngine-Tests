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
#include "WeatherFont.h"
#include "WeatherMetrics.h"
#include "WeatherUnits.h"
#include "WeatherMoon.h"
#include "WeatherDiagnostics.h"

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
#include <ctime>
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

   [[nodiscard]] inline std::string LocalClock() {
      std::time_t const now=std::time(nullptr);
      auto const* local=std::localtime(&now);
      if(!local) return "--:--:--";
      char formatted[16]{};
      if(!std::strftime(formatted,sizeof(formatted),"%H:%M:%S",local))
         return "--:--:--";
      return formatted;
   }

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
      wc::UnitSystem units{wc::UnitSystem::Metric};
      bool demo{};
      std::string caBundle;
      std::string lastSuccessfulRefresh;
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
         loading=true;
         error.clear();
         auto const id = generation;
         if(worker.joinable()) {
            // Do not block the UI waiting for an ongoing network request:
            // the previous request is still owned by its jthread. New
            // requests wait for it to finish on the worker's own queue.
            pending.clear();
            pending.emplace_back(kind,std::move(address),id);
            return;
         }
         Launch(kind,std::move(address),id);
      }

      struct Task { Request kind; std::string address; std::uint64_t id; };
      std::deque<Task> pending;

      void Launch(Request kind, std::string address, std::uint64_t id) {
         loading=true;
         bool const useDemo=demo;
         worker = std::jthread([this,kind,address=std::move(address),id,useDemo] {
            Completion result{.kind=kind,.generation=id};
            try {
               if(useDemo && kind==Request::Weather) {
                  result.weather=DemoWeather();
               } else if(kind==Request::Ip) {
                  result.location=wc::ResolveIp(caBundle);
               } else if(kind==Request::Address) {
                  result.location=wc::ResolveAddress(address,caBundle);
               } else {
                  std::optional<wc::Location> at;
                  {
                     std::lock_guard lock{mutex};
                     at=location;
                  }
                  if(!at) throw std::runtime_error{"Missing location"};
                  result.weather=wc::FetchWeather(*at,caBundle);
               }
            } catch(std::exception const& e) {
               result.error=e.what();
            } catch(...) {
               result.error="Unexpected failure in network worker";
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
         data.current.apparent_temperature=17.1;
         data.current.wind_gusts_10m=24;
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
            hour.apparent_temperature=*hour.temperature_2m-1.3;
            hour.relative_humidity_2m=64.0;
            hour.cloud_cover=48.0;
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
            day.precipitation_probability_max=static_cast<double>((i*13)%90);
            day.uv_index_max=2.5;
            day.sunrise=std::format("2026-10-{:02}T07:30",25+i);
            day.sunset=std::format("2026-10-{:02}T17:30",25+i);
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
         sc::transition<EvFailure,Error>,
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

   [[nodiscard]] bool VerifyStatechart() {
      Station station;
      Machine fsm{station};
      fsm.initiate();
      bool ok=fsm.state_cast<Off const*>()!=nullptr;
      fsm.process_event(EvStart{});
      ok=ok && fsm.state_cast<Resolving const*>()!=nullptr;
      fsm.process_event(EvLocation{});
      ok=ok && fsm.state_cast<Fetching const*>()!=nullptr;
      fsm.process_event(EvWeather{});
      ok=ok && fsm.state_cast<Ready const*>()!=nullptr;
      fsm.process_event(EvRefresh{});
      ok=ok && fsm.state_cast<Fetching const*>()!=nullptr;
      fsm.process_event(EvFailure{});
      ok=ok && fsm.state_cast<Error const*>()!=nullptr;
      fsm.process_event(EvStart{});
      ok=ok && fsm.state_cast<Resolving const*>()!=nullptr;
      fsm.process_event(EvLocation{});
      fsm.process_event(EvWeather{});
      ok=ok && fsm.state_cast<Ready const*>()!=nullptr;
      fsm.terminate();
      return ok && fsm.terminated();
   }

   void SetColor(SkPaint& paint, SkColor color) {
      paint.setAntiAlias(true);
      paint.setColor(color);
   }
   void Text(SkCanvas* canvas,float x,float y,std::string const& text,
      float size=18,SkColor color=0xffd7e7ec) {
      font::Draw(canvas,x,y,text,size,color);
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
      wc::diagnostics::Trace("Render: start");
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
      bool const daytime=!s.weather||wc::CurrentDaylight(*s.weather);
      auto const moon=s.weather?wc::CalculateMoonPhase(s.weather->current.time):
         std::optional<wc::MoonPhase>{};
      wc::diagnostics::Trace("Render: atmosphere");
      artwork::Atmosphere(canvas,code,daytime,animationTime,moon);
      wc::diagnostics::Trace("Render: header font");
      font::Draw(canvas,57,59,"ADECC / WEATHER OBSERVATORY",
         23,0xfff5f1e8,790);
      std::string label=s.location?s.location->address:"Locating your region...";
      if(s.location && s.location->approximate)
         label=s.location->label+" (approximate)";
      font::Draw(canvas,58,85,label,17,0xffb3d9e6,760.f);
      if(s.weather) {
         auto const brief=wc::ComposeWeatherBrief(*s.weather,s.units);
         font::Draw(canvas,58,117,
            brief.headline+(daytime?"  /  DAYLIGHT":"  /  NIGHT"),
            24,0xfff9d19c,790);
         font::Draw(canvas,58,142,brief.conditions,15,0xffdeedf2,796);
         font::Draw(canvas,58,164,brief.outlook,14,0xffe0e8e8,796);
         auto const* today=wc::Today(*s.weather);
         if(s.daily && s.offset<s.weather->days.size())
            today=&s.weather->days[s.offset];
         std::string const period=s.daily && today
            ? today->date : std::string{"TODAY"};
         std::string const sun=std::format("{} {}   SUNRISE {}   SUNSET {}",
            period,wc::RangeText(today,s.units),
            today?wc::ClockText(today->sunrise):"--:--",
            today?wc::ClockText(today->sunset):"--:--");
         font::Draw(canvas,58,184,sun,14,0xfff0d8b0,796);
         if(!daytime && moon) {
            font::Draw(canvas,800,184,
               std::string{wc::MoonStageName(moon->stage)}+
                  std::format("  {:.0f}% LIT",moon->illumination*100.),
               12,0xffd7e3f3,315);
         }
      } else {
         font::Draw(canvas,58,116,"Waiting for weather observations...",
            22,0xffe5d2b9,790);
         font::Draw(canvas,58,147,
            s.loading?"Resolving location and loading the forecast.":
               "Press ADDRESS to choose a location.",
            15,0xffb9d4dc,790);
      }

      wc::diagnostics::Trace("Render: gauges");
      artwork::Dial(canvas,208,305,104,"TEMPERATURE",
         current&&current->temperature_2m?
            std::optional<double>{wc::Temperature(*current->temperature_2m,s.units)}:
            std::nullopt,
         s.units==wc::UnitSystem::US?-13.f:-25.f,
         s.units==wc::UnitSystem::US?113.f:45.f,
         s.units==wc::UnitSystem::US?"°F":"°C",0xfff7b76a,animationTime);
      artwork::Dial(canvas,486,305,104,"BAROMETER",
         current&&current->pressure_msl?
            std::optional<double>{wc::Pressure(*current->pressure_msl,s.units)}:
            std::nullopt,
         s.units==wc::UnitSystem::US?wc::Pressure(950,wc::UnitSystem::US):950.,
         s.units==wc::UnitSystem::US?wc::Pressure(1050,wc::UnitSystem::US):1050.,
         s.units==wc::UnitSystem::US?"inHg":"hPa",0xff83d2ed,animationTime);
      artwork::Dial(canvas,764,305,104,"HUMIDITY",
         current?current->relative_humidity_2m:std::nullopt,
         0,100,"%",0xff80d5be,animationTime);
      artwork::WindRose(canvas,1041,305,86,
         current?current->wind_direction_10m:std::nullopt,animationTime);
      if(current&&current->wind_speed_10m) {
         font::Draw(canvas,959,415,
            "WIND "+wc::WindText(*current->wind_speed_10m,s.units),
            15,0xffdbeaf0,175);
      } else {
         Text(canvas,1010,415,"WIND --",15,0xffdbeaf0);
      }
      wc::diagnostics::Trace("Render: details");
      view::DrawConditionDetails(canvas,current,s.units);
      if(s.weather) {
         font::Draw(canvas,60,475,wc::StormLabel(wc::StormOutlook(*s.weather)),
            13,0xffe6c0a5,245);
      }
      font::Draw(canvas,365,475,
         s.daily?"DAILY OUTLOOK":"NEXT HOURS",16,0xfff0e3ce,360);
      wc::diagnostics::Trace("Render: forecast");
      view::DrawForecast(canvas,s.weather?&*s.weather:nullptr,s.daily,s.offset,s.units);
      wc::diagnostics::Trace("Render: controls");
      view::DrawControls(canvas,s.editing,s.daily,s.demo,s.input,s.units);

      std::string status=s.demo?"OFFLINE DEMO":
         "LIVE WEATHER "+LocalClock()+"  /  "+(s.lastSuccessfulRefresh.empty()
            ? std::string{"AWAITING FIRST UPDATE"}
            : std::string{"UPDATED "+s.lastSuccessfulRefresh});
      if(s.loading) status+=" / UPDATING";
      font::Draw(canvas,55,733,status,14,0xff9bcbb7,765);
      if(!s.error.empty())
         font::Draw(canvas,55,753,"ERROR: "+s.error,12,0xffffb9ae,530);
      font::Draw(canvas,600,753,
         "OPEN-METEO CC BY 4.0 / (C) OPENSTREETMAP CONTRIBUTORS ODbL",
         11,0xff8facba,540);
      font::Draw(canvas,998,733,
         s.weather?s.weather->timezone:"TIMEZONE PENDING",
         12,0xff91aebb,155);
      canvas->restore();
      wc::diagnostics::Trace("Render: complete");
   }

} // namespace weather_client::application
int main(int argc,char** argv) {
   using namespace weather_client::application;
   namespace view = weather_client::view;
   namespace font = weather_client::font;
   // Run the deterministic JSON/forecast smoke without initializing SDL.
   for(int i=1;i<argc;++i) {
      if(std::string_view(argv[i])=="--weather-probe" && i+2<argc) {
         std::ofstream report{"weather-client-probe.log",std::ios::trunc};
         report<<"Starting weather probe"<<std::endl;
         if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK) {
            report<<"curl_global_init failed"<<std::endl; return 4;
         }
         int exitCode=0;
         try {
            wc::Location location;
            location.latitude=std::stod(argv[i+1]);
            location.longitude=std::stod(argv[i+2]);
            auto weather=wc::FetchWeather(location);
            report<<"Open-Meteo OK: "<<weather.current.time
                  <<" / hours: "<<weather.hours.size()<<std::endl;
         } catch(std::exception const& e) {
            report<<"Open-Meteo FAILED: "<<e.what()<<std::endl;
            exitCode=5;
         }
         curl_global_cleanup();
         return exitCode;
      }
      if(std::string_view(argv[i])=="--fsm-test") {
         bool const passed=VerifyStatechart();
         std::cout << (passed?"WEATHER_FSM_TEST_PASS\n":"WEATHER_FSM_TEST_FAIL\n");
         return passed?0:2;
      }
      if(std::string_view(argv[i])=="--self-test" ||
         std::string_view(argv[i])=="--graphics-test") {
         bool const graphics=std::string_view(argv[i])=="--graphics-test";
         auto const data=Station::DemoWeather();
         std::string text{"Müller"};
         view::BackspaceUtf8(text);
         auto const brief=wc::ComposeWeatherBrief(data);
         if(graphics) {
            wc::diagnostics::Trace("Graphics test: begin");
            SDL_SetMainReady();
            if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0) {
               std::cerr << "WEATHER_GRAPHICS_INIT_FAIL: " << SDL_GetError() << '\n';
               return 3;
            }
            std::cerr << "[GRAPHICS] SDL initialized" << std::endl;
         }
         auto const glyphSurface=graphics?
            SkSurfaces::Raster(SkImageInfo::MakeN32Premul(72,24)):nullptr;
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
         auto const newMoon=wc::CalculateMoonPhase("2000-01-06T18:14");
         auto const fullMoon=wc::CalculateMoonPhase("2000-01-21T04:00");
         auto const waxing=wc::CalculateMoonPhase("2000-01-13T12:00");
         auto const waning=wc::CalculateMoonPhase("2000-01-28T12:00");
         auto stormData=data;
         stormData.hours.front().weather_code=95;
         Station preview;
         preview.demo=true;
         preview.location=wc::Location{"Example weather station",
            "Example address","Europe/Berlin",52.5,13.4,false};
         preview.weather=data;
         auto const previewSurface=graphics?
            SkSurfaces::Raster(SkImageInfo::MakeN32Premul(1200,780)):nullptr;
         bool overviewDrawn=false;
         if(previewSurface) {
            std::cerr << "[GRAPHICS] Rendering preview" << std::endl;
            Render(previewSurface->getCanvas(),1200,780,preview,1.f);
            std::cerr << "[GRAPHICS] Render complete" << std::endl;
            SkPixmap pixels;
            if(previewSurface->peekPixels(&pixels)) {
               overviewDrawn=SkColorGetR(*pixels.addr32(1000,85))!=
                  SkColorGetR(*pixels.addr32(60,550));
            }
         }
         bool valid=true;
         auto check=[&](bool result,char const* name) {
            if(!result) {
               std::cerr<<"WEATHER_CHECK_FAIL: "<<name<<std::endl;
               valid=false;
            }
         };
         check(((!graphics || (glyphVisible && overviewDrawn))),"model condition 1");
         check((newMoon && newMoon->illumination<0.001),"model condition 2");
         check((newMoon->stage==wc::MoonStage::New),"model condition 3");
         check((fullMoon && fullMoon->illumination>0.98),"model condition 4");
         check((waxing && waxing->waxing && waxing->illumination>0.25),"model condition 5");
         check((waning && !waning->waxing && waning->illumination>0.20),"model condition 6");
         check((!wc::CalculateMoonPhase("2026-02-30T12:00")),"model condition 7");
         check((!graphics || font::Width(font::Fit("A long address with Unicode \u00e4 and more",
               16.f,170.f),16.f)<=170.1f),"graphics text fitting");
         check((VerifyStatechart()),"model condition 9");
         check((wc::CurrentDaylight(data)),"model condition 10");
         check((wc::IsDayAt("2026-10-25T21:00",data)==false),"model condition 11");
         auto const* today=wc::Today(data);
         check((today && today->temperature_2m_min && today->temperature_2m_max &&
            *today->temperature_2m_min==9. && *today->temperature_2m_max==19.),
            "daily source min/max");
         auto const metricRange=wc::RangeText(today);
         auto const usRange=wc::RangeText(today,wc::UnitSystem::US);
         if(metricRange!="9 °C / 19 °C")
            std::cerr<<"METRIC RANGE: "<<metricRange<<std::endl;
         if(usRange!="48 °F / 66 °F")
            std::cerr<<"US RANGE: "<<usRange<<std::endl;
         check((metricRange=="9 °C / 19 °C"),"metric daily range");
         check((usRange=="48 °F / 66 °F"),"US daily range");
         check((wc::Temperature(0.,wc::UnitSystem::US)==32.),"model condition 14");
         check((wc::Temperature(100.,wc::UnitSystem::US)==212.),"model condition 15");
         check((std::abs(wc::WindSpeed(100.,wc::UnitSystem::US)-62.137119)<0.001),"model condition 16");
         check((std::abs(wc::Pressure(1013.25,wc::UnitSystem::US)-29.92125)<0.01),"model condition 17");
         check((wc::Precipitation(25.4,wc::UnitSystem::US)==1.),"model condition 18");
         check((wc::ParseUnits("us")==wc::UnitSystem::US),"model condition 19");
         check((wc::ParseUnits("metric")==wc::UnitSystem::Metric),"model condition 20");
         check((wc::ComposeWeatherBrief(data,wc::UnitSystem::US).conditions.find("mph")!=std::string::npos),"model condition 21");
         check((wc::ComposeWeatherBrief(data,wc::UnitSystem::US).conditions.find("°F")!=std::string::npos),"model condition 22");
         check((wc::ClockText(wc::Today(data)->sunrise)=="07:30"),"model condition 23");
         check((wc::StormOutlook(data)==wc::StormSignal::NotIndicated),"model condition 24");
         check((wc::StormOutlook(stormData)==wc::StormSignal::Forecast),"model condition 25");
         check((data.hours.size()==48 && data.days.size()==7),"model condition 26");
         check((data.current.temperature_2m.has_value()),"model condition 27");
         check((text=="Mülle"),"model condition 28");
         check((!brief.headline.empty()),"model condition 29");
         check((brief.outlook.starts_with("Next 6h:")),"model condition 30");
         check((wc::CardinalDirection(270)=="W"),"model condition 31");
         check((wc::WeatherDescription(61)=="Rain"),"model condition 32");
         check((view::HitTest(75,680,1200,780,false)==view::Action::Address),"model condition 33");
         check((view::HitTest(1100,680,1200,780,false)==view::Action::Units),"model condition 34");
         check((view::HitTest(1080,680,1200,780,true)==view::Action::Apply),"model condition 35");
         check((view::HitTest(75,680,1200,780,true)==view::Action::None),"model condition 36");
         check((view::MoveOffset(&data,false,0,true)==4),"model condition 37");
         check((view::MoveOffset(&data,true,0,true)==1),"model condition 38");
         check((view::ClampOffset(&data,false,999)==40),"model condition 39");
         check((view::ClampOffset(&data,true,999)==1),"model condition 40");
         std::cout << (valid?"WEATHER_CLIENT_TEST_PASS\n":"WEATHER_CLIENT_TEST_FAIL\n");
         if(graphics) {
            std::cerr << "[GRAPHICS] Releasing fonts" << std::endl;
            font::ReleaseFonts();
            SDL_Quit();
         }
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
      if(std::string_view(argv[i])=="--ca-bundle" && i+1<argc) station.caBundle=argv[++i];

   }
   station.units=wc::LoadUnits(station.config);
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
         station.demo=false;
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
            station.demo=false;
            fsm.process_event(EvStart{});
            station.Start(Request::Ip);
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
         case view::Action::Units:
            station.units=station.units==wc::UnitSystem::Metric?
               wc::UnitSystem::US:wc::UnitSystem::Metric;
            try { wc::SaveUnits(station.config,station.units); }
            catch(std::exception const& e) { station.error=e.what(); }
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
         if(key==SDLK_u) execute(view::Action::Units);
         if(key==SDLK_f||key==SDLK_F11) execute(view::Action::Fullscreen);
         if(key==SDLK_RIGHT) execute(view::Action::Next);
         if(key==SDLK_LEFT) execute(view::Action::Previous);
      }
      try {
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
            station.weather.reset();
            station.offset=0;
            if(result.kind==Request::Address) {
               try { wc::SaveLocation(station.config,*station.location,station.units); }
               catch(std::exception const& e) { station.error=e.what(); }
            }
            fsm.process_event(EvLocation{});
            station.Start(Request::Weather);
         } else if(result.weather) {
            station.weather=std::move(result.weather);
            station.lastSuccessfulRefresh=LocalClock();
            if(station.location) station.location->timezone=station.weather->timezone;
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
         station.next=std::chrono::steady_clock::now()+
            (station.error.empty()?refresh:retry);
      }
      } catch(std::exception const& e) {
         station.error=std::string{"Weather update: "}+e.what();
         station.loading=false;
         station.next=std::chrono::steady_clock::now()+retry;
         try { fsm.process_event(EvFailure{}); } catch(...) { }
      } catch(...) {
         station.error="Unexpected failure while processing weather update";
         station.loading=false;
         station.next=std::chrono::steady_clock::now()+retry;
      }
      int width{},height{};
      SDL_GetRendererOutputSize(renderer,&width,&height);
      width=(std::max)(width,1);height=(std::max)(height,1);
      auto surface=SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width,height));
      if(surface) {
         try {
            Render(surface->getCanvas(),width,height,station,SDL_GetTicks()/1000.f);
         } catch(std::exception const& e) {
            station.error=std::string{"Weather graphics: "}+e.what();
            surface->getCanvas()->clear(0xff091923);
            weather_client::lettering::Draw(surface->getCanvas(),24,50,
               station.error,16,SK_ColorWHITE);
         }
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
   font::ReleaseFonts();
   SDL_Quit();
   curl_global_cleanup();
   return 0;
}
