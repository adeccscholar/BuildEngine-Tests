# BuildEngine Tests — Documentation

[TOC]

This documentation belongs exclusively to the **BuildEngine-Tests** repository.
It describes independent C++23 / BCC64X consumer tests for libraries built by
BuildEngine, not BuildEngine's own internal administration or production services.

## Test areas

| Area | Libraries and purpose |
| --- | --- |
| [CompareTest](/docs/CompareTest/Description.md) | fast_float and oneTBB parsing/comparison |
| [Financial](/docs/financial/Description.md) | Boost, pugixml and OpenSSL finance examples |
| [Geospatial Skia](/docs/geospatial-skia/Description.md) | Skia, OpenMP, oneTBB, SDL2 and OpenGL |
| [OpenGL Earth](/docs/opengl-sdl2-earth/Description.md) | SDL2, GLEW, SOIL2 and OpenGL |
| [OpenMP Galaxy](/docs/openmp-galaxy/Description.md) | native OpenMP and SDL2 visualization |
| [OpenMP Mandelbrot](/docs/openmp-mandelbrot/Description.md) | native OpenMP and SDL2 visualization |
| [PDF Image Extraction](/docs/pdf-image-extraction/Description.md) | qpdf, zlib and JPEG |
| [PDFium SDL2](/docs/pdfium/SDL2/Description.md) | PDFium, SDL2, Skia 144 and runtime closure |
| [PDFium VCL](/docs/pdfium/VCL/Description.md) | RAD Studio VCL PDF viewer |
| [PDFium Common](/docs/pdfium/Common/Description.md) | page scrolling regression |
| [Skia Showcase](/docs/skia-direct-showcase/Description.md) | native Skia rendering |
| [Third-party Showcase](/docs/thirdparty-showcase/Description.md) | Boost, curl, OpenSSL and ZIP |
| [QPDF alternate test](/docs/tests/pdf-image-extraction/Description.md) | alternate standalone qpdf consumer |

Each test directory contains `Description.md`, `build.md`, `LICENSE.md`,
`SBOM.md` and one CycloneDX `SBOM-*.cdx.json` per executable.
The SBOM files currently describe declared dependencies; a complete verified
binary SBOM requires actual build outputs, complete installed package locks
and PE/DLL dependency analysis.

## Common workflow

1. Bootstrap tools: `cmake -P bootstrap/Bootstrap.cmake`
2. Prepare browser assets: `cmake -P docu-server/PrepareDocumentationAssets.cmake`
3. Build the standalone [documentation server](/docs/Docs/start/BUILD.md)
4. Select a test, read its [Description](/docs/CompareTest/Description.md),
   follow its `build.md`, and inspect its package license/SBOM evidence.

[Project license](/docs/LICENSE.md) and [adecc-core-lib license notice](/docs/ADECC_CORE_LIB_NOTICE.md).
The examples are educational and supplied AS IS.

## License and SBOM navigation

The documentation server index offers a license and CycloneDX SBOM index per project. License files reproduce the complete PolyForm Noncommercial 1.0.0 text, with a source-declared third-party license register. SBOMs are explicitly declared-only and require installed package and PE/DLL verification before distribution.
