// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Minimal headless PDFium consumer: create a page, render a rectangle,
// verify both image content and background without external PDF fixtures.

#include <fpdf_edit.h>
#include <fpdfview.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <string_view>

namespace {
   struct PdfiumLibrary final {
      PdfiumLibrary() {
         FPDF_LIBRARY_CONFIG aConfig {};
         aConfig.version = 2;
         FPDF_InitLibraryWithConfig(&aConfig);
      }

      ~PdfiumLibrary() {
         FPDF_DestroyLibrary();
      }

      PdfiumLibrary(PdfiumLibrary const&) = delete;
      PdfiumLibrary& operator=(PdfiumLibrary const&) = delete;
   };

   struct DocumentCloser final {
      void operator()(FPDF_DOCUMENT aDocument) const noexcept {
         if(aDocument) {
            FPDF_CloseDocument(aDocument);
         }
      }
   };

   struct PageCloser final {
      void operator()(FPDF_PAGE aPage) const noexcept {
         if(aPage) {
            FPDF_ClosePage(aPage);
         }
      }
   };

   struct BitmapCloser final {
      void operator()(FPDF_BITMAP aBitmap) const noexcept {
         if(aBitmap) {
            FPDFBitmap_Destroy(aBitmap);
         }
      }
   };

   struct ObjectCloser final {
      void operator()(FPDF_PAGEOBJECT aObject) const noexcept {
         if(aObject) {
            FPDFPageObj_Destroy(aObject);
         }
      }
   };

   using Document = std::unique_ptr<std::remove_pointer_t<FPDF_DOCUMENT>, DocumentCloser>;
   using Page = std::unique_ptr<std::remove_pointer_t<FPDF_PAGE>, PageCloser>;
   using Bitmap = std::unique_ptr<std::remove_pointer_t<FPDF_BITMAP>, BitmapCloser>;
   using PageObject = std::unique_ptr<std::remove_pointer_t<FPDF_PAGEOBJECT>, ObjectCloser>;

   void Require(bool const bCondition, std::string_view const svMessage) {
      if(!bCondition) {
         throw std::runtime_error(std::string(svMessage));
      }
   }

   [[nodiscard]] std::uint32_t Pixel(Bitmap const& aBitmap, int const iX, int const iY) {
      auto const* const pBuffer = static_cast<std::uint8_t const*>(
         FPDFBitmap_GetBuffer(aBitmap.get()));
      int const iStride = FPDFBitmap_GetStride(aBitmap.get());
      Require(pBuffer != nullptr && iStride >= 4 * 96, "Invalid PDFium BGRA buffer");
      auto const* const pPixel = pBuffer + static_cast<std::size_t>(iY) * iStride + 4 * iX;
      return (static_cast<std::uint32_t>(pPixel[2]) << 16U)
           | (static_cast<std::uint32_t>(pPixel[1]) << 8U)
           | static_cast<std::uint32_t>(pPixel[0]);
   }
}

int main() {
   try {
      PdfiumLibrary const aLibrary;
      Document const aDocument(FPDF_CreateNewDocument());
      Require(aDocument != nullptr, "FPDF_CreateNewDocument failed");

      Page const aPage(FPDFPage_New(aDocument.get(), 0, 96.0, 96.0));
      Require(aPage != nullptr, "FPDFPage_New failed");

      PageObject aRectangle(FPDFPageObj_CreateNewRect(16.0F, 16.0F, 64.0F, 64.0F));
      Require(aRectangle != nullptr, "FPDFPageObj_CreateNewRect failed");
      Require(FPDFPageObj_SetFillColor(aRectangle.get(), 255, 0, 0, 255),
              "FPDFPageObj_SetFillColor failed");
      Require(FPDFPath_SetDrawMode(aRectangle.get(), FPDF_FILLMODE_WINDING, 0),
              "FPDFPath_SetDrawMode failed");
      FPDFPage_InsertObject(aPage.get(), aRectangle.release());
      Require(FPDFPage_GenerateContent(aPage.get()), "FPDFPage_GenerateContent failed");
      Require(FPDF_GetPageCount(aDocument.get()) == 1, "Unexpected PDF page count");

      Bitmap const aBitmap(FPDFBitmap_Create(96, 96, 1));
      Require(aBitmap != nullptr, "FPDFBitmap_Create failed");
      FPDFBitmap_FillRect(aBitmap.get(), 0, 0, 96, 96, 0xFFFFFFFF);
      FPDF_RenderPageBitmap(aBitmap.get(), aPage.get(), 0, 0, 96, 96, 0, 0);

      Require(Pixel(aBitmap, 8, 8) == 0xFFFFFF, "PDF white background corrupted");
      Require(Pixel(aBitmap, 48, 48) == 0xFF0000, "PDF red rectangle did not render");
      std::cout << "PDFIUM|API|RENDER|PASS\n";
      return 0;
   }
   catch(std::exception const& aException) {
      std::cerr << "PDFIUM|API|RENDER|FAIL|" << aException.what() << '\n';
      return 1;
   }
}
