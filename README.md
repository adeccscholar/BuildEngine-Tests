# BuildEngine Tests

This repository can be checked out under the BuildEngine root as `tests` (or `Test`). Every application is a separate project directly below this directory. There is no test/demo split and no top-level multi-project configure required.

## Common bootstrap, independent per-project CMake commands

Run from the checkout directory (e.g. `BuildEngine/tests`), in the RAD Studio 13 developer command prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

The bootstrap checks BCC64X, BuildEngine's existing Ninja and toolchain, and validates the tracked `bootstrap/BuildEngineInit.cmake` initial-cache file. It does not build libraries or download tools.

Then configure, build and install any particular project separately:

```bat
cmake -S CompareTest -B ../build/Test/CompareTest/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CompareTest/Release
cmake --build ../build/Test/CompareTest/Release/cmake
cmake --install ../build/Test/CompareTest/Release/cmake
```

For `pdfium/SDL2` use its own independent build tree, e.g. `../build/Test/pdfium-SDL2/Release/cmake`. For `pdf-image-extraction` use `../build/Test/pdf-image-extraction/Release/cmake`. Never share a CMake binary directory across different source projects. For Debug use a `Debug` build tree and `-DCMAKE_BUILD_TYPE=Debug -DBUILDENGINE_CONFIGURATION=Debug`.

A CMake app is installed under `../build/Test/<project>/<configuration>/<target>/` with its executable and staged DLLs side-by-side. The shared functions `adecc_stage_runtime()` and `adecc_install_application()` originate from BuildEngine-Admin. Each app must declare **the complete exact package-root closure**, including transitive dependencies. No PATH-based newest-package probing is allowed.

## One-time cache reset after moving sources

If CMake reports `source ... does not match the source ... used to generate cache`, remove **only that project's** old build directory, e.g. from the checkout directory:

```bat
cmake -E rm -rf ../build/Test/CompareTest/Release/cmake
```

Then rerun the project's configure command. Do not delete library build caches or reinstall third-party packages.

## Current project status

- `pdfium/SDL2`: migrated C++ sources and standalone CMake target; full DLL closure needs validation.
- `pdfium/Common`: shared headers and a standalone PageScroll regression source (not its own CMake project).
- `pdfium/VCL`: native RAD Studio `.cbproj`; not configured by CMake.
- `pdf-image-extraction`: standalone CMake target, qpdf runtime closure still needs validation.
- `CompareTest`: CMake target migrated, **but the complete original C++ source set is still missing from this Git repository**. The latest source archive must still be added; until then it cannot configure or build.
- No successful full BCC64X build/install is claimed.

BuildEngine's original `Test/` copies should not be considered deleted or migrated completely until these gaps are closed.

## Historical examples (Demo.zip, 2026-10-22)

The uploaded archive was inspected and contains these independent legacy projects: `financial` (including `financial/test`, `test/vcl`, and `test/vcl-skia`), `opengl-sdl2-earth`, `skia-direct-showcase`, and `thirdparty-showcase`. Their original C++ files are not yet imported into GitHub. The archive also lacks graphics assets referenced by the OpenGL Earth and Skia showcase projects, and a `MainForm.dfm`/`app.ico` referenced by the third-party showcase. The old CMake definitions use `TP_CMAKE` and local vendor build folders, which must be replaced with BuildEngine installed dependencies before declaring them buildable. Do not run the old local third-party CMake scripts as part of the BuildEngine tests.
