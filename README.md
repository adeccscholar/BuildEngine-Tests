# BuildEngine Tests

This repository can be checked out under the BuildEngine root as `tests` (or `Test`). Every application is a separate project directly below this directory. There is no test/demo split and no top-level multi-project configure required.

## Common bootstrap, independent per-project CMake commands

Run from the checkout directory (e.g. `BuildEngine/tests`), in the RAD Studio 13 developer command prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

The bootstrap checks BCC64X, BuildEngine's existing Ninja and toolchain, and validates the tracked `bootstrap/BuildEngineInit.cmake` initial-cache file. It does not build libraries or download tools.

Then configure, build and install any particular project separately:

```bat
cmake -S CompareTest -B ../build/Test/CompareTest/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CompareTest/Release
cmake --build ../build/Test/CompareTest/Release/cmake
cmake --install ../build/Test/CompareTest/Release/cmake
```

For `pdfium/SDL2` use its own independent build tree, e.g. `../build/Test/pdfium-SDL2/Release/cmake`. For `pdf-image-extraction` use `../build/Test/pdf-image-extraction/Release/cmake`. Never share a CMake binary directory across different source projects. For Debug use a `Debug` build tree and `-DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug`.

A CMake app is installed directly under `../build/Test/<project>/<configuration>/` with its executable and staged DLLs side-by-side. The shared functions `adecc_stage_runtime()` and `adecc_install_application()` originate from BuildEngine-Admin. Each app must declare **the complete exact package-root closure**, including transitive dependencies. No PATH-based newest-package probing is allowed.

## One-time cache reset after moving sources

If CMake reports `source ... does not match the source ... used to generate cache`, remove **only that project's** old build directory, e.g. from the checkout directory:

```bat
cmake -E rm -rf ../build/Test/CompareTest/Release/cmake
```

Then rerun the project's configure command. Do not delete library build caches or reinstall third-party packages.

## VTK 9.7.2: interactive OpenGL2 visualization and stress tests

The standalone [VTK Scientific Visualization Laboratory](vtk-visualization/) is a
native C++23 / BCC64X consumer of the exact VTK 9.7.2 package built by
BuildEngine. It combines an interactive, two-viewport **1600 × 900**
scientific visualization with automated numerical regression and load testing:

- **Left viewport:** an illuminated, smoothly deformed 3D isosurface with
  translucent wireframe, generated with `vtkFlyingEdges3D` and VTK's OpenGL2 renderer.
- **Right viewport:** real volumetric rendering through
  `vtkGPUVolumeRayCastMapper`, with color and opacity transfer functions.
- **Numerical verification:** analytic spherical level set, mesh connectivity,
  closed-manifold boundary checks, volume and area measurements, and a 3.5%
  tolerance against the mathematical reference values.
- **Parallel execution:** separately selected `Sequential`, `TBB`, and `OpenMP`
  backends; repeatable load runs report geometry sizes and elapsed time.
- **Screenshot output:** genuine VTK framebuffer capture to PNG.

VTK supplies the rendering pipeline; the program does not substitute a
custom OpenGL renderer or static illustration. The library profile requests
`RenderingOpenGL2`, `RenderingVolumeOpenGL2`, `RenderingFreeType`,
`RenderingAnnotation`, `InteractionStyle`, `FiltersSources`, and `IOImage`.

### Requirements and build layout

Run the commands **from the BuildEngine-Tests repository root**, for example
from `D:\local\embarcadero\test_v3\tests`, in a RAD Studio 13/BCC64X developer
command prompt. Use the BuildEngine-managed CMake and Ninja tools and its
tracked initial cache; do not install or select unrelated system packages.

| BuildEngine package | Required version | Purpose |
| --- | --- | --- |
| `vtk` | `9.7.2` | Data processing, OpenGL2 rendering, GPU volume mapping |
| `onetbb` | `2023.2.0` | Native VTK TBB backend |
| `llvm-openmp` | `20.1.7` | Native VTK OpenMP backend |
| `opengl` | `26.2.1` | Managed Mesa/WGL OpenGL import library for linking |

The CMake project reads the active package versions from
`admin/build-libraries.xml` and resolves their exact installed roots.
The Mesa import library is a build-time dependency. The installed interactive
application explicitly excludes a locally bundled Mesa `opengl32.dll`, so
Windows resolves the system OpenGL driver; hardware acceleration still depends
on the actual host driver and must be verified on the target machine.

Build and install directories are **independent for Release and Debug**:

```text
../build/Test/vtk-visualization/Release/cmake   # CMake + Ninja output
../build/Test/vtk-visualization/Release         # installed executable + DLL closure
../build/Test/vtk-visualization/Debug/cmake     # separate Debug CMake + Ninja output
../build/Test/vtk-visualization/Debug           # separate Debug installation
```

### Release: bootstrap, configure, build, test, install

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S vtk-visualization -B ../build/Test/vtk-visualization/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/vtk-visualization/Release
cmake --build ../build/Test/vtk-visualization/Release/cmake --parallel 4
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake --output-on-failure
cmake --install ../build/Test/vtk-visualization/Release/cmake
```

### Debug: configure, build, test, install

```bat
cmake -S vtk-visualization -B ../build/Test/vtk-visualization/Debug/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug -DCMAKE_INSTALL_PREFIX=../build/Test/vtk-visualization/Debug
cmake --build ../build/Test/vtk-visualization/Debug/cmake --parallel 4
ctest --test-dir ../build/Test/vtk-visualization/Debug/cmake --output-on-failure
cmake --install ../build/Test/vtk-visualization/Debug/cmake
```

### Automated CTest coverage

| CTest name | Workload | Desktop/graphics required? |
| --- | --- | --- |
| `VTK972Geometry` | 129³ voxel sphere, `Sequential`, analytic geometry | No |
| `VTK972TBB` | 129³ voxel sphere, TBB backend | No |
| `VTK972OpenMP` | 129³ voxel sphere, OpenMP backend | No |
| `VTK972Load` | 193³ voxels, four iterations, TBB; 240-second timeout | No |
| `VTK972Screenshot` | Render a 129³ volume and save `VTK972.png` | **Yes**; opt-in |

Inspect registered tests or run only the parallel and stress cases:

```bat
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -N
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -R "VTK972(TBB|OpenMP)" --output-on-failure -V
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -R VTK972Load --output-on-failure -V
```

For more reproducible performance comparisons, run backend benchmarks
**sequentially** rather than launching multiple CPU-saturating backends in
parallel through `ctest -j`. The load test checks correctness and reports
timings; it does not enforce a machine-dependent performance threshold.

### Launch the interactive visualization

After Release installation:

```bat
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --resolution 161 --backend TBB
```

The two real VTK viewports share the application window and support camera
interaction with the trackball style. This command requires a Windows desktop
session and working OpenGL context, unlike the headless numerical CTests.

### Explicit parallel load and benchmark runs

```bat
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --stress --resolution 257 --iterations 6 --backend TBB
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --stress --resolution 257 --iterations 6 --backend OpenMP
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --verify --resolution 129 --backend Sequential
```

Each backend is selected once per process. A larger voxel grid increases both
numerical workload and memory use; adjust `--resolution` and `--iterations`
to the target machine rather than relying on an arbitrary run-time threshold.
Valid ranges are 33–321 grid points per axis and 1–100 iterations.

### PNG screenshot and optional graphics CTest

```bat
..\build\Test\vtk-visualization\Release\vtk-visualization.exe --screenshot vtk-lab.png --resolution 161 --backend TBB
```

To register the screenshot test in an **already configured** Release build,
enable the graphics option and rebuild before invoking CTest:

```bat
cmake -S vtk-visualization -B ../build/Test/vtk-visualization/Release/cmake -DVTK_ENABLE_GRAPHICS_CTEST=ON
cmake --build ../build/Test/vtk-visualization/Release/cmake --parallel 4
ctest --test-dir ../build/Test/vtk-visualization/Release/cmake -R VTK972Screenshot --output-on-failure -V
```

The generated CTest screenshot is
`../build/Test/vtk-visualization/Release/cmake/VTK972.png`.
This graphics test is **OFF by default**, since Windows services and
headless workers may not provide a usable graphics desktop.

### Evidence, troubleshooting, and verification state

The numerical tests print `VTK|METRIC|`, `VTK|CHECK|`, and `VTK|RESULT|`
records, including backend, grid resolution, region count, boundary edges,
triangle count, area, volume, and elapsed time. Failed CTest output can be
reviewed with `--output-on-failure` or `-V` for full verbose output.

If CMake cannot locate VTK, TBB, or OpenMP, confirm that the matching
Release/Debug package artifacts exist, and that the BuildEngine rendering
stack has been rebuilt and installed. Do **not** silently use another VTK,
oneTBB, OpenMP, or OpenGL installation from the workstation.

**[not verified]** The expanded native BCC64X rendering build, actual GPU
screenshots, and reported performance measurements have not yet been
validated against a completed Windows test run. Declared CTest cases are
not proof of a successful native execution.

See [project README](vtk-visualization/README.md),
[detailed build instructions](vtk-visualization/build.md),
[technical description](vtk-visualization/Description.md), and
[SBOM scope](vtk-visualization/SBOM.md).

## Current project status

- `pdfium/SDL2`: updated to Skia 153; headless PDFium API/render and navigation CTest consumers added; Win64x execution pending.
- `pdfium/Common`: shared headers and a standalone PageScroll regression source (not its own CMake project).
- `pdfium/VCL`: native RAD Studio `.cbproj`; not configured by CMake.
- `pdf-image-extraction`: standalone CMake target, qpdf runtime closure still needs validation.
- `CompareTest`: CMake target migrated, **but the complete original C++ source set is still missing from this Git repository**. The latest source archive must still be added; until then it cannot configure or build.
- No successful full BCC64X build/install is claimed.

BuildEngine's original `Test/` copies should not be considered deleted or migrated completely until these gaps are closed.

## Historical examples (Demo.zip, 2026-10-22)

The uploaded archive was inspected and contains these independent legacy projects: `financial` (including `financial/test`, `test/vcl`, and `test/vcl-skia`), `opengl-sdl2-earth`, `skia-direct-showcase`, and `thirdparty-showcase`. Their original C++ files are not yet imported into GitHub. The archive also lacks graphics assets referenced by the OpenGL Earth and Skia showcase projects, and a `MainForm.dfm`/`app.ico` referenced by the third-party showcase. The old CMake definitions use `TP_CMAKE` and local vendor build folders, which must be replaced with BuildEngine installed dependencies before declaring them buildable. Do not run the old local third-party CMake scripts as part of the BuildEngine tests.


## License and adecc-core-lib

Project-authored sources and documentation in this repository are released under
[PolyForm Noncommercial License 1.0.0](LICENSE), from the date of this update.
Earlier revisions distributed under MIT retain the rights granted for those
revisions; this change does not revoke previous grants. Third-party materials
and code with separate author or vendor licenses remain under their own terms.

The reusable header-only [adecc-core-lib](https://github.com/adeccscholar/adecc-core-lib)
is a **separate project**, not automatically relicensed by BuildEngine-Tests.
Its public license is PolyForm Noncommercial 1.0.0. Qualifying purchasers of
*Rethinking C++ (C++ neu denken)* may also use its adecc-owned sources under
the additional [adecc Book License](https://github.com/adeccscholar/adecc-core-lib/blob/main/adecc/ADECC_BOOK_LICENSE.md),
a custom MIT-like grant. The Book License remains a draft pending legal review.
It is specific to adecc-core-lib and does not grant commercial rights to
BuildEngine-Tests.

The library is header-only. When used in these examples it should be obtained
as a separately pinned Git dependency or submodule and its own copyright and
license notices must be preserved. Teaching and demonstration sources are
provided **AS IS**, without warranty or assurance of fitness for production.


## Local Markdown documentation server

A dedicated, read-only [BuildEngine-Tests documentation server](docu-server/)
serves the central [test overview](Docs/start/README.md), each test's
`Description.md`, `build.md`, license notices and CycloneDX SBOMs.
Browser extensions (syntax highlighting, Mermaid, MathJax) are prepared
separately into `Docs/js/`; test-owned images belong in `Docs/images/`.
The server does not execute builds over HTTP. See
[server setup and commands](Docs/start/BUILD.md). Default URL:
`http://127.0.0.1:8781/docs/`.

## Database backends

Shared C++23 logical-database checks are under `database/common/database_checks.h`.
`database/mssql` and `database/postgre` instantiate the same test functions;
only connection setup, SQL dialect, and driver-specific diagnostics differ.
SQL Server and PostgreSQL require running database services and a dedicated
`BuildEngineTest` database. See their `build.md` and `sql/` directories.
