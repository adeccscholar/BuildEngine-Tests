// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Shared PDFium document/raster code for the SDL2 and VCL viewers.
// No VCL, SDL2 or Skia dependencies in this file.
#pragma once

#include <fpdfview.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace adecc::pdfview {

struct PagePixels final {
   int width {};
   int height {};
   std::vector<std::uint8_t> bgra; // top-down, width * height * 4, opaque
};

class Library final {
public:
   Library() {
      FPDF_LIBRARY_CONFIG aConfig {};
      aConfig.version = 2;
      FPDF_InitLibraryWithConfig(&aConfig);
   }
   ~Library() { FPDF_DestroyLibrary(); }
   Library(Library const&) = delete;
   Library& operator=(Library const&) = delete;
};

class Document final {
   // PDFium keeps the input pointer: bytes must outlive FPDF_CloseDocument().
   std::vector<std::uint8_t> bytes_;
   FPDF_DOCUMENT document_ {};
   int pageCount_ {};

public:
   Document() = default;
   ~Document() { Close(); }
   Document(Document const&) = delete;
   Document& operator=(Document const&) = delete;

   void Close() noexcept {
      if(document_)
         FPDF_CloseDocument(document_);
      document_ = nullptr;
      pageCount_ = 0;
      bytes_.clear();
   }

   [[nodiscard]] bool IsOpen() const noexcept { return document_ != nullptr; }
   [[nodiscard]] int PageCount() const noexcept { return pageCount_; }

   [[nodiscard]] bool Open(std::filesystem::path const& path, std::string& error) {
      // Deliberately bounded for this teaching/test application; not a general
      // streaming PDF engine. A Unicode Windows path is handled by ifstream.
      constexpr std::uintmax_t uMaxFile = 256ULL * 1024 * 1024;
      std::error_code ec;
      std::uintmax_t const uSize = std::filesystem::file_size(path, ec);
      if(ec || uSize == 0 || uSize > uMaxFile) {
         error = "File missing, empty or larger than the 256 MiB demo limit";
         return false;
      }

      std::ifstream input(path, std::ios::binary);
      if(!input) {
         error = "Cannot open selected file";
         return false;
      }
      std::vector<std::uint8_t> candidate(static_cast<std::size_t>(uSize));
      if(!input.read(reinterpret_cast<char*>(candidate.data()),
                     static_cast<std::streamsize>(candidate.size()))) {
         error = "Could not read all PDF bytes";
         return false;
      }

      FPDF_DOCUMENT const opened =
         FPDF_LoadMemDocument64(candidate.data(), candidate.size(), nullptr);
      if(!opened) {
         error = "FPDF_LoadMemDocument64 failed; PDFium error " +
                 std::to_string(FPDF_GetLastError()) +
                 " (password-protected files are not supported)";
         return false;
      }
      int const count = FPDF_GetPageCount(opened);
      if(count <= 0) {
         FPDF_CloseDocument(opened);
         error = "PDF contains no readable pages";
         return false;
      }

      // Only replace the currently displayed document after successful load.
      Close();
      bytes_.swap(candidate);
      document_ = opened;
      pageCount_ = count;
      error.clear();
      return true;
   }

   // Page dimensions in PDF points; the SDL2 viewport computes a single
   // fit-to-page scale and applies the selected zoom factor to that scale.
   [[nodiscard]] bool PageSize(int zeroBasedPage, double& width,
                               double& height, std::string& error) const {
      if(!document_ || zeroBasedPage < 0 || zeroBasedPage >= pageCount_) {
         error = "Page number is outside the document";
         return false;
      }
      FPDF_PAGE const page = FPDF_LoadPage(document_, zeroBasedPage);
      if(!page) {
         error = "FPDF_LoadPage failed while reading page dimensions";
         return false;
      }
      struct ClosePage final {
         FPDF_PAGE page;
         ~ClosePage() { FPDF_ClosePage(page); }
      } const guard { page };
      width = FPDF_GetPageWidth(page);
      height = FPDF_GetPageHeight(page);
      if(!std::isfinite(width) || !std::isfinite(height) ||
         width <= 0.0 || height <= 0.0) {
         error = "Invalid PDF page dimensions";
         return false;
      }
      error.clear();
      return true;
   }

   // Render ONLY the portion of a potentially zoomed page that is visible.
   // PDFium maps the complete page to the supplied size and applies negative
   // start_x/start_y. The bitmap bounds clip off-screen content, limiting
   // memory to viewportWidth * viewportHeight * 4.
   [[nodiscard]] bool RenderViewport(int zeroBasedPage,
                                     int pagePixelWidth, int pagePixelHeight,
                                     int viewportWidth, int viewportHeight,
                                     int scrollX, int scrollY,
                                     PagePixels& output, std::string& error) const {
      constexpr std::uint64_t maxBitmapPixels = 24ULL * 1024 * 1024;
      if(!document_ || zeroBasedPage < 0 || zeroBasedPage >= pageCount_ ||
         pagePixelWidth < 1 || pagePixelHeight < 1 ||
         pagePixelWidth > 131072 || pagePixelHeight > 131072 ||
         viewportWidth < 1 || viewportHeight < 1 ||
         viewportWidth > 8192 || viewportHeight > 8192 ||
         scrollX < 0 || scrollY < 0 ||
         scrollX > std::max(0, pagePixelWidth - viewportWidth) ||
         scrollY > std::max(0, pagePixelHeight - viewportHeight)) {
         error = "Invalid PDF viewport or zoom dimensions";
         return false;
      }

      int const bitmapWidth = std::min(pagePixelWidth, viewportWidth);
      int const bitmapHeight = std::min(pagePixelHeight, viewportHeight);
      if(static_cast<std::uint64_t>(bitmapWidth) * bitmapHeight > maxBitmapPixels) {
         error = "Visible PDF bitmap exceeds the demo memory limit";
         return false;
      }
      FPDF_PAGE const page = FPDF_LoadPage(document_, zeroBasedPage);
      if(!page) {
         error = "FPDF_LoadPage failed";
         return false;
      }
      struct ClosePage final {
         FPDF_PAGE page;
         ~ClosePage() { FPDF_ClosePage(page); }
      } const pageGuard { page };

      FPDF_BITMAP const bitmap = FPDFBitmap_Create(bitmapWidth, bitmapHeight, 1);
      if(!bitmap) {
         error = "FPDFBitmap_Create failed";
         return false;
      }
      struct CloseBitmap final {
         FPDF_BITMAP bitmap;
         ~CloseBitmap() { FPDFBitmap_Destroy(bitmap); }
      } const bitmapGuard { bitmap };
      FPDFBitmap_FillRect(bitmap, 0, 0, bitmapWidth, bitmapHeight, 0xFFFFFFFF);
      FPDF_RenderPageBitmap(bitmap, page, -scrollX, -scrollY,
                            pagePixelWidth, pagePixelHeight, 0, FPDF_ANNOT);
      int const stride = FPDFBitmap_GetStride(bitmap);
      auto const* const buffer =
         static_cast<std::uint8_t const*>(FPDFBitmap_GetBuffer(bitmap));
      if(!buffer || stride < bitmapWidth * 4) {
         error = "Invalid PDFium viewport bitmap stride or buffer";
         return false;
      }

      PagePixels candidate;
      candidate.width = bitmapWidth;
      candidate.height = bitmapHeight;
      candidate.bgra.resize(static_cast<std::size_t>(bitmapWidth) * bitmapHeight * 4);
      for(int row = 0; row < bitmapHeight; ++row)
         std::copy_n(buffer + static_cast<std::size_t>(row) * stride,
                     static_cast<std::size_t>(bitmapWidth) * 4,
                     candidate.bgra.data() + static_cast<std::size_t>(row) *
                                            bitmapWidth * 4);
      output = std::move(candidate);
      error.clear();
      return true;
   }

   [[nodiscard]] bool Render(int const zeroBasedPage, int const availableWidth,
                             int const availableHeight, PagePixels& output,
                             std::string& error) const {
      if(!document_ || zeroBasedPage < 0 || zeroBasedPage >= pageCount_ ||
         availableWidth < 1 || availableHeight < 1) {
         error = "Invalid page or display area";
         return false;
      }
      FPDF_PAGE const page = FPDF_LoadPage(document_, zeroBasedPage);
      if(!page) {
         error = "FPDF_LoadPage failed";
         return false;
      }
      struct PageGuard final {
         FPDF_PAGE value;
         ~PageGuard() { if(value) FPDF_ClosePage(value); }
      } const guard { page };

      double const pageWidth = FPDF_GetPageWidth(page);
      double const pageHeight = FPDF_GetPageHeight(page);
      if(!std::isfinite(pageWidth) || !std::isfinite(pageHeight) ||
         pageWidth <= 0.0 || pageHeight <= 0.0) {
         error = "PDF page dimensions are invalid";
         return false;
      }
      double const scale = std::min(static_cast<double>(availableWidth) / pageWidth,
                                    static_cast<double>(availableHeight) / pageHeight);
      if(!std::isfinite(scale) || scale <= 0.0) {
         error = "PDF page scale is invalid";
         return false;
      }
      int const width = std::max(1, static_cast<int>(std::floor(pageWidth * scale)));
      int const height = std::max(1, static_cast<int>(std::floor(pageHeight * scale)));
      constexpr std::uint64_t uMaxPixels = 24ULL * 1024 * 1024;
      if(width > 8192 || height > 8192 ||
         static_cast<std::uint64_t>(width) * height > uMaxPixels) {
         error = "Page raster is too large";
         return false;
      }

      FPDF_BITMAP const bitmap = FPDFBitmap_Create(width, height, 1);
      if(!bitmap) {
         error = "FPDFBitmap_Create failed";
         return false;
      }
      struct BitmapGuard final {
         FPDF_BITMAP value;
         ~BitmapGuard() { if(value) FPDFBitmap_Destroy(value); }
      } const bmpGuard { bitmap };

      FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT);
      auto const* const source =
         static_cast<std::uint8_t const*>(FPDFBitmap_GetBuffer(bitmap));
      int const stride = FPDFBitmap_GetStride(bitmap);
      if(!source || stride < width * 4) {
         error = "PDFium bitmap buffer/stride is invalid";
         return false;
      }
      PagePixels candidate;
      candidate.width = width;
      candidate.height = height;
      candidate.bgra.resize(static_cast<std::size_t>(width) * height * 4);
      for(int row = 0; row < height; ++row)
         std::copy_n(source + static_cast<std::size_t>(row) * stride,
                     static_cast<std::size_t>(width) * 4,
                     candidate.bgra.data() + static_cast<std::size_t>(row) * width * 4);
      output = std::move(candidate);
      error.clear();
      return true;
   }
};

} // namespace adecc::pdfview
