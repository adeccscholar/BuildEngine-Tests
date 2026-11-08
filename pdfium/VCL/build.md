# Build — PDFium VCL

From the BuildEngine-Tests checkout root run `cmake -P bootstrap/Bootstrap.cmake`, then open `pdfium/VCL/PDFiumVCL.cbproj` in RAD Studio 13 and select Win64x Debug or Release. This VCL project is built by RAD Studio/MSBuild, not by CMake.

The import library is resolved from the exact `pdfium/2025.11.19` BuildEngine package. Deploy the version-pinned PDFium/Skia 153 closure and `icudtl.dat` as described in [README.md](README.md). Do not mix Skia 144, FreeType 2.14.3 or older zlib DLLs into this executable's output.

**Execution status:** [nicht verifiziert].
