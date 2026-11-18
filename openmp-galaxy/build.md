# Build — OpenMP galaxy

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S openmp-galaxy -B ../build/Test/openmp-galaxy/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/openmp-galaxy/Release
```

```bat
cmake --build ../build/Test/openmp-galaxy/Release/cmake
```

```bat
cmake --install ../build/Test/openmp-galaxy/Release/cmake
```

```bat
ctest --test-dir ../build/Test/openmp-galaxy/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.

## Launch individual applications

**galaxy_benchmark:** Deterministic galaxy-simulation benchmark.

```bat
..\build\Test\openmp-galaxy\Release\galaxy_benchmark.exe
```

**galaxy_viewer:** Interactive SDL2/OpenGL 3.3 star galaxy with CPU OpenMP updates.

```bat
..\build\Test\openmp-galaxy\Release\galaxy_viewer.exe
```

### Controls

Left-drag rotates, right-drag or arrows pan, wheel zooms. F1/F2/F3 select top/edge/oblique cameras; C resets camera; Space pauses; R resets seeded stars; 1/2/3/4 select serial/static/dynamic/guided; +/- changes threads; [ and ] change simulation speed; Esc exits. Window title reports FPS, simulation steps/s, CPU timing and GPU. The star dynamics model an analytic central potential, not N-body gravity.
