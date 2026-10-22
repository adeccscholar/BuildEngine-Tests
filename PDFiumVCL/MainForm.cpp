// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT
#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>
#include <Vcl.Graphics.hpp>
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

#pragma package(smart_init)
#pragma resource "*.dfm"
TViewerForm* ViewerForm = nullptr;

__fastcall TViewerForm::TViewerForm(TComponent* owner)
   : TForm(owner) { // DFM streams controls and event connections.
   AutoScroll = false;
   UpdateNavigation();
   // Arrow keys may be consumed by focused buttons as dialog navigation.
   Application->OnMessage = &ApplicationMessage;
}

__fastcall TViewerForm::~TViewerForm() {
   // Do not leave a VCL closure pointing to an already destroyed form.
   Application->OnMessage = nullptr;
}

void TViewerForm::UpdateNavigation(bool const refreshPageInput) {
   bool const ready = document_.IsOpen();
   previousButton_->Enabled = ready && currentPage_ > 0;
   nextButton_->Enabled = ready && currentPage_ + 1 < document_.PageCount();
   goButton_->Enabled = ready;
   pageEdit_->Enabled = ready;
   zoomOutButton_->Enabled = ready && zoomPercent_ > 50;
   zoomFitButton_->Enabled = ready;
   zoomInButton_->Enabled = ready && zoomPercent_ < 600;
   if(refreshPageInput)
      pageEdit_->Text = ready ? IntToStr(currentPage_ + 1) : L"";
   counterLabel_->Caption =
      L"Seite " + IntToStr(ready ? currentPage_ + 1 : 0) +
      L" / " + IntToStr(document_.PageCount());
   zoomLabel_->Caption = IntToStr(zoomPercent_) + L" %";
}

bool TViewerForm::Rasterize(adecc::pdfview::PageScroll const& requested) {
   adecc::pdfview::PagePixels candidate;
   std::string error;
   if(!document_.RenderViewport(currentPage_,
                                requested.PageWidth(), requested.PageHeight(),
                                requested.ViewportWidth(), requested.ViewportHeight(),
                                requested.X(), requested.Y(), candidate, error)) {
      statusLabel_->Caption = L"PDFium: " + UnicodeString(error.c_str());
      return false;
   }
   pagePixels_ = std::move(candidate);
   scroll_ = requested;
   statusLabel_->Caption =
      L"Mausrad: scrollen/blättern | Strg+Mausrad: Zoom | "
      L"Pfeil hoch/runter, Bild hoch/runter: Bildlauf";
   pageCanvas_->Invalidate();
   return true;
}

bool TViewerForm::RenderCurrentPage(adecc::pdfview::EntryEdge const edge,
                                    bool const preserveCentre) {
   if(!document_.IsOpen()) return true;
   if(!pageCanvas_ || pageCanvas_->ClientWidth <= 24 ||
      pageCanvas_->ClientHeight <= 24) return true;

   int const viewportWidth = pageCanvas_->ClientWidth - 24;
   int const viewportHeight = pageCanvas_->ClientHeight - 24;

   double widthPoints {}, heightPoints {};
   std::string error;
   if(!document_.PageSize(currentPage_, widthPoints, heightPoints, error)) {
      statusLabel_->Caption = L"PDFium: " + UnicodeString(error.c_str());
      return false;
   }
   double const fit = std::min(viewportWidth / widthPoints,
                               viewportHeight / heightPoints);
   double const scale = fit * zoomPercent_ / 100.0;
   double const rasterWidth = widthPoints * scale;
   double const rasterHeight = heightPoints * scale;
   // Check before converting to int; PdfRaster validates the limits again.
   if(!std::isfinite(rasterWidth) || !std::isfinite(rasterHeight) ||
      rasterWidth < 0.0 || rasterHeight < 0.0 ||
      rasterWidth > 131072.0 || rasterHeight > 131072.0) {
      statusLabel_->Caption = L"Zoom überschreitet die PDFium-Rastergrenze.";
      return false;
   }

   adecc::pdfview::PageScroll requested = scroll_;
   requested.Configure(
      std::max(1, static_cast<int>(std::lround(rasterWidth))),
      std::max(1, static_cast<int>(std::lround(rasterHeight))),
      viewportWidth, viewportHeight, edge);
   if(preserveCentre)
      requested.PreserveCentre(scroll_);
   return Rasterize(requested);
}

void TViewerForm::GoToPage(int const zeroBasedPage,
                            adecc::pdfview::EntryEdge const edge) {
   if(!document_.IsOpen() || zeroBasedPage < 0 ||
      zeroBasedPage >= document_.PageCount()) return;
   int const previousPage = currentPage_;
   currentPage_ = zeroBasedPage;
   if(!RenderCurrentPage(edge))
      currentPage_ = previousPage; // Keep previous image and page identity.
   UpdateNavigation();
}

void TViewerForm::ZoomTo(int percent) {
   if(!document_.IsOpen()) return;
   percent = std::clamp(percent, 50, 600);
   if(percent == zoomPercent_) return;
   int const oldZoom = zoomPercent_;
   zoomPercent_ = percent;
   if(!RenderCurrentPage(adecc::pdfview::EntryEdge::preserve, true))
      zoomPercent_ = oldZoom;
   // Zooming must not discard an unfinished page number in the edit box.
   UpdateNavigation(false);
}

void TViewerForm::ZoomBy(int const change) {
   ZoomTo(zoomPercent_ + change);
}

void TViewerForm::MoveVertical(int const pixels) {
   if(!document_.IsOpen() || pixels == 0) return;
   adecc::pdfview::PageScroll requested = scroll_;
   auto const action = requested.MoveVertical(pixels);
   if(action == adecc::pdfview::WheelAction::nextPage)
      GoToPage(currentPage_ + 1, adecc::pdfview::EntryEdge::top);
   else if(action == adecc::pdfview::WheelAction::previousPage)
      GoToPage(currentPage_ - 1, adecc::pdfview::EntryEdge::bottom);
   else if(requested.Y() != scroll_.Y())
      if(!Rasterize(requested)) return;
}

void TViewerForm::MoveHorizontal(int const pixels) {
   if(!document_.IsOpen() || pixels == 0) return;
   adecc::pdfview::PageScroll requested = scroll_;
   requested.MoveHorizontal(pixels);
   if(requested.X() != scroll_.X())
      if(!Rasterize(requested)) return;
}

void __fastcall TViewerForm::OpenClick(TObject*) {
   if(!openDialog_->Execute()) return;
   std::filesystem::path const path { openDialog_->FileName.c_str() };
   std::string error;
   if(!document_.Open(path, error)) {
      UnicodeString const message(error.c_str());
      Application->MessageBox(message.c_str(), L"PDFium: Öffnen fehlgeschlagen",
                              MB_OK | MB_ICONERROR);
      return;
   }
   currentPage_ = 0;
   zoomPercent_ = 100;
   wheelRemainder_ = 0;
   wheelMode_ = -1;
   scroll_ = {};
   dragging_ = false;
   pagePixels_ = {};
   Caption = L"PDFium / VCL - " + openDialog_->FileName;
   if(!RenderCurrentPage(adecc::pdfview::EntryEdge::top))
      Application->MessageBox(L"Die erste Seite konnte nicht gerendert werden.",
                              L"PDFium / VCL", MB_OK | MB_ICONERROR);
   UpdateNavigation();
   pageCanvas_->Invalidate();
}

void __fastcall TViewerForm::PreviousClick(TObject*) {
   GoToPage(currentPage_ - 1);
}
void __fastcall TViewerForm::NextClick(TObject*) {
   GoToPage(currentPage_ + 1);
}
void __fastcall TViewerForm::ZoomOutClick(TObject*) { ZoomBy(-25); }
void __fastcall TViewerForm::ZoomFitClick(TObject*) { ZoomTo(100); }
void __fastcall TViewerForm::ZoomInClick(TObject*) { ZoomBy(25); }

void TViewerForm::AcceptPageInput() {
   if(!document_.IsOpen()) return;
   int const page = StrToIntDef(pageEdit_->Text, -1);
   if(page < 1 || page > document_.PageCount()) {
      statusLabel_->Caption = L"Seitenzahl außerhalb des gültigen Bereichs.";
      pageEdit_->SetFocus();
      pageEdit_->SelectAll();
      return;
   }
   GoToPage(page - 1);
}

void __fastcall TViewerForm::GoClick(TObject*) { AcceptPageInput(); }
void __fastcall TViewerForm::PageKeyDown(TObject*, WORD& key, TShiftState) {
   if(key == VK_RETURN) {
      AcceptPageInput();
      key = 0;
   }
}

void __fastcall TViewerForm::FormMouseWheel(
   TObject*, System::Classes::TShiftState const shift, int wheelDelta,
   const System::Types::TPoint&, bool& handled) {
   if(!document_.IsOpen()) return;
   handled = true;

   int const mode = shift.Contains(ssCtrl) ? 2 :
                    shift.Contains(ssShift) ? 1 : 0;
   if(mode != wheelMode_) {
      wheelRemainder_ = 0;
      wheelMode_ = mode;
   }
   wheelRemainder_ += std::clamp(wheelDelta, -1200, 1200);
   int const notches = wheelRemainder_ / WHEEL_DELTA;
   wheelRemainder_ -= notches * WHEEL_DELTA;
   if(notches == 0) return;

   if(mode == 2) {
      ZoomBy(25 * notches);              // Ctrl+wheel
   } else if(mode == 1) {
      MoveHorizontal(-96 * notches);     // Shift+wheel
   } else {
      for(int i = 0; i < std::abs(notches); ++i)
         MoveVertical(notches > 0 ? -96 : 96);
   }
}

void __fastcall TViewerForm::ApplicationMessage(tagMSG& message, bool& handled) {
   if(handled || message.message != WM_KEYDOWN ||
      (!document_.IsOpen() && message.wParam != 'O'))
      return;
   if(message.hwnd != Handle && !::IsChild(Handle, message.hwnd))
      return;

   bool const ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
   unsigned int const key = static_cast<unsigned int>(message.wParam);

   if(ctrl && key == 'O') {
      handled = true;
      OpenClick(nullptr);
      return;
   }
   if(!document_.IsOpen()) return;
   if(ctrl) {
      if(key == VK_OEM_PLUS || key == VK_ADD) ZoomBy(25);
      else if(key == VK_OEM_MINUS || key == VK_SUBTRACT) ZoomBy(-25);
      else if(key == '0' || key == VK_NUMPAD0) ZoomTo(100);
      else return;
      handled = true;
      return;
   }

   // Arrow keys must remain ordinary cursor-editing commands in TEdit.
   if(pageEdit_ && ::GetFocus() == pageEdit_->Handle)
      return;

   switch(key) {
      case VK_UP:    MoveVertical(-72); break;
      case VK_DOWN:  MoveVertical(72); break;
      case VK_PRIOR: MoveVertical(
         -std::max(100, scroll_.ViewportHeight() * 9 / 10)); break;
      case VK_NEXT: MoveVertical(
          std::max(100, scroll_.ViewportHeight() * 9 / 10)); break;
      case VK_LEFT:
         if(scroll_.MaxX() > 0) MoveHorizontal(-72);
         else GoToPage(currentPage_ - 1);
         break;
      case VK_RIGHT:
         if(scroll_.MaxX() > 0) MoveHorizontal(72);
         else GoToPage(currentPage_ + 1);
         break;
      case VK_HOME:  GoToPage(0); break;
      case VK_END:   GoToPage(document_.PageCount() - 1); break;
      default: return;
   }
   handled = true;
}

void __fastcall TViewerForm::CanvasMouseDown(
   TObject*, TMouseButton button, TShiftState, int x, int y) {
   if(button == mbLeft && document_.IsOpen() &&
      (scroll_.MaxX() > 0 || scroll_.MaxY() > 0)) {
      dragging_ = true;
      dragX_ = x;
      dragY_ = y;
      pageCanvas_->Cursor = crSizeAll;
      // TPaintBox is a TGraphicControl without its own HWND.
      // Dragging is handled by the paint box's normal VCL mouse events.
   }
}

void __fastcall TViewerForm::CanvasMouseMove(
   TObject*, TShiftState shift, int x, int y) {
   if(!dragging_) return;
   if(!shift.Contains(ssLeft)) {
      dragging_ = false;
      pageCanvas_->Cursor = crDefault;
      return;
   }
   adecc::pdfview::PageScroll next = scroll_;
   next.PanBy(dragX_ - x, dragY_ - y);
   dragX_ = x;
   dragY_ = y;
   if(next.X() != scroll_.X() || next.Y() != scroll_.Y())
      (void)Rasterize(next);
}

void __fastcall TViewerForm::CanvasMouseUp(
   TObject*, TMouseButton button, TShiftState, int, int) {
   if(button == mbLeft && dragging_) {
      dragging_ = false;
      pageCanvas_->Cursor = crDefault;

   }
}

void __fastcall TViewerForm::PaintPage(TObject*) {
   TCanvas* const canvas = pageCanvas_->Canvas;
   TRect const area { 0, 0, pageCanvas_->ClientWidth, pageCanvas_->ClientHeight };
   canvas->Brush->Color = static_cast<TColor>(RGB(230, 235, 242));
   canvas->FillRect(area);
   if(pagePixels_.bgra.empty()) return;

   // PDFium's BGRA raster is already cropped to the visible portion. A
   // negative DIB height states that its first scanline is the TOP scanline.
   BITMAPINFO info {};
   info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   info.bmiHeader.biWidth = pagePixels_.width;
   info.bmiHeader.biHeight = -pagePixels_.height;
   info.bmiHeader.biPlanes = 1;
   info.bmiHeader.biBitCount = 32;
   info.bmiHeader.biCompression = BI_RGB;

   int const x = (pageCanvas_->ClientWidth - pagePixels_.width) / 2;
   int const y = (pageCanvas_->ClientHeight - pagePixels_.height) / 2;
   ::StretchDIBits(canvas->Handle,
                   x, y, pagePixels_.width, pagePixels_.height,
                   0, 0, pagePixels_.width, pagePixels_.height,
                   pagePixels_.bgra.data(), &info, DIB_RGB_COLORS, SRCCOPY);

   // Same passive vertical position indicator as the SDL2 viewer. The
   // content is inset by 12 px so this indicator does not cover PDF pixels.
   if(scroll_.MaxY() > 0) {
      int const trackHeight = std::min(scroll_.ViewportHeight(),
                                        pageCanvas_->ClientHeight - 24);
      if(trackHeight < 1) return;
      int const left = pageCanvas_->ClientWidth - 10;
      int const top = 12;
      canvas->Brush->Color = static_cast<TColor>(RGB(208, 216, 225));
      canvas->FillRect(TRect { left, top, left + 5, top + trackHeight });

      int const thumbHeight = std::clamp(
         static_cast<int>(static_cast<double>(trackHeight) *
                          scroll_.ViewportHeight() / scroll_.PageHeight()),
         std::min(28, trackHeight), trackHeight);
      int const thumbOffset = static_cast<int>(
         static_cast<double>(scroll_.Y()) / scroll_.MaxY() *
         (trackHeight - thumbHeight));
      canvas->Brush->Color = static_cast<TColor>(RGB(117, 137, 160));
      canvas->FillRect(TRect { left, top + thumbOffset,
                              left + 5, top + thumbOffset + thumbHeight });
   }
}

void __fastcall TViewerForm::FormResized(TObject*) {
   if(document_.IsOpen() &&
      !RenderCurrentPage(adecc::pdfview::EntryEdge::preserve, true)) {
      // Keep the last good image and report the error in the status label.
      pageCanvas_->Invalidate();
   }
}
