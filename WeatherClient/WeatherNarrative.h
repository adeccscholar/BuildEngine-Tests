// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherNarrative.h
    \brief Weather summary built from observations and the actual forecast.
    \details Pure C++23 formatting: no SDL2, Skia, networking or Windows APIs.
*/
#include "WeatherModel.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace weather_client {

   struct WeatherBrief {
      std::string headline;
      std::string conditions;
      std::string outlook;
   };

   [[nodiscard]] inline std::string CardinalDirection(double degrees) {
      static constexpr std::string_view names[]{
         "N", "NE", "E", "SE", "S", "SW", "W", "NW"
      };
      if(!std::isfinite(degrees)) return "--";
      double const normalized = std::fmod(std::fmod(degrees,360.0)+360.0,360.0);
      auto const index = static_cast<std::size_t>(
         std::floor((normalized+22.5)/45.0))%8U;
      return std::string{names[index]};
   }

   [[nodiscard]] inline WeatherBrief ComposeWeatherBrief(WeatherData const& data) {
      WeatherBrief result;
      result.headline = data.current.weather_code
         ? WeatherDescription(*data.current.weather_code)
         : "Current weather";

      // Describe only observations actually supplied by the provider.
      if(data.current.apparent_temperature) {
         result.conditions += std::format("Feels like {:.1f} C",
            *data.current.apparent_temperature);
      }
      if(data.current.relative_humidity_2m) {
         if(!result.conditions.empty()) result.conditions += "  /  ";
         result.conditions += std::format("{:.0f}% humidity",
            *data.current.relative_humidity_2m);
      }
      if(data.current.wind_speed_10m) {
         if(!result.conditions.empty()) result.conditions += "  /  ";
         result.conditions += std::format("{} wind at {:.0f} km/h",
            data.current.wind_direction_10m
               ? CardinalDirection(*data.current.wind_direction_10m)
               : "Local",
            *data.current.wind_speed_10m);
      }
      if(result.conditions.empty()) {
         result.conditions = "Detailed current observations unavailable.";
      }

      double highestChance = -1.0;
      std::optional<double> firstTemperature;
      std::optional<double> lastTemperature;
      std::size_t seen = 0;
      for(auto const& hour : data.hours) {
         if(hour.precipitation_probability) {
            highestChance = (std::max)(highestChance,
               *hour.precipitation_probability);
         }
         if(hour.temperature_2m) {
            if(!firstTemperature) firstTemperature = hour.temperature_2m;
            lastTemperature = hour.temperature_2m;
         }
         if(++seen == 6U) break;
      }
      if(seen == 0) {
         result.outlook = "Hourly outlook unavailable.";
      } else {
         result.outlook = "Next 6h: ";
         if(highestChance >= 60.0) {
            result.outlook += std::format("rain risk up to {:.0f}%", highestChance);
         } else if(highestChance >= 30.0) {
            result.outlook += std::format("possible showers ({:.0f}% peak)", highestChance);
         } else if(highestChance >= 0.0) {
            result.outlook += std::format("low rain risk ({:.0f}% peak)", highestChance);
         } else {
            result.outlook += "precipitation chance unavailable";
         }
         if(firstTemperature && lastTemperature) {
            double const delta = *lastTemperature - *firstTemperature;
            if(delta >= 1.0) {
               result.outlook += std::format("; warming by {:.1f} C",delta);
            } else if(delta <= -1.0) {
               result.outlook += std::format("; cooling by {:.1f} C",-delta);
            } else {
               result.outlook += "; temperature fairly steady";
            }
         }
         result.outlook += ".";
      }
      if(data.days.size() > 1 && data.days[1].temperature_2m_max &&
         data.days[1].temperature_2m_min) {
         result.outlook += std::format(" Tomorrow: {:.0f} / {:.0f} C.",
            *data.days[1].temperature_2m_max,
            *data.days[1].temperature_2m_min);
      }
      return result;
   }

} // namespace weather_client
