// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT
// SDL2 owns presentation/events. Windows GDI only rasterizes UI text/controls
// off-screen into BGRA textures. No VCL or additional UI library.
#pragma once
#include <SDL.h>
#include <windows.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace adecc::sdl_ui {

constexpr int ToolbarHeight = 66;
constexpr int StatusHeight = 30;
constexpr SDL_Rect OpenButton { 16, 14, 110, 38 };
constexpr SDL_Rect PreviousButton { 139, 14, 102, 38 };
constexpr SDL_Rect PageField { 254, 14, 80, 38 };
constexpr SDL_Rect GoButton { 346, 14, 59, 38 };
constexpr SDL_Rect NextButton { 418, 14, 94, 38 };

// Zoom controls follow the right edge as the SDL window is resized.
[[nodiscard]] inline SDL_Rect ZoomOutButton(int width) noexcept {
   return SDL_Rect { width - 262, 14, 41, 38 };
}
[[nodiscard]] inline SDL_Rect ZoomFitButton(int width) noexcept {
   return SDL_Rect { width - 211, 14, 65, 38 };
}
[[nodiscard]] inline SDL_Rect ZoomInButton(int width) noexcept {
   return SDL_Rect { width - 136, 14, 41, 38 };
}

enum class Hot : unsigned char {
   none, open, previous, page, go, next, zoomOut, zoomFit, zoomIn
};

struct State {
   bool hasDocument {};
   bool editing {};
   int pageIndex {};
   int pageCount {};
   Hot hovered { Hot::none };
   std::string pageEntry;
   int zoomPercent { 100 };
};

struct TextureCloser {
   void operator()(SDL_Texture* ptr) const noexcept {
      if(ptr) SDL_DestroyTexture(ptr);
   }
};
using TexturePtr = std::unique_ptr<SDL_Texture, TextureCloser>;

class Surface final {
   HDC dc_ {};
   HBITMAP bmp_ {};
   HGDIOBJ previous_ {};
   void* data_ {};
   int width_ {};
   int height_ {};

public:
   Surface(int width, int height) : width_(width), height_(height) {
      if(width <= 0 || width > 16384 || height <= 0)
         throw std::runtime_error("Invalid toolbar surface dimensions");
      dc_ = CreateCompatibleDC(nullptr);
      if(!dc_) throw std::runtime_error("CreateCompatibleDC failed");
      BITMAPINFO info {};
      info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      info.bmiHeader.biWidth = width_;
      info.bmiHeader.biHeight = -height_; // top-down pixels
      info.bmiHeader.biPlanes = 1;
      info.bmiHeader.biBitCount = 32;
      info.bmiHeader.biCompression = BI_RGB;
      bmp_ = CreateDIBSection(dc_, &info, DIB_RGB_COLORS, &data_, nullptr, 0);
      if(!bmp_ || !data_) {
         DeleteDC(dc_);
         throw std::runtime_error("CreateDIBSection failed");
      }
      previous_ = SelectObject(dc_, bmp_);
      if(!previous_ || previous_ == HGDI_ERROR) {
         DeleteObject(bmp_);
         DeleteDC(dc_);
         throw std::runtime_error("SelectObject(DIB) failed");
      }
      SetBkMode(dc_, TRANSPARENT);
   }
   ~Surface() {
      if(previous_) SelectObject(dc_, previous_);
      if(bmp_) DeleteObject(bmp_);
      if(dc_) DeleteDC(dc_);
   }
   Surface(Surface const&) = delete;
   Surface& operator=(Surface const&) = delete;

   [[nodiscard]] HDC Dc() const noexcept { return dc_; }
   [[nodiscard]] TexturePtr Upload(SDL_Renderer* renderer) const {
      TexturePtr result { SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA32,
                                            SDL_TEXTUREACCESS_STATIC,
                                            width_, height_) };
      if(!result || SDL_UpdateTexture(result.get(), nullptr, data_, width_ * 4))
         throw std::runtime_error(std::string("SDL_UpdateTexture: ") + SDL_GetError());
      if(SDL_SetTextureBlendMode(result.get(), SDL_BLENDMODE_NONE))
         throw std::runtime_error(std::string("SDL_SetTextureBlendMode: ") + SDL_GetError());
      return result;
   }
};

class Font final {
   HDC dc_ {};
   HFONT font_ {};
   HGDIOBJ previous_ {};
public:
   Font(HDC dc, int px, int weight) : dc_(dc) {
      font_ = CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
      if(!font_) throw std::runtime_error("CreateFontW(Segoe UI) failed");
      previous_ = SelectObject(dc_, font_);
      if(!previous_ || previous_ == HGDI_ERROR) {
         DeleteObject(font_);
         throw std::runtime_error("SelectObject(font) failed");
      }
   }
   ~Font() {
      if(previous_) SelectObject(dc_, previous_);
      if(font_) DeleteObject(font_);
   }
   Font(Font const&) = delete;
   Font& operator=(Font const&) = delete;
};

inline void Fill(HDC dc, RECT const& rect, COLORREF color) {
   HBRUSH const brush = CreateSolidBrush(color);
   if(!brush) throw std::runtime_error("CreateSolidBrush failed");
   FillRect(dc, &rect, brush);
   DeleteObject(brush);
}

inline void Round(HDC dc, RECT const& rect, COLORREF background, COLORREF stroke) {
   HPEN const pen = CreatePen(PS_SOLID, 1, stroke);
   HBRUSH const brush = CreateSolidBrush(background);
   if(!pen || !brush) {
      if(pen) DeleteObject(pen);
      if(brush) DeleteObject(brush);
      throw std::runtime_error("CreatePen/CreateSolidBrush failed");
   }
   HGDIOBJ const priorPen = SelectObject(dc, pen);
   HGDIOBJ const priorBrush = SelectObject(dc, brush);
   RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
   SelectObject(dc, priorBrush);
   SelectObject(dc, priorPen);
   DeleteObject(brush);
   DeleteObject(pen);
}

[[nodiscard]] inline RECT ToRect(SDL_Rect const& rect) {
   return RECT { rect.x, rect.y, rect.x + rect.w, rect.y + rect.h };
}

inline void Label(HDC dc, RECT rect, std::wstring_view value,
                  COLORREF textColor, UINT alignment) {
   SetTextColor(dc, textColor);
   DrawTextW(dc, value.data(), static_cast<int>(value.size()), &rect,
             alignment | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

inline void DrawButton(HDC dc, SDL_Rect const& box, std::wstring_view title,
                       bool enabled, bool hover, bool primary = false) {
   RECT const rect = ToRect(box);
   RECT shadow = rect;
   OffsetRect(&shadow, 0, 1);
   Round(dc, shadow, RGB(232, 238, 244), RGB(232, 238, 244));
   COLORREF const background = !enabled ? RGB(243, 245, 248)
      : primary ? (hover ? RGB(17, 93, 183) : RGB(36, 110, 203))
      : hover ? RGB(237, 246, 255) : RGB(255, 255, 255);
   COLORREF const border = !enabled ? RGB(217, 225, 232)
      : primary ? (hover ? RGB(15, 79, 165) : RGB(32, 98, 182))
      : hover ? RGB(122, 164, 216) : RGB(201, 213, 227);
   Round(dc, rect, background, border);
   Label(dc, rect, title, !enabled ? RGB(157, 169, 181)
         : primary ? RGB(255, 255, 255) : RGB(44, 59, 76),
         DT_CENTER);
}

class Chrome final {
   TexturePtr toolbar_;
   TexturePtr status_;
   TexturePtr empty_;
   std::string toolbarKey_;
   std::string statusKey_;

   [[nodiscard]] static TexturePtr MakeToolbar(SDL_Renderer* renderer,
                                                int width, State const& state) {
      Surface canvas { width, ToolbarHeight };
      HDC const dc = canvas.Dc();
      Fill(dc, RECT { 0, 0, width, ToolbarHeight }, RGB(248, 250, 253));
      Fill(dc, RECT { 0, ToolbarHeight - 1, width, ToolbarHeight },
           RGB(215, 225, 237));
      Font const font { dc, 16, FW_MEDIUM };
      DrawButton(dc, OpenButton, L"Open PDF", true, state.hovered == Hot::open, true);
      DrawButton(dc, PreviousButton, L"Previous",
                 state.hasDocument && state.pageIndex > 0,
                 state.hovered == Hot::previous);
      DrawButton(dc, GoButton, L"Go", state.hasDocument, state.hovered == Hot::go);
      DrawButton(dc, NextButton, L"Next",
                 state.hasDocument && state.pageIndex + 1 < state.pageCount,
                 state.hovered == Hot::next);
      DrawButton(dc, ZoomOutButton(width), L"-",
                 state.hasDocument && state.zoomPercent > 50,
                 state.hovered == Hot::zoomOut);
      DrawButton(dc, ZoomFitButton(width), L"Fit",
                 state.hasDocument, state.hovered == Hot::zoomFit);
      DrawButton(dc, ZoomInButton(width), L"+",
                 state.hasDocument && state.zoomPercent < 600,
                 state.hovered == Hot::zoomIn);
      Label(dc, RECT { width - 89, 13, width - 7, 53 },
            std::to_wstring(state.zoomPercent) + L"%",
            state.hasDocument ? RGB(62, 79, 99) : RGB(154, 165, 179),
            DT_CENTER | DT_END_ELLIPSIS);

      RECT const field = ToRect(PageField);
      Round(dc, field, state.hasDocument ? RGB(255, 255, 255) : RGB(244, 246, 249),
            !state.hasDocument ? RGB(220, 226, 235)
            : state.editing ? RGB(36, 116, 213)
            : state.hovered == Hot::page ? RGB(122, 164, 216)
            : RGB(201, 213, 227));
      std::wstring value;
      if(state.editing) value.assign(state.pageEntry.begin(), state.pageEntry.end());
      else if(state.hasDocument) value = std::to_wstring(state.pageIndex + 1);
      else value = L"-";
      RECT textBox = field;
      textBox.left += 11;
      textBox.right -= 7;
      Label(dc, textBox, value, state.hasDocument ? RGB(40, 56, 77)
            : RGB(157, 169, 181), DT_LEFT | DT_END_ELLIPSIS);
      if(state.editing) {
         SIZE extent {};
         if(!value.empty())
            GetTextExtentPoint32W(dc, value.c_str(),
                                  static_cast<int>(value.size()), &extent);
         int const x = std::min(field.right - 10, field.left + 11 + extent.cx + 1);
         HPEN const pen = CreatePen(PS_SOLID, 1, RGB(38, 107, 198));
         HGDIOBJ const old = SelectObject(dc, pen);
         MoveToEx(dc, x, field.top + 10, nullptr);
         LineTo(dc, x, field.bottom - 9);
         SelectObject(dc, old);
         DeleteObject(pen);
      }

      std::wstring const count = state.hasDocument
         ? L"Page " + std::to_wstring(state.pageIndex + 1) +
           L" of " + std::to_wstring(state.pageCount)
         : L"No document open";
      Label(dc, RECT { 534, 13, width - 276, 53 }, count,
            RGB(78, 94, 114), DT_LEFT | DT_END_ELLIPSIS);
      return canvas.Upload(renderer);
   }

   [[nodiscard]] static TexturePtr MakeStatus(SDL_Renderer* renderer,
                                               int width, std::string const& message) {
      Surface canvas { width, StatusHeight };
      HDC const dc = canvas.Dc();
      Fill(dc, RECT { 0, 0, width, StatusHeight }, RGB(248, 250, 253));
      Fill(dc, RECT { 0, 0, width, 1 }, RGB(215, 225, 237));
      Font const font { dc, 13, FW_NORMAL };
      std::wstring const wide(message.begin(), message.end());
      Label(dc, RECT { 16, 2, width - 12, StatusHeight - 1 },
            wide, RGB(94, 110, 129), DT_LEFT | DT_END_ELLIPSIS);
      return canvas.Upload(renderer);
   }

public:
   void Draw(SDL_Renderer* renderer, int width, int height, State const& state,
             std::string const& message) {
      if(width < 1 || height < 1) return;
      std::string const key = std::to_string(width) + "|" +
         std::to_string(state.hasDocument) + "|" + std::to_string(state.editing) +
         "|" + std::to_string(state.pageIndex) + "|" +
         std::to_string(state.pageCount) + "|" +
         std::to_string(state.zoomPercent) + "|" +
         std::to_string(static_cast<int>(state.hovered)) + "|" + state.pageEntry;
      if(key != toolbarKey_ || !toolbar_) {
         toolbar_ = MakeToolbar(renderer, width, state);
         toolbarKey_ = key;
      }
      std::string const footer = std::to_string(width) + "|" + message;
      if(footer != statusKey_ || !status_) {
         status_ = MakeStatus(renderer, width, message);
         statusKey_ = footer;
      }
      SDL_Rect const top { 0, 0, width, ToolbarHeight };
      SDL_Rect const bottom { 0, height - StatusHeight, width, StatusHeight };
      SDL_RenderCopy(renderer, toolbar_.get(), nullptr, &top);
      SDL_RenderCopy(renderer, status_.get(), nullptr, &bottom);
   }

   void DrawEmpty(SDL_Renderer* renderer, int width, int height) {
      constexpr int boxWidth = 440;
      constexpr int boxHeight = 74;
      if(!empty_) {
         Surface canvas { boxWidth, boxHeight };
         HDC const dc = canvas.Dc();
         Fill(dc, RECT { 0, 0, boxWidth, boxHeight }, RGB(230, 235, 242));
         {
            Font const title { dc, 21, FW_SEMIBOLD };
            Label(dc, RECT { 0, 0, boxWidth, 40 },
                  L"Open a PDF document", RGB(64, 82, 105), DT_CENTER);
         }
         {
            Font const hint { dc, 14, FW_NORMAL };
            Label(dc, RECT { 0, 39, boxWidth, 73 },
                  L"Use Open PDF or Ctrl+O to select a file",
                  RGB(102, 116, 134), DT_CENTER);
         }
         empty_ = canvas.Upload(renderer);
      }
      SDL_Rect const target { (width - boxWidth) / 2,
         ToolbarHeight + (height - ToolbarHeight - StatusHeight - boxHeight) / 2,
         boxWidth, boxHeight };
      SDL_RenderCopy(renderer, empty_.get(), nullptr, &target);
   }
};

} // namespace adecc::sdl_ui
