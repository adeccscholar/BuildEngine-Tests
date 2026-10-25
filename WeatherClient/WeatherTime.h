// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

/** \file WeatherTime.h
    \brief Wall-clock aligned weather refresh and analog clock geometry.
    \details Pure C++23 helpers, independent from SDL, Skia and networking.
*/
#include <algorithm>
#include <chrono>

namespace weather_client {
   /** \brief Return the next complete quarter-hour boundary.
       \details The epoch-based calculation also lands on local :00, :15,
       :30 and :45 because civil UTC offsets are multiples of 15 minutes.
       The boundary itself is exclusive: at 12:15:00 the result is 12:30:00.
       \param theNow Current system clock instant.
       \returns The next absolute quarter-hour boundary.
       \throw Nothing.
   */
   [[nodiscard]] constexpr std::chrono::system_clock::time_point NextQuarterHour(
      std::chrono::system_clock::time_point theNow
   ) noexcept {
      using namespace std::chrono;
      auto const theMinute=floor<minutes>(theNow);
      auto const iMinutes=theMinute.time_since_epoch().count();
      auto const iRemainder=(iMinutes%15+15)%15;
      return theMinute+minutes{15-iRemainder};
   }

   /** \brief Schedule a successful refresh or a failed-request retry.
       \details A retry after one minute must not skip an earlier fixed
       quarter-hour boundary. All future successful refreshes are aligned
       independently of the duration of the previous HTTPS operation.
       \param theNow Current system clock instant.
       \param bRetry True if the previous request failed.
       \returns The next time a request may start.
       \throw Nothing.
   */
   [[nodiscard]] constexpr std::chrono::system_clock::time_point NextWeatherAttempt(
      std::chrono::system_clock::time_point theNow,bool bRetry
   ) noexcept {
      auto const theBoundary=NextQuarterHour(theNow);
      return bRetry?(std::min)(theBoundary,theNow+std::chrono::minutes{1}):
         theBoundary;
   }

   /** \brief Angles for a conventional clockwise analog clock.
       \details Degrees are measured clockwise from 12 o'clock.
   */
   struct ClockAngles {
      double flHourDegrees{};
      double flMinuteDegrees{};
      double flSecondDegrees{};
   };

   /** \brief Compute continuous analog clock-hand angles.
       \details The hour and minute hands include the fraction elapsed in
       lower-order units, rather than jumping once per hour or minute.
       \param iHour Local clock hour in the range 0..23.
       \param iMinute Local clock minute in the range 0..59.
       \param iSecond Local clock second in the range 0..59.
       \returns Three angles measured clockwise from the 12 o'clock position.
       \throw Nothing.
   */
   [[nodiscard]] constexpr ClockAngles AnalogClockAngles(
      int iHour,int iMinute,int iSecond
   ) noexcept {
      return {
         30.0*(static_cast<double>(iHour%12)+iMinute/60.0+iSecond/3600.0),
         6.0*(static_cast<double>(iMinute)+iSecond/60.0),
         6.0*static_cast<double>(iSecond)
      };
   }
} // namespace weather_client
