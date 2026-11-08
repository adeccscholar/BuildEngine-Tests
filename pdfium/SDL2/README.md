# PDFium + SDL2 graphical test (no VCL)

A native BCC64X/Win64x C++23 **GUI** using the BuildEngine PDFium 2025.11.19
and SDL2 2.32.10 DLL packages. No VCL objects, DFM resources, Skia UI library,
SDL_ttf or C++Builder GUI designer are used.

The program opens an empty graphical SDL window. Click **OPEN** for the standard
Windows PDF file picker (GetOpenFileNameW; not VCL); select a PDF; PDFium renders
page 1 into BGRA pixels and SDL2 presents them as a texture. **PREV** and **NEXT**
stop at document bounds; **PAGE x / n** reports current and total pages; click
the numeric field, type a 1-based page number, then click **GO** or press Enter.
Keys: Ctrl+O open, Left/Right or PageUp/PageDown navigate, Home/End jump to first
or last, Escape cancels edit or exits. Window resize renders the selected page
again at the new fit-to-window resolution. The page content and toolbar are
drawn entirely via SDL2.

## Zoom, scrolling and keyboard navigation

The viewer opens in **Fit (100%)** mode. Toolbar buttons `-`, `Fit`,
`+` adjust the zoom in 25-percent steps between 50% and 600% of the
fit-to-page scale. Ctrl+mouse wheel, Ctrl+plus, Ctrl+minus and Ctrl+0
provide the same controls. Zoom and resizing preserve the approximate
centre of the visible page.

The mouse wheel moves vertically while part of a page is off-screen.
Once its top/bottom edge is reached, the **next wheel movement** opens the
previous page at its bottom or the next page at its top. If the page is
fully visible vertically, the wheel changes pages directly. Shift+wheel
pans horizontally when the page is wider than the viewport.

**Cursor Up / Cursor Down** move vertically by 72 pixels; **PageUp /
PageDown** by about 90% of the viewport height, with the same page-boundary
rules. Left/Right pan by 72 pixels whenever the page is wider than the
viewport, otherwise navigate to the previous/next page. Home/End still jump
to the first/last page. Plain + and - (including the numeric keypad)
zoom in/out without Ctrl. Left-button drag directly on the rendered page
pans horizontally and vertically, clamped at the edges without changing pages. The scrollbar indicates the vertical viewport location.

Only the visible portion of the PDF page is rasterized at zoom scale. No
large off-screen bitmap for the entire zoomed page is allocated. The
navigation transitions are independently regression-tested in
`../Common/PageScrollTest.cpp`.

**Common PDF code:** `../Common/PdfRaster.h` (also used by `../VCL`).
The PDF is loaded into memory with a 256 MiB demonstration limit so its input
bytes remain valid for PDFium, including for Unicode Windows filenames. This
is not a hardened viewer for untrusted PDFs.

## Runtime Abseil: obsolete Chromium component excluded

This application uses the installed BuildEngine Abseil package
`abseil@20260817.0` and its `libabseil_dll.dll`, **not**
`third_party_abseil-cpp_absl.dll`. The latter is a retired Chromium
component from previous PDFium GN builds and is not an application
dependency.

`CMakeLists.txt` declares
`EXCLUDE_DLLS third_party_abseil-cpp_absl.dll` in the generic BuildEngine
runtime-staging call. On the next build, staging removes that exact old DLL
from the SDL2 executable folder and ignores copies in package roots.
The canonical `libabseil_dll.dll` remains deployed; no arbitrary
wildcard exclusions, renaming or changes to the PDFium import library are
made. The staging target runs on **every** CMake build, including incremental
builds, so no forced PDFium or SDL2 source rebuild is needed.

The authoritative runtime PE contract is the PDFium BuildEngine install
gate: `pdfium.dll` must import `libabseil_dll.dll` and must **not**
import `third_party_abseil-cpp_absl.dll`. This is checked separately
from the package composition manifest.

PDFium's installed `sbom/bundled-components.json` now classifies
Abseil as an **external** BuildEngine dependency, not as
`pdfium:bundled:Abseil`. Rerun the PDFium install/metadata states to
refresh previously generated `sbom.cdx.json` and
`LICENSE-INFO.txt`; a Git update alone does not rewrite existing
installed metadata. The recorded runtime DLL in CycloneDX is a declared
identity, not a substituted PE-import verification.

## PDFium 2025.11.19 / Skia 153 runtime contract

The PDFium consumer uses **Skia 153**, not Skia 144. The exact dependency
closure is declared in [CMakeLists.txt](CMakeLists.txt), with Abseil
20260817.0, fast_float 8.2.3, zlib 1.4, libjpeg-turbo 3.2.0,
libpng 1.6.59, FreeType 2.15.0, ICU 78.3, LCMS2 2.20, OpenJPEG 2.5.4,
OpenGL 26.2.1, bzip2 1.0.8, Brotli 1.2.1, and HarfBuzz 15.1.0.

Every runtime library is taken from its exact BuildEngine package root.
The existing collision-checking stager runs on each build, excluding only
retired `third_party_abseil-cpp_absl.dll` and `skia144.dll` from the
consumer output. The correct modern binaries are `libabseil_dll.dll`
and `skia.dll`. No system PATH or alternate installed-library directory
is consulted. Skia's `icudtl.dat` is additionally copied from the
configuration-specific `skia/153/bin/win64/<configuration>` directory,
since the generic PE-DLL staging step does not handle data files.

**Tests:** In addition to the interactive `pdfium-sdl2-viewer`, this
project builds `pdfium-c-api-test` and `pdfium-page-scroll-test`.
The first creates a PDF entirely in memory and verifies PDFium's BGRA
raster output; the second runs existing page/zoom/scroll navigation
regressions without a GUI. Both run under CTest with nonzero exit on
failed assertions. They do not require a PDF fixture or graphical session.

## Build the graphical viewer (Release and Debug)

Run all commands from the BuildEngine-Tests root
(`D:\local\embarcadero\test_v3\tests`) in the RAD Studio 13 BCC64X
developer prompt. Each CMake command is a single physical `cmd.exe` line.
Run `cmake -P bootstrap/Bootstrap.cmake` when preparing a fresh checkout.

### Release

```bat
cmake -S pdfium/SDL2 -B ../build/Test/pdfium-SDL2/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-SDL2/Release
cmake --build ../build/Test/pdfium-SDL2/Release/cmake --target buildengine-stage-data-pdfium-sdl2-viewer
..\build\Test\pdfium-SDL2\Release\cmake\runtime\pdfium-sdl2-viewer\pdfium-sdl2-viewer.exe
```

### Debug

```bat
cmake -S pdfium/SDL2 -B ../build/Test/pdfium-SDL2/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-SDL2/Debug
cmake --build ../build/Test/pdfium-SDL2/Debug/cmake --target buildengine-stage-data-pdfium-sdl2-viewer
..\build\Test\pdfium-SDL2\Debug\cmake\runtime\pdfium-sdl2-viewer\pdfium-sdl2-viewer.exe
```

The dedicated target builds only the SDL2 viewer and stages its recursive
PDFium/SDL2 DLL closure plus the matching Skia 153 `icudtl.dat`. The two
headless test programs do not need to be built to launch the viewer.

For the complete Debug/Release build, install and CTest commands, see
[build.md](build.md). `ctest` executes `pdfium-c-api-render` and
`pdfium-page-scroll`, not the interactive GUI.

**Evidence status:** Build definitions and declared dependencies have
been reviewed. Native Windows BCC64X build, executable loading, pixel tests
and interactive rendering remain **[nicht verifiziert]**.
