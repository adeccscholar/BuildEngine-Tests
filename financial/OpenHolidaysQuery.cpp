// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

#include "OpenHolidaysQuery.h"


#include <charconv>
#include <chrono>
#include <format>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace adecc::holidays {

namespace details {

[[nodiscard]]
date_ty parseIsoDate(std::string_view text) {
   if (text.size() != 10 || text[4] != '-' || text[7] != '-')
      throw std::runtime_error(std::format("Invalid ISO date '{}'", text));

   int      yearValue {};
   unsigned monthValue {};
   unsigned dayValue {};

   auto parsePart = [](std::string_view value, auto& result) {
      auto const [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
      return error == std::errc {} && end == value.data() + value.size();
      };

   if (!parsePart(text.substr(0, 4), yearValue) || !parsePart(text.substr(5, 2), monthValue) ||
       !parsePart(text.substr(8, 2), dayValue))
      throw std::runtime_error(std::format("Invalid ISO date '{}'", text));

   date_ty const result {
      std::chrono::year  { yearValue },
      std::chrono::month { monthValue },
      std::chrono::day   { dayValue }
      };

   if (!result.ok()) throw std::runtime_error(std::format("Invalid calendar date '{}'", text));

   return result;
   }

[[nodiscard]]
bool isFullDayPublicHoliday(boost::json::object const& object) {
   auto const* type = object.if_contains("type");
   if (!type || !type->is_string() || type->as_string() != "Public") return false;

   auto const* temporalScope = object.if_contains("temporalScope");
   return !temporalScope || !temporalScope->is_string() || temporalScope->as_string() == "FullDay";
   }

} // namespace details


} // namespace irr_example::holidays
