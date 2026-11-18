# Build and run: ImPlot Real-Time Signal Theater

All commands below are individual, copyable one-line commands. Execute them from the root of the BuildEngine-Tests checkout with the configured RAD Studio 13 BCC64X environment. Do not reuse another test's CMake cache.

## Toolchain bootstrap

```bat
cmake -P bootstrap/Bootstrap.cmake
```

## Configure

```bat
cmake -S graphics/implot-signal-theater -B ../build/Test/implot-signal-theater/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/implot-signal-theater/Release
```

## Build

```bat
cmake --build ../build/Test/implot-signal-theater/Release/cmake --parallel 4
```

## Headless CTest verification

```bat
ctest --test-dir ../build/Test/implot-signal-theater/Release/cmake --output-on-failure
```

## Install

```bat
cmake --install ../build/Test/implot-signal-theater/Release/cmake
```

## Run the native interactive application

```bat
..\build\Test\implot-signal-theater\Release\implot-signal-theater.exe
```

## Verify without a graphics desktop

```bat
..\build\Test\implot-signal-theater\Release\implot-signal-theater.exe --verify
```

## Controls and purpose

Full-screen native scientific oscilloscope with four live, continuously recomputed ImPlot waveform models, readable axes and calibrated signal controls.

Select Interference, Modulated Waves, Wave Packets or Frequency Drift. Frequency, amplitude and speed are adjustable; toggle each signal, pause or reset the simulation; F11 switches to fullscreen.

For Debug, use a separate Debug build directory, set both `-DCMAKE_BUILD_TYPE=Debug` and `-DBUILDENGINE_CONFIGURATION=Debug` and use the matching installed package variants. Never mix Release and Debug imports.

**Verification:** Native BCC64X build and interactive operation confirmed by the operator on 2026-11-18.
