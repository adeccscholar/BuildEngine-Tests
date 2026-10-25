# Configure.md – PostgreSQL / Windows-SSPI

## Ziel und Voraussetzungen

**Datenbank:** BuildEngineTest  
**PostgreSQL-Rolle:** buildenginetest_user  
**Backend:** libpq 18.6 / libpqxx 8.0.2 aus der BuildEngine  
**Authentifizierung:** Windows Integrated Security (SSPI), **ohne Anwendungspasswort**.

Benötigt werden ein laufender PostgreSQL-Server (hier: Windows), ein administrativer Zugriff zur Einrichtung und die aktuelle Windows-Anmeldeidentität des Testprogramms. Ein laufender Datenbankserver ist für die Ausführung erforderlich, nicht für das Kompilieren.

**Wichtig für adecc-core-lib:** boIntegrated=true **verhindert lediglich die Übergabe eines Passwortparameters** an libpq. SSPI muss in der Serverdatei **pg_hba.conf** festgelegt sein. Andernfalls könnte eine andere bestehende HBA-Regel greifen. gssencmode=disable ist eine Einstellung für GSS-Transportverschlüsselung, nicht für die SSPI-Anmeldung.

## 1. PostgreSQL-Konfiguration ermitteln

Administrativ mit psql verbinden und die tatsächlichen Konfigurationspfade ermitteln:

~~~sql
SHOW data_directory;
SHOW hba_file;
SHOW ident_file;
SHOW port;
SHOW listen_addresses;
~~~

Auf einem lokalen Windows-Testserver üblicherweise **localhost:5432** verwenden. Für entfernte Domain-Clients ist eine passende Windows-Serviceidentität mit Kerberos/SPN **POSTGRES/<server-fqdn>** erforderlich. Auf demselben Windows-Rechner ist für localhost-SSPI kein Domänen-SPN erforderlich. Ein PostgreSQL-Server unter Linux verwendet anstelle der serverseitigen SSPI-Methode üblicherweise GSSAPI.

## 2. Datenbank und Rolle einrichten

Aus dem BuildEngine-Tests-Verzeichnis, zunächst mit einem **PostgreSQL-Administratorkonto** (dessen Einrichtungsauthentifizierung separat geregelt ist):

~~~bat
psql -X -v ON_ERROR_STOP=1 -h localhost -U postgres -d postgres -f database\postgre\sql\create-database.sql

psql -X -v ON_ERROR_STOP=1 -h localhost -U postgres -d BuildEngineTest -f database\postgre\sql\configure-access.sql
~~~

Das erste [SQL-Skript](sql/create-database.sql) legt BuildEngineTest bei Bedarf an. Das zweite [SQL-Skript](sql/configure-access.sql) erstellt buildenginetest_user mit LOGIN und erteilt der Rolle CONNECT und TEMPORARY auf der Datenbank. Die Backendtests erzeugen **temporäre Tabellen**, deshalb sind keine allgemeinen DDL-Rechte im Schema public erforderlich.

Falls BuildEngineTest tatsächlich ausschließlich dafür vorgesehen ist, kann man den standardmäßig oft über PUBLIC möglichen CONNECT-Zugriff einschränken. Nur nach Prüfung möglicher weiterer Anwender administrativ ausführen:

~~~sql
REVOKE CONNECT ON DATABASE "BuildEngineTest" FROM PUBLIC;
GRANT CONNECT ON DATABASE "BuildEngineTest" TO buildenginetest_user;
~~~

## 3. SSPI in pg_hba.conf erzwingen

Diese Regeln **vor** allgemeineren Host-Regeln einfügen. Sie gelten ausschließlich für die Datenbank BuildEngineTest, die angegebene PostgreSQL-Rolle und localhost:

~~~conf
# TYPE  DATABASE         USER                  ADDRESS       METHOD  OPTIONS
host    BuildEngineTest  buildenginetest_user  127.0.0.1/32  sspi    map=be_test include_realm=1
host    BuildEngineTest  buildenginetest_user  ::1/128       sspi    map=be_test include_realm=1
~~~

Die **erste** passende HBA-Regel entscheidet endgültig. Allgemeine trust-, password- oder scram-Regeln dürfen diese SSPI-Regeln nicht überdecken. Für entfernte Rechner ausschließlich konkrete Clientadressen freischalten; TLS/Zertifikatsprüfung und die Windows-SSPI-Authentifizierung sind unterschiedliche Sicherheitsebenen.

## 4. Windows-Identität in pg_ident.conf zuordnen

PostgreSQL muss den externen Windows-Benutzer auf die PostgreSQL-Rolle abbilden. Beispiel:

~~~conf
# MAPNAME  SYSTEM-USERNAME   PG-USERNAME
be_test    entwickler@FIRMA   buildenginetest_user
~~~

**entwickler@FIRMA** ist ein **Platzhalter**, kein universeller Kontoname. Bei SSPI mit include_realm=1 und dem Standard compat_realm=1 erfolgt die Zuordnung üblicherweise als **SAM-Benutzer@SAM-Domäne**. Den tatsächlich authentifizierten Prinzipalnamen am Server ermitteln und exakt einsetzen. Nicht einfach die Ausgabe von whoami (meist FIRMA\entwickler) ungeprüft übernehmen.

Der Map-Name **be_test** muss in beiden Dateien identisch sein. Nach dem Editieren administrativ neu einlesen und eventuelle Konfigurationsfehler prüfen:

~~~sql
SELECT pg_reload_conf();
SELECT * FROM pg_hba_file_rules WHERE error IS NOT NULL;
SELECT * FROM pg_ident_file_mappings WHERE error IS NOT NULL;
~~~

Unter Windows werden Änderungen der HBA-Datei bei neuen Verbindungen unmittelbar berücksichtigt; der Reload ist insbesondere für die Zuordnungstabelle relevant.

## 5. Verbindung ohne Passwort testen

Als das später eingesetzte Windows-Konto im Developer Command Prompt:

~~~bat
whoami

psql -h localhost -p 5432 -U buildenginetest_user -d BuildEngineTest -w -c "SELECT current_database(), session_user, current_user;"

..\build\Test\postgre\Release\adecc_postgresql_pqxx_test.exe --host localhost --database BuildEngineTest --user buildenginetest_user --integrated
~~~

Der psql-Schalter **-w** verhindert Passwortabfragen. Ein erfolgreicher Login ist nur dann ein **SSPI-Nachweis**, wenn die tatsächlich passende pg_hba.conf-Regel die Methode sspi verwendet (nicht trust). Das Testprogramm verwendet standardmäßig die Umgebungsvariablen **BUILDENGINE_PGHOST**, **BUILDENGINE_PGPORT**, **BUILDENGINE_PGDATABASE**, **BUILDENGINE_PGUSER** und **BUILDENGINE_PG_INTEGRATED=1**. Build und Staging: [build.md](build.md).

## 6. Unterschiedliche Rollen je Datenbank – ausdrücklich möglich

**Ja.** PostgreSQL-Rollen gelten **clusterweit**, nicht nur für eine einzelne Datenbank. Authentifizierungsregeln und **GRANT/REVOKE**-Rechte können aber pro Datenbank unterschiedlich sein. Ein und dasselbe Windows-Konto kann deshalb je nach angeforderter Datenbank eine andere PostgreSQL-Rolle benutzen.

Angenommen, neben BuildEngineTest gibt es eine bereits administrativ eingerichtete zweite Datenbank **AndereDB** und die Rolle **andereapp_user**. Ihre Erstellung und Rechteverwaltung gehören **nicht** zu den BuildEngine-Testskripten.

In **pg_hba.conf**:

~~~conf
host  BuildEngineTest  buildenginetest_user  127.0.0.1/32  sspi  map=be_test include_realm=1
host  AndereDB         andereapp_user       127.0.0.1/32  sspi  map=andere_app include_realm=1
~~~

In **pg_ident.conf**:

~~~conf
be_test     entwickler@FIRMA  buildenginetest_user
andere_app  entwickler@FIRMA  andereapp_user
~~~

**Derselbe Windows-Login** kann so als **buildenginetest_user** auf BuildEngineTest und als **andereapp_user** auf AndereDB arbeiten. Der Client muss den jeweiligen PostgreSQL-Rollennamen mit **user** beziehungsweise **-U** anfordern.

Die beiden Rollen sind verschieden und besitzen getrennte Rechte. **Gleichnamige Rollen je Datenbank mit unabhängigem Inhalt gibt es innerhalb eines Clusters dagegen nicht**. Für die beabsichtigte Isolation zusätzlich CONNECT-Rechte (einschließlich PUBLIC) und Rechte auf Schemas und Objekten prüfen.

## 7. Diagnose / Sicherheit

- **no pg_hba.conf entry:** angeforderte Datenbank, Rolle, IPv4/IPv6 und erste passende Hostregel prüfen.
- **SSPI mapping failed:** include_realm, echte Domäne/Realm, zugeordnetes Konto sowie Map-Namen prüfen.
- **password authentication failed:** meist greift die falsche, frühere HBA-Regel; keine Passwörter als „Workaround“ einführen.
- **permission denied for database:** CONNECT und gegebenenfalls TEMPORARY auf BuildEngineTest prüfen.
- **Kerberos/SPN-Probleme:** FQDN, Dienstkonto, POSTGRES-SPN und gegebenenfalls Domain-Controller prüfen. SSPI kann auch NTLM verwenden; erfolgreicher SSPI-Login beweist daher nicht zwingend Kerberos.
- **Verschlüsselung:** sslmode=prefer prüft kein vertrauenswürdiges Serverzertifikat. Für entfernte Verbindungen gesicherten Transport und Zertifikatsprüfung ausdrücklich konfigurieren.

## Dokumentationsserver / Referenzen

Diese Datei erscheint unter **/docs/database/postgre/Configure.md**. Der Markdown-Server ist **kein PostgreSQL-Server**.

- [PostgreSQL 18 – SSPI](https://www.postgresql.org/docs/18/sspi-auth.html)
- [PostgreSQL 18 – pg_hba.conf](https://www.postgresql.org/docs/18/auth-pg-hba-conf.html)
- [PostgreSQL 18 – User Name Maps](https://www.postgresql.org/docs/18/auth-username-maps.html)
- [PostgreSQL 18 – Database Roles](https://www.postgresql.org/docs/18/database-roles.html)
