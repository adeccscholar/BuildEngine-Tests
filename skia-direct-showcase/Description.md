# Direct Skia showcase

**Test purpose:** Demonstrates Skia shapes, text and composited images with the managed graphics DLL and assets.

This independent BuildEngine-Tests project exercises **Skia, FreeType, zlib, bzip2, brotli, JPEG, PNG, HarfBuzz** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** SkiaDirectShowcase.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications and interactive operation

**`SkiaDirectShowcase.exe`** — Fullscreen Skia rendering showcase with seven scenes and adjustable stress benchmark.

Left/Right or 1–7 chooses Images, Primitives, Paths, Gradients, Transform/Clip, BlendModes or Stress/FPS. F toggles fit-to-screen vs native design pixels; Space pauses/resumes animation; Up/Down adjusts stress load from 2,000 to 40,000 drawing calls in scene 7; R resets; Esc quits. FPS and CPU render time appear in the stress overlay.
