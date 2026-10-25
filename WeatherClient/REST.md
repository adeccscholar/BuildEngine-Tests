<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient: REST-Schnittstellen und curl-Tests

[TOC]

## Geltungsbereich und Quellen

Diese Beschreibung dokumentiert die **tatsächlich in [WeatherNetwork.h](WeatherNetwork.h) implementierten** HTTP-Aufrufe des WeatherClient. Das Programm ist ein lesender Client; es bietet selbst **keinen** REST-Server an, erfasst keine eigenen Messwerte und verwendet keine HTTP-POST-Anfragen. Stand der Quellcodeprüfung: 25.10.2026. Die folgenden Befehle sind reproduzierbare **manuelle Testaufrufe**, kein Nachweis dafür, dass die externen Dienste zum Ausführungszeitpunkt erreichbar sind.

| Dienst | HTTPS-Endpunkt | Aufruf im Programm | Ergebnis |
| --- | --- | --- | --- |
| Open-Meteo | `https://api.open-meteo.com/v1/forecast` | Start mit bekanntem Standort, Aktualisierung, manueller Refresh | Aktuelle Werte, stündliche und tägliche Vorhersage |
| Nominatim / OpenStreetMap | `https://nominatim.openstreetmap.org/search` | Ausschließlich nach bewusst eingegebener Adresse | Adresse → WGS84-Koordinaten |
| ipapi.co | `https://ipapi.co/json/` | Initiale ungefähre IP-Ortung ohne gespeicherte Adresse bzw. `IP LOCATION` | Region/Stadt und ungefähre Koordinaten |
| ipwho.is | `https://ipwho.is/` | Nur wenn ipapi.co nicht erfolgreich ist | Ersatz für die ungefähre IP-Ortung |

Es gibt **keinen** zusätzlichen DWD-, NINA-, Meteoalarm-, Mondphasen- oder Unwetterwarnungs-REST-Aufruf. Die Risikoindikatoren entstehen lokal aus den Open-Meteo-Prognosen. Sie sind keine amtlichen Warnungen. Siehe [Architecture.md](Architecture.md) und [WeatherServices.md](WeatherServices.md).

## Gemeinsame Transportbedingungen

Die C++-Implementierung verwendet `libcurl` und `nlohmann/json`:

- Nur `https://` wird akzeptiert. `CURLOPT_SSL_VERIFYPEER=1` und `CURLOPT_SSL_VERIFYHOST=2` bleiben aktiv.
- Verbindungen werden nach 7 Sekunden, ganze Anfragen nach 18 Sekunden begrenzt. Es werden maximal 2.000.000 Bytes Antworttext angenommen.
- Automatische HTTP-Weiterleitungen sind deaktiviert (`CURLOPT_FOLLOWLOCATION=0`); das Programm akzeptiert **genau HTTP 200**.
- Die Anwendung sendet den User-Agent `adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)`.
- `curl_easy_perform` liefert Antworttext, der als JSON geparst wird. Syntaxfehler, fehlende erforderliche Felder, ungültige Koordinaten und nicht erfolgreiche Provider-Ergebnisse führen zu Fehlern.
- Unter Windows versucht die mit OpenSSL gebaute Bibliothek die nativen CA-Vertrauensanker (`CURLSSLOPT_NATIVE_CA`) zu nutzen. Als explizite PEM-Datei werden `--ca-bundle`, `WEATHERCLIENT_CA_BUNDLE`, `CURL_CA_BUNDLE` oder `SSL_CERT_FILE` unterstützt (in dieser Reihenfolge).
- Die Benutzeroberfläche führt die Netzaufrufe nicht selbst aus. Der `std::jthread`-Worker liefert Ergebnisse über eine synchronisierte Queue an die SDL/FSM-Schleife.

**Wichtig:** Die separate Anwendung `curl.exe` kann einen anderen TLS-Backend oder CA-Speicher als die in WeatherClient gelinkte `libcurl` verwenden. Ein erfolgreicher Kommandozeilentest beweist deshalb nicht automatisch, dass der BCC64X-Client TLS erfolgreich aufbaut. Umgekehrt kann ein abweichender CLI-CA-Speicher einen CLI-Fehler erklären.

## Testumgebung (Windows CMD)

Die folgenden Befehle sind für die **Eingabeaufforderung (cmd.exe)** geschrieben, nicht für PowerShell. `^` am Zeilenende ist das CMD-Fortsetzungszeichen; danach dürfen keine weiteren Leerzeichen stehen. `curl.exe` bezeichnet ausdrücklich die Kommandozeilenanwendung und nicht die Bibliothek `libcurl`.

```bat
where curl.exe
curl.exe --version
```

Die Beispielkoordinaten **52.52, 13.405** sind reine Testwerte aus dem Demo-Datensatz, keine gespeicherte Benutzeradresse. API-Ergebnisse in `%TEMP%` können trotzdem IP-Adressen oder Ortsdaten enthalten. Sie gehören nicht in Versionsverwaltung, öffentliche Logs oder Screenshots.

Gemeinsame curl-Optionen der Beispiele: `--silent --show-error` für lesbare Ausgaben, `--fail-with-body` für einen Fehlerstatus bei HTTP 4xx/5xx, `--proto "=https"` zum Einschränken auf HTTPS, `--connect-timeout 7 --max-time 18` zur Orientierung an den App-Timeouts. Es wird **kein** `-k`/`--insecure` verwendet. Wer einen vertrauenswürdigen organisationsinternen PEM-CA-Satz benötigt, ergänzt `--cacert "C:\Pfad\ca-bundle.pem"`. Eine lokale selbst signierte oder unbekannte CA darf nicht durch Abschalten der Zertifikatsprüfung übergangen werden.

### 1. Open-Meteo: aktuelles Wetter, Stunden, Tage

Die Parameterlisten entsprechen dem aktuellen `FetchWeather` in [WeatherNetwork.h](WeatherNetwork.h). Die Daten bleiben in den API-Standard-Einheiten Celsius, km/h, mm und hPa; die US-Umrechnung erfolgt **erst bei der Ausgabe**, nicht im HTTP-Aufruf.

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

**Erwartete JSON-Struktur:** `timezone` und `timezone_abbreviation`, ein Objekt `current` mit `time` und optionalen Messwerten, sowie Objekte `hourly` und `daily` mit jeweils parallelen Datenarrays und `time`. Die API kann für einzelne Felder `null` liefern. `WeatherModel.h` speichert solche Werte als `std::optional`, nicht als erfundene Nullwerte. Für `current.time` verlangt der Parser einen plausiblen ISO-Zeitstring; wenn sowohl aktuelle Temperatur als auch stündliche Werte fehlen, schlägt das Parsing fehl.

| Gruppe | Wichtige verarbeitete Variablen | Bedeutung |
| --- | --- | --- |
| `current` | `temperature_2m`, `apparent_temperature`, `relative_humidity_2m`, `dew_point_2m`, `uv_index` | Temperatur, gefühlte Temperatur, Feuchte, Taupunkt und UV-Index |
| `current` | `pressure_msl`, `cloud_cover`, `precipitation`, `weather_code`, `is_day` | Luftdruck, Bewölkung, Niederschlag, Wettercode, Tag/Nacht |
| `current` | `wind_speed_10m`, `wind_gusts_10m`, `wind_direction_10m` | Geschwindigkeit, Böen, **Herkunftsrichtung** des Windes |
| `hourly` | Temperatur/UV/Taupunkt, Niederschlagsmenge und -wahrscheinlichkeit, `cape`, Wettercode und Böen | Stundenverlauf, Tageslicht-Symbole und lokale Risikoindikatoren |
| `daily` | Minimum/Maximum, Niederschlag, `uv_index_max`, Wind, `sunrise`, `sunset` | Tagesvorhersage und Sonnenzeiten in der Kopfzeile |

**Windsemantik:** `wind_direction_10m` bezeichnet die Richtung, **aus der** der Wind kommt (270° = Westwind). Der animierte Pfeil zeigt dagegen die Bewegungsrichtung (`(Richtung + 180) mod 360`; im Beispiel nach Osten). Das Modell behält die Quellangabe bei.

**15-Minuten-Aktualisierung:** Der Client fragt an den nächsten vollen Viertelstunden-Grenzen (:00, :15, :30, :45) ab. Das garantiert **keine** entsprechend neue Modellrechnung beim Provider, sondern allein den Abrufzeitpunkt des Clients.

Offizielle API-Dokumentation: [Open-Meteo Forecast API](https://open-meteo.com/en/docs).

### 2. Nominatim: Adresse in Koordinaten auflösen

`ResolveAddress` führt diesen Aufruf **nur nach aktiver Adresseingabe** aus. Der Code prüft 1 bis 300 Bytes Eingabelänge, kodiert den Suchtext und verwendet `format=jsonv2` sowie `limit=1`.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --get --url "https://nominatim.openstreetmap.org/search" ^
  --data-urlencode "q=Berlin, Deutschland" ^
  --data-urlencode "format=jsonv2" ^
  --data-urlencode "limit=1" ^
  --output "%TEMP%\weatherclient-nominatim.json"
```

**Erwartung:** Ein JSON-Array mit maximal einem Treffer, mit `lat`, `lon` und `display_name`. Keine Treffer: leeres Array, das `ParseAddressLocation` als Fehler behandelt. Die Koordinaten werden validiert; gespeichert werden nach erfolgreicher manueller Suche u. a. Originaladresse, Anzeigename, Koordinaten und Einheiten in `weather-location.json`.

**Nutzungsgrenzen:** Der öffentliche Nominatim-Server verlangt einen identifizierbaren User-Agent/Referer und höchstens **eine Anfrage pro Sekunde für die Anwendung**. Der aktuelle Code serialisiert nur innerhalb des eigenen Prozesses über einen Mutex und mindestens eine Sekunde Abstand. Das ist **kein globales Limit** für mehrere Installationen. Kein Autocomplete, keine automatisierten Serienabfragen und keine echten privaten Adressen in Testaufrufen. Für verteilte oder kommerzielle Anwendungen ist ein eigener/geprüft lizenzierter Geocoding-Dienst vorzusehen.

Verbindliche Referenz: [Nominatim Usage Policy](https://operations.osmfoundation.org/policies/nominatim/).

### 3. ipapi.co: ungefähre Ortung

`ResolveIp` verwendet bei fehlendem gespeichertem Ort zunächst diese API; es wird keine IP im URL übergeben. Der externe Dienst sieht die öffentliche IP der Anfrage.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --url "https://ipapi.co/json/" ^
  --output "%TEMP%\weatherclient-ipapi.json"
```

**Erwartung:** JSON-Objekt mit `latitude`, `longitude`, `city` und `timezone`. `ParseIpLocation` prüft eine eventuelle `error`-Kennzeichnung und die Koordinaten. Ein IP-Ort ist nur näherungsweise auf Stadt-/Regionsebene, **keine Straßenadresse**. IP-basierte Ortsdaten werden nicht allein zur Speicherung der Einheiteneinstellung persistiert.

Offizielle API-Referenz: [ipapi.co](https://ipapi.co/api/).

### 4. ipwho.is: Fallback bei fehlgeschlagener ipapi.co-Ortung

Dieser Aufruf ist im Client **keine zweite reguläre Parallelabfrage**, sondern wird erst nach einem Fehler der ersten IP-Ortung versucht.

```bat
curl.exe --silent --show-error --fail-with-body --proto "=https" --connect-timeout 7 --max-time 18 ^
  --user-agent "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)" ^
  --url "https://ipwho.is/" ^
  --output "%TEMP%\weatherclient-ipwho.json"
```

**Erwartung:** Ein JSON-Objekt mit `success=true`, `city`, `latitude`, `longitude` und optional `timezone.id`. **HTTP 200 allein genügt nicht**: Bei `success=false` wird ein Anwendungsfehler erzeugt. Fallen beide IP-Dienste aus, fordert die UI zur manuellen Adresseingabe auf.

Offizielle Referenz: [ipwho.is / ipwhois.io API](https://ipwhois.io/documentation). Dort ist für den öffentlichen kostenlosen Dienst eine Größenordnung von 1.000 Anfragen pro Tag und Client-IP dokumentiert. Nutzungsbedingungen und Kontingente sind vor Auslieferung erneut zu prüfen.

## Ergebnisprüfung und Fehlerbilder

```bat
type "%TEMP%\weatherclient-open-meteo.json"
type "%TEMP%\weatherclient-nominatim.json"
```

Die Textanzeige kontrolliert nur die Antwort, nicht den C++-Parser. Für HTTP-Status/Diagnose kann man **einmalig** `--include` statt `--output` verwenden und die zurückgegebenen Header ansehen. Die Beispiele dürfen nicht ungeprüft automatisiert oder in Schleifen wiederholt werden.

| Beobachtung | Häufige Ursache | Geeignete Prüfung |
| --- | --- | --- |
| DNS- oder Verbindungsfehler | Netz/Firewall/Proxy, DNS oder Endpoint | `curl.exe --version`, Proxy-Einstellungen, URL und Erreichbarkeit |
| TLS-Verifikationsfehler | Fehlende CA oder TLS-Inspection | CA-Vertrauen prüfen; gegebenenfalls `--cacert` bzw. App-PEM bereitstellen; **kein `-k`** |
| HTTP 403/429 | Rate Limit, unzureichender User-Agent oder Dienstbedingungen | Anbieterregel, Antworttext und gegebenenfalls `Retry-After` ansehen |
| HTTP 200, aber JSON-Fehler | Fehlende Felder, `success=false` oder geändertes Schema | JSON und Provider-Dokumentation mit `WeatherModel.h` vergleichen |
| CLI erfolgreich, Anwendung nicht | Unterschiedlicher TLS-Backend, Runtime-Closure oder Startverzeichnis | `--weather-probe`, `weather-client-probe.log`, aktive DLLs prüfen |
| App läuft, aber Daten veraltet | API liefert ältere Prognose oder Abruf fehlgeschlagen | Statuszeile `UPDATED`, aktuelles `current.time` und Fehlerzustand unterscheiden |

Den **echten** C++-Transport ohne SDL-Fenster prüft:

```bat
..\build\Test\WeatherClient\Release\WeatherClient.exe --weather-probe 52.52 13.405
type weather-client-probe.log
```

Der Probe-Modus schreibt in das **aktuelle Arbeitsverzeichnis**, verwendet die App-eigene `FetchWeather`-Implementierung und initiiert keinen IP- oder Nominatim-Aufruf. Ein erfolgloser Probe-Test ist kein Nachweis eines Renderingfehlers. Siehe [build.md](build.md) für Installation, Offline-/Grafik-/FSM-Tests und die Unterscheidung zwischen Build- und Installations-EXE.

## Rechte, Datenschutz und Lizenzgrenzen

Der Projektquellcode steht unter PolyForm Noncommercial 1.0.0. **Daraus ergibt sich keine Lizenz zur Nutzung externer API-Angebote.** Der freie Open-Meteo-API-Zugang ist nichtkommerziell; die Wetterdaten sind unter CC BY 4.0 mit Attribution verfügbar. OSM-Daten unterliegen der ODbL und der öffentliche Nominatim-Server seiner eigenen Nutzungsrichtlinie. IP-Anbieter haben eigene proprietäre Bedingungen. Limits, Preise und rechtliche Anforderungen können sich ändern.

Verbindliche bzw. vorrangige Ressourcen: [Open-Meteo Terms](https://open-meteo.com/en/terms), [Open-Meteo Pricing](https://open-meteo.com/en/pricing), [Nominatim Policy](https://operations.osmfoundation.org/policies/nominatim/), [OSM Copyright](https://www.openstreetmap.org/copyright), [ipapi.co Terms](https://ipapi.co/terms/) und [ipwho.is API](https://ipwhois.io/documentation). Weitere Hinweise enthält [WeatherServices.md](WeatherServices.md).

**Der lokale `curl.exe`-Test ist ausdrücklich optional und wird nicht durch CMake, CTest, BuildEngine oder den GUI-Demomodus automatisch ausgeführt.**
