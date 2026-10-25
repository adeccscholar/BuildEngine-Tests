<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – BCC64X Build

Aus `BuildEngine/tests` mit der produktiven Installation:

```bat
git pull
cmake -S WeatherClient -B ../build/Test/WeatherClient/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/WeatherClient/Release
cmake --build ../build/Test/WeatherClient/Release/cmake
cmake --install ../build/Test/WeatherClient/Release/cmake
```

Die CMake-Datei bezieht alle **aktiven** Versionen aus
`admin/build-libraries.xml`: Skia, SDL2, curl, OpenSSL, Boost,
nlohmann-json und die Skia-/HTTP-Laufzeitpakete. Es wird **ausschließlich
dynamisch** gelinkt; das selektive PE-Import-Staging kopiert nur
tatsächlich importierte DLLs.

```bat
..\build\Test\WeatherClient\Release\WeatherClient.exe --demo
..\build\Test\WeatherClient\Release\WeatherClient.exe
```

Zum Testen des Datensatzes `--self-test` verwenden.
Die reale Standortsuche benötigt HTTPS, gültige CA-Zertifikate sowie
Netzzugriff auf die konfigurierten Anbieter. Die Anwendung erzeugt
`weather-location.json` nur nach Eingabe einer Adresse; für
reproduzierbare Tests den Demo-Modus verwenden.

Der bisherige `WeatherAPIServer` nutzt CORBA/TAO und ältere
`BoostTools` und wird in diesem SDL2-Programm **nicht benötigt**.
Die gelieferten Originaldateien wurden nicht verändert.

[Beschreibung](Description.md) · [Lizenzen](LICENSE.md) · [SBOM](SBOM.md)
