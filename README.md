# BuildEngine Tests und Demos

Dieses Repository ist die Heimat der **komplexeren, paketuebergreifenden Tests und Demonstrationen** des C++Builder-Third-Party-Projekts.

Es enthaelt bewusst **nicht** die einfachen Library-Smoke-Tests. Diese sind fester Bestandteil des jeweiligen Bibliotheksvertrags und liegen zusammen mit `build-libraries.xml` unter `BuildEngine-Admin/admin/smokes`.

## Aufgabenteilung

```text
BuildEngine
   Ausfuehrung, Scheduling, ProcessRunner, Validatoren

BuildEngine-Admin
   Bibliotheksvertrag
   source -> build/test -> install -> publish -> smoke -> ready
   einfache, paketbezogene Consumer-Smokes

BuildEngine-Tests
   komplexe, paketuebergreifende Integrations-Tests
   Demos und repraesentative Anwendungsszenarien
```

Die Trennung folgt der Semantik des Tests, nicht der technischen Ausfuehrung:

- Ein **Library-Smoke** beantwortet nur die Frage, ob genau ein publiziertes Paket aus der gemeinsamen `Win64x`-Consumer-Sicht gefunden, kompiliert, gelinkt, geladen und minimal benutzt werden kann. Er gehoert deshalb zum Library-Vertrag in `BuildEngine-Admin`.
- Ein **Integrationstest** kombiniert mehrere publizierte Pakete, Prozesse oder Dienste und prueft deren Zusammenspiel.
- Eine **Demo** zeigt ein groesseres, nachvollziehbares Anwendungsszenario und darf mehrere Bibliotheken und Anwendungsbausteine verwenden.

## Verzeichnisstruktur

```text
BuildEngine-Tests/
   tests/
      <integration-test>/
   demos/
      <demo>/
```

Die Verzeichnisse enthalten keine Bibliotheksversionsnummern, solange nicht wirklich eine inkompatible Testimplementierung fuer eine bestimmte Version notwendig ist.

## Consumer-Sicht

Tests und Demos konsumieren die von BuildEngine publizierte SDK-Sicht:

```text
<InstallRoot>/Win64x/
   include/
   lib/
   bin/
   cmake/
```

Sie sollen keine privaten Build-Verzeichnisse und keine fest verdrahteten Pfade nach `install/packages/<id>/<version>` verwenden. CMake-Projekte benutzen nach Moeglichkeit normale `find_package()`-Aufrufe und die gemeinsame BuildEngine-Consumer-Integration.

## Komplexe Tests

Unter `tests/` gehoeren Szenarien, die ueber einen einzelnen Paket-Smoke hinausgehen, beispielsweise:

- mehrere Bibliotheken gemeinsam in einem Consumer;
- Client/Server- oder Mehrprozess-Tests;
- TLS-/Netzwerk-Interaktion;
- CORBA-Dienste und Clients;
- Zusammenspiel von Datenformat-, Kompressions-, Netzwerk- und Kryptographie-Bibliotheken.

Solche Tests duerfen eine eigene fachliche Auswertung besitzen. Sie ersetzen nicht die kleinen Library-Smokes und sind nicht automatisch Teil des `ready`-Knotens einer einzelnen Bibliothek.

## Demos

Unter `demos/` liegen groessere, reproduzierbare Beispielanwendungen. Dazu gehoert auch die vorgesehene Integration des Financial-Beispiels.

Das bereits vorhandene Repository `adeccscholar/Financial_Example` bleibt dabei eine eigenstaendige Quelle. Eine BuildEngine-Demo kann dieses Beispiel als echten Consumer des publizierten `Win64x`-SDK-Baums einbinden oder seinen Build reproduzierbar anstossen; die Library-Smoke-Logik wird nicht in das Financial-Beispiel kopiert.

## Alter `smoketests`-Baum

Der bisherige `smoketests/`-Baum dieses Repositories wird nach Uebernahme der neuen Admin-Smokes nicht mehr benoetigt. Die einfachen Consumer-Smokes liegen dann ausschliesslich unter:

```text
BuildEngine-Admin/admin/smokes/<library>/consumer/
```

Damit existiert fuer einfache Library-Smokes nur noch eine Quelle und ein Vertrag.

## Anforderungen

Tests und Demos sollen:

- keine absoluten Maschinenpfade enthalten;
- die publizierte `Win64x`-Consumer-Sicht verwenden;
- mit C++Builder/BCC64X ueber die normale Projekt-Toolchain gebaut werden;
- keine privaten Third-Party-Source- oder Build-Verzeichnisse verwenden;
- reproduzierbare Eingaben und nachvollziehbare Logs erzeugen;
- ihren Zweck und die beteiligten Pakete dokumentieren.

## Lizenz

Projekt-eigene Test-, Demo-, CMake- und Dokumentationsdateien stehen unter der MIT-Lizenz des Repositories. Drittanbieterbibliotheken behalten ihre jeweiligen Upstream-Lizenzen.
