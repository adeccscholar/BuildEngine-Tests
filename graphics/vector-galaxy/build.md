# Build Vector Galaxy

All commands run from the root of the BuildEngine-Tests checkout in the BCC64X environment.

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S graphics/vector-galaxy -B ../build/Test/vector-galaxy/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/vector-galaxy/Release
```

```bat
cmake --build ../build/Test/vector-galaxy/Release/cmake --parallel 4
```

```bat
ctest --test-dir ../build/Test/vector-galaxy/Release/cmake --output-on-failure
```

```bat
cmake --install ../build/Test/vector-galaxy/Release/cmake
```

```bat
..\build\Test\vector-galaxy\Release\vector-galaxy.exe
```

```bat
..\build\Test\vector-galaxy\Release\vector-galaxy.exe --verify
```

For Debug builds use an independent directory, `-DCMAKE_BUILD_TYPE=Debug`, and `-DBUILDENGINE_CONFIGURATION=Debug` and do not mix binary variants.

**[not verified]** Native BCC64X build and desktop run are pending.
