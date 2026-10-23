// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file CurrencyExchange.h
\brief XML parsing and currency conversion using ECB-style rates.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2026 adecc Systemhaus GmbH

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

#include "FinancialBasics.h"
#include "FinancialModel.h"

#include <pugixml.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <concepts>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace adecc::financial::currency {

   inline constexpr date_ty first_supported_date{
      std::chrono::year { 2007 }, std::chrono::January, std::chrono::day { 1 }
   };

   inline constexpr std::string_view euro_currency{ "EUR" };

   template <std::floating_point ty>
   using daily_exchange_rates = std::map<std::string, ty, std::less<>>;

   template <std::floating_point ty>
   using exchange_rate_history = std::map<date_ty, daily_exchange_rates<ty>>;

   struct exchange_rate_error : std::runtime_error {
      using std::runtime_error::runtime_error;
   };

   namespace detail {

      template <std::floating_point ty>
      [[nodiscard]]
      ty Round(ty value, int decimals) {
         auto factor = static_cast<ty>(1);
         for (auto index = 0; index < decimals; ++index)
            factor *= static_cast<ty>(10);
         return std::round(value * factor) / factor;
      }

      [[nodiscard]]
      inline date_ty ParseIsoDate(std::string_view text) {
         int year{};
         unsigned month{};
         unsigned day{};

         if (text.size() != 10 || text[4] != '-' || text[7] != '-')
            throw exchange_rate_error(std::format("Invalid ECB date: {}", text));

         auto parse = []<typename value_ty>(std::string_view part, value_ty & value) {
            auto const [end, error] = std::from_chars(
               part.data(), part.data() + part.size(), value);
            return error == std::errc{} && end == part.data() + part.size();
         };

         if (!parse(text.substr(0, 4), year) ||
            !parse(text.substr(5, 2), month) ||
            !parse(text.substr(8, 2), day))
            throw exchange_rate_error(std::format("Invalid ECB date: {}", text));

         date_ty const result{
            std::chrono::year { year },
            std::chrono::month { month },
            std::chrono::day { day }
         };

         if (!result.ok())
            throw exchange_rate_error(std::format("Invalid ECB date: {}", text));

         return result;
      }

      template <std::floating_point ty>
      [[nodiscard]]
      ty ParseRate(std::string_view text) {
         ty value{};
         auto const [end, error] = std::from_chars(
            text.data(), text.data() + text.size(), value);

         if (error != std::errc{} || end != text.data() + text.size() ||
            !std::isfinite(value) || value <= ty{})
            throw exchange_rate_error(std::format("Invalid ECB rate: {}", text));

         return value;
      }

      [[nodiscard]]
      inline bool IsCubeNode(pugi::xml_node const& node) {
         return std::string_view{ node.name() }.ends_with("Cube");
      }

   } // namespace detail

   template <std::floating_point ty>
   [[nodiscard]]
   exchange_rate_history<ty> ParseExchangeRateHistory(pugi::xml_document const& document) {

      exchange_rate_history<ty> history;

      auto const envelope = document.document_element();
      pugi::xml_node outer_cube;
      std::ranges::for_each(envelope.children(), [&](pugi::xml_node const& node) {
         if (!outer_cube && detail::IsCubeNode(node) && !node.attribute("time"))
            outer_cube = node;
         });

      if (!outer_cube)
         throw exchange_rate_error("ECB XML does not contain the outer Cube element");

      std::ranges::for_each(outer_cube.children(), [&](pugi::xml_node const& day_node) {
         if (!detail::IsCubeNode(day_node) || !day_node.attribute("time"))
            return;

         auto const date = detail::ParseIsoDate(day_node.attribute("time").value());
         if (date < first_supported_date)
            return;

         daily_exchange_rates<ty> rates;
         rates.emplace(std::string{ euro_currency }, static_cast<ty>(1));

         std::ranges::for_each(day_node.children(), [&](pugi::xml_node const& rate_node) {
            if (!detail::IsCubeNode(rate_node))
               return;

            auto const currency =
               std::string_view{ rate_node.attribute("currency").value() };
            auto const rate_text =
               std::string_view{ rate_node.attribute("rate").value() };

            if (currency.empty() || rate_text.empty())
               return;

            auto const [_, inserted] = rates.emplace(
               std::string{ currency }, detail::ParseRate<ty>(rate_text));

            if (!inserted)
               throw exchange_rate_error(std::format("Duplicate ECB rate for {} on {:%F}", currency, std::chrono::sys_days{ date }));
            });

         auto const [_, inserted] = history.emplace(date, std::move(rates));
         if (!inserted)
            throw exchange_rate_error(std::format("Duplicate ECB publication date: {:%F}", std::chrono::sys_days{ date }));
         });

      if (history.empty())
         throw exchange_rate_error("ECB XML contains no rates from 2007 onward");

      return history;
   }

   template <std::floating_point ty>
   [[nodiscard]]
   exchange_rate_history<ty> ReadExchangeRateHistory(
      std::filesystem::path const& filename) {

      pugi::xml_document document;
      auto const result = document.load_file(filename.string().c_str());
      if (!result)
         throw exchange_rate_error(std::format("Cannot read ECB XML file '{}': {}", filename.string(), result.description()));

      return ParseExchangeRateHistory<ty>(document);
      }

   template <std::floating_point ty>
   [[nodiscard]]
   exchange_rate_history<ty> ReadExchangeRateHistory(std::string_view xml) {
      if (xml.empty()) throw exchange_rate_error("Cannot read empty ECB XML content");

      pugi::xml_document document;
      auto const result = document.load_buffer(xml.data(), xml.size());
      if (!result)
         throw exchange_rate_error(std::format("Can't parse downloaded ECB XML: {} at offset {}", result.description(), result.offset));

      return ParseExchangeRateHistory<ty>(document);
      }

   template <std::floating_point ty>
   struct dated_exchange_rate {
      date_ty publication_date{};
      ty units_per_euro{};
      };

   template <std::floating_point ty>
   [[nodiscard]]
   dated_exchange_rate<ty> FindRateAsOf(exchange_rate_history<ty> const& history, date_ty requested_date, std::string_view currency) {

      if (currency == euro_currency) return { requested_date, static_cast<ty>(1) };

      auto position = history.upper_bound(requested_date);
      while (position != history.begin()) {
         --position;
         if (auto const rate = position->second.find(currency);
            rate != position->second.end())
            return { position->first, rate->second };
        }

      throw exchange_rate_error(std::format("No ECB rate for {} on or before {:%F}", currency, std::chrono::sys_days{ requested_date }));
      }

   enum class conversion_method {
      identity,
      direct_ecb,
      reciprocal,
      triangulated
   };

   template <std::floating_point ty>
   struct currency_conversion {
      ty source_amount{};
      std::string source_currency;
      std::string target_currency;
      date_ty requested_date{};
      date_ty source_rate_date{};
      date_ty target_rate_date{};
      ty source_currency_units_per_euro{};
      ty target_currency_units_per_euro{};
      ty conversion_rate{};
      ty target_amount{};
      conversion_method method{ conversion_method::identity };
      int rate_decimals{};
   };

   template <std::floating_point ty>
   [[nodiscard]]
   currency_conversion<ty> ConvertCurrency(
      exchange_rate_history<ty> const& history,
      ty amount,
      std::string_view source_currency,
      std::string_view target_currency,
      date_ty requested_date) {

      if (!std::isfinite(amount))
         throw exchange_rate_error("Currency amount must be finite");
      if (source_currency.empty() || target_currency.empty())
         throw exchange_rate_error("Currency code must not be empty");

      if (source_currency == target_currency)
         return {
            .source_amount = amount,
            .source_currency = std::string { source_currency },
            .target_currency = std::string { target_currency },
            .requested_date = requested_date,
            .source_rate_date = requested_date,
            .target_rate_date = requested_date,
            .source_currency_units_per_euro = static_cast<ty>(1),
            .target_currency_units_per_euro = static_cast<ty>(1),
            .conversion_rate = static_cast<ty>(1),
            .target_amount = amount,
            .method = conversion_method::identity,
            .rate_decimals = 0
      };

      auto const source = FindRateAsOf(history, requested_date, source_currency);
      auto const target = FindRateAsOf(history, requested_date, target_currency);

      ty conversion_rate{};
      conversion_method method{};
      int rate_decimals{};

      if (source_currency == euro_currency) {
         conversion_rate = target.units_per_euro;
         method = conversion_method::direct_ecb;
         rate_decimals = 0;
      }
      else if (target_currency == euro_currency) {
         conversion_rate = detail::Round(
            static_cast<ty>(1) / source.units_per_euro, 5);
         method = conversion_method::reciprocal;
         rate_decimals = 5;
      }
      else {
         conversion_rate = detail::Round(
            target.units_per_euro / source.units_per_euro, 10);
         method = conversion_method::triangulated;
         rate_decimals = 10;
      }

      return {
         .source_amount = amount,
         .source_currency = std::string { source_currency },
         .target_currency = std::string { target_currency },
         .requested_date = requested_date,
         .source_rate_date = source.publication_date,
         .target_rate_date = target.publication_date,
         .source_currency_units_per_euro = source.units_per_euro,
         .target_currency_units_per_euro = target.units_per_euro,
         .conversion_rate = conversion_rate,
         .target_amount = amount * conversion_rate,
         .method = method,
         .rate_decimals = rate_decimals
      };
   }

   // Production-oriented convenience function: perform the conversion and return
   // only the converted amount. The detailed ConvertCurrency result remains
   // available where audit information is required.
   template <std::floating_point ty>
   [[nodiscard]]
   ty ConvertCurrencyValue(
      exchange_rate_history<ty> const& history,
      ty amount,
      std::string_view source_currency,
      std::string_view target_currency,
      date_ty requested_date) {

      return ConvertCurrency(
         history,
         amount,
         source_currency,
         target_currency,
         requested_date).target_amount;
   }

   template <std::floating_point ty>
   [[nodiscard]]
   cashflow<ty> ConvertCashflow(exchange_rate_history<ty> const& history, cashflow<ty> const& source,
                                std::string_view target_currency) {

      if (target_currency.empty())
         throw exchange_rate_error("Target currency must not be empty");

      auto result = source;
      if (source.currency == target_currency)
         return result;

      auto const valuation_date = value_date_selector{}(source);
      result.value = ConvertCurrencyValue(
         history,
         source.value,
         source.currency,
         target_currency,
         valuation_date);
      result.currency = std::string{ target_currency };
      return result;
   }

   template <std::floating_point ty>
   [[nodiscard]]
   fund<ty> ConvertFundCashflows(
      exchange_rate_history<ty> const& history,
      fund<ty> const& source) {

      if (source.currency.empty())
         throw exchange_rate_error("Fund currency must not be empty");

      fund<ty> result{
         .name = source.name,
         .currency = source.currency
      };
      result.cashflows.reserve(source.cashflows.size());

      std::ranges::transform(
         source.cashflows,
         std::back_inserter(result.cashflows),
         [&](cashflow<ty> const& item) {
            return ConvertCashflow(history, item, source.currency);
         });

      return result;
   }

   /**
    * Creates a virtual fund in an explicitly selected currency.
    *
    * Every source fund is normalized only to its own fund currency before its
    * cashflows are appended. The resulting virtual fund carries the selected
    * currency, but its cashflows may still use the individual source-fund
    * currencies. The normal calculation path subsequently converts those
    * cashflows to the virtual fund currency at their value dates.
    */
   template <std::floating_point ty>
   [[nodiscard]]
   fund<ty> CombineFunds(
      exchange_rate_history<ty> const& history,
      std::span<fund<ty> const> funds,
      std::string_view target_currency,
      std::string name = "Combined funds") {

      if (target_currency.empty())
         throw exchange_rate_error("Combined fund currency must not be empty");
      if (funds.empty())
         throw exchange_rate_error("At least one fund is required for combination");

      fund<ty> result{
         .name = std::move(name),
         .currency = std::string { target_currency }
      };

      std::size_t cashflow_count{};
      std::ranges::for_each(funds, [&](fund<ty> const& current) {
         cashflow_count += current.cashflows.size();
         });
      result.cashflows.reserve(cashflow_count);

      std::ranges::for_each(funds, [&](fund<ty> const& current) {
         auto normalized = ConvertFundCashflows(history, current);
         result.cashflows.insert(result.cashflows.end(), std::make_move_iterator(normalized.cashflows.begin()),
                                 std::make_move_iterator(normalized.cashflows.end()));
         });

      std::ranges::sort(result.cashflows, {}, [](cashflow<ty> const& value) { return value_date_selector{}(value); });

      return result;
   }

} // namespace adecc::irr_example::currency