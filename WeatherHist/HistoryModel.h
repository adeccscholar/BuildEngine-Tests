// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once
/** \file HistoryModel.h
\brief Pure C++23 weather-history representation.
\details Reuses the JSON parsing and WMO descriptions from WeatherClient.
*/
#include "../WeatherClient/WeatherModel.h"
#include <chrono>
#include <cmath>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace weather_hist {
using Json = weather_client::Json;

/** \brief One UTF-8 label and its formatted UTF-8 value.
\details Missing numbers have an explicit unavailability marker.
*/
struct Metric {
   std::string strLabel;
   std::string strValue;
};

/** \brief A dated report at a geocoded location.
\details Contains local noon values and full-day aggregates.
*/
struct Report {
   std::string strAddress;
   std::string strDate;
   std::string strTimezone;
   double flLatitude{};
   double flLongitude{};
   std::vector<Metric> vecMetrics;
};

/** \brief Parse and validate a user-entered calendar date.
\details Accepts TT.MM.JJJJ or JJJJ-MM-TT; requires a completed past day after 1940.
\param svText Date to parse.
\returns ISO yyyy-mm-dd.
\throw std::invalid_argument On malformed, future, or out-of-range dates.
*/
[[nodiscard]] inline std::string ParseDate(std::string_view svText) {
   std::string const strText{svText};
   if(strText.size() != 10) {
      throw std::invalid_argument{"Datum: TT.MM.JJJJ oder JJJJ-MM-TT erwartet."};
      }
   auto Digits = [&strText](int iOffset, int iCount) -> int {
      int iValue{};
      for(int iPos = iOffset; iPos < iOffset + iCount; ++iPos) {
         char const chValue = strText[static_cast<std::size_t>(iPos)];
         if(chValue < '0' || chValue > '9') {
            throw std::invalid_argument{"Datum enthaelt ungueltige Zeichen."};
            }
         iValue = iValue * 10 + chValue - '0';
         }
      return iValue;
      };
   int iYear{}, iMonth{}, iDay{};
   if(strText[2] == '.' && strText[5] == '.') {
      iDay = Digits(0,2); iMonth = Digits(3,2); iYear = Digits(6,4);
      }
   else if(strText[4] == '-' && strText[7] == '-') {
      iYear = Digits(0,4); iMonth = Digits(5,2); iDay = Digits(8,2);
      }
   else {
      throw std::invalid_argument{"Datum: TT.MM.JJJJ oder JJJJ-MM-TT erwartet."};
      }
   std::chrono::year_month_day const theDate{
      std::chrono::year{iYear},
      std::chrono::month{static_cast<unsigned>(iMonth)},
      std::chrono::day{static_cast<unsigned>(iDay)}};
   if(!theDate.ok() || iYear < 1940 ||
      std::chrono::sys_days{theDate} >=
         std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())) {
      throw std::invalid_argument{"Datum ungueltig, zu frueh oder noch nicht abgeschlossen."};
      }
   return std::format("{:04}-{:02}-{:02}", iYear, iMonth, iDay);
   }

/** \brief Select an exact local timestamp from a provider's series.
\details Does not infer missing timestamps or use neighbouring values.
\param aObject A JSON series object containing time.
\param strTime Requested local ISO time.
\returns Index of the exact timestamp.
\throw std::runtime_error If time is missing or not found.
*/
[[nodiscard]] inline std::size_t TimeIndex(Json const& aObject, std::string const& strTime) {
   if(!aObject.is_object() || !aObject.contains("time") || !aObject.at("time").is_array()) {
      throw std::runtime_error{"Zeitreihe im Open-Meteo-JSON fehlt."};
      }
   Json const& aTimes = aObject.at("time");
   for(std::size_t uIndex = 0; uIndex < aTimes.size(); ++uIndex) {
      if(aTimes[uIndex].is_string() && aTimes[uIndex].get<std::string>() == strTime) {
         return uIndex;
         }
      }
   throw std::runtime_error{"Zeitpunkt nicht verfuegbar: " + strTime};
   }

/** \brief Format a finite optional value.
\details Missing or null measurements remain visibly unavailable.
\param flValue Optional number.
\param svUnit Display unit.
\param iDecimals Decimal places.
\returns Formatted number or missing-value marker.
\throw std::format_error If formatting fails.
*/
[[nodiscard]] inline std::string Text(std::optional<double> flValue,
   std::string_view svUnit, int iDecimals = 1) {
   if(!flValue || !std::isfinite(*flValue)) {
      return "nicht verfuegbar";
      }
   return std::format("{:.{}f} {}", *flValue, iDecimals, svUnit);
   }

/** \brief Get a string from a parallel JSON array.
\details Sunrise and sunset are supplied as local ISO strings.
\param aObject JSON series object.
\param szKey Array key.
\param uIndex Requested day.
\returns String or empty string if unavailable.
\throw None.
*/
[[nodiscard]] inline std::string SeriesText(Json const& aObject,
   char const* szKey, std::size_t uIndex) {
   if(!aObject.is_object() || !aObject.contains(szKey)) {
      return {};
      }
   Json const& aValues = aObject.at(szKey);
   return aValues.is_array() && uIndex < aValues.size() && aValues[uIndex].is_string()
      ? aValues[uIndex].get<std::string>() : std::string{};
   }

/** \brief Parse exactly one historical calendar day.
\details Noon values and daily aggregates are never mixed. Uses the archive API
variables, excluding UV, which requires a separate historical-forecast source.
\param aRoot Open-Meteo Historical Weather API JSON.
\param strIsoDate Validated ISO date.
\returns Report with labeled metrics.
\throw std::runtime_error On invalid provider response or missing noon sample.
\pre Requested hourly and daily series use local ISO timestamps.
\post Missing numbers are rendered as unavailable, not zero.
*/
[[nodiscard]] inline Report ParseArchive(Json const& aRoot, std::string const& strIsoDate) {
   if(!aRoot.is_object() || !aRoot.contains("hourly") || !aRoot.contains("daily") ||
      !aRoot.at("hourly").is_object() || !aRoot.at("daily").is_object()) {
      throw std::runtime_error{"Open-Meteo-Archiv: stündliche oder tägliche Daten fehlen."};
      }
   Json const& aHours = aRoot.at("hourly");
   Json const& aDays = aRoot.at("daily");
   std::size_t const uHour = TimeIndex(aHours, strIsoDate + "T12:00");
   std::size_t const uDay = TimeIndex(aDays, strIsoDate);
   Report theReport;
   theReport.strDate = strIsoDate;
   theReport.strTimezone = weather_client::GetString(aRoot, "timezone", "auto");
   auto AddHour = [&](std::string strName, char const* szKey,
      std::string_view svUnit, int iDecimals = 1) -> void {
      theReport.vecMetrics.push_back({std::move(strName),
         Text(weather_client::SeriesValue(aHours,szKey,uHour),svUnit,iDecimals)});
      };
   auto AddDay = [&](std::string strName, char const* szKey, std::string_view svUnit) -> void {
      theReport.vecMetrics.push_back({std::move(strName),
         Text(weather_client::SeriesValue(aDays,szKey,uDay),svUnit)});
      };
   theReport.vecMetrics.push_back({"Zeitpunkt",
      strIsoDate + " 12:00 (" + theReport.strTimezone + ")"});
   AddHour("Temperatur (12:00)", "temperature_2m", "°C");
   AddHour("Gefuehlte Temperatur (12:00)", "apparent_temperature", "°C");
   AddHour("Luftfeuchtigkeit (12:00)", "relative_humidity_2m", "%", 0);
   AddHour("Taupunkt (12:00)", "dew_point_2m", "°C");
   AddHour("Luftdruck NN (12:00)", "pressure_msl", "hPa");
   AddHour("Luftdruck am Boden (12:00)", "surface_pressure", "hPa");
   AddHour("Windgeschwindigkeit (12:00)", "wind_speed_10m", "km/h");
   AddHour("Windboeen (12:00)", "wind_gusts_10m", "km/h");
   AddHour("Windrichtung (aus, 12:00)", "wind_direction_10m", "°", 0);
   AddHour("Bewoelkung (12:00)", "cloud_cover", "%", 0);
   AddHour("Niederschlag (vorige Stunde)", "precipitation", "mm");
   AddHour("Regen (vorige Stunde)", "rain", "mm");
   std::optional<int> const iCode = weather_client::SeriesInteger(aHours, "weather_code",uHour);
   theReport.vecMetrics.push_back({"Wetterlage (WMO, 12:00)",
      iCode ? std::format("{} (Code {})", weather_client::WeatherDescription(*iCode), *iCode)
            : "nicht verfuegbar"});
   AddDay("Tagesmittel Temperatur", "temperature_2m_mean", "°C");
   AddDay("Tagesminimum Temperatur", "temperature_2m_min", "°C");
   AddDay("Tagesmaximum Temperatur", "temperature_2m_max", "°C");
   AddDay("Tagesniederschlag", "precipitation_sum", "mm");
   AddDay("Maximale Windgeschwindigkeit", "wind_speed_10m_max", "km/h");
   AddDay("Maximale Windboeen", "wind_gusts_10m_max", "km/h");
   std::string const strSunrise = SeriesText(aDays,"sunrise",uDay);
   std::string const strSunset = SeriesText(aDays,"sunset",uDay);
   theReport.vecMetrics.push_back({"Sonnenaufgang",
      strSunrise.empty() ? "nicht verfuegbar" : strSunrise});
   theReport.vecMetrics.push_back({"Sonnenuntergang",
      strSunset.empty() ? "nicht verfuegbar" : strSunset});
   return theReport;
   }

/** \brief Add the UV index from the separate historical forecast archive.
\details The value is always labeled as a historical *forecast*, not reanalysis.
\param theReport Report being built.
\param aForecast JSON from historical-forecast-api.open-meteo.com.
\returns Nothing.
\throw std::runtime_error If the noon time is missing.
*/
inline void AddForecastUv(Report& theReport, Json const& aForecast) {
   std::optional<double> flUv;
   if(aForecast.is_object() && aForecast.contains("hourly") &&
      aForecast.at("hourly").is_object()) {
      Json const& aHours = aForecast.at("hourly");
      std::size_t const uHour = TimeIndex(aHours,theReport.strDate+"T12:00");
      flUv = weather_client::SeriesValue(aHours,"uv_index",uHour);
      }
   theReport.vecMetrics.push_back({"UV-Index (Prognosearchiv, 12:00)",Text(flUv,"")});
   }

/** \brief Mark the optional UV as unavailable.
\details Does not affect the archive reanalysis report.
\param theReport Report being built.
\returns Nothing.
\throw None.
*/
inline void AddUnavailableUv(Report& theReport) {
   theReport.vecMetrics.push_back({"UV-Index (Prognosearchiv, 12:00)","nicht verfuegbar"});
   }
}
