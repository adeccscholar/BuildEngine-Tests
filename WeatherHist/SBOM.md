# WeatherHist SBOM

Source-level dependency inventory: curl, OpenSSL, zlib, Brotli, zstd, nlohmann/json, RAD Studio VCL and toolchain libraries, all resolved through the active BuildEngine installation and source CMake. There is no verified binary CycloneDX SBOM until BCC64X builds the executable and runtime imports are inspected. Record separate per-target SBOMs for WeatherHist and WeatherHistModelTest before publishing a binary release.
