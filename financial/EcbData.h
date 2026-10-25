// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbData.h
\brief Loads and conditionally refreshes the ECB exchange-rate history.

\details
The typed XML parsing remains in this header. Therefore the application
instantiates only the floating-point representation selected by main.cpp.
There are no extern-template declarations and no explicit template
instantiations.

Before an ECB download is attempted, the implementation checks only the dates
between the latest cached ECB publication and the current local date. Weekends
are handled locally and full-day public holidays in Hesse are queried ad hoc
from OpenHolidays. No annual holiday calendar is loaded or cached.
*/
#pragma once

#include "FinancialBasics.h"

#include "CurrencyExchange.h"
#include "EcbHistoryCache.h"
#include "EcbHttpsDownload.h"
#include "EcbPublicationCheck.h"

#include "BoostJsonTools.h"

#include <chrono>
#include <concepts>
#include <filesystem>
#include <ostream>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <functional>
#include <tuple>
#include <variant>
#include <utility>

#if defined(__BORLANDC__) && defined(_WIN32)
# include <windows.h>
#endif

namespace adecc::financial::ecb {

namespace detail {

struct localClockValue {
   date_ty date {};
   std::chrono::minutes timeOfDay {};
   };

[[nodiscard]]
inline localClockValue localNow() {
   using namespace std::chrono;

#if defined(__BORLANDC__) && defined(_WIN32)
   // BCC64X 7.80 currently ships without the C++20 chrono time-zone types
   // (std::zoned_time/current_zone). For the local wall-clock value needed
   // here, use the native Windows API and immediately convert back to the
   // standard chrono date/minutes representation used by the rest of the demo.
   SYSTEMTIME st {};
   ::GetLocalTime(&st);

   return {
      .date = date_ty { year { static_cast<int>(st.wYear) },
                        month { static_cast<unsigned>(st.wMonth) },
                        day { static_cast<unsigned>(st.wDay) } },
      .timeOfDay = hours { st.wHour } + minutes { st.wMinute }
      };
#else
   auto const localTime = zoned_time { current_zone(), system_clock::now() }
         .get_local_time();
   auto const localDay = floor<days>(localTime);

   return {
      .date = date_ty { localDay },
      .timeOfDay = floor<minutes>(localTime - localDay)
      };
#endif
   }

inline void validateEcbDocument(std::string_view xml) {
   constexpr std::string_view expected { "<gesmes:Envelope" };
   if (xml.find(expected) == std::string_view::npos)
      throw std::runtime_error("Cached ECB response is not the expected XML document.");
   }

template <std::floating_point ty>
[[nodiscard]]
currency::exchange_rate_history<ty> parseHistory(std::string_view xml)
{
   validateEcbDocument(xml);
   return currency::ReadExchangeRateHistory<ty>(xml);
}

template <std::floating_point ty, typename... output_ty>  requires optional_ostream<output_ty...>
[[nodiscard]]
currency::exchange_rate_history<ty> downloadInitialHistory(std::filesystem::path const& cacheFile, output_ty&&... outputArgs) {
   constexpr bool hasOutput = hasValidOutput<output_ty...>();
   decltype(auto) output = optionalOutput(std::forward<output_ty>(outputArgs)...);

   if constexpr (hasOutput) {
      std::println(output, "ECB XML source: download; no cache exists");
      }

   auto xml = DownloadEcbHistoryXml();
   auto history = parseHistory<ty>(xml);
   ReplaceCachedEcbHistoryXml(cacheFile, xml);


   if constexpr (hasOutput) {
      std::println(output, "ECB XML cache updated through {:%F}", std::chrono::sys_days{ history.rbegin()->first });
      }

   return history;
}

template <std::floating_point ty, typename... output_ty>  requires optional_ostream<output_ty...>
[[nodiscard]]
currency::exchange_rate_history<ty> loadHistory(std::filesystem::path const& cacheFile, std::chrono::minutes publicationTime, 
                                                output_ty&&... outputArgs) {
   constexpr bool hasOutput = hasValidOutput<output_ty...>();
   decltype(auto) output = optionalOutput(std::forward<output_ty>(outputArgs)...);

   if constexpr (hasOutput) {
      std::println(output, "ECB XML cache: {}", cacheFile.string());
      }

   auto cachedXml = ReadCachedEcbHistoryXml(cacheFile);
   if (!cachedXml) {
      if constexpr (hasOutput) {
         return downloadInitialHistory<ty>(cacheFile, output);
         }
      else {
         return downloadInitialHistory<ty>(cacheFile);
         }
      }

   auto cachedHistory = parseHistory<ty>(*cachedXml);
   auto const cachedLatest = cachedHistory.rbegin()->first;

   auto const now = localNow();
   if constexpr (hasOutput) {
      std::println(output, "ECB cached publication: {:%F}", std::chrono::sys_days{ cachedLatest });
      }


   auto const check = [&] {
      if constexpr (hasOutput) {
         return CheckForExpectedPublication(cachedLatest, { .date = now.date, .timeOfDay = now.timeOfDay }, publicationTime, output);
         }
      else {
         return CheckForExpectedPublication(cachedLatest, { .date = now.date, .timeOfDay = now.timeOfDay }, publicationTime);
      }
      }();

   if (!check.publicationExpected) {
      if constexpr (hasOutput) {
         std::println(output, "ECB XML source: existing cache; no open publication day is missing");
         }
      return cachedHistory;
      }

   if (WasPublicationChecked(cacheFile, check.expectedLatestPublication)) {
      if constexpr (hasOutput) {
         std::println(output, "ECB XML source: existing cache; publication {:%F} was already checked",
                      std::chrono::sys_days { check.expectedLatestPublication });
         }
      return cachedHistory;
      }

   try {
      if constexpr (hasOutput) {
         std::println(output, "ECB XML source: downloading missing publication(s)");
         }

      auto downloadedXml = DownloadEcbHistoryXml();
      auto downloadedHistory = parseHistory<ty>(downloadedXml);
      auto const downloadedLatest = downloadedHistory.rbegin()->first;

      MarkPublicationChecked(cacheFile, check.expectedLatestPublication);

      if (downloadedLatest > cachedLatest) {
         ReplaceCachedEcbHistoryXml(cacheFile, downloadedXml);

         if constexpr (hasOutput) {
            std::println(output, "ECB XML cache updated through {:%F}", std::chrono::sys_days{ downloadedLatest });
            }
         return downloadedHistory;
         }

      if constexpr (hasOutput) {
         std::println(output, "ECB XML source: server has no publication newer than the cache");
         }

      return cachedHistory;
      }
   catch (std::exception const& error) {
      try {
         MarkPublicationChecked(cacheFile,  check.expectedLatestPublication);
         }
      catch (...) {
         }

      if constexpr (hasOutput) {
         std::println(output, "ECB XML source: cache fallback after download failure: {}", error.what());
         }
      return cachedHistory;
      }
   }

} // namespace detail

template <std::floating_point ty, typename... output_ty>  requires optional_ostream<output_ty...>
[[nodiscard]]
currency::exchange_rate_history<ty> LoadEcbExchangeRateHistory(std::filesystem::path const& cacheFile, 
                                                               std::chrono::minutes publicationTime, output_ty&&... outputArgs) {
   constexpr bool hasOutput = hasValidOutput<output_ty...>();
   decltype(auto) output = optionalOutput(std::forward<output_ty>(outputArgs)...);

   if constexpr (hasOutput) {
      return detail::loadHistory<ty>(cacheFile, publicationTime, output);
      }
   else return detail::loadHistory<ty>(cacheFile, publicationTime);
   }

template <std::floating_point ty, typename... output_ty>  requires optional_ostream<output_ty...>
[[nodiscard]]
currency::exchange_rate_history<ty> LoadEcbExchangeRateHistory(std::chrono::minutes publicationTime, output_ty&&... outputArgs) {
   constexpr bool hasOutput = hasValidOutput<output_ty...>();
   decltype(auto) output = optionalOutput(std::forward<output_ty>(outputArgs)...);

   if constexpr (hasOutput) {
      return LoadEcbExchangeRateHistory<ty>(DefaultEcbCacheFile(), publicationTime, output);
      }
   else return LoadEcbExchangeRateHistory<ty>(DefaultEcbCacheFile(), publicationTime);
   }

} // namespace irr_example::ecb