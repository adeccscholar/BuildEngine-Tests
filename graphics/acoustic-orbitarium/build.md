# Build Acoustic Orbitarium
Run from BuildEngine-Tests root under RAD Studio 13/BCC64X. The OpenAL package import library must be taken from the exact native Release or Debug producer, not Windows SDK or a second compiler.
```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S graphics/acoustic-orbitarium -B ../build/Test/acoustic-orbitarium/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/acoustic-orbitarium/Release
cmake --build ../build/Test/acoustic-orbitarium/Release/cmake --parallel 4
ctest --test-dir ../build/Test/acoustic-orbitarium/Release/cmake --output-on-failure
cmake --install ../build/Test/acoustic-orbitarium/Release/cmake
..\build\Test\acoustic-orbitarium\Release\acoustic-orbitarium.exe
```
Headless CTest never requires speakers. Click to enable actual audio after GUI launch. Use distinct Debug paths and matching native packages.
**[not verified]** The application and its sound playback are not yet accepted under BCC64X.