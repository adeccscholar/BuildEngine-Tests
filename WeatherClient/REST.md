<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient: REST Interfaces and curl Tests

[TOC]

## Scope and Sources

This document describes the **actual HTTP requests implemented in [WeatherNetwork.h](WeatherNetwork.h)**, reviewed on October 25, 2026. WeatherClient is a read-only client. It does **not** expose a REST server, collect measurements locally, or issue POST requests. The commands below are manual reproducibility aids, **not claims of live connectivity tests**.

| Service | HTTPS endpoint | Trigger | Result |
| --- | --- | --- | --- |
| Open-Meteo | `https://api.open-meteo.com/v1/forecast` | Startup with known location, scheduled or manual refresh | Current, hourly, and daily weather |
| Nominatim / OpenStreetMap | `https://nominatim.openstreetmap.org/search` | Explicitly submitted address | Address to geographic coordinates |
| ipapi.co | `https://ipapi.co/json/` | No saved address or `IP LOCATION` | Approximate city/region coordinates |
| ipwho.is | `https://ipwho.is/` | Only after ipapi.co fails | Backup approximate IP location |

There are **no additional** DWD, NINA, Meteoalarm, lunar-phase, or official severe-weather alert API calls. Risk indicators are derived locally from Open-Meteo forecasts and are **not official warnings**.

## Shared Transport Rules

The C++ implementation uses libcurl and nlohmann/json.

- Only `https://` URLs are accepted. Peer and hostname verification remain enabled with `CURLOPT_SSL_VERIFYPEER=1` and `CURLOPT_SSL_VERIFYHOST=2`.
- Connection timeout: 7 seconds. Total timeout: 18 seconds. Response size limit: 2,000,000 bytes.
- Automatic redirects are disabled (`CURLOPT_FOLLOWLOCATION=0`); exactly **HTTP 200** is required.
- User-Agent: `adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)`.
- The response is parsed as JSON; malformed data, missing required fields, invalid coordinates, and unsuccessful provider results produce errors.
- On Windows, the OpenSSL-backed libcurl uses `CURLSSLOPT_NATIVE_CA`. An explicit PEM bundle may be provided through `--ca-bundle`, `WEATHERCLIENT_CA_BUNDLE`, `CURL_CA_BUNDLE`, or `SSL_CERT_FILE`, in that order.
- Blocking requests execute in a `std::jthread` worker; results are returned to the SDL/FSM main loop through a synchronized queue.

**Important:** The separate `curl.exe` program might use a different TLS backend or CA store than WeatherClient's linked libcurl. A successful CLI request does not prove the BCC64X client can establish the same TLS connection.

## Windows CMD Test Environment

These commands target **cmd.exe**, not PowerShell. A trailing `^` continues a command onto the next line; no whitespace may follow it. `curl.exe` refers to the executable rather than the embedded libcurl library.

```bat
where curl.exe
curl.exe --version
```

The sample location **52.52, 13.405** comes from the demo data and is not a saved user address. Responses in `%TEMP%` may contain location/IP data; do not commit them or publish them in logs.

The examples use `--silent --show-error`, `--fail-with-body`, `--proto "=https"`, `--connect-timeout 7`, and `--max-time 18`. Never use `-k` or `--insecure` to bypass certificate verification. For a trusted organizational PEM file, add `--cacert "C:\path\to\ca-bundle.pem"`.

### 1. Open-Meteo: Current, Hourly, and Daily Weather

The fields match `FetchWeather` in [WeatherNetwork.h](WeatherNetwork.h). Data remains in the provider's metric units (Celsius, km/h, mm, and hPa). Conversion to US units happens only during presentation.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --get --url "https://api.open-meteo.com/v1/forecast" ^
  --data-urlencode "latitude=52.5200000" ^
  --data-urlencode "longitude=13.4050000" ^
  --data-urlencode "timezone=auto" ^
  --data-urlencode "forecast_days=7" ^
  --data-urlencode "current=temperature_2m,relative_humidity_2m,dew_point_2m,uv_index,apparent_temperature,pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m,precipitation,cloud_cover,weather_code,is_day" ^
  --data-urlencode "hourly=temperature_2m,apparent_temperature,relative_humidity_2m,dew_point_2m,uv_index,precipitation_probability,precipitation,wind_speed_10m,wind_gusts_10m,cloud_cover,cape,weather_code" ^
  --data-urlencode "daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum,precipitation_probability_max,wind_speed_10m_max,wind_gusts_10m_max,uv_index_max,sunrise,sunset" ^
  --output "%TEMP%\weatherclient-open-meteo.json"
```

**Expected JSON structure:** `timezone`, `timezone_abbreviation`, a `current` object with `time`, and parallel `time` arrays in `hourly` and `daily`. Some measurements may be `null`; `WeatherModel.h` represents them with `std::optional`, not fabricated zero values. The parser rejects a response lacking both a current temperature and hourly values.

| Group | Key fields | Usage |
| --- | --- | --- |
| `current` | `temperature_2m`, `apparent_temperature`, `relative_humidity_2m`, `dew_point_2m`, `uv_index` | Temperature, apparent temperature, humidity, dew point, UV |
| `current` | `pressure_msl`, `cloud_cover`, `precipitation`, `weather_code`, `is_day` | Pressure, cloud cover, precipitation, weather condition and daylight |
| `current` | `wind_speed_10m`, `wind_gusts_10m`, `wind_direction_10m` | Speed, gusts and **wind origin** |
| `hourly` | Temperature, dew point, UV, precipitation amount/probability, `cape`, weather code and gusts | Forecast charts and risk indicators |
| `daily` | Minimum/maximum, precipitation, `uv_index_max`, wind, `sunrise`, `sunset` | Daily forecast and selected-day solar times |

**Wind direction:** `wind_direction_10m` describes where the wind **comes from**: 270° means a westerly wind. The movement arrow points toward `(origin + 180) mod 360`, which is east in that example. The source measurement is preserved.

**Refresh timing:** Requests are scheduled at the next quarter-hour boundary (:00, :15, :30, :45). A new client request does **not** guarantee that the provider has issued a newer forecast model run.

API reference: [Open-Meteo Forecast API](https://open-meteo.com/en/docs).

### 2. Nominatim: Resolve an Address to Coordinates

`ResolveAddress` is invoked **only after explicit address submission**. It validates an input length of 1–300 bytes, URL-encodes the query, and specifies `format=jsonv2` and `limit=1`.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --get --url "https://nominatim.openstreetmap.org/search" ^
  --data-urlencode "q=Berlin, Deutschland" ^
  --data-urlencode "format=jsonv2" ^
  --data-urlencode "limit=1" ^
  --output "%TEMP%\weatherclient-nominatim.json"
```

**Expected response:** A JSON array containing at most one object with `lat`, `lon`, and `display_name`. An empty array is treated as no match. Once valid coordinates have been returned, the user-entered address, display label, coordinates, and units are saved in `weather-location.json`.

**Public server policy:** Requests must identify the application and respect a maximum of **one request per second per application**. The current implementation serializes requests within its own process using a mutex and a one-second gap. This is **not** a global rate limit across installations. No autocomplete, mass geocoding, or real private addresses in sample commands. A distributed/commercial deployment requires a suitably licensed geocoder or self-hosted solution.

Reference: [Nominatim Usage Policy](https://operations.osmfoundation.org/policies/nominatim/).

### 3. ipapi.co: Approximate Location

When no saved location exists, `ResolveIp` first contacts ipapi.co. No IP address is embedded in the URL; the remote provider sees the request's public source IP.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --url "https://ipapi.co/json/" ^
  --output "%TEMP%\weatherclient-ipapi.json"
```

**Expected response:** An object with `latitude`, `longitude`, `city`, and `timezone`. `ParseIpLocation` validates coordinates and a possible `error` indicator. The result is an approximate city/region, **never a confirmed street address**. Merely saving unit preferences does not persist IP-derived address data.

Reference: [ipapi.co API](https://ipapi.co/api/).

### 4. ipwho.is: Fallback for IP Location

This request is **not** a routine parallel query. It runs only if the first IP-location provider fails.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --url "https://ipwho.is/" ^
  --output "%TEMP%\weatherclient-ipwho.json"
```

**Expected response:** A JSON object with `success=true`, `city`, `latitude`, `longitude`, and optionally `timezone.id`. **HTTP 200 is not sufficient:** `success=false` is an application failure. If both providers fail, the UI asks the user to enter an address manually.

Reference: [ipwho.is API](https://ipwhois.io/documentation). Provider limits and usage terms must be checked again before distribution.

## Response Inspection and Diagnostics

```bat
type "%TEMP%\weatherclient-open-meteo.json"
type "%TEMP%\weatherclient-nominatim.json"
```

This inspection checks only the returned text, not the C++ parser. For one-off HTTP status diagnostics, `--include` can show headers. Do not repeatedly automate these public API examples without considering provider policy.

| Symptom | Possible cause | Check |
| --- | --- | --- |
| DNS/connection failure | DNS, firewall, proxy, or endpoint unavailable | `curl.exe --version`, network and proxy configuration |
| TLS verification failure | Missing CA, organizational TLS inspection | Trust store and `--cacert`; **never `-k`** |
| HTTP 403/429 | Rate limit, User-Agent, or usage policy | Provider response and `Retry-After` |
| HTTP 200 with JSON error | Missing fields, `success=false`, changed schema | Compare JSON with `WeatherModel.h` |
| CLI succeeds, application fails | Different TLS backend, DLL closure, working directory | `--weather-probe`, logs, imported DLLs |
| Application runs but data is stale | Older provider data or a failed refresh | `UPDATED` status, `current.time`, error state |

Test the **actual C++ HTTPS path** without opening SDL:

```bat
..\build\Test\WeatherClient\Release\WeatherClient.exe --weather-probe 52.52 13.405
type weather-client-probe.log
```

The probe writes `weather-client-probe.log` to the **current working directory** and uses the application's own `FetchWeather` path; it does not contact Nominatim or IP-location services. A failing probe does not demonstrate a rendering defect. See [build.md](build.md) for the installed-EXE distinction and offline/graphics/FSM tests.

## Rights, Privacy, and Licensing

PolyForm Noncommercial 1.0.0 governs the project's own source code; it does **not** grant permission to use external APIs. Open-Meteo hosted free access is non-commercial; its weather data has CC BY 4.0 attribution conditions. OSM data uses ODbL, and the public Nominatim server imposes its own operational rules. IP providers have separate proprietary terms.

Authoritative references: [Open-Meteo Terms](https://open-meteo.com/en/terms), [Open-Meteo Pricing](https://open-meteo.com/en/pricing), [Nominatim Policy](https://operations.osmfoundation.org/policies/nominatim/), [OSM Copyright](https://www.openstreetmap.org/copyright), [ipapi.co Terms](https://ipapi.co/terms/), and [ipwho.is API](https://ipwhois.io/documentation).

**These manual `curl.exe` tests are optional. They are not automatically executed by CMake, CTest, BuildEngine, or GUI demo mode.**
