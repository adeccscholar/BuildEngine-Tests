// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file IrrJson.h
\brief JSON mapping and file-loading declarations for IRR data.

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

#include "FinancialBasics.h"
#include "BoostJsonFrom.h"
#include "BoostJsonTools.h"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace adecc::boost_tools {

/// \brief Tag type for ADL-dispatch of user-defined C++ to JSON conversions.
struct to_json_tag {};

/// \brief Detects whether a type provides an ADL-visible to_json overload.
template <typename ty, typename = void>
struct has_to_json : std::false_type {};

template <typename ty>
struct has_to_json<ty, std::void_t<decltype(
   to_json(std::declval<ty const&>(), to_json_tag{}))>> : std::true_type {};

template <typename ty>
inline constexpr bool has_to_json_v = has_to_json<ty>::value;

/// \brief Converts a supported C++ object into a Boost.JSON value.
template <typename ty>
[[nodiscard]]
boost::json::value to_json(ty const& value) {
   if constexpr (has_to_json_v<ty>) {
      return to_json(value, to_json_tag{});
      }
   else {
      static_assert(sizeof(ty) == 0, "No to_json overload found for this type.");
      }
   }

/// \brief Converts a range of supported objects into a Boost.JSON array.
template <std::ranges::input_range range_ty>
[[nodiscard]]
boost::json::array to_json_array(range_ty const& values) {
   boost::json::array result;
   if constexpr (std::ranges::sized_range<range_ty>) result.reserve(std::ranges::size(values));

   for (auto const& value : values) result.emplace_back(to_json(value));
   return result;
   }

} // namespace adecc::boost_tools

namespace adecc::financial {

/// \brief Identifies whether a JSON file contains one fund or a fund collection.
enum class fund_file_type {
   single_fund,
   multiple_funds
   };

/// \brief Validates that a cash-flow amount is finite and positive.
template <std::floating_point ty>
struct positive_amount_validator {
   static void check(ty value) {
      if (!std::isfinite(value) || value <= ty {})
         throw std::runtime_error(std::format(
            "cashflow value must be finite and positive, got {}", value));
      }
   };

namespace detail {

[[nodiscard]]
boost::json::value ReadJson(std::filesystem::path const& filename);

[[nodiscard]]
constexpr std::string_view ToJsonString(management_fee_basis value) noexcept {
   switch (value) {
      case management_fee_basis::commitment: return "commitment";
      case management_fee_basis::invested_capital: return "invested_capital";
      case management_fee_basis::net_asset_value: return "net_asset_value";
      }
   return "unknown";
   }

[[nodiscard]]
inline std::string ToJsonString(date_ty value) {
   return std::format("{:%F}", std::chrono::sys_days { value });
   }

template <typename ty>
void AddOptional(boost::json::object& obj, std::string_view name, std::optional<ty> const& value) {
   if (value) obj[name] = *value;
   }

template <>
inline void AddOptional(boost::json::object& obj, std::string_view name,
                        std::optional<date_ty> const& value) {
   if (value) obj[name] = ToJsonString(*value);
   }

template <>
inline void AddOptional(boost::json::object& obj, std::string_view name,
                        std::optional<management_fee_basis> const& value) {
   if (value) obj[name] = ToJsonString(*value);
   }

} // namespace detail

template <std::floating_point ty>
void from_json(cashflow<ty>& result, boost::json::object const& obj,
               adecc::boost_tools::from_json_tag) {
   using adecc::boost_tools::get_value;

   result.direction = get_value<payment_direction>(obj, "direction");
   result.category = get_value<payment_category>(obj, "category");
   result.description = get_value<std::string>(obj, "description");
   result.currency = obj.contains("currency")
      ? get_value<std::string>(obj, "currency")
      : std::string { default_currency };
   result.due_date = get_value<date_ty>(obj, "due_date");
   result.actual_date = obj.contains("actual_date")
      ? get_value<date_ty, true>(obj, "actual_date") : std::nullopt;
   result.value_date = obj.contains("value_date")
      ? get_value<date_ty, true>(obj, "value_date") : std::nullopt;
   result.booking_date = obj.contains("booking_date")
      ? get_value<date_ty, true>(obj, "booking_date") : std::nullopt;
   result.value = get_value<ty, false, positive_amount_validator<ty>>(obj, "value");
   }

template <std::floating_point ty>
void from_json(fund<ty>& result, boost::json::object const& obj,
               adecc::boost_tools::from_json_tag) {
   using adecc::boost_tools::from_json_array;
   using adecc::boost_tools::get_value;

   result.name = get_value<std::string>(obj, "name");
   result.currency = obj.contains("currency") ? get_value<std::string>(obj, "currency")
                                              : std::string { default_currency };
   result.fund_type = obj.contains("fund_type")
      ? get_value<std::string>(obj, "fund_type") : std::string {};
   result.fund_stage = obj.contains("fund_stage")
      ? get_value<std::string>(obj, "fund_stage") : std::string {};
   result.vintage_year = obj.contains("vintage_year")
      ? get_value<uint32_t, true>(obj, "vintage_year") : std::nullopt;
   result.first_closing = obj.contains("first_closing")
      ? get_value<date_ty, true>(obj, "first_closing") : std::nullopt;
   result.final_closing = obj.contains("final_closing")
      ? get_value<date_ty, true>(obj, "final_closing") : std::nullopt;
   result.investment_start_date = obj.contains("investment_start_date")
      ? get_value<date_ty, true>(obj, "investment_start_date") : std::nullopt;
   result.investment_end_date = obj.contains("investment_end_date")
      ? get_value<date_ty, true>(obj, "investment_end_date") : std::nullopt;
   result.terms_end_date = obj.contains("terms_end_date")
      ? get_value<date_ty, true>(obj, "terms_end_date") : std::nullopt;
   result.commitment = obj.contains("commitment")
      ? get_value<ty, true>(obj, "commitment") : std::nullopt;
   result.fund_volume = obj.contains("fund_volume")
      ? get_value<ty, true>(obj, "fund_volume") : std::nullopt;
   result.hurdle_rate = obj.contains("hurdle_rate")
      ? get_value<ty, true>(obj, "hurdle_rate") : std::nullopt;
   result.carry_rate = obj.contains("carry_rate")
      ? get_value<ty, true>(obj, "carry_rate") : std::nullopt;
   result.catch_up_rate = obj.contains("catch_up_rate")
      ? get_value<ty, true>(obj, "catch_up_rate") : std::nullopt;
   result.management_fee_base = obj.contains("management_fee_base")
      ? get_value<management_fee_basis, true>(obj, "management_fee_base") : std::nullopt;
   result.management_fee_rate = obj.contains("management_fee_rate")
      ? get_value<ty, true>(obj, "management_fee_rate") : std::nullopt;
   result.management_fee_step_down_date = obj.contains("management_fee_step_down_date")
      ? get_value<date_ty, true>(obj, "management_fee_step_down_date") : std::nullopt;
   result.management_fee_rate_after_step_down = obj.contains("management_fee_rate_after_step_down")
      ? get_value<ty, true>(obj, "management_fee_rate_after_step_down") : std::nullopt;
   result.setup_fee_rate = obj.contains("setup_fee_rate")
      ? get_value<ty, true>(obj, "setup_fee_rate") : std::nullopt;
   result.setup_fee_maximum = obj.contains("setup_fee_maximum")
      ? get_value<ty, true>(obj, "setup_fee_maximum") : std::nullopt;
   result.clawback_applicable = obj.contains("clawback_applicable")
      ? get_value<bool>(obj, "clawback_applicable") : false;
   result.recycling_allowed = obj.contains("recycling_allowed")
      ? get_value<bool>(obj, "recycling_allowed") : false;
   result.recycling_maximum_rate = obj.contains("recycling_maximum_rate")
      ? get_value<ty, true>(obj, "recycling_maximum_rate") : std::nullopt;
   result.leverage_rate = obj.contains("leverage_rate")
      ? get_value<ty, true>(obj, "leverage_rate") : std::nullopt;
   result.late_payment_interest_rate = obj.contains("late_payment_interest_rate")
      ? get_value<ty, true>(obj, "late_payment_interest_rate") : std::nullopt;
   result.default_interest_rate = obj.contains("default_interest_rate")
      ? get_value<ty, true>(obj, "default_interest_rate") : std::nullopt;
   result.valuation_date = obj.contains("valuation_date")
      ? get_value<date_ty, true>(obj, "valuation_date") : std::nullopt;
   result.residual_value = obj.contains("residual_value")
      ? get_value<ty, true>(obj, "residual_value") : std::nullopt;
   result.initial_rate = obj.contains("initial_rate")
      ? get_value<ty, true>(obj, "initial_rate") : std::nullopt;

   if (!obj.contains("cashflows")) throw std::runtime_error("value cashflows does not exist");

   result.cashflows = from_json_array<cashflow<ty>>(obj.at("cashflows"));

   if (result.cashflows.empty())
      throw std::runtime_error(std::format("fund '{}' does not contain cashflows", result.name));

   std::ranges::sort(result.cashflows, {}, &cashflow<ty>::due_date);
   }

template <std::floating_point ty>
void from_json(fund_document<ty>& result, boost::json::object const& obj,
               adecc::boost_tools::from_json_tag) {
   using adecc::boost_tools::from_json_array;

   if (!obj.contains("funds")) throw std::runtime_error("value funds does not exist");

   result.funds = from_json_array<fund<ty>>(obj.at("funds"));
   if (result.funds.empty()) throw std::runtime_error("fund document does not contain funds");
   }

template <std::floating_point ty>
[[nodiscard]]
boost::json::value to_json(cashflow<ty> const& value, adecc::boost_tools::to_json_tag) {
   boost::json::object result {
      { "direction", std::string { value.direction == payment_direction::inflow ? "inflow" : "outflow" } },
      { "category", [&] {
           switch (value.category) {
              case payment_category::capital_call: return "capital_call";
              case payment_category::distribution: return "distribution";
              case payment_category::fee: return "fee";
              case payment_category::management_fee: return "management_fee";
              case payment_category::carried_interest: return "carried_interest";
              case payment_category::catch_up: return "catch_up";
              case payment_category::equalisation_payment: return "equalisation_payment";
              case payment_category::late_payment_interest: return "late_payment_interest";
              case payment_category::late_payment_fee: return "late_payment_fee";
              case payment_category::valuation: return "valuation";
              case payment_category::correction: return "correction";
              case payment_category::clawback: return "clawback";
              case payment_category::other: return "other";
              }
           return "other";
           }() },
      { "description", value.description },
      { "currency", value.currency },
      { "due_date", detail::ToJsonString(value.due_date) },
      { "value", value.value }
      };

   detail::AddOptional(result, "actual_date", value.actual_date);
   detail::AddOptional(result, "value_date", value.value_date);
   detail::AddOptional(result, "booking_date", value.booking_date);
   return result;
   }

template <std::floating_point ty>
[[nodiscard]]
boost::json::value to_json(fund<ty> const& value, adecc::boost_tools::to_json_tag) {
   boost::json::object result {
      { "name", value.name },
      { "currency", value.currency },
      { "fund_type", value.fund_type },
      { "fund_stage", value.fund_stage },
      { "clawback_applicable", value.clawback_applicable },
      { "recycling_allowed", value.recycling_allowed },
      { "cashflows", adecc::boost_tools::to_json_array(value.cashflows) }
      };

   detail::AddOptional(result, "vintage_year", value.vintage_year);
   detail::AddOptional(result, "first_closing", value.first_closing);
   detail::AddOptional(result, "final_closing", value.final_closing);
   detail::AddOptional(result, "investment_start_date", value.investment_start_date);
   detail::AddOptional(result, "investment_end_date", value.investment_end_date);
   detail::AddOptional(result, "terms_end_date", value.terms_end_date);
   detail::AddOptional(result, "commitment", value.commitment);
   detail::AddOptional(result, "fund_volume", value.fund_volume);
   detail::AddOptional(result, "hurdle_rate", value.hurdle_rate);
   detail::AddOptional(result, "carry_rate", value.carry_rate);
   detail::AddOptional(result, "catch_up_rate", value.catch_up_rate);
   detail::AddOptional(result, "management_fee_base", value.management_fee_base);
   detail::AddOptional(result, "management_fee_rate", value.management_fee_rate);
   detail::AddOptional(result, "management_fee_step_down_date", value.management_fee_step_down_date);
   detail::AddOptional(result, "management_fee_rate_after_step_down", value.management_fee_rate_after_step_down);
   detail::AddOptional(result, "setup_fee_rate", value.setup_fee_rate);
   detail::AddOptional(result, "setup_fee_maximum", value.setup_fee_maximum);
   detail::AddOptional(result, "recycling_maximum_rate", value.recycling_maximum_rate);
   detail::AddOptional(result, "leverage_rate", value.leverage_rate);
   detail::AddOptional(result, "late_payment_interest_rate", value.late_payment_interest_rate);
   detail::AddOptional(result, "default_interest_rate", value.default_interest_rate);
   detail::AddOptional(result, "valuation_date", value.valuation_date);
   detail::AddOptional(result, "residual_value", value.residual_value);
   detail::AddOptional(result, "initial_rate", value.initial_rate);
   return result;
   }

template <std::floating_point ty>
[[nodiscard]]
boost::json::value to_json(fund_document<ty> const& value, adecc::boost_tools::to_json_tag) {
   return boost::json::object {
      { "funds", adecc::boost_tools::to_json_array(value.funds) }
      };
   }

template <std::floating_point ty = double>
[[nodiscard]]
fund<ty> ReadFund(std::filesystem::path const& filename) {
   try {
      return adecc::boost_tools::from_json<fund<ty>>(detail::ReadJson(filename));
      }
   catch (std::exception const& error) {
      throw std::runtime_error(std::format(
         "invalid fund document '{}': {}", filename.string(), error.what()));
      }
   }

template <std::floating_point ty = double>
[[nodiscard]]
fund_document<ty> ReadFundDocument(std::filesystem::path const& filename) {
   try {
      return adecc::boost_tools::from_json<fund_document<ty>>(detail::ReadJson(filename));
      }
   catch (std::exception const& error) {
      throw std::runtime_error(std::format(
         "invalid multi-fund document '{}': {}", filename.string(), error.what()));
      }
   }

template <std::floating_point ty>
void WriteFund(std::filesystem::path const& filename, fund<ty> const& value) {
   std::ofstream output { filename, std::ios::binary | std::ios::trunc };
   if (!output)
      throw std::runtime_error(std::format("file '{}' could not be opened", filename.string()));

   output << boost::json::serialize(adecc::boost_tools::to_json(value));
   if (!output)
      throw std::runtime_error(std::format("file '{}' could not be written", filename.string()));
   }

template <std::floating_point ty>
void WriteFundDocument(std::filesystem::path const& filename, fund_document<ty> const& value) {
   std::ofstream output { filename, std::ios::binary | std::ios::trunc };
   if (!output)
      throw std::runtime_error(std::format("file '{}' could not be opened", filename.string()));

   output << boost::json::serialize(adecc::boost_tools::to_json(value));
   if (!output)
      throw std::runtime_error(std::format("file '{}' could not be written", filename.string()));
   }

} // namespace adecc::financial

namespace adecc::boost_tools {

template <>
struct value_converter<adecc::financial::payment_direction> {
   static adecc::financial::payment_direction convert(boost::json::value const& value) {
      auto const text = value_converter<std::string>::convert(value);
      if (text == "inflow") return adecc::financial::payment_direction::inflow;
      if (text == "outflow") return adecc::financial::payment_direction::outflow;
      throw std::runtime_error(std::format("unknown payment direction '{}'", text));
      }
   };

template <>
struct value_converter<adecc::financial::payment_category> {
   static adecc::financial::payment_category convert(boost::json::value const& value) {
      using category_ty = adecc::financial::payment_category;
      using entry_ty = std::pair<std::string_view, category_ty>;

      static constexpr std::array entries {
         entry_ty { "capital_call",          category_ty::capital_call },
         entry_ty { "distribution",          category_ty::distribution },
         entry_ty { "fee",                   category_ty::fee },
         entry_ty { "management_fee",        category_ty::management_fee },
         entry_ty { "carried_interest",      category_ty::carried_interest },
         entry_ty { "catch_up",              category_ty::catch_up },
         entry_ty { "equalisation_payment",  category_ty::equalisation_payment },
         entry_ty { "late_payment_interest", category_ty::late_payment_interest },
         entry_ty { "late_payment_fee",      category_ty::late_payment_fee },
         entry_ty { "valuation",             category_ty::valuation },
         entry_ty { "correction",            category_ty::correction },
         entry_ty { "clawback",              category_ty::clawback },
         entry_ty { "other",                 category_ty::other }
         };

      auto const text = value_converter<std::string>::convert(value);
      auto const iter = std::ranges::find(entries, std::string_view { text }, &entry_ty::first);
      if (iter == entries.end())
         throw std::runtime_error(std::format("unknown payment category '{}'", text));
      return iter->second;
      }
   };

template <>
struct value_converter<adecc::financial::management_fee_basis> {
   static adecc::financial::management_fee_basis convert(boost::json::value const& value) {
      using basis_ty = adecc::financial::management_fee_basis;
      using entry_ty = std::pair<std::string_view, basis_ty>;

      static constexpr std::array entries {
         entry_ty { "commitment",       basis_ty::commitment },
         entry_ty { "invested_capital", basis_ty::invested_capital },
         entry_ty { "net_asset_value",  basis_ty::net_asset_value }
         };

      auto const text = value_converter<std::string>::convert(value);
      auto const iter = std::ranges::find(entries, std::string_view { text }, &entry_ty::first);
      if (iter == entries.end())
         throw std::runtime_error(std::format("unknown management fee basis '{}'", text));
      return iter->second;
      }
   };

} // namespace adecc::boost_tools