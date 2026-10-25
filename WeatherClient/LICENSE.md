<!-- SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH -->
<!-- SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 -->

# WeatherClient – vollständige Projektlizenz

## Eigene neue Dateien

`WeatherClient.cpp`, `WeatherModel.h`, `WeatherNetwork.h`,
`CMakeLists.txt`, Dokumentation und Testdaten wurden unter der
**PolyForm Noncommercial License 1.0.0** erstellt. Die vollständigen
Lizenzbedingungen und der Required Notice folgen unverändert.

## Web services, data licensing and system font

The **PolyForm Noncommercial** project license applies to the *new code*,
not to weather records, OSM address data or access to public service APIs.
Open-Meteo weather data is **CC BY 4.0**, but its free hosted endpoint is
for non-commercial usage. Nominatim/OSM geographic data is **ODbL 1.0**
with attribution and strict public endpoint policy. IP geolocation providers
are proprietary services with individual request limits and retention terms.

The PDFium/SDL2 example uses **Segoe UI**. WeatherClient references that
operating-system font through Skia without redistributing Microsoft's font
files. The original hand-authored Skia fallback glyphs are part of this
project, not a copied commercial font.

See [WeatherServices.md](WeatherServices.md) for current third-party URLs,
rights, operational limits and privacy guidance.

## Externe Bestandteile

| Bibliothek / Datenquelle | Lizenz / Nutzung |
| --- | --- |
| Skia | BSD-3-Clause; eingebundene Komponenten mit eigenen Hinweisen |
| SDL2 | Zlib |
| Boost | BSL-1.0 |
| libcurl | curl |
| OpenSSL | Apache-2.0 |
| nlohmann/json | MIT |
| zlib | Zlib |
| Brotli | MIT |
| Zstandard | BSD-3-Clause |
| XZ / liblzma | 0BSD (Komponenten beachten) |
| bzip2 | bzip2-1.0.6 |
| FreeType | FTL OR GPL-2.0-or-later |
| HarfBuzz | MIT |
| libpng | libpng-2.0 |
| libjpeg-turbo | IJG AND BSD-3-Clause (Komponenten beachten) |
| Open-Meteo | Dienst-/Attributionsbedingungen prüfen |
| OpenStreetMap/Nominatim | ODbL-Datenattribution + öffentliche API-Nutzungsbedingungen |
| ipapi.co | API-Nutzungsbedingungen beachten |

Die tatsächlich ausgelieferten Original-Lizenz- und Notice-Texte aus
den BuildEngine-Paketen sowie die binär verifizierte Abhängigkeitskette
sind **vor einer Distribution** zu ergänzen. Die gelieferten alten
`WeatherAPI`-/`WeatherAPIServer`-Quellen tragen überwiegend
`GPL-3.0-or-later`; sie werden **nicht** in dieses neue Programm
übernommen und nicht unter PolyForm umetikettiert.

## PolyForm Noncommercial License 1.0.0

# PolyForm Noncommercial License 1.0.0

<https://polyformproject.org/licenses/noncommercial/1.0.0>

## Acceptance

In order to get any license under these terms, you must agree
to them as both strict obligations and conditions to all
your licenses.

## Copyright License

The licensor grants you a copyright license for the
software to do everything you might do with the software
that would otherwise infringe the licensor's copyright
in it for any permitted purpose.  However, you may
only distribute the software according to [Distribution
License](#distribution-license) and make changes or new works
based on the software according to [Changes and New Works
License](#changes-and-new-works-license).

## Distribution License

The licensor grants you an additional copyright license
to distribute copies of the software.  Your license
to distribute covers distributing the software with
changes and new works permitted by [Changes and New Works
License](#changes-and-new-works-license).

## Notices

You must ensure that anyone who gets a copy of any part of
the software from you also gets a copy of these terms or the
URL for them above, as well as copies of any plain-text lines
beginning with `Required Notice:` that the licensor provided
with the software.  For example:

> Required Notice: Copyright Yoyodyne, Inc. (http://example.com)

## Changes and New Works License

The licensor grants you an additional copyright license to
make changes and new works based on the software for any
permitted purpose.

## Patent License

The licensor grants you a patent license for the software that
covers patent claims the licensor can license, or becomes able
to license, that you would infringe by using the software.

## Noncommercial Purposes

Any noncommercial purpose is a permitted purpose.

## Personal Uses

Personal use for research, experiment, and testing for
the benefit of public knowledge, personal study, private
entertainment, hobby projects, amateur pursuits, or religious
observance, without any anticipated commercial application,
is use for a permitted purpose.

## Noncommercial Organizations

Use by any charitable organization, educational institution,
public research organization, public safety or health
organization, environmental protection organization,
or government institution is use for a permitted purpose
regardless of the source of funding or obligations resulting
from the funding.

## Fair Use

You may have "fair use" rights for the software under the
law. These terms do not limit them.

## No Other Rights

These terms do not allow you to sublicense or transfer any of
your licenses to anyone else, or prevent the licensor from
granting licenses to anyone else.  These terms do not imply
any other licenses.

## Patent Defense

If you make any written claim that the software infringes or
contributes to infringement of any patent, your patent license
for the software granted under these terms ends immediately. If
your company makes such a claim, your patent license ends
immediately for work on behalf of your company.

## Violations

The first time you are notified in writing that you have
violated any of these terms, or done anything with the software
not covered by your licenses, your licenses can nonetheless
continue if you come into full compliance with these terms,
and take practical steps to correct past violations, within
32 days of receiving notice.  Otherwise, all your licenses
end immediately.

## No Liability

***As far as the law allows, the software comes as is, without
any warranty or condition, and the licensor will not be liable
to you for any damages arising out of these terms or the use
or nature of the software, under any kind of legal claim.***

## Definitions

The **licensor** is the individual or entity offering these
terms, and the **software** is the software the licensor makes
available under these terms.

**You** refers to the individual or entity agreeing to these
terms.

**Your company** is any legal entity, sole proprietorship,
or other kind of organization that you work for, plus all
organizations that have control over, are under the control of,
or are under common control with that organization.  **Control**
means ownership of substantially all the assets of an entity,
or the power to direct its management and policies by vote,
contract, or otherwise.  Control can be direct or indirect.

**Your licenses** are all the licenses granted to you for the
software under these terms.

**Use** means anything you do with the software requiring one
of your licenses.

Required Notice: Copyright (c) 2026 adecc Systemhaus GmbH

## Original unit conversion and icon additions

`WeatherUnits.h`, vector-painted moon, sunrise/sunset icons,
differentiated dial motifs, local unit preferences and updated
project documentation are first-party code under the
project's PolyForm Noncommercial License 1.0.0.
No commercial weather icon artwork or Microsoft font file
has been copied into the repository.
