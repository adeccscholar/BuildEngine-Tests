// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherGdiFont.h
    \brief Segoe UI rasterization through GDI, composed by Skia.
    \details Same CreateFontW font path as the working PDFium/SDL2 example.
    The raster is a white-on-black grayscale coverage mask, not LCD ClearType.
    It is tinted in premultiplied Skia pixels and never uses DirectWrite.
*/
#ifdef _WIN32
#include <windows.h>
#include "include/core/SkBitmap.h"
#include "include/core/SkImage.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSamplingOptions.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <string>
#include <string_view>
#include <utility>

namespace weather_client::gdi_font {
   [[nodiscard]] inline std::wstring Utf16(std::string_view text) {
      if(text.empty()) return {};
      int const count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,
         text.data(),static_cast<int>(text.size()),nullptr,0);
      if(count<=0) return {};
      std::wstring result(static_cast<std::size_t>(count),L'\0');
      if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,
         text.data(),static_cast<int>(text.size()),result.data(),count)!=count)
         return {};
      return result;
   }

   struct GlyphImage {
      SkBitmap pixels;
      float width{};
      float top{};
   };

   class Renderer {
      std::map<std::tuple<std::wstring,int,SkColor>,GlyphImage> cache_;
      HDC dc_{};
      HBITMAP bitmap_{};
      HGDIOBJ originalBitmap_{};
      void* raw_{};
      HFONT selectedFont_{};
      HGDIOBJ originalFont_{};
      int bitmapWidth_{};
      int bitmapHeight_{};

      [[nodiscard]] bool Prepare(int width,int height,int px) {
         if(!dc_) {
            dc_=CreateCompatibleDC(nullptr);
            if(!dc_) return false;
         }
         if(width>bitmapWidth_||height>bitmapHeight_) {
            if(bitmap_) {
               SelectObject(dc_,originalBitmap_);
               DeleteObject(bitmap_);
               bitmap_=nullptr;
               raw_=nullptr;
            }
            bitmapWidth_=(std::max)(width,bitmapWidth_);
            bitmapHeight_=(std::max)(height,bitmapHeight_);
            BITMAPINFO info{};
            info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth=bitmapWidth_;
            info.bmiHeader.biHeight=-bitmapHeight_;
            info.bmiHeader.biPlanes=1;
            info.bmiHeader.biBitCount=32;
            info.bmiHeader.biCompression=BI_RGB;
            bitmap_=CreateDIBSection(dc_,&info,DIB_RGB_COLORS,&raw_,nullptr,0);
            if(!bitmap_||!raw_) return false;
            originalBitmap_=SelectObject(dc_,bitmap_);
         }
         if(selectedFont_) {
            SelectObject(dc_,originalFont_);
            DeleteObject(selectedFont_);
         }
         selectedFont_=CreateFontW(-px,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
         if(!selectedFont_) return false;
         originalFont_=SelectObject(dc_,selectedFont_);
         SetBkMode(dc_,OPAQUE);
         SetBkColor(dc_,RGB(0,0,0));
         SetTextColor(dc_,RGB(255,255,255));
         return true;
      }

   public:
      Renderer()=default;
      Renderer(Renderer const&)=delete;
      Renderer& operator=(Renderer const&)=delete;
      ~Renderer() {
         if(dc_) {
            if(selectedFont_) {
               SelectObject(dc_,originalFont_);
               DeleteObject(selectedFont_);
            }
            if(bitmap_) {
               SelectObject(dc_,originalBitmap_);
               DeleteObject(bitmap_);
            }
            DeleteDC(dc_);
         }
      }

      [[nodiscard]] GlyphImage const* Raster(std::wstring const& text,int px,SkColor color) {
         if(text.empty()||px<=0) return nullptr;
         auto const key=std::tuple{text,px,color};
         if(auto it=cache_.find(key);it!=cache_.end()) return &it->second;
         if(cache_.size()>192) cache_.clear();
         if(!Prepare(16,16,px)) return nullptr;
         SIZE measured{};
         if(!GetTextExtentPoint32W(dc_,text.c_str(),static_cast<int>(text.size()),&measured))
            return nullptr;
         TEXTMETRICW metrics{};
         if(!GetTextMetricsW(dc_,&metrics)) return nullptr;
         int const width=(std::max)(1,measured.cx+4);
         int const height=(std::max)(1,measured.cy+4);
         if(width>8192||height>256||!Prepare(width,height,px)) return nullptr;
         RECT area{0,0,width,height};
         FillRect(dc_,&area,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
         if(!TextOutW(dc_,2,2,text.c_str(),static_cast<int>(text.size()))) return nullptr;
         GdiFlush();
         GlyphImage glyph;
         glyph.width=static_cast<float>(measured.cx);
         glyph.top=static_cast<float>(metrics.tmAscent+2);
         glyph.pixels.allocPixels(SkImageInfo::MakeN32Premul(width,height));
         if(!glyph.pixels.getPixels()) return nullptr;
         auto* src=static_cast<unsigned char const*>(raw_);
         for(int row=0;row<height;++row) {
            auto* dst=glyph.pixels.getAddr32(0,row);
            for(int col=0;col<width;++col) {
               auto const pixel=src+4*(static_cast<std::size_t>(row)*bitmapWidth_+col);
               unsigned const a=pixel[0];
               dst[col]=SkPreMultiplyARGB(a,SkColorGetR(color),
                  SkColorGetG(color),SkColorGetB(color));
            }
         }
         return &cache_.emplace(key,std::move(glyph)).first->second;
      }

      [[nodiscard]] float Width(std::wstring const& text,int size) {
         if(auto glyph=Raster(text,size,SK_ColorWHITE)) return glyph->width;
         return static_cast<float>(text.size()*size)*.6f;
      }

      [[nodiscard]] bool Draw(SkCanvas* canvas,float x,float baseline,
         std::wstring const& text,int size,SkColor color) {
         auto glyph=Raster(text,size,color);
         if(!glyph) return false;
         auto image=SkImages::RasterFromBitmap(glyph->pixels);
         if(!image) return false;
         SkPaint paint;
         paint.setColor(SK_ColorWHITE);
         paint.setAntiAlias(true);
         canvas->drawImage(image,x-2,baseline-glyph->top,SkSamplingOptions(),&paint);
         return true;
      }
   };

   [[nodiscard]] inline Renderer& Instance() {
      static Renderer renderer;
      return renderer;
   }
}
#endif
