# Build and run: Dear ImGui Neon Laboratory

All commands below are individual, copyable one-line commands. Execute them from the root of the BuildEngine-Tests checkout with the configured RAD Studio 13 BCC64X environment. Do not reuse another test's CMake cache.

## Toolchain bootstrap

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## Configure

```bat
cmake -S graphics/imgui-neon-lab -B ../build/Test/imgui-neon-lab/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/imgui-neon-lab/Release
```

## Build

```bat
cmake --build ../build/Test/imgui-neon-lab/Release/cmake --parallel 4
```

## Headless CTest verification

```bat
ctest --test-dir ../build/Test/imgui-neon-lab/Release/cmake --output-on-failure
```

## Install

```bat
cmake --install ../build/Test/imgui-neon-lab/Release/cmake
```

## Run the native interactive application

```bat
..\build\Test\imgui-neon-lab\Release\imgui-neon-lab.exe
```

## Verify without a graphics desktop

```bat
..\build\Test\imgui-neon-lab\Release\imgui-neon-lab.exe --verify
```

## Controls and purpose

Interactive native SDL2/ImGui particle observatory with four animated scientific scenes and real font-texture rendering.

Choose Nebula Vortex, Quantum Orbit, Wave Interference or Stellar Bloom; pause/resume and reset the animation; adjust the zoom slider; F11 switches to fullscreen.

For Debug, use a separate Debug build directory, set both `-DCMAKE_BUILD_TYPE=Debug` and `-DBUILDENGINE_CONFIGURATION=Debug` and use the matching installed package variants. Never mix Release and Debug imports. **[not verified]** New native GUI compilation, frame rate and visual quality await BCC64X host verification.
