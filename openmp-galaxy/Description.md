# OpenMP galaxy

**Test purpose:** Visualizes a particle galaxy and benchmarks native OpenMP simulations through an SDL2/OpenGL viewer.

This independent BuildEngine-Tests project exercises **llvm-openmp, SDL2, GLEW** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** galaxy_benchmark, galaxy_viewer.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications and interactive operation

**`galaxy_benchmark.exe`** — Deterministic galaxy-simulation benchmark.

**`galaxy_viewer.exe`** — Interactive SDL2/OpenGL 3.3 star galaxy with CPU OpenMP updates.

Left-drag rotates, right-drag or arrows pan, wheel zooms. F1/F2/F3 select top/edge/oblique cameras; C resets camera; Space pauses; R resets seeded stars; 1/2/3/4 select serial/static/dynamic/guided; +/- changes threads; [ and ] change simulation speed; Esc exits. Window title reports FPS, simulation steps/s, CPU timing and GPU. The star dynamics model an analytic central potential, not N-body gravity.
