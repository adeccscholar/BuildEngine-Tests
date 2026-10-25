#pragma once

#include "db_test_framework.h"
#include "text_wrapper.h"
#include "stream_wrappers.h"
#include "value_types.h"

#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace database_error_scenario {

   template <adecc::db::logical_database_type db_ty>
   void CreateErrorScenarioTables(db_ty& db) {
      db.ExecuteCommand(
         "DROP TABLE IF EXISTS Test_Error_Transaction\n;"
         "DROP TABLE IF EXISTS Test_Error_Child;\n"
         "DROP TABLE IF EXISTS Test_Error_Parent;\n"
         "DROP TABLE IF EXISTS Test_Error_Check;\n"
         "DROP TABLE IF EXISTS Test_Error_Unique;\n"
         "DROP TABLE IF EXISTS Test_Error_Length;\n"
         "DROP TABLE IF EXISTS Test_Error_Select;\n"
         "DROP TABLE IF EXISTS Test_Error_TupleInsert;\n"
         "DROP TABLE IF EXISTS Test_Error_UpdateVerify;\n"
         "DROP TABLE IF EXISTS Test_Error_DeleteVerify;");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Parent (\n"
         "   Id   int NOT NULL,\n"
         "   Name nvarchar(50) NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_Parent PRIMARY KEY CLUSTERED(Id)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Child (\n"
         "   Id       int IDENTITY(1,1) NOT NULL,\n"
         "   ParentId int NOT NULL,\n"
         "   Text     nvarchar(50) NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_Child PRIMARY KEY CLUSTERED(Id),\n"
         "   CONSTRAINT fk_Test_Error_Child_Parent FOREIGN KEY (ParentId) REFERENCES Test_Error_Parent(Id)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Check (\n"
         "   Id     int IDENTITY(1,1) NOT NULL,\n"
         "   Amount int NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_Check PRIMARY KEY CLUSTERED(Id),\n"
         "   CONSTRAINT ck_Test_Error_Check_Amount_Positive CHECK (Amount > 0)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Unique (\n"
         "   Id   int IDENTITY(1,1) NOT NULL,\n"
         "   Code nvarchar(20) NOT NULL,\n"
         "   CONSTRAINT pk_Test_Error_Unique PRIMARY KEY CLUSTERED(Id),\n"
         "   CONSTRAINT uk_Test_Error_Unique_Code UNIQUE (Code)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Length (\n"
         "   Id   int IDENTITY(1,1) NOT NULL,\n"
         "   Code nvarchar(3) NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_Length PRIMARY KEY CLUSTERED(Id)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Select (\n"
         "   Id     int NOT NULL,\n"
         "   Name   nvarchar(50) NOT NULL,\n"
         "   Amount int NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_Select PRIMARY KEY CLUSTERED(Id)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_TupleInsert (\n"
         "   Id            int NOT NULL,\n"
         "   Name          nvarchar(50) NOT NULL,\n"
         "   Amount        int NOT NULL,\n"
         "   OptionalValue int NULL,\n"

         "   CONSTRAINT pk_Test_Error_TupleInsert PRIMARY KEY CLUSTERED(Id),\n"
         "   CONSTRAINT ck_Test_Error_TupleInsert_Amount CHECK (Amount >= 0)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_UpdateVerify (\n"
         "   Id     int NOT NULL,\n"
         "   Name   nvarchar(50) NOT NULL,\n"
         "   Amount int NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_UpdateVerify PRIMARY KEY CLUSTERED(Id),\n"
         "   CONSTRAINT ck_Test_Error_UpdateVerify_Amount CHECK (Amount >= 0)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_DeleteVerify (\n"
         "   Id     int NOT NULL,\n"
         "   Name   nvarchar(50) NOT NULL,\n"
         "   Amount int NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_DeleteVerify PRIMARY KEY CLUSTERED(Id)\n"
         ")");

      db.ExecuteCommand(
         "CREATE TABLE Test_Error_Transaction (\n"
         "   Id     int NOT NULL,\n"
         "   Name   nvarchar(50) NOT NULL,\n"
         "   Amount int NOT NULL,\n"

         "   CONSTRAINT pk_Test_Error_Transaction PRIMARY KEY CLUSTERED(Id),\n"
         "   CONSTRAINT ck_Test_Error_Transaction_Amount CHECK (Amount >= 0)\n"
         ")");

      db.ExecuteCommand(
         "INSERT INTO Test_Error_Parent (Id, Name) VALUES (1, 'Parent 1');\n"
         "INSERT INTO Test_Error_Parent (Id, Name) VALUES (2, 'Parent 2');\n"
         "INSERT INTO Test_Error_Check (Amount) VALUES (10);\n"
         "INSERT INTO Test_Error_Check (Amount) VALUES (20);\n"
         "INSERT INTO Test_Error_Unique (Code) VALUES ('A');\n"
         "INSERT INTO Test_Error_Unique (Code) VALUES ('B');\n"
         "INSERT INTO Test_Error_Select (Id, Name, Amount) VALUES (1, 'Alpha', 10);\n"
         "INSERT INTO Test_Error_Select (Id, Name, Amount) VALUES (2, 'Beta', 20);\n"
         "INSERT INTO Test_Error_Select (Id, Name, Amount) VALUES (3, 'Gamma', 30);\n"
         "INSERT INTO Test_Error_UpdateVerify (Id, Name, Amount) VALUES (1, 'Before 1', 100);\n"
         "INSERT INTO Test_Error_UpdateVerify (Id, Name, Amount) VALUES (2, 'Before 2', 200);\n"
         "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (1, 'Delete 1', 10);\n"
         "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (2, 'Delete 2', 20);\n"
         "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (3, 'Delete 3', 30);\n"
         "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (4, 'Keep 4', 40);");
   }

   template <adecc::db::logical_database_type db_ty>
   void InsertTupleVectorData(db_ty& db) {
      using database_ty = std::remove_cvref_t<db_ty>;

      using row_ty = std::tuple<int, std::string, int, std::optional<int>>;

      std::vector<row_ty> vecRows{
         { 1, "Tuple Alpha", 11, std::optional<int>{ 101 } },
         { 2, "Tuple Beta",  22, std::optional<int>{} },
         { 3, "Tuple Gamma", 33, std::optional<int>{ 303 } },
         { 4, "Tuple Delta", 44, std::optional<int>{} }
      };

      for (auto const& [iId, strName, iAmount, optOptionalValue] : vecRows) {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_params{
               database_ty::Param("Id", iId),
               database_ty::Param("Name", strName),
               database_ty::Param("Amount", iAmount),
               database_ty::Param("OptionalValue", optOptionalValue, true)
            });
      }
   }

   template <adecc::db::logical_database_type db_ty>
   void RunPositiveScenario(db_ty& db, test_context& ctx) {
      using database_ty = std::remove_cvref_t<db_ty>;

      ExpectDbSuccess(ctx, "Inserting a vector of tuple values", [&] {
         InsertTupleVectorData(db);
         });

      ExpectDbSuccess(ctx, "Number of tuple records inserted", [&] {
         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_TupleInsert");

         ExpectEqual(ctx, "Tuple Insert Count == 4", iCount, 4);
         });

      ExpectDbSuccess(ctx, "Number of records with OptionalValue IS NULL", [&] {
         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_TupleInsert "
            "WHERE OptionalValue IS NULL");

         ExpectEqual(ctx, "Tuple NULL Count == 2", iCount, 2);
         });

      ExpectDbSuccess(ctx, "SELECT with multiple result sets", [&] {
         auto const vecAmounts = SelectValues<int>(db,
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "WHERE Amount >= :MinAmount "
            "ORDER BY Id",
            adecc::db_params{
               database_ty::Param("MinAmount", 20)
            });

         ExpectEqual(ctx, "SELECT Number of results == 3", static_cast<int>(vecAmounts.size()), 3);
         ExpectEqual(ctx, "SELECT 1st result == 22", vecAmounts.at(0), 22);
         ExpectEqual(ctx, "SELECT 2nd result == 33", vecAmounts.at(1), 33);
         ExpectEqual(ctx, "SELECT 3rd result == 44", vecAmounts.at(2), 44);
         });

      ExpectDbSuccess(ctx, "UPDATE Check the result", [&] {
         ExecuteStatement(db,
            "UPDATE Test_Error_UpdateVerify "
            "SET Name = :Name, Amount = :Amount "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("Name", std::string { "After 1" }),
               database_ty::Param("Amount", 777),
               database_ty::Param("keyID", 1)
            });

         auto const iAmount = SelectSingleValue<int>(db,
            "SELECT Amount "
            "FROM Test_Error_UpdateVerify "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("keyID", 1)
            });

         ExpectEqual(ctx, "UPDATE Amount == 777", iAmount, 777);
         });

      ExpectDbSuccess(ctx, "UPDATE applies to only one record", [&] {
         auto const iUnchanged = SelectSingleValue<int>(db,
            "SELECT Amount "
            "FROM Test_Error_UpdateVerify "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("keyID", 2)
            });

         ExpectEqual(ctx, "2nd record unchanged == 200", iUnchanged, 200);
         });

      ExpectDbSuccess(ctx, "DELETE individual records and check counts", [&] {
         ExecuteStatement(db,
            "DELETE FROM Test_Error_DeleteVerify "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("keyID", 2)
            });

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_DeleteVerify");

         ExpectEqual(ctx, "DELETE Remaining quantity == 3", iCount, 3);
         });

      ExpectDbSuccess(ctx, "DELETE range and check count", [&] {
         ExecuteStatement(db,
            "DELETE FROM Test_Error_DeleteVerify "
            "WHERE Amount >= :MinAmount",
            adecc::db_params{
               database_ty::Param("MinAmount", 30)
            });

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_DeleteVerify");

         ExpectEqual(ctx, "DELETE range, remaining quantity == 1", iCount, 1);
         });

      ExpectDbSuccess(ctx, "An optional parameter may be omitted in the SQL statement", [&] {
         auto const iValue = SelectSingleValue<int>(db,
            "SELECT CAST(:Wert AS int) AS Wert "
            "FROM Test_Error_Select "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("Wert", std::string { "123" }),
               database_ty::Param("keyID", 1),
               database_ty::Param("AddValue", std::optional<int> {}, false)
            });

         ExpectEqual(ctx, "Optional missing parameter Result == 123", iValue, 123);
         });

      ExpectDbSuccess(ctx, "Case-insensitive parameter binding of 'keyID' to 'keyId'", [&] {
         auto const iValue = SelectSingleValue<int>(db,
            "SELECT Amount "
            "FROM Test_Error_Select "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("keyID", 2)
            });

         ExpectEqual(ctx, "keyID/keyId result == 20", iValue, 20);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::distance counts result sets", [&] {
         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto const iCount = static_cast<int>(std::ranges::distance(rngRows));

         ExpectEqual(ctx, "Range SELECT Count == 4", iCount, 4);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::copy in vector", [&] {
         std::vector<std::tuple<int>> vecAmounts;

         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         std::ranges::copy(rngRows, std::back_inserter(vecAmounts));

         ExpectEqual(ctx, "copy result count == 4", static_cast<int>(vecAmounts.size()), 4);
         ExpectEqual(ctx, "copy 1st result == 11", vecAmounts.at(0), 11);
         ExpectEqual(ctx, "copy 2nd result == 22", vecAmounts.at(1), 22);
         ExpectEqual(ctx, "copy 3rd result == 33", vecAmounts.at(2), 33);
         ExpectEqual(ctx, "copy 4th result == 44", vecAmounts.at(3), 44);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::views::transform and std::ranges::copy into value vector", [&] {
         std::vector<int> vecAmounts;

         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto rngAmounts = rngRows | std::views::transform([](auto const& tupRow) {
            return std::get<0>(tupRow);
            });

         std::ranges::copy(rngAmounts, std::back_inserter(vecAmounts));

         ExpectEqual(ctx, "copy transformed result count == 4", static_cast<int>(vecAmounts.size()), 4);
         ExpectEqual(ctx, "copy transformed 1st result == 11", vecAmounts.at(0), 11);
         ExpectEqual(ctx, "copy transformed 2nd result == 22", vecAmounts.at(1), 22);
         ExpectEqual(ctx, "copy transformed 3rd result == 33", vecAmounts.at(2), 33);
         ExpectEqual(ctx, "copy transformed 4th result == 44", vecAmounts.at(3), 44);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::for_each sums values", [&] {
         int iSum{};

         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         std::ranges::for_each(rngRows, [&](auto const& tupRow) {
            iSum += std::get<0>(tupRow);
            });

         ExpectEqual(ctx, "for_each sum == 110", iSum, 110);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::find_if finds value", [&] {
         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto itFound = std::ranges::find_if(rngRows, [](auto const& tupRow) {
            return std::get<0>(tupRow) == 33;
            });

         if (itFound == std::default_sentinel) {
            throw std::runtime_error{ "Value 33 was not found" };
         }

         ExpectEqual(ctx, "find_if found value == 33", std::get<0>(*itFound), 33);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::count_if counts matching rows", [&] {
         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto const iCount = static_cast<int>(
            std::ranges::count_if(rngRows, [](auto const& tupRow) {
               return std::get<0>(tupRow) >= 30;
               }));

         ExpectEqual(ctx, "count_if rows >= 30 == 2", iCount, 2);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::all_of validates positive values", [&] {
         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto const boAllPositive = std::ranges::all_of(rngRows, [](auto const& tupRow) {
            return std::get<0>(tupRow) > 0;
            });

         ExpectEqual(ctx, "all_of all values positive == true", boAllPositive, true);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::any_of validates high value", [&] {
         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto const boAnyHighValue = std::ranges::any_of(rngRows, [](auto const& tupRow) {
            return std::get<0>(tupRow) == 44;
            });

         ExpectEqual(ctx, "any_of value 44 exists == true", boAnyHighValue, true);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::ranges::none_of validates missing negative values", [&] {
         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto const boNoNegativeValue = std::ranges::none_of(rngRows, [](auto const& tupRow) {
            return std::get<0>(tupRow) < 0;
            });

         ExpectEqual(ctx, "none_of negative values == true", boNoNegativeValue, true);
         });

      ExpectDbSuccess(ctx, "Range SELECT: std::views::filter over query range", [&] {
         std::vector<int> vecFiltered;

         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto rngFiltered =
            rngRows
            | std::views::filter([](auto const& tupRow) {
            return std::get<0>(tupRow) >= 30;
               })
            | std::views::transform([](auto const& tupRow) {
            return std::get<0>(tupRow);
               });

         std::ranges::copy(rngFiltered, std::back_inserter(vecFiltered));

         ExpectEqual(ctx, "filter result count == 2", static_cast<int>(vecFiltered.size()), 2);
         ExpectEqual(ctx, "filter 1st result == 33", vecFiltered.at(0), 33);
         ExpectEqual(ctx, "filter 2nd result == 44", vecFiltered.at(1), 44);
         });

      ExpectDbSuccess(ctx, "Range SELECT: filter and transform combined", [&] {
         std::vector<int> vecProcessed;

         auto rngRows = db.template Execute<int>(
            "SELECT Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         auto rngProcessed = rngRows
            | std::views::filter([](auto const& tupRow) {
            return std::get<0>(tupRow) > 20;
               })
            | std::views::transform([](auto const& tupRow) {
            return std::get<0>(tupRow) + 1;
               });

         std::ranges::copy(rngProcessed, std::back_inserter(vecProcessed));

         ExpectEqual(ctx, "filter transform result count == 3", static_cast<int>(vecProcessed.size()), 3);
         ExpectEqual(ctx, "filter transform 1st result == 23", vecProcessed.at(0), 23);
         ExpectEqual(ctx, "filter transform 2nd result == 34", vecProcessed.at(1), 34);
         ExpectEqual(ctx, "filter transform 3rd result == 45", vecProcessed.at(2), 45);
         });

      ExpectDbSuccess(ctx, "Range SELECT: tuple range with multiple columns", [&] {
         std::vector<std::tuple<int, std::string, int>> vecRows;

         auto rngRows = db.template Execute<int, std::string, int>(
            "SELECT Id, Name, Amount "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         std::ranges::copy(rngRows, std::back_inserter(vecRows));

         ExpectEqual(ctx, "multi column tuple count == 4", static_cast<int>(vecRows.size()), 4);
         ExpectEqual(ctx, "multi column 1st id == 1", std::get<0>(vecRows.at(0)), 1);
         ExpectEqual(ctx, "multi column 1st name == Tuple Alpha", std::get<1>(vecRows.at(0)), std::string{ "Tuple Alpha" });
         ExpectEqual(ctx, "multi column 1st amount == 11", std::get<2>(vecRows.at(0)), 11);
         });

      ExpectDbSuccess(ctx, "Range SELECT: optional column values", [&] {
         std::vector<std::tuple<int, std::optional<int>>> vecRows;

         auto rngRows = db.template Execute<int, std::optional<int>>(
            "SELECT Id, OptionalValue "
            "FROM Test_Error_TupleInsert "
            "ORDER BY Id");

         std::ranges::copy(rngRows, std::back_inserter(vecRows));

         ExpectEqual(ctx, "optional column result count == 4", static_cast<int>(vecRows.size()), 4);
         ExpectEqual(ctx, "optional column 1st row has value == true", std::get<1>(vecRows.at(0)).has_value(), true);
         ExpectEqual(ctx, "optional column 2nd row is null == true", !std::get<1>(vecRows.at(1)).has_value(), true);
         ExpectEqual(ctx, "optional column 3rd row has value == true", std::get<1>(vecRows.at(2)).has_value(), true);
         ExpectEqual(ctx, "optional column 4th row is null == true", !std::get<1>(vecRows.at(3)).has_value(), true);
         });

      ExpectDbSuccess(ctx, "OutputSink: range assignment with tuple vector", [&] {
         using row_ty = std::tuple<int, std::string, int, std::optional<int>>;

         db.ExecuteCommand("DELETE FROM Test_Error_TupleInsert");

         std::vector<row_ty> vecRows{
            { 10, "Sink Alpha", 100, std::optional<int>{ 1000 } },
            { 20, "Sink Beta",  200, std::optional<int>{} },
            { 30, "Sink Gamma", 300, std::optional<int>{ 3000 } }
         };

         auto aSink = db.template MakeOutputSink<int, std::string, int, std::optional<int>>(
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_output_parameters{
               { "Id", adecc::db_output_param_role::needed_value },
               { "Name", adecc::db_output_param_role::needed_value },
               { "Amount", adecc::db_output_param_role::needed_value },
               { "OptionalValue", adecc::db_output_param_role::needed_value }
            });

         aSink = vecRows;

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_TupleInsert");

         ExpectEqual(ctx, "output sink assignment row count == 3", iCount, 3);
         });

      ExpectDbSuccess(ctx, "OutputSink: std::ranges::copy with output iterator", [&] {
         using row_ty = std::tuple<int, std::string, int, std::optional<int>>;

         db.ExecuteCommand("DELETE FROM Test_Error_TupleInsert");

         std::vector<row_ty> vecRows{
            { 101, "Copy Alpha", 10, std::optional<int>{} },
            { 102, "Copy Beta",  20, std::optional<int>{ 2020 } },
            { 103, "Copy Gamma", 30, std::optional<int>{} }
         };

         auto aSink = db.template MakeOutputSink<int, std::string, int, std::optional<int>>(
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_output_parameters{
               { "Id", adecc::db_output_param_role::needed_value },
               { "Name", adecc::db_output_param_role::needed_value },
               { "Amount", adecc::db_output_param_role::needed_value },
               { "OptionalValue", adecc::db_output_param_role::needed_value }
            });

         std::ranges::copy(vecRows, aSink.OutputIterator());

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_TupleInsert");

         auto const iNullCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_TupleInsert "
            "WHERE OptionalValue IS NULL");

         ExpectEqual(ctx, "output iterator copy row count == 3", iCount, 3);
         ExpectEqual(ctx, "output iterator copy null count == 2", iNullCount, 2);
         });

      ExpectDbSuccess(ctx, "OutputSink: std::views::transform as input range", [&] {
         using src_ty = std::tuple<int, std::string, int>;
         using dst_ty = std::tuple<int, std::string, int, std::optional<int>>;

         db.ExecuteCommand("DELETE FROM Test_Error_TupleInsert");

         std::vector<src_ty> vecSource{
            { 201, "Transform Alpha", 1 },
            { 202, "Transform Beta",  2 },
            { 203, "Transform Gamma", 3 }
         };

         auto rngTransformed = vecSource | std::views::transform([](src_ty const& tupValue) -> dst_ty {
            return {
               std::get<0>(tupValue),
               std::get<1>(tupValue),
               std::get<2>(tupValue) * 10,
               std::optional<int>{}
            };
            });

         auto aSink = db.template MakeOutputSink<int, std::string, int, std::optional<int>>(
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_output_parameters{
               { "Id", adecc::db_output_param_role::needed_value },
               { "Name", adecc::db_output_param_role::needed_value },
               { "Amount", adecc::db_output_param_role::needed_value },
               { "OptionalValue", adecc::db_output_param_role::needed_value }
            });

         aSink(rngTransformed);

         auto const iSum = SelectSingleValue<int>(db,
            "SELECT SUM(Amount) "
            "FROM Test_Error_TupleInsert");

         ExpectEqual(ctx, "output sink transform sum == 60", iSum, 60);
         });

      ExpectDbSuccess(ctx, "OutputSink: range with std::views::filter", [&] {
         using row_ty = std::tuple<int, std::string, int, std::optional<int>>;

         db.ExecuteCommand("DELETE FROM Test_Error_TupleInsert");

         std::vector<row_ty> vecRows{
            { 401, "Filter Alpha", 10, std::optional<int>{} },
            { 402, "Filter Beta",  20, std::optional<int>{} },
            { 403, "Filter Gamma", 30, std::optional<int>{} },
            { 404, "Filter Delta", 40, std::optional<int>{} }
         };

         auto rngFiltered = vecRows | std::views::filter([](row_ty const& tupValue) {
            return std::get<2>(tupValue) >= 30;
            });

         auto aSink = db.template MakeOutputSink<int, std::string, int, std::optional<int>>(
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_output_parameters{
               { "Id", adecc::db_output_param_role::needed_value },
               { "Name", adecc::db_output_param_role::needed_value },
               { "Amount", adecc::db_output_param_role::needed_value },
               { "OptionalValue", adecc::db_output_param_role::needed_value }
            });

         aSink(rngFiltered);

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_TupleInsert");

         auto const iMinAmount = SelectSingleValue<int>(db,
            "SELECT MIN(Amount) "
            "FROM Test_Error_TupleInsert");

         ExpectEqual(ctx, "output sink filter row count == 2", iCount, 2);
         ExpectEqual(ctx, "output sink filter min amount == 30", iMinAmount, 30);
         });

      ExpectDbSuccess(ctx, "Range DELETE: collect ids from query and delete rows", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         db.ExecuteCommand("DELETE FROM Test_Error_DeleteVerify");
         db.ExecuteCommand(
            "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (10, 'Delete A', 10);\n"
            "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (20, 'Delete B', 20);\n"
            "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (30, 'Delete C', 30);\n"
            "INSERT INTO Test_Error_DeleteVerify (Id, Name, Amount) VALUES (40, 'Keep D', 40);");

         std::vector<int> vecIds;

         auto rngIds =
            db.template Execute<int>(
               "SELECT Id "
               "FROM Test_Error_DeleteVerify "
               "WHERE Amount < :MaxAmount "
               "ORDER BY Id",
               adecc::db_params{
                  database_ty::Param("MaxAmount", 40)
               })
            | std::views::transform([](auto const& tupRow) {
            return std::get<0>(tupRow);
               });

         std::ranges::copy(rngIds, std::back_inserter(vecIds));

         std::ranges::for_each(vecIds, [&](int const iId) {
            ExecuteStatement(db,
               "DELETE FROM Test_Error_DeleteVerify "
               "WHERE Id = :keyId",
               adecc::db_params{
                  database_ty::Param("keyID", iId)
               });
            });

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_DeleteVerify");

         auto const iRemainingId = SelectSingleValue<int>(db,
            "SELECT Id "
            "FROM Test_Error_DeleteVerify");

         ExpectEqual(ctx, "range delete remaining row count == 1", iCount, 1);
         ExpectEqual(ctx, "range delete remaining id == 40", iRemainingId, 40);
         });

      ExpectDbSuccess(ctx, "Transaction: Commit(false) persists inserted row", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         {
            auto aTransaction = db.Transaction();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 1),
                  database_ty::Param("Name", std::string{ "Commit false" }),
                  database_ty::Param("Amount", 100)
               });

            aTransaction.Commit(false);
         }

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_Transaction "
            "WHERE Id = :Id",
            adecc::db_params{
               database_ty::Param("Id", 1)
            });

         ExpectEqual(ctx, "commit false row count == 1", iCount, 1);
         });

      ExpectDbSuccess(ctx, "Transaction: explicit Rollback removes inserted row", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         {
            auto aTransaction = db.Transaction();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 2),
                  database_ty::Param("Name", std::string{ "Rollback" }),
                  database_ty::Param("Amount", 200)
               });

            aTransaction.Rollback();
         }

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_Transaction "
            "WHERE Id = :Id",
            adecc::db_params{
               database_ty::Param("Id", 2)
            });

         ExpectEqual(ctx, "rollback row count == 0", iCount, 0);
         });

      ExpectDbSuccess(ctx, "Transaction: destructor rolls back active transaction", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         {
            auto aTransaction = db.Transaction();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 3),
                  database_ty::Param("Name", std::string{ "Destructor rollback" }),
                  database_ty::Param("Amount", 300)
               });

            // Kein Commit, kein expliziter Rollback.
         }

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_Transaction "
            "WHERE Id = :Id",
            adecc::db_params{
               database_ty::Param("Id", 3)
            });

         ExpectEqual(ctx, "destructor rollback row count == 0", iCount, 0);
         });

      ExpectDbSuccess(ctx, "Transaction: Commit(true) starts a new transaction and destructor rolls back second part", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         {
            auto aTransaction = db.Transaction();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 4),
                  database_ty::Param("Name", std::string{ "Committed part" }),
                  database_ty::Param("Amount", 400)
               });

            aTransaction.Commit();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 5),
                  database_ty::Param("Name", std::string{ "Rolled back part" }),
                  database_ty::Param("Amount", 500)
               });

            // Die zweite Transaktion bleibt aktiv und muss im Destruktor zurückgerollt werden.
         }

         auto const iCommittedCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_Transaction "
            "WHERE Id = :Id",
            adecc::db_params{
               database_ty::Param("Id", 4)
            });

         auto const iRolledBackCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_Transaction "
            "WHERE Id = :Id",
            adecc::db_params{
               database_ty::Param("Id", 5)
            });

         ExpectEqual(ctx, "commit true first row count == 1", iCommittedCount, 1);
         ExpectEqual(ctx, "commit true second row count == 0", iRolledBackCount, 0);
         });

      ExpectDbSuccess(ctx, "Transaction: multiple Commit(true) calls persist transaction chunks", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         {
            auto aTransaction = db.Transaction();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 6),
                  database_ty::Param("Name", std::string{ "Chunk 1" }),
                  database_ty::Param("Amount", 600)
               });

            aTransaction.Commit();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 7),
                  database_ty::Param("Name", std::string{ "Chunk 2" }),
                  database_ty::Param("Amount", 700)
               });

            aTransaction.Commit();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 8),
                  database_ty::Param("Name", std::string{ "Chunk 3" }),
                  database_ty::Param("Amount", 800)
               });

            aTransaction.Commit(false);
         }

         auto const iCount = SelectCount(db,
            "SELECT COUNT(*) "
            "FROM Test_Error_Transaction "
            "WHERE Id IN (6, 7, 8)");

         ExpectEqual(ctx, "multiple commit true row count == 3", iCount, 3);
         });
   }




   template <class db_ty>
   void RunErrorScenario(db_ty& db, test_context& ctx) {
      using database_ty = std::remove_cvref_t<db_ty>;

      ExpectDbError(ctx, "SELECT Conversion error during fetch", [&] {
         SelectValues<int>(db,
            "SELECT CAST(:Wert AS int) AS Wert "
            "FROM Test_Error_Select "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("Wert", std::string { "kein-int" }),
               database_ty::Param("keyID", 1),
               database_ty::Param("AddValue", std::optional<int> {}, false)
            });
         });

      ExpectDbError(ctx, "INSERT violates NOT NULL", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_Parent (Id, Name) "
            "VALUES (:Id, :Name)",
            adecc::db_params{
               database_ty::Param("Id", 3),
               database_ty::Param("Name", std::optional<std::string> {}, true)
            });
         });

      ExpectDbError(ctx, "INSERT violates CHECK Constraint", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_Check (Amount) "
            "VALUES (:Amount)",
            adecc::db_params{
               database_ty::Param("Amount", -1)
            });
         });

      ExpectDbError(ctx, "UPDATE violates CHECK Constraint", [&] {
         ExecuteStatement(db,
            "UPDATE Test_Error_Check "
            "SET Amount = :Amount "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("Amount", -5),
               database_ty::Param("keyID", 1)
            });
         });

      ExpectDbError(ctx, "DELETE violates FOREIGN KEY", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_Child (ParentId, Text) "
            "VALUES (:ParentId, :Text)",
            adecc::db_params{
               database_ty::Param("ParentId", 1),
               database_ty::Param("Text", std::string { "child keeps parent" })
            });

         ExecuteStatement(db,
            "DELETE FROM Test_Error_Parent "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("keyID", 1)
            });
         });

      ExpectDbError(ctx, "INSERT violates FOREIGN KEY", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_Child (ParentId, Text) "
            "VALUES (:ParentId, :Text)",
            adecc::db_params{
               database_ty::Param("ParentId", 999999),
               database_ty::Param("Text", std::string { "orphan" })
            });
         });

      ExpectDbError(ctx, "INSERT violates UNIQUE Constraint", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_Unique (Code) "
            "VALUES (:Code)",
            adecc::db_params{
               database_ty::Param("Code", std::string { "A" })
            });
         });

      ExpectDbError(ctx, "Required parameters are not present in the SQL", [&] {
         SelectValues<int>(db,
            "SELECT CAST(:Wert AS int) AS Wert "
            "FROM Test_Error_Select "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("Wert", std::string { "123" }),
               database_ty::Param("keyID", 1),
               database_ty::Param("AddValue", std::optional<int> {}, true)
            });
         });

      ExpectDbError(ctx, "SQL is missing a parameter", [&] {
         SelectValues<int>(db,
            "SELECT CAST(:MissingValue AS int) AS Wert "
            "FROM Test_Error_Select "
            "WHERE Id = :keyId",
            adecc::db_params{               database_ty::Param("keyID", 1)
            });
         });

      ExpectDbError(ctx, "Invalid SQL expression", [&] {
         SelectValues<int>(db,
            "SELECT CAST(:Wert AS int) AS Wert "
            "FROM Test_Error_Select "
            "WHERE Id = :keyId "
            "THIS_IS_NOT_VALID_SQL",
            adecc::db_params{
               database_ty::Param("Wert", std::string { "123" }),
               database_ty::Param("keyID", 1)
            });
         });

      ExpectDbError(ctx, "Tuple Insert violates CHECK Constraint", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_params{
               database_ty::Param("Id", 100),
               database_ty::Param("Name", std::string { "Invalid Tuple" }),
               database_ty::Param("Amount", -100),
               database_ty::Param("OptionalValue", std::optional<int> {}, true)
            });
         });

      ExpectDbError(ctx, "Tuple Insert violates PRIMARY KEY", [&] {
         ExecuteStatement(db,
            "DELETE FROM Test_Error_TupleInsert "
            "WHERE Id = :keyId",
            adecc::db_params{
               database_ty::Param("keyID", 999)
            });

         ExecuteStatement(db,
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_params{
               database_ty::Param("Id", 999),
               database_ty::Param("Name", std::string { "Primary Key Source" }),
               database_ty::Param("Amount", 999),
               database_ty::Param("OptionalValue", std::optional<int> {}, true)
            });

         ExecuteStatement(db,
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_params{
               database_ty::Param("Id", 999),
               database_ty::Param("Name", std::string { "Duplicate Id" }),
               database_ty::Param("Amount", 1000),
               database_ty::Param("OptionalValue", std::optional<int> {}, true)
            });
         });

      ExpectDbError(ctx, "String too long for column", [&] {
         ExecuteStatement(db,
            "INSERT INTO Test_Error_Length (Code) "
            "VALUES (:Code)",
            adecc::db_params{
               database_ty::Param("Code", std::string { "ABCDE" })
            });
         });

      ExpectDbError(ctx, "Range SELECT: error propagates through std::ranges::copy", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         std::vector<int> vecValues;

         auto rngValues =
            db.template Execute<int>(
               "SELECT CAST(:Wert AS int) AS Wert "
               "FROM Test_Error_Select "
               "WHERE Id = :keyId",
               adecc::db_params{
                  database_ty::Param("Wert", std::string{ "kein-int" }),
                  database_ty::Param("keyID", 1)
               })
            | std::views::transform([](auto const& tupRow) {
            return std::get<0>(tupRow);
               });

         std::ranges::copy(rngValues, std::back_inserter(vecValues));
         });

      ExpectDbError(ctx, "OutputSink: error propagates through std::ranges::copy", [&] {
         using row_ty = std::tuple<int, std::string, int, std::optional<int>>;

         std::vector<row_ty> vecRows{
            { 501, "Good", 10, std::optional<int>{} },
            { 502, "Bad", -20, std::optional<int>{} },
            { 503, "Never", 30, std::optional<int>{} }
         };

         auto aSink = db.template MakeOutputSink<int, std::string, int, std::optional<int>>(
            "INSERT INTO Test_Error_TupleInsert (Id, Name, Amount, OptionalValue) "
            "VALUES (:Id, :Name, :Amount, :OptionalValue)",
            adecc::db_output_parameters{
               { "Id", adecc::db_output_param_role::needed_value },
               { "Name", adecc::db_output_param_role::needed_value },
               { "Amount", adecc::db_output_param_role::needed_value },
               { "OptionalValue", adecc::db_output_param_role::needed_value }
            });

         std::ranges::copy(vecRows, aSink.OutputIterator());
         });

      ExpectDbError(ctx, "Transaction: failed statement keeps exception visible and scope rolls back", [&] {
         using database_ty = std::remove_cvref_t<db_ty>;

         {
            auto aTransaction = db.Transaction();

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 9),
                  database_ty::Param("Name", std::string{ "Before error" }),
                  database_ty::Param("Amount", 900)
               });

            ExecuteStatement(db,
               "INSERT INTO Test_Error_Transaction (Id, Name, Amount) "
               "VALUES (:Id, :Name, :Amount)",
               adecc::db_params{
                  database_ty::Param("Id", 10),
                  database_ty::Param("Name", std::string{ "Invalid" }),
                  database_ty::Param("Amount", -1)
               });

            aTransaction.Commit(false);
         }
         });
   }

   template <class db_ty>
   std::vector<test_result> RunDatabaseErrorScenario(db_ty& db, std::ostream& out, bool const boPrintExceptionResults = false) {
      test_context ctx{ out };

      std::println(out, "");
      std::println(out, "Start Database Error Scenario.");

      ExpectDbSuccess(ctx, "Create tables and insert basic data", [&] {
         CreateErrorScenarioTables(db);
         });

      RunPositiveScenario(db, ctx);
      RunErrorScenario(db, ctx);

      ctx.Summary(boPrintExceptionResults);
      return std::move(ctx).TakeResults();
   }

} // namespace database_error_scenario