# Build — Third-party integration VCL

Run from the BuildEngine-Tests root in the RAD Studio 13 Developer Command Prompt:

```bat
cmake -P bootstrap/Bootstrap.cmake
```

```bat
cmake -S thirdparty-showcase -B ../build/Test/thirdparty-showcase/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/thirdparty-showcase/Release
```

```bat
cmake --build ../build/Test/thirdparty-showcase/Release/cmake
```

```bat
cmake --install ../build/Test/thirdparty-showcase/Release/cmake
```

```bat
ctest --test-dir ../build/Test/thirdparty-showcase/Release/cmake --output-on-failure
```

Use independent Debug paths and switch both configuration settings to Debug. Ensure the BuildEngine package catalog and DLL staging are available; CTest may report zero tests when only an interactive viewer is defined.

## Application execution and operation

**ThirdPartyShowcase:** Interactive VCL dashboard for real HTTPS/JSON/zlib/libzip workflow.

```bat
..\build\Test\thirdparty-showcase\Release\ThirdPartyShowcase.exe
```

### Controls and verification boundaries

The primary action button starts the integration sequence. The dashboard displays individual HTTP results from Boost.Beast/OpenSSL and curl, parsed nlohmann JSON, zlib compression/round-trip and libzip output. It reports statuses, byte counts, runtime versions and the generated archive. Internet access is required; do not claim an offline result.
