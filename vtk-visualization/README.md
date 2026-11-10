# BuildEngine VTK 9.7.2 Visualization

The **VTK Scientific Visualization Laboratory** is a standalone native
BCC64X/C++23 Windows project. Its interactive 1600 × 900 scene combines a
lit isosurface and transparent wireframe with GPU volume raycasting in two
live VTK OpenGL2 viewports. The executable also performs headless mathematical
verification, multi-backend SMP tests, and repeatable CPU load measurements.

## Quick start

Use a RAD Studio 13 developer prompt in the **BuildEngine-Tests root**
(`D:\local\embarcadero\test_v3\tests`). Complete the BuildEngine VTK 9.7.2
rendering build and installation first.

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S vtk-visualization -B ../build/Test/vtk-visualization/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/vtk-visualization/Release
cmake --build ../build/Test/vtk-visualization/Release/cmake --parallel 4
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake --output-on-failure
cmake --install ../build/Test/vtk-visualization/Release/cmake
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --resolution 161 --backend TBB
```

## Tests and run modes

| Mode | Details |
| --- | --- |
| `--verify --resolution 129 --backend Sequential` | Analytic sphere geometry, topology, volume and area |
| `--verify --resolution 129 --backend TBB` | Same verification with oneTBB |
| `--verify --resolution 129 --backend OpenMP` | Same verification with LLVM OpenMP |
| `--stress --resolution 257 --iterations 6 --backend TBB` | Repeated geometric workload; performance records |
| `--screenshot vtk-lab.png --resolution 161 --backend TBB` | Render actual VTK OpenGL2 scene to PNG |
| `--resolution 161 --backend TBB` | Interactive dual-viewport application |

CTest registers `VTK972Geometry`, `VTK972TBB`, `VTK972OpenMP` and
`VTK972Load` by default. `VTK972Screenshot` is opt-in via
`-DVTK_ENABLE_GRAPHICS_CTEST=ON` and needs an interactive Windows
desktop with a functional OpenGL driver. The numerical tests do not
require a graphics desktop.

```bat
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -N
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -R VTK972Load --output-on-failure -V
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --stress --resolution 257 --iterations 6 --backend OpenMP
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --screenshot vtk-lab.png --resolution 161 --backend TBB
```

## Exact dependencies and GPU selection

Requires installed active `vtk@9.7.2`, `onetbb@2023.2.0`,
`llvm-openmp@20.1.7`, and `opengl@26.2.1`. The Mesa package supplies
the managed build-time OpenGL import library, while the installed
executable deliberately does not stage Mesa `opengl32.dll`: the native
Windows graphics driver handles runtime OpenGL dispatch.
The actual graphics acceleration is host-dependent and not assumed.

See the [root test README](../README.md#vtk-972-interactive-opengl2-visualization-and-stress-tests)
for full Release/Debug CMake commands, screenshot CTest configuration,
test evidence and troubleshooting. Also see [build details](build.md),
[architecture and algorithms](Description.md), [license](LICENSE.md),
and [SBOM declaration](SBOM.md).

**[not verified]** Native BCC64X build, OpenGL2 screenshots, and measured
performance remain pending validation on the target Windows machine.
