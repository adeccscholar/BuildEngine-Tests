# Database consumer tests

Both SQL Server and PostgreSQL execute the same public `adecc::db::logical_database` C++23 checks from [common/database_checks.h](common/database_checks.h). Each backend owns only its driver connection, DDL and dialect-specific diagnostic assertions. The server must be running for integration execution, not for compilation. Both use the isolated database `BuildEngineTest`.

- [Microsoft SQL Server](mssql/Description.md)
- [PostgreSQL](postgre/Description.md)

## Structured documentation

[Database overview](README.md) · [Common C++23](common/README.md) · [Original Test.zip sources](common/original/README.md). Both backends now have a second executable that runs the same [additional_tests.h](common/additional_tests.h), with only backend-specific DDL and connection wiring.
