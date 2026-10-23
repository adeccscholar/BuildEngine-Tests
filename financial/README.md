# Financial demo — BCC64X / BuildEngine

Original financial C++ source migrated from Demo.zip. Uses managed Boost 1.92.0, pugixml 1.16 and OpenSSL 3.5.8.

Install prefix: `D:/local/embarcadero/test_v3/build/test/financial/Release/`.
Run CMake from `D:/local/embarcadero/test_v3/tests`:

```bat
cmake -S financial -B D:/local/embarcadero/test_v3/build/test/financial/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/financial/Release
cmake --build D:/local/embarcadero/test_v3/build/test/financial/Release/cmake
cmake --install D:/local/embarcadero/test_v3/build/test/financial/Release/cmake
```

`financial_demo.exe`, runtime DLLs, and `Tests/*.json` install under the Release prefix. The JSON reader resolves bare filenames relative to `Release/Tests` if they are absent in the working directory.

This project has **not** been compiled or run with BCC64X in this environment.
