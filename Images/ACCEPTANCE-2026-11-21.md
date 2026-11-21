# Showcase image and keyboard integration: repository acceptance record

**Date:** 2026-11-21  
**Scope:** Ten initial key-visual showcases and shared keyboard infrastructure.

## Repository verification

The following showcase manifests were individually inspected against their `Description.md` and the SVG asset referenced by `images.tile`. Each manifest points to a committed `Images/showcase/.../tile.svg` file and labels it `generated-concept`, not a runtime screenshot. Each description contains a matching image link, a mathematical display equation, and a Mermaid diagram.

| Showcase | Manifest and image | Mathematical explanation | Dependency diagram |
| --- | --- | --- | --- |
| HDR Photon Observatory | Present | Present | Present |
| Matplot Chaos Atlas | Present | Present | Present |
| Navigation Reactor | Present | Present | Present |
| Tesseract OCR Studio | Present | Present | Present |
| Vector Galaxy | Present | Present | Present |
| VTK Visualization | Present | Present | Present |
| WeatherClient | Present | Present | Present |
| Skia Direct Showcase | Present | Present | Present |
| Pulse Engine | Present | Present | Present |
| Barcode Signal Lab | Present | Present | Present |

The shared `graphics/common/NativeShowcase.h` and `graphics/common/OpenGLShowcase.h` both contain F1 About handling and F11 fullscreen handling. Platform-specific handlers are committed in several independent applications, including VTK, PDFium VCL, Skia Direct and WeatherClient. This record does not assert a complete source-code audit of every standalone GUI.

The server route and presentation implementation are maintained in `BuildEngine-Common` (`src/HttpServer.cpp` and `src/PostgresHtmlRenderer.h`), not duplicated in the thin `BuildEngine-Server` repository.

## Provenance and limits

The committed tiles are SVG illustrations made specifically for the showcases. They are not application screenshots, and they do not demonstrate rendering success.

The original high-resolution generated PNGs and derivative WebP tiles described in the previous handover are **not asserted to be present in this repository**. Do not point manifests at those files unless they are actually imported. Current SVG links are self-contained and resolve to committed repository files.

Repository verification proves source and metadata presence only. The following checks are still **[not verified]** and must not be reported as successful:

- Native Windows BCC64X Debug/Release compilation for all GUI applications.
- Interactive F1/F11 tests on the actual Windows desktop.
- HTTP 200 and correct browser rendering against the running production-root server.
- Import and provenance verification of the separate original PNG/WebP archive.

These acceptance checks require the native Windows checkout or the original binary archive. They are intentionally not replaced with a fabricated pass result.

## Manual acceptance commands

From the synchronized Windows production checkout, build each relevant GUI project using its own `build.md` and isolated Debug/Release build directory. Open the installed executable, press F1 and F11, restore the previous window mode, and verify the About dialog closes correctly.

Visit `http://localhost:8765/showcases/`, open each of the ten showcase detail pages, and confirm that its image URL returns HTTP 200 and retains the label for generated artwork. After replacing any concept visual with a real screenshot, update both the manifest and its description in the same commit.
