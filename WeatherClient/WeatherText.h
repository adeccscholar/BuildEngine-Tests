// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherText.h
    \brief Deterministic, original vector-pixel lettering drawn by Skia.
    \details Unlike default SkFont, it has no dependency on a platform font
    manager. It is intentionally limited to readable ASCII; non-ASCII UTF-8
    bytes are replaced by a visible placeholder rather than disappearing.
*/
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <string_view>

namespace weather_client::lettering {
   using Glyph = std::array<std::uint8_t,7>;

   [[nodiscard]] inline Glyph GlyphFor(unsigned char ch) {
      if(ch >= 'a' && ch <= 'z') ch = static_cast<unsigned char>(ch-'a'+'A');
      switch(ch) {
         case 'A': return {14,17,17,31,17,17,17};
         case 'B': return {30,17,17,30,17,17,30};
         case 'C': return {15,16,16,16,16,16,15};
         case 'D': return {30,17,17,17,17,17,30};
         case 'E': return {31,16,16,30,16,16,31};
         case 'F': return {31,16,16,30,16,16,16};
         case 'G': return {15,16,16,23,17,17,15};
         case 'H': return {17,17,17,31,17,17,17};
         case 'I': return {31,4,4,4,4,4,31};
         case 'J': return {7,2,2,2,18,18,12};
         case 'K': return {17,18,20,24,20,18,17};
         case 'L': return {16,16,16,16,16,16,31};
         case 'M': return {17,27,21,21,17,17,17};
         case 'N': return {17,25,21,19,17,17,17};
         case 'O': return {14,17,17,17,17,17,14};
         case 'P': return {30,17,17,30,16,16,16};
         case 'Q': return {14,17,17,17,21,18,13};
         case 'R': return {30,17,17,30,20,18,17};
         case 'S': return {15,16,16,14,1,1,30};
         case 'T': return {31,4,4,4,4,4,4};
         case 'U': return {17,17,17,17,17,17,14};
         case 'V': return {17,17,17,17,17,10,4};
         case 'W': return {17,17,17,21,21,21,10};
         case 'X': return {17,17,10,4,10,17,17};
         case 'Y': return {17,17,10,4,4,4,4};
         case 'Z': return {31,1,2,4,8,16,31};
         case '0': return {14,17,19,21,25,17,14};
         case '1': return {4,12,4,4,4,4,14};
         case '2': return {14,17,1,2,4,8,31};
         case '3': return {30,1,1,14,1,1,30};
         case '4': return {2,6,10,18,31,2,2};
         case '5': return {31,16,16,30,1,1,30};
         case '6': return {14,16,16,30,17,17,14};
         case '7': return {31,1,2,4,8,8,8};
         case '8': return {14,17,17,14,17,17,14};
         case '9': return {14,17,17,15,1,1,14};
         case '.': return {0,0,0,0,0,12,12};
         case ',': return {0,0,0,0,0,12,8};
         case ':': return {0,12,12,0,12,12,0};
         case ';': return {0,12,12,0,12,8,0};
         case '-': return {0,0,0,31,0,0,0};
         case '_': return {0,0,0,0,0,0,31};
         case '/': return {1,1,2,4,8,16,16};
         case '\\': return {16,16,8,4,2,1,1};
         case '+': return {0,4,4,31,4,4,0};
         case '=': return {0,0,31,0,31,0,0};
         case '%': return {17,2,4,8,17,0,0};
         case '(': return {2,4,8,8,8,4,2};
         case ')': return {8,4,2,2,2,4,8};
         case '[': return {14,8,8,8,8,8,14};
         case ']': return {14,2,2,2,2,2,14};
         case '<': return {2,4,8,16,8,4,2};
         case '>': return {8,4,2,1,2,4,8};
         case '!': return {4,4,4,4,4,0,4};
         case '?': return {14,17,1,2,4,0,4};
         case '#': return {10,31,10,10,31,10,0};
         case '&': return {12,18,20,8,21,18,13};
         case '\'': return {4,4,8,0,0,0,0};
         case '"': return {10,10,0,0,0,0,0};
         case '*': return {0,21,14,31,14,21,0};
         case '@': return {14,17,23,21,23,16,14};
         case ' ': return {0,0,0,0,0,0,0};
         default: return {31,17,1,6,4,0,4};
      }
   }

   inline void Draw(SkCanvas* canvas,float x,float baseline,
      std::string_view text,float size,SkColor color) {
      if(size<=0) return;
      float const unit=size/8.5f;
      float const top=baseline-7.5f*unit;
      SkPaint paint;
      paint.setAntiAlias(false);
      paint.setColor(color);
      float at=x;
      for(unsigned char ch:text) {
         Glyph const glyph=GlyphFor(ch);
         for(unsigned row=0;row<7;++row) {
            for(unsigned col=0;col<5;++col) {
               if(glyph[row]&(1U<<(4U-col))) {
                  canvas->drawRect(SkRect::MakeXYWH(
                     at+static_cast<float>(col)*unit,
                     top+static_cast<float>(row)*unit,
                     unit*.88f,unit*.88f),paint);
               }
            }
         }
         at+=6.f*unit;
      }
   }
} // namespace weather_client::lettering
