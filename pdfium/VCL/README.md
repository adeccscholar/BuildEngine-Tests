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

## Deploy the exact PDFium 2025.11.19 / Skia 153 closure

The Win64x VCL project still uses the PDFium 2025.11.19 C import library,
but PDFium's current installed dependency is **Skia 153**, not Skia 144.
Its transitive runtime closure must match the current BuildEngine XML
contract. The VCL/IDE build has no automatic CMake runtime stage.

Create a UTF-8 manifest with exactly these package roots for the current
PDFium and Skia dependency versions. The example production root below
is the one already used by this application; adjust it when deploying
to another BuildEngine workspace. Do not use PATH discovery or select
an arbitrary newer dependency in place of a catalog-declared version.

```text
D:/local/embarcadero/test_v3/install/packages/pdfium/2025.11.19
D:/local/embarcadero/test_v3/install/packages/skia/153
D:/local/embarcadero/test_v3/install/packages/abseil/20260817.0
D:/local/embarcadero/test_v3/install/packages/fast_float/8.2.3
D:/local/embarcadero/test_v3/install/packages/icu/78.3
D:/local/embarcadero/test_v3/install/packages/freetype/2.15.0
D:/local/embarcadero/test_v3/install/packages/harfbuzz/15.1.0
D:/local/embarcadero/test_v3/install/packages/zlib/1.4
D:/local/embarcadero/test_v3/install/packages/libpng/1.6.59
D:/local/embarcadero/test_v3/install/packages/libjpeg-turbo/3.2.0
D:/local/embarcadero/test_v3/install/packages/brotli/1.2.1
D:/local/embarcadero/test_v3/install/packages/bzip2/1.0.8
D:/local/embarcadero/test_v3/install/packages/lcms2/2.20
D:/local/embarcadero/test_v3/install/packages/openjpeg/2.5.4
D:/local/embarcadero/test_v3/install/packages/opengl/26.2.1
```

Stage the exact PE dependencies with BuildEngine's existing checked
runtime stager, using the chosen configuration. These are single-line
`cmd.exe` invocations (the `roots.txt` path refers to the manifest
created above):

```bat
cmake "-DADECC_RUNTIME_ROOTS_FILE:FILEPATH=D:/path/to/roots.txt" "-DADECC_RUNTIME_CONFIGURATION=Release" "-DADECC_RUNTIME_OUTPUT_DIR=D:/local/embarcadero/test_v3/tests/pdfium/VCL/Win64x/Release" -P "D:/local/embarcadero/test_v3/admin/cmake/consumer/BuildEngineStageRuntime.cmake"
cmake -E copy_if_different "D:/local/embarcadero/test_v3/install/packages/skia/153/bin/win64/Release/icudtl.dat" "D:/local/embarcadero/test_v3/tests/pdfium/VCL/Win64x/Release/icudtl.dat"
```

For Debug, substitute `Debug` consistently in the MSBuild configuration,
the staging configuration, the output folder and the Skia ICU data source.
The generic runtime stager validates PE64 dependencies and rejects name/hash
collisions, but it copies DLLs only. The explicitly versioned
`icudtl.dat` copy is therefore required in addition.

Do not stage obsolete `skia144.dll` or
`third_party_abseil-cpp_absl.dll`. PDFium's current imports use
`skia.dll` and `libabseil_dll.dll`. The same modern FreeType and
HarfBuzz versions must be used throughout the installed closure.
If old DLLs remain in the output directory, remove those retired
artifacts before deployment; do not suppress collisions between
distinct current dependencies.

**Verification:** The configured package declarations and VCL
source/project structure were inspected. Native Win64x compilation,
deployment and graphical rendering remain **[nicht verifiziert]**.
