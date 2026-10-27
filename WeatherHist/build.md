# WeatherHist build

Run in the RAD Studio 13 developer command prompt in `BuildEngine/tests`.

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S WeatherHist -B ../build/Test/WeatherHist/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/WeatherHist/Release
cmake --build ../build/Test/WeatherHist/Release/cmake
ctest --test-dir ../build/Test/WeatherHist/Release/cmake --output-on-failure
cmake --install ../build/Test/WeatherHist/Release/cmake
..\build\Test\WeatherHist\Release\WeatherHist.exe
```

Change Release to Debug in all paths and configuration options for the Debug build. The installed target is `../build/Test/WeatherHist/Release/WeatherHist.exe`; the uninstalled CMake runtime target lives under `.../cmake/runtime/WeatherHist/`. Use the installed EXE and its staged recursive DLL closure. No copying DLLs from PATH or the SDK, and no static OSS archives. The offline CTest does not make network requests.
