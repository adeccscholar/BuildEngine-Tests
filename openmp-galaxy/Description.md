# OpenMP galaxy

## Showcase in action: science and architecture

### Why this example matters

Explore a stylized star galaxy whose particles move in a seeded analytic central potential. OpenMP parallelizes updates, while SDL2/OpenGL renders star motion. The experiment is about deterministic numerical workloads and scheduling, not a gravitational N-body solver.

### The demonstration

Rotate the galaxy, pause, switch serial/static/dynamic/guided scheduling and compare CPU time, steps per second and effective threads on the same simulation state.

### Mathematical and technical foundations

$$\mathbf a(\mathbf r)=-\frac{GM\,\mathbf r}{\left(\|\mathbf r\|^2+\varepsilon^2\right)^{3/2}}.$$

The equation provides context for the implemented numerical or rendering path. It is not evidence of runtime success or a claim that a simplified model represents every possible physical case.

### Native data flow and dependencies

```mermaid
flowchart LR
   S0["Seeded star positions"] --> S1["Analytic central-force update"]
   S1["Analytic central-force update"] --> S2["LLVM OpenMP scheduling"]
   S2["LLVM OpenMP scheduling"] --> S3["CPU star trajectory buffer"]
   S3["CPU star trajectory buffer"] --> S4["SDL2 OpenGL starfield"]
```

All package/library identities remain those declared in the existing showcase manifest. Existing build and verification instructions below are retained.

---


**Test purpose:** Visualizes a particle galaxy and benchmarks native OpenMP simulations through an SDL2/OpenGL viewer.

This independent BuildEngine-Tests project exercises **llvm-openmp, SDL2, GLEW** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** galaxy_benchmark, galaxy_viewer.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications and interactive operation

**`galaxy_benchmark.exe`** — Deterministic galaxy-simulation benchmark.

**`galaxy_viewer.exe`** — Interactive SDL2/OpenGL 3.3 star galaxy with CPU OpenMP updates.

Left-drag rotates, right-drag or arrows pan, wheel zooms. F1/F2/F3 select top/edge/oblique cameras; C resets camera; Space pauses; R resets seeded stars; 1/2/3/4 select serial/static/dynamic/guided; +/- changes threads; [ and ] change simulation speed; Esc exits. Window title reports FPS, simulation steps/s, CPU timing and GPU. The star dynamics model an analytic central potential, not N-body gravity.
