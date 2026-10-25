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
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setShader(SkShaders::LinearGradient(points,
         {{colors,nullptr,SkTileMode::kClamp},{}}));
      canvas->drawRoundRect(rect,radius,radius,paint);
   }

   inline void Glow(SkCanvas* canvas,float x,float y,float radius,SkColor center) {
      SkColor4f const colors[]{
         SkColor4f::FromColor(center),
         SkColor4f::FromColor(SkColorSetA(center,0))
      };
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setShader(SkShaders::RadialGradient(SkPoint::Make(x,y),radius,
         {{colors,nullptr,SkTileMode::kClamp},{}}));
      canvas->drawCircle(x,y,radius,paint);
   }

   inline void Cloud(SkCanvas* canvas,float x,float y,float scale,SkColor tint) {
      SkPaint paint;view::Color(paint,tint);
      canvas->drawOval(SkRect::MakeXYWH(x,y+15*scale,125*scale,38*scale),paint);
      canvas->drawCircle(x+31*scale,y+20*scale,28*scale,paint);
      canvas->drawCircle(x+64*scale,y+3*scale,36*scale,paint);
      canvas->drawCircle(x+99*scale,y+21*scale,25*scale,paint);
   }

   inline void Atmosphere(SkCanvas* canvas,int code,bool daytime,float phase) {
      // All art is synthesized from paths and gradients. No textures/assets.
      GradientRect(canvas,SkRect::MakeXYWH(30,24,1140,160),
         daytime?0xff164e72:0xff1b254b,
         daytime?0xff275c71:0xff1e3c62,21);
      Glow(canvas,990+std::sin(phase*.12f)*10,80,125,
         daytime?0x8abed9ee:0x678aa8ed);
      SkPaint paint;
      view::Color(paint,daytime?0xfff7d18a:0xffd9e2f7);
      canvas->drawCircle(1000,85,daytime?28:23,paint);
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
         Cloud(canvas,575+i*120.f+offset,60+(i%2)*27.f,
            i%2==0?.57f:.42f,0x8addf0f5);
      }
      if(code>=51&&code<=95) {
         view::Color(paint,0x927ac9ea);paint.setStrokeWidth(2);
         for(int i=0;i<16;++i) {
            float const x=640.f+(i*41)%465;
            float const y=119.f+(i*23)%35;
            canvas->drawLine(x,y,x-7,y+14,paint);
         }
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
      paint.setShader(SkShaders::LinearGradient(
         {SkPoint::Make(cx-radius,cy-radius),SkPoint::Make(cx+radius,cy+radius)},
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
         view::Text(canvas,cx-radius*.41f,cy+radius*.47f,
            std::format("{:.1f} {}",*value,unit),radius<100?13.f:19.f,
            0xffebf6f9);
      } else {
         view::Text(canvas,cx-12,cy+radius*.47f,"--",18);
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
