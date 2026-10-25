// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

/**
\file test_postgresql_pqxx_database.cpp
\brief Integration test for the adecc PostgreSQL/libpqxx backend.
*/

#include "database.h"
#include "database_checks.h"
#include "diagnostic_text.h"
#include "pqxx_database.h"

#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono;
namespace common = buildengine_tests::database::common;

using physical_database_ty = adecc::db::postgres::postgres_database;
using database_ty = adecc::db::logical_database<
   physical_database_ty,
   adecc::db::postgres::query
   >;

using row_ty = std::tuple<
   long long,
   std::string,
   std::optional<std::string>,
   std::wstring,
   adecc::money_ty,
   double,
   adecc::timestamp_ty,
   adecc::date_ty,
   adecc::time_ty,
   bool,
   std::optional<long long>
   >;

struct test_config {
   std::string   strHost{ "localhost" };
   std::uint16_t uPort{ 5432 };
   std::string   strDatabase{ "BuildEngineTest" };
   std::string   strUser{ "buildenginetest_user" };
   std::string   strPassword{};
   bool          boIntegrated{ true };
   };

using common::Fail;
using common::Require;

using common::GetEnvironment;
using common::ParseBoolean;

std::uint16_t ParsePort(std::string_view const svValue) {
   unsigned int uPort{};
   auto const [pEnd, ec] = std::from_chars(
      svValue.data(),
      svValue.data() + svValue.size(),
      uPort
      );

   if (ec != std::errc{} ||
       pEnd != svValue.data() + svValue.size() ||
       uPort == 0 ||
       uPort > 65535) {
      Fail(std::format("invalid PostgreSQL port '{}'", svValue));
      }

   return static_cast<std::uint16_t>(uPort);
   }


void PrintUsage(char const* const szProgram) {
   std::cout
      << "Usage:\n"
      << "  " << szProgram
      << " --host <host> --database <database> --user <user> [--integrated]\n"
      << "  " << szProgram
      << " --host <host> --database <database> --user <user> "
         "--password-login --password <password>\n";
   }


test_config ParseArguments(int const iArgc, char* const* const argv) {
   test_config aConfig{
      .strHost = GetEnvironment("BUILDENGINE_PGHOST", "localhost"),
      .uPort = ParsePort(GetEnvironment("BUILDENGINE_PGPORT", "5432")),
      .strDatabase = GetEnvironment("BUILDENGINE_PGDATABASE", "BuildEngineTest"),
      .strUser = GetEnvironment("BUILDENGINE_PGUSER", "buildenginetest_user"),
      .strPassword = GetEnvironment("BUILDENGINE_PGPASSWORD"),
      .boIntegrated = ParseBoolean(
         GetEnvironment("BUILDENGINE_PG_INTEGRATED"),
         true
         )
      };

   auto fnNeedValue = [&](int& iIndex, std::string_view const svOption) -> std::string {
      if (iIndex + 1 >= iArgc) {
         Fail(std::format("missing value for {}", svOption));
         }

      ++iIndex;
      return argv[iIndex];
      };

   for (int iIndex{ 1 }; iIndex < iArgc; ++iIndex) {
      std::string_view const svArg{ argv[iIndex] };

      if (svArg == "--help" || svArg == "-h") {
         PrintUsage(argv[0]);
         std::exit(0);
         }
      else if (svArg == "--host") {
         aConfig.strHost = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--port") {
         aConfig.uPort = ParsePort(fnNeedValue(iIndex, svArg));
         }
      else if (svArg == "--database") {
         aConfig.strDatabase = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--user") {
         aConfig.strUser = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--password") {
         aConfig.strPassword = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--integrated") {
         aConfig.boIntegrated = true;
         }
      else if (svArg == "--password-login") {
         aConfig.boIntegrated = false;
         }
      else {
         Fail(std::format("unknown argument '{}'", svArg));
         }
      }

   if (aConfig.strHost.empty()) {
      Fail("PostgreSQL host is missing");
      }

   if (aConfig.strDatabase.empty()) {
      Fail("PostgreSQL database is missing");
      }

   if (aConfig.strUser.empty()) {
      Fail("PostgreSQL user is missing");
      }

   if (!aConfig.boIntegrated && aConfig.strPassword.empty()) {
      Fail("password login requires --password or BUILDENGINE_PGPASSWORD");
      }

   return aConfig;
   }


using common::MakeSuffix;

struct database_object_guard {
   database_ty* pDatabase{};
   std::string strTable{};

   ~database_object_guard() {
      if (pDatabase == nullptr || strTable.empty()) {
         return;
         }

      try {
         pDatabase->ExecuteCommand(std::format("DROP TABLE IF EXISTS {}", strTable));
         }
      catch (...) {
         }
      }
   };


using common::MakeDate;
using common::MakeTimestamp;
using common::MakeTime;
using common::RequireOne;
using common::CheckRow;

} // namespace


int main(int const iArgc, char* const* const argv) {
   try {
      test_config const aConfig = ParseArguments(iArgc, argv);

      adecc::db::postgres::postgres_credentials const aCredentials{
         .strHost = aConfig.strHost,
         .uPort = aConfig.uPort,
         .strDatabase = aConfig.strDatabase,
         .strUser = aConfig.strUser,
         .strPassword = aConfig.strPassword,
         .boIntegrated = aConfig.boIntegrated,
         .strSslMode = "prefer",
         .strGssEncMode = "disable",
         .strKrbSrvName = "postgres",
         .strGssLib = {},
         .strApplicationName = "BuildEngineTest-PostgreSQL-Backend-Test",
         .iConnectTimeout = 30
         };

      std::cout << "[TEST] connect to "
                << aConfig.strHost
                << ':'
                << aConfig.uPort
                << " / "
                << aConfig.strDatabase
                << " as "
                << aConfig.strUser
                << " using "
                << (aConfig.boIntegrated ? "integrated authentication" : "password authentication")
                << '\n';

      database_ty theDatabase{ aCredentials };
      Require(theDatabase.Connected(), "database reports not connected after construction");
      std::cout << "[PASS] connection\n";

      std::string const strTable = "adecc_pqxx_data_" + MakeSuffix();

      database_object_guard aCleanup{
         .pDatabase = &theDatabase,
         .strTable = strTable
         };

      theDatabase.ExecuteCommand(std::format(
         "CREATE TEMP TABLE {} ("
         "Id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,"
         "Name varchar(200) NOT NULL UNIQUE,"
         "AlternateName varchar(200) NULL,"
         "WideName text NOT NULL,"
         "Amount numeric(18,2) NOT NULL,"
         "Ratio numeric(18,6) NOT NULL,"
         "CreatedAt timestamp(0) NOT NULL,"
         "BusinessDate date NOT NULL,"
         "BusinessTime time(0) NOT NULL,"
         "Active boolean NOT NULL,"
         "ParentId bigint NULL"
         ") ON COMMIT PRESERVE ROWS",
         strTable
         ));
      std::cout << "[PASS] temporary test table created\n";

      std::string const strInsert = std::format(
         "INSERT INTO {} "
         "(Name, AlternateName, WideName, Amount, Ratio, CreatedAt, "
         " BusinessDate, BusinessTime, Active, ParentId) "
         "VALUES "
         "(:Name, :AlternateName, :WideName, :Amount, :Ratio, :CreatedAt, "
         " :BusinessDate, :BusinessTime, :Active, :ParentId)",
         strTable
         );

      auto theOutput = theDatabase.MakeOutputRange<
         long long,
         std::string,
         std::optional<std::string>,
         std::wstring,
         adecc::money_ty,
         double,
         adecc::timestamp_ty,
         adecc::date_ty,
         adecc::time_ty,
         bool,
         std::optional<long long>
         >(
            strInsert,
            {
               { "Id", adecc::db_output_param_role::identity },
               { "Name", adecc::db_output_param_role::needed_value },
               { "AlternateName", adecc::db_output_param_role::needed_value },
               { "WideName", adecc::db_output_param_role::needed_value },
               { "Amount", adecc::db_output_param_role::needed_value },
               { "Ratio", adecc::db_output_param_role::needed_value },
               { "CreatedAt", adecc::db_output_param_role::needed_value },
               { "BusinessDate", adecc::db_output_param_role::needed_value },
               { "BusinessTime", adecc::db_output_param_role::needed_value },
               { "Active", adecc::db_output_param_role::needed_value },
               { "ParentId", adecc::db_output_param_role::needed_value }
            }
         );

      auto fnInsertOne = [&](row_ty const& aInput) -> row_ty {
         return common::InsertOne(theOutput, aInput);
         };

      adecc::date_ty const aDate1 = MakeDate(2026, 10, 8);
      adecc::timestamp_ty const aTimestamp1 = MakeTimestamp(
         aDate1,
         hours{ 1 },
         minutes{ 2 },
         seconds{ 3 }
         );
      adecc::time_ty const aTime1 = MakeTime(
         hours{ 14 },
         minutes{ 15 },
         seconds{ 16 }
         );

      row_ty aInput1{
         0LL,
         "Muenchen",
         std::nullopt,
         L"M\u00FCnchen \u2013 \u6771\u4EAC",
         adecc::money_ty{ 1234.56 },
         12.125,
         aTimestamp1,
         aDate1,
         aTime1,
         true,
         std::nullopt
         };

      row_ty aInserted1 = fnInsertOne(aInput1);
      long long const iId1 = std::get<0>(aInserted1);
      Require(iId1 > 0, "identity value was not returned");
      std::get<0>(aInput1) = iId1;
      CheckRow(aInserted1, aInput1);
      std::cout << "[PASS] identity and first insert, Id=" << iId1 << '\n';

      auto const aRead1 = RequireOne(
         theDatabase.ExecuteOne<
            long long,
            std::string,
            std::optional<std::string>,
            std::wstring,
            adecc::money_ty,
            double,
            adecc::timestamp_ty,
            adecc::date_ty,
            adecc::time_ty,
            bool,
            std::optional<long long>
            >(
               std::format(
                  "SELECT Id, Name, AlternateName, WideName, Amount, Ratio, CreatedAt, "
                  "BusinessDate, BusinessTime, Active, ParentId "
                  "FROM {} WHERE Id = :Id",
                  strTable
                  ),
               database_ty::Params(database_ty::Param("Id", iId1))
               ),
         "read first inserted row"
         );

      CheckRow(aRead1, aInput1);
      std::cout << "[PASS] narrow, wide, NULL, numeric, timestamp, date, time and bool roundtrip\n";

      common::CheckWideParameters(theDatabase,
         "SELECT CAST(:Value AS text) AS Value");

      adecc::date_ty const aDate2 = MakeDate(2026, 10, 9);
      row_ty aInput2{
         0LL,
         "Child",
         std::optional<std::string>{ "Alternate" },
         L"Kind \u20AC",
         adecc::money_ty{ 42.10 },
         0.125,
         MakeTimestamp(aDate2, hours{ 7 }, minutes{ 8 }, seconds{ 9 }),
         aDate2,
         MakeTime(hours{ 17 }, minutes{ 18 }, seconds{ 19 }),
         false,
         iId1
         };

      row_ty aInserted2 = fnInsertOne(aInput2);
      long long const iId2 = std::get<0>(aInserted2);
      Require(iId2 > iId1, "second identity value is not greater than first value");
      std::get<0>(aInput2) = iId2;
      CheckRow(aInserted2, aInput2);

      common::CheckRepeatedParameters(theDatabase,
         strTable, iId1, iId2);

      common::CheckTransactions(theDatabase, theOutput, aInput1,
         strTable, MakeSuffix(), "COUNT(*)");

      common::CheckConstraintError(theOutput, aInput1,
         [](std::string const& strWhat) {
            Require(!strWhat.empty(), "missing PostgreSQL diagnostic");
            });

      std::cout << "\nALL POSTGRESQL LIBPQXX TESTS PASSED\n";
      return 0;
      }
   catch (std::exception const& ex) {
      adecc::diagnostic::WriteUtf8(std::cerr, "\nTEST FAILED\n");
      adecc::diagnostic::WriteUtf8(std::cerr, ex.what());
      adecc::diagnostic::WriteUtf8(std::cerr, "\n");
      return 1;
      }
   catch (...) {
      std::cerr << "\nTEST FAILED\nunknown exception\n";
      return 2;
      }
   }
