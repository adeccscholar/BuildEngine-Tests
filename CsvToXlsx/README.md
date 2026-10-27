# CsvToXlsx (BuildEngine BCC64X)

Converts the **non-recursive** contents of one directory into one `.xlsx` workbook per CSV file. This project uses the **installed** OpenXLSX library from BuildEngine, BCC64X only, C++23, Ninja, and BuildEngine's explicit runtime-DLL staging. No dependencies are downloaded or built by this consumer.

## Four positional parameters

```text
adecc_csv_to_xlsx.exe <input-directory> [extension] [moved-directory] [xlsx-directory]
```

1. **input-directory** is mandatory; only regular files directly in this directory are processed.
2. **extension** is optional, default `csv`; `.csv`, `CSV`, `.CsV` match. Both `csv` and `.csv` are accepted.
3. **moved-directory** is optional; when omitted or passed as `""`, input files remain untouched. When specified, a source file is moved **only after** successfully writing its XLSX. An identical source directory disables moving.
4. **xlsx-directory** is optional; when omitted or passed as `""`, files are written next to the source.

Examples in Windows CMD:

```bat
adecc_csv_to_xlsx.exe "D:\Data\Input"
adecc_csv_to_xlsx.exe "D:\Data\Input" txt
adecc_csv_to_xlsx.exe "D:\Data\Input" csv "D:\Data\Processed"
adecc_csv_to_xlsx.exe "D:\Data\Input" "" "" "D:\Data\Excel"
```

An input named `example.CSV` produces `example.xlsx`. One workbook contains one worksheet, `Sheet1`, and all values are **strings**. Leading zeros (`00123`), decimal separators, and text beginning with `=` are kept as string values. No column count is required; rows can contain different numbers of fields. Empty fields are retained. The worksheet respects Excel's 1,048,576-row, 16,384-column, and 32,767 UTF-16-code-unit-per-cell limits.

Delimiter detection automatically chooses comma, semicolon or TAB sampling up to 32 logical records outside quotes; a unique delimiter in the first record is preferred (up to 128 KiB). In ties, comma wins. **This is a heuristic**, not a guarantee for ambiguous CSV data. Quoted delimiters, doubled quotes and line breaks in quoted fields are supported by the parser. Quoting is strict; malformed records fail explicitly.

**Input encoding:** UTF-8, with or without UTF-8 BOM. Legacy Windows-1252/ANSI is **not** implicitly converted and will be rejected when a byte sequence is not valid UTF-8. CSV data contains arbitrary text, so conversion to numbers, formulas, and dates is deliberately disabled. Unsupported XML characters trigger an error instead of generating a corrupt workbook.

**File safety:** Existing XLSX output files are not overwritten. Workbook creation is staged in the destination directory and published by rename after save. On a conversion failure, the source is not moved. A failed archive move after a successful XLSX save is reported; the XLSX remains for inspection and rerunning will refuse to overwrite it. When moving files across volumes, a copy followed by a full byte comparison and deletion is used. This is not a transactional two-phase commit.

## Build

Place this directory under the `tests` checkout, for example:

```text
D:\local\embarcadero\test_v3\tests\CsvToXlsx
```

Launch a RAD Studio 13 developer command prompt at `D:\local\embarcadero\test_v3\tests` and run:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S CsvToXlsx -B ../build/Test/CsvToXlsx/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/CsvToXlsx/Release
cmake --build ../build/Test/CsvToXlsx/Release/cmake
ctest --test-dir ../build/Test/CsvToXlsx/Release/cmake --output-on-failure
cmake --install ../build/Test/CsvToXlsx/Release/cmake
```

A Debug build is analogous with `Debug` in all three positions. The installed program and its runtime closure are located under `build/Test/CsvToXlsx/Release`.

## Dependencies

Direct: OpenXLSX, its installed BuildEngine CMake package and imported target `OpenXLSX::OpenXLSX`. Runtime closure: the installed BuildEngine package roots for `openxlsx`, `pugixml`, `libzip`, `boost`, `zlib` and `xz`, plus BCC64X toolchain DLLs. Windows system import library: `shell32` for Unicode command-line processing. No static third-party linkage, no MSVC compilation, no MinGW toolchain.

License details for third-party packages are supplied by their installed BuildEngine package notices and SBOMs. This project contains new source files and does not specify a distribution license; select one before external redistribution.

## Verification status

The standalone CSV parser and text validator can be compiled and exercised on a host C++23 compiler, but final linking, XLSX integration, and the full CTest suite **must be verified under your installed BCC64X BuildEngine stack**. The consumer has been aligned with the repository's existing `OpenXlsxGrid/CMakeLists.txt` package resolution and staging rules.
