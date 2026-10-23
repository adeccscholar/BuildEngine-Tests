# OpenMP Mandelbrot — BuildEngine consumer

C++23 CPU Mandelbrot computation with the **managed native LLVM OpenMP DLL**
(llvm-openmp 20.1.7 from the BuildEngine XML contract). SDL2 supplies the
interactive window; no Mesa OpenGL runtime is staged.

## Configure, build, install

From `D:/local/embarcadero/test_v3/tests`:

```bat
cmake -S openmp-mandelbrot -B D:/local/embarcadero/test_v3/build/test/openmp-mandelbrot/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/openmp-mandelbrot/Release
cmake --build D:/local/embarcadero/test_v3/build/test/openmp-mandelbrot/Release/cmake
ctest --test-dir D:/local/embarcadero/test_v3/build/test/openmp-mandelbrot/Release/cmake --output-on-failure
cmake --install D:/local/embarcadero/test_v3/build/test/openmp-mandelbrot/Release/cmake
```

Executables and dependency DLLs are installed directly in `Release/`.

## Benchmark

`openmp_mandelbrot_benchmark.exe [width height iterations threads]`

Compares serial, static, dynamic and guided schedules; validates complete
pixel arrays and iteration totals, prints timings and speedup.
The fixed workload and identical output are reproducible; wall-clock timings
depend on hardware and system load.

## Viewer

`openmp_mandelbrot_viewer.exe` is a native GUI application without a console.
Use mouse wheel to zoom, arrow keys to pan, 1/2/3/4 for
serial/static/dynamic/guided, and +/- for thread count. Esc quits.
The title displays CPU compute time and effective thread count.

The compute kernel and SDL presentation are separate. Updates currently
compute synchronously: long zooms can briefly block SDL events. Future work
could use asynchronous double buffering and generation-based cancellation.

## Status

Source and BuildEngine integration committed. Native BCC64X execution remains
to be verified on Windows; no performance figures are claimed.
