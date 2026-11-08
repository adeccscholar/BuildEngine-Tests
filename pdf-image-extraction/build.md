# Build — QPDF 12.4.2 image extraction

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S pdf-image-extraction -B ../build/Test/pdf-image-extraction/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pdf-image-extraction/Release
cmake --build ../build/Test/pdf-image-extraction/Release/cmake
cmake --install ../build/Test/pdf-image-extraction/Release/cmake
ctest --test-dir ../build/Test/pdf-image-extraction/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. This project has a registered noninteractive `qpdf-image-extraction` CTest case and uses the BuildEngine QPDF 12.4.2, zlib 1.4 and libjpeg-turbo 3.2.0 package roots. Native Windows testing is [nicht verifiziert].
