# Database — shared C++23 tests

[TOC]

The tests under `database/` prove that identical algorithms using
`adecc::db::logical_database` run with different physical backends.
Both use the isolated database **BuildEngineTest** and require the
respective database server for execution, not compilation.

| Area | Documentation | Programs |
| --- | --- | --- |
| [Shared C++23 algorithms](common/README.md) | [Sources](common/database_checks.h), [additional tests](common/additional_tests.h) | Shared by MSSQL and PostgreSQL |
| [MSSQL / ODBC](mssql/Description.md) | [Configure](mssql/Configure.md), [Build](mssql/build.md), [License](mssql/LICENSE.md), [SBOM](mssql/SBOM.md) | `adecc_mssql_odbc_test`, `adecc_mssql_additional_test`, `adecc_mssql_csv_pipeline` |
| [PostgreSQL / libpqxx](postgre/Description.md) | [Configure](postgre/Configure.md), [Build](postgre/build.md), [License](postgre/LICENSE.md), [SBOM](postgre/SBOM.md) | `adecc_postgresql_pqxx_test`, `adecc_postgresql_additional_test` |

The second executable in each backend uses the **same**
[additional_tests.h](common/additional_tests.h). Only the
[dialect mapping for MSSQL](mssql/additional_dialect.h) and
[dialect mapping for PostgreSQL](postgre/additional_dialect.h),
plus connection setup, differ.

The full [original Test.zip headers](common/original/README.md) are
preserved for teaching and comparison. Only a subset of the original
scenarios has been migrated to the two runnable second executables;
the remainder is not yet verified or enabled.

SQL environment scripts are available as read-only, highlighted sources:
[MSSQL create database](mssql/sql/create-database.sql),
[PostgreSQL create database](postgre/sql/create-database.sql) and
[PostgreSQL permissions](postgre/sql/configure-access.sql).

The documentation server **does not start database services or execute SQL**.
