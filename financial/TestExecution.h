// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file TestExecution.h
\brief Test execution and formatted output for financial return and currency scenarios.

\details Exercises direct and analytical XIRR and classic IRR calculations,
multiple-root detection, residual and sensitivity output, day-count conventions,
period normalization, annualization, currency conversion, direct fund KPI output and
multi-fund comparison. The value date is selected as a compile-time policy. Classic monthly
IRR uses cashflows normalized to the first day of each calendar month.

\version 1.0
\date    05.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)
*/
#pragma once

#include "FinancialModel.h"
#include "FinancialCalculation.h"
#include "FinancialJson.h"
#include "CurrencyExchange.h"

#include <chrono>
#include <concepts>
#include <expected>
#include <filesystem>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace adecc::financial::test {

   template <std::floating_point ty>
   struct test_case {
      std::string_view name;
      std::filesystem::path filename;
      fund_file_type file_type;
      std::string virtual_fund_currency;
      std::vector<ty> start_values;
      std::vector<day_count_convention> day_counts{
         day_count_convention::actual_365_fixed,
         day_count_convention::actual_360
      };
      irr_period classic_period{ irr_period::year };
      std::size_t maximum_iterations{ 100 };
      std::size_t scan_intervals{ 512 };
   };

   template <std::floating_point ty>
   void PrintRoot(std::ostream& output, irr_root<ty> const& root, ty displayed_rate) {
      constexpr auto percent = static_cast<ty>(100);
      std::println(output, "  Rate                      {:>14.4f} %", displayed_rate * percent);
      std::println(output, "  Iterations                {:>14}", root.iterations);
      std::println(output, "  Absolute residual         {:>14.4e}", root.residual);
      std::println(output, "  Relative residual         {:>14.4e}", root.relative_residual);
      std::println(output, "  Absolute derivative       {:>14.4e}", root.derivative);
      std::println(output, "  Normalized derivative     {:>14.4e}", root.normalized_derivative);
      std::println(output, "  Local IRR sensitivity     {:>14.4e}", root.rate_sensitivity);
   }

   template <std::floating_point ty>
   void PrintBatchComparison(std::ostream& output, std::expected<ty, irr_error> const& batch,
      ty displayed_batch, ty displayed_analysis) {
      constexpr auto percent = static_cast<ty>(100);
      std::println(output, "\n  Batch comparison");
      if (!batch) {
         std::println(output, "  Batch error               {}", batch.error().message);
         return;
      }
      std::println(output, "  Batch rate                {:>14.4f} %", displayed_batch * percent);
      std::println(output, "  Analysis rate             {:>14.4f} %", displayed_analysis * percent);
      std::println(output, "  Displayed difference      {:>14.4e}",
         displayed_batch - displayed_analysis);
   }

   template <std::floating_point ty, typename rate_fy>
   void PrintAnalysis(std::ostream& output, std::string_view method,
      irr_analysis<ty> const& analysis, rate_fy&& displayed_rate) {
      std::println(output, "\n{}", method);
      std::println(output, "  Status                    {}", ToString(analysis.status));
      std::println(output, "  Roots                     {}", analysis.roots.size());
      std::println(output, "  Scanned intervals         {}", analysis.scanned_intervals);
      std::println(output, "  Non-finite intervals      {}", analysis.non_finite_intervals);
      if (!analysis.message.empty())
         std::println(output, "  Message                   {}", analysis.message);

      for (std::size_t index = 0; index < analysis.roots.size(); ++index) {
         std::println(output, "\n  Root {}", index + 1);
         PrintRoot(output, analysis.roots[index], displayed_rate(analysis.roots[index].rate));
      }
   }

   template <std::floating_point ty>
   void TestFundOverview(std::ostream& output, fund<ty> const& current) {
      constexpr auto percent = static_cast<ty>(100);

      auto const printText = [&](std::string_view label, std::string const& value) {
         if (!value.empty()) std::println(output, "  {:<31} {}", label, value);
         };
      auto const printDate = [&](std::string_view label, std::optional<date_ty> const& value) {
         if (value) std::println(output, "  {:<31} {:%Y-%m-%d}", label, std::chrono::sys_days{ *value });
         };
      auto const printAmount = [&](std::string_view label, std::optional<ty> const& value) {
         if (value) std::println(output, "  {:<31} {:>14.2f} {}", label, *value, current.currency);
         };
      auto const printRate = [&](std::string_view label, std::optional<ty> const& value) {
         if (value) std::println(output, "  {:<31} {:>14.4f} %", label, *value * percent);
         };

      std::println(output, "\n====================== Fund overview ======================");
      std::println(output, "  {:<31} {}", "Name", current.name);
      std::println(output, "  {:<31} {}", "Currency", current.currency);
      printText("Fund type", current.fund_type);
      printText("Fund stage", current.fund_stage);
      if (current.vintage_year)
         std::println(output, "  {:<31} {}", "Vintage year", *current.vintage_year);
      printDate("First closing", current.first_closing);
      printDate("Final closing", current.final_closing);
      printDate("Investment start", current.investment_start_date);
      printDate("Investment end", current.investment_end_date);
      printDate("Terms end", current.terms_end_date);
      printAmount("Commitment", current.commitment);
      printAmount("Fund volume", current.fund_volume);
      printRate("Hurdle rate", current.hurdle_rate);
      printRate("Carry rate", current.carry_rate);
      printRate("Catch-up rate", current.catch_up_rate);
      if (current.management_fee_base) {
         auto const basis = *current.management_fee_base == management_fee_basis::commitment
            ? "commitment"
            : *current.management_fee_base == management_fee_basis::invested_capital
            ? "invested capital" : "net asset value";
         std::println(output, "  {:<31} {}", "Management fee basis", basis);
      }
      printRate("Management fee rate", current.management_fee_rate);
      printDate("Management fee step-down", current.management_fee_step_down_date);
      printRate("Management fee after step-down", current.management_fee_rate_after_step_down);
      printRate("Setup fee rate", current.setup_fee_rate);
      printAmount("Setup fee maximum", current.setup_fee_maximum);
      printRate("Recycling maximum rate", current.recycling_maximum_rate);
      printRate("Leverage rate", current.leverage_rate);
      printRate("Late-payment interest rate", current.late_payment_interest_rate);
      printRate("Default interest rate", current.default_interest_rate);
      printDate("Valuation date", current.valuation_date);
      printAmount("Residual value", current.residual_value);
      std::println(output, "  {:<31} {}", "Clawback applicable", current.clawback_applicable);
      std::println(output, "  {:<31} {}", "Recycling allowed", current.recycling_allowed);
      std::println(output, "  {:<31} {:>14.4f} %", "IRR start value", current.InitialRate() * percent);

      ty paid_in{};
      ty distributions{};
      ty valuation_cashflows{};
      ty fees{};

      std::ranges::for_each(current.cashflows, [&](cashflow<ty> const& item) {
         if (item.direction == payment_direction::outflow) paid_in += item.value;
         else if (item.category == payment_category::valuation) valuation_cashflows += item.value;
         else distributions += item.value;

         if (item.category == payment_category::fee ||
            item.category == payment_category::management_fee)
            fees += item.value;
         });

      auto const residual_value = current.residual_value.value_or(valuation_cashflows);
      auto const net_value = distributions + residual_value - paid_in;
      auto const dpi = paid_in > ty{} ? distributions / paid_in : ty{};
      auto const rvpi = paid_in > ty{} ? residual_value / paid_in : ty{};
      auto const tvpi = paid_in > ty{} ? (distributions + residual_value) / paid_in : ty{};
      auto const paid_in_ratio = current.commitment && *current.commitment > ty{}
      ? paid_in / *current.commitment : ty{};
      auto const remaining_commitment = current.commitment
         ? std::max(ty{}, *current.commitment - paid_in) : ty{};

      std::println(output, "\n  Key figures");
      std::println(output, "  {:<31} {:>14.2f} {}", "Paid-in capital", paid_in, current.currency);
      std::println(output, "  {:<31} {:>14.2f} {}", "Distributions", distributions, current.currency);
      std::println(output, "  {:<31} {:>14.2f} {}", "Residual value", residual_value, current.currency);
      std::println(output, "  {:<31} {:>14.2f} {}", "Fees", fees, current.currency);
      std::println(output, "  {:<31} {:>14.2f} {}", "Net value", net_value, current.currency);
      if (current.commitment) {
         std::println(output, "  {:<31} {:>14.2f} {}", "Remaining commitment", remaining_commitment, current.currency);
         std::println(output, "  {:<31} {:>14.4f} %", "Paid-in ratio", paid_in_ratio * percent);
      }
      if (paid_in > ty{}) {
         std::println(output, "  {:<31} {:>14.4f} x", "DPI", dpi);
         std::println(output, "  {:<31} {:>14.4f} x", "RVPI", rvpi);
         std::println(output, "  {:<31} {:>14.4f} x", "TVPI", tvpi);
      }



      auto const cashflows = std::span<cashflow<ty> const>{ current.cashflows };
      irr_options<ty> const xirrOptions { .start_value = current.InitialRate() };
      auto const classicPeriod = irr_period::month;

      irr_options<ty> const classicIrrOptions { .start_value = PeriodicRate(current.InitialRate(), classicPeriod) };

      auto const xirr = CalculateXirrValue<ty, value_date_selector>(cashflows, xirrOptions, day_count_convention::actual_365_fixed);

      auto const classicIrr = CalculateClassicIrrValue<ty, value_date_selector>(cashflows, classicIrrOptions, classicPeriod);

      std::println(output, "\n  Direct return calculations");
      if (xirr)
         std::println(output, "  {:<31} {:>14.4f} %", "XIRR, Actual/365 Fixed", *xirr * percent);
      else
         std::println(output, "  {:<31} {}", "XIRR, Actual/365 Fixed", xirr.error().message);

      if (classicIrr) {
         std::println(output, "  {:<31} {:>14.4f} %", "Classic IRR, monthly", *classicIrr * percent);
         std::println(output, "  {:<31} {:>14.4f} %", "Classic IRR, annualized",
            AnnualizeRate(*classicIrr, irr_period::month) * percent);
      }
      else std::println(output, "  {:<31} {}", "Classic IRR, monthly", classicIrr.error().message);

      std::println(output, "\n  Cashflows");
      std::println(output, "  {:<10} {:<10} {:>14} {:<3} {:<7} {:<20} {}",
         "Due date", "Value date", "Amount", "Cur", "Flow", "Category", "Description");
      std::println(output, "  {:-<10} {:-<10} {:-<14} {:-<3} {:-<7} {:-<20} {:-<20}",
         "", "", "", "", "", "", "");
      std::ranges::for_each(current.cashflows, [&](cashflow<ty> const& item) {
         std::println(output, "  {:%Y-%m-%d} {:%Y-%m-%d} {:>14.2f} {:<3} {:<7} {:<20} {}",
            std::chrono::sys_days{ item.due_date },
            std::chrono::sys_days{ value_date_selector {}(item) },
            item.SignedValue(), item.currency, item.direction, item.category,
            item.description);
         });
   }

   template <std::floating_point ty>
   void TestSingleFund(
      std::ostream& output,
      fund<ty> const& current,
      currency::exchange_rate_history<ty> const& exchange_rates,
      test_case<ty> const& test) {
      std::println(output, "\nFund:      {}", current.name);
      std::println(output, "Currency:  {}", current.currency);
      std::println(output, "Cashflows: {}", current.cashflows.size());
      std::println(output,
         "  {:<10} {:>14} {:<3} {:>14} {:<3} {:<7} {:<16} {}",
         "Value date", "Original", "Cur", "Fund amount", "Cur",
         "Flow", "Category", "Description");
      std::println(output, "  {:-<10} {:-<14} {:-<3} {:-<14} {:-<3} {:-<7} {:-<16} {:-<20}",
         "", "", "", "", "", "", "", "");
      std::ranges::for_each(current.cashflows, [&](cashflow<ty> const& item) {
         auto const converted = currency::ConvertCashflow(
            exchange_rates, item, current.currency);
         std::println(output,
            "  {:%Y-%m-%d} {:>14.2f} {:<3} {:>14.2f} {:<3} {:<7} {:<16} {}",
            std::chrono::sys_days{ value_date_selector {}(item) },
            item.SignedValue(),
            item.currency,
            converted.SignedValue(),
            current.currency,
            item.direction,
            item.category,
            item.description);
         });
      auto const normalized =
         currency::ConvertFundCashflows(exchange_rates, current);
      auto const cashflows =
         std::span<cashflow<ty> const>{ normalized.cashflows };
      std::ranges::for_each(test.start_values, [&](ty start_value) {
         irr_analysis_options<ty> const analysis_options{
            .solver = {
               .start_value = start_value,
               .maximum_iterations = test.maximum_iterations
               },
            .scan_intervals = test.scan_intervals
         };

         std::println(output, "\n------------------------------------------------------------");
         std::println(output, "Start value: {:.4f} %", start_value * static_cast<ty>(100));

         std::ranges::for_each(test.day_counts, [&](day_count_convention day_count) {
            auto const analysis = AnalyzeXirr<ty, value_date_selector>(
               cashflows,
               xirr_options<ty> {
               .analysis = analysis_options,
                  .day_count = day_count
            });
            PrintAnalysis(output, std::format("XIRR, {}", ToString(day_count)),
               analysis.result, [](ty rate) { return rate; });
            if (auto const root = UniqueRoot(analysis.result); root) {
               auto const batch = CalculateXirrValue<ty, value_date_selector>(
                  cashflows, analysis_options.solver, day_count);
               PrintBatchComparison(output, batch, batch ? *batch : ty{}, root->rate);
            }
            });

         auto const analysis = AnalyzeClassicIrr<ty, value_date_selector>(cashflows, classic_irr_options<ty> {
            .analysis = analysis_options,
               .period = test.classic_period
         });
         std::println(output, "\nClassic normalization");
         std::println(output, "  Period                    {}", ToString(analysis.normalized.period));
         std::println(output, "  First period              {:%Y-%m-%d}",
            std::chrono::sys_days{ analysis.normalized.first_period_date });
         std::println(output, "  Number of periods         {}", analysis.normalized.values.size());
         std::println(output, "  Cashflow sign changes     {}", analysis.normalized.sign_changes);
         PrintAnalysis(output, std::format("Classic IRR, {} periods",
            ToString(test.classic_period)),
            analysis.result, [&](ty rate) {
               return AnnualizeRate(rate, test.classic_period);
            });
         if (auto const root = UniqueRoot(analysis.result); root) {
            auto const batch = CalculateClassicIrrValue<ty, value_date_selector>(
               cashflows, analysis_options.solver, test.classic_period);
            auto const batch_display = batch
               ? AnnualizeRate(*batch, test.classic_period) : ty{};
            auto const analysis_display =
               AnnualizeRate(root->rate, test.classic_period);
            PrintBatchComparison(output, batch, batch_display, analysis_display);
         }
         });
   }


   [[nodiscard]]
   std::string CompactColumnName(std::string_view name, std::size_t width) {
      if (name.size() <= width)
         return std::string{ name };
      if (width <= 3)
         return std::string(name.substr(0, width));
      return std::format("{}...", name.substr(0, width - 3));
   }

   template <std::floating_point ty, typename display_fy>
   void PrintComparisonValue(std::ostream& output,
      std::expected<ty, irr_error> const& value, display_fy&& display) {
      if (value)
         std::print(output, "{:>17.4f} %", display(*value) * static_cast<ty>(100));
      else
         std::print(output, "{:>19}", "n/a");
   }

   template <std::floating_point ty>
   void PrintMultiFundComparison(
      std::ostream& output,
      fund_document<ty> const& document,
      fund<ty> const& combined,
      test_case<ty> const& test,
      currency::exchange_rate_history<ty> const& exchange_rates) {
      constexpr std::size_t variant_width = 30;
      constexpr std::size_t value_width = 19;

      auto const normalized_combined =
         currency::ConvertFundCashflows(exchange_rates, combined);

      std::vector<fund<ty>> normalized_funds;
      normalized_funds.reserve(document.funds.size());
      std::ranges::transform(
         document.funds,
         std::back_inserter(normalized_funds),
         [&](fund<ty> const& current) {
            return currency::ConvertFundCashflows(
               exchange_rates, current);
         });

      std::ranges::for_each(test.start_values, [&](ty start_value) {
         irr_options<ty> const options{
            .start_value = start_value,
            .maximum_iterations = test.maximum_iterations
         };

         std::println(output, "\n================ Multi-fund IRR comparison ================");
         std::println(output, "Start value: {:.4f} %", start_value * static_cast<ty>(100));
         std::print(output, "{:<{}}", "Variant", variant_width);
         std::print(output, "{:>{}}", "Global", value_width);
         std::ranges::for_each(document.funds, [&](fund<ty> const& current) {
            std::print(output, "{:>{}}", CompactColumnName(current.name, value_width), value_width);
            });
         std::println(output);

         std::print(output, "{:-<{}}", "", variant_width);
         std::print(output, "{:-<{}}", "", value_width);
         std::ranges::for_each(document.funds, [&](auto const&) {
            std::print(output, "{:-<{}}", "", value_width);
            });
         std::println(output);

         std::ranges::for_each(test.day_counts,
            [&](day_count_convention day_count) {
               auto const label = std::format("XIRR, {}", ToString(day_count));
               std::print(output, "{:<{}}", label, variant_width);

               auto const global = CalculateXirrValue<ty, value_date_selector>(
                  std::span<cashflow<ty> const> {
                  normalized_combined.cashflows
               },
                  options, day_count);
               PrintComparisonValue(output, global, [](ty rate) { return rate; });

               std::ranges::for_each(
                  normalized_funds,
                  [&](fund<ty> const& current) {
                     auto const value = CalculateXirrValue<ty, value_date_selector>(
                        std::span<cashflow<ty> const> {
                        current.cashflows
                     },
                        options, day_count);
                     PrintComparisonValue(
                        output, value, [](ty rate) { return rate; });
                  });
               std::println(output);
            });

         auto const label = std::format("Classic IRR, {}",
            ToString(test.classic_period));
         std::print(output, "{:<{}}", label, variant_width);

         auto const global = CalculateClassicIrrValue<ty, value_date_selector>(std::span<cashflow<ty> const> {
            normalized_combined.cashflows
         }, options, test.classic_period);
         PrintComparisonValue(output, global, [&](ty rate) {
            return AnnualizeRate(rate, test.classic_period);
            });

         std::ranges::for_each(
            normalized_funds,
            [&](fund<ty> const& current) {
               auto const value = CalculateClassicIrrValue<ty, value_date_selector>(
                  std::span<cashflow<ty> const> {
                  current.cashflows
               },
                  options, test.classic_period);
               PrintComparisonValue(output, value, [&](ty rate) {
                  return AnnualizeRate(rate, test.classic_period);
                  });
            });
         std::println(output);
         });
   }

   template <std::floating_point ty>
   void TestFund(std::ostream& output, test_case<ty> const& test, currency::exchange_rate_history<ty> const& exchange_rates) {
      std::println(output, "\n============================================================");
      std::println(output, "Test: {}", test.name);
      std::println(output, "File: {}", test.filename.string());
      std::println(output, "============================================================");

      switch (test.file_type) {
      case fund_file_type::single_fund:
         TestSingleFund(output, ReadFund<ty>(test.filename), exchange_rates, test);
         break;
      case fund_file_type::multiple_funds: {
         auto const document = ReadFundDocument<ty>(test.filename);
         std::println(output, "\nFunds: {}", document.funds.size());
         std::println(output, "\n==================== Individual funds ====================");
         std::ranges::for_each(document.funds, [&](fund<ty> const& current) {
            TestSingleFund(output, current, exchange_rates, test);
            });

         if (test.virtual_fund_currency.empty()) {
            throw std::runtime_error("A currency must be defined for the virtual combined fund.");
         }

         std::println(output, "\n===================== Combined funds =====================");
         std::println(output, "Virtual fund currency: {}", test.virtual_fund_currency);
         auto const combined = currency::CombineFunds(exchange_rates,
            std::span<fund<ty> const> { document.funds },
            test.virtual_fund_currency,
            "Combined portfolio");
         TestSingleFund(output, combined, exchange_rates, test);
         PrintMultiFundComparison(output, document, combined, test, exchange_rates);
         break;
      }
      }
   }


   template <std::floating_point ty>
   void RunTests(std::ostream& output, std::string_view title, std::span<test_case<ty> const> tests,
      currency::exchange_rate_history<ty> const& exchange_rates) {
      std::println(output, "\n############################################################");
      std::println(output, "{}", title);
      std::println(output, "############################################################");

      std::ranges::for_each(tests, [&](test_case<ty> const& current) {
         TestFund(output, current, exchange_rates);
         });
   }

} // namespace test
