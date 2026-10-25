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

Zum Testen des Datensatzes und der plattformneutralen SDL-/Skia-Steuerlogik `--self-test` verwenden. Die Schaltflächen lassen sich anklicken; `F`/`F11` schaltet Vollbild. Für Mausbedienung mit hoher Pixeldichte wird die Fenstergröße von SDL2 verwendet.
Die reale Standortsuche benötigt HTTPS, gültige CA-Zertifikate sowie
Netzzugriff auf die konfigurierten Anbieter. Die Anwendung erzeugt
`weather-location.json` nur nach Eingabe einer Adresse; für
reproduzierbare Tests den Demo-Modus verwenden.

Der bisherige `WeatherAPIServer` nutzt CORBA/TAO und ältere
`BoostTools` und wird in diesem SDL2-Programm **nicht benötigt**.
Die gelieferten Originaldateien wurden nicht verändert.

[Beschreibung](Description.md) · [Lizenzen](LICENSE.md) · [SBOM](SBOM.md)

## Visual quality and portability checks

The new Skia effects are procedural. Typography uses the locally installed
Segoe UI font if available (as in the PDFium/SDL2 example); no font file is
copied, bundled or distributed. The original vector glyphs remain a fallback. Inspect the animated sky, three graded instrument faces,
compass rose, forecast curve and the English summary in GUI demo mode. Use
`ctest --test-dir ../build/Test/WeatherClient/Release/cmake --output-on-failure`
to check the deterministic English narrative and control/forecast model.
This does **not** substitute for visually checking raster output in SDL2.
No package versions or Windows-specific GUI APIs were added.

## HTTPS and live location

```bat
..\build\Test\WeatherClient\Release\WeatherClient.exe
..\build\Test\WeatherClient\Release\WeatherClient.exe --ca-bundle C:\path\to\trusted-ca-bundle.pem
```

The first command uses the Windows trusted CA roots through libcurl/OpenSSL;
on other systems curl uses its configured trust store. CA errors are not
suppressed. Add the organization's CA to the trust store or use an authorized
PEM bundle if a TLS inspection proxy is present. IP geolocation is approximate;
click ADDRESS or press A to provide a full address. Typing a new address also
leaves --demo mode and downloads live weather. The `--demo` option itself
always starts with synthetic offline observations.

## Updated demonstration and FSM test commands

The build target's EXE is staged under
`../build/Test/WeatherClient/Release/cmake/runtime/WeatherClient/WeatherClient.exe`.
The **installed EXE** is `../build/Test/WeatherClient/Release/WeatherClient.exe`.
Do not confuse them: `cmake --build` does **not** update the installed EXE
until `cmake --install` runs.

```bat
cmake --build ../build/Test/WeatherClient/Release/cmake
ctest --test-dir ../build/Test/WeatherClient/Release/cmake --output-on-failure
cmake --install ../build/Test/WeatherClient/Release/cmake
..\build\Test\WeatherClient\Release\WeatherClient.exe
```

Two tests are expected: `WeatherClientOfflineModel` validates parsers,
GUI glyph pixels, daylight, sunshine times, thunderstorm indicators and
forecast navigation. `WeatherClientStatechart` tests the actual
Boost.Statechart state transitions `Off -> Resolving -> Fetching -> Ready
-> Fetching -> Error -> Resolving -> Fetching -> Ready`.
The latter does not sleep for 15 minutes or simulate external HTTPS failures,
so real timer intervals and transport recovery still require integration tests.

On a normal launch, IP geolocation is approximate if no user-selected
address has been stored. After typing an address in the GUI, the resolved
postal location is saved to `weather-location.json` and is loaded next time.
The service contracts and attribution are recorded in
[WeatherServices.md](WeatherServices.md).
