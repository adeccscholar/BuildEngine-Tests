# PDFium VCL Viewer (RAD Studio 13 / C++Builder Win64x)

A separate **VCL application**, not an SDL2 application. It consumes the
same installed PDFium 2025.11.19 DLL as Test/PDFiumSDL2, with the shared
PDFium loading/raster logic in ../PDFiumCommon/PdfRaster.h.

## UI

The main form is a **designer-editable C++Builder form**: `MainForm.h`
(published components/events), `MainForm.cpp` (PDFium and navigation), and
`MainForm.dfm` (layout/resources). It includes TOpenDialog, TButton
open/previous/next/go and zoom controls, TEdit for 1-based page navigation,
TLabel for "Seite current / total", and a TPaintBox displaying PDF pages. The previous/next buttons are disabled
at the first/last page. An entered page number is validated for 1..PageCount;
press Enter or "Gehe zu" to navigate.

PDFium renders opaque BGRA pixels. The VCL TPaintBox.OnPaint handler
presents the pixels using Win32 StretchDIBits with negative DIB height (top
row first); no SDL2 is linked into this application. VCL uses the same
`PageScroll` state and `RenderViewport` PDFium operation as the SDL2 viewer,
so at increased zoom only the visible page area is rasterized. Form resize and
zoom changes preserve the approximate centre of the viewport. No large
off-screen bitmap is allocated for the entire zoomed page.

## Zoom, wheel and keyboard controls

- **− / Fit / +** in the VCL toolbar: zoom in 25% increments, 50–600%.
  **100% = fit the complete PDF page to the VCL viewport.**
  The toolbar displays the selected zoom percentage and disables out-of-range
  zoom buttons.
- **Ctrl+mouse wheel**, **Ctrl+Plus / Ctrl+Minus**, **Ctrl+0**: zoom or fit.
- **Mouse wheel**: while the current PDF page is vertically larger than the
  viewport, scroll through it. The first movement that reaches the end stops
  there; the *following* movement goes to the next page at its top. Scrolling
  upwards past the top enters the previous page at its bottom. If the page is
  fully visible vertically, the wheel goes directly to the adjacent page.
- **Up/Down**: 72-pixel vertical steps with the same page-boundary semantics.
  **PageUp/PageDown**: approximately 90% of viewport height per step, then
  the neighbouring page when the current edge is reached.
- **Shift+wheel**: horizontal movement inside a zoomed page.
  **Left/Right**: immediate previous/next page; **Home/End**: first/last page.
  The numeric page input keeps its normal cursor/editing keys and Enter
  validates the selected page.

At higher zoom levels Left/Right now pan the page horizontally instead of
changing pages. The TPaintBox can also be dragged with the left mouse button
in both axes (without triggering automatic page changes). The same shared
`PageScroll::PanBy` is used by the SDL2 viewer.

The vertical scroll position is indicated by a small passive scrollbar on
the right of the `TPaintBox`. The VCL form uses a narrow
`TApplication::OnMessage` handler for viewer keyboard shortcuts; VCL's
`KeyPreview` alone cannot reliably receive arrow keys when buttons have
focus. The application-wide keyboard handler is removed when the form closes.
The wheel uses VCL `OnMouseWheel` and never changes the zoom unless Ctrl is
pressed.

Navigation is shared with SDL2 through
`../PDFiumCommon/PageScroll.h`. The pure C++ regression tests are in
`../PDFiumCommon/PageScrollTest.cpp`. Native C++Builder GUI operation
remains **[not verified]**.

## Build

Open PDFiumVCL.cbproj in RAD Studio 13, select **Win64x**, then Release or
Debug. Build. This example deliberately does not rely on the PDFium DLL
already being present on the developer's system PATH.

The MSBuild property StackRoot is the BuildEngine production folder
(D:\local\embarcadero\test_v3 by default in this test). Set StackRoot from
the IDE's MSBuild options, or use the RAD Studio MSBuild environment:

    msbuild PDFiumVCL.cbproj /p:Platform=Win64x /p:Config=Release /p:StackRoot=D:\local\embarcadero\test_v3

PDFiumVCL.cbproj uses the exact file
$(StackRoot)\install\packages\pdfium\2025.11.19\lib\win64\$(Config)\pdfium.lib
and $(StackRoot)\install\packages\pdfium\2025.11.19\include.
DynamicRTL=true matches the BCC64X third-party runtime contract.
VCL runtime packages come from the RAD Studio installation.

## Deploy the exact PDFium dependency closure

The .cbproj IDE linker does not do the CMake application's runtime staging.
Use the **existing BuildEngine** generic stage script; do not rename DLLs or
copy libraries indiscriminately from PATH. Generate an absolute-path UTF-8
root manifest containing one installed package root per line, for example:

    D:/local/embarcadero/test_v3/install/packages/pdfium/2025.11.19
    D:/local/embarcadero/test_v3/install/packages/skia/144
    D:/local/embarcadero/test_v3/install/packages/abseil/20260817.0
    D:/local/embarcadero/test_v3/install/packages/icu/78.3
    D:/local/embarcadero/test_v3/install/packages/freetype/2.14.3
    D:/local/embarcadero/test_v3/install/packages/harfbuzz/14.4.0
    D:/local/embarcadero/test_v3/install/packages/zlib/1.3.2
    D:/local/embarcadero/test_v3/install/packages/libpng/1.6.58
    D:/local/embarcadero/test_v3/install/packages/libjpeg-turbo/3.2.0
    D:/local/embarcadero/test_v3/install/packages/brotli/1.2.0
    D:/local/embarcadero/test_v3/install/packages/bzip2/1.0.8
    D:/local/embarcadero/test_v3/install/packages/lcms2/2.19.1
    D:/local/embarcadero/test_v3/install/packages/openjpeg/2.5.4

Call cmake from the RAD Studio command environment with the same Config as the
VCL build (example after creating roots.txt from the list above):

    cmake "-DADECC_RUNTIME_ROOTS_FILE:FILEPATH=D:/path/to/roots.txt" "-DADECC_RUNTIME_CONFIGURATION=Release" "-DADECC_RUNTIME_OUTPUT_DIR=D:/path/to/Test/PDFiumVCL/Win64x/Release" -P "D:/local/embarcadero/test_v3/admin/cmake/consumer/BuildEngineStageRuntime.cmake"

For selected toolchain DLLs such as libwinpthread-1-370.dll, supply
ADECC_RUNTIME_TOOLCHAIN_DLLS_FILE and ADECC_RUNTIME_TOOLCHAIN_BIN to the same
BuildEngineStageRuntime.cmake script. The script validates PE64 DLLs and
rejects name/hash collisions. Do not deploy copies of Windows API-set DLLs.
Change Release to Debug consistently for the Debug build.

The project is intentionally a minimal source-controlled RAD Studio project.
The user can open it and adjust RAD Studio generated IDE metadata for their
installation if required.

**Verification:** XML project structure/source consistency statically checked.
Native RAD Studio Win64x compile, DLL staging and VCL GUI operation remain
[not verified].
