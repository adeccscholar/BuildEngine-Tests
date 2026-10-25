// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbCurrencyDemo.h
\brief Declarations for ECB currency conversion tests.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#pragma once

#include "FinancialBasics.h"
#include "CurrencyExchange.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <format>
#include <ostream>
#include <print>
#include <stdexcept>
#include <string_view>

namespace adecc::financial::ecb::demo {

namespace detail {

[[nodiscard]]
constexpr std::string_view ToString(currency::conversion_method method) {
   switch (method) {
   case currency::conversion_method::identity:
      return "identity";
   case currency::conversion_method::direct_ecb:
      return "direct ECB";
   case currency::conversion_method::reciprocal:
      return "reciprocal (5 decimals)";
   case currency::conversion_method::triangulated:
      return "triangulated (10 decimals)";
      }

   return "unknown";
   }

template <std::floating_point ty>
void PrintConversion(std::ostream& output, currency::currency_conversion<ty> const& value) {
   std::println(output, 
      "{:.2f} {} -> {:.2f} {} at {:.10f}; {}; requested {:%F}, rates {:%F}/{:%F}",
      value.source_amount,
      value.source_currency,
      value.target_amount,
      value.target_currency,
      value.conversion_rate,
      ToString(value.method),
      std::chrono::sys_days { value.requested_date },
      std::chrono::sys_days { value.source_rate_date },
      std::chrono::sys_days { value.target_rate_date });
   }

template <std::floating_point ty>
[[nodiscard]]
bool NearlyEqual(
   ty left,
   ty right,
   ty relative_tolerance,
   ty absolute_tolerance) {

   auto const difference = std::abs(left - right);
   auto const scale = std::max(std::abs(left), std::abs(right));
   return difference <= std::max(
      absolute_tolerance,
      relative_tolerance * scale);
   }

template <std::floating_point ty>
void TestReciprocal(
   std::ostream& output,
   currency::exchange_rate_history<ty> const& history,
   date_ty date,
   std::string_view currency_code) {

   auto const direct = currency::ConvertCurrency(
      history, static_cast<ty>(1), "EUR", currency_code, date);
   auto const reciprocal = currency::ConvertCurrency(
      history, static_cast<ty>(1), currency_code, "EUR", date);

   auto const expected = currency::detail::Round(
      static_cast<ty>(1) / direct.conversion_rate, 5);

   if (reciprocal.method != currency::conversion_method::reciprocal ||
       reciprocal.rate_decimals != 5 ||
       reciprocal.conversion_rate != expected) {
      throw std::runtime_error(std::format(
         "Reciprocal test failed for {}/EUR: expected {:.5f}, got {:.5f}",
         currency_code,
         expected,
         reciprocal.conversion_rate));
      }

   std::println(output, 
      "  {:3}/EUR reciprocal: {:.5f}; direct product: {:.10f}",
      currency_code,
      reciprocal.conversion_rate,
      direct.conversion_rate * reciprocal.conversion_rate);
   }

template <std::floating_point ty>
void TestTriangulation(
   std::ostream& output,
   currency::exchange_rate_history<ty> const& history,
   date_ty date,
   std::string_view source_currency,
   std::string_view target_currency) {

   auto const conversion = currency::ConvertCurrency(
      history,
      static_cast<ty>(1),
      source_currency,
      target_currency,
      date);

   auto const source = currency::FindRateAsOf(
      history, date, source_currency);
   auto const target = currency::FindRateAsOf(
      history, date, target_currency);
   auto const expected = currency::detail::Round(
      target.units_per_euro / source.units_per_euro, 10);

   if (conversion.method != currency::conversion_method::triangulated ||
       conversion.rate_decimals != 10 ||
       !NearlyEqual(
          conversion.conversion_rate,
          expected,
          static_cast<ty>(1e-12),
          static_cast<ty>(1e-12))) {
      throw std::runtime_error(std::format(
         "Triangulation test failed for {}/{}: expected {:.10f}, got {:.10f}",
         source_currency,
         target_currency,
         expected,
         conversion.conversion_rate));
      }

   std::println(output, 
      "  {:3}/{:3} triangulated: {:.10f}",
      source_currency,
      target_currency,
      conversion.conversion_rate);
   }

} // namespace detail

template <std::floating_point ty>
void TestEcbExchangeRateHistory(
   std::ostream& output,
   currency::exchange_rate_history<ty> const& history) {

   using namespace std::chrono;

   if (history.empty()) {
      throw std::runtime_error(
         "ECB exchange-rate history must not be empty.");
      }

   auto const& [first_date, first_rates] = *history.begin();
   auto const& [latest_date, latest_rates] = *history.rbegin();

   std::println(output, "\nECB exchange-rate history");
   std::println(output, "  Publication days: {}", history.size());
   std::println(output, "  First date:       {:%F}", sys_days { first_date });
   std::println(output, "  Latest date:      {:%F}", sys_days { latest_date });
   std::println(output, "  First currencies: {}", first_rates.size());
   std::println(output, "  Latest currencies:{}", latest_rates.size());

   for (auto const currency_code : { "EUR", "USD", "GBP", "CHF" }) {
      auto const rate = currency::FindRateAsOf(
         history, latest_date, currency_code);
      std::println(output, 
         "  {:3} per EUR:      {:>14.8f} ({:%F})",
         currency_code,
         rate.units_per_euro,
         sys_days { rate.publication_date });
      }

   std::println(output, "\nLatest-date conversions");
   detail::PrintConversion(output, currency::ConvertCurrency(
      history, static_cast<ty>(1'000), "EUR", "USD", latest_date));
   detail::PrintConversion(output, currency::ConvertCurrency(
      history, static_cast<ty>(1'000), "USD", "EUR", latest_date));
   detail::PrintConversion(output, currency::ConvertCurrency(
      history, static_cast<ty>(1'000), "USD", "GBP", latest_date));
   detail::PrintConversion(output, currency::ConvertCurrency(
      history, static_cast<ty>(1'000), "GBP", "CHF", latest_date));

   std::println(output, "\nProduction value conversion");
   auto const production_value = currency::ConvertCurrencyValue(
      history,
      static_cast<ty>(1'000),
      "USD",
      "GBP",
      latest_date);
   std::println(output, "  1000.00 USD -> {:.2f} GBP", production_value);

   std::println(output, "\nReciprocal-rate tests");
   detail::TestReciprocal(output, history, latest_date, "USD");
   detail::TestReciprocal(output, history, latest_date, "GBP");
   detail::TestReciprocal(output, history, latest_date, "CHF");

   std::println(output, "\nTriangulation tests");
   detail::TestTriangulation(output, history, latest_date, "USD", "GBP");
   detail::TestTriangulation(output, history, latest_date, "GBP", "CHF");
   detail::TestTriangulation(output, history, latest_date, "CHF", "USD");

   auto const weekend = date_ty { year { 2025 }, January, day { 5 } };
   std::println(output, "\nAs-of test for Sunday {:%F}", sys_days { weekend });
   detail::PrintConversion(output, currency::ConvertCurrency(
      history, static_cast<ty>(1'000), "EUR", "USD", weekend));

   try {
      [[maybe_unused]] auto const invalid =
         currency::FindRateAsOf(history, latest_date, "XXX");
      throw std::runtime_error(
         "Unknown-currency test unexpectedly succeeded");
      }
   catch (currency::exchange_rate_error const& error) {
      std::println(output, "\nExpected error test");
      std::println(output, "  {}", error.what());
      }
   }

} // namespace irr_example::ecb::demo