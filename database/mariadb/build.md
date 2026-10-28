# MariaDB / adecc-core-lib integration

This project uses the **BuildEngine-installed MariaDB Connector/C 3.4.11 DLL**, not a system-installed MinGW or MSVC client, and the `adecc::mariadb` header-only interface target. It validates `framework_database_type`, client loading, and, when explicitly invoked, a live MariaDB 11 connection with an RAII transaction and rollback.

**Current scope:** The connection and transaction layer has been added. Named-parameter queries, typed result conversion, generated identities, and `PersistentSystemData` are **not yet implemented** for MariaDB and the full shared tests from MSSQL/PostgreSQL are therefore **not yet available**. The project must not be represented as full database-backend parity.

## Configure and build

From `D:\local\embarcadero\test_v3\tests` in the RAD Studio developer shell:

```bat
git pull --ff-only
git submodule update --init
cmake -S database/mariadb -B ../build/Test/database/mariadb/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/database/mariadb/Release
cmake --build ../build/Test/database/mariadb/Release/cmake
ctest --test-dir ../build/Test/database/mariadb/Release/cmake --output-on-failure
cmake --install ../build/Test/database/mariadb/Release/cmake
```

Optional live test after provisioning the MariaDB 11 server (`BuildEngineTest`, least-privilege user and valid TLS certificate):

```bat
set BUILDENGINE_MARIADB_HOST=localhost
set BUILDENGINE_MARIADB_PORT=3306
set BUILDENGINE_MARIADB_DATABASE=BuildEngineTest
set BUILDENGINE_MARIADB_USER=buildenginetest_user
set BUILDENGINE_MARIADB_CA=C:\path\to\trusted-server-ca.pem
set BUILDENGINE_MARIADB_PASSWORD=
..\build\Test\database\mariadb\Release\adecc_mariadb_connection_test.exe
```

Do not pass secrets on command lines or check them into source. This test enforces TLS certificate verification. The MariaDB server is an external requirement and is not built by BuildEngine.

## Next contract gates

A passing test requires a native BCC64X Release/Debug compile, installed binary runtime closure, local offline CTest and a live MariaDB 11 authenticated transaction. All are **unverified** until run on the target Windows machine.
