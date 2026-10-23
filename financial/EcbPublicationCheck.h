// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
 * \file EcbPublicationCheck.h
 * \brief Determines whether an ECB history download is expected to be useful.
 *
 * \details
 * The check searches backwards from the newest eligible publication date to
 * the day after the latest publication contained in the local ECB XML cache.
 * Weekends and recurring locally configured closed days are classified
 * locally. Full-day public holidays in Hesse are queried ad hoc from
 * OpenHolidays for exactly this interval.
 *
 * The current day is also classified before the normal ECB publication time.
 * This makes the cache decision transparent: an open current day before the
 * publication time does not trigger a download, but is reported explicitly.
 */
#pragma once

#include "FinancialBasics.h"
#include "OpenHolidaysQuery.h"
#include "FinancialModel.h"

#include <array>
#include <chrono>
#include <ostream>
#include <string_view>
#include <vector>

namespace adecc::financial::ecb {


/**
 * \brief Recurring locally configured closed day.
 *
 * \details
 * The year is deliberately omitted. Each entry therefore applies to every
 * calendar year. These dates supplement weekends and the holidays returned
 * by OpenHolidays.
 */
struct fixedClosedDay {
   std::chrono::day day {};
   std::chrono::month month {};
   std::string_view description {};
   };

/**
 * \brief Locally configured recurring ECB closed days.
 *
 * \details
 * The list is evaluated before the web-service result. Add further recurring
 * dates here as pairs of day and month.
 */
inline constexpr std::array fixedClosedDays {
   fixedClosedDay {
      .day = std::chrono::day { 9 },
      .month = std::chrono::May,
      .description = "individual closed day, Europe Day"
      }
   };

struct localPublicationTime {
   date_ty date {};
   std::chrono::minutes timeOfDay {};
   };

struct publicationCheckResult {
   date_ty firstUncheckedDate {};
   date_ty currentDate {};
   date_ty lastEligibleDate {};
   date_ty expectedLatestPublication {};
   bool publicationExpected {};
   bool currentOpenDayBeforePublication {};
   };

namespace details {

   std::vector<std::chrono::sys_days> daysBackward(std::chrono::sys_days first, std::chrono::sys_days last);
   bool isWeekend(date_ty date);
   fixedClosedDay const* findFixedClosedDay(date_ty date);
   void printTime(std::ostream& output, std::chrono::minutes value);
} // end of namespace details

/**
 * \brief Checks the cache gap and explains the resulting download strategy.
 *
 * \param output                  Target stream for diagnostic output.
 * \param cachedLatestPublication Latest publication found in the XML cache.
 * \param now                     Current local date and time.
 * \param publicationTime         Normal ECB publication time in local time.
 * \return Classification and the latest publication that should be available.
 *
 * \details
 * The current date is eligible only after the configured publication time.
 * Starting with that date, or with the previous date before publication time,
 * days are produced in descending order. The first open day is the latest
 * publication that should already be available. The search stops immediately
 * at that point.
 */
template <typename... output_ty>  requires optional_ostream<output_ty...>
[[nodiscard]]
publicationCheckResult CheckForExpectedPublication(date_ty cachedLatestPublication, localPublicationTime now,
                                                   std::chrono::minutes publicationTime, output_ty&&... outputArgs) {
   using namespace std::chrono;

   constexpr bool hasOutput = hasValidOutput<output_ty...>();
   decltype(auto) output = optionalOutput(std::forward<output_ty>(outputArgs)...);

   auto const firstUncheckedDay = sys_days { cachedLatestPublication } + days { 1 };
   auto const currentDay        = sys_days { now.date };

   // The current day can only contain a newly expected ECB publication after
   // the configured publication time. Before that time the search therefore
   // starts with the preceding calendar day.
   auto lastEligibleDay = currentDay;
   if (now.timeOfDay < publicationTime) lastEligibleDay -= days { 1 };

   publicationCheckResult result {
      .firstUncheckedDate              = date_ty { firstUncheckedDay },
      .currentDate                     = now.date,
      .lastEligibleDate                = date_ty { lastEligibleDay },
      .expectedLatestPublication       = cachedLatestPublication,
      .publicationExpected             = false,
      .currentOpenDayBeforePublication = now.timeOfDay < publicationTime
      };

   if constexpr (hasOutput) {
      std::println(output, "ECB cache strategy: cached through {:%F}; searching backwards from {:%F}",
                   sys_days { cachedLatestPublication }, lastEligibleDay);
      }

   if (now.timeOfDay < publicationTime) {
      if constexpr (hasOutput) {
         std::print(output, "ECB cache day: {:%F} not yet eligible - current time ", currentDay);
         details::printTime(output, now.timeOfDay);
         std::print(output, ", publication expected after ");
         details::printTime(output, publicationTime);
         std::println(output);
         }
      }

   if (lastEligibleDay < firstUncheckedDay) {
      if constexpr (hasOutput) {
         std::println(output, "ECB cache strategy: existing cache is current; no eligible day lies after the cached publication");
         }
      return result;
      }

   auto const holidays = [&] {
      if constexpr (hasOutput) {
         return holidays::QueryPublicHolidays(date_ty { firstUncheckedDay }, date_ty { lastEligibleDay }, "DE-HE", output);
         }
      else return holidays::QueryPublicHolidays(date_ty { firstUncheckedDay }, date_ty { lastEligibleDay }, "DE-HE");
      }();
                    
   for (auto const current : details::daysBackward(lastEligibleDay, firstUncheckedDay)) {
      date_ty const date { current };

      if (details::isWeekend(date)) {
         if constexpr (hasOutput) {
            std::println(output, "ECB cache day: {:%F} closed - weekend", current);
            }
         continue;
         }

      if (auto const* fixedDay = details::findFixedClosedDay(date)) {
         if constexpr (hasOutput) {
            std::println(output, "ECB cache day: {:%F} closed - {}", current, fixedDay->description);
            }
         continue;
         }

      if (holidays.contains(date)) {
         if constexpr (hasOutput) {
            std::println(output, "ECB cache day: {:%F} closed - public holiday in Hessen", current);
            }
         continue;
         }

      // The first open day found while moving backwards is the newest
      // publication that should already be available. Older days cannot alter
      // the cache decision and therefore need not be examined.
      result.expectedLatestPublication = date;
      result.publicationExpected       = true;

      if constexpr (hasOutput) {
         std::println(output, "ECB cache day: {:%F} open - publication should be available", current);
         std::println(output, "ECB cache strategy: download required; latest expected publication is {:%F}", current);
         }
      return result;
      }

   if constexpr (hasOutput) {
      std::println(output, "ECB cache strategy: keep existing cache; all eligible intervening days are closed");
      }
   return result;
   }

} // namespace irr_example::ecb