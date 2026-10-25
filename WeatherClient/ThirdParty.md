<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient: Third-Party-Bibliotheken, Runtime und Lizenzen

[TOC]

## Maßgebliche Referenzen

Diese Aufstellung basiert auf dem **aktuellen** [CMakeLists.txt](CMakeLists.txt), der BuildEngine-Consumer-Integration [../cmake/BuildEngineTests.cmake](../cmake/BuildEngineTests.cmake), [SBOM-WeatherClient.cdx.json](SBOM-WeatherClient.cdx.json), [SBOM.md](SBOM.md) und [LICENSE.md](LICENSE.md). Der deklarierte CycloneDX-Status lautet **`declared-only-unverified`**. Die Liste unterscheidet ausdrücklich:

1. **Direkte Verwendung**: Bibliotheken, die WeatherClient im Quelltext oder CMake unmittelbar verwendet.
2. **Deklarierte Paket-Closure**: installierte BuildEngine-Paketroots, aus denen die benötigten PE-Import-DLLs aufgelöst werden dürfen. Sie behauptet **nicht**, dass alle Pakete vollständig in die Anwendung kopiert werden.
3. **Tatsächliche Laufzeit-Closure**: erst nach dem BCC64X-Build durch PE-Imports und das BuildEngine-Staging verifizierbar. Aktuelle Versionen und Dateinamen sind nicht aus dem Testprojekt allein ableitbar.

Die **einzig aktive Bibliotheksversion** wird durch `adecc_package_current_root` aus `admin/build-libraries.xml` bestimmt; als `supersededBy` markierte Varianten sind nicht aktiv. Es werden keine festen Versionsnummern in der Test-CMake-Datei hinterlegt. Änderungen am Katalog erzwingen eine neue CMake-Konfiguration.

## Unmittelbar verwendete Bibliotheken

| Bibliothek | Einsatz im WeatherClient | Art der Einbindung | Lizenz (deklarierte Referenz) |
| --- | --- | --- | --- |
| **Skia** | 2D-Zeichnung, Raster-Surface, Farben, Verläufe, Zeiger, Wettergrafiken, Schrift | `find_package(Skia CONFIG)`, `Skia::skia` | BSD-3-Clause; Komponentenlizenzen beachten |
| **SDL2** | Fenster, Tastatur/Maus, Events, Software-Renderer, Texture-Present | `find_package(SDL2 CONFIG)`, `SDL2::SDL2` | Zlib |
| **Boost** | `boost::statechart`, `boost::mpl`; FSM und Übergänge | Headers mit produktivem BCC64X-Clang-`Boost.Config`-Override | BSL-1.0 |
| **libcurl** | HTTPS-GET, URL-Encoding, CA-/TLS-Prüfung, Antwort und HTTP-Status | importiertes dynamisches Ziel `WeatherCurl` | curl |
| **OpenSSL** | TLS-/Krypto-Backend des verwendeten curl-/Windows-Stacks | `WeatherSSL`, `WeatherCrypto`, Import-Libraries | Apache-2.0 |
| **nlohmann/json** | Parsen der Wetter-/Geocoding-Antworten und lokale JSON-Einstellungen | Include-only aus installiertem Paket | MIT |

**Wichtig:** `Boost.Statechart` wird hier über die installierten Header genutzt. Das CMake-Projekt verlinkt **keine frei geratenen Boost-DLLs** und stellt nicht automatisch einen separaten Boost-Binary-Consumer dar. Umgekehrt verlinken Skia, SDL2 und curl gegen dynamische Import-Libraries; die endgültigen System- und Dritt-DLLs hängen von den PE-Imports der tatsächlich ausgewählten BuildEngine-Pakete ab.

## Deklarierte weitere Paketroots

Die folgenden Pakete sind im CMake-Loop explizit Teil von `WEATHER_PACKAGE_ROOTS`, auch wenn WeatherClient ihre APIs nicht unmittelbar aus dem eigenen Quelltext aufruft. Ihre tatsächliche Notwendigkeit ergibt sich erst aus den importierten DLLs der ausgewählten Versionen.

| Paketkennung in BuildEngine | Typische Funktion im Abhängigkeitsstack | Lizenz laut Projekt-SBOM |
| --- | --- | --- |
| `zlib` | Verlustfreie Komprimierung und mögliche Bild-/Netzwerkabhängigkeit | Zlib |
| `brotli` | Komprimierung, möglicher Skia-/HTTP-Stack | MIT |
| `zstd` | Komprimierung, paketabhängig | BSD-3-Clause |
| `xz` | liblzma-Komprimierung, paketabhängig | 0BSD, Komponenten prüfen |
| `bzip2` | Komprimierung, paketabhängig | bzip2-1.0.6 |
| `libjpeg-turbo` | JPEG-Decodierung, Skia-Umfeld | IJG und BSD-3-Clause, Notices prüfen |
| `libpng` | PNG-Decodierung, Skia-Umfeld | libpng-2.0 |
| `harfbuzz` | Schriftformung im Rendering-Stack | MIT |
| `freetype` | Font-Rasterisierung, Rendering-Stack | FTL oder GPL-2.0-or-later, gewählte Lizenz beachten |

Dies sind **potenzielle transitive Abhängigkeiten**, keine Behauptung, dass jeder dieser Codepfade zur Laufzeit genutzt wird. Insbesondere darf aus einer Paketroot-Liste **nicht** gefolgert werden, dass alle Dateien daraus ausgeliefert werden müssen.

## Plattform- und Toolchain-Abhängigkeiten

- **C++23 / BCC64X (RAD Studio 13)**: `CXX_STANDARD 23`, Erweiterungen aus; das Programm wird als `WIN32`-Subsystem-Anwendung mit `-Wl,--subsystem,windows` gebaut.
- **Windows-Systembibliotheken**: `ws2_32` und `crypt32` werden ausdrücklich gelinkt. Das sind Betriebssystemkomponenten, **keine** eigenständig lizenzierten BuildEngine-Third-Party-Pakete.
- **C++-Runtime-DLLs**: `libc++-370.dll` und `libunwind-370.dll` werden als `TOOLCHAIN_DLLS` für das kontrollierte Runtime-Staging angegeben. Die Namen sind im aktuellen Consumer-Vertrag konkret festgehalten und dürfen nicht durch zufällig auf `PATH` gefundene DLLs ersetzt werden.
- **Segoe UI**: Skia bezieht die Betriebssystemschrift zur Laufzeit, falls verfügbar; Schriftdateien werden **nicht** in die Anwendung kopiert. `WeatherText.h` enthält eine eigene Grafik-Fallback-Darstellung. Der Font ist keine Bibliothek des BuildEngine-Paketloops.
- **CMake und Ninja**: Build-Werkzeuge, keine unmittelbaren Laufzeit-Dependencies des WeatherClient.
- **`curl.exe`**: Optionales unabhängiges CLI-Diagnosewerkzeug, **kein Bestandteil der WeatherClient-Runtime**. `libcurl` wird in der Anwendung eingebunden. Siehe [REST.md](REST.md).

## Auflösung und DLL-Staging (aus CMake)

```cmake
foreach(_id IN ITEMS skia sdl2 curl openssl boost nlohmann-json zlib
   brotli zstd xz bzip2 libjpeg-turbo libpng harfbuzz freetype)
   string(REPLACE "-" "_" _name "${_id}")
   string(TOUPPER "${_name}" _name)
   adecc_package_current_root("${_name}_PACKAGE" "${_id}")
   list(APPEND WEATHER_PACKAGE_ROOTS "${${_name}_PACKAGE}")
endforeach()
```

Die tatsächliche CMake-Datei prüft zusätzlich definierte Import-Libraries von curl und OpenSSL sowie die Verfügbarkeit von `nlohmann/json.hpp`. Es werden `WeatherCurl`, `WeatherSSL` und `WeatherCrypto` als **SHARED IMPORTED** deklariert. `adecc_test_use_boost` stellt die produktive BCC64X-native Clang-Klassifikation für Boost bereit.

`adecc_test_install(WeatherClient RUNTIME_CLOSURE PACKAGE_ROOTS ... TOOLCHAIN_DLLS ...)` erzeugt das Ziel `buildengine-stage-WeatherClient` und ermittelt die PE-Import-Closure. Die abhängigen DLLs werden neben die ausführbare Datei im projektspezifischen CMake-Runtime-Verzeichnis gestellt und bei `cmake --install` mit der EXE ins Installationsziel übernommen. **Kein komplettes SDK, keine statischen Third-Party-Archive und kein „nächstes“ DLL-Paket via PATH.**

Die beiden Ausgabepfade sind zu unterscheiden:

| Ort | Zweck |
| --- | --- |
| `../build/Test/WeatherClient/Release/cmake/runtime/WeatherClient/` | Build-/Staging-EXE mit den ermittelten DLLs |
| `../build/Test/WeatherClient/Release/` | Installierte EXE samt bereitgestellten DLLs; wird erst durch `cmake --install` erneuert |

Für Debug wird eine eigene `Debug`-Konfiguration verwendet. Die aktuellen Befehle stehen in [build.md](build.md).

## Prüfung der tatsächlichen Runtime-Abhängigkeiten

Nach lokalem Build und Installation:

```bat
cmake --build ../build/Test/WeatherClient/Release/cmake
cmake --install ../build/Test/WeatherClient/Release/cmake
dir ..\build\Test\WeatherClient\Release\*.dll
ctest --test-dir ../build/Test/WeatherClient/Release/cmake --output-on-failure
```

Das `dir` zeigt lediglich das Verzeichnis, **nicht** die vollständige transitive PE-Importkette. Für einen auditierbaren Nachweis sind die BuildEngine-Staging-Protokolle und die PE-Import-Auswertung der konkreten installierten EXE und DLLs maßgeblich. Erst anschließend können pro Laufzeitbibliothek exakte Version, Datei/SHA-256, Lizenztext und Herkunft im Auslieferungs-SBOM verifiziert werden.

## Lizenz- und SBOM-Verantwortung

Die projektinternen Quellen stehen unter [PolyForm Noncommercial 1.0.0](LICENSE.md). Die Lizenzen der Drittbibliotheken werden dadurch **nicht** geändert. Original-Lizenztexte aus installierten BuildEngine-Paketen und mitgelieferte Notices gehen den vereinfachten Tabellen dieser Datei vor. Der CycloneDX-Eintrag [SBOM-WeatherClient.cdx.json](SBOM-WeatherClient.cdx.json) ist derzeit eine **deklarative Stückliste ohne konkrete Paketversionen und ohne verifizierte binäre Dependency-Closure**; er darf nicht als endgültig geprüfter Distribution-SBOM ausgegeben werden.

Die lizenzierten bzw. vertragsgebundenen **externen Webdienste** sind von den Bibliotheken strikt zu trennen. Open-Meteo, Nominatim/OSM, ipapi.co und ipwho.is werden über HTTPS angesprochen, nicht als Drittbibliotheken gelinkt. Wetter-/Kartendaten, öffentliche API-Kontingente und Schriftrechte folgen eigenen Regeln. Siehe [WeatherServices.md](WeatherServices.md) und [REST.md](REST.md).

## Referenzdateien

- [CMakeLists.txt](CMakeLists.txt) – verbindliche Target- und Paket-Definition.
- [../cmake/BuildEngineTests.cmake](../cmake/BuildEngineTests.cmake) – XML-basierte aktive Paketwahl, Boost-Setup und DLL-Staging.
- [SBOM-WeatherClient.cdx.json](SBOM-WeatherClient.cdx.json) und [SBOM.md](SBOM.md) – deklarierte CycloneDX-Komponenten.
- [LICENSE.md](LICENSE.md) – Projektlizenz und Drittbibliotheks-Lizenzen.
- [Architecture.md](Architecture.md) – Ablauf und Abhängigkeitsgrenzen.
- [build.md](build.md) – konkrete Kommandos und Verifikationsschritte.

**Verifikationsstand:** Quell-/CMake-Abgleich von `main`; keine Behauptung über einen bereits ausgeführten BCC64X-Build oder eine vollständig geprüfte DLL-Closure.
