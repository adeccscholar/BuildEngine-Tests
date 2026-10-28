# MariaDB integration tests

The native BCC64X MariaDB Connector/C 3.4.11 package and updated `adecc-core-lib` physical query backend support the **unchanged** common test in `database/common/additional_tests.h`. The dialect adapter is `additional_dialect.h` and uses MariaDB InnoDB and a CHECK constraint.

## Command-line options

```text
adecc_mariadb_logical_test.exe [--host HOST] [--port PORT] [--database DB] [--user USER] [--password PASSWORD] [--ca CA_FILE] [--offline] [--integrated] [--no-tls]
```

Defaults: `--host localhost`, `--port 3306`, `--user root`, `--database BuildEngineTest`. Options override their respective `BUILDENGINE_MARIADB_HOST`, `BUILDENGINE_MARIADB_PORT`, `BUILDENGINE_MARIADB_USER`, `BUILDENGINE_MARIADB_DATABASE`, `BUILDENGINE_MARIADB_PASSWORD` and `BUILDENGINE_MARIADB_CA` environment variables.

`--password` works but exposes a secret to process listings and shell history. Prefer setting `BUILDENGINE_MARIADB_PASSWORD` securely in the test environment. The `--integrated` switch currently fails explicitly because GSSAPI is not yet implemented/verified in the connection adapter. There is no silent password fallback.

The shared test **creates and drops** `Test_Error_TupleInsert` in the specified database. Use a disposable test database with appropriate rights; never point it at production. The minimum user account is appropriate DDL/DML access to that test schema. `root` is a default for convenience, not the recommended test account.

## Build

```bat
cmake -S database/mariadb -B ../build/Test/database/mariadb/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/database/mariadb/Release
cmake --build ../build/Test/database/mariadb/Release/cmake
ctest --test-dir ../build/Test/database/mariadb/Release/cmake --output-on-failure
cmake --install ../build/Test/database/mariadb/Release/cmake
..\build\Test\database\mariadb\Release\adecc_mariadb_logical_test.exe --database BuildEngineTest --user root
```

Offline tests verify the parameter scanner without contacting a server. Real BCC64X compile and MariaDB 11 execution remain pending; no PASS is claimed until logs are supplied.

## BCC64X chrono compatibility

Both `adecc_mariadb_logical_test` and `adecc_mariadb_connection_test` define `CHRONO_WORK_AROUND` and `CHRONO_MISSED_ZONEDB` at target scope, identically to the existing `database/postgre` and `database/mssql` test projects. These definitions support adecc-core-lib's chrono helpers with the RAD Studio 13 BCC64X standard library.
