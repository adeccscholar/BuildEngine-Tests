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

## Unit toggle / graphics verification (metric and US)

`U` or the lower-right **METRIC/US** control switches the view
immediately. Weather values from Open-Meteo stay in SI/metric
form; no API parameter changes and no extra HTTPS request are
necessary. The choice persists in the user's local
`weather-location.json` even if no address is stored yet.
Start `--demo`, switch units repeatedly and verify temperature,
wind, gusts, rain, pressure **hPa ↔ inHg**, daily min/max and
narrative values all change together. The forecast graph must
retain its shape because Fahrenheit conversion is affine.
Switch to DAILY to inspect sunrise and sunset pictograms;
after sunset, hourly forecast icons must show the moon.

`WeatherClientOfflineModel` now tests both unit systems and
the US selector's SDL hit geometry. No new third-party
packages, fonts, proprietary weather graphics or Windows APIs
are introduced by this feature.

## Lunar visualization and compile verification

The `WeatherMoon.h` calculation and both Skia moon renderers use only
the existing C++23 and Skia DLL dependencies. They neither download
ephemeris data nor require an additional library. New/full/waxing/waning
assertions are part of `WeatherClientOfflineModel`.

The immediately preceding US-unit update introduced a signature
mismatch: `WeatherView::DrawForecast` used `units` but still accepted
only four parameters. This was corrected to take the fifth,
defaulted `UnitSystem units` parameter. Test that BCC64X compiles
again before validating the different moon phases visually.

In demo mode, the fixed demonstration date yields a fixed moon phase.
Live mode uses the current Open-Meteo observation and forecast
timestamps, which are provided in the location's local time.

## Failure isolation: offline SIGSEGV and live network errors

The previous `WeatherClientOfflineModel` combined value assertions with
a full 1200x780 Skia render **without initializing SDL**. The combined test
could terminate with an access violation without identifying the affected
subsystem. This has been separated:

- `WeatherClientOfflineModel` / `--self-test`: deterministic weather,
  moon phase, conversions, Unicode-editing, input hit tests and FSM checks;
  no Skia surface or OS font initialization
- `WeatherClientGraphics` / `--graphics-test`: initializes SDL2, draws
  original Skia fallback glyphs plus the dashboard, then releases the
  cached DirectWrite typeface **before** `SDL_Quit()`
- `WeatherClientStatechart` / `--fsm-test`: direct Boost.Statechart
  transition validation, no SDL or network

Because the CTest registry now includes a third test, **reconfigure once**
before rebuilding:

```bat
cmake -S WeatherClient -B ../build/Test/WeatherClient/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/WeatherClient/Release
cmake --build ../build/Test/WeatherClient/Release/cmake
ctest --test-dir ../build/Test/WeatherClient/Release/cmake --output-on-failure
```

A failed HTTPS request is captured by the worker as a data error and shown
inside the GUI; an unexpected C++ exception from completion processing or
rendering is now caught on the SDL thread instead of propagating out of
the main loop. A failed request schedules retry after a minute while
preserving the last successful data. Native crashes (access violations)
cannot be recovered by C++ exception handling; a graphics-test crash
still requires a native stack trace or crash dump to pinpoint the faulty
Skia/DirectWrite operation. Do not hide such crashes by disabling TLS
verification or excluding the graphic test.

## Executable diagnostics (enabled in repository)

The diagnostic flags are now implemented in the actual WeatherClient.
It is a **WIN32 GUI-subsystem executable**; `--weather-probe` writes a
flushed result file instead of relying on attached console output:

```bat
..\\build\\Test\\WeatherClient\\Release\\WeatherClient.exe --weather-probe 52.52 13.405
type weather-client-probe.log

set WEATHERCLIENT_TRACE_RENDER=1
set WEATHERCLIENT_FONT_FALLBACK=1
ctest --test-dir ../build/Test/WeatherClient/Release/cmake -R WeatherClientGraphics --output-on-failure
type ..\\build\\Test\\WeatherClient\\Release\\cmake\\weather-client-render.log
```

`WEATHERCLIENT_FONT_FALLBACK=1` now bypasses DirectWrite/Segoe UI and
forces the first-party Skia-vector glyphs, allowing isolation of font
manager faults. `WEATHERCLIENT_TRACE_RENDER=1` flushes named drawing
phases to a persistent file before each major scene component. A native
segmentation fault can still occur, but the last logged phase identifies
the part to inspect with a debugger. To return to Segoe UI run
`set WEATHERCLIENT_FONT_FALLBACK=` and repeat the graphics test.

The model test prints each failed condition separately as
`WEATHER_CHECK_FAIL: model condition N` rather than one silent failure.
The model-only test no longer initializes DirectWrite when checking text
fitting; that verification belongs to the graphics test.

These instruments **do not** prove that live weather retrieval or the
full worker/retry lifecycle is functioning. A transition-only FSM PASS is
not a live transport integration PASS; a failing weather probe or a crash
in the graphics path must be treated independently.

Note: CTest launches its executable with the CMake build directory as
working directory. Thus the render log created by the `ctest` command is
under `../build/Test/WeatherClient/Release/cmake/weather-client-render.log`.
A manually launched probe writes `weather-client-probe.log` in the
shell's current directory.

## Regression stabilization (2026-10-25)

The 2026-10-25 CTest evidence with `WEATHERCLIENT_FONT_FALLBACK=1`
confirmed that **all rendering stages complete**, including the full
forecast and controls. Both non-FSM tests failed only on exact
formatted-string assertions (conditions 12/13), not on a graphics
fault. The model test now checks underlying temperatures separately
from the displayed unit labels; it does not assume a byte-for-byte
localized formatting result.

The first-party raster lettering is temporarily **the default**, because
it is the rendering path confirmed to complete. The Segoe UI/DirectWrite
path introduced after the functioning version must be separately verified:

```bat
set WEATHERCLIENT_USE_SEGOE=1
set WEATHERCLIENT_FONT_FALLBACK=
ctest --test-dir ../build/Test/WeatherClient/Release/cmake -R WeatherClientGraphics --output-on-failure
```

Without `WEATHERCLIENT_USE_SEGOE=1`, the first-party lettering is used.
No external font binaries are distributed. This is a conservative
regression rollback, **not** a claim that the live-fetch problem is fixed.
The live transport and worker recovery are still to be independently
validated against the formerly working version.

## Segoe UI text rasterizer (GDI, 2026-10-25)

The earlier fallback to the custom 5x7 alphabet passed all three CTests
but produced visibly poor typography. WeatherClient now uses the same
Windows GDI Segoe UI mechanism already used by
`pdfium/SDL2/ToolbarGraphics.h` (`CreateFontW`, `TextOutW`, GDI
surface), **without using Skia's DirectWrite font manager**. GDI text is
rasterized off-screen in gray-scale, converted into premultiplied tinted
Skia pixels and composited onto the existing SkCanvas. The same font's
`GetTextExtentPoint32W` measurements govern clipping, labels and centering.
Text rasters are cached with a bounded cache. No external font file is
included. On non-Windows platforms or GDI failure, the original 5x7
lettering remains the fallback. The force-fallback flag remains available:

```bat
set WEATHERCLIENT_FONT_FALLBACK=1
ctest --test-dir ../build/Test/WeatherClient/Release/cmake -R WeatherClientGraphics --output-on-failure
set WEATHERCLIENT_FONT_FALLBACK=
```

The **unverified** legacy Skia/DirectWrite experiment can still be selected
with `WEATHERCLIENT_USE_SEGOE=1`, but it is no longer the normal GUI
text renderer. Because this font change uses the platform GDI font exactly
as the existing PDFium example does, the application becomes Windows-
specific for high-quality typography. The C++ model, FSM and weather
network code remain unchanged. The final BCC64X build and visual check
must still be performed on the user's system.
