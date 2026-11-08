# Build — PDFium viewer and regressions

Run from the BuildEngine-Tests checkout root in the RAD Studio 13 BCC64X developer command prompt. Every invocation is a single physical command line.

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S pdfium/SDL2 -B ../build/Test/pdfium-SDL2/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-SDL2/Release
cmake --build ../build/Test/pdfium-SDL2/Release/cmake
ctest --test-dir ../build/Test/pdfium-SDL2/Release/cmake --output-on-failure
cmake --install ../build/Test/pdfium-SDL2/Release/cmake
```

```bat
cmake -S pdfium/SDL2 -B ../build/Test/pdfium-SDL2/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-SDL2/Debug
cmake --build ../build/Test/pdfium-SDL2/Debug/cmake
ctest --test-dir ../build/Test/pdfium-SDL2/Debug/cmake --output-on-failure
cmake --install ../build/Test/pdfium-SDL2/Debug/cmake
```

CTest cases: `pdfium-c-api-render` and `pdfium-page-scroll`. The SDL2 graphical viewer is launched interactively after installation; CTest does not open an unattended GUI.

Use only the exact BuildEngine-installed package versions. A missing package or `skia/153/bin/win64/<configuration>/icudtl.dat` fails at configure time. DLL collisions fail at staging time. No software is rebuilt from external sources during this consumer test.

**Execution status:** [nicht verifiziert].
