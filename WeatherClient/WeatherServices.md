<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient — providers, interfaces, licensing and privacy

[TOC]

## Scope

WeatherClient is an **educational, non-commercial example**. Its PolyForm
Noncommercial software license does **not** grant access to third-party web
services, and their data/API conditions remain independently binding.
Requests are made from the running user's machine. The model and GUI are
distinct from the API providers and use no third-party artwork.

## External HTTP interfaces

| Provider | Endpoint and purpose | Data/rights and permitted use | Operational restrictions |
| --- | --- | --- | --- |
| **Open-Meteo** | `https://api.open-meteo.com/v1/forecast`: current, hourly and 7-day daily forecast; weather codes, temperature, humidity, cloud, wind, CAPE, sunrise/sunset, UV | Weather data **CC BY 4.0** with attribution. Free API endpoint is **non-commercial only**; commercial access requires an appropriate subscription and customer API endpoint or separately hosted service | Free usage limit per published terms: 10,000/day, 5,000/hour, 600/min; not an uptime guarantee. Requests with many variables may count as more than one API call |
| **OSMF Nominatim** | `https://nominatim.openstreetmap.org/search`: user-submitted address to WGS84 coordinates | OSM data **ODbL 1.0** with visible “© OpenStreetMap contributors” attribution and license notice | Maximum **1 request/second for the whole application**, identifying User-Agent, no autocomplete, bulk scraping or automatic periodic searches. Local process limiter is not sufficient if many clients are deployed; use a dedicated/commercial provider or centralized proxy |
| **ipapi.co** | `https://ipapi.co/json/`: first attempt at approximate city/IP location | Provider proprietary service, **not open-licensed**; free/paid limits and restrictions in contract. The resulting coordinates are transient in this client, **never treated as house-level address** | Provider terms restrict persistent retention of IP-derived data (generally no more than 24 hours) and prohibit public resale / public archival. Do not save IP geolocation to user JSON |
| **ipwho.is** | `https://ipwho.is/`: approximate IP fallback | Proprietary service; free endpoint subject to provider terms, quotas, and possible limitations | Published free endpoint limit: **1,000 requests/day per client IP** at the time of review. Re-check terms before shipping; no address-level accuracy claim |

### Official terms and documentation

- Open-Meteo: [Terms & Privacy](https://open-meteo.com/en/terms),
  [Pricing](https://open-meteo.com/en/pricing),
  [Forecast API](https://open-meteo.com/en/docs)
- Nominatim: [Usage Policy](https://operations.osmfoundation.org/policies/nominatim/)
- OSM: [Copyright / ODbL / Attribution](https://www.openstreetmap.org/copyright)
- ipapi.co: [Terms](https://ipapi.co/terms/)
- ipwho.is: [Provider API documentation](https://ipwhois.io/documentation)

These are *service* and *data* terms, **not** source-library licenses. Recheck
them for each distribution. Provider IP addresses, endpoint contracts, and
terms can change independently from BuildEngine's library versions.

## Actual API parameters

The Open-Meteo call uses `latitude`, `longitude`, `timezone=auto`,
`forecast_days=7`, `current`, `hourly`, and `daily` lists.
Important fields include:

- `current.dew_point_2m` — dew-point temperature, displayed in Celsius/Fahrenheit
- `current.uv_index` — dimensionless instantaneous UV index and exposure category
- `hourly.dew_point_2m` / `hourly.uv_index` — forecast fields retained in the model

- `current.is_day` — the provider's current astronomical daylight flag
- `daily.sunrise` and `daily.sunset` — **local ISO date-time** for each day
- `daily.temperature_2m_min/max` — actual forecast daily low/high
- `hourly.weather_code` — WMO weather interpretation, codes 95/96/99 indicate thunderstorm conditions where supplied
- `hourly.cape` — Convective Available Potential Energy, in J/kg.
  The app labels `>= 1000 J/kg` as **“Elevated CAPE”**: an *instability
  indicator*, never a numeric lightning/thunderstorm probability
- `hourly.precipitation_probability` — probability of **precipitation**,
  which must **not** be presented as probability of thunderstorm
- `daily.uv_index_max`, `daily.precipitation_sum`,
  `daily.wind_gusts_10m_max`, `hourly.cloud_cover`

Not all weather models expose each field. Optional JSON values remain
optional and missing values are rendered as unavailable. None of the
thunder indicators is an official severe-weather warning or a substitute
for local emergency-alert systems.

## Attribution in the GUI

The bottom line displays attribution for Open-Meteo (CC BY 4.0) and
OSM/ODbL. The location provider must also be identified in product
documentation; for redistributed applications, evaluate whether additional
prominent attribution, source-specific copyrights, or terms are required.

## Security, location and storage

- TLS certificate and hostname checks remain **enabled**.
- With BCC64X/OpenSSL on Windows, libcurl is configured to consult native
  trusted certificate authorities, with an explicit PEM override using
  `--ca-bundle`, `WEATHERCLIENT_CA_BUNDLE`, `CURL_CA_BUNDLE`, or
  `SSL_CERT_FILE`. Never use “insecure” mode to hide TLS errors.
- IP location is approximate and used only in memory for the active session.
- `weather-location.json` stores **only the address entered by the user
  and resolved through Nominatim**, plus associated coordinates and timezone.
- Never upload personal/private addresses to a public geocoder without
  the user's explicit action or consent.
- The public Nominatim endpoint is for moderate usage. A commercialized,
  centrally deployed, or high-volume WeatherClient requires a replaceable
  geocoding service and proper license/contract.
- Open-Meteo free API is not a suitable default for commercial distribution;
  use a licensed customer endpoint or a valid self-hosted deployment.

## Typography / graphics licenses

The GUI uses the **Segoe UI** family seen in the existing
`pdfium/SDL2/ToolbarGraphics.h` example. The font is resolved from the
operating system using Skia or a Windows system-font-file lookup. Microsoft
font data is **not** included or copied into the repository or distributed
archives. The font remains subject to the OS/vendor's font terms.
`WeatherText.h` provides original Skia-drawn fallback glyphs if the
typeface cannot be loaded; graphics (sun, moon, clouds, lightning,
instruments) are generated from original Skia paths.

## FSM / refresh lifecycle

Boost.Statechart manages `Off -> Resolving -> Fetching -> Ready`, error
transitions, retries and manual refresh. HTTPS remains on the worker
thread, never in SDL2's render thread. A successful live response records
**the system-local retrieval clock** in the status line and schedules the
next refresh at the **next wall-clock quarter-hour boundary** (:00, :15,
:30, :45). A failed request schedules a **1-minute** retry (or an earlier
quarter-hour boundary), while previously successful data can remain visible. Demo mode
shows simulated values and must not be represented as live data.
`WeatherClientStatechart` exercises transitions without network access.

Review of terms: 2026-10-25. This file is informational, not legal advice.

## Weather data units and presentation conversion

The Open-Meteo request deliberately leaves its data in the
API's metric defaults: temperature in °C, precipitation in mm,
wind speed in km/h and mean sea-level pressure in hPa.
[WeatherUnits.h](WeatherUnits.h) performs the following **local**
conversions when the user toggles US display:

- °F = °C × 9/5 + 32 (temperature *changes* use only × 9/5)
- mph = km/h × 0.621371192237334
- inches of precipitation = mm / 25.4
- inHg = hPa × 0.029529983071445

Humidities, cloud coverage and precipitation probabilities
remain percentages; wind direction remains degrees. These
computations are not a separate remote service and carry no
external API license or recurring network charges.
The local settings JSON stores the unit identifier but does
not copy or convert source measurements.

## Moon-phase calculation (no external provider)

Unlike weather, address and IP geolocation, the lunar stage is computed
entirely locally from a known reference new-moon date and the mean
synodic month. No external lunar website or API is contacted. The
algorithm uses an astronomical approximation (29.530588853 days per
cycle, reference 2000-01-06 18:14 UTC). Its mathematical operations
do not require any third-party licensing. The date/time in the current
Open-Meteo response is local to the forecast location, so UTC offset
is approximated; lunar event timings may be inaccurate by several hours
and moonrise/moonset are **not** calculated.

## Local severe-weather model indicators (no extra service)

The 48-hour *forecast hazard* banner is derived from the Open-Meteo
hourly request, not from an official warning feed or external alert API.
The program requests `wind_gusts_10m` in the hourly field list in
addition to the existing precipitation, weather code and CAPE values.

The transparent program thresholds are intended as **risk flags**:
- >= 8 mm in any hour: WATCH (intense showers)
- >= 15 mm in an hour or >= 20 mm in a **complete** rolling six-hour
  window: ELEVATED (heavy rain)
- >= 25 mm in an hour or >= 35 mm in a complete six-hour window: HIGH
  (very heavy rain)
- WMO 95: ELEVATED forecast thunderstorm
- WMO 96/99: HIGH forecast thunderstorm with hail
- CAPE >= 1500 J/kg: WATCH instability only (not thunder probability)
- Gusts >= 65/85/105 km/h: WATCH / ELEVATED / HIGH, respectively

These are **software indicator thresholds**, not authoritative
warning classifications. Particularly the 6-hour and wind thresholds
do not imply identical operational warning levels in every country.
Hourly precipitation values are forecast *amounts*, while
`precipitation_probability` represents chance of precipitation and
is explicitly **not** an estimate of hail, lightning or severe-storm
probability. Model outputs can understate or overstate local extreme
weather and cannot replace an official warning.

Current reference material:
- [DWD warning criteria](https://www.dwd.de/DE/wetter/warnungen_aktuell/kriterien/warnkriterien.html)
- [Open-Meteo API parameters](https://open-meteo.com/en/docs)
