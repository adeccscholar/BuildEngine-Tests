# PDFium page scrolling

## Showcase in action: science and architecture

### Why this example matters

A PDF viewer's scrolling logic can be tested without a PDF renderer. This pure C++23 regression verifies page numbers, boundaries and navigation transitions before SDL2/VCL or PDFium adds their separate complexities.

### The demonstration

Run the independent page-scroll executable. Exercise first/last page and out-of-range navigation cases; no PDFium DLL or desktop window is required.

### Mathematical and technical foundations

$$p_{\mathrm{next}}=\min(N,\max(1,p+\Delta p)),\quad N\ge1.$$

The equation provides context for the implemented numerical or rendering path. It is not evidence of runtime success or a claim that a simplified model represents every possible physical case.

### Native data flow and dependencies

```mermaid
flowchart LR
   S0["Navigation command"] --> S1["Pure C++23 page-state transition"]
   S1["Pure C++23 page-state transition"] --> S2["Boundary clamp"]
   S2["Boundary clamp"] --> S3["Deterministic regression result"]
```

All package/library identities remain those declared in the existing showcase manifest. Existing build and verification instructions below are retained.

---


**Test purpose:** Pure C++23 page-scroll boundary and navigation tests, without linking PDFium.

This independent BuildEngine-Tests project exercises **C++23 standard library; no linked PDFium dependency** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** pdfium-pagescroll-test.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.
