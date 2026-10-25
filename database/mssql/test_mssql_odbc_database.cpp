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

      std::string const strSuffix = MakeSuffix();
      std::string const strDataTable = "dbo.AdeccOdbcData_" + strSuffix;
      std::string const strAuditTable = "dbo.AdeccOdbcAudit_" + strSuffix;
      std::string const strTrigger = "dbo.AdeccOdbcTrigger_" + strSuffix;

      database_objects_guard aCleanup{
         .pDatabase = &theDatabase,
         .strDataTable = strDataTable,
         .strAuditTable = strAuditTable
         };

      theDatabase.ExecuteCommand(std::format(
         "CREATE TABLE {} ("
         "Id bigint IDENTITY(1,1) NOT NULL PRIMARY KEY,"
         "Name nvarchar(200) NOT NULL,"
         "AlternateName nvarchar(200) NULL,"
         "Amount decimal(18,2) NOT NULL,"
         "Ratio decimal(18,6) NOT NULL,"
         "LegacyTime datetime NOT NULL,"
         "PreciseTime datetime2(7) NOT NULL,"
         "BusinessDate date NOT NULL,"
         "BusinessTime time(0) NOT NULL,"
         "Active bit NOT NULL,"
         "ParentId bigint NULL,"
         "CONSTRAINT UQ_AdeccOdbcData_{} UNIQUE(Name)"
         ");",
         strDataTable,
         strSuffix
         ));

      theDatabase.ExecuteCommand(std::format(
         "CREATE TABLE {} ("
         "AuditId bigint IDENTITY(1000000,1) NOT NULL PRIMARY KEY,"
         "RowId bigint NOT NULL"
         ");",
         strAuditTable
         ));

      theDatabase.ExecuteCommand(std::format(
         "CREATE TRIGGER {} ON {} AFTER INSERT AS "
         "BEGIN "
         "SET NOCOUNT ON; "
         "INSERT INTO {} (RowId) SELECT Id FROM inserted; "
         "END;",
         strTrigger,
         strDataTable,
         strAuditTable
         ));

      std::cout << "[PASS] test schema created\n";

      std::string const strInsert = std::format(
         "INSERT INTO {} "
         "(Name, AlternateName, Amount, Ratio, LegacyTime, PreciseTime, "
         " BusinessDate, BusinessTime, Active, ParentId) "
         "VALUES "
         "(:Name, :AlternateName, :Amount, :Ratio, :LegacyTime, :PreciseTime, "
         " :BusinessDate, :BusinessTime, :Active, :ParentId)",
         strDataTable
         );

      auto theOutput = theDatabase.MakeOutputRange<
         long long,
         std::string,
         std::optional<std::string>,
         adecc::money_ty,
         double,
         adecc::timestamp_ty,
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
               { "Amount", adecc::db_output_param_role::needed_value },
               { "Ratio", adecc::db_output_param_role::needed_value },
               { "LegacyTime", adecc::db_output_param_role::needed_value },
               { "PreciseTime", adecc::db_output_param_role::needed_value },
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
      adecc::timestamp_ty const aLegacy1 = MakeTimestamp(
         aDate1,
         hours{ 1 },
         minutes{ 2 },
         seconds{ 3 }
         );
      adecc::timestamp_ty const aPrecise1 = MakeTimestamp(
         aDate1,
         hours{ 4 },
         minutes{ 5 },
         seconds{ 6 },
         nanoseconds{ 123456700 }
         );
      adecc::time_ty const aTime1 = MakeTime(hours{ 14 }, minutes{ 15 }, seconds{ 16 });

      row_ty aInput1{
         0LL,
         "Muenchen",
         std::nullopt,
         adecc::money_ty{ 1234.56 },
         12.125,
         aLegacy1,
         aPrecise1,
         aDate1,
         aTime1,
         true,
         std::nullopt
         };

      row_ty aInserted1 = fnInsertOne(aInput1);
      long long const iId1 = std::get<0>(aInserted1);
      Require(iId1 > 0, "IDENTITY value was not returned");
      std::get<0>(aInput1) = iId1;
      CheckRow(aInserted1, aInput1);
      std::cout << "[PASS] IDENTITY and first insert, Id=" << iId1 << '\n';

      auto const aAudit = RequireOne(
         theDatabase.ExecuteOne<long long, long long>(
            std::format(
               "SELECT AuditId, RowId FROM {} WHERE RowId = :Id",
               strAuditTable
               ),
            database_ty::Params(database_ty::Param("Id", iId1))
            ),
         "read trigger audit row"
         );

      Require(std::get<1>(aAudit) == iId1, "trigger audit row references wrong data row");
      Require(std::get<0>(aAudit) >= 1000000, "audit IDENTITY has unexpected value");
      Require(std::get<0>(aAudit) != iId1,
              "test cannot prove SCOPE_IDENTITY because both identities are equal");
      std::cout << "[PASS] SCOPE_IDENTITY is not confused by trigger IDENTITY\n";

      auto const aRead1 = RequireOne(
         theDatabase.ExecuteOne<
            long long,
            std::string,
            std::optional<std::string>,
            adecc::money_ty,
            double,
            adecc::timestamp_ty,
            adecc::timestamp_ty,
            adecc::date_ty,
            adecc::time_ty,
            bool,
            std::optional<long long>
            >(
               std::format(
                  "SELECT Id, Name, AlternateName, Amount, Ratio, LegacyTime, PreciseTime, "
                  "BusinessDate, BusinessTime, Active, ParentId "
                  "FROM {} WHERE Id = :Id",
                  strDataTable
                  ),
               database_ty::Params(database_ty::Param("Id", iId1))
               ),
         "read first inserted row"
         );

      CheckRow(aRead1, aInput1);
      std::cout << "[PASS] narrow string, NULL, decimal, datetime, datetime2, date, time and bit roundtrip\n";

      common::CheckWideParameters(theDatabase,
         "SELECT CAST(:Value AS nvarchar(200)) AS Value");

      adecc::date_ty const aDate2 = MakeDate(2026, 10, 9);
      row_ty aInput2{
         0LL,
         "Child",
         std::optional<std::string>{ "Alternate" },
         adecc::money_ty{ 42.10 },
         0.125,
         MakeTimestamp(aDate2, hours{ 7 }, minutes{ 8 }, seconds{ 9 }),
         MakeTimestamp(
            aDate2,
            hours{ 10 },
            minutes{ 11 },
            seconds{ 12 },
            nanoseconds{ 765432100 }
            ),
         aDate2,
         MakeTime(hours{ 17 }, minutes{ 18 }, seconds{ 19 }),
         false,
         iId1
         };

      row_ty aInserted2 = fnInsertOne(aInput2);
      long long const iId2 = std::get<0>(aInserted2);
      Require(iId2 > iId1, "second IDENTITY value is not greater than first value");
      std::get<0>(aInput2) = iId2;
      CheckRow(aInserted2, aInput2);

      common::CheckRepeatedParameters(theDatabase,
         strDataTable, iId1, iId2);

      common::CheckTransactions(theDatabase, theOutput, aInput1,
         strDataTable, strSuffix, "COUNT_BIG(*)");

      common::CheckConstraintError(theOutput, aInput1,
         [](std::string const& strWhat) {
            Require(strWhat.find("SQLSTATE=") != std::string::npos,
               "constraint error has no SQLSTATE");
            Require(strWhat.find("Native=") != std::string::npos,
               "constraint error has no SQL Server native code");
            Require(strWhat.find("ODBC parameter map:") != std::string::npos,
               "constraint error has no parameter mapping");
            });

      std::cout << "\nALL MS SQL SERVER ODBC TESTS PASSED\n";
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
