# SQL Server Configuration — Windows Integrated Authentication

## Test environment

**Database:** `BuildEngineTest`  
**Test executables:** `adecc_mssql_odbc_test.exe` and `adecc_mssql_csv_pipeline.exe`  
**Authentication:** Windows Integrated Security (SSPI) through Microsoft ODBC Driver 18.

Prerequisites: a running Microsoft SQL Server instance, **Microsoft ODBC Driver 18 for SQL Server**, a Windows account permitted to access the test instance, and an SQL Server administrator for initial setup. The server is an external prerequisite, **not** a service installed by CMake or BuildEngine. It need not be running during compilation, but it must be available for tests.

Identify the Windows account used by the RAD Studio Developer Command Prompt:

```bat
whoami
```

**`FIRMA\entwickler` is a placeholder**, not a real credential. Replace it with the actual Windows account or authorized Windows group. A named instance may use `localhost\SQLEXPRESS` instead of `localhost`.

## 1. Create the database

Run as a Windows account with SQL Server administrative permissions:

```bat
sqlcmd -S localhost -E -b -i database\mssql\sql\create-database.sql
```

Alternatively execute [create-database.sql](sql/create-database.sql) in SQL Server Management Studio (SSMS) using **Windows Authentication**. The script creates `BuildEngineTest` only if missing. `-E` enables integrated authentication and `-b` reports SQL errors through the process exit code.

## 2. Create the Windows login and database user

Execute as an SQL Server administrator. Map the Windows login to database user `buildenginetest_user` without creating an SQL password.

```sql
USE [master];
GO

-- Replace FIRMA\entwickler before executing.
IF SUSER_ID(N'FIRMA\entwickler') IS NULL
   CREATE LOGIN [FIRMA\entwickler] FROM WINDOWS;
GO

USE [BuildEngineTest];
GO

IF USER_ID(N'buildenginetest_user') IS NULL
   CREATE USER [buildenginetest_user] FOR LOGIN [FIRMA\entwickler];
GO
```

If the identity already exists, verify its login mapping. A Windows group login can be more suitable than individual developer logins for shared team machines.

## 3. Permissions

The current tests create tables and triggers explicitly under `dbo`, execute SELECT/INSERT/UPDATE/DELETE, then remove test objects. **Only in the dedicated test database**, grant:

```sql
USE [BuildEngineTest];
GO
GRANT CONNECT TO [buildenginetest_user];
GRANT CREATE TABLE TO [buildenginetest_user];
GRANT ALTER ON SCHEMA::[dbo] TO [buildenginetest_user];
GRANT SELECT, INSERT, UPDATE, DELETE
   ON SCHEMA::[dbo] TO [buildenginetest_user];
GO
```

**Security boundary:** `ALTER` on `dbo` is powerful. Grant it only within an isolated test database, never production. Alternatively, a fully dedicated test database may use `db_ddladmin`, `db_datareader` and `db_datawriter`. `sysadmin` or `db_owner` are not normally required.

## 4. Verify login and run programs

Run these commands **under the Windows account that will execute the tests**:

```bat
sqlcmd -S localhost -E -d BuildEngineTest -b -Q "SELECT DB_NAME() AS Datenbank, SYSTEM_USER AS WindowsLogin, USER_NAME() AS DBUser;"

..\build\Test\mssql\Release\adecc_mssql_odbc_test.exe --server localhost --database BuildEngineTest --integrated

set ADECC_MSSQL_DATABASE=BuildEngineTest
..\build\Test\mssql\Release\adecc_mssql_csv_pipeline.exe
```

Expect `BuildEngineTest`, the correct Windows login and `buildenginetest_user`. The CSV pipeline also requires `bookings.csv` in its runtime directory. See [build.md](build.md) for build and runtime staging.

## 5. Troubleshooting and security limits

- **Login failed:** Check `whoami`, the SQL login, database user and CONNECT permissions. Integrated security uses the process identity, not a separately supplied username.
- **CREATE TABLE / TRIGGER denied:** Check CREATE TABLE and ALTER permission on schema `dbo`.
- **ODBC driver not found:** Install 64-bit ODBC Driver 18 and verify the instance name.
- **Transport encryption:** The current adecc MSSQL adapter explicitly supplies both `Trusted_Connection=Yes` and `Encrypt=No`. Windows authentication therefore **does not demonstrate TLS**. This profile is suitable only for an isolated, trusted test network. Require TLS and certificate validation for remote access.

## Documentation server and evidence

The Markdown documentation server exposes this page at `/docs/database/mssql/Configure.md`; it is **not** the SQL Server itself.

References:

- [Microsoft — Windows Login](https://learn.microsoft.com/en-us/sql/relational-databases/security/authentication-access/create-a-login)
- [Microsoft — Database User](https://learn.microsoft.com/en-us/sql/relational-databases/security/authentication-access/create-a-database-user)
- [Microsoft — Schema permissions](https://learn.microsoft.com/en-us/sql/t-sql/statements/grant-schema-permissions-transact-sql)
- [Microsoft — ODBC encryption](https://learn.microsoft.com/en-us/sql/connect/odbc/dsn-connection-string-attribute)
