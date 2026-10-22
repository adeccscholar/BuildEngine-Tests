# BuildEngine Tests

This repository is checked out as `BuildEngine/Test`. Every application is a separate project directly below this directory. There is no test/demo split and no top-level multi-project configure required.

## Common bootstrap, independent per-project CMake commands

Run from `BuildEngine/Test`, in the RAD Studio 13 developer command prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

The bootstrap checks BCC64X, BuildEngine's existing Ninja and toolchain, and generates `bootstrap/BuildEngineInit.cmake` (a local, untracked CMake initial-cache file). It does not build libraries or download tools.

Then configure, build and install any particular project separately:

```bat
cmake -S CompareTest -B ../build/Test/CompareTest/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CompareTest/Release
cmake --build ../build/Test/CompareTest/Release/cmake
cmake --install ../build/Test/CompareTest/Release/cmake
```

For PDFiumSDL2, replace `CompareTest` by `PDFiumSDL2`; for pdf-image-extraction replace by `pdf-image-extraction`. For Debug use a `Debug` build tree and `-DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug`.

A CMake app is installed under `../build/Test/<project>/<configuration>/<target>/` with its executable and staged DLLs side-by-side. The shared functions `adecc_stage_runtime()` and `adecc_install_application()` originate from BuildEngine-Admin. Each app must declare **the complete exact package-root closure**, including transitive dependencies. No PATH-based newest-package probing is allowed.

## Current project status

- `PDFiumSDL2`: migrated C++ sources and standalone CMake target; full DLL closure needs validation.
- `PDFiumCommon`: shared headers and a standalone PageScroll regression source (not its own CMake project).
- `PDFiumVCL`: native RAD Studio `.cbproj`; not configured by CMake.
- `pdf-image-extraction`: standalone CMake target, qpdf runtime closure still needs validation.
- `CompareTest`: CMake target migrated, **but the complete original C++ source set is still missing from this Git repository**. The latest source archive must still be added; until then it cannot configure or build.
- No successful full BCC64X build/install is claimed.

BuildEngine's original `Test/` copies should not be considered deleted or migrated completely until these gaps are closed.
