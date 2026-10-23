// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
 * \file OpenHolidaysQuery.h
 * \brief Ad-hoc public-holiday queries against the OpenHolidays API.
 */
#pragma once

#include "FinancialBasics.h"  // für ostream concept, später verlagern

#include "FinancialModel.h"

#include "HttpsClient.h"
#include "CommonTools.h"

#include <boost/json.hpp>

#include <ostream>
#include <set>
#include <string>

namespace adecc::holidays {

   using date_ty = boost_tools::date_ty;


   namespace details {

      date_ty parseIsoDate(std::string_view text);
      bool isFullDayPublicHoliday(boost::json::object const& object);

    } // end of namespace details

/**
 * Requests full-day public holidays for the supplied date interval.
 *
 * The query is intentionally not cached. The ECB reader calls it only for
 * the small interval between the latest cached ECB publication and the
 * current local date.
 */
   template <typename... output_ty>  requires optional_ostream<output_ty...>
   [[nodiscard]]
   std::set<date_ty> QueryPublicHolidays(date_ty firstDate, date_ty lastDate,
      std::string const& subdivisionCode, output_ty&&... outputArgs) {
      constexpr bool hasOutput = hasValidOutput<output_ty...>();
      decltype(auto) output    = optionalOutput(std::forward<output_ty>(outputArgs)...);

      if (lastDate < firstDate) return {};

      auto const target = std::format("/PublicHolidays?countryIsoCode=DE&subdivisionCode={}"
                                      "&languageIsoCode=DE&validFrom={:%F}&validTo={:%F}",
                                       subdivisionCode, std::chrono::sys_days{ firstDate }, std::chrono::sys_days{ lastDate });

      if constexpr (hasOutput) {
         std::println(output, "Holiday check: querying {} from {:%F} through {:%F}", subdivisionCode,
            std::chrono::sys_days{ firstDate }, std::chrono::sys_days{ lastDate });
      }

      adecc::net::HttpsClient client{ "openholidaysapi.org" };
      auto const response = client.Get(target, { { "Accept", "application/json" } });

      if (response.status_code != 200)
         throw std::runtime_error(std::format("OpenHolidays request failed: HTTP {} {}", response.status_code, response.reason));

      auto const document = boost::json::parse(response.body);
      if (!document.is_array()) throw std::runtime_error("OpenHolidays response is not a JSON array");

      std::set<date_ty> result;

      for (auto const& value : document.as_array()) {
         if (!value.is_object()) continue;

         auto const& object = value.as_object();
         if (!details::isFullDayPublicHoliday(object)) continue;

         auto const* startDate = object.if_contains("startDate");
         auto const* endDate = object.if_contains("endDate");
         if (!startDate || !startDate->is_string() || !endDate || !endDate->is_string())
            throw std::runtime_error("OpenHolidays entry has no valid date range");

         auto current = std::chrono::sys_days{ details::parseIsoDate(startDate->as_string()) };
         auto const end = std::chrono::sys_days{ details::parseIsoDate(endDate->as_string()) };

         for (; current <= end; current += std::chrono::days{ 1 }) {
            date_ty const date{ current };
            if (date >= firstDate && date <= lastDate) result.insert(date);
         }
      }

      if constexpr (hasOutput) {
         std::println(output, "Holiday check: {} full-day public holiday(s) found", result.size());
      }
      return result;
   }

} // namespace irr_example::holidays
