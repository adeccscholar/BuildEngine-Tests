# Build CsvToXlsx

Run from the existing BuildEngine `tests` directory in the RAD Studio 13 developer command prompt.

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S CsvToXlsx -B ../build/Test/CsvToXlsx/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CsvToXlsx/Release
cmake --build ../build/Test/CsvToXlsx/Release/cmake
ctest --test-dir ../build/Test/CsvToXlsx/Release/cmake --output-on-failure
cmake --install ../build/Test/CsvToXlsx/Release/cmake
```

## Run the installed application

From the BuildEngine `tests` directory, after a successful `cmake --install`:

```bat
..\build\Test\CsvToXlsx\Release\adecc_csv_to_xlsx.exe "D:\Data\Input"
..\build\Test\CsvToXlsx\Release\adecc_csv_to_xlsx.exe "D:\Data\Input" csv "D:\Data\Processed" "D:\Data\Excel"
```

The first call processes CSV files case-insensitively, writes XLSX next to their sources, and does not move them. The second call writes XLSX to `D:\Data\Excel` and moves successfully processed source CSV files to `D:\Data\Processed`. The input directory must exist; the archive and output directories are created when necessary. Existing XLSX and archive files are not overwritten.

To specify only the fourth argument, pass empty strings for the preceding optional arguments:

```bat
..\build\Test\CsvToXlsx\Release\adecc_csv_to_xlsx.exe "D:\Data\Input" "" "" "D:\Data\Excel"
```

For Debug, replace every occurrence of `Release` by `Debug`. Package versions are selected by `adecc_package_current_root` from `admin/build-libraries.xml`, without manual version numbers, dependency downloads, static third-party archives or MSVC/MinGW.

See [README.md](README.md) for argument examples.

The runtime search roots include the full transitive BuildEngine dependency graph for OpenXLSX: `openxlsx`, `pugixml`, `libzip`, `boost`, `zlib`, `xz`, `zstd`, `openssl` and `brotli`. The PE staging step copies only imported DLLs. `libzstd.dll` is provided by `zstd` and imported by the `libzip` dependency.
