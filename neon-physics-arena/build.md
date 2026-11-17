# Build — Neon Physics Arena

Open a RAD Studio 13 developer command prompt in `D:\local\embarcadero\test_v3\tests` (BuildEngine-Tests checkout).

Both `sdl3@3.4.20` and `box2d@3.2.0` must already be installed in BuildEngine, including Release and Debug DLL/import artifacts.

## Release

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S neon-physics-arena -B ../build/Test/neon-physics-arena/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/neon-physics-arena/Release
cmake --build ../build/Test/neon-physics-arena/Release/cmake --parallel 4
ctest --test-dir ../build/Test/neon-physics-arena/Release/cmake --output-on-failure
cmake --install ../build/Test/neon-physics-arena/Release/cmake
..\build\Test\neon-physics-arena\Release\neon-physics-arena.exe
```

## Debug

```bat
cmake -S neon-physics-arena -B ../build/Test/neon-physics-arena/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/neon-physics-arena/Debug
cmake --build ../build/Test/neon-physics-arena/Debug/cmake --parallel 4
ctest --test-dir ../build/Test/neon-physics-arena/Debug/cmake --output-on-failure
cmake --install ../build/Test/neon-physics-arena/Debug/cmake
..\build\Test\neon-physics-arena\Debug\neon-physics-arena.exe
```

## Headless regression and stress

```bat
ctest --test-dir ../build/Test/neon-physics-arena/Release/cmake -R NeonPhysicsDeterminism --output-on-failure -V
ctest --test-dir ../build/Test/neon-physics-arena/Release/cmake -R NeonPhysicsStress --output-on-failure -V
..\build\Test\neon-physics-arena\Release\neon-physics-verify.exe --stress
```

Do not run the GUI in a service session without a Windows desktop. The console verifier requires no video initialization.

For screenshots launch the GUI in an interactive session, press `P` and inspect the generated `neon-physics-arena.bmp` in the working directory. It is captured from the real SDL3 renderer.

Expected CMake packages:
`install/packages/sdl3/3.4.20/lib/cmake/SDL3/SDL3Config.cmake` and
`install/packages/box2d/3.2.0/lib/cmake/box2d/box2dConfig.cmake`, with exact variant DLL and import paths.

**[not verified]** Native test results are not claimed until the Windows host completes this workflow.
