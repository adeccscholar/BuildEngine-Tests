# CsvToXlsx — Description

Command-line CSV-to-XLSX batch converter for BuildEngine BCC64X. Every CSV field is handled as `std::string`, including leading zeros, arbitrary field widths and values that resemble formulas or dates. Records may have different numbers of columns and quoted fields may contain embedded record delimiters.

The application uses installed `OpenXLSX::OpenXLSX` packages. Its CSV parser does not require any third-party library. Two executables are built: the converter and a CTest consumer covering CSV interpretation, UTF-8 validation and a real XLSX roundtrip.

See [README.md](README.md) for semantics, error handling and limitations, and [build.md](build.md) for commands.
