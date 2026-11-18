# Third-party integration VCL

**Test purpose:** Combines VCL user interfaces, HTTPS, JSON, ZIP archives and compression across managed native DLLs.

This independent BuildEngine-Tests project exercises **Boost, OpenSSL, curl, zlib, libzip, nlohmann-json, brotli, zstd, xz** with native BCC64X/C++23 consumer code. Its source and its runtime dependency closure are defined by this directory's CMake/IDE configuration and the BuildEngine package catalog.

**Programs:** ThirdPartyShowcase.

See [build.md](build.md), [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md) and the generated machine-readable CycloneDX declaration for each program. This page is test documentation, not a statement of verified production suitability.

## Applications, execution modes and controls

**`ThirdPartyShowcase.exe`** — Interactive VCL dashboard for real HTTPS/JSON/zlib/libzip workflow.

The primary action button starts the integration sequence. The dashboard displays individual HTTP results from Boost.Beast/OpenSSL and curl, parsed nlohmann JSON, zlib compression/round-trip and libzip output. It reports statuses, byte counts, runtime versions and the generated archive. Internet access is required; do not claim an offline result.
