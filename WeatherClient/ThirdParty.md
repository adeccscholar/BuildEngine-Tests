<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient: Third-Party Libraries, Runtime, and Licenses

[TOC]

## Authoritative Sources and Verification Status

This document reflects [CMakeLists.txt](CMakeLists.txt), [BuildEngineTests.cmake](../cmake/BuildEngineTests.cmake), [SBOM-WeatherClient.cdx.json](SBOM-WeatherClient.cdx.json), [SBOM.md](SBOM.md), and [LICENSE.md](LICENSE.md). The CycloneDX metadata explicitly states **`declared-only-unverified`**.

The following categories must remain distinct:

1. **Direct dependencies:** software actually called from WeatherClient sources or linked by CMake.
2. **Declared package roots:** installed BuildEngine packages eligible to satisfy PE-import dependencies. Declaring a root does **not** mean every DLL from that package is distributed.
3. **Verified binary runtime closure:** actual PE imports and DLL staging for a specific BCC64X build, which must be determined after building.

`adecc_package_current_root` obtains the **single active version** from `admin/build-libraries.xml`. Entries marked `supersededBy` are not eligible. The WeatherClient CMake file does not hardcode third-party versions; catalog changes trigger CMake reconfiguration.

## Directly Used Libraries

| Library | Use in WeatherClient | Integration | Declared license |
| --- | --- | --- | --- |
| **Skia** | 2D graphics, raster surfaces, gradients, dials, weather illustrations, text | `Skia::skia`, `find_package(Skia CONFIG)` | BSD-3-Clause; component notices apply |
| **SDL2** | Window, keyboard/mouse events, software renderer, texture presentation | `SDL2::SDL2`, `find_package(SDL2 CONFIG)` | Zlib |
| **Boost** | `boost::statechart` and `boost::mpl` for the FSM | Installed headers with production BCC64X Clang `Boost.Config` override | BSL-1.0 |
| **libcurl** | HTTPS GET, URL escaping, TLS verification, response handling | Dynamic imported `WeatherCurl` target | curl |
| **OpenSSL** | TLS and cryptographic backend for the installed transport stack | Dynamic `WeatherSSL`, `WeatherCrypto` imported targets | Apache-2.0 |
| **nlohmann/json** | Provider JSON parsing and persisted local preferences | Installed include-only headers | MIT |

Boost.Statechart is consumed through the installed headers; the CMake target does **not** arbitrarily link a guessed Boost DLL. Skia, SDL2, and libcurl are linked using dynamic import libraries. The resulting transitive system and third-party DLL list depends on the actual PE imports of the selected BuildEngine packages.

## Additional Declared Package Roots

The following packages are also part of `WEATHER_PACKAGE_ROOTS` in the current CMake configuration. WeatherClient does not necessarily call their APIs directly.

| BuildEngine package | Possible role in the runtime stack | Declared license |
| --- | --- | --- |
| `zlib` | Lossless compression, image/network dependencies | Zlib |
| `brotli` | Compression in Skia/HTTP components | MIT |
| `zstd` | Package-dependent compression | BSD-3-Clause |
| `xz` | liblzma compression | 0BSD; inspect component licenses |
| `bzip2` | Compression | bzip2-1.0.6 |
| `libjpeg-turbo` | JPEG decoding in graphics components | IJG and BSD-3-Clause; notices apply |
| `libpng` | PNG decoding in graphics components | libpng-2.0 |
| `harfbuzz` | Text shaping within graphics/font dependencies | MIT |
| `freetype` | Font rasterization in rendering dependencies | FTL or GPL-2.0-or-later; select and verify license |

These are **potential transitive dependencies**, not evidence that every code path is used or every package file must be shipped.

## Platform and Toolchain Dependencies

- **C++23 / BCC64X (RAD Studio 13):** `CXX_STANDARD 23` with compiler extensions disabled; `WIN32` subsystem using `-Wl,--subsystem,windows`.
- **Windows system libraries:** `ws2_32` and `crypt32` are explicitly linked. They are operating-system components rather than BuildEngine third-party packages.
- **Toolchain runtime DLLs:** `libc++-370.dll` and `libunwind-370.dll` are declared in `TOOLCHAIN_DLLS`. They must come from the configured toolchain, not a coincidental PATH match.
- **Segoe UI:** The application may access this installed OS font through Skia; **no Microsoft font files are copied or redistributed**. `WeatherText.h` contains original fallback lettering.
- **CMake and Ninja:** build tools, not direct WeatherClient runtime dependencies.
- **`curl.exe`:** optional independent CLI diagnostic tool; **not the embedded libcurl dependency**. See [REST.md](REST.md).

## Package Selection and Runtime Staging

The current package-root configuration includes:

```cmake
foreach(_id IN ITEMS skia sdl2 curl openssl boost nlohmann-json zlib
   brotli zstd xz bzip2 libjpeg-turbo libpng harfbuzz freetype)
   string(REPLACE "-" "_" _name "${_id}")
   string(TOUPPER "${_name}" _name)
   adecc_package_current_root("${_name}_PACKAGE" "${_id}")
   list(APPEND WEATHER_PACKAGE_ROOTS "${${_name}_PACKAGE}")
endforeach()
```

CMake also verifies the curl/OpenSSL import libraries and `nlohmann/json.hpp`. `WeatherCurl`, `WeatherSSL`, and `WeatherCrypto` are declared as **SHARED IMPORTED** targets. `adecc_test_use_boost` applies the production BCC64X-native Clang configuration.

`adecc_test_install(WeatherClient RUNTIME_CLOSURE PACKAGE_ROOTS ... TOOLCHAIN_DLLS ...)` builds `buildengine-stage-WeatherClient` and stages the required **PE-import closure**. DLLs are placed next to the build executable and subsequently installed with `cmake --install`. The application does not distribute an entire SDK, use static third-party archives, or resolve library versions by guessing from PATH.

| Output location | Purpose |
| --- | --- |
| `../build/Test/WeatherClient/Release/cmake/runtime/WeatherClient/` | Build/staging executable and resolved DLLs |
| `../build/Test/WeatherClient/Release/` | Installed executable and staged DLLs; updated only by `cmake --install` |

Debug uses a separate configuration.

## Runtime Dependency Verification

After local build and installation:

```bat
cmake --build ../build/Test/WeatherClient/Release/cmake
cmake --install ../build/Test/WeatherClient/Release/cmake
dir ..\build\Test\WeatherClient\Release\*.dll
ctest --test-dir ../build/Test/WeatherClient/Release/cmake --output-on-failure
```

The `dir` command lists directory contents; it does **not** prove the full transitive PE-import graph. For an auditable delivery, inspect BuildEngine staging logs and the imported DLLs of the exact installed executable and its dependencies. Then verify versions, filenames, SHA-256 checksums, license texts, and provenance in the distribution SBOM.

## Licensing and SBOM Responsibility

WeatherClient's original project code uses [PolyForm Noncommercial 1.0.0](LICENSE.md). This **does not relicense** third-party components. Original license texts and notices from installed BuildEngine packages take precedence over this summary table.

The current [CycloneDX SBOM](SBOM-WeatherClient.cdx.json) is a **declarative component list without verified binary closure or concrete versions** and must not be represented as a completed distribution audit.

**External services are separate from linked libraries.** Open-Meteo, Nominatim/OSM, ipapi.co, and ipwho.is are contacted through HTTPS. API access, weather/geographic data, public rate limits, and OS font rights have independent conditions. See [WeatherServices.md](WeatherServices.md) and [REST.md](REST.md).

## References

- [CMakeLists.txt](CMakeLists.txt): binding target and package declarations.
- [BuildEngineTests.cmake](../cmake/BuildEngineTests.cmake): XML-driven package selection, Boost integration, and DLL staging.
- [SBOM-WeatherClient.cdx.json](SBOM-WeatherClient.cdx.json), [SBOM.md](SBOM.md): CycloneDX metadata.
- [LICENSE.md](LICENSE.md): project and third-party licensing information.
- [Architecture.md](Architecture.md): application workflow and dependency boundaries.
- [build.md](build.md): build, installation, and test commands.

**Verification status:** Repository source/CMake review only; no local BCC64X test or binary-closure audit is claimed.
