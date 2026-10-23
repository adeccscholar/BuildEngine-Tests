// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file TestData.h
\brief Construction of the financial test-case collections.
*/

#pragma once

#include "TestExecution.h"

#include <concepts>
#include <vector>

namespace adecc::financial::test {

/** \brief Creates the technical and numerical IRR test cases. */
template <std::floating_point value_ty>
[[nodiscard]]
std::vector<test_case<value_ty>> CreateTechnicalTests() {
   return  {
              { .name = "Standard fund with one root", 
                .filename = "fund.json",
                .file_type = fund_file_type::single_fund,
                .start_values = { 0.10 } },
              { .name = "Fund with multiple roots", 
                .filename = "multiple_root_fund.json",
                .file_type = fund_file_type::single_fund,
                .start_values = { 0.03, 0.12, 0.50 } },
              { .name = "Fund with cycle for start value", 
                .filename = "newton_cycle_fund.json",
                .file_type = fund_file_type::single_fund,
                .start_values = { 0.02 } },
              { .name = "Positive start fails, negative start converges",
                .filename = "negative_start_fund.json",
                .file_type = fund_file_type::single_fund,
                .start_values = { 0.20, -0.50 }, 
                .maximum_iterations = 1000 },
              { .name = "Multiple funds individually and combined",
                .filename = "multiple_funds.json",
                .file_type = fund_file_type::multiple_funds,
                .virtual_fund_currency = "EUR",
                .start_values = { 0.10 } },
              { .name = "Multi-currency investors and investments",
                .filename = "multi_currency_fund.json",
                .file_type = fund_file_type::single_fund,
                .start_values = { 0.10 } },
              { .name = "Ten multi-currency funds combined in EUR",
                .filename = "ten_multi_currency_funds.json",
                 .file_type = fund_file_type::multiple_funds,
                .virtual_fund_currency = "EUR",
                .start_values = { 0.10 } },
              { .name = "Ten multi-currency funds combined in USD",
                .filename = "ten_multi_currency_funds.json",
                .file_type = fund_file_type::multiple_funds,
                .virtual_fund_currency = "USD",
                .start_values = { 0.10 } },
              { .name = "Ten multi-currency funds combined in GBP",
                .filename = "ten_multi_currency_funds.json",
                .file_type = fund_file_type::multiple_funds,
                .virtual_fund_currency = "GBP",
                .start_values = { 0.10 } },
              { .name = "Monthly normalization failure",
                .filename = "monthly_normalization_failure_fund.json",
                .file_type = fund_file_type::single_fund,
                .virtual_fund_currency = {},
                .start_values = { static_cast<value_ty>(0.10) },
                .classic_period = irr_period::month
              }
           };
      }



/** \brief Creates the technical and numerical IRR test cases. */
template <std::floating_point value_ty>
[[nodiscard]]
std::vector<test_case<value_ty>> CreateLongRunningTests() {
   return {
             { .name = "Long-term buyout fund",
               .filename = "long_term_buyout_fund.json", 
               .file_type = fund_file_type::single_fund,
               .start_values = { 0.10 }, 
               .scan_intervals = 1024 },
             { .name = "Early distributions",
               .filename = "early_distribution_fund.json",
               .file_type = fund_file_type::single_fund,
               .start_values = { 0.10 }, 
               .scan_intervals = 1024 },
             { .name = "Late distributions",
               .filename = "late_distribution_fund.json",
               .file_type = fund_file_type::single_fund,
               .start_values = { 0.10 }, 
               .scan_intervals = 1024 },
             { .name = "Leap-year infrastructure fund",
               .filename = "leap_year_infrastructure_fund.json",
               .file_type = fund_file_type::single_fund,
               .start_values = { 0.08 },
               .day_counts = { day_count_convention::actual_365_fixed,
                               day_count_convention::actual_360,
                               day_count_convention::actual_actual
                             },
               .scan_intervals = 1024 },
             { .name = "Monthly infrastructure fund",
               .filename = "monthly_infrastructure_fund.json",
               .file_type = fund_file_type::single_fund,
               .start_values = { 0.08 },
               .classic_period = irr_period::month,
               .scan_intervals = 1024 },
             { .name = "Long-horizon venture fund",
               .filename = "long_horizon_venture_fund.json",
               .file_type = fund_file_type::single_fund,
               .start_values = { 0.15 }, 
               .scan_intervals = 1024 },
             { .name = "Long-term comparison portfolio",
               .filename = "long_term_comparison_portfolio.json",
               .file_type = fund_file_type::multiple_funds,
               .virtual_fund_currency = "EUR",
               .start_values = { 0.10 }, 
               .scan_intervals = 1024 },
             { .name = "Long-running monthly normalization failure",
               .filename = "long_running_monthly_normalization_outlier_fund.json",
               .file_type = fund_file_type::single_fund,
               .virtual_fund_currency = {},
               .start_values = { static_cast<value_ty>(0.10) },
               .classic_period = irr_period::month
             },
             { .name = "XIRR versus monthly classic IRR divergence",
               .filename = "long_running_xirr_classic_divergence_fund.json",
               .file_type = fund_file_type::single_fund,
               .virtual_fund_currency = {},
               .start_values = { static_cast<value_ty>(0.10) },
               .classic_period = irr_period::month
             }
          };
   }

} // end of namespace