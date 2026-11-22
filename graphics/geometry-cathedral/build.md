# Build Geometry Cathedral
Run from BuildEngine-Tests root in the configured C++Builder 13 BCC64X shell. Only exact managed packages and version-specific CGAL/Assimp CMake exports are permitted.
**Bootstrap and validate the managed toolchain:**

```bat
cmake -P bootstrap/Bootstrap.cmake
```

**Configure Release with Ninja and the BuildEngine initial cache:**

```bat
cmake -S graphics/geometry-cathedral -B ../build/Test/geometry-cathedral/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/geometry-cathedral/Release
```

**Compile the native application:**

```bat
cmake --build ../build/Test/geometry-cathedral/Release/cmake --parallel 4
```

**Execute the registered CTest checks:**

```bat
ctest --test-dir ../build/Test/geometry-cathedral/Release/cmake --output-on-failure
```

**Install the executable and exact runtime closure:**

```bat
cmake --install ../build/Test/geometry-cathedral/Release/cmake
```

**Launch the interactive showcase:**

```bat
..\build\Test\geometry-cathedral\Release\geometry-cathedral.exe
```
Debug requires its own configured directory and managed Debug DLL closure. Native GUI F11 and F1 follow the common showcase policy.
**[not verified]** Awaiting genuine BCC64X application execution.