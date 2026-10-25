# Build — PDFium viewer SDL2

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S pdfium/SDL2 -B ../build/Test/pdfium-SDL2/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-SDL2/Release
cmake --build ../build/Test/pdfium-SDL2/Release/cmake
cmake --install ../build/Test/pdfium-SDL2/Release/cmake
ctest --test-dir ../build/Test/pdfium-SDL2/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.
