# PDFium tests

- `SDL2/`: CMake SDL2 PDFium page viewer. Configure from Tests root with `cmake -S pdfium/SDL2 -B ../build/Test/pdfium-SDL2/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-SDL2/Release`.
- `VCL/`: RAD Studio native `PDFiumVCL.cbproj` viewer.
- `Common/`: shared raster and PageScroll sources.

Use `cmake --build ../build/Test/pdfium-SDL2/Release/cmake` and `cmake --install ../build/Test/pdfium-SDL2/Release/cmake` afterwards. DLL copying uses BuildEngineRuntime.cmake.
