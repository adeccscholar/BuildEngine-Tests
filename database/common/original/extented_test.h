#pragma once

#include "system_data_persistent.h"

#include "extented_test_types.h"
#include "extented_test_create.h"
#include "extented_test_data.h"
#include "extented_test_grids.h"
#include "extented_test_details.h"
#include "extented_test_details_bwa.h"
#include "extented_test_helper.h"

#include "text_grid_wrapper.h"
#include "file_as_tuple.h"

#include <chrono>
#include <format>
#include <map>
#include <print>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <cassert>


namespace extented_test {

   template <adecc::db::logical_database_type app_db>
   void RunTests(app_db& db, std::ostream& out) {
      using backend_ty = adecc::grid::OStreamGridBackend<adecc::AnsiStreamPolicy>;
      using grid_ty = adecc::grid::WriteGridModel<backend_ty>;

      auto aTransaction = db.Transaction();
      CreateTables(db, out);
      aTransaction.Commit();

      auto vecAcctType = InsertPersistent<TAccountType, account_type_list>(db, vecAccountTypes,
         IdentityTransform<acct_type_ty>)
         | std::ranges::to<std::vector>();

      acct_type_bridge mapAcctType = BuildBridge<2, 0>(vecAcctType);

      auto vecAcctClass = InsertPersistent<TAccountClass, account_class_list>(db, vecAccountClasses, 
                                                             IdentityTransform<acct_class_ty>)
                             | std::ranges::to<std::vector>();

      acct_class_bridge mapAcctClass = BuildBridge<1, 0>(vecAcctClass);
      act_class_set AccountClasses   = ToPersistentSet<TAccountClass>(vecAcctClass);

      {
         backend_ty theBackend{ out, backend_ty::TableSeparators() };
         grid_ty theGrid{ std::move(theBackend), vecAccountClassesCaps, true };
         auto guard = theGrid.freeze_guard();
         using acct_type_grid_seq = std::index_sequence<0, 1, 2, 3, 4>;
         std::ranges::for_each(vecAcctClass, [&theGrid](auto val) { 
                                     theGrid += adecc::selectTplValues<acct_type_grid_seq>(val);
                                     });

      }


     auto vecAccs = InsertPersistent<TAccount, account_raw_list>(db, vecAccounts, 
                                                                 TransformAccount, mapAcctType, mapAcctClass)
                             | std::ranges::to<std::vector>();
  
      acct_bridge mapAccs = BuildBridge<1, 0>(vecAccs);
      act_set Accounts    = ToPersistentSet<TAccount>(vecAccs);

      auto vecTaxes = InsertPersistent<TTaxClass, tax_class_list>(db, vecTaxClasses, 
                                                                  TransformTaxClass, mapAccs)
                             | std::ranges::to<std::vector>();

      tax_class_bridge mapTaxes = BuildBridge<1, 0>(vecTaxes);
      tax_class_set Taxes       = ToPersistentSet<TTaxClass>(vecTaxes);

      {
         backend_ty theBackend{ out, backend_ty::TableSeparators() };
         grid_ty theGrid{ std::move(theBackend), vecAccountCaps, true };
         //theGrid += vecAccs;
         //*
         auto guard = theGrid.freeze_guard();
         using acct_grid_seq = std::index_sequence<0, 1, 2, 3, 4, 5, 6>;
         std::ranges::for_each(vecAccs, [&theGrid](auto val) {
            theGrid += adecc::selectTplValues<acct_grid_seq>(val);
            });
         //*/
      }

      {
         std::filesystem::path file = "e:\\Test\\buchungen.csv";
         backend_ty theBackend{ out, backend_ty::TableSeparators() };
         grid_ty theGrid{ std::move(theBackend), vecPostingCaps, true };
         theGrid += InsertPostings(db, vecPostings, mapAccs, mapTaxes);
         theGrid += InsertPostings(db, adecc::FromCsvFileTyped<true, true, entry_raw_list>(file, ";", true),
                                   mapAccs, mapTaxes);
      }
      {
         backend_ty theBackend{ out, backend_ty::TableSeparators() };
         grid_ty theGrid{ std::move(theBackend), vecBalAcctByYearCaps, true };
         // pair ist kein Tuple
         theGrid += CalculateBalanceOfAccountByYear(db, 2026) |
                         std::views::transform([&Accounts, &AccountClasses](auto const& entry) {
                               auto acct = FindInSet(Accounts, entry.first);
                               return std::tuple<int, std::string, std::string, adecc::money_ty> {
                                    acct.Account_No(),
                                    acct.Denotation(),
                                    FindInSet(AccountClasses, acct.Account_Class()).Denotation(),
                                    entry.second
                                    };
                                });
      }

   
      aTransaction.Commit();


      TAccount Acc5 = adecc::db::PersistentDataExecutor::SelectPersistent<TAccount>(
         db,
         TAccount{ 5 }
      );
      std::println(std::cout, "Account No:  {}", Acc5.Account_No());
      std::println(std::cout, "Denotation:  {}", Acc5.Denotation());
      std::println(std::cout, "Currency:    {}", Acc5.Currency());

      Acc5.Description("Testdatensatz");

      auto _ = adecc::db::PersistentDataExecutor::UpdatePersistent<TAccount>(
            db, Acc5);

      TAccount AccNew1 = { 0,  1660, "Betriebs- und Geschäftsausstattung", 
                           BridgeValue(mapAcctType, "SK"s),
                           BridgeValue(mapAcctClass, "Aktivkonto"s),
                           "EUR", true, "Anlagekonto fuer Betriebs- und Geschaeftsausstattung" };
      TAccount AccIns =
         adecc::db::PersistentDataExecutor::InsertPersistent<TAccount>(db, std::move(AccNew1));
      std::println(std::cout, "New ID:  {}", AccIns.ID());

      // Lesetest
      //PersistentDataExecutor<TAccount>()
      {
         adecc::vecCaptions<adecc::AnsiStreamPolicy> Descr = {
               { "Beschreibung", 50,  adecc::EAlignmentType::left } };

         backend_ty theBackend{ out, backend_ty::TableSeparators() };
         grid_ty theGrid{ std::move(theBackend), vecAccountCaps + Descr, true };
         theGrid += adecc::db::PersistentDataExecutor::SelectAll<TAccount>(db);
      }

      Acc5 = adecc::db::PersistentDataExecutor::SelectPersistent<TAccount>(
         db,
         TAccount{ 5 }
         );
      std::println(std::cout, "Account No:  {}", Acc5.Account_No());
      std::println(std::cout, "Denotation:  {}", Acc5.Denotation());
      std::println(std::cout, "Currency:    {}", Acc5.Currency());


   }


   template <adecc::db::logical_database_type app_db>
   std::string RunExtendedHtml(app_db& db) {
      std::ostringstream os;
      {
         using backend_ty = adecc::grid::HtmlOStreamGridBackend<adecc::AnsiStreamPolicy>;
         using grid_ty = adecc::grid::WriteGridModel<backend_ty>;
         backend_ty theBackend{ os, 10 };
         grid_ty theGrid{ std::move(theBackend), vecAccountCaps, true };
         theGrid += vecAccounts;

      }
      return os.str();
   }

} // end of namespace extented_test