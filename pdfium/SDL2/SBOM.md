# SBOM — PDFium / SDL2 test executables

The CycloneDX 1.6 files below describe the source-declared package dependencies. They are **not** verified binary/runtime SBOMs; final DLL imports, hashes and installed-package licenses require native Windows evidence.

- [pdfium-sdl2-viewer](SBOM-pdfium-sdl2-viewer.cdx.json)
- [pdfium-c-api-test](SBOM-pdfium-c-api-test.cdx.json)
- [pdfium-page-scroll-test](SBOM-pdfium-page-scroll-test.cdx.json)

The PDFium-based targets use Skia 153 and the corresponding current transitive closure. The standalone navigation test has no PDFium/SDL2 dependency.
