// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT
// Pure VCL UI; PDFium document/raster and navigation are shared with SDL2.
#pragma once

#include <vcl.h>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <System.Classes.hpp>
#include <System.Types.hpp>
#include <windows.h>
#include "../PDFiumCommon/PdfRaster.h"
#include "../PDFiumCommon/PageScroll.h"

class TViewerForm final : public TForm {
   // Member destruction is reversed: the document closes before PDFium exits.
   adecc::pdfview::Library pdfLibrary_;
   adecc::pdfview::Document document_;
   adecc::pdfview::PagePixels pagePixels_;
   adecc::pdfview::PageScroll scroll_;

   int currentPage_ {};
   int zoomPercent_ { 100 }; // 100% fits the entire page to the canvas.
   int wheelRemainder_ {};
   int wheelMode_ { -1 };
   bool dragging_ {};
   int dragX_ {};
   int dragY_ {};

__published:
   TPanel* toolbar_;
   TPanel* statusBar_;
   TPanel* zoomPanel_;
   TPaintBox* pageCanvas_;
   TButton* openButton_;
   TButton* previousButton_;
   TButton* nextButton_;
   TButton* goButton_;
   TButton* zoomOutButton_;
   TButton* zoomFitButton_;
   TButton* zoomInButton_;
   TEdit* pageEdit_;
   TLabel* counterLabel_;
   TLabel* zoomLabel_;
   TLabel* statusLabel_;
   TOpenDialog* openDialog_;

   void __fastcall OpenClick(TObject* sender);
   void __fastcall PreviousClick(TObject* sender);
   void __fastcall NextClick(TObject* sender);
   void __fastcall GoClick(TObject* sender);
   void __fastcall ZoomOutClick(TObject* sender);
   void __fastcall ZoomFitClick(TObject* sender);
   void __fastcall ZoomInClick(TObject* sender);
   void __fastcall PageKeyDown(TObject* sender, WORD& key, TShiftState shift);
   void __fastcall PaintPage(TObject* sender);
   void __fastcall FormResized(TObject* sender);
   void __fastcall FormMouseWheel(TObject* sender,
      System::Classes::TShiftState shift, int wheelDelta,
      const System::Types::TPoint& mousePos, bool& handled);

   // Mouse handlers are published so DFM event streaming can bind them.
   void __fastcall CanvasMouseDown(TObject* sender, TMouseButton button,
      TShiftState shift, int x, int y);
   void __fastcall CanvasMouseMove(TObject* sender, TShiftState shift,
      int x, int y);
   void __fastcall CanvasMouseUp(TObject* sender, TMouseButton button,
      TShiftState shift, int x, int y);

private:
   // Called from TApplication::OnMessage; not bound through the DFM.
   void __fastcall ApplicationMessage(tagMSG& message, bool& handled);

   void UpdateNavigation(bool refreshPageInput = true);
   [[nodiscard]] bool Rasterize(adecc::pdfview::PageScroll const& requested);
   [[nodiscard]] bool RenderCurrentPage(
      adecc::pdfview::EntryEdge edge = adecc::pdfview::EntryEdge::preserve,
      bool preserveCentre = false);
   void GoToPage(int zeroBasedPage,
                 adecc::pdfview::EntryEdge edge = adecc::pdfview::EntryEdge::top);
   void AcceptPageInput();
   void ZoomTo(int percent);
   void ZoomBy(int change);
   void MoveVertical(int pixels);
   void MoveHorizontal(int pixels);

public:
   __fastcall TViewerForm(TComponent* owner);
   __fastcall ~TViewerForm();
};

extern TViewerForm* ViewerForm;
