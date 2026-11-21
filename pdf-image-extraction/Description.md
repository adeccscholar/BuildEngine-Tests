# QPDF image extraction

## Showcase in action: science and architecture

### Why this example matters

An image embedded inside a PDF is an object with its own stream, filter chain and metadata, not simply a screen crop. The native QPDF example traverses a known fixture and checks that extraction uses the real PDF object model.

### The demonstration

Run the fixture-based CTest, inspect the extraction diagnostics and compare actual extracted bytes and image assets with the declared expectations. Distinguish encoded streams from fully decoded, display-ready rasters.

### Mathematical and technical foundations

$$\text{PDF image bytes}=\operatorname{DecodeFilters}(\text{encoded object stream}).$$

The equation provides context for the implemented numerical or rendering path. It is not evidence of runtime success or a claim that a simplified model represents every possible physical case.

### Native data flow and dependencies

```mermaid
flowchart LR
   S0["PDF fixture"] --> S1["QPDF object and stream traversal"]
   S1["QPDF object and stream traversal"] --> S2["zlib / JPEG decoding where applicable"]
   S2["zlib / JPEG decoding where applicable"] --> S3["Extracted asset"]
   S3["Extracted asset"] --> S4["CTest validation"]
```

All package/library identities remain those declared in the existing showcase manifest. Existing build and verification instructions below are retained.

---


**Test purpose:** Extracts image assets from a known PDF fixture and checks QPDF output using managed compression libraries.

This independent BuildEngine-Tests project exercises **qpdf 12.4.2, zlib 1.4, libjpeg-turbo 3.2.0** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** pdf-image-extraction.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.
