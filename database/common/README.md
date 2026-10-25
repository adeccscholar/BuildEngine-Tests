# Common — physical database-independent C++23

[TOC]

Two shared headers express the core educational goal: the same algorithm
is instantiated with MSSQL and PostgreSQL adapters, without duplicating its
C++ test logic.

- [database_checks.h](database_checks.h): shared baseline acceptance checks,
  Unicode conversion, row comparison, named parameters, output ranges,
  transaction rollback and commit.
- [additional_tests.h](additional_tests.h): second test executable's
  currently enabled cases: tuple OutputRange, optional/NULL values,
  count, named parameters, lazy Execute ranges, PRIMARY KEY and CHECK errors.
- [Original source collection](original/README.md): preserved Test.zip headers
  for further educational migration, including persistence and grid tests.

Database-specific SQL, credentials and runtime details belong in the
[MSSQL](../mssql/Description.md) or
[PostgreSQL](../postgre/Description.md) folders.

**Status:** no successful end-to-end BCC64X run has yet been confirmed for
these new second test targets. The original persistence and grid scenarios
are not all enabled. The documentation makes this distinction explicit.
