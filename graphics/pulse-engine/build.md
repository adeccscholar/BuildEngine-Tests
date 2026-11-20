# Build Pulse Engine

From the BuildEngine-Tests checkout root in a configured RAD Studio 13 BCC64X environment:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S graphics/pulse-engine -B ../build/Test/pulse-engine/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/pulse-engine/Release
```

```bat
cmake --build ../build/Test/pulse-engine/Release/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/pulse-engine/Release/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/pulse-engine/Release/cmake
```

```bat
..\build\Test\pulse-engine\Release\pulse-engine.exe
```

```bat
..\build\Test\pulse-engine\Release\pulse-engine.exe --verify
```

The verification mode works without a sound card. The interactive playback option explicitly opens the operating system's default audio device. Match Debug and Release packages and caches; do not mix CRT and DLL variants. **[not verified]** Windows BCC64X build and device acceptance are pending.
