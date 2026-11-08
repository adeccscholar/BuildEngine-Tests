# Build — PDFium SDL2 viewer and regression tests

Run from `D:\local\embarcadero\test_v3\tests` (the BuildEngine-Tests checkout)
in the RAD Studio 13 BCC64X developer command prompt. The build uses the
already installed **PDFium 2025.11.19**, **SDL2 2.32.10** and **Skia 153**;
no third-party library sources are rebuilt here.

All commands below are **single physical cmd.exe lines**. CMake uses separate
Debug and Release binary directories. When first setting up the checkout,
run:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## Build and launch the graphical PDF viewer only

The executable is `pdfium-sdl2-viewer.exe`. Use the dedicated
`buildengine-stage-data-pdfium-sdl2-viewer` target, which builds and links
the viewer and then stages its isolated recursive PE/DLL runtime closure
and the matching Skia `icudtl.dat`. It does **not** build the unrelated
`pdfium-c-api-test` or `pdfium-page-scroll-test` executables.

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

The viewer starts with an empty SDL window. Choose **Open PDF** or press
**Ctrl+O** to select a document using the native Windows file picker.
It supports previous/next page, direct page selection, zoom, wheel scrolling
and mouse drag/pan. PDFium renders the page and SDL2 presents the pixels.
The application has no VCL dependency.

## Build all programs and run automated CTest regressions

To build the graphical viewer **and both headless test programs**, use the
default build target. CTest runs only the registered noninteractive tests:
`pdfium-c-api-render` and `pdfium-page-scroll`; it does not launch the
graphical viewer. The following commands assume the corresponding configure
command above was already run.

### Release

```bat
cmake --build ../build/Test/pdfium-SDL2/Release/cmake
ctest --test-dir ../build/Test/pdfium-SDL2/Release/cmake --output-on-failure
cmake --install ../build/Test/pdfium-SDL2/Release/cmake
```

### Debug

```bat
cmake --build ../build/Test/pdfium-SDL2/Debug/cmake
ctest --test-dir ../build/Test/pdfium-SDL2/Debug/cmake --output-on-failure
cmake --install ../build/Test/pdfium-SDL2/Debug/cmake
```

Each executable is isolated under
`../build/Test/pdfium-SDL2/<configuration>/cmake/runtime/<target>/`.
Runtime libraries are selected from exact BuildEngine package roots through
the recursive PE import closure, with DLL collision and missing-dependency
checks retained. For the graphical viewer and PDFium C API test,
`icudtl.dat` comes from the matching Skia 153 Debug/Release package.
The navigation regression does not stage PDFium, Skia or SDL2.

**Verification status:** the project wiring and commands are source-reviewed;
native Windows BCC64X viewer execution and CTest results remain
**[nicht verifiziert]** until the tests run locally.
