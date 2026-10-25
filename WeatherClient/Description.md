<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – analoge Skia-Wetterstation

[TOC]


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
local sunrise and sunset. Daily forecast columns show original
sunrise/sunset pictograms with the local times.

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

## Extended weather observatory (2026-10-25)

The GUI now uses the **Segoe UI family** from the repository's existing
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

## Idee und übernommene Bestandteile

SDL2 behandelt Fenster, Tastatur und Adresseingabe. Skia rendert vier
analog wirkende Instrumente für Temperatur, Luftdruck, Feuchtigkeit und Wind
sowie eine Text-/Vorhersagefläche. Die Anzeige skaliert mit dem Fenster.
**Boost.Statechart** bildet die FSM mit den Zuständen
`Off → Resolving → Fetching → Ready` und `Error`.
HTTP-Aufgaben laufen in einem `std::jthread`; Ergebnisse werden auf dem
SDL-Hauptthread konsumiert. Erfolgreiche Wetterabrufe sind nach 15 Minuten,
Fehler nach einer Minute erneut fällig.

Aus den vier gelieferten Archiven wurden das Open-Meteo-Datenmodell,
die Stunden-/Tagesvorschau, die Zeitsteuerung und die Statechart-Idee
übernommen. Die alten Beispielprojekte verwenden jedoch weitere,
nicht im BuildEngine-Teststack vorhandene `BoostTools`- und CORBA-
Schnittstellen. Um diese *nicht* umzuschreiben, stellt WeatherClient
die nötigen Adapter in neuen Dateien separat bereit.
Die Originalarchive bleiben unverändert beim Benutzer; sie werden
nicht unter neuer Lizenz als eigene Quellen ausgegeben.

## Datenquellen

| Zweck | Quelle | API / Besonderheiten |
| --- | --- | --- |
| Wetter | [Open-Meteo](https://open-meteo.com/en/docs) | Current, hourly, daily; HTTPS/JSON; Ort + automatische IANA-Zeitzone |
| Adresse → GPS | [Nominatim / OpenStreetMap](https://operations.osmfoundation.org/policies/nominatim/) | vom Benutzer ausgelöste Suchanfrage; maximal eine Anfrage/Sekunde, eindeutige User-Agent-Kennung |
| Erste ungefähre IP-Ortung | [ipapi.co](https://ipapi.co/api/) | *nur* ungefähre Stadt/Region; keine Hausanschrift |

Es wird **keine genaue IP-Hausadresse** behauptet. Keine Ortung erfolgt
allein beim Erstellen von Dokumenten oder beim Build. Bei Offline-Test
`--demo` wird weder IP noch Wetterdienst abgefragt.
Nominatim verlangt OSM-Attribution, Nutzungsbedingungen und verantwortliche
Ratenbegrenzung; der Beispielclient sollte nicht massenhaft Adressen
auflösen. Dienste können Rate Limits, Netzwerk- und Datenschutzvorgaben haben.

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

## Visual redesign: original Skia weather observatory

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

## Portable GUI und Diagramme

Der eigenständige Header [WeatherView.h](WeatherView.h) zeichnet
Skia-Bedienelemente, stündliche Temperaturkurven, Tageshöchst- und
Tiefsttemperaturen, Regenbalken, Wetterpiktogramme und einen
Windrichtungskompass. Seine Funktionen benötigen weder Win32 noch
Netzwerk und greifen nicht auf FSM-Zustände zu. SDL2 liefert Maus-,
Tastatur- und UTF-8-Texteingaben sowie den Vollbildmodus.
Der skaleninvariante Hit-Test wird im Offline-Self-Test geprüft.
Das vorhandene Layout der vier Analoginstrumente und die
Boost.Statechart-FSM wurden beibehalten.

## SDL2-Steuerung

| Taste | Aktion |
| --- | --- |
| Linksklick | Schaltflächen direkt mit SDL2-Maussteuerung bedienen |
| `F` / `F11` | zwischen Fenster und Vollbild umschalten |
| Mausrad | Vorschau zurück/vorwärts |
| `A` | Adresse eingeben und mittels Nominatim auflösen; bei Erfolg in `weather-location.json` speichern |
| `I` | neue grobe IP-Ortung (bei Demo deaktiviert) |
| `R` | erneuter Wetterabruf |
| `H`, `D` | Stunden- oder Tagesvorschau |
| `←`, `→` | Vorschau verschieben |
| `Enter` | Adresse übernehmen |
| `Backspace`, `Esc` | Text bearbeiten bzw. Eingabe/Fenster verlassen |

Der erste reguläre Start lädt eine gespeicherte Adresse. Ist keine
vorhanden, beginnt die FSM mit der ungefähren IP-Ortung; erst danach
werden Wetterdaten geladen. Eingabe und Speicherung erfolgen nach
erfolgreicher Geokodierung in einer lokalen JSON-Datei. Das Programm
verwendet das **Arbeitsverzeichnis**, nicht ein hartkodiertes Benutzerprofil.
Für einen anderen Dateipfad kann `--config <pfad>` angegeben werden.
Auch die GPS-Koordinaten und die Herkunftsart `approximate` werden
persistiert; das Zeitzonenfeld stammt aus dem Wetterdienst.

## Reproduzierbarer Test

`--demo` liefert ohne Netz Testwetter mit 48 Stunden und sieben Tagen.
`--self-test` kontrolliert diesen Beispieldatensatz, Schaltflächen-Hit-Tests, die Vorhersage-Navigation und UTF-8-Backspace ohne SDL-Fenster.
Die Visualisierung und netzabhängige Live-Integration sind separate
manuelle Prüfungen. Ein externer HTTP-Dienst ist für den Offline-Test
nicht nötig. Keine reale Ortsadresse wird im Quellcode gespeichert.

## Dateien und Bildungsziel

- [WeatherModel.h](WeatherModel.h): Wetterdaten, JSON und Prüfung.
- [WeatherNetwork.h](WeatherNetwork.h): curl HTTPS, Geocoding,
  IP-Näherung und Konfigurationspersistenz.
- [WeatherClient.cpp](WeatherClient.cpp): Skia-Uhren, SDL2-Steuerung,
  Boost.Statechart-FSM und Ereignisschleife.
- [build.md](build.md): BCC64X BuildEngine-Pakete, DLL und Laufzeit.
- [LICENSE.md](LICENSE.md), [SBOM.md](SBOM.md): Quelllizenz und
  deklarative Komponenten.

**Status:** The previous WeatherClient iteration was confirmed to compile and display live Open-Meteo results on the user's BCC64X system. The updated Segoe UI layout, daylight/storm indicators and expanded FSM tests still require a fresh local build and GUI review.

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
