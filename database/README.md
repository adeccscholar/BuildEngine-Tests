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
| [MariaDB / Connector/C](mariadb/Description.md) | [Build](mariadb/build.md), [License](mariadb/LICENSE.md), [SBOM](mariadb/SBOM.md) | `adecc_mariadb_connection_test`, `adecc_mariadb_logical_test`, `adecc_mariadb_member_pipeline` |
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

## Native UUID integration

The existing live acceptance programs `adecc_mssql_odbc_test`,
`adecc_postgresql_pqxx_test` and `adecc_mariadb_logical_test` invoke the
same `adecc::test::RunUuidTests` from the pinned adecc-core-lib submodule.
The test checks generated UUID identity write-back, native UUID parameters,
UUID-keyed updates, typed and textual reads, and NULL versus Nil UUID.
A failure prevents the executable's final success report. MariaDB's offline
mode remains serverless; it does not claim to execute the live UUID checks.
Run against the isolated BuildEngineTest database with DDL permissions.
MariaDB requires native UUID support (10.7 or later); PostgreSQL requires
`gen_random_uuid()`. The shared fixture creates and drops Test_Db_Uuid;
do not run multiple copies against the same database concurrently.
