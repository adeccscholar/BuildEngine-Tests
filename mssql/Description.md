# mssql — original DeckKernel consumer test

This test is migrated from [DeckKernel](https://github.com/adeccscholar/DeckKernel/tree/main/test/mssql) at commit `893f9247264c27a2d938b587f4b7315f26a74973`.

The business test algorithm is **unchanged**: only build paths, backend test connection defaults (PostgreSQL: BuildEngineTest/buildenginetest_user; MSSQL CSV: BuildEngineTest) and the BuildEngine packages have been adapted. Shared query, tuple, range and transaction code stays in the independently licensed `adecc-core-lib`, linked through `adecc::core` and `adecc::mssql` INTERFACE targets.

**Prerequisites:** MSSQL Server service with Microsoft ODBC Driver 18; execute sql/create-database.sql (SSMS/sqlcmd). The SQL Server is required at execution time, not at compilation.

Database engines and drivers are services or installed OS dependencies, **not** copied into this repository. The backend test uses `BuildEngineTest` only; do not run the tests against a productive database. Tests create/drop their own objects. OpenXLSX generates its own XLSX output.

The attached `tests.zip` contains an additional SQL-Server-oriented suite, imported separately in `mssql/additional-tests` without changing the original logic. It is not PostgreSQL SQL dialect-neutral.

## Supplementary MSSQL headers

The accompanying `BuildEngine-Tests-additional-MSSQL.zip` provides the full original 13-header SQL Server exercise suite supplied with this migration. Extract its `mssql/additional-tests/` directory into this checkout to enable the additional standalone test. The core error scenario is exercised by its `main.cpp`; all other source headers remain available unchanged. This supplementary archive has not yet been merged into GitHub and must not be counted as an already committed test.
