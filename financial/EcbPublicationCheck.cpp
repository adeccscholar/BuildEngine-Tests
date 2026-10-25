// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * \file EcbPublicationCheck.cpp
 * \brief Implements the ad-hoc ECB publication-day check.
 */
#include "FinancialBasics.h"
#include "EcbPublicationCheck.h"

#include "OpenHolidaysQuery.h"

#include <chrono>
#include <memory>
#include <print>
#include <ranges>

namespace adecc::financial::ecb {


namespace details {

/**
 * \brief Yields calendar days in descending order.
 *
 * \param first First and therefore newest day to yield.
 * \param last  Last and therefore oldest day to yield.
 */
[[nodiscard]]
std::vector<std::chrono::sys_days> daysBackward(std::chrono::sys_days first, std::chrono::sys_days last) {
   using namespace std::chrono;

   std::vector<sys_days> result;
   if (first < last) return result;

   auto const count = (first - last).count() + 1;
   result.reserve(static_cast<std::size_t>(count));
   for (auto current = first; current >= last; current -= days { 1 }) result.push_back(current);
   return result;
   }

[[nodiscard]]
bool isWeekend(date_ty date) {
   auto const weekday = std::chrono::weekday { std::chrono::sys_days { date } };
   return weekday == std::chrono::Saturday || weekday == std::chrono::Sunday;
   }

[[nodiscard]]
fixedClosedDay const* findFixedClosedDay(date_ty date) {
   auto const position = std::ranges::find_if(fixedClosedDays, 
                                              [date](fixedClosedDay const& value)  {
                                                      return date.day() == value.day && date.month() == value.month;
                                                      });
   if (position == fixedClosedDays.end()) return nullptr;
   return std::addressof(*position);
   }

void printTime(std::ostream& output, std::chrono::minutes value) {
   auto const hours = std::chrono::duration_cast<std::chrono::hours>(value);
   auto const minutes = value - hours;
   std::print(output, "{:02}:{:02}", hours.count(), minutes.count());
   }


} // namespace details


} // namespace adecc::ecb
