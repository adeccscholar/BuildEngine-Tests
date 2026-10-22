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
`../PDFiumCommon/PageScrollTest.cpp`.

**Common PDF code:** ../PDFiumCommon/PdfRaster.h (also used by Test/PDFiumVCL).
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

## Configure / build

Use the BCC64X RAD Studio 13 environment and the *exact* matching installed
BuildEngine package artifacts. Package roots are required by the generic
BuildEngine runtime stage; do not replace them with system PATH entries.

The BCC64X toolchain reads the **environment variables** `CB_BDS` and
`CB_BCC64X` during CMake configuration (`ENV{CB_BDS}` and
`ENV{CB_BCC64X}`). A `-DCB_BDS=...` CMake cache argument is **not**
sufficient. Set both in the **same cmd.exe session** that runs configure and
build; use `set "NAME=value"` so the path with spaces is handled correctly.
All CMake invocations below are **single physical cmd.exe lines** (no caret
continuations).

The following cmd.exe example assumes the production root and uses the
configuration-matched package DLLs. Add the complete PDFium transitive
runtime closure when it differs; refer to admin/build-libraries.xml for pinned
versions. No separate PDFium/SDL2 sources are built.

~~~bat
set "CB_BDS=C:\Program Files (x86)\Embarcadero\Studio\37.0"
set "CB_BCC64X=%CB_BDS%\bin64\bcc64x.exe"
set "ROOT=D:\local\embarcadero\test_v3"
set "PKG=%ROOT%\install\packages"
set "ROOTS=%PKG%\pdfium\2025.11.19;%PKG%\sdl2\2.32.10;%PKG%\skia\144;%PKG%\abseil\20260817.0;%PKG%\icu\78.3;%PKG%\freetype\2.14.3;%PKG%\harfbuzz\14.4.0;%PKG%\brotli\1.2.0;%PKG%\bzip2\1.0.8;%PKG%\libpng\1.6.58;%PKG%\zlib\1.3.2;%PKG%\libjpeg-turbo\3.2.0;%PKG%\lcms2\2.19.1;%PKG%\openjpeg\2.5.4"
cmake -S Test/PDFiumSDL2 -B build/pdfium-sdl2/Release -G Ninja "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=%ROOT%\admin\cmake\toolchains\bcc64x-buildengine-cxx.cmake" "-DCMAKE_MAKE_PROGRAM:FILEPATH=%ROOT%\tools\ninja\1.13.2\ninja.exe" "-DADECC_RUNTIME_CMAKE_DIR:PATH=%ROOT%\admin\cmake\consumer" "-DADECC_RUNTIME_PACKAGE_ROOTS:STRING=%ROOTS%" "-DPDFium_DIR:PATH=%PKG%\pdfium\2025.11.19\lib\cmake\PDFium" "-DSDL2_DIR:PATH=%PKG%\sdl2\2.32.10\lib\cmake\SDL2"
cmake --build build/pdfium-sdl2/Release
~~~ 

For Debug in the same initialized cmd.exe session, use this **single-line**
configure command followed by the build:

~~~bat
cmake -S Test/PDFiumSDL2 -B build/pdfium-sdl2/Debug -G Ninja "-DCMAKE_BUILD_TYPE:STRING=Debug" "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=%ROOT%\admin\cmake\toolchains\bcc64x-buildengine-cxx.cmake" "-DCMAKE_MAKE_PROGRAM:FILEPATH=%ROOT%\tools\ninja\1.13.2\ninja.exe" "-DADECC_RUNTIME_CMAKE_DIR:PATH=%ROOT%\admin\cmake\consumer" "-DADECC_RUNTIME_PACKAGE_ROOTS:STRING=%ROOTS%" "-DPDFium_DIR:PATH=%PKG%\pdfium\2025.11.19\lib\cmake\PDFium" "-DSDL2_DIR:PATH=%PKG%\sdl2\2.32.10\lib\cmake\SDL2"
cmake --build build/pdfium-sdl2/Debug
~~~

The
runtime stage copies/validates exact Win64x DLLs beside the EXE with collision
checks. The application does not need the SDL2main startup library: its own
WinMain calls SDL_SetMainReady.

**Evidence:** the PDFium+SDL2 package smoke checks passed separately; the
graphical application must still be compiled and run with native BCC64X
[not verified].
