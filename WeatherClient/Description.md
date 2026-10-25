<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – Skia Weather Dashboard

[TOC]

## Technical Documentation and Navigation

WeatherClient is a **weather dashboard**, not a measurement observatory. The following pages document implemented behavior and distinguish source-code facts from runtime checks that have not been performed locally:

- [Architecture.md](Architecture.md): startup, network worker, Boost.Statechart FSM, scheduling, model, and display
- [REST.md](REST.md): all four HTTPS services, actual GET parameters, JSON structures, and reproducible `curl.exe` tests
- [ThirdParty.md](ThirdParty.md): direct libraries, BuildEngine packages, DLL staging, dependency verification, and licenses
- [build.md](build.md): BCC64X/CMake/Ninja, installation, and tests
- [WeatherServices.md](WeatherServices.md): provider terms, attribution, and privacy
- [SBOM.md](SBOM.md), [LICENSE.md](LICENSE.md): software bill of materials and license texts

## Calculated Moon phase and nighttime forecast

The night illustration no longer always uses the same crescent. The
new pure C++23 header [WeatherMoon.h](WeatherMoon.h) calculates the
**lunar cycle** from an ISO 8601 calendar date and time:

- Reference new moon: **2000-01-06 18:14 UTC**
- Mean synodic lunar month: **29.530588853 days**
- Cycle fraction: **0.0** at new moon, **0.5** at full moon
- Illuminated fraction: **(1 − cos(2π × cycle))/2**
- Eight stages: new, waxing crescent, first quarter, waxing gibbous,
  full, waning gibbous, last quarter and waning crescent

Skia paints a dark lunar disc and its illuminated terminator directly
with **original geometry**. Waxing phases illuminate the right half,
waning phases the left (conventional Northern Hemisphere representation).
Each nightly *hourly forecast* pictogram calculates the phase from its
own hour, and the current nighttime header uses the weather observation
time. The header displays the stage name and approximate illumination.

**Accuracy boundary:** This is a mean-synodic approximation, not a
high-precision ephemeris. Open-Meteo's timestamps are location-local;
they do not carry the exact numeric UTC offset in this API model, so
the local time is currently treated as an approximate UTC instant.
Depending on timezone and lunar perturbations the displayed stage
can be off by several hours or near a stage boundary. It is for
educational visualization only, not navigation or astronomy research.
No lunar API, artwork asset, telemetry or new third-party license
is needed.

The offline model test checks the reference new moon, an approximate
full moon, representative waxing and waning dates and invalid
calendar dates.

## Night sky, distinct instruments and US units

The Skia weather artwork is entirely procedural and original.
At night a constructed crescent **moon** replaces the daytime sun;
clouds, rain, snow and thunderstorm layers appear over either sky.
Every hourly forecast symbol chooses sun or moon using that date's
local sunrise and sunset. Daily forecast columns retain the weather
symbol, temperature range and precipitation; sunrise/sunset times
are displayed once in the header for the selected day.

The analog instruments intentionally have different surface treatments:
a warm enamel thermometer, a machined blue barometer with a secondary
precision ring and a translucent jade hygrometer. Each has a centered
heading and measured centered numeric readout. The wind rose is a
separate compass, rather than a fourth copy of the main dial.

Press **U**, or click **METRIC / US** at the bottom right, to change
the **entire display** without contacting Open-Meteo again:

| Measurement | Metric | US |
| --- | --- | --- |
| Air and feels-like temperature, hourly/daily forecast | Celsius (°C) | Fahrenheit (°F) |
| Temperature change described in English | °C difference | °F difference (no +32 offset) |
| Wind speed and gusts | km/h | mph |
| Daily precipitation and current rainfall | mm | inches (in) |
| Sea-level pressure / barometer | hPa | **inHg** |
| Humidity, cloudiness, precipitation *probability*, direction | %, degrees | unchanged |

The conversion functions live in
[WeatherUnits.h](WeatherUnits.h), independent from SDL, Skia,
HTTP and Boost.Statechart. Model data is **always stored in the
metric units requested from Open-Meteo**. All conversions are made
at presentation time, including the forecast chart's numerical
values and geometry; the view change does **not** trigger a network
request or a new FSM transition. US inches are shown with two
decimal places, inHg with two decimal places.

The chosen display system is saved as `"units": "metric"` or
`"units": "us"` in `weather-location.json`, alongside the
user-selected address if available. A preferences-only JSON file
is valid before the user enters an address; in that case the
normal first-run IP lookup still occurs. No IP-derived address
is stored merely to preserve the units.

The headless model test covers 0°C→32°F, 100°C→212°F,
100 km/h→62.137 mph, 25.4 mm→1 in and 1013.25 hPa→29.92 inHg,
plus US narrative strings and SDL2 button hit-testing.

## Extended Weather Dashboard (2026-10-25)

The GUI can use the **Segoe UI family** from the repository's existing
PDFium SDL2 example through Skia's font manager. It does **not** copy or ship
Microsoft font files; original `WeatherText.h` vector lettering is used if
the OS cannot supply a typeface. All text is measured, truncated by physical
width and restricted to its panel instead of truncating arbitrary bytes.

The upper-left text zone is reserved for title, address, English weather
brief, daily temperature low/high and the selected day's local
sunrise/sunset. The upper-right area is reserved for original Skia sky
illustrations. Weather codes combine sunlight or a nighttime sky with clouds,
rain, snow, fog or lightning. Both the global header and hourly weather
symbols use the correct day/night state where data permits.

The forecast draws hourly or daily values in non-overlapping bands
(time -> condition icon -> temperature -> curve -> rain).
In daily mode, arrow navigation changes which day's sunrise, sunset and
minimum/maximum temperatures appear in the header. The additional label
`STORM FORECAST` means a weather-code thunderstorm signal; `ELEVATED
CAPE` is an **instability indication, not a quantified storm probability**.
Missing readings are explicitly reported as unavailable.

The bottom line reads **LIVE WEATHER HH:MM:SS / UPDATED HH:MM:SS**.
The first clock uses the system's local wall clock, and the second is
recorded **only after successful weather retrieval**, not on a timer tick
or failed request. The station timezone remains separately visible. The
15-minute refresh and 1-minute retry are driven by Boost.Statechart events
and the steady-clock timer, while HTTPS work stays off the SDL thread.

See **[WeatherServices.md](WeatherServices.md)** for the precise endpoints,
rate limits, permission requirements, attribution, caveats and licensing.
The public Open-Meteo endpoint is for non-commercial use; a customer license
or approved alternative is required for commercial operation.

## Project Concept and Reused Ideas

SDL2 handles windows, the keyboard, and address input. Skia renders analog temperature, pressure, and humidity instruments, a wind rose, weather graphics, and forecasts. The UI scales with the window. **Boost.Statechart** models the `Off → Resolving → Fetching → Ready` and `Error` states; HTTPS requests run in a `std::jthread` and their results are consumed by the SDL main thread. Successful requests are scheduled on absolute quarter-hour boundaries; failures retry after approximately one minute.

The supplied older archives contributed the Open-Meteo data model, hourly/daily forecasting concepts, and state-machine approach. They also contain CORBA interfaces and `BoostTools` dependencies not required by the new standalone example. The original archives remain unchanged and are not reclassified under a new license.

## Data Sources

| Purpose | Provider | Interface |
| --- | --- | --- |
| Weather | [Open-Meteo](https://open-meteo.com/en/docs) | HTTPS/JSON; current, hourly, daily, automatic time zone |
| Address geocoding | [Nominatim / OpenStreetMap](https://operations.osmfoundation.org/policies/nominatim/) | User-initiated search; identifiable User-Agent; request-rate limits |
| Initial IP location | [ipapi.co](https://ipapi.co/api/) | Approximate city/region, not a street address |
| IP fallback | [ipwho.is](https://ipwhois.io/documentation) | Only if the primary IP provider fails |

No exact street address is inferred from an IP. Neither documentation generation nor building the project triggers geolocation. `--demo` runs without external IP or weather requests. Public endpoints have independent rate limits, privacy considerations, and terms; see [REST.md](REST.md) and [WeatherServices.md](WeatherServices.md).

## Skia fonts: reliable portable lettering

The original screenshot showed **no text at all** while Skia geometry
rendered. The default-constructed `SkFont` was not resolving usable glyphs
on the current BCC64X/Skia configuration. The GUI now uses original,
deterministic 5x7 pixel-vector glyphs in [WeatherText.h](WeatherText.h),
drawn directly with Skia primitives, with no platform font manager or
external font asset. The small alphabet is intentionally an ASCII fallback:
non-ASCII characters render as a visible placeholder rather than silently
disappearing. It is meant to guarantee readability, not to replace a proper
Unicode-capable typeface system in a future release.

The photographed light-ivory gauge faces are from the **older renderer**.
The current WeatherArtwork version has dark dial faces, rim gradients,
a graphical sky and a wind rose. Verify that the freshly built executable
is the same path that is launched.

## Visual Redesign: Original Skia Weather Dashboard

The display is now an English-language, international demonstration of Skia
rather than a collection of basic gauges. All new illustrations are generated
at runtime from **original paths and paint operations**: no logos, stock
weather icons, photos, proprietary typefaces or third-party artworks are used.

The weather instruments use multi-stop metallic gradients, separate
illuminated rims, sweeping dial arcs, fine scales, analog needle shadows and
animated specular highlights. A procedural sky varies with weather code and
day/night data (sun/moon, stylized cloud shapes, precipitation and contour
lines). A custom compass rose visualizes wind direction.

The header displays an automatically generated English
**current-weather summary** and **next-six-hours forecast**, including
perceived temperature, humidity, wind, rain probability and temperature trend.
These sentences come from [WeatherNarrative.h](WeatherNarrative.h) using
Open-Meteo observations; they are not predictions invented by the UI.
A blank measurement remains explicitly unavailable rather than becoming
a fabricated sensor value. Hourly and daily forecast graphics, buttons,
address entry and the Statechart FSM are preserved.

| File | Responsibility |
| --- | --- |
| [WeatherArtwork.h](WeatherArtwork.h) | Pure Skia gradients, arcs, original sky illustration, dials and compass |
| [WeatherNarrative.h](WeatherNarrative.h) | Pure C++23 weather summary and forecast description |
| [WeatherView.h](WeatherView.h) | Forecast graphs, buttons, text rendering and pointer hit-testing |
| [WeatherClient.cpp](WeatherClient.cpp) | SDL2 window and events, presentation composition, FSM and asynchronous requests |

Only SDL2 and Skia supply the GUI; there are no direct Win32 UI calls.
The artwork uses Skia raster surfaces for portability; the application still
needs an actual SDL GUI test on each platform.

## Portable GUI and Forecast Charts

[WeatherView.h](WeatherView.h) draws Skia controls, hourly temperature curves, daily minima/maxima, precipitation bars, and weather symbols without Win32 window APIs, networking, or direct FSM-state access. SDL2 supplies mouse, keyboard, UTF-8 text entry, and fullscreen behavior. The scale-independent hit test is covered by offline tests.

The three instruments show centered numeric values and units on separate lines. The wind panel combines the direction rose, origin/destination labels, speed, and gusts without a duplicate miniature compass.

## SDL2 Controls

| Input | Action |
| --- | --- |
| Left click | Activate a visible button |
| `F` / `F11` | Toggle fullscreen |
| Mouse wheel | Navigate forecast entries |
| `A` | Enter an address, geocode through Nominatim, and save on success |
| `I` | Request approximate IP location and switch to live data |
| `R` | Refresh current weather |
| `H` / `D` | Select hourly/daily forecast |
| `←` / `→` | Previous/next forecast entries |
| `Enter` | Confirm address input |
| `Backspace` / `Esc` | Edit or cancel/exit |

At startup, the client loads a previously saved valid address if available; otherwise, it resolves an approximate IP location before retrieving weather. A successfully geocoded user address is saved as a local JSON file, including coordinates and the `approximate` marker. The path is relative to the **working directory**, not a hardcoded user profile. Use `--config <path>` to change it.

## Reproducible Tests

`--demo` supplies offline synthetic weather with 48 hourly and seven daily observations. `--self-test` checks deterministic models, navigation, SDL hit testing, and UTF-8 backspace without an SDL window. `--graphics-test` exercises the initialized SDL/Skia renderer; `--fsm-test` tests state transitions. Live HTTPS and UI visual checks are separate verification activities.

## Project Files and Learning Goals

- [WeatherModel.h](WeatherModel.h): weather data, JSON parsing, and validation.
- [WeatherNetwork.h](WeatherNetwork.h): libcurl HTTPS, geocoding, IP lookup, and local settings.
- [WeatherClient.cpp](WeatherClient.cpp): SDL2 application, rendering, FSM, and worker.
- [build.md](build.md): BCC64X installation, dynamic libraries, and tests.
- [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md): original license and declared dependencies.

**Verification:** Source documentation does not imply that the latest BCC64X GUI or live HTTPS regression tests have passed.

## Live services and SSL trust

The Windows BCC64X build of libcurl uses OpenSSL. Without a configured
certificate authority, HTTPS requests failed before reaching any provider.
The client now enables libcurl's **native CA trust-store option** on Windows,
keeping peer and hostname verification enabled. A PEM bundle can also be
supplied through `--ca-bundle <absolute-path>` or
`WEATHERCLIENT_CA_BUNDLE`, `CURL_CA_BUNDLE` / `SSL_CERT_FILE`.
Never disable `CURLOPT_SSL_VERIFYPEER` or `CURLOPT_SSL_VERIFYHOST`.
If an organization intercepts TLS, its trusted CA must be installed in the
system root store or supplied explicitly as a PEM file.

IP location uses ipapi.co first and ipwho.is as a fallback; both estimate
city/region rather than a postal address. Use ADDRESS for Nominatim geocoding.
Address and IP search **leave demo mode** and fetch real Open-Meteo data.
The saved address and its coordinates are restored at normal startup.
Current observations include apparent temperature, gusts, clouds and wind;
forecast parsing includes rain probability, UV, sunrise/sunset and daily
wind gusts where supplied. The GUI remains in English; error messages
are shown without replacing previously displayed weather data.

Network services may be unavailable, rate limited or require separate
licenses for production. Successful online use must be checked on the
actual Windows installation and is not asserted by the offline CTest.

## Failure recovery and test separation

The original two-test setup reported `WeatherClientOfflineModel` as
`SEGFAULT` while `WeatherClientStatechart` passed. The offline test
combined Skia/DirectWrite graphics with data checks before SDL2
initialization. The three separated CTests identify whether failures
belong to data/model logic, the initialized Skia raster path or
Boost.Statechart. DirectWrite typefaces are released before SDL shuts
down. The live GUI now reports C++ worker/FSM/render errors without
intentionally terminating; fatal native access violations still require
a native stack trace. The entire weather fetch path remains HTTPS with
peer/hostname verification.

## Active runtime diagnostics

`--weather-probe <latitude> <longitude>` performs a real HTTPS/Open-Meteo
request without opening an SDL window, recording success or detailed
failure in `weather-client-probe.log` and returning a nonzero exit code
on failure. `WEATHERCLIENT_TRACE_RENDER=1` adds flushed per-stage GUI
trace entries in `weather-client-render.log`. The optional
`WEATHERCLIENT_FONT_FALLBACK=1` bypasses the Skia DirectWrite font manager
and renders using the original vector glyphs. This helps distinguish
font-manager crashes from other Skia drawing faults. They are diagnostic
options, not a replacement for a fully reliable GUI or worker lifecycle.

## Strong-rain and severe-weather outlook (model-based)

The station now evaluates the next **48 hours of available hourly forecast
data**, not just the six-hour narrative, through the self-contained
[WeatherHazards.h](WeatherHazards.h). The strongest indicator is shown in
a **48H MODEL / LOW | WATCH | ELEVATED | HIGH** label above the forecast;
details and the triggering forecast time are displayed at the bottom
of the screen when there is no more important network error.

Inputs: hourly precipitation amount (mm/hour), complete rolling
six-hour precipitation amounts (mm/six hours), WMO weather codes,
convective available potential energy (CAPE) and forecast wind gusts.
Missing fields are never silently interpreted as benign. The
implementation selects the strongest signal and retains its time; it
never derives a numeric thunderstorm probability from precipitation
probability. High and elevated signals include examples such as
heavy rainfall, thunderstorms with hail and extreme gusts.
The **model** label and text **NOT AN OFFICIAL WARNING** are intentional:
these are risk indicators based on forecast model data, not official
DWD/NINA/Meteoalarm bulletins.

Thresholds are documented in [WeatherServices.md](WeatherServices.md).
This does not introduce a new weather service, third-party dependency,
non-commercial-use exception or additional HTTP connection.

The temperature dial displays its current reading in the same
anti-aliased system font, transparent background and color treatment
as the other instruments, over the dial needle and highlights.
The temporary dark readout plate and hardwired 5x7 pixel digits
were removed. The instrument geometry and font-provider selection
remain unchanged.
