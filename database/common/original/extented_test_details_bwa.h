#pragma once

#include "database.h"
#include "type_lists.h"
#include "fixed_numeric.h"
#include "convert_fixed.h"

#include "extented_test_types.h"
#include "extented_test_helper.h"

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <print>

namespace extented_test {

   using monthly_bwa_list = adecc::defined_type_list<
      int,                 // Jahr
      int,                 // Monat
      adecc::money_ty,     // Ertrag
      adecc::money_ty,     // Aufwand
      adecc::money_ty,     // Ergebnis
      int                  // Anzahl der Buchungen
      >;

   using monthly_bwa_ty = typename monthly_bwa_list::type_list;

   struct monthly_bwa_accu_ty {
      adecc::money_ty mRevenue {};
      adecc::money_ty mExpense {};
      int iPostingCount {};
      };

   struct monthly_bwa_effect_ty {
      int iYear {};
      int iMonth {};
      adecc::money_ty mRevenueEffect {};
      adecc::money_ty mExpenseEffect {};
      };

   [[nodiscard]] inline int YearOf(adecc::date_ty const& aDate) {
	  return static_cast<int>(aDate.year());
	  }

   [[nodiscard]] inline int MonthOf(adecc::date_ty const& aDate) {
      return static_cast<int>(static_cast<unsigned>(aDate.month()));
   }
/*
   inline [[nodiscard]] bool IsRevenueAccount(TAccount const& acct) noexcept {
      return acct.Account_Type() == "Ertragskonto";
      }

   inline [[nodiscard]] bool IsExpenseAccount(TAccount const& acct) noexcept {
      return acct.Account_Type() == "Aufwandskonto";
      }
*/
/*
   inline [[nodiscard]] adecc::money_ty RevenueEffectOf(TAccount const& debit_acct,
                                                        TAccount const& credit_acct,
                                                        adecc::money_ty const& amount) {
      if (IsRevenueAccount(credit_acct))     return amount;
      else if (IsRevenueAccount(debit_acct)) return -amount;
      else return adecc::money_ty {};
      }
*/
   /*
   [[nodiscard]]
   adecc::money_ty ExpenseEffectOf(
      std::string_view const svDebitAccountType,
      std::string_view const svCreditAccountType,
      adecc::money_ty const& mAmount
   ) {
      if (IsExpenseAccount(svDebitAccountType)) {
         return mAmount;
      }

      if (IsExpenseAccount(svCreditAccountType)) {
         return -mAmount;
      }

      return adecc::money_ty{};
   }

   template <
      class value_ty,
      class compare_ty,
      class allocator_ty,
      class key_ty,
      class predicate_ty
   >
   [[nodiscard]]
   value_ty const& FindInSetOrThrow(
      std::set<value_ty, compare_ty, allocator_ty> const& setValues,
      key_ty const& theKey,
      predicate_ty Predicate
   ) {
      auto const theIter = std::ranges::find_if(
         setValues,
         [&](value_ty const& theValue) {
            return Predicate(theValue, theKey);
         }
      );

      if (theIter == setValues.end()) {
         throw std::out_of_range{
            std::format("Value '{}' not found in set.", theKey)
         };
      }

      return *theIter;
   }

   template <std::ranges::input_range posting_range_ty>
   concept monthly_bwa_posting_range = std::same_as<
      std::remove_cvref_t<std::ranges::range_value_t<posting_range_ty>>,
      entry_ty
   >;

   template <
      std::ranges::input_range posting_range_ty,
      class account_ty,
      class account_compare_ty,
      class account_allocator_ty,
      class account_id_fn_ty,
      class account_type_fn_ty
   >
      requires monthly_bwa_posting_range<posting_range_ty>
   [[nodiscard]]
   std::vector<monthly_bwa_ty> CalculateMonthlyBwaByYear(
      posting_range_ty&& rngPostings,
      std::set<account_ty, account_compare_ty, account_allocator_ty> const& setAccounts,
      int const iYear,
      account_id_fn_ty AccountIdOf,
      account_type_fn_ty AccountTypeOf
   ) {
      using month_key_ty = std::pair<int, int>;

      auto const AccountById = [&](int const iAccountId) -> account_ty const& {
         return FindInSetOrThrow(
            setAccounts,
            iAccountId,
            [&](account_ty const& aAccount, int const iSearchAccountId) {
               return std::invoke(AccountIdOf, aAccount) == iSearchAccountId;
            }
         );
         };

      auto rngEffects =
         std::forward<posting_range_ty>(rngPostings)
         | std::views::filter([&](entry_ty const& tplPosting) {
         return YearOf(std::get<1>(tplPosting)) == iYear;
            })
         | std::views::transform([&](entry_ty const& tplPosting) {
         adecc::date_ty const& aBookingDate = std::get<1>(tplPosting);

         int const iDebitAccountId = std::get<3>(tplPosting);
         int const iCreditAccountId = std::get<4>(tplPosting);
         adecc::money_ty const& mAmount = std::get<5>(tplPosting);

         account_ty const& aDebitAccount = AccountById(iDebitAccountId);
         account_ty const& aCreditAccount = AccountById(iCreditAccountId);

         std::string_view const svDebitAccountType = std::invoke(AccountTypeOf, aDebitAccount);
         std::string_view const svCreditAccountType = std::invoke(AccountTypeOf, aCreditAccount);

         return monthly_bwa_effect_ty{
            YearOf(aBookingDate),
            MonthOf(aBookingDate),
            RevenueEffectOf(svDebitAccountType, svCreditAccountType, mAmount),
            ExpenseEffectOf(svDebitAccountType, svCreditAccountType, mAmount)
         };
            });

      std::map<month_key_ty, monthly_bwa_accu_ty> mpBwaByMonth;

      std::ranges::for_each(
         rngEffects,
         [&mpBwaByMonth](monthly_bwa_effect_ty const& aEffect) {
            monthly_bwa_accu_ty& aAccu =
               mpBwaByMonth[{ aEffect.iYear, aEffect.iMonth }];

            aAccu.mRevenue += aEffect.mRevenueEffect;
            aAccu.mExpense += aEffect.mExpenseEffect;
            ++aAccu.iPostingCount;
         }
      );

      return mpBwaByMonth
         | std::views::transform([](auto const& theEntry) {
         auto const& [theMonthKey, aAccu] = theEntry;
         auto const& [iYear, iMonth] = theMonthKey;

         return monthly_bwa_ty{
            iYear,
            iMonth,
            aAccu.mRevenue,
            aAccu.mExpense,
            aAccu.mRevenue - aAccu.mExpense,
            aAccu.iPostingCount
         };
            })
         | std::ranges::to<std::vector>();
   }

   [[nodiscard]]
   adecc::money_ty TotalRevenue(std::vector<monthly_bwa_ty> const& vecMonthlyBwa) {
      return std::ranges::fold_left(
         vecMonthlyBwa
         | std::views::transform([](monthly_bwa_ty const& tplEntry) {
            return std::get<2>(tplEntry);
            }),
         adecc::money_ty{},
         std::plus<> {}
      );
   }

   [[nodiscard]]
   adecc::money_ty TotalExpense(std::vector<monthly_bwa_ty> const& vecMonthlyBwa) {
      return std::ranges::fold_left(
         vecMonthlyBwa
         | std::views::transform([](monthly_bwa_ty const& tplEntry) {
            return std::get<3>(tplEntry);
            }),
         adecc::money_ty{},
         std::plus<> {}
      );
   }

   [[nodiscard]]
   adecc::money_ty TotalResult(std::vector<monthly_bwa_ty> const& vecMonthlyBwa) {
      return std::ranges::fold_left(
         vecMonthlyBwa
         | std::views::transform([](monthly_bwa_ty const& tplEntry) {
            return std::get<4>(tplEntry);
            }),
         adecc::money_ty{},
         std::plus<> {}
      );
   }

   void ValidateMonthlyBwa(std::vector<monthly_bwa_ty> const& vecMonthlyBwa) {
      adecc::money_ty const mTotalRevenue = TotalRevenue(vecMonthlyBwa);
      adecc::money_ty const mTotalExpense = TotalExpense(vecMonthlyBwa);
      adecc::money_ty const mTotalResult = TotalResult(vecMonthlyBwa);

      if (mTotalResult != mTotalRevenue - mTotalExpense) {
         throw std::logic_error{
            "BWA invariant failed: result != revenue - expense."
         };
      }

      bool const boAllMonthsValid = std::ranges::all_of(
         vecMonthlyBwa,
         [](monthly_bwa_ty const& tplEntry) {
            int const iMonth = std::get<1>(tplEntry);
            int const iPostingCount = std::get<5>(tplEntry);

            return iMonth >= 1
               && iMonth <= 12
               && iPostingCount > 0;
         }
      );

      if (!boAllMonthsValid) {
         throw std::logic_error{
            "BWA invariant failed: invalid month or empty posting count."
         };
      }
   }

   template <class grid_ty>
   void WriteMonthlyBwaToGrid(
      grid_ty& aGrid,
      std::vector<monthly_bwa_ty> const& vecMonthlyBwa
   ) {
      auto rngGridRows =
         vecMonthlyBwa
         | std::views::transform([](monthly_bwa_ty const& tplEntry) {
         return monthly_bwa_ty{
            std::get<0>(tplEntry),
            std::get<1>(tplEntry),
            std::get<2>(tplEntry),
            std::get<3>(tplEntry),
            std::get<4>(tplEntry),
            std::get<5>(tplEntry)
         };
            });

      aGrid += rngGridRows;
   }

   template <
      std::ranges::input_range posting_range_ty,
      class account_ty,
      class account_compare_ty,
      class account_allocator_ty,
      class account_id_fn_ty,
      class account_type_fn_ty,
      class grid_ty
   >
      requires monthly_bwa_posting_range<posting_range_ty>
   [[nodiscard]]
   std::vector<monthly_bwa_ty> RunMonthlyBwaDemo(
      posting_range_ty&& rngPostings,
      std::set<account_ty, account_compare_ty, account_allocator_ty> const& setAccounts,
      int const iYear,
      account_id_fn_ty AccountIdOf,
      account_type_fn_ty AccountTypeOf,
      grid_ty& aGrid
   ) {
      std::vector<monthly_bwa_ty> vecMonthlyBwa = CalculateMonthlyBwaByYear(
         std::forward<posting_range_ty>(rngPostings),
         setAccounts,
         iYear,
         AccountIdOf,
         AccountTypeOf
      );

      ValidateMonthlyBwa(vecMonthlyBwa);
      WriteMonthlyBwaToGrid(aGrid, vecMonthlyBwa);

      return vecMonthlyBwa;
   }

*/
/*
Beispielaufruf:

std::vector<adecc::demo::monthly_bwa_ty> vecMonthlyBwa =
   adecc::demo::RunMonthlyBwaDemo(
      rngPostings,
      setAccounts,
      2026,
      [](TAccount const& aAccount) {
         return aAccount.AccountID();
         },
      [](TAccount const& aAccount) -> std::string_view {
         return aAccount.AccountType();
         },
      aGrid
   );
*/


} // end of namespace extented_test