# Native Matplot++ Visual Mathematics Gallery

Execute each command separately from the BuildEngine-Tests checkout root. All CMake commands are copyable single lines.

## Prepare

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## Configure Release

```bat
cmake -S graphics/matplot-chaos-atlas -B ../build/Test/matplot-chaos-atlas/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/matplot-chaos-atlas/Release
```

## Build

```bat
cmake --build ../build/Test/matplot-chaos-atlas/Release/cmake --parallel 4
```

## CTest

```bat
ctest --test-dir ../build/Test/matplot-chaos-atlas/Release/cmake --output-on-failure
```

## Install

```bat
cmake --install ../build/Test/matplot-chaos-atlas/Release/cmake
```

## Run gallery

```bat
..\build\Test\matplot-chaos-atlas\Release\matplot-chaos-atlas.exe
```

## Export six SVG scenes

```bat
..\build\Test\matplot-chaos-atlas\Release\matplot-chaos-atlas.exe --export
```

Use `--verify` for a headless numerical CTest. SVG files are optional output in the working directory's `artifacts/`. The application itself is the interactive presentation. Debug requires independent Debug configuration/build/install directories.

**[not verified]** New visual version requires Windows BCC64X testing.
