# Build mssql (RAD Studio 13 Developer Command Prompt)

**Datenbank und Windows-Konto einrichten:** [Configure.md](Configure.md). Integrierte Sicherheit ohne SQL-Kennwort.

From `BuildEngine/tests`, use the productive BuildEngine root, existing installed libraries, and checked-out `adecc-core-lib` submodule:

```bat
git submodule update --init --recursive
cmake -P bootstrap/Bootstrap.cmake
cmake -S database/mssql -B ../build/Test/mssql/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/mssql/Release
cmake --build ../build/Test/mssql/Release/cmake
cmake --install ../build/Test/mssql/Release/cmake
```

For Debug substitute `Debug` consistently. No local third-party bootstrap, no static archive or package-version guessing.

```bat
sqlcmd -S localhost -E -i database\mssql\sql\create-database.sql
..\build\Test\mssql\Release\adecc_mssql_odbc_test.exe --server localhost --database BuildEngineTest --integrated
set ADECC_MSSQL_DATABASE=BuildEngineTest
..\build\Test\mssql\Release\adecc_mssql_csv_pipeline.exe
```

**Server dependency:** A running database server and correctly configured driver, account, network and authentication are required. A successful CMake build alone cannot validate the integration.
Results, license file and CycloneDX per-target records are accessible from the BuildEngine-Tests Markdown documentation server at `http://127.0.0.1:8781/docs/`.

### Additional user-supplied tests.zip suite

Extract `BuildEngine-Tests-additional-MSSQL.zip` into the test repository root; then follow `mssql/additional-tests/build.md`. The archive contains all 13 original headers and a separate executable for the existing SQL Server error scenarios.

## One-time migration from the former source location

If this build directory was configured before the move from `tests/mssql` to `tests/database/mssql`, the existing CMake cache contains the old absolute source path. Run the configure command from above **once with `--fresh`** (immediately after `cmake`) to discard only `CMakeCache.txt` and its associated `CMakeFiles` directory. This does not remove BuildEngine package installations, test sources, or the test database. Subsequent normal configure/build invocations do not require `--fresh`.
