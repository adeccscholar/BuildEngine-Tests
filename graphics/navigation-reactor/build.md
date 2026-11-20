# Build Navigation Reactor

Use the configured BCC64X toolchain, from the BuildEngine-Tests checkout root.

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S graphics/navigation-reactor -B ../build/Test/navigation-reactor/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/navigation-reactor/Release
```

```bat
cmake --build ../build/Test/navigation-reactor/Release/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/navigation-reactor/Release/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/navigation-reactor/Release/cmake
```

```bat
..\build\Test\navigation-reactor\Release\navigation-reactor.exe
```

```bat
..\build\Test\navigation-reactor\Release\navigation-reactor.exe --verify
```

Debug and Release must use separate CMake caches and exact matching managed import DLL variants. **[not verified]** Native Windows build and visual operation are pending.
