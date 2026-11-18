# Build and run: Matplot++ Mathematical Discovery Gallery

All commands below are individual, copyable one-line commands. Execute them from the root of the BuildEngine-Tests checkout with the configured RAD Studio 13 BCC64X environment. Do not reuse another test's CMake cache.

## Toolchain bootstrap

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## Configure

```bat
cmake -S graphics/matplot-chaos-atlas -B ../build/Test/matplot-chaos-atlas/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/matplot-chaos-atlas/Release
```

## Build

```bat
cmake --build ../build/Test/matplot-chaos-atlas/Release/cmake --parallel 4
```

## Headless CTest verification

```bat
ctest --test-dir ../build/Test/matplot-chaos-atlas/Release/cmake --output-on-failure
```

## Install

```bat
cmake --install ../build/Test/matplot-chaos-atlas/Release/cmake
```

## Run the native interactive application

```bat
..\build\Test\matplot-chaos-atlas\Release\matplot-chaos-atlas.exe
```

## Verify without a graphics desktop

```bat
..\build\Test\matplot-chaos-atlas\Release\matplot-chaos-atlas.exe --verify
```

## Export six standalone SVG scenes

```bat
..\build\Test\matplot-chaos-atlas\Release\matplot-chaos-atlas.exe --export
```

SVG files are produced in the current working directory's `artifacts/` folder. The exported files are optional: the interactive gallery renders directly in the application.

## Controls and purpose

Six mathematically distinct scenes sampled by Matplot++, visualized inside a native ImGui/SDL2 gallery with labeled previews, animated drawing, zoom and optional SVG export.

Select one of six tiles and choose Explore scene. Pause, restart, adjust speed and zoom, switch to the next or previous scene, or return to the gallery. SVG export is available but not needed for live viewing.

For Debug, use a separate Debug build directory, set both `-DCMAKE_BUILD_TYPE=Debug` and `-DBUILDENGINE_CONFIGURATION=Debug` and use the matching installed package variants. Never mix Release and Debug imports.

**Verification:** Native BCC64X build and interactive operation confirmed by the operator on 2026-11-18.
