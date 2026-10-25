/**
\file test_mssql_odbc_database.cpp
\brief Integration and acceptance test for the adecc Microsoft SQL Server ODBC backend.
\details
   This test intentionally uses only the public cpp_core database abstraction.
   ODBC is not accessed by the test itself.

   Covered cases:
   - integrated and SQL login credentials
   - SQL Server bigint IDENTITY(1,1)
   - trigger with a second IDENTITY table, proving SCOPE_IDENTITY semantics
   - repeated logical named parameters supplied only once by the caller
   - Unicode text and NULL values
   - decimal to money_ty and decimal to double
   - datetime and datetime2(7)
   - date and time(0)
   - transaction rollback and commit
   - complete ODBC diagnostics for a server-side constraint violation
*/

#include "database.h"
#include "database_checks.h"
#include "additional_tests.h"
#include "additional_dialect.h"
#include "diagnostic_text.h"
#include "mssql_odbc_database.h"

#include <array>
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
using namespace std::string_literals;

using physical_database_ty = adecc::db::mssql::mssql_database;
using database_ty = adecc::db::logical_database<
   physical_database_ty,
   adecc::db::mssql::query
   >;

using row_ty = std::tuple<
   long long,                      // Id - bigint IDENTITY(1,1)
   std::string,                    // Name - nvarchar
   std::optional<std::string>,     // AlternateName - nullable nvarchar
   adecc::money_ty,                // Amount - decimal(18,2)
   double,                         // Ratio - decimal(18,6)
   adecc::timestamp_ty,            // LegacyTime - datetime
   adecc::timestamp_ty,            // PreciseTime - datetime2(7)
   adecc::date_ty,                 // BusinessDate - date
   adecc::time_ty,                 // BusinessTime - time(0)
   bool,                           // Active - bit
   std::optional<long long>        // ParentId - nullable bigint
   >;

struct test_config {
   std::string strServer{};
   std::string strDatabase{};
   bool boIntegrated{ true };
   std::string strUser{};
   std::string strPassword{};
   };

using common::Fail;
using common::Require;

using common::GetEnvironment;
using common::ParseBoolean;

void PrintUsage(char const* const szProgram) {
   std::cout
      << "Usage:\n"
      << "  " << szProgram << " --server <server> --database <database> [--integrated]\n"
      << "  " << szProgram << " --server <server> --database <database> "
         "--sql-login --user <user> --password <password>\n\n"
      << "Environment alternatives:\n"
      << "  ADECC_MSSQL_SERVER\n"
      << "  ADECC_MSSQL_DATABASE\n"
      << "  ADECC_MSSQL_INTEGRATED=1|0\n"
      << "  ADECC_MSSQL_USER\n"
      << "  ADECC_MSSQL_PASSWORD\n";
   }

test_config ParseArguments(int const iArgc, char* const* const argv) {
   test_config aConfig{
      .strServer = GetEnvironment("ADECC_MSSQL_SERVER"),
      .strDatabase = GetEnvironment("ADECC_MSSQL_DATABASE"),
      .boIntegrated = ParseBoolean(GetEnvironment("ADECC_MSSQL_INTEGRATED"), true),
      .strUser = GetEnvironment("ADECC_MSSQL_USER"),
      .strPassword = GetEnvironment("ADECC_MSSQL_PASSWORD")
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
      else if (svArg == "--server") {
         aConfig.strServer = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--database") {
         aConfig.strDatabase = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--integrated") {
         aConfig.boIntegrated = true;
         }
      else if (svArg == "--sql-login") {
         aConfig.boIntegrated = false;
         }
      else if (svArg == "--user") {
         aConfig.strUser = fnNeedValue(iIndex, svArg);
         }
      else if (svArg == "--password") {
         aConfig.strPassword = fnNeedValue(iIndex, svArg);
         }
      else {
         Fail(std::format("unknown argument '{}'", svArg));
         }
      }

   if (aConfig.strServer.empty()) {
      Fail("SQL Server is missing; use --server or ADECC_MSSQL_SERVER");
      }

   if (aConfig.strDatabase.empty()) {
      Fail("database is missing; use --database or ADECC_MSSQL_DATABASE");
      }

   if (!aConfig.boIntegrated &&
       (aConfig.strUser.empty() || aConfig.strPassword.empty())) {
      Fail("SQL login requires both user and password");
      }

   return aConfig;
   }

using common::MakeSuffix;

struct database_objects_guard {
   database_ty* pDatabase{};
   std::string strDataTable{};
   std::string strAuditTable{};

   ~database_objects_guard() {
      if (pDatabase == nullptr) {
         return;
         }

      try {
         pDatabase->ExecuteCommand(std::format("DROP TABLE IF EXISTS {};", strDataTable));
         }
      catch (...) {
         }

      try {
         pDatabase->ExecuteCommand(std::format("DROP TABLE IF EXISTS {};", strAuditTable));
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

      adecc::db::mssql::mssql_credentials const aCredentials{
         .strServer = aConfig.strServer,
         .strDatabase = aConfig.strDatabase,
         .boIntegrated = aConfig.boIntegrated,
         .strUser = aConfig.strUser,
         .strPassword = aConfig.strPassword,
         .boMARS = true,
         .iTimeout = 30
         };

      std::cout << "[TEST] connect to " << aConfig.strServer
                << " / " << aConfig.strDatabase
                << " using " << (aConfig.boIntegrated ? "integrated authentication" : "SQL login")
                << '\n';

      database_ty theDatabase{ aCredentials };
      Require(theDatabase.Connected(), "database reports not connected after construction");
      std::cout << "[PASS] connection\n";
      auto const count = buildengine_tests::database::additional::Run(theDatabase, Dialect{});
      std::cout << "[RESULT] shared additional tests passed: " << count << "\n";
      return 0;
      }
   catch(std::exception const& ex) {
      adecc::diagnostic::WriteUtf8(std::cerr, ex.what());
      return 1;
      }
   catch(...) {
      std::cerr << "Unknown test exception\n";
      return 2;
      }
   }
