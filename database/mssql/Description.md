# Microsoft SQL Server data access and transaction tests

This project exercises the Windows ODBC Driver 18 connection to SQL Server, typed C++ result ranges, relational constraints, transactions, CSV processing and XLSX integration. The tests use the independently maintained adecc-core-lib database and spreadsheet interfaces.

Two scenarios are implemented: the direct SQL Server consumer and a common logical-database test suite with backend-specific SQL dialect configuration. Integration runs require a dedicated `BuildEngineTest` database; never use a production database because tests may create and drop their own objects.

Dependencies include managed OpenXLSX, pugixml, libzip, Boost, zlib, xz, zstd and OpenSSL, plus the system-installed Microsoft ODBC Driver and a running SQL Server. Follow [Configure.md](Configure.md) for access setup and [build.md](build.md) for configure, build, CTest and execution commands.

## Supplementary MSSQL headers


## Configuration

See [Configure.md](Configure.md) for the Windows login, database user, permissions and SQL Server service.

## Additional shared database executable

`adecc_mssql_additional_test` instantiates [common/additional_tests.h](../common/additional_tests.h) against this backend's `logical_database`. The common suite currently covers source-derived tuple OutputRange, NULL, named parameters, lazy Execute ranges, primary key and CHECK failures. Only [additional_dialect.h](additional_dialect.h) creates the native schema. Supplemental reference headers are retained under [common/original](../common/original/) for further migration; not all original cases are enabled yet. No JSON/XML fixture is required by the currently enabled tests.
