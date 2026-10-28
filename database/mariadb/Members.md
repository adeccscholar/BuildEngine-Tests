# Member contribution integration test

## Goal

Generic business workload in `database/common/member_pipeline.h`, with a MariaDB 11 dialect adapter in `database/mariadb/member_dialect.h`. The same C++ algorithm is intended for PostgreSQL and SQL Server; those dialects are **not yet added**.

Uses original adecc facilities: `FromCsvFileTyped` as a range, `defined_type_list`, `Transform` with `BindMapping`, `PersistentSystemData` generated insert metadata, logical database output ranges, and `WriteGridModel<OStreamGridBackend<>>` for console reporting. XML `Anreden.xml` is parsed with the BuildEngine-installed `pugixml` DLL, not as CSV.

The fixture has 50 invented persons, 2983 synthetic bookings (2019–October 2026), four salutation entries and deliberate historical changes of IBAN. `Steuer` uses the adecc `VatId_DE` validation type; `IBAN` uses adecc `Iban`. Changes to types must remain validated; do not silently store malformed data.

The executable **only permits** databases named `BuildEngineTest` or beginning with `BuildEngineTest_`. It first connects to the MariaDB server without an active schema, creates the database if missing (requires CREATE DATABASE privileges), reconnects to it and drops tables in foreign-key order. This is intentionally destructive to the **test** schema. All source fixtures are stored under `database/mariadb/fixtures`, not generated during the test.

From `D:\local\embarcadero\test_v3\tests`:

```bat
git pull --ff-only
git submodule update --init --recursive
cmake -S database/mariadb -B ../build/Test/database/mariadb/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/database/mariadb/Release
cmake --build ../build/Test/database/mariadb/Release/cmake
ctest --test-dir ../build/Test/database/mariadb/Release/cmake --output-on-failure
cmake --install ../build/Test/database/mariadb/Release/cmake
..\build\Test\database\mariadb\Release\adecc_mariadb_member_pipeline.exe --host localhost --port 3306 --database BuildEngineTest --user root --no-tls
```

Use `--password YOUR_PASSWORD` if needed. Prefer `BUILDENGINE_MARIADB_PASSWORD` to avoid password exposure in command history. For remote database use TLS with verified certificate. On a normal user account, arrange database creation separately or grant limited CREATE DATABASE privileges.

Expected output: counts for all three tables, at least one historical IBAN mismatch, 50-row `adecc::Grid` contribution report, and a final `SMOKE|RESULT|PASS`. The backend binary and tests have not been compiled or run on BCC64X in this session; this is **unverified** code until test logs confirm it.

## German console presentation via typed policy projection

The SQL generator returns `adecc::date_ty` and `adecc::money_ty`. The reusable report uses `adecc::Transform` to turn those into `Geburtstag_ty` (EUROPE formatting, e.g. `28.10.1990`) and `Beitragssumme_ty` (German thousands and decimal separators, e.g. `1.234,50`). A second typed `adecc::Transform` explicitly calls each policy's `.str()` to feed `std::string` columns into `WriteGridModel<OStreamGridBackend<>>`. The positive-only `Beitrag_ty` now uses the same German numeric formatter; a separate non-negative `Beitragssumme_ty` allows `0,00` for members without bookings. No SQL text formatting, locale-dependent iostream state or data model change is involved.

The `--offline` test now asserts exact formatting for `28.10.1990`, `1.234,50` and `0,00` as a regression gate.
