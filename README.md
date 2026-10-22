# BuildEngine Test Applications

This repository is checked out as `BuildEngine/Test`. All applications and shared test code live directly in the repository root. No distinction between demos and tests.

```text
BuildEngine/
  Test/                    (this repository)
    CMakeLists.txt
    CMakePresets.json
    bootstrap/Bootstrap.cmake
    cmake/BuildEngineTests.cmake
    CompareTest/
    PDFiumSDL2/
    PDFiumVCL/             (native RAD Studio VCL project)
    PDFiumCommon/
    pdf-image-extraction/
  build/Test/
    Release/cmake/         (CMake/Ninja build tree)
    Release/<target>/      (installed EXE + runtime DLLs)
    Debug/cmake/
    Debug/<target>/
```

## Commands

Use the RAD Studio 13 developer environment (BDS must be defined), then from `BuildEngine/Test`:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake --preset Release
cmake --build --preset Release
cmake --install ../build/Test/Release/cmake
```

For Debug, replace Release with Debug. Bootstrap validates tools but does not install a new Ninja or alter production BuildEngine packages.

The CMake install uses the existing BuildEngine-Admin `BuildEngineRuntime.cmake` for each target with a declared package closure. It copies the EXE and its staged DLLs into the target-specific directory. The declared runtime package closure MUST include all transitive DLL-producing dependencies. It is not sufficient to stage only a top-level package. No automatic DLL selection from PATH is permitted.

## Status and limitations

* `PDFiumSDL2`, `PDFiumCommon`, `PDFiumVCL` source files were migrated from `BuildEngine/Test`.
* `pdf-image-extraction` is migrated to the same flat application layout.
* `CompareTest/CMakeLists.txt` has been moved, but **its original C++ sources are still not committed to this Git repository**; the most recent complete CompareTest archive must be imported before enabling that target. Its `MyData.h` must use `auto const aResult = fast_float::from_chars(...)`, not `std::from_chars_result`.
* Runtime package closures for PDFium/SDL2 and qpdf must be validated against the installed package imports. They are configurable via `ADECC_PDFIUM_RUNTIME_ROOTS` and `ADECC_QPDF_RUNTIME_ROOTS`; no runtime closure completeness is claimed yet.
* PDFiumVCL is a `.cbproj` and remains separate from the CMake build.
* No local BCC64X build/install has been executed here.
