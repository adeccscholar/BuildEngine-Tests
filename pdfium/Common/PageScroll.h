// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Pure C++23 navigation: independent of PDFium, SDL2, Windows and VCL.
#pragma once

#include <algorithm>
#include <cmath>

namespace adecc::pdfview {

enum class EntryEdge { preserve, top, bottom };
enum class WheelAction { none, previousPage, nextPage };

// Coordinates are measured in rendered-page pixels (not PDF points).
// Rendering clips the page to the viewport; no full-zoom bitmap is allocated.
class PageScroll final {
   int width_ { 1 };
   int height_ { 1 };
   int viewportWidth_ { 1 };
   int viewportHeight_ { 1 };
   int x_ {};
   int y_ {};

public:
   [[nodiscard]] int PageWidth() const noexcept { return width_; }
   [[nodiscard]] int PageHeight() const noexcept { return height_; }
   [[nodiscard]] int ViewportWidth() const noexcept { return viewportWidth_; }
   [[nodiscard]] int ViewportHeight() const noexcept { return viewportHeight_; }
   [[nodiscard]] int X() const noexcept { return x_; }
   [[nodiscard]] int Y() const noexcept { return y_; }
   [[nodiscard]] int MaxX() const noexcept {
      return std::max(0, width_ - viewportWidth_);
   }
   [[nodiscard]] int MaxY() const noexcept {
      return std::max(0, height_ - viewportHeight_);
   }
   [[nodiscard]] bool WholePageVisible() const noexcept {
      return MaxX() == 0 && MaxY() == 0;
   }
   [[nodiscard]] int RasterWidth() const noexcept {
      return std::min(width_, viewportWidth_);
   }
   [[nodiscard]] int RasterHeight() const noexcept {
      return std::min(height_, viewportHeight_);
   }

   void SetPosition(int x, int y) noexcept {
      x_ = std::clamp(x, 0, MaxX());
      y_ = std::clamp(y, 0, MaxY());
   }

   void Configure(int pageWidth, int pageHeight, int viewportWidth,
                  int viewportHeight, EntryEdge edge = EntryEdge::preserve) noexcept {
      width_ = std::max(1, pageWidth);
      height_ = std::max(1, pageHeight);
      viewportWidth_ = std::max(1, viewportWidth);
      viewportHeight_ = std::max(1, viewportHeight);
      if(edge == EntryEdge::top)
         SetPosition(0, 0);
      else if(edge == EntryEdge::bottom)
         SetPosition(0, MaxY());
      else
         SetPosition(x_, y_);
   }

   // Zooming/resizing keeps the same *relative page position* near the view
   // centre. This also works across landscape, portrait and changed DPI.
   void PreserveCentre(PageScroll const& old) noexcept {
      double const fractionX =
         (static_cast<double>(old.x_) +
          static_cast<double>(old.RasterWidth()) * 0.5) / old.width_;
      double const fractionY =
         (static_cast<double>(old.y_) +
          static_cast<double>(old.RasterHeight()) * 0.5) / old.height_;
      SetPosition(static_cast<int>(std::lround(
                     fractionX * width_ - RasterWidth() * 0.5)),
                  static_cast<int>(std::lround(
                     fractionY * height_ - RasterHeight() * 0.5)));
   }

   // Positive direction: move down. Negative: move up.
   // First reach an edge, *then* the next event changes pages.
   // A fully vertically visible page immediately permits page navigation.
   [[nodiscard]] WheelAction MoveVertical(int pixels) noexcept {
      if(pixels == 0) return WheelAction::none;
      if(MaxY() == 0)
         return pixels > 0 ? WheelAction::nextPage : WheelAction::previousPage;
      if(pixels > 0) {
         if(y_ == MaxY()) return WheelAction::nextPage;
         y_ = static_cast<int>(std::min(static_cast<long long>(MaxY()),
                                           static_cast<long long>(y_) + pixels));
      } else {
         if(y_ == 0) return WheelAction::previousPage;
         y_ = static_cast<int>(std::max(0LL,
                                           static_cast<long long>(y_) + pixels));
      }
      return WheelAction::none;
   }

   // Panning never changes pages. Positive values move the viewport right
   // and down, matching SDL2's drag-to-pan with negative mouse deltas.
   // Use 64-bit intermediates so hostile/large relative motions cannot
   // overflow signed 32-bit page coordinates.
   void PanBy(int dx, int dy) noexcept {
      int const newX = static_cast<int>(std::clamp(
         static_cast<long long>(x_) + dx, 0LL,
         static_cast<long long>(MaxX())));
      int const newY = static_cast<int>(std::clamp(
         static_cast<long long>(y_) + dy, 0LL,
         static_cast<long long>(MaxY())));
      SetPosition(newX, newY);
   }

   void MoveHorizontal(int pixels) noexcept { PanBy(pixels, 0); }
};

} // namespace adecc::pdfview
