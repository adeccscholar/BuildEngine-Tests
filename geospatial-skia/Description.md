# Geospatial and Skia

## Scientific showcase: experience and mechanisms

### What the visitor experiences

Address records become navigable density landscapes. Show the same points as a Skia map, Gaussian density map and artificially elevated SDL2/OpenGL terrain. Switch between serial, oneTBB and OpenMP implementations, and compare timing on identical data.

### Demonstration sequence

Open the viewer; use 1/2/3 for points, density and relief, S/T/P for execution backends. Pan, zoom and rotate the synthetic terrain. The altitude is a visualization of address density, not measured geography.

### Mathematical background

$$D(i,j)=\sum_{u,v}G_\sigma(i-u)G_\sigma(j-v)n(u,v),\qquad G_\sigma(t)\propto e^{-t^2/(2\sigma^2)}.$$

The equation describes the relevant mathematical mechanism, rather than independently proving any performance, physical fidelity or runtime success.

### Native pipeline and managed dependencies

```mermaid
flowchart LR
   A0["Address coordinates"] --> A1["fast_float numeric parsing"]
   A1["fast_float numeric parsing"] --> A2["Grid density and Gaussian smoothing"]
   A2["Grid density and Gaussian smoothing"] --> A3["oneTBB or OpenMP scheduling"]
   A3["oneTBB or OpenMP scheduling"] --> A4["Skia map and SDL2/OpenGL terrain"]
```

The diagram describes data responsibilities. BuildEngine-managed libraries remain versioned package dependencies; a successful visual display cannot replace the declared headless correctness checks.

---


**Test purpose:** Renders terrain and geographic data while comparing OpenMP and oneTBB processing.

This independent BuildEngine-Tests project exercises **Skia, oneTBB, llvm-openmp, fast_float, SDL2, GLEW, zlib, bzip2, brotli, JPEG, libpng, HarfBuzz, FreeType** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** geospatial_benchmark, geospatial_viewer, geospatial_terrain.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications and interactive operation

**`geospatial_benchmark.exe`** — Geospatial calculation and raster benchmark.

**`geospatial_viewer.exe`** — Interactive native Win32/Skia 2D address-point, density and relief view.

**`geospatial_terrain.exe`** — Interactive SDL2/OpenGL synthetic address-density terrain.

2D Skia viewer: Ctrl+O opens flat address data, Ctrl+D opens street data/hierarchy, 1/2/3 selects point/density/relief view; S/T/P selects serial/oneTBB/OpenMP; +/- changes cell size; wheel zooms around pointer, left-drag pans, 0 resets extent. 3D terrain viewer: F opens file, D opens directory, L reloads, drag-and-drop changes source, left-drag rotates, wheel zooms, 1/2/3 selects top/side/oblique view and Esc exits. Synthetic elevations describe address density, not physical terrain.
