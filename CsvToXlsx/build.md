# Build CsvToXlsx

Run from the existing BuildEngine `tests` directory in the RAD Studio 13 developer command prompt.

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S CsvToXlsx -B ../build/Test/CsvToXlsx/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CsvToXlsx/Release
cmake --build ../build/Test/CsvToXlsx/Release/cmake
ctest --test-dir ../build/Test/CsvToXlsx/Release/cmake --output-on-failure
cmake --install ../build/Test/CsvToXlsx/Release/cmake
```

For Debug, replace every occurrence of `Release` by `Debug`. Package versions are selected by `adecc_package_current_root` from `admin/build-libraries.xml`, without manual version numbers, dependency downloads, static third-party archives or MSVC/MinGW.

See [README.md](README.md) for argument examples.

The runtime search roots include the full transitive BuildEngine dependency graph for OpenXLSX: `openxlsx`, `pugixml`, `libzip`, `boost`, `zlib`, `xz`, `zstd`, `openssl` and `brotli`. The PE staging step copies only imported DLLs. `libzstd.dll` is provided by `zstd` and imported by the `libzip` dependency.
