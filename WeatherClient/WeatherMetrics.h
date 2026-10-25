// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherMetrics.h
    \brief Weather and daylight interpretation without GUI dependencies.
    \details Thunderstorm signals are weather-code/CAPE indicators, not
    probabilities of lightning or certified severe-weather warnings.
*/
#include "WeatherModel.h"
#include "WeatherUnits.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace weather_client {
   enum class StormSignal { Unavailable, NotIndicated, Instability, Forecast };

   /** \brief Classify the UV index in public-health bands.
       \details UV index is dimensionless and never converted to US units.
       \param flIndex Open-Meteo UV index reading.
       \returns A category label, or UNKNOWN for invalid measurements.
       \throw Nothing.
   */
   [[nodiscard]] inline std::string_view UvCategory(double flIndex) noexcept {
      if(!std::isfinite(flIndex)||flIndex<0.) return "UNKNOWN";
      if(flIndex<3.) return "LOW";
      if(flIndex<6.) return "MODERATE";
      if(flIndex<8.) return "HIGH";
      if(flIndex<11.) return "VERY HIGH";
      return "EXTREME";
   }


   [[nodiscard]] constexpr bool ThunderCode(int code) noexcept {
      return code==95 || code==96 || code==97 || code==99;
   }

   [[nodiscard]] inline WeatherDay const* Today(WeatherData const& weather) {
      std::string const date=weather.current.time.substr(0,10);
      for(auto const& day:weather.days) {
         if(day.date==date) return &day;
      }
      return weather.days.empty()?nullptr:&weather.days.front();
   }

   [[nodiscard]] inline std::string ClockText(std::string_view iso) {
      // Both date-time formats in Open-Meteo use location-local ISO 8601.
      if(iso.size()>=16 && iso[10]=='T') return std::string{iso.substr(11,5)};
      return "--:--";
   }

   [[nodiscard]] inline std::optional<bool> IsDayAt(
      std::string_view iso,WeatherData const& data
   ) {
      if(iso.size()<16) return std::nullopt;
      std::string_view const date=iso.substr(0,10);
      for(auto const& day:data.days) {
         if(day.date!=date || day.sunrise.size()<16 || day.sunset.size()<16) continue;
         return iso>=std::string_view{day.sunrise} &&
            iso<std::string_view{day.sunset};
      }
      return std::nullopt;
   }

   [[nodiscard]] inline bool CurrentDaylight(WeatherData const& data) {
      if(data.current.is_day) return *data.current.is_day!=0;
      return IsDayAt(data.current.time,data).value_or(true);
   }

   [[nodiscard]] inline StormSignal StormOutlook(WeatherData const& data) {
      if(data.current.weather_code && ThunderCode(*data.current.weather_code))
         return StormSignal::Forecast;
      double maxCape=-1.;
      bool weatherCodeAvailable=data.current.weather_code.has_value();
      std::size_t count=0;
      for(auto const& hour:data.hours) {
         if(hour.weather_code) {
            weatherCodeAvailable=true;
            if(ThunderCode(*hour.weather_code)) return StormSignal::Forecast;
         }
         if(hour.cape && std::isfinite(*hour.cape))
            maxCape=(std::max)(maxCape,*hour.cape);
         if(++count>=6) break;
      }
      // CAPE >= 1000 J/kg is an instability flag, NOT storm probability.
      if(maxCape>=1000.) return StormSignal::Instability;
      if(weatherCodeAvailable || maxCape>=0.) return StormSignal::NotIndicated;
      return StormSignal::Unavailable;
   }

   [[nodiscard]] inline std::string StormLabel(StormSignal signal) {
      switch(signal) {
         case StormSignal::Forecast: return "STORM FORECAST";
         case StormSignal::Instability: return "ELEVATED CAPE";
         case StormSignal::NotIndicated: return "NO STORM SIGNAL";
         default: return "STORM DATA N/A";
      }
   }

   [[nodiscard]] inline std::string RangeText(WeatherDay const* day,UnitSystem units=UnitSystem::Metric) {
      if(!day || !day->temperature_2m_min || !day->temperature_2m_max)
         return units==UnitSystem::US?"-- / -- °F":"-- / -- °C";
      return std::format("{} / {}",
         TempText(*day->temperature_2m_min,units,true),
         TempText(*day->temperature_2m_max,units,true));
   }
} // namespace weather_client
