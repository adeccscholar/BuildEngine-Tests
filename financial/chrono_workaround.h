// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file chrono_workaround.h
\brief Compatibility helpers for parsing chrono date and time values.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

  \licenseblock{MIT}
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#pragma once

#include <charconv>
#include <chrono>
#include <optional>
#include <string_view>

#define CHRONO_WORK_AROUND

enum class DateFmt { ISO };
enum class DateTimeFmt { ISO_HMS };

inline std::optional<std::chrono::year_month_day> parseDate(
   std::string_view text,
   DateFmt
) {
   if (text.size() != 10 || text[4] != '-' || text[7] != '-')
      return std::nullopt;

   auto parse_part = [text](std::size_t offset, std::size_t count)
      -> std::optional<int> {
      int value {};
      auto const first = text.data() + offset;
      auto const last = first + count;
      auto const [ptr, ec] = std::from_chars(first, last, value);
      if (ec != std::errc {} || ptr != last)
         return std::nullopt;
      return value;
      };

   auto const year = parse_part(0, 4);
   auto const month = parse_part(5, 2);
   auto const day = parse_part(8, 2);
   if (!year || !month || !day)
      return std::nullopt;

   std::chrono::year_month_day result {
      std::chrono::year { *year },
      std::chrono::month { static_cast<unsigned>(*month) },
      std::chrono::day { static_cast<unsigned>(*day) }
      };

   if (!result.ok())
      return std::nullopt;

   return result;
}

inline std::optional<std::chrono::sys_seconds> parseDateTime(
   std::string_view text,
   DateTimeFmt
) {
   if (text.size() < 16 || text[10] != 'T' || text[13] != ':')
      return std::nullopt;

   auto const date = parseDate(text.substr(0, 10), DateFmt::ISO);
   if (!date)
      return std::nullopt;

   auto parse_part = [text](std::size_t offset, std::size_t count)
      -> std::optional<int> {
      int value {};
      auto const first = text.data() + offset;
      auto const last = first + count;
      auto const [ptr, ec] = std::from_chars(first, last, value);
      if (ec != std::errc {} || ptr != last)
         return std::nullopt;
      return value;
      };

   auto const hour = parse_part(11, 2);
   auto const minute = parse_part(14, 2);
   auto const second = text.size() >= 19 ? parse_part(17, 2) : std::optional<int>{0};
   if (!hour || !minute || !second || *hour > 23 || *minute > 59 || *second > 59)
      return std::nullopt;

   return std::chrono::sys_days { *date }
      + std::chrono::hours { *hour }
      + std::chrono::minutes { *minute }
      + std::chrono::seconds { *second };
}
