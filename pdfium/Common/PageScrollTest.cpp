// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT
// Standalone C++ navigation regression tests; no PDFs, SDL2 or Windows needed.
#include "PageScroll.h"
// CHECK must remain active even with -DNDEBUG in Release.
#define CHECK(condition) do { if(!(condition)) return __LINE__; } while(false)

int main() {
   using adecc::pdfview::EntryEdge;
   using adecc::pdfview::PageScroll;
   using adecc::pdfview::WheelAction;

   PageScroll state;
   state.Configure(600, 800, 700, 900);
   CHECK(state.WholePageVisible());
   CHECK(state.MoveVertical(96) == WheelAction::nextPage);
   CHECK(state.MoveVertical(-96) == WheelAction::previousPage);
   CHECK(state.X() == 0 && state.Y() == 0);

   state.Configure(1200, 2400, 700, 800, EntryEdge::top);
   CHECK(!state.WholePageVisible());
   CHECK(state.MaxX() == 500 && state.MaxY() == 1600);
   CHECK(state.MoveVertical(96) == WheelAction::none && state.Y() == 96);
   state.SetPosition(0, 1570);
   CHECK(state.MoveVertical(96) == WheelAction::none && state.Y() == 1600);
   CHECK(state.MoveVertical(96) == WheelAction::nextPage);

   // Previous page is entered at its bottom, not at its first line.
   state.Configure(1100, 1800, 700, 800, EntryEdge::bottom);
   CHECK(state.Y() == 1000);
   CHECK(state.MoveVertical(-96) == WheelAction::none && state.Y() == 904);
   state.SetPosition(0, 0);
   CHECK(state.MoveVertical(-96) == WheelAction::previousPage);

   // Next page is entered at its top.
   state.Configure(1100, 1800, 700, 800, EntryEdge::top);
   CHECK(state.Y() == 0);
   CHECK(state.MoveVertical(96) == WheelAction::none && state.Y() == 96);

   // A page wider than the viewport but shorter than the viewport has no
   // vertical scroll range. The ordinary wheel must not become trapped.
   state.Configure(1500, 400, 700, 800, EntryEdge::top);
   CHECK(!state.WholePageVisible() && state.MaxY() == 0);
   CHECK(state.MoveVertical(96) == WheelAction::nextPage);
   state.MoveHorizontal(2000);
   CHECK(state.X() == 800);
   state.MoveHorizontal(-2000);
   CHECK(state.X() == 0);

   // Hand-tool drag: move both axes in one clamped operation and never
   // change pages when the mouse reaches an image boundary.
   state.Configure(1600, 2100, 700, 800, EntryEdge::top);
   state.PanBy(200, 300);
   CHECK(state.X() == 200 && state.Y() == 300);
   state.PanBy(-100, -170);
   CHECK(state.X() == 100 && state.Y() == 130);
   state.PanBy(-5000, -5000);
   CHECK(state.X() == 0 && state.Y() == 0);
   state.PanBy(5000, 5000);
   CHECK(state.X() == state.MaxX() && state.Y() == state.MaxY());
   state.PanBy(0, 1);
   CHECK(state.Y() == state.MaxY());

   // Resizing/zooming preserves the normalized centre and clamps to bounds.
   state.Configure(1000, 2000, 600, 800, EntryEdge::top);
   state.SetPosition(300, 600);
   auto const old = state;
   state.Configure(2000, 4000, 600, 800);
   state.PreserveCentre(old);
   CHECK(state.X() == 900 && state.Y() == 1600);

   // PageUp/PageDown have exactly the same semantics as the wheel, but with
   // viewport-sized increments rather than wheel-sized increments.
   state.Configure(600, 2200, 600, 800, EntryEdge::top);
   CHECK(state.MoveVertical(720) == WheelAction::none && state.Y() == 720);
   CHECK(state.MoveVertical(720) == WheelAction::none && state.Y() == 1400);
   CHECK(state.MoveVertical(720) == WheelAction::nextPage);
   CHECK(state.MoveVertical(-720) == WheelAction::none && state.Y() == 680);
   CHECK(state.MoveVertical(-720) == WheelAction::none && state.Y() == 0);
   CHECK(state.MoveVertical(-720) == WheelAction::previousPage);
}
