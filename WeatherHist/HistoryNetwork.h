// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once
/** \file HistoryNetwork.h
\brief User-driven historical weather retrieval via WeatherClient transport.
\details Reuses the original libcurl HTTPS and Nominatim code, not copied clients.
*/
#include "HistoryModel.h"
#include "HistoryTimeline.h"
#include "../WeatherClient/WeatherNetwork.h"
#include <format>
#include <stdexcept>
#include <string>
namespace weather_hist {
/** \brief Resolve an address and fetch the requested historical day.
\details Requests Open-Meteo archive data; a separate optional historical
forecast query supplies UV when available and is labeled accordingly.
\param strAddress UTF-8 postal address.
\param strIsoDate ISO date (yyyy-mm-dd).
\returns A completed historical weather report.
\throw std::runtime_error On address or essential archive network failures.
\pre Called only after explicit user action.
\post Requested coordinates and API sources are never misrepresented as observations.
*/
[[nodiscard]] inline Report FetchHistory(std::string const& strAddress,
   std::string const& strIsoDate) {
   weather_client::Location const theLocation = weather_client::ResolveAddress(strAddress);
   std::string const strUrl = std::format(
      "https://archive-api.open-meteo.com/v1/archive?"
      "latitude={:.7f}&longitude={:.7f}&start_date={}&end_date={}&timezone=auto"
      "&hourly=temperature_2m,apparent_temperature,relative_humidity_2m,"
      "dew_point_2m,pressure_msl,surface_pressure,wind_speed_10m,"
      "wind_gusts_10m,wind_direction_10m,cloud_cover,precipitation,rain,weather_code"
      "&daily=temperature_2m_mean,temperature_2m_min,temperature_2m_max,"
      "precipitation_sum,wind_speed_10m_max,wind_gusts_10m_max,sunrise,sunset",
      theLocation.latitude,theLocation.longitude,strIsoDate,strIsoDate);
   Json const aArchive = Json::parse(weather_client::GetHttps(strUrl));
   Report theReport = ParseArchive(aArchive,strIsoDate);
   theReport.strAddress = theLocation.label;
   theReport.flLatitude = theLocation.latitude;
   theReport.flLongitude = theLocation.longitude;
   BuildTimeline(theReport,aArchive,nullptr);
   try {
      std::string const strUvUrl = std::format(
         "https://historical-forecast-api.open-meteo.com/v1/forecast?"
         "latitude={:.7f}&longitude={:.7f}&start_date={}&end_date={}"
         "&timezone=auto&hourly=uv_index"
         "&minutely_15=temperature_2m,apparent_temperature,relative_humidity_2m,"
         "dew_point_2m,wind_speed_10m,wind_gusts_10m,wind_direction_10m,"
         "precipitation,rain,weather_code",
         theLocation.latitude,theLocation.longitude,strIsoDate,strIsoDate);
      Json const aUv = Json::parse(weather_client::GetHttps(strUvUrl));
      AddForecastUv(theReport,aUv);
      BuildTimeline(theReport,aArchive,&aUv);
      }
   catch(std::exception const&) {
      AddUnavailableUv(theReport);
      }
   return theReport;
   }
}
