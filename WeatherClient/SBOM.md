<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – CycloneDX

- [WeatherClient application SBOM](SBOM-WeatherClient.cdx.json)

Deklarative Komponenten. Die tatsächliche DLL-Closure, Versionen und Lizenztexte müssen nach einem realen BCC64X-Build geprüft werden.

## Third-party data and remote services

The CycloneDX file lists **build/runtime software dependencies**, not a
license to use external weather/geocoding services. Open-Meteo (CC BY 4.0
weather data, hosted free API non-commercial only), Nominatim/OSM (ODbL),
ipapi.co and ipwho.is (proprietary APIs), and the runtime Segoe UI
operating-system font are documented separately in
[WeatherServices.md](WeatherServices.md). No font file is redistributed.
