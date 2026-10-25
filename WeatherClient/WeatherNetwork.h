// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherNetwork.h
    \brief HTTPS transfers and address lookup, isolated from SDL and the FSM.
*/
#include "WeatherModel.h"
#include "WeatherUnits.h"
#include <curl/curl.h>
#include <array>
#include <cstdlib>
#include <format>
#include <filesystem>
#include <fstream>
#include <optional>
#include <system_error>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>
#include <string_view>

namespace weather_client {
   [[nodiscard]] inline std::size_t CurlWrite(
      char* data, std::size_t size, std::size_t count, void* user
   ) {
      std::size_t const bytes = size * count;
      auto& result = *static_cast<std::string*>(user);
      if(result.size() + bytes > 2'000'000U) return 0U;
      result.append(data, bytes);
      return bytes;
   }

   [[nodiscard]] inline std::string SelectCaBundle(std::string_view explicitBundle = {}) {
      if(!explicitBundle.empty()) return std::string{explicitBundle};
      for(char const* name : {"WEATHERCLIENT_CA_BUNDLE","CURL_CA_BUNDLE","SSL_CERT_FILE"}) {
         if(char const* value=std::getenv(name);value&&*value)
            return std::string{value};
      }
      return {};
   }

   [[nodiscard]] inline std::string GetHttps(
      std::string const& url,
      std::string_view agent = "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)",
      std::string_view caBundle = {}
   ) {
      if(!url.starts_with("https://"))
         throw std::runtime_error{"Only HTTPS endpoints are accepted"};
      using Handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
      Handle handle{curl_easy_init(),&curl_easy_cleanup};
      if(!handle) throw std::runtime_error{"curl_easy_init failed"};
      std::string result;
      std::string const userAgent{agent};
      std::array<char,CURL_ERROR_SIZE> errorBuffer{};
      curl_easy_setopt(handle.get(),CURLOPT_ERRORBUFFER,errorBuffer.data());
      curl_easy_setopt(handle.get(),CURLOPT_URL,url.c_str());
      curl_easy_setopt(handle.get(),CURLOPT_USERAGENT,userAgent.c_str());
      curl_easy_setopt(handle.get(),CURLOPT_WRITEFUNCTION,&CurlWrite);
      curl_easy_setopt(handle.get(),CURLOPT_WRITEDATA,&result);
      curl_easy_setopt(handle.get(),CURLOPT_FOLLOWLOCATION,0L);
      curl_easy_setopt(handle.get(),CURLOPT_CONNECTTIMEOUT,7L);
      curl_easy_setopt(handle.get(),CURLOPT_TIMEOUT,18L);
      curl_easy_setopt(handle.get(),CURLOPT_SSL_VERIFYPEER,1L);
      curl_easy_setopt(handle.get(),CURLOPT_SSL_VERIFYHOST,2L);
      curl_easy_setopt(handle.get(),CURLOPT_PROTOCOLS_STR,"https");
      // The BCC64X OpenSSL build has no guaranteed CA bundle path.
      // On Windows use the OS trust roots without any Windows API calls.
#if defined(_WIN32)
      if(curl_easy_setopt(handle.get(),CURLOPT_SSL_OPTIONS,
         static_cast<long>(CURLSSLOPT_NATIVE_CA))!=CURLE_OK) {
         throw std::runtime_error{"libcurl cannot use the native CA store; specify a PEM CA bundle"};
      }
#endif
      std::string const bundle=SelectCaBundle(caBundle);
      if(!bundle.empty()) {
         if(!std::filesystem::is_regular_file(std::filesystem::path{bundle}))
            throw std::runtime_error{"CA bundle does not exist: "+bundle};
         if(curl_easy_setopt(handle.get(),CURLOPT_CAINFO,bundle.c_str())!=CURLE_OK)
            throw std::runtime_error{"Cannot configure CA bundle"};
      }
      auto const error=curl_easy_perform(handle.get());
      if(error!=CURLE_OK) {
         std::string const detail=errorBuffer[0]
            ? std::string{errorBuffer.data()} : std::string{curl_easy_strerror(error)};
         if(error==CURLE_PEER_FAILED_VERIFICATION)
            throw std::runtime_error{"TLS trust failed. Check OS trusted CAs or use --ca-bundle / WEATHERCLIENT_CA_BUNDLE (PEM). "+detail};
         throw std::runtime_error{"HTTPS request failed: "+detail};
      }
      long code{};
      curl_easy_getinfo(handle.get(),CURLINFO_RESPONSE_CODE,&code);
      if(code!=200) throw std::runtime_error{std::format("Weather/location provider returned HTTP {}",code)};
      return result;
   }

   [[nodiscard]] inline std::string Escape(std::string_view value) {
      using Handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
      Handle handle{curl_easy_init(), &curl_easy_cleanup};
      if(!handle) throw std::runtime_error{ "URL encoder unavailable" };
      std::string text{value};
      char* const raw = curl_easy_escape(handle.get(), text.c_str(), static_cast<int>(text.size()));
      if(!raw) throw std::runtime_error{ "Cannot encode address" };
      std::string result{raw};
      curl_free(raw);
      return result;
   }

   [[nodiscard]] inline Location ResolveAddress(std::string const& address,std::string_view caBundle = {}) {
      if(address.empty() || address.size() > 300U) {
         throw std::runtime_error{ "Address must contain 1..300 characters" };
      }
      // Nominatim policy: explicit application identity, user-initiated only,
      // max one query per second. Never use it for autocomplete.
      // The public instance is not a bulk geocoder. Serialize searches
      // with a process-wide minimum 1-second interval on the I/O worker.
      static std::mutex geocodingMutex;
      static auto nextRequest=std::chrono::steady_clock::time_point{};
      std::unique_lock guard{geocodingMutex};
      auto const now=std::chrono::steady_clock::now();
      if(now<nextRequest) std::this_thread::sleep_until(nextRequest);
      nextRequest=std::chrono::steady_clock::now()+std::chrono::seconds{1};
      auto const url = "https://nominatim.openstreetmap.org/search?q="
         + Escape(address) + "&format=jsonv2&limit=1";
      return ParseAddressLocation(Json::parse(GetHttps(url,
         "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)",
         caBundle)),address);
   }

   [[nodiscard]] inline Location ResolveIp(std::string_view caBundle = {}) {
      // IP-based location is approximate (city / region only).
      // If one free provider is unavailable, try a second compatible API.
      std::string initialError;
      try {
         auto const json = Json::parse(GetHttps("https://ipapi.co/json/",
            "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)",
            caBundle));
         return ParseIpLocation(json);
      } catch(std::exception const& error) {
         initialError=error.what();
      }
      try {
         auto const json = Json::parse(GetHttps("https://ipwho.is/",
            "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)",
            caBundle));
         if(!json.is_object() || !json.value("success",false))
            throw std::runtime_error{"IP backup provider returned no location"};
         Location result;
         result.label=json.value("city",std::string{"Approximate IP region"});
         result.address=result.label;
         result.latitude=json.at("latitude").get<double>();
         result.longitude=json.at("longitude").get<double>();
         result.approximate=true;
         if(json.contains("timezone") && json["timezone"].is_object())
            result.timezone=GetString(json["timezone"],"id");
         if(!ValidCoordinates(result.latitude,result.longitude))
            throw std::runtime_error{"IP fallback returned invalid coordinates"};
         return result;
      } catch(std::exception const& error) {
         throw std::runtime_error{
            "IP location unavailable (first: "+initialError+
            "; fallback: "+error.what()+"). Use ADDRESS to enter a location."
         };
      }
   }

   [[nodiscard]] inline WeatherData FetchWeather(Location const& location,std::string_view caBundle = {}) {
      if(!ValidCoordinates(location.latitude, location.longitude))
         throw std::runtime_error{ "Cannot fetch weather for invalid coordinates" };
      auto const url = std::format(
         "https://api.open-meteo.com/v1/forecast?"
         "latitude={:.7f}&longitude={:.7f}&timezone=auto"
         "&forecast_days=7"
         "&current=temperature_2m,relative_humidity_2m,apparent_temperature,"
         "pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m,"
         "precipitation,cloud_cover,weather_code,is_day"
         "&hourly=temperature_2m,apparent_temperature,relative_humidity_2m,"
         "precipitation_probability,precipitation,wind_speed_10m,cloud_cover,cape,weather_code"
         "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
         "precipitation_sum,precipitation_probability_max,wind_speed_10m_max,"
         "wind_gusts_10m_max,uv_index_max,sunrise,sunset",
         location.latitude, location.longitude);
      return ParseWeather(Json::parse(GetHttps(url,
         "adecc-Scholar-WeatherClient/1.1 (+https://github.com/adeccscholar/BuildEngine-Tests)",
         caBundle)));
   }

   [[nodiscard]] inline UnitSystem LoadUnits(std::filesystem::path const& file) {
      if(!std::filesystem::is_regular_file(file)) return UnitSystem::Metric;
      std::ifstream input{file,std::ios::binary};
      if(!input) return UnitSystem::Metric;
      auto const json=Json::parse(input,nullptr,false);
      return json.is_object()?ParseUnits(json.value("units",std::string{"metric"})):
         UnitSystem::Metric;
   }

   [[nodiscard]] inline std::optional<Location> LoadLocation(
      std::filesystem::path const& file
   ) {
      if(!std::filesystem::is_regular_file(file)) return std::nullopt;
      std::ifstream input{file, std::ios::binary};
      if(!input) throw std::runtime_error{"Cannot read location JSON"};
      Json json = Json::parse(input);
      Location location{
         .label = json.at("label").get<std::string>(),
         .address = json.at("address").get<std::string>(),
         .timezone = json.value("timezone",std::string{}),
         .latitude = json.at("latitude").get<double>(),
         .longitude = json.at("longitude").get<double>(),
         .approximate = json.value("approximate",false)
      };
      if(!ValidCoordinates(location.latitude,location.longitude) ||
         location.address.size() > 300U)
         throw std::runtime_error{"Invalid saved location"};
      return location;
   }

   inline void SaveUnits(std::filesystem::path const& file,UnitSystem units) {
      Json settings=Json::object();
      if(std::filesystem::is_regular_file(file)) {
         std::ifstream input{file,std::ios::binary};
         if(input) {
            Json const old=Json::parse(input,nullptr,false);
            if(old.is_object()) settings=old;
         }
      }
      settings["units"]=UnitsId(units);
      auto const temporary=std::filesystem::path{file.string()+".tmp"};
      {
         std::ofstream output{temporary,std::ios::binary|std::ios::trunc};
         if(!output) throw std::runtime_error{"Cannot write unit preferences"};
         output<<settings.dump(2)<<'\n';
         output.flush();
         if(!output) throw std::runtime_error{"Cannot flush unit preferences"};
      }
      std::error_code error;
      std::filesystem::rename(temporary,file,error);
      if(error) {
         std::filesystem::remove(file,error);
         error.clear();
         std::filesystem::rename(temporary,file,error);
         if(error) throw std::runtime_error{"Cannot replace unit preferences"};
      }
   }

   inline void SaveLocation(std::filesystem::path const& file, Location const& location,
      UnitSystem units=UnitSystem::Metric) {
      if(!ValidCoordinates(location.latitude,location.longitude))
         throw std::runtime_error{"Invalid location"};
      Json const json{
         {"schema",1},{"units",UnitsId(units)},{"label",location.label},{"address",location.address},
         {"timezone",location.timezone},{"latitude",location.latitude},
         {"longitude",location.longitude},{"approximate",location.approximate}
      };
      auto const temp = std::filesystem::path{file.string() + ".tmp"};
      {
         std::ofstream output{temp,std::ios::binary | std::ios::trunc};
         if(!output) throw std::runtime_error{"Cannot save location"};
         output << json.dump(2) << '\n';
         output.flush();
         if(!output) throw std::runtime_error{"Location JSON write failed"};
      }
      std::error_code error;
      std::filesystem::rename(temp,file,error);
      if(error) {
         std::filesystem::remove(file,error);
         error.clear();
         std::filesystem::rename(temp,file,error);
         if(error) throw std::runtime_error{"Cannot replace saved location JSON"};
      }
   }
} // namespace weather_client
