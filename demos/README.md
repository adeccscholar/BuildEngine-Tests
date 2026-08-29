# Demos

Dieses Verzeichnis enthaelt groessere, reproduzierbare Consumer-Beispiele fuer den von BuildEngine publizierten `Win64x`-SDK-Baum.

Demos duerfen mehrere Bibliotheken und Anwendungsbausteine kombinieren. Sie sind bewusst von den kleinen Library-Smokes getrennt, die als fester Bestandteil des Bibliotheksvertrags unter `BuildEngine-Admin/admin/smokes` liegen.

Vorgesehene Struktur:

```text
demos/
   financial/
   <weitere-demo>/
```

Eine Demo soll normale Consumer-Schnittstellen verwenden, insbesondere `find_package()` beziehungsweise die gemeinsame BuildEngine-CMake-Consumer-Integration, und keine privaten Build-Verzeichnisse oder versionsfesten Paketpfade voraussetzen.
