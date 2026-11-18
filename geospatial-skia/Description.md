# Geospatial and Skia

**Test purpose:** Renders terrain and geographic data while comparing OpenMP and oneTBB processing.

This independent BuildEngine-Tests project exercises **Skia, oneTBB, llvm-openmp, fast_float, SDL2, GLEW, zlib, bzip2, brotli, JPEG, libpng, HarfBuzz, FreeType** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** geospatial_benchmark, geospatial_viewer, geospatial_terrain.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications and interactive operation

**`geospatial_benchmark.exe`** — Geospatial calculation and raster benchmark.

**`geospatial_viewer.exe`** — Interactive native Win32/Skia 2D address-point, density and relief view.

**`geospatial_terrain.exe`** — Interactive SDL2/OpenGL synthetic address-density terrain.

2D Skia viewer: Ctrl+O opens flat address data, Ctrl+D opens street data/hierarchy, 1/2/3 selects point/density/relief view; S/T/P selects serial/oneTBB/OpenMP; +/- changes cell size; wheel zooms around pointer, left-drag pans, 0 resets extent. 3D terrain viewer: F opens file, D opens directory, L reloads, drag-and-drop changes source, left-drag rotates, wheel zooms, 1/2/3 selects top/side/oblique view and Esc exits. Synthetic elevations describe address density, not physical terrain.
