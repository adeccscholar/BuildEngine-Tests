# PDFium viewer SDL2

This independent BCC64X/C++23 BuildEngine-Tests project consumes **PDFium 2025.11.19, SDL2 2.32.10 and Skia 153**. The exact transitive runtime closure is defined in [CMakeLists.txt](CMakeLists.txt) and resolved through versioned BuildEngine packages.

**Programs:** `pdfium-sdl2-viewer` (interactive GUI), `pdfium-c-api-test` (headless C API plus rendering regression), and `pdfium-page-scroll-test` (navigation regression without PDFium or SDL2).

The two noninteractive executables are registered with CTest for Debug and Release. The executable runtime stage preserves exact package versions, rejects DLL collisions, excludes retired Abseil/Skia binaries, and deploys Skia 153's `icudtl.dat` to PDFium-based executables.

See [README.md](README.md), [build.md](build.md), [LICENSE.md](LICENSE.md), and [SBOM.md](SBOM.md). Native Windows execution remains **[nicht verifiziert]**.
