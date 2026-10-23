// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file FinancialCalculation.h
\brief Financial return calculations and numerical analysis for dated cashflows.

\details The header indicates direct IRR and XIRR calculation, Newton iteration,
interval scanning and bisection, detection of multiple roots, residual and
sensitivity analysis, day-count conventions, periodic cashflow normalization
and annualization of periodic rates. Date selection is expressed as a compile-
time policy. Classic monthly IRR groups all cashflows at the first day of their
calendar month.

\version 1.0
\date    05.08.2026
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

#define FINANCIAL_CALCULATION_API_VERSION 20260805L

#include "FinancialModel.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace adecc::financial {


   enum class irr_period {
      month,
      quarter,
      year
   };

   enum class irr_status {
      unique_root,
      multiple_roots,
      no_root,
      indeterminate,
      invalid_cashflows
   };

   template <std::floating_point ty>
   struct irr_options {
      ty start_value{ static_cast<ty>(0.10L) };
      ty rate_tolerance{ static_cast<ty>(1.0e-12L) };
      ty absolute_residual_tolerance{ static_cast<ty>(1.0e-7L) };
      ty relative_residual_tolerance{ static_cast<ty>(1.0e-12L) };
      ty minimum_derivative{ static_cast<ty>(1.0e-14L) };
      std::size_t maximum_iterations{ 100 };
      ty minimum_rate{ static_cast<ty>(-0.999999999L) };
      ty maximum_rate{ static_cast<ty>(1000.0L) };
   };

   template <std::floating_point ty>
   struct irr_analysis_options {
      irr_options<ty> solver;
      std::size_t scan_intervals{ 512 };
   };

   template <std::floating_point ty>
   struct xirr_options {
      irr_analysis_options<ty> analysis;
      day_count_convention day_count{ day_count_convention::actual_365_fixed };
   };

   template <std::floating_point ty>
   struct classic_irr_options {
      irr_analysis_options<ty> analysis;
      irr_period period{ irr_period::year };
   };

   template <std::floating_point ty>
   struct irr_root {
      ty rate{};
      ty residual{};
      ty relative_residual{};
      ty derivative{};
      ty normalized_derivative{};
      ty rate_sensitivity{};
      std::size_t iterations{};
   };

   enum class irr_error_code {
      invalid_cashflows,
      invalid_start_value,
      invalid_rate,
      derivative_too_small,
      not_finite,
      no_convergence
   };

   struct irr_error {
      irr_error_code code{};
      std::string message;
   };

   template <std::floating_point ty>
   struct irr_terms {
      ty npv{};
      ty derivative{};
      ty absolute_terms{};
   };

   template <typename evaluator_ty, typename ty>
   concept irr_evaluator =
      std::floating_point<ty> &&
      std::invocable<evaluator_ty&, ty>&&
      std::same_as<std::invoke_result_t<evaluator_ty&, ty>, irr_terms<ty>>;

   template <std::floating_point ty>
   struct normalized_cashflows {
      irr_period period{ irr_period::year };
      date_ty first_period_date{};
      std::vector<ty> values;
      std::size_t sign_changes{};
   };

   template <std::floating_point ty>
   struct irr_analysis {
      irr_status status{ irr_status::indeterminate };
      std::vector<irr_root<ty>> roots;
      std::size_t scanned_intervals{};
      std::size_t non_finite_intervals{};
      std::string message;
   };

   template <std::floating_point ty>
   struct xirr_analysis {
      irr_analysis<ty> result;
      day_count_convention day_count{ day_count_convention::actual_365_fixed };
   };

   template <std::floating_point ty>
   struct classic_irr_analysis {
      irr_analysis<ty> result;
      normalized_cashflows<ty> normalized;
   };

   template <std::floating_point ty>
   [[nodiscard]]
   ty PeriodicRate(ty annualRate, irr_period period) {
      auto const periods = period == irr_period::month ? static_cast<ty>(12)
         : period == irr_period::quarter ? static_cast<ty>(4) : static_cast<ty>(1);

      return std::pow(static_cast<ty>(1) + annualRate,  static_cast<ty>(1) / periods) - static_cast<ty>(1);
      }

   namespace detail {

      template <std::floating_point ty>
      [[nodiscard]]
      constexpr ty Abs(ty value) noexcept {
         return value < ty{} ? -value : value;
      }

      template <std::floating_point ty>
      [[nodiscard]]
      ty YearFraction(std::chrono::sys_days from, std::chrono::sys_days to,
         day_count_convention convention) {
         auto const days = static_cast<ty>((to - from).count());
         if (convention == day_count_convention::actual_365_fixed)
            return days / static_cast<ty>(365);
         if (convention == day_count_convention::actual_360)
            return days / static_cast<ty>(360);
         if (from == to)
            return ty{};

         auto current = from;
         ty result{};
         while (current < to) {
            auto const date = std::chrono::year_month_day{ current };
            auto const next_year = std::chrono::sys_days{
               std::chrono::year_month_day { date.year() + std::chrono::years { 1 },
                                            std::chrono::January, std::chrono::day { 1 } } };
            auto const section_end = std::min(to, next_year);
            auto const year_days = date.year().is_leap() ? static_cast<ty>(366)
               : static_cast<ty>(365);
            result += static_cast<ty>((section_end - current).count()) / year_days;
            current = section_end;
         }
         return result;
      }

      [[nodiscard]]
      inline date_ty PeriodStart(date_ty date, irr_period period) {
         auto const year = date.year();
         auto const month = unsigned(date.month());
         if (period == irr_period::month)
            return date_ty{ year, date.month(), std::chrono::day { 1 } };
         if (period == irr_period::quarter) {
            auto const quarter_month = ((month - 1) / 3) * 3 + 1;
            return date_ty{ year, std::chrono::month { quarter_month }, std::chrono::day { 1 } };
         }
         return date_ty{ year, std::chrono::January, std::chrono::day { 1 } };
      }

      [[nodiscard]]
      inline std::int64_t PeriodIndex(date_ty date, irr_period period) {
         auto const period_start = PeriodStart(date, period);
         auto const year = static_cast<std::int64_t>(int(period_start.year()));
         auto const month = static_cast<std::int64_t>(unsigned(period_start.month())) - 1;
         if (period == irr_period::month)
            return year * 12 + month;
         if (period == irr_period::quarter)
            return year * 4 + month / 3;
         return year;
      }

      template <std::floating_point ty>
      [[nodiscard]]
      ty RelativeResidual(irr_terms<ty> const& terms) noexcept {
         if (!std::isfinite(terms.absolute_terms) ||
            terms.absolute_terms <= std::numeric_limits<ty>::min())
            return std::numeric_limits<ty>::infinity();
         return Abs(terms.npv) / terms.absolute_terms;
      }

      template <std::floating_point ty>
      [[nodiscard]]
      bool ResidualReached(irr_terms<ty> const& terms, irr_options<ty> const& options) noexcept {
         return Abs(terms.npv) <= options.absolute_residual_tolerance ||
            RelativeResidual(terms) <= options.relative_residual_tolerance;
      }

      template <std::floating_point ty>
      [[nodiscard]]
      irr_root<ty> MakeRoot(ty rate, irr_terms<ty> const& terms, std::size_t iterations) {
         auto const derivative = Abs(terms.derivative);
         auto const normalized = terms.absolute_terms > ty{} ? derivative / terms.absolute_terms : ty{};
         auto const sensitivity = derivative > std::numeric_limits<ty>::min()
            ? terms.absolute_terms / derivative : std::numeric_limits<ty>::infinity();
         return { rate, terms.npv, RelativeResidual(terms), terms.derivative,
                  normalized, sensitivity, iterations };
      }

      template <std::floating_point ty, irr_evaluator<ty> npv_fy>
      [[nodiscard]]
      std::expected<irr_root<ty>, irr_error> SolveIrrRoot(irr_options<ty> const& options,
         npv_fy&& evaluate_npv) {
         if (!std::isfinite(options.start_value) || options.start_value <= options.minimum_rate ||
            options.start_value > options.maximum_rate)
            return std::unexpected(irr_error{ irr_error_code::invalid_start_value,
               "start value is outside the allowed rate interval" });

         auto rate = options.start_value;
         for (auto const iteration :
         std::views::iota(std::size_t{ 1 }, options.maximum_iterations + 1)) {
            auto const terms = std::invoke(evaluate_npv, rate);
            if (!std::isfinite(terms.npv) || !std::isfinite(terms.derivative) ||
               !std::isfinite(terms.absolute_terms))
               return std::unexpected(irr_error{ irr_error_code::not_finite,
                  "calculation produced a non-finite value" });
            if (ResidualReached(terms, options))
               return MakeRoot(rate, terms, iteration);
            if (Abs(terms.derivative) <= options.minimum_derivative)
               return std::unexpected(irr_error{ irr_error_code::derivative_too_small,
                  "Newton iteration cannot continue because the derivative is too small" });

            auto const next_rate = rate - terms.npv / terms.derivative;
            if (!std::isfinite(next_rate) || next_rate <= options.minimum_rate ||
               next_rate > options.maximum_rate)
               return std::unexpected(irr_error{ irr_error_code::invalid_rate,
                  "Newton iteration left the allowed rate interval" });
            if (Abs(next_rate - rate) <= options.rate_tolerance) {
               auto const final_terms = std::invoke(evaluate_npv, next_rate);
               if (ResidualReached(final_terms, options))
                  return MakeRoot(next_rate, final_terms, iteration);
            }
            rate = next_rate;
         }
         return std::unexpected(irr_error{ irr_error_code::no_convergence,
            "Newton iteration did not converge" });
      }

      template <std::floating_point ty>
      [[nodiscard]]
      bool ValidCashflows(std::span<cashflow<ty> const> cashflows) {
         return cashflows.size() >= 2 &&
            std::ranges::any_of(cashflows, [](cashflow<ty> const& item) {
            return item.direction == payment_direction::inflow && item.value > ty{};
               }) &&
            std::ranges::any_of(cashflows, [](cashflow<ty> const& item) {
                  return item.direction == payment_direction::outflow && item.value > ty{};
               });
      }

      template <std::floating_point ty>
      [[nodiscard]]
      bool ValidCashflows(std::span<ty const> cashflows) {
         return cashflows.size() >= 2 &&
            std::ranges::any_of(cashflows, [](ty value) { return value > ty{}; }) &&
            std::ranges::any_of(cashflows, [](ty value) { return value < ty{}; });
      }

      template <std::floating_point ty>
      [[nodiscard]]
      std::size_t CountSignChanges(std::span<ty const> values) {
         std::size_t result{};
         int previous{};
         for (auto const value : values) {
            auto const current = value > ty{} ? 1 : value < ty{} ? -1 : 0;
            if (current == 0)
               continue;
            if (previous != 0 && current != previous)
               ++result;
            previous = current;
         }
         return result;
      }

      template <std::floating_point ty>
      [[nodiscard]]
      ty ScanRate(ty minimum_rate, ty maximum_rate, std::size_t index, std::size_t intervals) {
         auto const minimum = std::log1p(minimum_rate);
         auto const maximum = std::log1p(maximum_rate);
         auto const position = static_cast<ty>(index) / static_cast<ty>(intervals);
         return std::expm1(minimum + (maximum - minimum) * position);
      }

      template <std::floating_point ty>
      [[nodiscard]]
      bool SameRootRate(ty left, ty right, ty solver_tolerance) noexcept {
         auto const scale = std::max({ static_cast<ty>(1), Abs(left), Abs(right) });
         auto const numerical_tolerance =
            std::sqrt(std::numeric_limits<ty>::epsilon()) * scale;
         auto const requested_tolerance = solver_tolerance * scale;
         return Abs(left - right) <=
            std::max(numerical_tolerance, requested_tolerance);
      }

      template <std::floating_point ty>
      void AddRoot(std::vector<irr_root<ty>>& roots, irr_root<ty> root, ty tolerance) {
         auto const position = std::ranges::find_if(
            roots,
            [&](irr_root<ty> const& current) {
               return SameRootRate(current.rate, root.rate, tolerance);
            });

         if (position == roots.end()) {
            roots.push_back(root);
            return;
         }

         // Keep the numerically better representative when Newton and the scan
         // converge to the same mathematical root.
         if (root.relative_residual < position->relative_residual ||
            (root.relative_residual == position->relative_residual &&
               Abs(root.residual) < Abs(position->residual)))
            *position = root;
      }

      template <std::floating_point ty, irr_evaluator<ty> npv_fy>
      [[nodiscard]]
      std::optional<irr_root<ty>> BisectRoot(ty left, ty right, irr_options<ty> const& options,
         npv_fy&& evaluate_npv) {
         auto left_terms = std::invoke(evaluate_npv, left);
         auto right_terms = std::invoke(evaluate_npv, right);
         if (!std::isfinite(left_terms.npv) || !std::isfinite(right_terms.npv) ||
            left_terms.npv * right_terms.npv > ty{})
            return std::nullopt;

         for (auto const iteration :
         std::views::iota(std::size_t{ 1 }, options.maximum_iterations + 1)) {
            auto const middle = std::midpoint(left, right);
            auto const terms = std::invoke(evaluate_npv, middle);
            if (!std::isfinite(terms.npv))
               return std::nullopt;
            if (ResidualReached(terms, options) || Abs(right - left) <= options.rate_tolerance)
               return MakeRoot(middle, terms, iteration);
            if (left_terms.npv * terms.npv <= ty{}) {
               right = middle;
               right_terms = terms;
            }
            else {
               left = middle;
               left_terms = terms;
            }
         }
         return std::nullopt;
      }

      template <std::floating_point ty, irr_evaluator<ty> npv_fy>
      [[nodiscard]]
      irr_analysis<ty> Analyze(irr_analysis_options<ty> const& options,
         npv_fy&& evaluate_npv) {
         irr_analysis<ty> result;
         result.scanned_intervals = options.scan_intervals;
         if (options.scan_intervals == 0) {
            result.message = "analysis requires at least one scan interval";
            return result;
         }

         auto left = ScanRate(options.solver.minimum_rate, options.solver.maximum_rate,
            std::size_t{}, options.scan_intervals);
         auto left_terms = std::invoke(evaluate_npv, left);
         for (auto const index :
         std::views::iota(std::size_t{ 1 }, options.scan_intervals + 1)) {
            auto const right = ScanRate(options.solver.minimum_rate, options.solver.maximum_rate,
               index, options.scan_intervals);
            auto const right_terms = std::invoke(evaluate_npv, right);
            if (!std::isfinite(left_terms.npv) || !std::isfinite(right_terms.npv))
               ++result.non_finite_intervals;
            else if (ResidualReached(left_terms, options.solver))
               AddRoot(result.roots, MakeRoot(left, left_terms, 0), options.solver.rate_tolerance);
            else if (left_terms.npv * right_terms.npv < ty{})
               if (auto root = BisectRoot(left, right, options.solver, evaluate_npv); root)
                  AddRoot(result.roots, *root, options.solver.rate_tolerance);
            left = right;
            left_terms = right_terms;
         }

         if (auto root = SolveIrrRoot(options.solver, evaluate_npv); root)
            AddRoot(result.roots, *root, options.solver.rate_tolerance);
         std::ranges::sort(result.roots, {}, &irr_root<ty>::rate);

         if (result.roots.size() == 1)
            result.status = irr_status::unique_root;
         else if (result.roots.size() > 1)
            result.status = irr_status::multiple_roots;
         else if (result.non_finite_intervals == options.scan_intervals)
            result.status = irr_status::indeterminate;
         else
            result.status = irr_status::no_root;
         return result;
      }

   } // namespace detail

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   irr_terms<ty> CalculateXirrTerms(std::span<cashflow<ty> const> cashflows,
      std::chrono::sys_days reference_date, ty rate,
      day_count_convention convention) {
      auto const base = static_cast<ty>(1) + rate;
      auto const select_date = date_selector_policy{};
      return std::transform_reduce(cashflows.begin(), cashflows.end(), irr_terms<ty> {},
         [](irr_terms<ty> const& left, irr_terms<ty> const& right) {
            return irr_terms<ty> { left.npv + right.npv, left.derivative + right.derivative,
            left.absolute_terms + right.absolute_terms };
         },
         [reference_date, base, convention, &select_date](cashflow<ty> const& item) {
            auto const years = detail::YearFraction<ty>(reference_date,
               std::chrono::sys_days{ std::invoke(select_date, item) }, convention);
            auto const amount = item.SignedValue();
            auto const discounted = amount / std::pow(base, years);
            return irr_terms<ty> { discounted,
               -years * amount / std::pow(base, years + static_cast<ty>(1)),
               detail::Abs(discounted) };
         });
   }

   template <std::floating_point ty>
   [[nodiscard]]
   irr_terms<ty> CalculateClassicIrrTerms(std::span<ty const> cashflows, ty rate) {
      auto const base = static_cast<ty>(1) + rate;
      irr_terms<ty> result;
      std::ranges::for_each(std::views::iota(std::size_t{}, cashflows.size()),
         [&](std::size_t index) {
            auto const exponent = static_cast<ty>(index);
            auto const discounted = cashflows[index] / std::pow(base, exponent);
            result.npv += discounted;
            result.absolute_terms += detail::Abs(discounted);
            if (index != 0)
               result.derivative -= exponent * cashflows[index] /
               std::pow(base, exponent + static_cast<ty>(1));
         });
      return result;
   }

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   normalized_cashflows<ty> NormalizeCashflows(std::span<cashflow<ty> const> cashflows,
      irr_period period) {
      if (cashflows.empty())
         return { period, {}, {}, 0 };

      auto const select_date = date_selector_policy{};
      auto const dates = cashflows | std::views::transform([&](cashflow<ty> const& item) {
         return detail::PeriodStart(std::invoke(select_date, item), period);
         });
      auto const [first, last] = std::ranges::minmax(dates);
      auto const first_index = detail::PeriodIndex(first, period);
      auto const last_index = detail::PeriodIndex(last, period);
      normalized_cashflows<ty> result{ period, first,
         std::vector<ty>(static_cast<std::size_t>(last_index - first_index + 1), ty {}), 0 };
      std::ranges::for_each(cashflows, [&](cashflow<ty> const& item) {
         auto const date = detail::PeriodStart(std::invoke(select_date, item), period);
         auto const index = detail::PeriodIndex(date, period) - first_index;
         result.values[static_cast<std::size_t>(index)] += item.SignedValue();
         });
      result.sign_changes = detail::CountSignChanges(std::span<ty const> { result.values });
      return result;
   }

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   std::expected<irr_root<ty>, irr_error> SolveXirrRoot(
      std::span<cashflow<ty> const> cashflows, irr_options<ty> const& options,
      day_count_convention convention = day_count_convention::actual_365_fixed) {
      auto const select_date = date_selector_policy{};
      auto const first = std::ranges::min_element(cashflows, {}, [&](cashflow<ty> const& item) {
         return std::invoke(select_date, item);
         });
      auto const reference = std::chrono::sys_days{ std::invoke(select_date, *first) };
      return detail::SolveIrrRoot(options, [&](ty rate) {
         return CalculateXirrTerms<ty, date_selector_policy>(cashflows, reference, rate, convention);
         });
   }

   template <std::floating_point ty>
   [[nodiscard]]
   std::expected<irr_root<ty>, irr_error> SolveClassicIrrRoot(
      std::span<ty const> cashflows, irr_options<ty> const& options) {
      return detail::SolveIrrRoot(options,
         [&](ty rate) { return CalculateClassicIrrTerms(cashflows, rate); });
   }

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   std::expected<ty, irr_error> CalculateXirrValue(
      std::span<cashflow<ty> const> cashflows, irr_options<ty> const& options = {},
      day_count_convention convention = day_count_convention::actual_365_fixed) {
      if (!detail::ValidCashflows(cashflows))
         return std::unexpected(irr_error{ irr_error_code::invalid_cashflows,
            "at least one positive inflow and one positive outflow are required" });
      auto const result = SolveXirrRoot<ty, date_selector_policy>(cashflows, options, convention);
      if (!result)
         return std::unexpected(result.error());
      return result->rate;
   }

   template <std::floating_point ty>
   [[nodiscard]]
   std::expected<ty, irr_error> CalculateClassicIrrValue(
      std::span<ty const> cashflows, irr_options<ty> const& options = {}) {
      if (!detail::ValidCashflows(cashflows))
         return std::unexpected(irr_error{ irr_error_code::invalid_cashflows,
            "at least one positive and one negative period value are required" });
      auto const result = SolveClassicIrrRoot(cashflows, options);
      if (!result)
         return std::unexpected(result.error());
      return result->rate;
   }

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   std::expected<ty, irr_error> CalculateClassicIrrValue(
      std::span<cashflow<ty> const> cashflows, irr_options<ty> const& options,
      irr_period period) {
      auto const normalized = NormalizeCashflows<ty, date_selector_policy>(cashflows, period);
      return CalculateClassicIrrValue(std::span<ty const> { normalized.values }, options);
   }

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   xirr_analysis<ty> AnalyzeXirr(std::span<cashflow<ty> const> cashflows,
      xirr_options<ty> const& options = {}) {
      xirr_analysis<ty> result;
      result.day_count = options.day_count;
      if (!detail::ValidCashflows(cashflows)) {
         result.result.status = irr_status::invalid_cashflows;
         result.result.message = "at least one positive inflow and one positive outflow are required";
         return result;
      }
      auto const select_date = date_selector_policy{};
      auto const first = std::ranges::min_element(cashflows, {}, [&](cashflow<ty> const& item) {
         return std::invoke(select_date, item);
         });
      auto const reference = std::chrono::sys_days{ std::invoke(select_date, *first) };
      result.result = detail::Analyze(options.analysis, [&](ty rate) {
         return CalculateXirrTerms<ty, date_selector_policy>(cashflows, reference, rate,
            options.day_count);
         });
      return result;
   }

   template <std::floating_point ty, cashflow_date_selector<ty> date_selector_policy = due_date_selector>
   [[nodiscard]]
   classic_irr_analysis<ty> AnalyzeClassicIrr(std::span<cashflow<ty> const> cashflows,
      classic_irr_options<ty> const& options = {}) {
      classic_irr_analysis<ty> result;
      result.normalized = NormalizeCashflows<ty, date_selector_policy>(cashflows, options.period);
      if (!detail::ValidCashflows(cashflows)) {
         result.result.status = irr_status::invalid_cashflows;
         result.result.message = "at least one positive inflow and one positive outflow are required";
         return result;
      }
      auto const values = std::span<ty const>{ result.normalized.values };
      result.result = detail::Analyze(options.analysis,
         [&](ty rate) { return CalculateClassicIrrTerms(values, rate); });
      return result;
   }

   template <std::floating_point ty>
   [[nodiscard]]
   irr_root<ty> const* UniqueRoot(irr_analysis<ty> const& analysis) noexcept {
      return analysis.status == irr_status::unique_root ? &analysis.roots.front() : nullptr;
   }

   template <std::floating_point ty>
   [[nodiscard]]
   ty AnnualizeRate(ty periodic_rate, irr_period period) {
      auto const periods = period == irr_period::month ? static_cast<ty>(12)
         : period == irr_period::quarter ? static_cast<ty>(4)
         : static_cast<ty>(1);
      return std::pow(static_cast<ty>(1) + periodic_rate, periods) - static_cast<ty>(1);
   }

   [[nodiscard]] inline std::string_view ToString(day_count_convention value) noexcept {
      switch (value) {
      case day_count_convention::actual_365_fixed: return "Actual/365 Fixed";
      case day_count_convention::actual_360: return "Actual/360";
      case day_count_convention::actual_actual: return "Actual/Actual";
      }
      return "Unknown";
   }

   [[nodiscard]] inline std::string_view ToString(irr_period value) noexcept {
      switch (value) {
      case irr_period::month: return "month";
      case irr_period::quarter: return "quarter";
      case irr_period::year: return "year";
      }
      return "unknown";
   }

   [[nodiscard]] inline std::string_view ToString(irr_status value) noexcept {
      switch (value) {
      case irr_status::unique_root: return "unique root";
      case irr_status::multiple_roots: return "multiple roots";
      case irr_status::no_root: return "no root";
      case irr_status::indeterminate: return "indeterminate";
      case irr_status::invalid_cashflows: return "invalid cashflows";
      }
      return "unknown";
   }

} // namespace adecc::financial