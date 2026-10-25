# Database consumer tests

Both SQL Server and PostgreSQL execute the same public `adecc::db::logical_database` C++23 checks from [common/database_checks.h](common/database_checks.h). Each backend owns only its driver connection, DDL and dialect-specific diagnostic assertions. The server must be running for integration execution, not for compilation. Both use the isolated database `BuildEngineTest`.

- [Microsoft SQL Server](mssql/Description.md)
- [PostgreSQL](postgre/Description.md)
