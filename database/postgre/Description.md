# PostgreSQL typed query and transaction tests

**Test purpose:** Validates libpq/libpqxx queries, SSPI authentication, result ranges, transactions and database constraints.

This project checks native libpq and libpqxx consumers, typed C++ query result ranges, parameter binding, transaction semantics, constraints and shared logical-database operations through adecc-core-lib.

Tests use a dedicated `BuildEngineTest` PostgreSQL database and may create or drop their own objects. A PostgreSQL server with configured SSPI/GSSAPI or password authentication is required for live integration; no production database should be targeted.

Managed library roots include libpq, libpqxx, OpenSSL, zlib, Brotli and zstd. Follow [Configure.md](Configure.md) for PostgreSQL access configuration and [build.md](build.md) for reproducible BCC64X build and execution commands.

## Configuration

See [Configure.md](Configure.md) for Windows SSPI, user mapping through pg_hba.conf/pg_ident.conf and separate database roles.

## Additional shared database executable

`adecc_postgresql_additional_test` instantiates [common/additional_tests.h](../common/additional_tests.h) against this backend's `logical_database`. The common suite currently covers source-derived tuple OutputRange, NULL, named parameters, lazy Execute ranges, primary key and CHECK failures. Only [additional_dialect.h](additional_dialect.h) creates the native schema. Supplemental reference headers are retained under [common/original](../common/original/) for further migration; not all original cases are enabled yet. No JSON/XML fixture is required by the currently enabled tests.
