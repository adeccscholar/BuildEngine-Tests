# OpenXLSX Grid and Worksheet Test

**Test purpose:** Validates worksheet cells, typed spreadsheet data, tabular access and XLSX roundtrip without Microsoft Excel.

This standalone C++23 test checks typed spreadsheet data access, worksheet operations, and XLSX file generation through the managed OpenXLSX library and the adecc-core-lib spreadsheet interface.

The test operates locally, without a database server or Microsoft Excel. It produces an XLSX file that can be inspected after execution and exercises the data-access abstractions with real worksheet content.

The CMake project resolves OpenXLSX, pugixml, libzip, Boost and zlib from the BuildEngine package catalog. See [build.md](build.md) for the exact commands, [SBOM.md](SBOM.md) for declared software inventory, and [LICENSE.md](LICENSE.md) for licensing.

**[not verified]** A fresh native Windows BCC64X test execution has not been performed as part of this documentation update.
