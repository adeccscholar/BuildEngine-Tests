#pragma once
#include "database.h"
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace buildengine_tests::database::additional {
   template<class DB, class Dialect>
   int Run(DB& db, Dialect const& dialect) {
      int passed = 0;
      auto require = [&](bool ok, char const* what) {
         if(!ok) throw std::runtime_error{what};
         ++passed;
         std::cout << "[PASS] " << what << '\n';
      };
      // Schema DDL alone is backend-specific; the C++ exercises are shared.
      dialect.Create(db);
      try {
         using db_ty = std::remove_cvref_t<DB>;
         using row_ty = std::tuple<int, std::string, int, std::optional<int>>;
         std::vector<row_ty> values{
            {1,"Alpha",10,100}, {2,"Beta",20,std::nullopt},
            {3,"Gamma",30,300}, {4,"Delta",40,std::nullopt}
         };
         auto output = db.template MakeOutputRange<int,std::string,int,std::optional<int>>(
            "INSERT INTO Test_Error_TupleInsert (Id,Name,Amount,OptionalValue) "
            "VALUES (:Id,:Name,:Amount,:OptionalValue)",
            {{"Id",adecc::db_output_param_role::needed_value},
             {"Name",adecc::db_output_param_role::needed_value},
             {"Amount",adecc::db_output_param_role::needed_value},
             {"OptionalValue",adecc::db_output_param_role::needed_value}});
         std::size_t inserted = 0;
         for(auto const& row: output(values)) {
            (void)row;
            ++inserted;
         }
         require(inserted == values.size(), "OutputRange tuple insertion");
         auto count = [&](std::string sql) {
            auto result = db.template ExecuteOne<int>(sql);
            if(!result) throw std::runtime_error{"SELECT COUNT returned no row"};
            return std::get<0>(*result);
         };
         require(count("SELECT COUNT(*) FROM Test_Error_TupleInsert") == 4,
                 "logical ExecuteOne count");
         require(count("SELECT COUNT(*) FROM Test_Error_TupleInsert WHERE OptionalValue IS NULL") == 2,
                 "optional NULL values");
         std::vector<int> amounts;
         for(auto const& item: db.template Execute<int>(
            "SELECT Amount FROM Test_Error_TupleInsert "
            "WHERE Amount >= :MinAmount ORDER BY Id",
            db_ty::Params(db_ty::Param("MinAmount",20)))) {
            amounts.push_back(std::get<0>(item));
         }
         require(amounts == std::vector<int>{20,30,40},
                 "named parameters and Execute range");
         bool caught = false;
         try {
            db.ExecuteCommand("INSERT INTO Test_Error_TupleInsert "
               "(Id,Name,Amount) VALUES (1,'Duplicate',10)");
         } catch(std::exception const&) { caught = true; }
         require(caught,"primary key diagnostic");
         dialect.ResetAfterExpectedError(db);
         caught = false;
         try {
            db.ExecuteCommand("INSERT INTO Test_Error_TupleInsert "
               "(Id,Name,Amount) VALUES (99,'Invalid',-1)");
         } catch(std::exception const&) { caught = true; }
         require(caught,"check constraint diagnostic");
         dialect.ResetAfterExpectedError(db);
      }
      catch(...) {
         dialect.Drop(db);
         throw;
      }
      dialect.Drop(db);
      return passed;
   }
} // namespace buildengine_tests::database::additional
