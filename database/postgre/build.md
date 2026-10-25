# Build postgre (RAD Studio 13 Developer Command Prompt)

From `BuildEngine/tests`, use the productive BuildEngine root, existing installed libraries, and checked-out `adecc-core-lib` submodule:

```bat
git submodule update --init --recursive
cmake -P bootstrap/Bootstrap.cmake
cmake -S database/postgre -B ../build/Test/postgre/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/postgre/Release
cmake --build ../build/Test/postgre/Release/cmake
cmake --install ../build/Test/postgre/Release/cmake
```

For Debug substitute `Debug` consistently. No local third-party bootstrap, no static archive or package-version guessing.

```bat
psql -U postgres -d postgres -f database\postgre\sql\create-database.sql
psql -U postgres -d BuildEngineTest -f database\postgre\sql\configure-access.sql
..\build\Test\postgre\Release\adecc_postgresql_pqxx_test.exe --host localhost --database BuildEngineTest --user buildenginetest_user --integrated
```

**Server dependency:** A running database server and correctly configured driver, account, network and authentication are required. A successful CMake build alone cannot validate the integration.
Results, license file and CycloneDX per-target records are accessible from the BuildEngine-Tests Markdown documentation server at `http://127.0.0.1:8781/docs/`.

## One-time migration from the former source location

If this build directory was configured before the move from `tests/postgre` to `tests/database/postgre`, the existing CMake cache contains the old absolute source path. Run the configure command from above **once with `--fresh`** (immediately after `cmake`) to discard only `CMakeCache.txt` and its associated `CMakeFiles` directory. This does not remove BuildEngine package installations, test sources, or the test database. Subsequent normal configure/build invocations do not require `--fresh`.
