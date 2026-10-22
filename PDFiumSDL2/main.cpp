// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT
// SDL2-only UI. PDFium rasterizes; SDL2 draws pages, buttons and text.
// The native Windows file picker is a system dialog, not VCL.
#include "../PDFiumCommon/PdfRaster.h"
#include "ToolbarGraphics.h"
#include "../PDFiumCommon/PageScroll.h"

#include <SDL.h>
#include <SDL_main.h>
#include <SDL_syswm.h>
#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct SdlSystem final {
   SdlSystem() {
      SDL_SetMainReady();
      if(SDL_Init(SDL_INIT_VIDEO) != 0)
         throw std::runtime_error(SDL_GetError());
   }
   ~SdlSystem() { SDL_Quit(); }
};
struct DestroyWindow {
   void operator()(SDL_Window* p) const noexcept { if(p) SDL_DestroyWindow(p); }
};
struct DestroyRenderer {
   void operator()(SDL_Renderer* p) const noexcept { if(p) SDL_DestroyRenderer(p); }
};
struct DestroyTexture {
   void operator()(SDL_Texture* p) const noexcept { if(p) SDL_DestroyTexture(p); }
};
using WindowPtr = std::unique_ptr<SDL_Window, DestroyWindow>;
using RendererPtr = std::unique_ptr<SDL_Renderer, DestroyRenderer>;
using TexturePtr = std::unique_ptr<SDL_Texture, DestroyTexture>;

constexpr int iToolbarHeight = adecc::sdl_ui::ToolbarHeight;
constexpr int iStatusHeight = adecc::sdl_ui::StatusHeight;

[[nodiscard]] std::optional<std::filesystem::path>
ChoosePdf(SDL_Window* window) {
   SDL_SysWMinfo info {};
   SDL_VERSION(&info.version);
   HWND const owner = SDL_GetWindowWMInfo(window, &info) == SDL_TRUE
                      ? info.info.win.window : nullptr;
   std::array<wchar_t, 32768> path {};
   OPENFILENAMEW ofn {};
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = owner;
   ofn.lpstrFile = path.data();
   ofn.nMaxFile = static_cast<DWORD>(path.size());
   ofn.lpstrFilter = L"PDF files (*.pdf)\0*.pdf\0All files (*.*)\0*.*\0\0";
   ofn.lpstrTitle = L"Open PDF";
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
   if(!GetOpenFileNameW(&ofn))
      return std::nullopt;
   return std::filesystem::path { path.data() };
}

class Viewer final {
   SdlSystem sdl_;
   WindowPtr window_;
   RendererPtr renderer_;
   TexturePtr texture_;
   adecc::pdfview::Document document_;
   adecc::pdfview::PagePixels image_;
   adecc::pdfview::PageScroll scroll_;
   int index_ {};
   int zoomPercent_ { 100 };
   bool dragging_ {};
   int dragX_ {};
   int dragY_ {}; // 100% = page fitted to the visible viewport.
   bool editing_ {};
   std::string entry_;
   std::string status_ = "OPEN A PDF FILE";
   adecc::sdl_ui::Chrome chrome_;
   adecc::sdl_ui::Hot hovered_ { adecc::sdl_ui::Hot::none };

   void UpdateTitle(std::filesystem::path const& path) {
      auto const title = path.filename().u8string();
      std::string const file { reinterpret_cast<char const*>(title.data()),
                               title.size() };
      std::string const caption = "PDFium / SDL2 - " + file;
      SDL_SetWindowTitle(window_.get(), caption.c_str());
   }

   // PDFium only rasterizes the current viewport. The page itself may extend
   // well beyond it, but GPU/CPU bitmap memory never grows with the zoom.
   [[nodiscard]] bool Rasterize(adecc::pdfview::PageScroll const& requested) {
      adecc::pdfview::PagePixels candidate;
      std::string error;
      if(!document_.RenderViewport(index_, requested.PageWidth(),
                                    requested.PageHeight(),
                                    requested.ViewportWidth(),
                                    requested.ViewportHeight(),
                                    requested.X(), requested.Y(),
                                    candidate, error)) {
         status_ = "RENDER ERROR: " + error;
         return false;
      }
      TexturePtr newTexture { SDL_CreateTexture(renderer_.get(),
         SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STATIC,
         candidate.width, candidate.height) };
      if(!newTexture || SDL_UpdateTexture(newTexture.get(), nullptr,
                                          candidate.bgra.data(),
                                          candidate.width * 4) != 0) {
         status_ = std::string("SDL TEXTURE ERROR: ") + SDL_GetError();
         return false;
      }
      image_ = std::move(candidate);
      texture_ = std::move(newTexture);
      scroll_ = requested;
      status_ = "WHEEL: SCROLL / PAGES   CTRL+WHEEL: ZOOM   UP/DOWN: SCROLL   PAGEUP/DOWN: VIEWPORT";
      return true;
   }

   [[nodiscard]] bool RenderPage(
      adecc::pdfview::EntryEdge entry = adecc::pdfview::EntryEdge::preserve,
      bool keepCentre = false) {
      if(!document_.IsOpen()) return true;

      int screenWidth {}, screenHeight {};
      if(SDL_GetRendererOutputSize(renderer_.get(),
                                   &screenWidth, &screenHeight) != 0) {
         status_ = std::string("SDL renderer size: ") + SDL_GetError();
         return false;
      }
      int const viewportWidth = std::max(1, screenWidth - 36);
      int const viewportHeight = std::max(
         1, screenHeight - iToolbarHeight - iStatusHeight - 28);

      double pageWidthPoints {}, pageHeightPoints {};
      std::string error;
      if(!document_.PageSize(index_, pageWidthPoints, pageHeightPoints, error)) {
         status_ = "PAGE SIZE ERROR: " + error;
         return false;
      }

      double const fit = std::min(viewportWidth / pageWidthPoints,
                                  viewportHeight / pageHeightPoints);
      double const ratio = fit * zoomPercent_ / 100.0;
      double const exactWidth = pageWidthPoints * ratio;
      double const exactHeight = pageHeightPoints * ratio;
      // The raster limits are validated in PdfRaster too. Reject huge page
      // coordinates before converting the floating point result to int.
      if(!std::isfinite(exactWidth) || !std::isfinite(exactHeight) ||
         exactWidth < 0.0 || exactHeight < 0.0 ||
         exactWidth > 131072.0 || exactHeight > 131072.0) {
         status_ = "ZOOM EXCEEDS PDFIUM VIEWPORT LIMIT";
         return false;
      }

      int const pageWidthPixels =
         std::max(1, static_cast<int>(std::lround(exactWidth)));
      int const pageHeightPixels =
         std::max(1, static_cast<int>(std::lround(exactHeight)));
      adecc::pdfview::PageScroll requested = scroll_;
      requested.Configure(pageWidthPixels, pageHeightPixels,
                          viewportWidth, viewportHeight, entry);
      if(keepCentre) requested.PreserveCentre(scroll_);
      return Rasterize(requested);
   }

   void GoToPage(int requested,
                 adecc::pdfview::EntryEdge entry =
                    adecc::pdfview::EntryEdge::top) {
      if(!document_.IsOpen() || requested < 0 ||
         requested >= document_.PageCount()) return;
      int const prior = index_;
      index_ = requested;
      if(!RenderPage(entry))
         index_ = prior; // Retain the previously displayed texture and index.
   }

   void ZoomTo(int percent) {
      if(!document_.IsOpen()) return;
      percent = std::clamp(percent, 50, 600);
      if(percent == zoomPercent_) return;
      int const oldZoom = zoomPercent_;
      zoomPercent_ = percent;
      if(!RenderPage(adecc::pdfview::EntryEdge::preserve, true))
         zoomPercent_ = oldZoom;
   }

   void ZoomBy(int deltaPercent) {
      ZoomTo(zoomPercent_ + deltaPercent);
   }

   // A wheel/key event moves within the page. A subsequent event at the edge
   // switches to the neighbouring page with its opposite edge in view.
   void MoveVertical(int pixels) {
      if(!document_.IsOpen() || pixels == 0) return;
      adecc::pdfview::PageScroll moved = scroll_;
      auto const action = moved.MoveVertical(pixels);
      if(action == adecc::pdfview::WheelAction::nextPage) {
         GoToPage(index_ + 1, adecc::pdfview::EntryEdge::top);
      } else if(action == adecc::pdfview::WheelAction::previousPage) {
         GoToPage(index_ - 1, adecc::pdfview::EntryEdge::bottom);
      } else if(moved.Y() != scroll_.Y()) {
         if(!Rasterize(moved))
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Vertical PDF scroll: %s", status_.c_str());
      }
   }

   void MoveHorizontal(int pixels) {
      if(!document_.IsOpen() || pixels == 0) return;
      adecc::pdfview::PageScroll moved = scroll_;
      moved.MoveHorizontal(pixels);
      if(moved.X() != scroll_.X() && !Rasterize(moved))
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                      "Horizontal PDF scroll: %s", status_.c_str());
   }

   void DragTo(int x, int y) {
      if(!dragging_ || !document_.IsOpen()) return;
      int const dx = dragX_ - x;
      int const dy = dragY_ - y;
      dragX_ = x;
      dragY_ = y;
      adecc::pdfview::PageScroll next = scroll_;
      next.PanBy(dx, dy);
      if((next.X() != scroll_.X() || next.Y() != scroll_.Y()) &&
         !Rasterize(next))
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PDF drag-pan: %s",
                      status_.c_str());
   }

   void HandleWheel(SDL_MouseWheelEvent const& event) {
      if(!document_.IsOpen()) return;
      int const sign = event.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
      int const units = std::clamp(event.y, -12, 12) * sign;
      if(units == 0) return;

      SDL_Keymod const mods = SDL_GetModState();
      if((mods & KMOD_CTRL) != 0) {
         ZoomBy(units * 25);
      } else if((mods & KMOD_SHIFT) != 0) {
         // For wide zoomed pages, Shift+wheel pans horizontally.
         MoveHorizontal(-units * 96);
      } else {
         // Preserve wheel-notch semantics at page boundaries: on a scroll
         // event that first reaches the bottom, do not skip the next page.
         int const count = std::abs(units);
         for(int i = 0; i < count; ++i)
            MoveVertical(units > 0 ? -96 : 96);
      }
   }

   void Open() {
      std::optional<std::filesystem::path> const path = ChoosePdf(window_.get());
      if(!path) return;
      std::string error;
      if(!document_.Open(*path, error)) {
         status_ = "PDF OPEN FAILED";
         MessageBoxA(nullptr, error.c_str(), "PDFium / SDL2", MB_ICONERROR | MB_OK);
         return;
      }
      index_ = 0;
      zoomPercent_ = 100;
      scroll_ = {};
      dragging_ = false;
      SDL_SetCursor(SDL_GetDefaultCursor());
      editing_ = false;
      entry_.clear();
      SDL_StopTextInput();
      UpdateTitle(*path);
      texture_.reset();
      image_ = {};
      if(!RenderPage(adecc::pdfview::EntryEdge::top))
         MessageBoxA(nullptr, "PDF page could not be rendered.",
                     "PDFium / SDL2", MB_OK | MB_ICONERROR);
   }

   void AcceptPageNumber() {
      if(!editing_) return;
      int value {};
      auto const parsed = std::from_chars(entry_.data(),
                                          entry_.data() + entry_.size(), value);
      if(!entry_.empty() && parsed.ec == std::errc {} &&
         parsed.ptr == entry_.data() + entry_.size() &&
         value >= 1 && value <= document_.PageCount())
         GoToPage(value - 1);
      else
         status_ = "PAGE NUMBER OUT OF RANGE";
      editing_ = false;
      entry_.clear();
      SDL_StopTextInput();
   }

   void DrawScrollBar(int width, int height) {
      if(scroll_.MaxY() == 0) return;
      SDL_Rect const track { width - 11, iToolbarHeight + 14,
                             5, scroll_.ViewportHeight() };
      SDL_SetRenderDrawColor(renderer_.get(), 208, 216, 225, 255);
      SDL_RenderFillRect(renderer_.get(), &track);
      int const thumbHeight = std::clamp(
         static_cast<int>(
            static_cast<double>(scroll_.ViewportHeight()) *
            scroll_.ViewportHeight() / scroll_.PageHeight()),
         28, scroll_.ViewportHeight());
      int const position =
         static_cast<int>(static_cast<double>(scroll_.Y()) / scroll_.MaxY() *
                          (track.h - thumbHeight));
      SDL_Rect const thumb { track.x, track.y + position,
                             track.w, thumbHeight };
      SDL_SetRenderDrawColor(renderer_.get(), 117, 137, 160, 255);
      SDL_RenderFillRect(renderer_.get(), &thumb);
   }

   void Draw() {
      int width {}, height {};
      SDL_GetRendererOutputSize(renderer_.get(), &width, &height);
      SDL_SetRenderDrawColor(renderer_.get(), 230, 235, 242, 255);
      SDL_RenderClear(renderer_.get());
      if(texture_) {
         int const viewportLeft = 18;
         int const viewportTop = iToolbarHeight + 14;
         SDL_Rect const destination {
            viewportLeft + (scroll_.ViewportWidth() - image_.width) / 2,
            viewportTop + (scroll_.ViewportHeight() - image_.height) / 2,
            image_.width, image_.height
         };
         SDL_RenderCopy(renderer_.get(), texture_.get(), nullptr, &destination);
         DrawScrollBar(width, height);
      } else {
         chrome_.DrawEmpty(renderer_.get(), width, height);
      }
      chrome_.Draw(renderer_.get(), width, height,
         adecc::sdl_ui::State { document_.IsOpen(), editing_, index_,
                                 document_.PageCount(), hovered_, entry_,
                                 zoomPercent_ }, status_);
      SDL_RenderPresent(renderer_.get());
   }

   [[nodiscard]] static adecc::sdl_ui::Hot HitTest(int x, int y, int width) {
      SDL_Point const point { x, y };
      using namespace adecc::sdl_ui;
      if(SDL_PointInRect(&point, &OpenButton)) return Hot::open;
      if(SDL_PointInRect(&point, &PreviousButton)) return Hot::previous;
      if(SDL_PointInRect(&point, &PageField)) return Hot::page;
      if(SDL_PointInRect(&point, &GoButton)) return Hot::go;
      if(SDL_PointInRect(&point, &NextButton)) return Hot::next;
      SDL_Rect const zoomOut = ZoomOutButton(width);
      SDL_Rect const fit = ZoomFitButton(width);
      SDL_Rect const zoomIn = ZoomInButton(width);
      if(SDL_PointInRect(&point, &zoomOut)) return Hot::zoomOut;
      if(SDL_PointInRect(&point, &fit)) return Hot::zoomFit;
      if(SDL_PointInRect(&point, &zoomIn)) return Hot::zoomIn;
      return Hot::none;
   }

public:
   Viewer() {
      window_.reset(SDL_CreateWindow("PDFium / SDL2", SDL_WINDOWPOS_CENTERED,
         SDL_WINDOWPOS_CENTERED, 1100, 850, SDL_WINDOW_RESIZABLE));
      if(!window_) throw std::runtime_error(SDL_GetError());
      SDL_SetWindowMinimumSize(window_.get(), 920, 440);
      renderer_.reset(SDL_CreateRenderer(window_.get(), -1,
                                        SDL_RENDERER_ACCELERATED));
      if(!renderer_)
         renderer_.reset(SDL_CreateRenderer(window_.get(), -1,
                                            SDL_RENDERER_SOFTWARE));
      if(!renderer_) throw std::runtime_error(SDL_GetError());
   }

   void Run() {
      bool running = true;
      while(running) {
         SDL_Event event {};
         while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = false;
            else if(event.type == SDL_WINDOWEVENT &&
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
               if(!RenderPage(adecc::pdfview::EntryEdge::preserve, true))
                  SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                               "Resize render failed: %s", status_.c_str());
            }
            else if(event.type == SDL_MOUSEMOTION) {
               if(dragging_) DragTo(event.motion.x, event.motion.y);
               int width {}, height {};
               SDL_GetRendererOutputSize(renderer_.get(), &width, &height);
               hovered_ = HitTest(event.motion.x, event.motion.y, width);
            }
            else if(event.type == SDL_MOUSEBUTTONUP &&
                    event.button.button == SDL_BUTTON_LEFT) {
               dragging_ = false;
            }
            else if(event.type == SDL_WINDOWEVENT &&
                    event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
               dragging_ = false;
            }
            else if(event.type == SDL_MOUSEWHEEL) {
               HandleWheel(event.wheel);
            }
            else if(event.type == SDL_MOUSEBUTTONDOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
               using adecc::sdl_ui::Hot;
               int width {}, height {};
               SDL_GetRendererOutputSize(renderer_.get(), &width, &height);
               switch(HitTest(event.button.x, event.button.y, width)) {
                  case Hot::open: Open(); break;
                  case Hot::previous: GoToPage(index_ - 1); break;
                  case Hot::next: GoToPage(index_ + 1); break;
                  case Hot::zoomOut: ZoomBy(-25); break;
                  case Hot::zoomFit: ZoomTo(100); break;
                  case Hot::zoomIn: ZoomBy(25); break;
                  case Hot::page:
                     if(document_.IsOpen()) {
                        editing_ = true;
                        entry_.clear();
                        SDL_StartTextInput();
                     }
                     break;
                  case Hot::go: AcceptPageNumber(); break;
                  case Hot::none:
                     if(document_.IsOpen() && event.button.y >= iToolbarHeight &&
                        event.button.y < height - iStatusHeight) {
                        dragging_ = true;
                        dragX_ = event.button.x;
                        dragY_ = event.button.y;
                     }
                     break;
               }
            }
            else if(event.type == SDL_TEXTINPUT && editing_) {
               for(char const ch : std::string_view { event.text.text })
                  if(ch >= '0' && ch <= '9' && entry_.size() < 8)
                     entry_ += ch;
            }
            else if(event.type == SDL_KEYDOWN) {
               SDL_Keycode const key = event.key.keysym.sym;
               bool const ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
               if(ctrl && key == SDLK_o)
                  Open();
               else if((key == SDLK_EQUALS || key == SDLK_PLUS ||
                         key == SDLK_KP_PLUS) && !editing_)
                  ZoomBy(25);
               else if((key == SDLK_MINUS || key == SDLK_KP_MINUS) && !editing_)
                  ZoomBy(-25);
               else if(ctrl && key == SDLK_0)
                  ZoomTo(100);
               else if(key == SDLK_ESCAPE) {
                  if(editing_) {
                     editing_ = false;
                     entry_.clear();
                     SDL_StopTextInput();
                  } else running = false;
               } else if(key == SDLK_RETURN || key == SDLK_KP_ENTER)
                  AcceptPageNumber();
               else if(key == SDLK_BACKSPACE && editing_ && !entry_.empty())
                  entry_.pop_back();
               else if(!editing_) {
                  if(key == SDLK_UP) MoveVertical(-72);
                  if(key == SDLK_DOWN) MoveVertical(72);
                  if(key == SDLK_PAGEUP) MoveVertical(
                     -std::max(100, scroll_.ViewportHeight() * 9 / 10));
                  if(key == SDLK_PAGEDOWN) MoveVertical(
                      std::max(100, scroll_.ViewportHeight() * 9 / 10));
                  if(key == SDLK_LEFT) {
                     if(scroll_.MaxX() > 0) MoveHorizontal(-72);
                     else GoToPage(index_ - 1);
                  }
                  if(key == SDLK_RIGHT) {
                     if(scroll_.MaxX() > 0) MoveHorizontal(72);
                     else GoToPage(index_ + 1);
                  }
                  if(key == SDLK_HOME) GoToPage(0);
                  if(key == SDLK_END) GoToPage(document_.PageCount() - 1);
               }
            }
         }
         Draw();
         SDL_Delay(16);
      }
   }
};
} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
   try {
      adecc::pdfview::Library const pdfium;
      Viewer viewer;
      viewer.Run();
      return 0;
   }
   catch(std::exception const& e) {
      MessageBoxA(nullptr, e.what(), "PDFium / SDL2", MB_OK | MB_ICONERROR);
      return 1;
   }
}
