# Build — OpenMP Mandelbrot

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S openmp-mandelbrot -B ../build/Test/openmp-mandelbrot/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/openmp-mandelbrot/Release
```

```bat
cmake --build ../build/Test/openmp-mandelbrot/Release/cmake
```

```bat
cmake --install ../build/Test/openmp-mandelbrot/Release/cmake
```

```bat
ctest --test-dir ../build/Test/openmp-mandelbrot/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.

## Launch individual applications

**openmp_mandelbrot_benchmark:** Deterministic serial/static/dynamic/guided numerical benchmark.

```bat
..\build\Test\openmp-mandelbrot\Release\openmp_mandelbrot_benchmark.exe 1280 720 512 8
```

**openmp_mandelbrot_viewer:** Interactive SDL2 Mandelbrot and Julia explorer.

```bat
..\build\Test\openmp-mandelbrot\Release\openmp_mandelbrot_viewer.exe
```

### Controls

Mouse wheel zooms; arrows pan. 1/2/3/4 select serial/static/dynamic/guided OpenMP scheduling; J switches Mandelbrot/Julia; PageUp/PageDown doubles/halves iteration limit; +/- adjusts thread count; R resets the current fractal; Esc exits. The window title displays effective thread count, CPU time and current parameters.
