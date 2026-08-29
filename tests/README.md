# Komplexe Integrationstests

Dieses Verzeichnis enthaelt paketuebergreifende Consumer-Tests, die bewusst nicht dem einfachen per-Library-Smoke-Muster entsprechen.

Ein Test darf mehrere publizierte Pakete aus `Win64x`, mehrere Prozesse oder Dienste kombinieren und eine eigene fachliche Auswertung besitzen. Kleine Tests, die lediglich die Nutzbarkeit genau einer Bibliothek pruefen, gehoeren nicht hierher, sondern nach `BuildEngine-Admin/admin/smokes` in den Library-Vertrag.
