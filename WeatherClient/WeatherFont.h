// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherFont.h
    \brief Measured Skia text using the Segoe UI family from PDFium/SDL2.
    \details The user's installed system font is only referenced at runtime,
    never copied or redistributed. The original vector lettering remains the
    safe fallback when no usable SkTypeface can be loaded.
*/
#include "WeatherText.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkFontStyle.h"
#if defined(SK_BUILD_FOR_WIN)
#include "include/ports/SkTypeface_win.h"
#endif

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

namespace weather_client::font {
   [[nodiscard]] inline sk_sp<SkTypeface> UiTypeface() {
      static sk_sp<SkTypeface> const loaded=[] {
         // Skia 153 no longer provides SkFontMgr::RefDefault().
         // BuildEngine Skia enables DirectWrite; use its public factory.
#if defined(SK_BUILD_FOR_WIN)
         auto mgr=SkFontMgr_New_DirectWrite();
#else
         auto mgr=SkFontMgr::RefEmpty();
#endif
         if(mgr) {
            if(auto face=mgr->matchFamilyStyle("Segoe UI",SkFontStyle()))
               return face;
         }
#ifdef _WIN32
         // Same Segoe UI family as the PDFium SDL2 toolbar example, without
         // using the Win32 GDI text renderer or distributing Microsoft's font.
         if(char const* system=std::getenv("WINDIR");system && *system && mgr) {
            auto const path=std::filesystem::path{system}/"Fonts"/"segoeui.ttf";
            if(std::filesystem::is_regular_file(path)) {
               auto const utf8=path.u8string();
               std::string file{utf8.begin(),utf8.end()};
               if(auto face=mgr->makeFromFile(file.c_str())) return face;
            }
         }
#endif
         return sk_sp<SkTypeface>{};
      }();
      return loaded;
   }

   [[nodiscard]] inline SkFont Make(float size) {
      SkFont font{UiTypeface(),size};
      font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
      font.setSubpixel(true);
      font.setHinting(SkFontHinting::kNormal);
      return font;
   }

   [[nodiscard]] inline float Width(std::string_view text,float size) {
      if(text.empty()) return 0.f;
      if(!UiTypeface()) return text.size()*size*6.f/8.5f;
      auto font=Make(size);
      return font.measureText(text.data(),text.size(),SkTextEncoding::kUTF8);
   }

   [[nodiscard]] inline std::string Fit(std::string_view text,float size,float maxWidth) {
      if(maxWidth<=0) return {};
      if(Width(text,size)<=maxWidth) return std::string{text};
      std::string const dots="...";
      if(Width(dots,size)>maxWidth) return {};
      std::size_t good=0;
      for(std::size_t i=0;i<text.size();) {
         std::size_t next=i+1;
         while(next<text.size() &&
            (static_cast<unsigned char>(text[next])&0xc0U)==0x80U) ++next;
         if(Width(std::string{text.substr(0,next)}+dots,size)>maxWidth) break;
         good=next;
         i=next;
      }
      return std::string{text.substr(0,good)}+dots;
   }

   inline void Draw(SkCanvas* canvas,float x,float y,std::string_view text,
      float size,SkColor color,float maxWidth=0.f) {
      if(!canvas||text.empty()||size<=0) return;
      std::string const fitted=maxWidth>0?Fit(text,size,maxWidth):std::string{text};
      if(fitted.empty()) return;
      if(!UiTypeface()) {
         lettering::Draw(canvas,x,y,fitted,size,color);
         return;
      }
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setColor(color);
      auto font=Make(size);
      canvas->drawSimpleText(fitted.data(),fitted.size(),SkTextEncoding::kUTF8,
         x,y,font,paint);
   }
} // namespace weather_client::font
