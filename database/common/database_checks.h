// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once

/** \file database_checks.h
    \brief Shared C++23 acceptance tests against adecc::db::logical_database.
    \details Neither ODBC nor libpqxx is visible here. Database-specific
    connection parameters, CREATE TABLE SQL and diagnostics remain in the
    corresponding backend project. Both executables instantiate these exact
    same functions for their different physical database adapters.
*/

#include "database.h"
#include "diagnostic_text.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <expected>
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace buildengine_tests::database::common {

[[noreturn]] inline void Fail(std::string const& strMessage) {
   throw std::runtime_error{ strMessage };
   }

inline void Require(bool const boCondition, std::string const& strMessage) {
   if(!boCondition) {
      Fail(strMessage);
      }
   }

[[nodiscard]] inline std::string GetEnvironment(
   char const* const szName,
   std::string_view const svFallback = {}
) {
   if(char const* const szValue = std::getenv(szName);
      szValue != nullptr && *szValue != '\0') {
      return szValue;
      }
   return std::string{ svFallback };
   }

[[nodiscard]] inline bool ParseBoolean(
   std::string_view const svValue,
   bool const boDefault
) {
   if(svValue.empty()) {
      return boDefault;
      }
   if(svValue == "1" || svValue == "true" || svValue == "TRUE" ||
      svValue == "yes" || svValue == "YES" || svValue == "on" ||
      svValue == "ON") {
      return true;
      }
   if(svValue == "0" || svValue == "false" || svValue == "FALSE" ||
      svValue == "no" || svValue == "NO" || svValue == "off" ||
      svValue == "OFF") {
      return false;
      }
   Fail(std::format("invalid boolean value '{}'", svValue));
   }

[[nodiscard]] inline std::string MakeSuffix() {
   using namespace std::chrono;
   auto const iNow = duration_cast<microseconds>(
      system_clock::now().time_since_epoch()).count();
   return std::to_string(static_cast<unsigned long long>(iNow));
   }

[[nodiscard]] inline adecc::date_ty MakeDate(
   int const iYear,
   unsigned const uMonth,
   unsigned const uDay
) {
   adecc::date_ty const aDate{
      std::chrono::year{iYear},
      std::chrono::month{uMonth},
      std::chrono::day{uDay}
      };
   Require(aDate.ok(), "invalid test date");
   return aDate;
   }

[[nodiscard]] inline adecc::timestamp_ty MakeTimestamp(
   adecc::date_ty const& aDate,
   std::chrono::hours const aHours,
   std::chrono::minutes const aMinutes,
   std::chrono::seconds const aSeconds,
   std::chrono::nanoseconds const aNanoseconds = std::chrono::nanoseconds{0}
) {
   using namespace std::chrono;
   return time_point_cast<system_clock::duration>(
      sys_days{aDate} + aHours + aMinutes + aSeconds + aNanoseconds);
   }

[[nodiscard]] inline adecc::time_ty MakeTime(
   std::chrono::hours const aHours,
   std::chrono::minutes const aMinutes,
   std::chrono::seconds const aSeconds
) {
   return adecc::time_ty{
      std::chrono::duration_cast<std::chrono::seconds>(
         aHours + aMinutes + aSeconds)
      };
   }

template <typename... Types>
[[nodiscard]] std::tuple<Types...> RequireOne(
   std::expected<std::tuple<Types...>, adecc::db::SingleRowError> const& aResult,
   std::string_view const svContext
) {
   if(!aResult) {
      switch(aResult.error()) {
         case adecc::db::SingleRowError::NotFound:
            Fail(std::format("{}: row not found", svContext));
         case adecc::db::SingleRowError::TooMany:
            Fail(std::format("{}: more than one row", svContext));
         }
      }
   return *aResult;
   }

template <typename Value>
[[nodiscard]] bool EqualValue(Value const& aActual, Value const& aExpected) {
   if constexpr(std::is_floating_point_v<Value>) {
      return std::abs(aActual - aExpected) < 0.000000001;
      }
   else if constexpr(requires { aActual.Get(); aExpected.Get(); }) {
      return std::abs(aActual.Get() - aExpected.Get()) < 0.000001;
      }
   else if constexpr(requires { aActual.to_duration(); }) {
      return aActual.to_duration() == aExpected.to_duration();
      }
   else {
      return aActual == aExpected;
      }
   }

template <typename Row, std::size_t... Indices>
void CheckRowImpl(
   Row const& aActual,
   Row const& aExpected,
   std::index_sequence<Indices...>
) {
   (Require(EqualValue(std::get<Indices>(aActual), std::get<Indices>(aExpected)),
      std::format("row column {} differs", Indices)), ...);
   }

template <typename Row>
void CheckRow(Row const& aActual, Row const& aExpected) {
   CheckRowImpl(aActual, aExpected,
      std::make_index_sequence<std::tuple_size_v<Row>>{});
   }

/** Consume the same adecc output range for either database backend. */
template <typename Output, typename Row>
[[nodiscard]] Row InsertOne(Output& aOutput, Row const& aInput) {
   std::array<Row, 1> arrInput{aInput};
   std::optional<Row> optResult;
   for(auto const& aRow : aOutput(arrInput)) {
      Require(!optResult.has_value(), "output range returned multiple rows");
      optResult = aRow;
      }
   Require(optResult.has_value(), "output range returned no row");
   return *optResult;
   }

/** Identical logical parameter :Id is bound once, regardless of SQL driver. */
template <typename Database>
void CheckRepeatedParameters(
   Database& aDatabase,
   std::string const& strTable,
   long long const iParent,
   long long const iChild
) {
   std::vector<std::tuple<long long, std::string>> vecRows;
   for(auto const& aRow :
      aDatabase.template Execute<long long, std::string>(
         std::format("SELECT Id, Name FROM {} "
            "WHERE Id = :Id OR ParentId = :Id ORDER BY Id", strTable),
         Database::Params(Database::Param("Id", iParent)))) {
      vecRows.emplace_back(aRow);
      }
   Require(vecRows.size() == 2, "repeated :Id did not return two rows");
   Require(std::get<0>(vecRows[0]) == iParent, "first repeated :Id row differs");
   Require(std::get<0>(vecRows[1]) == iChild, "second repeated :Id row differs");
   std::cout << "[PASS] shared repeated logical :Id parameter test\n";
   }

/** Same UTF-16/public C++ semantics; only SQL CAST syntax is injected. */
template <typename Database>
void CheckWideParameters(
   Database& aDatabase,
   std::string const& strParameterQuery
) {
   std::wstring const strWideValue{L"M\u00FCnchen \u2013 \u6771\u4EAC"};
   auto const [aWide] = RequireOne(
      aDatabase.template ExecuteOne<std::wstring>(
         strParameterQuery,
         Database::Params(Database::Param("Value", strWideValue))),
      "wstring roundtrip");
   Require(aWide == strWideValue, "wstring roundtrip differs");

   std::wstring_view const svWideValue{strWideValue};
   auto const [aView] = RequireOne(
      aDatabase.template ExecuteOne<std::wstring>(
         strParameterQuery,
         Database::Params(Database::Param("Value", svWideValue))),
      "wstring_view roundtrip");
   Require(aView == strWideValue, "wstring_view roundtrip differs");

   wchar_t const* const szWideValue = L"\u00C4\u00D6\u00DC \u20AC";
   auto const [aPointer] = RequireOne(
      aDatabase.template ExecuteOne<std::wstring>(
         strParameterQuery,
         Database::Params(Database::Param("Value", szWideValue))),
      "wchar_t pointer roundtrip");
   Require(aPointer == std::wstring{szWideValue}, "wchar_t pointer differs");
   std::cout << "[PASS] shared Unicode string/view/pointer roundtrip\n";
   }

/** Same transaction lifetime semantics for both logical_database adapters. */
template <typename Database, typename Output, typename Row>
void CheckTransactions(
   Database& aDatabase,
   Output& aOutput,
   Row const& aSeed,
   std::string const& strTable,
   std::string const& strSuffix,
   std::string_view const svCountExpression
) {
   auto fnCheckCount = [&](std::string const& strName, long long const iCount) {
      auto const [iActual] = RequireOne(
         aDatabase.template ExecuteOne<long long>(
            std::format("SELECT {} FROM {} WHERE Name = :Name",
               svCountExpression, strTable),
            Database::Params(Database::Param("Name", strName))),
         "transaction count");
      Require(iActual == iCount, "transaction count differs");
      };

   std::string const strRollbackName = "Rollback_" + strSuffix;
   {
      auto aTransaction = aDatabase.Transaction();
      Row aRow = aSeed;
      std::get<0>(aRow) = 0LL;
      std::get<1>(aRow) = strRollbackName;
      [[maybe_unused]] Row const aInserted = InsertOne(aOutput, aRow);
      }
   fnCheckCount(strRollbackName, 0);
   std::cout << "[PASS] shared RAII transaction rollback\n";

   std::string const strCommitName = "Commit_" + strSuffix;
   {
      auto aTransaction = aDatabase.Transaction();
      Row aRow = aSeed;
      std::get<0>(aRow) = 0LL;
      std::get<1>(aRow) = strCommitName;
      [[maybe_unused]] Row const aInserted = InsertOne(aOutput, aRow);
      aTransaction.CommitAndClose();
      }
   fnCheckCount(strCommitName, 1);
   std::cout << "[PASS] shared transaction commit\n";
   }

/** Same uniqueness assertion and UTF-8 diagnostics; detail is driver-specific. */
template <typename Output, typename Row, typename DiagnosticCheck>
void CheckConstraintError(
   Output& aOutput,
   Row const& aSeed,
   DiagnosticCheck&& fnValidate
) {
   bool boCaught = false;
   try {
      Row aRow = aSeed;
      std::get<0>(aRow) = 0LL;
      [[maybe_unused]] Row const aUnexpected = InsertOne(aOutput, aRow);
      }
   catch(std::exception const& aException) {
      boCaught = true;
      std::string const strText{aException.what()};
      Require(!strText.empty(), "constraint diagnostic is empty");
      std::forward<DiagnosticCheck>(fnValidate)(strText);
      adecc::diagnostic::WriteUtf8(std::cout, "[DIAGNOSTIC SAMPLE]\n");
      adecc::diagnostic::WriteUtf8(std::cout, strText);
      adecc::diagnostic::WriteUtf8(std::cout, "\n[END DIAGNOSTIC SAMPLE]\n");
      }
   Require(boCaught, "duplicate UNIQUE(Name) insert unexpectedly succeeded");
   std::cout << "[PASS] shared constraint failure verification\n";
   }

} // namespace buildengine_tests::database::common
