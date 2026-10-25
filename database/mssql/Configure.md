# Configure.md – SQL Server / integrierte Sicherheit

## Testumgebung

**Datenbank:** BuildEngineTest  
**Testprogramm:** adecc_mssql_odbc_test.exe und adecc_mssql_csv_pipeline.exe  
**Authentifizierung:** Windows Integrated Security (SSPI) über ODBC Driver 18.

Erforderlich sind eine laufende Microsoft-SQL-Server-Instanz, der installierte **Microsoft ODBC Driver 18 for SQL Server**, ein Windows-Konto mit Zugriff auf die Testinstanz und zur Ersteinrichtung ein SQL-Server-Administrator. Der Server ist eine externe Voraussetzung, kein durch CMake oder die BuildEngine bereitgestellter Dienst. Für den Build muss der Server nicht laufen, für die Tests jedoch schon.

In einem RAD Studio Developer Command Prompt das tatsächlich verwendete Windows-Konto feststellen:

~~~bat
whoami
~~~

Die Schreibweise **FIRMA\entwickler** in den folgenden Beispielen ist ein **Platzhalter**. Sie muss durch das reale Windows-Konto oder die autorisierte Windows-Gruppe ersetzt werden. Eine benannte SQL-Server-Instanz wird beispielsweise mit **localhost\SQLEXPRESS** anstelle von localhost angegeben.

## 1. Datenbank einrichten

Als Windows-Konto mit Administratorrechten auf der SQL-Server-Instanz:

~~~bat
sqlcmd -S localhost -E -b -i database\mssql\sql\create-database.sql
~~~

Oder das vorhandene [create-database.sql](sql/create-database.sql) in SQL Server Management Studio (SSMS) mit **Windows Authentication** ausführen. Das Skript erstellt BuildEngineTest nur, wenn die Datenbank noch nicht existiert. **-E** aktiviert integrierte Sicherheit; **-b** meldet SQL-Fehler über den Prozess-Exitcode.

## 2. Windows-Login und Datenbankbenutzer erstellen

Die folgenden Befehle führt ein SQL-Server-Administrator aus. Hier wird ein Windows-Login einem Datenbankbenutzer **buildenginetest_user** zugeordnet. Es wird **kein SQL-Kennwort** eingerichtet.

~~~sql
USE [master];
GO

-- FIRMA\entwickler vor dem Ausführen ersetzen!
IF SUSER_ID(N'FIRMA\entwickler') IS NULL
   CREATE LOGIN [FIRMA\entwickler] FROM WINDOWS;
GO

USE [BuildEngineTest];
GO

IF USER_ID(N'buildenginetest_user') IS NULL
   CREATE USER [buildenginetest_user] FOR LOGIN [FIRMA\entwickler];
GO
~~~

Existiert die Kennung bereits, ihre bestehende Login-Zuordnung prüfen. Für Team-Rechner ist ein Windows-Gruppenlogin oft praktikabler als einzelne Entwickler-Logins.

## 3. Berechtigungen

Die gegenwärtigen Tests legen Tabellen und Trigger explizit unter **dbo** an, benutzen SELECT/INSERT/UPDATE/DELETE und entfernen Testobjekte wieder. Für die **isolierte Testdatenbank**:

~~~sql
USE [BuildEngineTest];
GO
GRANT CONNECT TO [buildenginetest_user];
GRANT CREATE TABLE TO [buildenginetest_user];
GRANT ALTER ON SCHEMA::[dbo] TO [buildenginetest_user];
GRANT SELECT, INSERT, UPDATE, DELETE
   ON SCHEMA::[dbo] TO [buildenginetest_user];
GO
~~~

**Sicherheitsgrenze:** ALTER auf dbo ist weitreichend. Solche Berechtigungen sind nur für die gesonderte Testdatenbank vorgesehen, niemals für eine Produktivdatenbank. Alternativ sind in einer vollständig dedizierten Testdatenbank die Rollen db_ddladmin, db_datareader und db_datawriter möglich. Standardmäßig wird kein sysadmin- oder db_owner-Recht benötigt.

## 4. Anmeldung und Programmausführung prüfen

Das folgende Kommando **unter dem späteren Windows-Testkonto** aufrufen:

~~~bat
sqlcmd -S localhost -E -d BuildEngineTest -b -Q "SELECT DB_NAME() AS Datenbank, SYSTEM_USER AS WindowsLogin, USER_NAME() AS DBUser;"

..\build\Test\mssql\Release\adecc_mssql_odbc_test.exe --server localhost --database BuildEngineTest --integrated

set ADECC_MSSQL_DATABASE=BuildEngineTest
..\build\Test\mssql\Release\adecc_mssql_csv_pipeline.exe
~~~

Erwartet werden BuildEngineTest, das korrekte Windows-Login und die Datenbankkennung buildenginetest_user. Für die CSV-Pipeline muss zusätzlich die Beispieldatei **bookings.csv** im Laufzeitverzeichnis vorhanden sein. Build und Runtime-Staging: [build.md](build.md).

## 5. Diagnose und Grenzen

- **Login failed:** Windows-Konto per whoami sowie Login, Datenbankbenutzer und CONNECT prüfen. Integrierte Sicherheit verwendet die Prozessidentität, nicht eine separat übergebene Benutzerkennung.
- **CREATE TABLE / TRIGGER denied:** CREATE TABLE und ALTER auf dem Schema dbo prüfen.
- **ODBC driver not found:** 64-Bit-ODBC Driver 18 installieren und Instanznamen prüfen.
- **Transportverschlüsselung:** Der aktuelle adecc-MSSQL-Adapter übergibt ausdrücklich **Trusted_Connection=Yes** und ebenfalls **Encrypt=No**. Windows-Authentifizierung ist daher **kein** Beleg für TLS. Diese Konfiguration ist nur für ein isoliertes, vertrauenswürdiges Testnetz geeignet; für entfernte Verbindungen muss die Client-Konfiguration auf verpflichtendes TLS mit Zertifikatsprüfung umgestellt werden.

## Dokumentationsserver / Nachweise

Diese Seite wird automatisch über den Markdown-Dokumentationsserver unter **/docs/database/mssql/Configure.md** angezeigt. Der Dokumentationsserver ist nicht der SQL Server.

Referenzen:

- [Microsoft – Windows Login](https://learn.microsoft.com/en-us/sql/relational-databases/security/authentication-access/create-a-login)
- [Microsoft – Database User](https://learn.microsoft.com/en-us/sql/relational-databases/security/authentication-access/create-a-database-user)
- [Microsoft – Schema-Rechte](https://learn.microsoft.com/en-us/sql/t-sql/statements/grant-schema-permissions-transact-sql)
- [Microsoft – ODBC-Verschlüsselung](https://learn.microsoft.com/en-us/sql/connect/odbc/dsn-connection-string-attribute)
