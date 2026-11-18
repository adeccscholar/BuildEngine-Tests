# VTK Scientific Visualization Laboratory

**Test purpose:** Interactive analytical isosurfaces and GPU volume rendering with deterministic VTK geometry and TBB/OpenMP stress checks.

A native **VTK 9.7.2 / BCC64X / Win64** interactive 3D visualization with
two live viewports, analytical isosurface geometry, wireframe detail and
GPU volume ray casting. No fake screenshot, static background image,
OpenGL replacement renderer or fallback canvas.

The left view renders an illuminated, deformed 3D isosurface and translucent
wireframe; the right view runs VTK's actual `vtkGPUVolumeRayCastMapper`
using color/opacity transfer functions. Smooth gradient backgrounds and
interactive trackball navigation are provided by VTK RenderingOpenGL2.

The same executable supports headless, deterministic numerical checks:
`vtkSMPTools` fills a regular voxel field, `vtkFlyingEdges3D` extracts a
known sphere, `vtkMassProperties` checks area/volume, connectivity and
feature-edge filters verify manifold closure, and configurable repeated
runs report timings and triangle counts.

`--backend Sequential|TBB|OpenMP` selects native VTK SMP backends;
one backend is used per process. Numerical results are independent of
desktop graphics. GPU screenshot tests require a real Windows graphics
desktop with OpenGL capability; they are opt-in at CMake configure time.
The GPU path is never silently converted into a CPU rasterizer.

See [Build commands](build.md), [License](LICENSE.md) and [SBOM](SBOM.md).

## Applications, execution modes and controls

**`vtk-visualization.exe`** — Interactive dual-viewport VTK scientific isosurface and volume renderer.

The viewer shows side-by-side VTK OpenGL2 isosurface and GPU volume rendering, with native VTK interactor camera controls. Use mouse drag to rotate, wheel to zoom and the VTK interaction gestures to inspect geometry. `--resolution` sets sample density, `--backend` chooses Sequential, TBB or OpenMP. `--verify` performs headless geometric checks, `--stress` repeats computational loads, and `--screenshot` saves a rendered PNG.
