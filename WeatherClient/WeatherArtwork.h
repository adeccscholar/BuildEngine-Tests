// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherArtwork.h
    \brief Original procedural Skia artwork, without external artwork assets.
    \details Color gradients, translucent layers, procedural cloud shapes,
    radar-like gauge markings and animated highlights are drawn on a raster
    SkCanvas. All positioning is independent from the windowing backend.
*/
#include "WeatherView.h"
#include "WeatherMoon.h"
#include "include/core/SkPathBuilder.h"
#include "include/effects/SkGradient.h"

#include <array>
#include <cmath>
#include <format>
#include <optional>
#include <string>

namespace weather_client::artwork {
   constexpr float Pi=3.14159265358979323846f;

   inline void GradientRect(SkCanvas* canvas,SkRect rect,SkColor first,
      SkColor second,float radius=0) {
      SkColor4f const colors[]{
         SkColor4f::FromColor(first),SkColor4f::FromColor(second)
      };
      SkPoint const points[]{SkPoint::Make(rect.left(),rect.top()),
         SkPoint::Make(rect.right(),rect.bottom())};
      SkScalar const stops[]{0.f,1.f};
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setShader(SkShaders::LinearGradient(points,
         {{colors,stops,SkTileMode::kClamp},{}}));
      canvas->drawRoundRect(rect,radius,radius,paint);
   }

   inline void Glow(SkCanvas* canvas,float x,float y,float radius,SkColor center) {
      SkColor4f const colors[]{
         SkColor4f::FromColor(center),
         SkColor4f::FromColor(SkColorSetA(center,0))
      };
      SkScalar const stops[]{0.f,1.f};
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setShader(SkShaders::RadialGradient(SkPoint::Make(x,y),radius,
         {{colors,stops,SkTileMode::kClamp},{}}));
      canvas->drawCircle(x,y,radius,paint);
   }

   inline void Cloud(SkCanvas* canvas,float x,float y,float scale,SkColor tint) {
      SkPaint paint;view::Color(paint,tint);
      canvas->drawOval(SkRect::MakeXYWH(x,y+15*scale,125*scale,38*scale),paint);
      canvas->drawCircle(x+31*scale,y+20*scale,28*scale,paint);
      canvas->drawCircle(x+64*scale,y+3*scale,36*scale,paint);
      canvas->drawCircle(x+99*scale,y+21*scale,25*scale,paint);
   }

   inline void Moon(SkCanvas* canvas,float x,float y,float radius,
      MoonPhase const& phase) {
      Glow(canvas,x,y,radius*2.4f,0x458daadd);
      SkPaint paint;
      view::Color(paint,0xff142c41);
      canvas->drawCircle(x,y,radius,paint);
      // Illuminated portion of a sphere, with the terminator adjusted for
      // both waxing/waning orientation and the computed illuminated fraction.
      view::Color(paint,0xffe8edf6);
      float const cycle=static_cast<float>(phase.cycle);
      for(int row=0;row<80;++row) {
         float const yy=-radius+radius*2.f*(row+.5f)/80.f;
         float const half=std::sqrt((std::max)(0.f,radius*radius-yy*yy));
         float const terminator=std::cos(2.f*Pi*cycle)*half;
         float const left=phase.waxing?terminator:-half;
         float const right=phase.waxing?half:-terminator;
         if(right>left) {
            canvas->drawRect(SkRect::MakeXYWH(
               x+left,y+yy-radius/80.f,right-left,radius*2.f/80.f+1.f),paint);
         }
      }
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1.3f);
      paint.setColor(0x95c2d2e0);
      canvas->drawCircle(x,y,radius,paint);
   }

   inline void SunriseSunset(SkCanvas* canvas,float x,float y,
      bool sunrise,float radius=9.f) {
      SkPaint p;
      view::Color(p,0xfff3c47a);
      canvas->drawCircle(x,y,radius,p);
      p.setColor(0xff213e51);
      canvas->drawRect(SkRect::MakeXYWH(x-radius-5,y+2,2*radius+10,7),p);
      p.setColor(0xffeccf9f);
      p.setStrokeWidth(1.8f);
      canvas->drawLine(x-radius-8,y+2,x+radius+8,y+2,p);
      float const dir=sunrise?-1.f:1.f;
      canvas->drawLine(x,y-15*dir,x,y-7*dir,p);
   }

   inline void Atmosphere(SkCanvas* canvas,int code,bool daytime,float phase,
      std::optional<MoonPhase> const& moon=std::nullopt) {
      // All art is synthesized from paths and gradients. No textures/assets.
      GradientRect(canvas,SkRect::MakeXYWH(30,24,1140,160),
         daytime?0xff164e72:0xff1b254b,
         daytime?0xff275c71:0xff1e3c62,21);
      Glow(canvas,990+std::sin(phase*.12f)*10,80,125,
         daytime?0x8abed9ee:0x678aa8ed);
      SkPaint paint;
      view::Color(paint,daytime?0xfff7d18a:0xffd9e2f7);
      if(daytime) canvas->drawCircle(1000,85,28,paint);
      else Moon(canvas,1000,85,28,moon.value_or(MoonPhase{.cycle=.25,.illumination=.5,
         .stage=MoonStage::FirstQuarter,.waxing=true}));
      if(!daytime) {
         for(int i=0;i<31;++i) {
            float const x=62.f+static_cast<float>((i*191)%1075);
            float const y=35.f+static_cast<float>((i*47)%117);
            paint.setColor(SkColorSetARGB(115+(i%4)*22,224,240,255));
            canvas->drawCircle(x,y,i%3==0?1.8f:1.f,paint);
         }
      }
      int const cloudiness=code==0?0:code==1?1:code==2?2:3;
      for(int i=0;i<cloudiness;++i) {
         float const offset=std::sin(phase*.16f+i*1.9f)*14.f;
         Cloud(canvas,920+i*61.f+offset,60+(i%2)*24.f,
            i%2==0?.44f:.35f,0x8addf0f5);
      }
      if(code>=51&&code<=99) {
         view::Color(paint,0x927ac9ea);paint.setStrokeWidth(2);
         for(int i=0;i<16;++i) {
            float const x=930.f+(i*37)%193;
            float const y=120.f+(i*19)%27;
            canvas->drawLine(x,y,x-7,y+14,paint);
         }
      }
      if(code==95||code==96||code==97||code==99) {
         SkPathBuilder lightning;
         lightning.moveTo(1020,104).lineTo(995,141).lineTo(1011,139)
            .lineTo(1000,170).lineTo(1043,121).lineTo(1024,126).close();
         view::Color(paint,0xffffd072);
         canvas->drawPath(lightning.detach(),paint);
      }
      if(code>=71&&code<=86 && !(code>=80&&code<=82)) {
         view::Color(paint,0xffdbecff);
         for(int i=0;i<17;++i) {
            float const x=940.f+static_cast<float>((i*53)%179);
            float const y=113.f+static_cast<float>((i*23)%52);
            canvas->drawCircle(x,y,2.f,paint);
         }
      }
      if(code==45||code==48) {
         paint.setStyle(SkPaint::kStroke_Style);
         paint.setStrokeWidth(5.f);
         paint.setColor(0x9ab1c9d3);
         for(int i=0;i<3;++i)
            canvas->drawLine(925.f,115.f+i*15.f,1122.f,115.f+i*15.f,paint);
      }
      // Contour lines make the translucent atmosphere look instrument-made.
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1.f);
      paint.setColor(0x355bb3db);
      for(int i=0;i<4;++i) {
         SkPathBuilder wave;
         float y=123.f+i*15.f;
         wave.moveTo(33,y);
         wave.cubicTo(238,y-42,371,y+37,550,y-12);
         wave.cubicTo(740,y-44,966,y+31,1168,y-13);
         canvas->drawPath(wave.detach(),paint);
      }
   }

   inline void Dial(SkCanvas* canvas,float cx,float cy,float radius,
      std::string const& title,std::optional<double> value,
      float minimum,float maximum,std::string const& unit,SkColor accent,
      float phase=0) {
      SkPaint paint;
      Glow(canvas,cx,cy,radius+27,SkColorSetA(accent,38));
      view::Color(paint,0xff081824);
      canvas->drawCircle(cx+3,cy+5,radius+10,paint);
      const SkColor4f rim[]{
         SkColor4f::FromColor(0xffacbfc7),
         SkColor4f::FromColor(0xff3e5765),
         SkColor4f::FromColor(0xffa0bdc6)
      };
      SkScalar const stops[]{0,0.53f,1};
      SkPoint const rimPoints[]{
         SkPoint::Make(cx-radius,cy-radius),
         SkPoint::Make(cx+radius,cy+radius)
      };
      paint.setShader(SkShaders::LinearGradient(rimPoints,
         {{rim,stops,SkTileMode::kClamp},{}}));
      canvas->drawCircle(cx,cy,radius+7,paint);
      paint.setShader(nullptr);
      view::Color(paint,0xff152a37);
      canvas->drawCircle(cx,cy,radius+1,paint);
      Glow(canvas,cx-radius*.29f,cy-radius*.37f,radius*.78f,0x274c7d95);
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeCap(SkPaint::kRound_Cap);
      paint.setStrokeWidth(radius<100?2.5f:4.5f);
      paint.setColor(0x50395362);
      canvas->drawArc(SkRect::MakeLTRB(cx-radius*.78f,cy-radius*.78f,
         cx+radius*.78f,cy+radius*.78f),135,270,false,paint);
      paint.setColor(accent);
      double const ratio=value?std::clamp(
         (*value-minimum)/(maximum-minimum),0.0,1.0):0.0;
      if(value) {
         canvas->drawArc(SkRect::MakeLTRB(cx-radius*.78f,cy-radius*.78f,
            cx+radius*.78f,cy+radius*.78f),135,static_cast<float>(ratio)*270.f,
            false,paint);
      }
      for(int i=0;i<=40;++i) {
         float const angle=(135.f+i*270.f/40.f)*Pi/180.f;
         float const inner=radius*(i%5==0?.67f:.72f);
         float const outer=radius*.82f;
         paint.setShader(nullptr);
         paint.setColor(i%5==0?0xffc1d8df:0xff66828d);
         paint.setStrokeWidth(i%5==0?2.f:1.f);
         canvas->drawLine(cx+std::cos(angle)*inner,cy+std::sin(angle)*inner,
            cx+std::cos(angle)*outer,cy+std::sin(angle)*outer,paint);
      }
      view::Text(canvas,cx-radius*.45f,cy-radius*.39f,title,
         radius<100?11.f:14.f,0xffa9c7d0);
      if(value) {
         float const a=(135.f+static_cast<float>(ratio)*270.f)*Pi/180.f;
         paint.setShader(nullptr);
         paint.setColor(0xff06121b);
         paint.setStrokeWidth(6.f);
         canvas->drawLine(cx+2,cy+3,cx+std::cos(a)*radius*.64f+2,
            cy+std::sin(a)*radius*.64f+3,paint);
         paint.setColor(0xffffd387);
         paint.setStrokeWidth(3.f);
         canvas->drawLine(cx,cy,cx+std::cos(a)*radius*.64f,
            cy+std::sin(a)*radius*.64f,paint);
         // Keep the verified dial rendering; only pressure formatting differs.
         std::string const valueText=unit=="inHg"
            ? std::format("{:.2f} {}",*value,unit)
            : std::format("{:.1f} {}",*value,unit);
         if(title=="TEMPERATURE") {
            // The needle and scale must never obscure the live reading.
            // Keep this independent of Skia font metrics/centering calls:
            // the pre-redesign dial is already verified with Segoe UI.
            SkPaint plate;
            view::Color(plate,0xe6091d2b);
            canvas->drawRoundRect(SkRect::MakeXYWH(
               cx-radius*.70f,cy+radius*.20f,
               radius*1.40f,radius*.33f),7.f,7.f,plate);
            view::Text(canvas,cx-radius*.55f,cy+radius*.44f,
               valueText,21.f,0xffffe4b0);
         } else {
            view::Text(canvas,cx-radius*.41f,cy+radius*.47f,
               valueText,radius<100?13.f:19.f,0xffebf6f9);
         }
      } else {
         if(title=="TEMPERATURE") {
            view::Text(canvas,cx-radius*.39f,cy+radius*.47f,
               "NO DATA",16,0xffffc1a8);
         } else {
            view::Text(canvas,cx-12,cy+radius*.47f,"--",18);
         }
      }
      view::Color(paint,0xfff0d3a1);
      canvas->drawCircle(cx,cy,5.f,paint);
      // Small animated light glint follows the clock, not weather data.
      Glow(canvas,cx-radius*.28f+std::sin(phase)*2,
         cy-radius*.40f,radius*.35f,0x0ff9fbff);
   }

   inline void WindRose(SkCanvas* canvas,float cx,float cy,float radius,
      std::optional<double> direction,float phase=0) {
      SkPaint paint;
      view::Color(paint,0xff091a26);
      canvas->drawCircle(cx,cy,radius+7,paint);
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(2);
      paint.setColor(0xff7397a8);
      canvas->drawCircle(cx,cy,radius,paint);
      for(int i=0;i<8;++i) {
         float const angle=static_cast<float>(i)*Pi/4.f-Pi/2.f;
         paint.setColor(0xff617e8b);
         canvas->drawLine(cx+std::cos(angle)*radius*.76f,
            cy+std::sin(angle)*radius*.76f,
            cx+std::cos(angle)*radius*.91f,
            cy+std::sin(angle)*radius*.91f,paint);
      }
      view::Text(canvas,cx-5,cy-radius*.68f,"N",12,0xffdbe6ea);
      view::Text(canvas,cx+radius*.70f,cy+4,"E",12,0xffdbe6ea);
      view::Text(canvas,cx-5,cy+radius*.81f,"S",12,0xffdbe6ea);
      view::Text(canvas,cx-radius*.84f,cy+4,"W",12,0xffdbe6ea);
      if(direction) {
         float a=(static_cast<float>(*direction)-90.f)*Pi/180.f;
         SkPathBuilder arrow;
         arrow.moveTo(cx+std::cos(a)*radius*.62f,
            cy+std::sin(a)*radius*.62f);
         arrow.lineTo(cx+std::cos(a+2.45f)*radius*.25f,
            cy+std::sin(a+2.45f)*radius*.25f);
         arrow.lineTo(cx+std::cos(a-2.45f)*radius*.25f,
            cy+std::sin(a-2.45f)*radius*.25f);
         arrow.close();
         paint.setStyle(SkPaint::kFill_Style);
         paint.setColor(0xffffcf8e);
         canvas->drawPath(arrow.detach(),paint);
      }
      Glow(canvas,cx,cy,radius*.27f,0x2584cfe8);
      view::Color(paint,0xfff2d8b3);
      canvas->drawCircle(cx,cy,3.5f,paint);
   }

} // namespace weather_client::artwork
