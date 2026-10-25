// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

/**
\file test_postgresql_pqxx_database.cpp
\brief Integration test for the adecc PostgreSQL/libpqxx backend.
*/

#include "database.h"
#include "database_checks.h"
#include "additional_tests.h"
#include "additional_dialect.h"
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
