<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – BCC64X Build

Die projektbezogenen Details sind getrennt dokumentiert:
[Architecture.md](Architecture.md) (Ablauf, Zustandsautomat und Worker),
[REST.md](REST.md) (vollständige HTTPS-GET-Anfragen mit `curl.exe`),
[ThirdParty.md](ThirdParty.md) (direkte und transitive BuildEngine-Pakete)
sowie [WeatherServices.md](WeatherServices.md) (Providerbedingungen).
Die hier aufgeführten Kommandos arbeiten mit dem Produktionsroot der
BuildEngine und **installierten dynamischen** Bibliotheken.


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
Switch to DAILY to inspect the reduced six-column forecast: weather
symbol, min/max temperature and precipitation. The selected day's
sunrise and sunset stay in the header, not in every forecast column.
After sunset, hourly forecast icons must show the moon.

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

The prior opt-in `WEATHERCLIENT_USE_SEGOE` has been retired: the original
Segoe UI path is restored unconditionally unless the diagnostic fallback
is explicitly selected.
No external font binaries are distributed. This is a conservative
regression rollback, **not** a claim that the live-fetch problem is fixed.
The live transport and worker recovery are still to be independently
validated against the formerly working version.

## Targeted dial regression rollback (2026-10-25)

The BCC64X graphics CTest runs through the day/night atmosphere and header,
then access-violates **inside the gauges stage** when Segoe UI is enabled.
The last known-good `Dial` source is copied specifically from revision
`69824e9b1734f199a4818ef6ea3398bb6c83712c`, made **after** the
measured Skia Segoe UI font was integrated and **before** the visual
instrument upgrade. All other weather art, forecast, lunar phase and
unit conversions remain unchanged. Only the minimum US inHg precision
(2 decimal places) was added to the original dial body.

This is a targeted rollback of the newer instrument drawing code, **not** a
font-provider or architecture change. The render trace now identifies each
individual dial and the wind rose. The next BCC64X graphics CTest will
verify whether the regression is resolved. If it still crashes, use the
last instrument name in `weather-client-render.log` to target the next
change; do not replace working subsystems on speculation.

## Targeted temperature and severe weather regression

The temperature dial displays the current value *after* its needle
and highlights using the same anti-aliased `font::Draw` path and
transparent background as the barometer. The temporary dark plate
and forced 5x7 pixel glyphs are removed. Both Celsius and Fahrenheit
continue to use the existing unit system.

The deterministic offline model CTest now also checks:
- benign conditions -> LOW model signal
- 17 mm forecast in one hour -> ELEVATED heavy-rain signal
- WMO 99 -> HIGH hail/thunderstorm signal
- 110 km/h forecast gust -> HIGH gust signal
- empty forecast -> UNKNOWN, rather than falsely indicating LOW

No new build targets are required. Standard CTest invocation remains:
`ctest --test-dir ../build/Test/WeatherClient/Release/cmake --output-on-failure`.

A successful offline test does not substitute for validating the actual
weather worker/HTTPS path against the installed Windows DLL runtime.

## Quarter-hour refresh and extended weather information (2026-10-25)

Weather refresh now targets wall-clock quarter-hour boundaries at **:00,
:15, :30 and :45**, not a 15-minute delay after the previous HTTP response.
A failed request is retried after one minute, without losing an earlier
quarter-hour boundary. The offline self-test checks the boundary arithmetic,
rollover, retry timing and analog clock-hand calculation; it does not perform
live HTTPS or replace a BCC64X GUI validation.

The upper-right analog clock displays the **system-local time**, marked
`LOCAL TIME`, independently of the weather location's timezone. The
procedural sun/moon/weather artwork is shifted left. The interface is now
named **WEATHER DASHBOARD**, because the application reads provider data
instead of measuring its own weather observations.

Additional Open-Meteo request fields `current.dew_point_2m`,
`hourly.dew_point_2m`, `current.uv_index` and `hourly.uv_index` are
parsed as optional quantities. The current dew point is formatted in the
selected temperature unit; UV remains dimensionless with LOW, MODERATE,
HIGH, VERY HIGH and EXTREME categories. Existing daily `uv_index_max`
remains available in the model. Wind-origin degrees are preserved; arrows
show the *travel* direction, i.e. a wind FROM W travels TO E. The
METRIC/US control uses a smaller font and maintains its original hit area.

To verify after fetching this branch, run the existing independent build
and CTest commands listed above. In addition, open the installed GUI in
`--demo` and check the clock's separation from the weather illustration,
the dew/UV labels, weather direction, dial readability and the METRIC button.

## Temperature graphics regression isolation (2026-10-25)

The extra temperature-only `font::Draw(..., maxWidth)` call was removed
because it introduced an additional `SkFont::measureText` / `Fit` path
not used by the barometer readout. Temperature, pressure and humidity now
use the same transparent `view::Text` rendering call in `Dial`.
The dark plate and pixel glyphs remain removed. Render tracing now surrounds
the temperature-value draw with `Dial: temperature value begin` and
`Dial: temperature value complete`. This is a targeted regression isolation,
**not a verified fix** until the Windows BCC64X graphics test passes.

To locate a remaining native crash in the CTest working directory:

```bat
set WEATHERCLIENT_TRACE_RENDER=1
ctest --test-dir ../build/Test/WeatherClient/Release/cmake -R WeatherClientGraphics --output-on-failure
 type ..\build\Test\WeatherClient\Release\cmake\weather-client-render.log
```

If the trace stops before `Dial: temperature value complete`, investigate
Skia's font provider with a native debugger; otherwise follow the next
`Render: ...` phase. A test passing with the fallback environment variable
is diagnostic and does not establish that the default Segoe UI path is safe.

## Centered instrument values and units (2026-10-25)

Temperature, barometer and humidity now use a shared two-line dial readout:
the numeric value is horizontally centered, and the selected unit is
centered immediately below it. The current formatter keeps one fractional
digit except for barometric pressure in inHg, which retains two. A missing
value shows `NO DATA` for the temperature or `--` for the other gauges;
the unit line remains visible.

The text placement uses an approximate font-dependent glyph advance,
**not** `SkFont::measureText` or `font::Fit`, which were implicated in
earlier BCC64X/Skia graphics regressions. For temperatures the degree sign
is an antialiased Skia circle and C/F is drawn as a separate letter.
The readout is transparent and drawn last, above dial effects.

`WeatherClientGraphics` now checks that the current temperature value
and its lower unit band contain visible pixels in the offline preview.
A successful pixel test does not fully validate the visual center
alignment for every installed font, locale or numeric range. A Windows
BCC64X/Skia CTest and visual review remain necessary.
