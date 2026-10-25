// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherHazards.h
    \brief Explainable forecast indicators, NOT official weather warnings.
    \details Evaluates the supplied forward hourly forecast using amounts
    in mm/h, rolling 6-hour amounts, WMO codes, CAPE and gusts.
*/
#include "WeatherModel.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace weather_client {
   enum class HazardLevel { Unknown, None, Advisory, Elevated, High };
   enum class HazardKind { None, HeavyRain, Thunderstorm, Wind };

   struct ForecastHazard {
      HazardLevel level{HazardLevel::Unknown};
      HazardKind kind{HazardKind::None};
      std::string at;
      std::string summary{"Forecast hazard data unavailable"};
      double peakRainMm{};
      double maxSixHourRainMm{};
      double peakGustKmh{};
      double peakCape{};
      bool modelSignal{};
   };

   [[nodiscard]] inline ForecastHazard EvaluateHazards(
      WeatherData const& data,std::size_t limit=48
   ) {
      ForecastHazard result;
      std::size_t const n=(std::min)(limit,data.hours.size());
      if(n==0) return result;
      bool hasSignal=false;
      std::vector<double> rain(n,0.);
      auto promote=[&](HazardLevel level,HazardKind kind,
         std::string const& at,std::string text) {
         if(static_cast<int>(level)>static_cast<int>(result.level)) {
            result.level=level;
            result.kind=kind;
            result.at=at;
            result.summary=std::move(text);
         }
      };
      for(std::size_t i=0;i<n;++i) {
         auto const& hour=data.hours[i];
         auto const time=hour.time.size()>=16?hour.time.substr(5,11):hour.time;
         if(hour.precipitation && std::isfinite(*hour.precipitation)) {
            hasSignal=true;
            rain[i]=(std::max)(0.,*hour.precipitation);
            result.peakRainMm=(std::max)(result.peakRainMm,rain[i]);
            if(rain[i]>=25.)
               promote(HazardLevel::High,HazardKind::HeavyRain,time,
                  std::format("VERY HEAVY RAIN {:.1f} mm/h",rain[i]));
            else if(rain[i]>=15.)
               promote(HazardLevel::Elevated,HazardKind::HeavyRain,time,
                  std::format("HEAVY RAIN {:.1f} mm/h",rain[i]));
            else if(rain[i]>=8.)
               promote(HazardLevel::Advisory,HazardKind::HeavyRain,time,
                  std::format("INTENSE SHOWERS {:.1f} mm/h",rain[i]));
         }
         if(hour.weather_code) {
            hasSignal=true;
            int const code=*hour.weather_code;
            if(code==96||code==99)
               promote(HazardLevel::High,HazardKind::Thunderstorm,time,
                  "THUNDERSTORM WITH HAIL FORECAST");
            else if(code==95)
               promote(HazardLevel::Elevated,HazardKind::Thunderstorm,time,
                  "THUNDERSTORM FORECAST");
            else if(code==65||code==67||code==82)
               promote(HazardLevel::Advisory,HazardKind::HeavyRain,time,
                  "HEAVY RAIN WEATHER CODE");
         }
         if(hour.cape && std::isfinite(*hour.cape)) {
            hasSignal=true;
            result.peakCape=(std::max)(result.peakCape,*hour.cape);
            if(*hour.cape>=1500.)
               promote(HazardLevel::Advisory,HazardKind::Thunderstorm,time,
                  std::format("INSTABILITY CAPE {:.0f} J/kg",*hour.cape));
         }
         if(hour.wind_gusts_10m && std::isfinite(*hour.wind_gusts_10m)) {
            hasSignal=true;
            result.peakGustKmh=(std::max)(result.peakGustKmh,*hour.wind_gusts_10m);
            if(*hour.wind_gusts_10m>=105.)
               promote(HazardLevel::High,HazardKind::Wind,time,
                  std::format("SEVERE GUSTS {:.0f} km/h",*hour.wind_gusts_10m));
            else if(*hour.wind_gusts_10m>=85.)
               promote(HazardLevel::Elevated,HazardKind::Wind,time,
                  std::format("STRONG GUSTS {:.0f} km/h",*hour.wind_gusts_10m));
            else if(*hour.wind_gusts_10m>=65.)
               promote(HazardLevel::Advisory,HazardKind::Wind,time,
                  std::format("GUSTY WINDS {:.0f} km/h",*hour.wind_gusts_10m));
         }
      }
      // Only sum complete 6-hour windows with explicitly measured/forecast
      // hourly amounts. Missing hours must not be interpreted as zero.
      for(std::size_t i=0;i+6<=n;++i) {
         double sum=0.;
         bool complete=true;
         for(std::size_t j=i;j<i+6;++j) {
            if(!data.hours[j].precipitation) { complete=false;break; }
            sum+=rain[j];
         }
         if(!complete) continue;
         result.maxSixHourRainMm=(std::max)(result.maxSixHourRainMm,sum);
         auto const time=data.hours[i].time.size()>=16?
            data.hours[i].time.substr(5,11):data.hours[i].time;
         if(sum>=35.)
            promote(HazardLevel::High,HazardKind::HeavyRain,time,
               std::format("VERY HEAVY RAIN {:.1f} mm/6h",sum));
         else if(sum>=20.)
            promote(HazardLevel::Elevated,HazardKind::HeavyRain,time,
               std::format("HEAVY RAIN {:.1f} mm/6h",sum));
      }
      result.modelSignal=hasSignal;
      if(hasSignal && result.level==HazardLevel::Unknown) {
         result.level=HazardLevel::None;
         result.summary="NO ELEVATED HAZARD INDICATED";
      }
      return result;
   }

   [[nodiscard]] constexpr char const* HazardLevelName(HazardLevel level) {
      switch(level) {
         case HazardLevel::High: return "HIGH";
         case HazardLevel::Elevated: return "ELEVATED";
         case HazardLevel::Advisory: return "WATCH";
         case HazardLevel::None: return "LOW";
         default: return "N/A";
      }
   }
} // namespace weather_client
