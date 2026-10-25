// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file main.cpp
\brief Console test application for IRR calculations and ECB currency conversion.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#include "TestExecution.h"
#include "TestData.h"
#include "EcbCurrencyDemo.h"

#include "EcbData.h"

#include <iostream>
#include <print>

#ifdef _WIN32
#include <Windows.h>
#endif


int main() {
#ifdef _WIN32
   SetConsoleOutputCP(CP_UTF8);
   SetConsoleCP(CP_UTF8);
#endif
   std::println(std::cout, "Test cases starting ...\n");
   try {
      using value_ty = double;
      using namespace adecc::financial;
      namespace test = adecc::financial::test;

      constexpr bool withOutput = true;
      
      auto const ecb_history = [&] {
         constexpr auto publicationTime = std::chrono::hours{ 16 } + std::chrono::minutes{ 15 };
         if constexpr (withOutput) {
            auto const ecb_history = ecb::LoadEcbExchangeRateHistory<value_ty>(publicationTime, std::cout);
            ecb::demo::TestEcbExchangeRateHistory<value_ty>(std::cout, ecb_history);
            return ecb_history;
            }
         else {
            auto const ecb_history = ecb::LoadEcbExchangeRateHistory<value_ty>(publicationTime);
            return ecb_history;
            }
         }();

      

  
 
      if constexpr (withOutput) {
         auto technical_tests    = test::CreateTechnicalTests<value_ty>();
         auto long_running_tests = test::CreateLongRunningTests<value_ty>();
         auto start = std::chrono::high_resolution_clock::now();

         test::RunTests<value_ty>(std::cout, "Technical and numerical scenarios", technical_tests, ecb_history);
         test::RunTests<value_ty>(std::cout, "Long-running comparison scenarios", long_running_tests, ecb_history);
        
         auto end = std::chrono::high_resolution_clock::now();
         auto uset = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
         std::println("\nTest finished in {:6.3f} seconds", uset / 1000.);
         }
      else {
         auto long_running_tests = test::CreateLongRunningTests<value_ty>();

         auto singleFundNames = long_running_tests | std::views::filter([](auto const& test) {
                                                            return test.file_type == fund_file_type::single_fund;
                                                            })
                                                   | std::views::transform(&test::test_case<double>::filename);

         for (auto fundname : singleFundNames) {
            auto fundData = ReadFund<double>(fundname);
            test::TestFundOverview(std::cout, fundData);
            }
         }

      auto fundData = ReadFund<double>("fund.json");
      WriteFund("exported_fund.json", fundData);
      }
   catch (std::exception const& error) {
      std::println(stderr, "Error: {}", error.what());
      return 1;
      }
   std::println(std::cout, "\nprogram finished successful.");
   return 0;
   }