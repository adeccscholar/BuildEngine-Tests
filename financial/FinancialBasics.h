// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbBasics.h
\brief Basics ECB exchange-rate history.

\details Types for Financial Calculations
*/
#pragma once

#include "BoostJsonTools.h"
#include <string_view>
#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace adecc::financial {

   using date_ty = boost_tools::date_ty;

   inline constexpr std::string_view default_currency{ "EUR" };

   enum class day_count_convention : uint32_t {
      actual_365_fixed,
      actual_360,
      actual_actual
      };

   enum class payment_direction : uint32_t {
      outflow,
      inflow
      };

   enum class payment_category : uint32_t {
      capital_call,
      distribution,
      fee,
      management_fee,
      carried_interest,
      catch_up,
      equalisation_payment,
      late_payment_interest,
      late_payment_fee,
      valuation,
      correction,
      clawback,
      other
      };

   enum class management_fee_basis : uint32_t {
      commitment,
      invested_capital,
      net_asset_value
      };


   /// \brief Represents one dated cash flow used for IRR and related calculations.
   template <std::floating_point ty>
   struct cashflow {
      payment_direction direction{};            //< Defines whether the cash flow is an inflow or outflow.
      payment_category category{};              //< Classifies the economic purpose of the cash flow.
      std::string description;                  //< Provides a short human-readable description.
      std::string currency{ default_currency }; //< Identifies the original currency of the cash flow.
      date_ty due_date{};                       //< Stores the contractually expected payment date.
      std::optional<date_ty> actual_date;       //< Stores the actual payment date when it differs from the due date.
      std::optional<date_ty> value_date;        //< Stores the value date relevant for interest and FX conversion.
      std::optional<date_ty> booking_date;      //< Stores the accounting booking date when available.
      ty value{};                               //< Stores the unsigned cash-flow amount.

      [[nodiscard]]
      ty SignedValue() const noexcept {
         return direction == payment_direction::inflow ? value : -value;
         }
      };

   /// \brief Describes one fund together with its contractual terms and cash flows.
   template <std::floating_point ty>
   struct fund {
      std::string name;                                        //< Stores the descriptive fund name.
      std::string currency{ default_currency };                //< Defines the fund currency used for normalized values.
      std::string fund_type;                                   //< Describes the fund strategy or product type.
      std::string fund_stage;                                  //< Describes the current lifecycle stage of the fund.
      std::optional<uint32_t> vintage_year;                    //< Identifies the fund vintage year when known.
      std::optional<date_ty> first_closing;                    //< Stores the date of the first closing.
      std::optional<date_ty> final_closing;                    //< Stores the date of the final closing.
      std::optional<date_ty> investment_start_date;            //< Stores the start of the investment period.
      std::optional<date_ty> investment_end_date;              //< Stores the current or contractual end of the investment period.
      std::optional<date_ty> terms_end_date;                   //< Stores the current contractual end of the fund term.
      std::optional<ty> commitment;                            //< Stores the committed capital in the fund currency.
      std::optional<ty> fund_volume;                           //< Stores the total fund volume in the fund currency.
      std::optional<ty> hurdle_rate;                           //< Stores the preferred-return hurdle as a decimal rate.
      std::optional<ty> carry_rate;                            //< Stores the carried-interest participation as a decimal rate.
      std::optional<ty> catch_up_rate;                         //< Stores the catch-up participation as a decimal rate.
      std::optional<management_fee_basis> management_fee_base; //< Defines the economic base used for management fees.
      std::optional<ty> management_fee_rate;                   //< Stores the initial annual management-fee rate.
      std::optional<date_ty> management_fee_step_down_date;    //< Stores the date on which the management fee changes.
      std::optional<ty> management_fee_rate_after_step_down;   //< Stores the reduced annual management-fee rate.
      std::optional<ty> setup_fee_rate;                        //< Stores the setup-fee rate as a decimal rate.
      std::optional<ty> setup_fee_maximum;                     //< Limits the setup fee to a maximum amount.
      bool clawback_applicable{};                              //< Indicates whether a clawback provision applies.
      bool recycling_allowed{};                                //< Indicates whether returned capital may be reinvested.
      std::optional<ty> recycling_maximum_rate;                //< Limits recycling as a share of the commitment.
      std::optional<ty> leverage_rate;                         //< Describes leverage relative to the fund capital.
      std::optional<ty> late_payment_interest_rate;            //< Stores the annual interest rate for late capital payments.
      std::optional<ty> default_interest_rate;                 //< Stores the annual interest rate applied after default.
      std::optional<date_ty> valuation_date;                   //< Stores the valuation date for the residual value.
      std::optional<ty> residual_value;                        //< Stores the unrealized residual value in the fund currency.
      std::optional<ty> initial_rate;                          //< Stores the optional starting value for direct IRR calculation.
      std::vector<cashflow<ty>> cashflows;                     //< Contains the complete dated cash-flow sequence.

      [[nodiscard]]
      constexpr ty InitialRate() const noexcept {
         return initial_rate.value_or(static_cast<ty>(0.1));
         }
      };

   /// \brief Groups all funds contained in one self-contained input document.
   template <std::floating_point ty>
   struct fund_document {
      std::vector<fund<ty>> funds; //< Contains all funds defined by the document.
   };

   /// \brief Selects the contractual due date of a cash flow.
   struct due_date_selector {
      template <std::floating_point ty>
      [[nodiscard]]
      constexpr date_ty operator()(cashflow<ty> const& value) const noexcept {
         return value.due_date;
         }
     };

   /// \brief Selects the actual payment date and falls back to the due date.
   struct actual_date_selector {
      template <std::floating_point ty>
      [[nodiscard]]
      constexpr date_ty operator()(cashflow<ty> const& value) const noexcept {
         return value.actual_date.value_or(value.due_date);
         }
      };

   /// \brief Selects the value date and falls back to the actual payment date.
   struct value_date_selector {
      template <std::floating_point ty>
      [[nodiscard]]
      constexpr date_ty operator()(cashflow<ty> const& value) const noexcept {
         return value.value_date.value_or(actual_date_selector{}(value));
         }
      };

   /// \brief Selects the booking date and falls back to the due date.
   struct booking_date_selector {
      template <std::floating_point ty>
      [[nodiscard]]
      constexpr date_ty operator()(cashflow<ty> const& value) const noexcept {
         return value.booking_date.value_or(value.due_date);
      }
   };

   template <typename selector_ty, typename ty>
   concept cashflow_date_selector =
      std::floating_point<ty> &&
      std::invocable<selector_ty const&, cashflow<ty> const&>&&
      std::same_as<std::invoke_result_t<selector_ty const&, cashflow<ty> const&>, date_ty>;





   [[nodiscard]]
   constexpr std::string_view ToString(payment_direction value) noexcept {
      switch (value) {
      case payment_direction::outflow: return "outflow";
      case payment_direction::inflow: return "inflow";
      }
      return "unknown";
   }

   [[nodiscard]]
   constexpr std::string_view ToString(payment_category value) noexcept {
      switch (value) {
      case payment_category::capital_call: return "capital call";
      case payment_category::distribution: return "distribution";
      case payment_category::fee: return "fee";
      case payment_category::management_fee: return "management fee";
      case payment_category::carried_interest: return "carried interest";
      case payment_category::catch_up: return "catch-up";
      case payment_category::equalisation_payment: return "equalisation";
      case payment_category::late_payment_interest: return "late interest";
      case payment_category::late_payment_fee: return "late fee";
      case payment_category::valuation: return "valuation";
      case payment_category::correction: return "correction";
      case payment_category::clawback: return "clawback";
      case payment_category::other: return "other";
      }
      return "unknown";
   }

} // end of namespace adecc::financial

namespace std {

   /// \brief Formats a payment direction as a readable string.
   template <>
   struct formatter<adecc::financial::payment_direction, char> : formatter<string_view, char> {
      template <typename format_context_ty>
      auto format(adecc::financial::payment_direction value, format_context_ty& context) const {
         return formatter<string_view, char>::format(adecc::financial::ToString(value), context);
      }
   };

   /// \brief Formats a payment category as a readable string.
   template <>
   struct formatter<adecc::financial::payment_category, char> : formatter<string_view, char> {
      template <typename format_context_ty>
      auto format(adecc::financial::payment_category value, format_context_ty& context) const {
         return formatter<string_view, char>::format(adecc::financial::ToString(value), context);
      }
   };

   /// \brief Formats a cash flow as one compact table row.
   template <floating_point ty>
   struct formatter<adecc::financial::cashflow<ty>, char> {
      constexpr auto parse(format_parse_context& context) { return context.begin(); }

      template <typename format_context_ty>
      auto format(adecc::financial::cashflow<ty> const& value, format_context_ty& context) const {
         return format_to(context.out(), "{:%d.%m.%Y}  {:>14.2f}  {:<3}  {:<7}  {:<16}  {}",
            chrono::sys_days{ value.due_date }, value.SignedValue(), value.currency,
            value.direction, value.category, value.description);
      }
   };

} // namespace std
