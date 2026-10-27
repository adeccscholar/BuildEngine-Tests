# WeatherHist

Native RAD Studio 13 / BCC64X VCL application to retrieve historical weather for a postal address and date.

The VCL form has an address field, a date field (TT.MM.JJJJ or ISO yyyy-mm-dd), and an Abrufen button. The results list contains the available hourly values at **12:00 local time** and separate daily aggregates. When a variable is absent or null, it is shown as **nicht verfügbar**, not zero.

The project **reuses** the existing WeatherClient implementation in `../WeatherClient/WeatherNetwork.h` for HTTPS, Windows certificate trust, URL escaping and user-triggered Nominatim geocoding; it also reuses `WeatherModel.h` for JSON value parsing and weather-code descriptions. The historical archive is served by `https://archive-api.open-meteo.com/v1/archive`; optional UV uses `https://historical-forecast-api.open-meteo.com/v1/forecast`. UV, when available, is **historical forecast data**, not a historical reanalysis observation. The input date must be a completed past day, from 1940 onwards.

The HTTPS request is made in a `std::jthread`. The VCL thread polls a mutex-protected result with a `TTimer`; no VCL control is accessed by a worker thread. Address lookup happens only after the user presses the button, with the same process-local Nominatim throttling used in WeatherClient. Requests are time-limited using the existing WeatherClient curl configuration. Closing the form joins the worker thread.

## Tagesverlauf (15 Minuten)

The second VCL tab shows 96 local time slots for the selected day, 00:00–23:45. **F15** means an original 15-minute historical forecast, **A60** an original hourly reanalysis sample, **A~** an interpolated hourly reanalysis value, **F60/F~** hourly historical-forecast UV and its interpolated value. Precipitation and categorical weather codes are not interpolated; missing values remain '-'. These are different weather model products, not measurements at a street address. Daylight-saving switch dates may contain repeated or missing local hours; the fixed 96-slot view does not resolve this ambiguity.

## Build and launch

From the RAD Studio developer prompt in `D:\local\embarcadero\test_v3\tests`:

```bat
cmake -P bootstrap/Bootstrap.cmake
cmake -S WeatherHist -B ../build/Test/WeatherHist/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=../build/Test/WeatherHist/Release
cmake --build ../build/Test/WeatherHist/Release/cmake
ctest --test-dir ../build/Test/WeatherHist/Release/cmake --output-on-failure
cmake --install ../build/Test/WeatherHist/Release/cmake
..\build\Test\WeatherHist\Release\WeatherHist.exe
```

Debug uses `Debug` consistently. The target loads only BuildEngine-installed dynamic third-party dependencies; existing package versions are resolved through `admin/build-libraries.xml`. The project does not modify the existing WeatherClient or BuildEngine bootstrap.

## Attribution and limitations

The Open-Meteo archive is a meteorological reanalysis/model-derived product, not a certified observation at a given street address. Location coordinates come from OpenStreetMap Nominatim. The UI identifies both providers. Public Open-Meteo free API access and Nominatim are subject to their service usage conditions; see [REST.md](REST.md) and [LICENSE.md](LICENSE.md). Neither bulk geocoding nor automated scraping is implemented.

**Verification:** The CMake, VCL sources and deterministic model tests require a native Win64x BCC64X build and a live service check. These have not been executed in the user's local installation.
