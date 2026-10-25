// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once


#include <string>
#include <concepts>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <ctime>
#include <format>
#include <array>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

using myClock = std::chrono::high_resolution_clock;
using myTimePoint = std::chrono::time_point<std::chrono::system_clock>;

// RAD Studio 13 libc++ does not provide the C++20 time-zone database API.
// Use the Windows CRT's local time conversion, retaining the original local-time semantics.
[[nodiscard]] inline std::tm local_time_of(std::chrono::system_clock::time_point now) {
   auto const raw = std::chrono::system_clock::to_time_t(now);
   std::tm result {};
#if defined(_WIN32)
   localtime_s(&result, &raw);
#else
   localtime_r(&raw, &result);
#endif
   return result;
}

[[nodiscard]] inline std::string get_current_time_and_date(std::chrono::system_clock::time_point now) {
   auto const local = local_time_of(now);
   char strDate[32] {};
   std::strftime(strDate, sizeof(strDate), "%d.%m.%Y %H:%M:%S", &local);
   auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
   return std::format("{},{:03d}", strDate, millis.count());
}

[[nodiscard]] inline std::string get_current_timestamp(std::chrono::system_clock::time_point now) {
   auto const local = local_time_of(now);
   char strDate[32] {};
   std::strftime(strDate, sizeof(strDate), "%Y%m%d%H%M", &local);
   return strDate;
}

[[nodiscard]] inline std::string get_current_time_and_date() {
   return get_current_time_and_date(std::chrono::system_clock::now());
}

[[nodiscard]] inline std::string get_current_timestamp() {
   return get_current_timestamp(std::chrono::system_clock::now());
}

template <typename ty>
   requires std::floating_point<ty>
using  measurements_type = std::vector<std::vector<ty>>;


template <typename ty>
   requires std::floating_point<ty>
struct TTestData {
   std::locale              loc;
   std::string              strTest;
   std::string              strDescription;
   std::vector<std::string> captions;
   std::vector<std::string> captions_short;
   int                      iTestCases;
   fs::path                 strDirectory;
   fs::path                 strProtocol;
   fs::path                 strOverview;
   myTimePoint              start_time;
   myTimePoint              end_time;
   measurements_type<ty>    measurements;
   int                      prec;
   };


template <typename ty>
   requires std::floating_point<ty>
struct Statistical_Data {
   ty minimum = 0.0;
   ty percentile_05 = 0.0;
   ty percentile_10 = 0.0;
   ty mean = 0.0;
   ty median = 0.0;
   ty percentile_90 = 0.0;
   ty percentile_95 = 0.0;
   ty maximum = 0.0;
   ty variance = 0.0;
   ty std_deviation = 0.0;
   ty relative_std_deviation = 0.0;
   ty standard_error = 0.0;
   ty median_absolute_deviation = 0.0;
   std::size_t count = 0u;
};

template <typename ty>
   requires std::floating_point<ty>
auto operator + (Statistical_Data<ty> const a, Statistical_Data<ty> const b) {
   static constexpr auto my_round = [](ty const& val) {
      return std::round(val * 1000000.0) / 1000000.0;
      };
   Statistical_Data<ty> result;
   result.minimum = my_round(my_round(a.minimum) + my_round(b.minimum));
   result.mean = my_round(my_round(a.mean) + my_round(b.mean));
   result.median = my_round(my_round(a.median) + my_round(b.median));
   result.maximum = my_round(my_round(a.maximum) + my_round(b.maximum));
   return result;
}


template <std::floating_point ty>
ty CalculateQuantile(std::vector<ty> const& vecSortedSeries, ty const flQuantile) {
   if (vecSortedSeries.empty()) {
      throw std::invalid_argument("CalculateQuantile: Die Messreihe ist leer.");
      }

   if (flQuantile < ty{ 0.0 } || flQuantile > ty{ 1.0 }) {
      throw std::out_of_range("CalculateQuantile: Das Quantil muss zwischen 0 und 1 liegen.");
      }

   if (vecSortedSeries.size() == 1u) {
      return vecSortedSeries.front();
      }

   ty const flPosition = flQuantile * static_cast<ty>(vecSortedSeries.size() - 1u);

   std::size_t const iLower = static_cast<std::size_t>(std::floor(flPosition));

   std::size_t const iUpper = static_cast<std::size_t>(std::ceil(flPosition));

   if (iLower == iUpper) {
      return vecSortedSeries[iLower];
      }

   ty const flFraction = flPosition - static_cast<ty>(iLower);

   return vecSortedSeries[iLower] + flFraction * (vecSortedSeries[iUpper] - vecSortedSeries[iLower]);
   }


template <std::floating_point ty>
Statistical_Data<ty> CalculateStats(std::vector<ty> vecSeries) {
   if (vecSeries.empty()) {
      throw std::invalid_argument("CalculateStats: Die Messreihe ist leer.");
      }

   Statistical_Data<ty> aResult;

   std::ranges::sort(vecSeries);

   aResult.count   = vecSeries.size();
   aResult.minimum = vecSeries.front();
   aResult.maximum = vecSeries.back();

   aResult.mean = std::accumulate(vecSeries.begin(), vecSeries.end(), ty{ 0.0 }) / static_cast<ty>(aResult.count);

   std::size_t const iHalfSize = aResult.count / 2u;

   if ((aResult.count % 2u) == 1u) {
      aResult.median = vecSeries[iHalfSize];
      }
   else {
      aResult.median = (vecSeries[iHalfSize - 1u] + vecSeries[iHalfSize]) / ty{ 2.0 };
      }

   aResult.percentile_05 = CalculateQuantile(vecSeries, ty{ 0.05 });
   aResult.percentile_10 = CalculateQuantile(vecSeries, ty{ 0.10 });
   aResult.percentile_90 = CalculateQuantile(vecSeries, ty{ 0.90 });
   aResult.percentile_95 = CalculateQuantile(vecSeries, ty{ 0.95 });

   if (aResult.count > 1u) {
      ty const flSquaredDeviationSum = std::transform_reduce(vecSeries.begin(), vecSeries.end(),
                                                             ty{ 0.0 }, std::plus<> {},
                                                             [&aResult](ty const flValue) {
                                                                ty const flDeviation = flValue - aResult.mean;
                                                                return flDeviation * flDeviation;
                                                                });

      aResult.variance = flSquaredDeviationSum / static_cast<ty>(aResult.count - 1u);
      aResult.std_deviation = std::sqrt(aResult.variance);
      aResult.standard_error = aResult.std_deviation / std::sqrt(static_cast<ty>(aResult.count));
      }

   if (aResult.mean != ty{ 0.0 }) {
      aResult.relative_std_deviation = aResult.std_deviation / std::abs(aResult.mean);
      }

   std::vector<ty> vecAbsoluteDeviations;
   vecAbsoluteDeviations.reserve(aResult.count);

   std::ranges::transform(vecSeries, std::back_inserter(vecAbsoluteDeviations),
                          [&aResult](ty const flValue) {
                             return std::abs(flValue - aResult.median);
                             });

   std::ranges::sort(vecAbsoluteDeviations);
   aResult.median_absolute_deviation = CalculateQuantile(vecAbsoluteDeviations, ty{ 0.5 });
   return aResult;
   }


template <typename ty>
   requires std::floating_point<ty>
void WriteStart(std::ostream& out, TTestData<ty> test) {
   out << "\n\nstatistical analysis of the test: " << test.strTest << "\n"
      << test.strDescription << "\n\n"
      << " - directory for test:        " << test.strDirectory << "\n"
      << " - count of test runs:        " << test.iTestCases << "\n"
      << " - start time:                " << get_current_time_and_date(test.start_time) << "\n"s
      << " - detail data saved in file: " << test.strProtocol << "\n"
      << " - overview saved in file:    " << test.strOverview << "\n";

   //out << std::format
}

template <typename ty>
   requires std::floating_point<ty>
void WriteProtocol(std::ostream& out, TTestData<ty> test) {
   out.setf(std::ios::fixed);
   out.setf(std::ios::showpoint);

   out << std::format("{}", *test.captions_short.cbegin());
   for (auto const& caption : std::ranges::subrange(test.captions_short.cbegin() + 1, test.captions_short.cend())) out << std::format(";{}", caption);
   out << std::endl;
   for (auto const& v : test.measurements) {
      out << std::format(test.loc, "{1:.{0}f}", test.prec, *v.cbegin());
      std::ranges::subrange w(v.cbegin() + 1, v.cend());
      for (auto const& val : w) out << std::format(test.loc, ";{1:.{0}f}", test.prec, val);
      out << std::endl;
      }
}

template <typename ty>
   requires std::floating_point<ty>
void WriteStatistic(std::ostream& out, TTestData<ty> const& test) {
   // Use the same column widths for headings, rules and every data row.
   static constexpr std::size_t nameWidth = 44u;
   static constexpr std::size_t countWidth = 5u;
   static constexpr std::size_t valueWidth = 9u;
   static constexpr std::array<std::string_view, 11u> headings = {
      "minimum", "P10", "mean", "median", "P90", "maximum",
      "variance", "std_dev", "rel_std%", "std_error", "MAD"
      };

   static auto const print_line = [](std::ostream& stream) {
      stream << "+-" << std::string(nameWidth, '-') << "-+-"
             << std::string(countWidth, '-') << "-+";
      for (std::size_t i = 0u; i < headings.size(); ++i) {
         stream << "-" << std::string(valueWidth, '-') << "-+";
         }
      stream << '\n';
      };

   static auto const print_caption = [](std::ostream& stream) {
      stream << std::format("| {:<44} | {:>5} |", "", "count");
      for (auto const heading : headings) {
         stream << std::format(" {:>9} |", heading);
         }
      stream << '\n';
      };

   auto const print_row = [&test](std::ostream& stream,
                                  std::string const& name,
                                  std::string const& count,
                                  std::array<ty, 11u> const& values,
                                  std::size_t const numberOfValues) {
      stream << std::format("| {:<44} | {:>5} |", name, count);
      for (std::size_t i = 0u; i < values.size(); ++i) {
         if (i < numberOfValues) {
            if (i == 8u) {
               stream << std::format(" {:>9.2f} |", values[i]);
               }
            else {
               stream << std::format(" {:>9.{}f} |", values[i], test.prec);
               }
            }
         else {
            stream << std::format(" {:>9} |", "");
            }
         }
      stream << '\n';
      };

   auto const print_data = [&print_row](std::ostream& stream,
                                       std::string const& name,
                                       Statistical_Data<ty> const& stats) {
      print_row(stream, name, std::to_string(stats.count), {
         stats.minimum, stats.percentile_10, stats.mean, stats.median,
         stats.percentile_90, stats.maximum, stats.variance,
         stats.std_deviation, stats.relative_std_deviation * ty{ 100.0 },
         stats.standard_error, stats.median_absolute_deviation
         }, 11u);
      };

   auto const print_sum = [&print_row](std::ostream& stream,
                                      std::string const& name,
                                      Statistical_Data<ty> const& stats) {
      print_row(stream, name, "", {
         stats.minimum, stats.percentile_10, stats.mean, stats.median,
         stats.percentile_90, stats.maximum, stats.variance,
         ty{}, ty{}, ty{}, ty{}
         }, 7u);
      };

   out.setf(std::ios::fixed);
   out.setf(std::ios::showpoint);

   WriteStart(out, test);

   out << " - end time:                  "
      << get_current_time_and_date(test.end_time)
      << "\n\n"s;

   print_line(out);
   print_caption(out);
   print_line(out);

   std::vector<std::vector<ty>> vecRandomValues;
   vecRandomValues.reserve(test.captions.size() + 1u);

   for (std::size_t iColumn = 0u; iColumn < test.captions.size() + 1u; ++iColumn) {
      std::vector<ty> vecSeries;
      vecSeries.reserve(test.measurements.size());

      for (std::size_t iMeasurement = 0u; iMeasurement < test.measurements.size(); ++iMeasurement) {
         vecSeries.emplace_back(test.measurements[iMeasurement][iColumn]);
         }

      vecRandomValues.emplace_back(std::move(vecSeries));
      }

   Statistical_Data<ty> aSum;

   for (std::size_t iTest = 0u; auto const& strTestName : test.captions) {
      auto const aStats = CalculateStats(vecRandomValues[iTest++]);
      aSum = aSum + aStats;
      print_data(out, strTestName, aStats);
      }

   auto const aTotalStats = CalculateStats(vecRandomValues[test.captions.size()]);

   print_line(out);
   print_data(out, "total time", aTotalStats);
   print_sum(out, "cumulated time", aSum);
   print_line(out);
}