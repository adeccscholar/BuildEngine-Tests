# PostgreSQL Configuration — Windows SSPI

## Scope and prerequisites

**Database:** `BuildEngineTest`  
**PostgreSQL role:** `buildenginetest_user`  
**Backend:** BuildEngine libpq 18.6 / libpqxx 8.0.2  
**Authentication:** Windows Integrated Security (SSPI), **without an application password**.

A running PostgreSQL server (Windows in these examples), administrative access for setup and the actual Windows login of the test process are required. The server must be running for tests, not compilation.

**Important for adecc-core-lib:** `boIntegrated=true` **only prevents passing a password parameter** to libpq. The server must explicitly select SSPI in `pg_hba.conf`; otherwise another existing HBA rule may be used. `gssencmode=disable` controls GSS transport encryption, **not** SSPI authentication.

## 1. Locate PostgreSQL configuration files

Connect with an administrative `psql` account:

```sql
SHOW data_directory;
SHOW hba_file;
SHOW ident_file;
SHOW port;
SHOW listen_addresses;
```

For a local Windows test server use `localhost:5432` unless configured differently. Remote domain clients may require a Windows service identity and Kerberos SPN `POSTGRES/<server-fqdn>`. For local SSPI on the same Windows computer, a domain SPN is not required. On Linux, PostgreSQL normally uses server-side GSSAPI instead of SSPI.

## 2. Create database and role

From the BuildEngine-Tests root, initially use a **PostgreSQL administrative account** whose authentication is configured separately:

```bat
psql -X -v ON_ERROR_STOP=1 -h localhost -U postgres -d postgres -f database\postgre\sql\create-database.sql

psql -X -v ON_ERROR_STOP=1 -h localhost -U postgres -d BuildEngineTest -f database\postgre\sql\configure-access.sql
```

The first [SQL script](sql/create-database.sql) creates `BuildEngineTest` when needed. The second [script](sql/configure-access.sql) creates `buildenginetest_user` with LOGIN and grants CONNECT and TEMPORARY. Backend tests create **temporary tables**, so general DDL privileges in schema `public` are unnecessary.

If `BuildEngineTest` is truly dedicated to these tests, restricting CONNECT previously available through PUBLIC may be appropriate, but only after checking other users:

```sql
REVOKE CONNECT ON DATABASE "BuildEngineTest" FROM PUBLIC;
GRANT CONNECT ON DATABASE "BuildEngineTest" TO buildenginetest_user;
```

## 3. Enforce SSPI in pg_hba.conf

Place these rules **before** general host rules. They apply only to the specified test database, role and localhost:

```conf
# TYPE  DATABASE         USER                  ADDRESS       METHOD  OPTIONS
host    BuildEngineTest  buildenginetest_user  127.0.0.1/32  sspi    map=be_test include_realm=1
host    BuildEngineTest  buildenginetest_user  ::1/128       sspi    map=be_test include_realm=1
```

The **first matching HBA rule** determines authentication. Do not let earlier general trust, password or scram rules shadow the SSPI rules. For remote clients authorize only specific addresses. TLS/certificate validation and SSPI authentication protect different layers.

## 4. Map Windows identity in pg_ident.conf

PostgreSQL must map the external Windows principal to a database role:

```conf
# MAPNAME  SYSTEM-USERNAME   PG-USERNAME
be_test    entwickler@FIRMA   buildenginetest_user
```

**`entwickler@FIRMA` is a placeholder**, not a universal account. With `include_realm=1` and default `compat_realm=1`, the mapping commonly uses **SAM-user@SAM-domain**. Determine the real authenticated principal on the server. Do not blindly copy the `whoami` output, which typically uses `FIRMA\entwickler`.

The map name `be_test` must match in both files. After editing, reload and inspect configuration errors:

```sql
SELECT pg_reload_conf();
SELECT * FROM pg_hba_file_rules WHERE error IS NOT NULL;
SELECT * FROM pg_ident_file_mappings WHERE error IS NOT NULL;
```

On Windows, changes to HBA rules apply immediately to new connections; reload is particularly important for the identity map.

## 5. Test the connection without a password

Run under the intended Windows login:

```bat
whoami

psql -h localhost -p 5432 -U buildenginetest_user -d BuildEngineTest -w -c "SELECT current_database(), session_user, current_user;"

..\build\Test\postgre\Release\adecc_postgresql_pqxx_test.exe --host localhost --database BuildEngineTest --user buildenginetest_user --integrated
```

`-w` prohibits a password prompt. A successful connection proves SSPI **only if the matching HBA rule actually selects `sspi`**, not `trust`.

By default, the test program uses `BUILDENGINE_PGHOST`, `BUILDENGINE_PGPORT`, `BUILDENGINE_PGDATABASE`, `BUILDENGINE_PGUSER` and `BUILDENGINE_PG_INTEGRATED=1`. See [build.md](build.md) for build and staging.

## 6. Different database roles for the same Windows account

**Supported.** PostgreSQL roles are cluster-wide, but authentication rules and GRANT/REVOKE permissions may differ by database. One Windows account can therefore connect under different PostgreSQL roles depending on the requested database.

Assume another already provisioned database `AndereDB` and another role `andereapp_user`. Their administration is **outside** the BuildEngine test scripts.

In `pg_hba.conf`:

```conf
host  BuildEngineTest  buildenginetest_user  127.0.0.1/32  sspi  map=be_test include_realm=1
host  AndereDB         andereapp_user       127.0.0.1/32  sspi  map=andere_app include_realm=1
```

In `pg_ident.conf`:

```conf
be_test     entwickler@FIRMA  buildenginetest_user
andere_app  entwickler@FIRMA  andereapp_user
```

The **same Windows login** can thus connect as `buildenginetest_user` to `BuildEngineTest` and as `andereapp_user` to `AndereDB`. The client must explicitly request the corresponding PostgreSQL role through `user` or `-U`.

These are different roles with separate privileges. Conversely, a PostgreSQL cluster cannot contain separate independent roles with identical names in different databases. Also review database CONNECT (including PUBLIC), schema and object permissions.

## 7. Troubleshooting and security

- **no pg_hba.conf entry:** Check the requested database, role, IPv4/IPv6 and first matching host rule.
- **SSPI mapping failed:** Check `include_realm`, actual domain/realm, principal and map name.
- **password authentication failed:** Often an earlier HBA rule is selected. Do not introduce passwords as a workaround.
- **permission denied for database:** Check CONNECT and possibly TEMPORARY on `BuildEngineTest`.
- **Kerberos/SPN failure:** Verify FQDN, service identity, `POSTGRES` SPN and any domain controller. SSPI may also negotiate NTLM; an SSPI login therefore does not necessarily prove Kerberos.
- **Encryption:** `sslmode=prefer` does not validate a trusted server certificate. Explicitly configure protected transport and certificate verification for remote access.

## Documentation server and references

This page is exposed at `/docs/database/postgre/Configure.md`. The Markdown documentation server is **not** a PostgreSQL server.

- [PostgreSQL 18 — SSPI](https://www.postgresql.org/docs/18/sspi-auth.html)
- [PostgreSQL 18 — pg_hba.conf](https://www.postgresql.org/docs/18/auth-pg-hba-conf.html)
- [PostgreSQL 18 — User Name Maps](https://www.postgresql.org/docs/18/auth-username-maps.html)
- [PostgreSQL 18 — Database Roles](https://www.postgresql.org/docs/18/database-roles.html)
