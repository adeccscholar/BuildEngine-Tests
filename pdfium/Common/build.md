# Build — PDFium page scrolling

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S pdfium/Common -B ../build/Test/pdfium-Common/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pdfium-Common/Release
```

```bat
cmake --build ../build/Test/pdfium-Common/Release/cmake
```

```bat
cmake --install ../build/Test/pdfium-Common/Release/cmake
```

```bat
ctest --test-dir ../build/Test/pdfium-Common/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.
