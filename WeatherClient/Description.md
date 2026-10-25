<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – analoge Skia-Wetterstation

[TOC]

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

**Status:** The previous WeatherClient iteration compiled and passed its offline CTest on BCC64X. The new visual redesign has not yet been compiled or visually verified on the user's machine.
