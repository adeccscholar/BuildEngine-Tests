// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherView.h
    \brief Portable Skia dashboard and SDL2 hit-test geometry.
    \details Stateless drawing and controls. No Win32, SDL window handles,
    network calls, filesystem access or FSM transitions are used here.
*/
#include "WeatherModel.h"
#include "WeatherText.h"
#include "WeatherFont.h"
#include "WeatherMetrics.h"
#include "WeatherUnits.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkPathBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace weather_client::view {
   constexpr float DesignWidth = 1200.f;
   constexpr float DesignHeight = 780.f;
   constexpr float Pi = 3.14159265358979323846f;

   enum class Action {
      None, Address, Ip, Refresh, Hours, Days, Previous, Next,
      Fullscreen, Units, Apply, Cancel
   };

   struct Button {
      Action action;
      float x, y, width, height;
      std::string_view label;
   };

   inline constexpr std::array<Button,9> Buttons{{
      {Action::Address,   50, 669, 148, 40, "ADDRESS"},
      {Action::Ip,       206, 669, 128, 40, "IP LOCATION"},
      {Action::Refresh,  342, 669, 144, 40, "UPDATE"},
      {Action::Hours,    494, 669, 132, 40, "HOURLY"},
      {Action::Days,     634, 669, 115, 40, "DAILY"},
      {Action::Previous, 757, 669,  90, 40, "<"},
      {Action::Next,     855, 669,  90, 40, ">"},
      {Action::Fullscreen,953,669, 100, 40, "FULL"},
      {Action::Units,1061,669, 79, 40, "UNITS"}
   }};
   inline constexpr Button Apply{Action::Apply, 1015,669,125,40,"OK"};
   inline constexpr Button Cancel{Action::Cancel, 878,669,129,40,"CANCEL"};

   [[nodiscard]] inline bool Inside(Button const& button,float x,float y) noexcept {
      return x>=button.x && y>=button.y &&
         x<button.x+button.width && y<button.y+button.height;
   }

   [[nodiscard]] inline std::pair<float,float> ToDesign(
      float pixelX,float pixelY,int outputWidth,int outputHeight
   ) noexcept {
      if(outputWidth<=0 || outputHeight<=0) return {-10000.f,-10000.f};
      float const scale=(std::min)(outputWidth/DesignWidth,outputHeight/DesignHeight);
      float const ox=(outputWidth-DesignWidth*scale)*.5f;
      float const oy=(outputHeight-DesignHeight*scale)*.5f;
      return {(pixelX-ox)/scale,(pixelY-oy)/scale};
   }

   [[nodiscard]] inline Action HitTest(
      float pixelX,float pixelY,int outputWidth,int outputHeight,bool editing
   ) noexcept {
      auto const [x,y]=ToDesign(pixelX,pixelY,outputWidth,outputHeight);
      if(editing) {
         if(Inside(Apply,x,y)) return Action::Apply;
         if(Inside(Cancel,x,y)) return Action::Cancel;
         return Action::None;
      }
      for(auto const& button:Buttons) {
         if(Inside(button,x,y)) return button.action;
      }
      return Action::None;
   }

   [[nodiscard]] inline std::size_t ForecastSize(
      WeatherData const* weather,bool daily
   ) noexcept {
      if(!weather) return 0;
      return daily ? weather->days.size() : weather->hours.size();
   }

   [[nodiscard]] inline std::size_t ClampOffset(
      WeatherData const* weather,bool daily,std::size_t offset
   ) noexcept {
      std::size_t const count=ForecastSize(weather,daily);
      std::size_t const page=daily?6U:8U;
      return count>page?(std::min)(offset,count-page):0U;
   }

   [[nodiscard]] inline std::size_t MoveOffset(
      WeatherData const* weather,bool daily,std::size_t offset,bool forward
   ) noexcept {
      std::size_t const step=daily?3U:4U;
      offset=ClampOffset(weather,daily,offset);
      if(!forward) return offset>step?offset-step:0U;
      std::size_t const count=ForecastSize(weather,daily);
      std::size_t const page=daily?6U:8U;
      return count>page?(std::min)(offset+step,count-page):0U;
   }

   // One UTF-8 code point, not one byte. SDL_TEXTINPUT supplies UTF-8.
   inline void BackspaceUtf8(std::string& text) {
      if(text.empty()) return;
      text.pop_back();
      while(!text.empty() &&
         (static_cast<unsigned char>(text.back())&0xc0U)==0x80U) {
         text.pop_back();
      }
   }

   [[nodiscard]] inline std::string ClipText(
      std::string_view text,std::size_t maxCodepoints
   ) {
      if(maxCodepoints==0) return {};
      std::size_t points=0;
      std::size_t cut=0;
      for(;cut<text.size();++cut) {
         if((static_cast<unsigned char>(text[cut])&0xc0U)!=0x80U) {
            if(points==maxCodepoints) break;
            ++points;
         }
      }
      std::string result{text.substr(0,cut)};
      if(cut<text.size()) result+="...";
      return result;
   }

   inline void Color(SkPaint& paint,SkColor color) {
      paint.setAntiAlias(true);
      paint.setColor(color);
   }

   inline void Text(SkCanvas* canvas,float x,float y,
      std::string const& text,float size=16,SkColor color=0xffd7e7ec
   ) {
      font::Draw(canvas,x,y,text,size,color);
   }

   inline void DrawButton(
      SkCanvas* canvas,Button const& button,bool active,bool disabled
   ) {
      SkPaint paint;
      Color(paint,disabled?0xff344651:active?0xff506e79:0xff162c3a);
      canvas->drawRoundRect(
         SkRect::MakeXYWH(button.x,button.y,button.width,button.height),7,7,paint);
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(active?2.f:1.f);
      Color(paint,disabled?0xff64737b:active?0xffdfbe80:0xff7795a1);
      canvas->drawRoundRect(
         SkRect::MakeXYWH(button.x,button.y,button.width,button.height),7,7,paint);
      font::Draw(canvas,button.x+13,button.y+26,button.label,
         button.width<100?17.f:14.f,disabled?0xff72828b:0xffeff0e7,
         button.width-22.f);
   }

   inline void DrawControls(
      SkCanvas* canvas,bool editing,bool daily,bool demo,std::string_view input,
      UnitSystem units=UnitSystem::Metric
   ) {
      SkPaint paint;
      if(editing) {
         Color(paint,0xff0e2632);
         canvas->drawRoundRect(SkRect::MakeXYWH(50,669,818,40),7,7,paint);
         Color(paint,0xffb39a66);
         paint.setStyle(SkPaint::kStroke_Style);
         canvas->drawRoundRect(SkRect::MakeXYWH(50,669,818,40),7,7,paint);
         font::Draw(canvas,63,695,"ADDRESS: "+std::string{input}+"_",16,0xfff1e7cc,798.f);
         DrawButton(canvas,Cancel,false,false);
         DrawButton(canvas,Apply,false,false);
      } else {
         for(auto const& button:Buttons) {
            bool const active=(button.action==Action::Hours&&!daily) ||
               (button.action==Action::Days&&daily);
            if(button.action==Action::Units) {
               Button unitButton=button;
               unitButton.label=units==UnitSystem::US?"US":"METRIC";
               DrawButton(canvas,unitButton,units==UnitSystem::US,false);
            } else DrawButton(canvas,button,active,demo&&button.action==Action::Ip);
         }
      }
   }

   inline void DrawConditionDetails(
      SkCanvas* canvas,WeatherCurrent const* current,
      UnitSystem units=UnitSystem::Metric
   ) {
      if(!current) {
         Text(canvas,65,449,"Loading observations ...",16,0xff99b2bc);
         return;
      }
      auto number=[](std::optional<double> value,int digits=0)->std::string {
         if(!value) return "--";
         return digits==1?std::format("{:.1f}",*value):std::format("{:.0f}",*value);
      };
      Text(canvas,63,451,
         std::format("FEELS LIKE: {}",current->apparent_temperature?
            TempText(*current->apparent_temperature,units):"--"),17,0xffe6d6b7);
      Text(canvas,331,451,
         std::format("CLOUDS: {} %",number(current->cloud_cover)),17,0xffc0d4df);
      Text(canvas,565,451,
         std::format("PRECIPITATION: {}",current->precipitation?
            RainText(*current->precipitation,units):"--"),17,0xff9dcced);
      Text(canvas,919,451,
         current->wind_gusts_10m?
            std::format("GUSTS: {}",WindText(*current->wind_gusts_10m,units)):
            std::format("WIND: {}",current->wind_direction_10m?
               std::format("{:.0f} DEG",*current->wind_direction_10m):"--"),
         14,0xffc9dbdb);

      if(current->wind_direction_10m) {
         float const angle=(static_cast<float>(*current->wind_direction_10m)-90.f)*Pi/180.f;
         float const x=1119.f,y=444.f;
         SkPaint p;
         Color(p,0xffd7c79f);
         p.setStyle(SkPaint::kStroke_Style);
         p.setStrokeWidth(2);
         canvas->drawCircle(x,y,19,p);
         p.setStyle(SkPaint::kFill_Style);
         Color(p,0xffe7aa76);
         canvas->drawLine(x,y,x+std::cos(angle)*16,
            y+std::sin(angle)*16,p);
         canvas->drawCircle(x,y,3,p);
         Text(canvas,x-5,y-24,"N",11,0xffe1e1d5);
      }
   }

   inline void DrawMoonSymbol(SkCanvas* canvas,float x,float y) {
      SkPaint paint;
      Color(paint,0xffe8e8d2);
      // Tiny original crescent silhouette: do not obscure with the cloud.
      SkPathBuilder crescent;
      crescent.moveTo(x+4,y-10);
      crescent.cubicTo(x-10,y-12,x-12,y+8,x+2,y+10);
      crescent.cubicTo(x-5,y+3,x-4,y-7,x+4,y-10);
      crescent.close();
      canvas->drawPath(crescent.detach(),paint);
   }

   inline void DrawSkySymbol(
      SkCanvas* canvas,float x,float y,int code,bool isDay
   ) {
      SkPaint paint;
      bool const clear=code<=1;
      bool const rain=(code>=51&&code<=67)||(code>=80&&code<=82)||code>=95;
      bool const snow=(code>=71&&code<=77)||code==85||code==86;
      bool const storm=code==95||code==96||code==99;
      if(isDay) {
         Color(paint,0xffefc56b);
         canvas->drawCircle(x-10,y-8,7,paint);
         paint.setStrokeWidth(1.6f);
         for(int i=0;i<8;++i) {
            float const a=static_cast<float>(i)*Pi/4.f;
            canvas->drawLine(x-10+std::cos(a)*10,y-8+std::sin(a)*10,
               x-10+std::cos(a)*14,y-8+std::sin(a)*14,paint);
         }
      } else {
         DrawMoonSymbol(canvas,x-10,y-8);
      }
      if(!clear) {
         Color(paint,0xffb9c9d5);
         canvas->drawCircle(x,y+1,8,paint);
         canvas->drawCircle(x+11,y-3,10,paint);
         canvas->drawCircle(x+20,y+2,7,paint);
         canvas->drawRect(SkRect::MakeXYWH(x-1,y,21,10),paint);
         if(rain) {
            Color(paint,0xff7cc9ed);
            paint.setStrokeWidth(2.f);
            canvas->drawLine(x+3,y+12,x-1,y+20,paint);
            canvas->drawLine(x+13,y+12,x+9,y+20,paint);
         }
         if(snow) {
            Color(paint,0xffecf7ff);
            canvas->drawCircle(x+3,y+16,2.3f,paint);
            canvas->drawCircle(x+15,y+19,2.3f,paint);
         }
         if(storm) {
            SkPathBuilder bolt;
            bolt.moveTo(x+9,y+10).lineTo(x+3,y+19)
               .lineTo(x+9,y+17).lineTo(x+5,y+26)
               .lineTo(x+19,y+13).lineTo(x+13,y+15).close();
            Color(paint,0xffffd179);
            canvas->drawPath(bolt.detach(),paint);
         }
      }
   }

   inline void DrawSolarEvent(SkCanvas* canvas,float x,float y,
      bool sunrise,std::string_view time) {
      SkPaint paint;
      Color(paint,0xffeac27c);
      canvas->drawCircle(x+7,y-4,5,paint);
      paint.setColor(0xff183344);
      canvas->drawRect(SkRect::MakeXYWH(x-1,y-1,18,7),paint);
      paint.setColor(0xffebce9a);
      paint.setStrokeWidth(1.5f);
      canvas->drawLine(x-2,y+1,x+19,y+1,paint);
      canvas->drawLine(x+7,sunrise?y+8:y-15,
         x+7,sunrise?y+3:y-9,paint);
      font::Draw(canvas,x+22,y+5,time,12,0xffb5d3d9,55);
   }

   struct Point {
      std::string label;
      std::optional<double> high;
      std::optional<double> low;
      std::optional<double> rain;
      std::optional<int> code;
      bool daylight{true};
      std::string sunrise;
      std::string sunset;
   };

   inline void DrawForecast(
      SkCanvas* canvas,WeatherData const* weather,bool daily,std::size_t offset
   ) {
      SkPaint paint;
      Color(paint,0xff172f3e);
      canvas->drawRoundRect(SkRect::MakeXYWH(48,483,1104,175),11,11,paint);
      paint.setStyle(SkPaint::kStroke_Style);
      Color(paint,0xff496372);paint.setStrokeWidth(1);
      canvas->drawRoundRect(SkRect::MakeXYWH(48,483,1104,175),11,11,paint);
      if(!weather) {
         Text(canvas,78,559,"Loading forecast ...",21);
         return;
      }

      std::vector<Point> points;
      offset=ClampOffset(weather,daily,offset);
      std::size_t const page=daily?6U:8U;
      std::size_t const count=ForecastSize(weather,daily);
      for(std::size_t i=offset;i<count&&i<offset+page;++i) {
         Point point;
         if(daily) {
            auto const& day=weather->days[i];
            point.label=day.date.size()>=10?day.date.substr(5):day.date;
            point.high=day.temperature_2m_max?
               std::optional<double>{Temperature(*day.temperature_2m_max,units)}:std::nullopt;
            point.low=day.temperature_2m_min?
               std::optional<double>{Temperature(*day.temperature_2m_min,units)}:std::nullopt;
            point.rain=day.precipitation_sum?
               std::optional<double>{Precipitation(*day.precipitation_sum,units)}:std::nullopt;
            point.code=day.weather_code;
            point.sunrise=ClockText(day.sunrise);
            point.sunset=ClockText(day.sunset);
         } else {
            auto const& hour=weather->hours[i];
            point.label=hour.time.size()>=16?hour.time.substr(11,5):hour.time;
            point.daylight=IsDayAt(hour.time,*weather).value_or(true);
            point.high=hour.temperature_2m?
               std::optional<double>{Temperature(*hour.temperature_2m,units)}:std::nullopt;
            point.rain=hour.precipitation_probability;
            point.code=hour.weather_code;
         }
         points.push_back(std::move(point));
      }
      if(points.empty()) {
         Text(canvas,78,559,"Forecast not available",20);
         return;
      }

      double low=1000.0,high=-1000.0,rainMax=daily?0.1:100.0;
      for(auto const& point:points) {
         if(point.high) {
            low=(std::min)(low,*point.high);
            high=(std::max)(high,*point.high);
         }
         if(point.low) {
            low=(std::min)(low,*point.low);
            high=(std::max)(high,*point.low);
         }
         if(daily&&point.rain) rainMax=(std::max)(rainMax,*point.rain);
      }
      if(high<low) {
         low=0;high=25;
      }
      if(high-low<4.0) {
         double const mid=(high+low)*.5;
         low=mid-2;high=mid+2;
      }
      auto const xAt=[&](std::size_t i)->float {
         return 94.f+(points.size()==1?520.f:
            static_cast<float>(i)*1005.f/static_cast<float>(points.size()-1));
      };
      auto const yAt=[&](double value)->float {
         return 606.f-static_cast<float>((value-low)/(high-low))*27.f;
      };
      for(float y:{580.f,594.f,608.f}) {
         Color(paint,0xff3a5360);
         paint.setStrokeWidth(1);
         canvas->drawLine(78,y,1126,y,paint);
      }
      auto const trace=[&](bool lower,SkColor color) {
         Color(paint,color);
         paint.setStrokeWidth(3);
         std::optional<std::pair<float,float>> prior;
         for(std::size_t i=0;i<points.size();++i) {
            auto const& value=lower?points[i].low:points[i].high;
            if(!value) {
               prior.reset();
               continue;
            }
            auto const pos=std::pair{xAt(i),yAt(*value)};
            if(prior) {
               canvas->drawLine(prior->first,prior->second,pos.first,pos.second,paint);
            }
            canvas->drawCircle(pos.first,pos.second,4,paint);
            prior=pos;
         }
      };
      for(std::size_t i=0;i<points.size();++i) {
         float const x=xAt(i);
         font::Draw(canvas,x-30,510,points[i].label,14,0xffc9d9df,91);
         DrawSkySymbol(canvas,daily?x+37:x-7,daily?511.f:532.f,
            points[i].code.value_or(-1),points[i].daylight);
         if(points[i].rain) {
            float const portion=static_cast<float>(
               std::clamp(*points[i].rain/rainMax,0.0,1.0));
            Color(paint,0xff4b9eb8);
            canvas->drawRect(SkRect::MakeXYWH(x-17,636.f-portion*19.f,34,
               portion*19.f),paint);
         }
         font::Draw(canvas,x-27,652,points[i].rain?
            (daily?(units==UnitSystem::US?
               std::format("{:.2f} in",*points[i].rain):
               std::format("{:.1f} mm",*points[i].rain)):
               std::format("{:.0f} %",*points[i].rain))
            :"--",13,0xff9ccfda,96);
         if(daily && !points[i].sunrise.empty() && !points[i].sunset.empty()) {
            // Sunrise and sunset for each daily column below its caption.
            DrawSolarEvent(canvas,x-39,527,true,points[i].sunrise);
            DrawSolarEvent(canvas,x-39,550,false,points[i].sunset);
         }
         if(points[i].high) {
            std::string const label=daily && points[i].low
               ? std::format("{:.0f}/{:.0f} {}",*points[i].low,*points[i].high,TempUnit(units))
               : std::format("{:.0f} {}",*points[i].high,TempUnit(units));
            font::Draw(canvas,x-28,566,label,14,0xffe8ca89,105);
         }
      }
      trace(false,0xffeabb70);
      if(daily) trace(true,0xff86bae0);
   }
} // namespace weather_client::view
