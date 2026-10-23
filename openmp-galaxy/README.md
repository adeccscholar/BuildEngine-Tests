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
- Right mouse drag: pan the view
- Mouse wheel: smooth zoom
- Arrow keys: pan the view
- F1/F2/F3: top-down / edge-on / angled camera
- C: reset camera
- [ and ]: halve / double simulation speed (0.125x to 8x)
- Space: pause / resume
- 1–4: serial / static / dynamic / guided
- + and -: OpenMP thread count
- R: reset seeded galaxy
- Esc: close GUI

## GPU selection and diagnostics

The viewer exports `NvOptimusEnablement=1` and `AmdPowerXpressRequestHighPerformance=1`
to **request** the discrete GPU. These hints do not guarantee adapter selection.
After SDL creates the OpenGL context, the viewer reads `GL_VENDOR`,
`GL_RENDERER` and `GL_VERSION`, writes them to the Windows debugger
with `OutputDebugStringA`, and displays `GL_RENDERER` in the title bar.
Verify that the title bar names your NVIDIA model instead of Intel or
`softpipe`. GPU *rendering* is through hardware OpenGL, but the star
*simulation* is still OpenMP on the CPU; CUDA and OpenGL compute shaders
are not used.

When Windows selects the integrated GPU, choose the application's
high-performance graphics preference under Windows Settings > System >
Display > Graphics; the ultimate choice depends on OS, GPU drivers and
display topology. This project does not deploy Mesa's `opengl32.dll`.

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
