// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once
/**
\file HistoryTimeline.h
\brief Day-long historical weather timeline with quarter-hour display slots.
\details The Historical Weather Archive supplies hourly reanalysis, while the
Historical Forecast API can supply genuine 15-minute regional forecast values.
Every displayed cell carries its source and interpolation provenance.
*/
#include "HistoryModel.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace weather_hist {

inline constexpr std::array<std::string_view,13> kTimelineColumns{
   "Temperatur °C", "Gefuehlt °C", "Feuchte %",
   "Taupunkt °C", "Druck NN hPa", "Wind km/h",
   "Boeen km/h", "Wind aus °", "Wolken %",
   "Niederschlag mm", "Regen mm", "UV", "WMO Code"
};

/**
\brief A single display slot in local civil time.
\details Each value contains [F15], [A60], [A~], [F60], or [F~], or "-" if
missing. F denotes historical forecast; A denotes reanalysis. A tilde denotes
a computed linear interpolation rather than original data.
*/
/**
\brief Read a numeric value at a known local timestamp.
\details No neighbouring value is substituted.
\param aSeries Provider JSON series with time and numeric arrays.
\param szKey The field to read.
\param strTime A local ISO date and time.
\returns Optional finite number.
\throw None.
*/
[[nodiscard]] inline std::optional<double> ExactNumber(
   Json const& aSeries, char const* szKey, std::string const& strTime) {
   if(!aSeries.is_object() || !aSeries.contains("time") ||
      !aSeries.at("time").is_array()) {
      return std::nullopt;
      }
   Json const& aTimes=aSeries.at("time");
   for(std::size_t uIndex=0;uIndex<aTimes.size();++uIndex) {
      if(aTimes[uIndex].is_string() &&
         aTimes[uIndex].get<std::string>()==strTime) {
         return weather_client::SeriesValue(aSeries,szKey,uIndex);
         }
      }
   return std::nullopt;
   }

/**
\brief Obtain the preceding and next hourly sample for one quarter-hour.
\details Missing next-hour data is never extrapolated. For directions,
the shortest clockwise/counter-clockwise arc is interpolated.
\param aSeries The archive or forecast hourly JSON series.
\param szKey The parameter to read.
\param strDate Requested ISO calendar day.
\param iHour Local hour from 0 to 23.
\param iMinute The minute: 0, 15, 30, or 45.
\param bCircular True only for compass direction.
\returns The optional value, and a boolean that is true if interpolated.
\throw None.
*/
[[nodiscard]] inline std::pair<std::optional<double>,bool> HourlyValue(
   Json const& aSeries, char const* szKey, std::string const& strDate,
   int iHour, int iMinute, bool bCircular=false) {
   std::string const strStart=std::format("{}T{:02}:00",strDate,iHour);
   std::optional<double> const flStart=ExactNumber(aSeries,szKey,strStart);
   if(iMinute==0) {
      return {flStart,false};
      }
   if(!flStart || iHour>=23) {
      return {std::nullopt,false};
      }
   std::string const strEnd=std::format("{}T{:02}:00",strDate,iHour+1);
   std::optional<double> const flEnd=ExactNumber(aSeries,szKey,strEnd);
   if(!flEnd) {
      return {std::nullopt,false};
      }
   double const flFraction=static_cast<double>(iMinute)/60.0;
   double flValue{};
   if(bCircular) {
      double const flDelta=std::remainder(*flEnd-*flStart,360.0);
      flValue=std::fmod(*flStart+flDelta*flFraction+360.0,360.0);
      }
   else {
      flValue=*flStart+(*flEnd-*flStart)*flFraction;
      }
   return {flValue,true};
   }

/**
\brief Format a provenance-tagged numeric cell.
\details A source marker is present on every valid numerical value.
\param flValue Optional numerical value.
\param svSource "F15", "F60", "F~", "A60", or "A~".
\param iDecimals Number of decimals to display.
\returns Value with source tag, or a dash.
\throw std::format_error On invalid formatting parameters.
*/
[[nodiscard]] inline std::string Cell(
   std::optional<double> flValue,std::string_view svSource,int iDecimals=1) {
   if(!flValue || !std::isfinite(*flValue)) {
      return "-";
      }
   return std::format("{:.{}f} [{}]",*flValue,iDecimals,svSource);
   }

/**
\brief Populate the chronological day view.
\details 96 labelled civil-time positions are produced for an ordinary day.
When forecast minutely_15 values exist they take precedence. Missing
quarter-hour values may use linearly interpolated archive hourly data with
an A~ marker; weather codes and interval precipitation are never interpolated.
UV derives only from the separate hourly historical forecast time series.
\param theReport Existing archive report, updated in place.
\param aArchive Historical Weather API hourly/daily JSON.
\param pForecast Optional Historical Forecast API JSON (minutely_15 and hourly).
\returns Nothing.
\throw std::runtime_error For a missing archive hourly series.
\note DST transition dates have a civil-time ambiguity: this regular local
clock grid does not represent repeated clock hours as separate UTC instants.
*/
inline void BuildTimeline(Report& theReport,Json const& aArchive,
   Json const* pForecast) {
   if(!aArchive.is_object() || !aArchive.contains("hourly") ||
      !aArchive.at("hourly").is_object()) {
      throw std::runtime_error{"Historischer Stundenverlauf fehlt."};
      }
   Json const& aArchiveHours=aArchive.at("hourly");
   Json const aEmpty=Json::object();
   Json const& aForecast15=pForecast && pForecast->is_object() &&
      pForecast->contains("minutely_15") &&
      pForecast->at("minutely_15").is_object()
      ? pForecast->at("minutely_15") : aEmpty;
   Json const& aForecastHours=pForecast && pForecast->is_object() &&
      pForecast->contains("hourly") &&
      pForecast->at("hourly").is_object()
      ? pForecast->at("hourly") : aEmpty;

   theReport.vecTimeline.clear();
   theReport.vecTimeline.reserve(96U);
   for(int iHour=0;iHour<24;++iHour) {
      for(int iMinute=0;iMinute<60;iMinute+=15) {
         TimelineRow theRow;
         theRow.strTime=std::format("{:02}:{:02}",iHour,iMinute);
         std::string const strTime=theReport.strDate+"T"+theRow.strTime;
         auto Forecast = [&](char const* szKey) -> std::optional<double> {
            return ExactNumber(aForecast15,szKey,strTime);
            };
         auto Archive = [&](char const* szKey,bool bCircular=false)
            -> std::pair<std::optional<double>,bool> {
            return HourlyValue(aArchiveHours,szKey,theReport.strDate,
               iHour,iMinute,bCircular);
            };
         auto FromNativeOrArchive = [&](std::size_t uIndex,char const* szKey,
            int iDecimals=1,bool bCircular=false) -> void {
            std::optional<double> const flNative=Forecast(szKey);
            if(flNative) {
               theRow.arValues[uIndex]=Cell(flNative,"F15",iDecimals);
               theRow.bNativeQuarterHour=true;
               }
            else {
               auto const [flArchive,bInterpolated]=Archive(szKey,bCircular);
               theRow.arValues[uIndex]=Cell(flArchive,bInterpolated?"A~":"A60",iDecimals);
               }
            };
         FromNativeOrArchive(0,"temperature_2m");
         FromNativeOrArchive(1,"apparent_temperature");
         FromNativeOrArchive(2,"relative_humidity_2m",0);
         FromNativeOrArchive(3,"dew_point_2m");
         {
            auto const [flValue,bInterpolated]=Archive("pressure_msl");
            theRow.arValues[4]=Cell(flValue,bInterpolated?"A~":"A60");
         }
         FromNativeOrArchive(5,"wind_speed_10m");
         FromNativeOrArchive(6,"wind_gusts_10m");
         FromNativeOrArchive(7,"wind_direction_10m",0,true);
         {
            auto const [flValue,bInterpolated]=Archive("cloud_cover");
            theRow.arValues[8]=Cell(flValue,bInterpolated?"A~":"A60",0);
         }
         // Interval sums cannot be linearly interpolated.
         auto Precip = [&](std::size_t uIndex,char const* szKey) -> void {
            std::optional<double> const flNative=Forecast(szKey);
            if(flNative) {
               theRow.arValues[uIndex]=Cell(flNative,"F15/15m");
               theRow.bNativeQuarterHour=true;
               }
            else if(iMinute==0) {
               theRow.arValues[uIndex]=Cell(
                  ExactNumber(aArchiveHours,szKey,strTime),"A60/1h");
               }
            else {
               theRow.arValues[uIndex]="-";
               }
            };
         Precip(9,"precipitation");
         Precip(10,"rain");
         {
            auto const [flValue,bInterpolated]=HourlyValue(
               aForecastHours,"uv_index",theReport.strDate,iHour,iMinute);
            theRow.arValues[11]=Cell(flValue,bInterpolated?"F~":"F60");
         }
         {
            std::optional<double> flCode=Forecast("weather_code");
            if(flCode) {
               theRow.arValues[12]=Cell(flCode,"F15",0);
               theRow.bNativeQuarterHour=true;
               }
            else if(iMinute==0) {
               theRow.arValues[12]=Cell(
                  ExactNumber(aArchiveHours,"weather_code",strTime),"A60",0);
               }
            else {
               theRow.arValues[12]="-";
               }
         }
         theReport.vecTimeline.push_back(std::move(theRow));
         }
      }
   }
} // namespace weather_hist
