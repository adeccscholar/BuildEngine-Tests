// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "HistoryModel.h"
#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
using namespace weather_hist;
void Check(bool bValue,std::string_view svMessage) {
   if(!bValue) {
      throw std::runtime_error{std::string{svMessage}};
      }
   }
int main() {
   Check(ParseDate("03.08.2026")=="2026-08-03","German date");
   Check(ParseDate("2026-08-03")=="2026-08-03","ISO date");
   for(std::string const& strBad : {"31.02.2026","2026-02-30","01.01.1939"}) {
      bool bRejected{};
      try { (void)ParseDate(strBad); }
      catch(std::invalid_argument const&) { bRejected=true; }
      Check(bRejected,"reject invalid date");
      }
   Json const aArchive=Json::parse(R"json({
      "timezone":"Europe/Berlin",
      "hourly":{"time":["2026-08-03T11:00","2026-08-03T12:00"],
         "temperature_2m":[17.5,21.3],"apparent_temperature":[17.0,22.1],
         "relative_humidity_2m":[70,53],"pressure_msl":[1016.1,1015.4],
         "wind_speed_10m":[12.0,18.4],"wind_direction_10m":[190,270],
         "cloud_cover":[50,25],"weather_code":[2,1]},
      "daily":{"time":["2026-08-03"],"temperature_2m_mean":[19.5],
         "temperature_2m_min":[11.1],"temperature_2m_max":[24.4],
         "precipitation_sum":[1.5],"sunrise":["2026-08-03T05:35"],
         "sunset":["2026-08-03T20:52"]}
   })json");
   Report theReport=ParseArchive(aArchive,"2026-08-03");
   Check(theReport.strTimezone=="Europe/Berlin","timezone");
   auto Find=[&theReport](std::string_view svLabel) -> std::string {
      auto const aIt=std::ranges::find_if(theReport.vecMetrics,
         [svLabel](Metric const& aMetric) -> bool { return aMetric.strLabel==svLabel; });
      if(aIt==theReport.vecMetrics.end()) throw std::runtime_error{"Missing metric"};
      return aIt->strValue;
      };
   Check(Find("Temperatur (12:00)").find("21.3")!=std::string::npos,"12:00 sample");
   Check(Find("Luftfeuchtigkeit (12:00)").find("53")!=std::string::npos,"humidity");
   Check(Find("Windrichtung (aus, 12:00)").find("270")!=std::string::npos,"wind from");
   Check(Find("Taupunkt (12:00)")=="nicht verfuegbar","missing not zero");
   Check(Find("Tagesmaximum Temperatur").find("24.4")!=std::string::npos,"daily");
   AddForecastUv(theReport,Json::parse(R"json(
      {"hourly":{"time":["2026-08-03T12:00"],"uv_index":[5.2]}})json"));
   Check(Find("UV-Index (Prognosearchiv, 12:00)").find("5.2")!=std::string::npos,"UV");
   Report aMissing=ParseArchive(aArchive,"2026-08-03");
   AddUnavailableUv(aMissing);
   Check(aMissing.vecMetrics.back().strValue=="nicht verfuegbar","UV unavailable");
   bool bMissingNoon{};
   try { (void)ParseArchive(aArchive,"2026-08-04"); }
   catch(std::runtime_error const&) { bMissingNoon=true; }
   Check(bMissingNoon,"missing noon rejected");
   return 0;
   }
