// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherModel.h
    \brief Open-Meteo data model and strict JSON adapters.
    \details Field names follow the supplied WeatherAPI/WeatherData.h. This
    standalone consumer accepts the current Open-Meteo API names rather than
    changing the historical GPL-licensed WeatherAPI sources.
*/

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace weather_client {

   using Json = nlohmann::json;

   struct Location {
      std::string label;
      std::string address;
      std::string timezone;
      double latitude{};
      double longitude{};
      bool approximate{false};
   };

   struct WeatherCurrent {
      std::string time;
      std::optional<double> temperature_2m;
      std::optional<double> relative_humidity_2m;
      std::optional<double> apparent_temperature;
      std::optional<double> pressure_msl;
      std::optional<double> wind_speed_10m;
      std::optional<double> wind_gusts_10m;
      std::optional<double> wind_direction_10m;
      std::optional<double> precipitation;
      std::optional<double> cloud_cover;
      std::optional<int> weather_code;
      std::optional<int> is_day;
   };

   struct WeatherHour {
      std::string time;
      std::optional<double> temperature_2m;
      std::optional<double> apparent_temperature;
      std::optional<double> relative_humidity_2m;
      std::optional<double> cloud_cover;
      std::optional<double> cape;
      std::optional<double> precipitation_probability;
      std::optional<double> precipitation;
      std::optional<double> wind_speed_10m;
      std::optional<int> weather_code;
   };

   struct WeatherDay {
      std::string date;
      std::optional<double> temperature_2m_max;
      std::optional<double> temperature_2m_min;
      std::optional<double> precipitation_sum;
      std::optional<double> precipitation_probability_max;
      std::optional<double> wind_speed_10m_max;
      std::optional<double> wind_gusts_10m_max;
      std::optional<double> uv_index_max;
      std::string sunrise;
      std::string sunset;
      std::optional<int> weather_code;
   };

   struct WeatherData {
      std::string timezone;
      std::string timezone_abbreviation;
      WeatherCurrent current;
      std::vector<WeatherHour> hours;
      std::vector<WeatherDay> days;
   };

   [[nodiscard]] inline bool ValidCoordinates(double latitude, double longitude) noexcept {
      return std::isfinite(latitude) && std::isfinite(longitude)
         && latitude >= -90.0 && latitude <= 90.0
         && longitude >= -180.0 && longitude <= 180.0;
   }

   [[nodiscard]] inline std::optional<double> GetNumber(
      Json const& object, char const* name
   ) {
      if(!object.is_object()) {
         return std::nullopt;
      }
      auto const it = object.find(name);
      if(it == object.end() || it->is_null() || !it->is_number()) {
         return std::nullopt;
      }
      double const value = it->get<double>();
      return std::isfinite(value) ? std::optional<double>{value} : std::nullopt;
   }

   [[nodiscard]] inline std::optional<int> GetInteger(
      Json const& object, char const* name
   ) {
      auto const value = GetNumber(object, name);
      if(!value || *value < -1000000.0 || *value > 1000000.0 ||
         std::trunc(*value) != *value) {
         return std::nullopt;
      }
      return static_cast<int>(*value);
   }

   [[nodiscard]] inline std::string GetString(
      Json const& object, char const* name, std::string fallback = {}
   ) {
      if(!object.is_object()) {
         return fallback;
      }
      auto const it = object.find(name);
      return it != object.end() && it->is_string()
         ? it->get<std::string>()
         : std::move(fallback);
   }

   [[nodiscard]] inline std::optional<double> SeriesValue(
      Json const& series, char const* name, std::size_t index
   ) {
      if(!series.is_object() || !series.contains(name)) {
         return std::nullopt;
      }
      Json const& values = series.at(name);
      if(!values.is_array() || index >= values.size() || !values[index].is_number()) {
         return std::nullopt;
      }
      double const value = values[index].get<double>();
      return std::isfinite(value) ? std::optional<double>{value} : std::nullopt;
   }

   [[nodiscard]] inline std::optional<int> SeriesInteger(
      Json const& series, char const* name, std::size_t index
   ) {
      auto const value = SeriesValue(series, name, index);
      return value && std::trunc(*value) == *value &&
         *value >= -1000000.0 && *value <= 1000000.0
         ? std::optional<int>{static_cast<int>(*value)}
         : std::nullopt;
   }

   [[nodiscard]] inline WeatherData ParseWeather(Json const& root) {
      if(!root.is_object() || !root.contains("current") ||
         !root.at("current").is_object()) {
         throw std::runtime_error{ "Open-Meteo JSON: current object missing" };
      }
      Json const& current = root.at("current");
      WeatherData data;
      data.timezone = GetString(root, "timezone", "GMT");
      data.timezone_abbreviation = GetString(root, "timezone_abbreviation");
      data.current.time = GetString(current, "time");
      if(data.current.time.size() < 16) {
         throw std::runtime_error{ "Open-Meteo JSON: current.time missing" };
      }
      data.current.temperature_2m = GetNumber(current, "temperature_2m");
      data.current.relative_humidity_2m = GetNumber(current, "relative_humidity_2m");
      data.current.apparent_temperature = GetNumber(current, "apparent_temperature");
      data.current.pressure_msl = GetNumber(current, "pressure_msl");
      data.current.wind_speed_10m = GetNumber(current, "wind_speed_10m");
      data.current.wind_gusts_10m = GetNumber(current, "wind_gusts_10m");
      data.current.wind_direction_10m = GetNumber(current, "wind_direction_10m");
      data.current.precipitation = GetNumber(current, "precipitation");
      data.current.cloud_cover = GetNumber(current, "cloud_cover");
      data.current.weather_code = GetInteger(current, "weather_code");
      data.current.is_day = GetInteger(current, "is_day");

      if(root.contains("hourly") && root.at("hourly").is_object()) {
         Json const& series = root.at("hourly");
         if(series.contains("time") && series.at("time").is_array()) {
            std::size_t const count = (std::min)(series.at("time").size(), std::size_t{384});
            for(std::size_t i = 0; i < count; ++i) {
               if(!series.at("time")[i].is_string()) {
                  continue;
               }
               WeatherHour hour;
               hour.time = series.at("time")[i].get<std::string>();
               // ISO 8601 local times sort chronologically within a local day.
               if(hour.time < data.current.time.substr(0, 13)) {
                  continue;
               }
               hour.temperature_2m = SeriesValue(series, "temperature_2m", i);
               hour.apparent_temperature = SeriesValue(series, "apparent_temperature", i);
               hour.relative_humidity_2m = SeriesValue(series, "relative_humidity_2m", i);
               hour.cloud_cover = SeriesValue(series, "cloud_cover", i);
               hour.cape = SeriesValue(series, "cape", i);
               hour.precipitation_probability = SeriesValue(series, "precipitation_probability", i);
               hour.precipitation = SeriesValue(series, "precipitation", i);
               hour.wind_speed_10m = SeriesValue(series, "wind_speed_10m", i);
               hour.weather_code = SeriesInteger(series, "weather_code", i);
               data.hours.push_back(std::move(hour));
            }
         }
      }

      if(root.contains("daily") && root.at("daily").is_object()) {
         Json const& series = root.at("daily");
         if(series.contains("time") && series.at("time").is_array()) {
            std::size_t const count = (std::min)(series.at("time").size(), std::size_t{16});
            for(std::size_t i = 0; i < count; ++i) {
               if(!series.at("time")[i].is_string()) {
                  continue;
               }
               WeatherDay day;
               day.date = series.at("time")[i].get<std::string>();
               day.temperature_2m_max = SeriesValue(series, "temperature_2m_max", i);
               day.temperature_2m_min = SeriesValue(series, "temperature_2m_min", i);
               day.precipitation_sum = SeriesValue(series, "precipitation_sum", i);
               day.precipitation_probability_max = SeriesValue(series, "precipitation_probability_max", i);
               day.wind_gusts_10m_max = SeriesValue(series, "wind_gusts_10m_max", i);
               day.uv_index_max = SeriesValue(series, "uv_index_max", i);
               if(series.contains("sunrise") && series["sunrise"].is_array() &&
                  i<series["sunrise"].size() && series["sunrise"][i].is_string())
                  day.sunrise=series["sunrise"][i].get<std::string>();
               if(series.contains("sunset") && series["sunset"].is_array() &&
                  i<series["sunset"].size() && series["sunset"][i].is_string())
                  day.sunset=series["sunset"][i].get<std::string>();
               day.wind_speed_10m_max = SeriesValue(series, "wind_speed_10m_max", i);
               day.weather_code = SeriesInteger(series, "weather_code", i);
               data.days.push_back(std::move(day));
            }
         }
      }
      if(!data.current.temperature_2m && data.hours.empty()) {
         throw std::runtime_error{ "Open-Meteo JSON: temperature measurements missing" };
      }
      return data;
   }

   [[nodiscard]] inline Location ParseIpLocation(Json const& root) {
      if(!root.is_object() || root.value("error", false)) {
         throw std::runtime_error{ "IP geolocation provider returned an error" };
      }
      Location location;
      location.latitude = root.at("latitude").get<double>();
      location.longitude = root.at("longitude").get<double>();
      location.label = GetString(root, "city", "Approximate IP region");
      location.address = location.label;
      location.timezone = GetString(root, "timezone");
      location.approximate = true;
      if(!ValidCoordinates(location.latitude, location.longitude)) {
         throw std::runtime_error{ "Invalid IP geolocation coordinates" };
      }
      return location;
   }

   [[nodiscard]] inline Location ParseAddressLocation(
      Json const& root, std::string address
   ) {
      if(!root.is_array() || root.empty() || !root[0].is_object()) {
         throw std::runtime_error{ "No matching address returned" };
      }
      Json const& item = root[0];
      Location location;
      location.latitude = std::stod(GetString(item, "lat"));
      location.longitude = std::stod(GetString(item, "lon"));
      location.label = GetString(item, "display_name", address);
      location.address = std::move(address);
      location.approximate = false;
      if(!ValidCoordinates(location.latitude, location.longitude)) {
         throw std::runtime_error{ "Invalid address coordinates" };
      }
      return location;
   }

   [[nodiscard]] inline std::string WeatherDescription(int code) {
      if(code == 0) return "Clear sky";
      if(code == 1 || code == 2) return "Partly cloudy";
      if(code == 3) return "Overcast";
      if(code == 45 || code == 48) return "Fog";
      if(code >= 51 && code <= 57) return "Drizzle";
      if(code >= 61 && code <= 67) return "Rain";
      if(code >= 71 && code <= 77) return "Snow";
      if(code >= 80 && code <= 82) return "Rain showers";
      if(code >= 85 && code <= 86) return "Snow showers";
      if(code >= 95 && code <= 99) return "Thunderstorms";
      return "Weather unavailable";
   }

} // namespace weather_client
