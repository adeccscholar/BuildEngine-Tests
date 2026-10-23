# OpenMP Galaxy

Dynamic 3D spiral galaxy, C++23, native BCC64X LLVM OpenMP DLL, hardware OpenGL 3.3, SDL2 and GLEW.

**Read [architecture.md](architecture.md) first.** It contains the mathematical model,
MathJax formulas, Mermaid architecture and sequence diagrams, and the BuildEngine
documentation-renderer acceptance criteria.

## Build and install

From `D:/local/embarcadero/test_v3/tests`:

```bat
git pull
cmake -S openmp-galaxy -B D:/local/embarcadero/test_v3/build/test/openmp-galaxy/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/openmp-galaxy/Release
cmake --build D:/local/embarcadero/test_v3/build/test/openmp-galaxy/Release/cmake
ctest --test-dir D:/local/embarcadero/test_v3/build/test/openmp-galaxy/Release/cmake --output-on-failure
cmake --install D:/local/embarcadero/test_v3/build/test/openmp-galaxy/Release/cmake
```

`Release/galaxy_viewer.exe` starts the GUI; `Release/galaxy_benchmark.exe`
provides numerical reproducibility and parallel scheduling comparisons.

## Controls

- Left mouse drag: orbit 3D camera
- Mouse wheel: zoom
- Space: pause / resume
- 1–4: serial / static / dynamic / guided
- + and -: OpenMP thread count
- R: reset seeded galaxy
- Esc: close GUI

## Stage-1 boundaries

This is a **live orbit simulation in an analytic central potential**.
It is *not* a physically self-gravitating stellar N-body system. The external-field
approach makes a compelling first visual reference without a costly O(N²) step.
Phase 2 will add a direct N-body implementation and numerical comparisons;
Phase 3 introduces Barnes–Hut. The simulation currently runs synchronously on
the GUI thread using OpenMP, so larger particle counts can stall input handling.

## Status

Sources and contracts are checked into GitHub; real BCC64X compiler/linker
validation and graphics hardware startup remain pending.
