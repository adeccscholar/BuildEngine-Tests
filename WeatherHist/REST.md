# WeatherHist HTTP interfaces

Uses the same libcurl configuration as [WeatherClient/WeatherNetwork.h](../WeatherClient/WeatherNetwork.h): HTTPS-only, certificate/hostname verification, Windows native CA support, 7 second connection timeout, 18 second request timeout, 2 MB response cap, HTTP 200 required. An explicit CA bundle can be configured through the same environment variables as WeatherClient.

## 1. Geocoding

User-initiated Nominatim request:

```bat
curl.exe --get "https://nominatim.openstreetmap.org/search" --data-urlencode "q=Breite Strasse 21, 13597 Berlin, Deutschland" --data-urlencode "format=jsonv2" --data-urlencode "limit=1" --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)"
```

Extract the returned `lat`, `lon` and `display_name`. This example must not be automated as a bulk-geocoding request.

## 2. Historical archive

After geocoding, the application calls:

```text
https://archive-api.open-meteo.com/v1/archive?latitude=LAT&longitude=LON&start_date=YYYY-MM-DD&end_date=YYYY-MM-DD&timezone=auto&hourly=temperature_2m,apparent_temperature,relative_humidity_2m,dew_point_2m,pressure_msl,surface_pressure,wind_speed_10m,wind_gusts_10m,wind_direction_10m,cloud_cover,precipitation,rain,weather_code&daily=temperature_2m_mean,temperature_2m_min,temperature_2m_max,precipitation_sum,wind_speed_10m_max,wind_gusts_10m_max,sunrise,sunset
```

Values at the **exact** local `YYYY-MM-DDT12:00` array index are selected. Daily aggregates use the `YYYY-MM-DD` index.

## 3. Optional forecast archive for UV

```text
https://historical-forecast-api.open-meteo.com/v1/forecast?latitude=LAT&longitude=LON&start_date=YYYY-MM-DD&end_date=YYYY-MM-DD&timezone=auto&hourly=uv_index&minutely_15=temperature_2m,apparent_temperature,relative_humidity_2m,dew_point_2m,wind_speed_10m,wind_gusts_10m,wind_direction_10m,precipitation,rain,weather_code
```

This API is a different data source; UV values are marked as historical *forecast*, not historical measured UV or reanalysis. When unavailable, the UI reports that rather than inventing a value. There are no default address or IP lookups at startup.

## Provider contracts

Official documentation: [Historical Weather](https://open-meteo.com/en/docs/historical-weather-api), [Historical Forecast](https://open-meteo.com/en/docs/historical-forecast-api), [Nominatim](https://operations.osmfoundation.org/policies/nominatim/). Observe Open-Meteo service restrictions and OSM attribution, and do not disable TLS verification.

## 15-minute timeline

Historical forecast minutely_15 fields are preferred when available. All other displayed samples carry an explicit archive provenance marker and interpolation flag. Precipitation and weather codes are never interpolated. A local civil day with a DST transition cannot be faithfully represented by a fixed 96-row list.
