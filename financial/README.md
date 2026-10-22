# Financial Example

Eigenstaendiger Projektbereich im Repository BuildEngine-Tests. Alle Anwendungen liegen direkt im Repository-Wurzelverzeichnis; es gibt keinen separaten demos-Baum mehr.

Das separate Quellrepository [Financial_Example](https://github.com/adeccscholar/Financial_Example) ist fuer Berechnung der IRR und das Lesen von JSON-Dateien vorgesehen. Stand der geprueften GitHub-Hauptbranch: nur README, LICENSE und .gitignore; noch kein CMakeLists.txt und keine C++-Quelldateien. Entsprechend ist financial aktuell **nicht baubar oder installierbar**.

## Vorgesehene einheitliche Befehle

Vom Verzeichnis BuildEngine/Test aus, nachdem Financial_Example tatsaechliche C++-Quellen und eine CMakeLists.txt bereitstellt und in financial integriert ist:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S financial -B ../build/Test/financial/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/financial/Release
cmake --build ../build/Test/financial/Release/cmake
cmake --install ../build/Test/financial/Release/cmake
```

Die gemeinsame Infrastruktur befindet sich in ../cmake/BuildEngineTests.cmake. Ein spaeteres CMake-Projekt muss diese einbinden und fuer jede Anwendung adecc_test_install() mit den expliziten Paketwurzel-Abhaengigkeiten verwenden, damit EXE und erforderliche DLLs im jeweiligen Installationsverzeichnis liegen. Es wird hier **keine leere Fake-Anwendung** angelegt.
