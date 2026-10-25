#pragma once

#include "value_types.h"
#include "text_grid_wrapper.h"
#include "stream_wrappers.h"
#include "stream_redirect.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <exception>
#include <format>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace adecc::test {

   enum class test_case_kind {
      expected_success,
      expected_error
   };

   inline std::string_view ToText(test_case_kind const aKind) {
      switch (aKind) {
      case test_case_kind::expected_success:
         return "Expected";
      case test_case_kind::expected_error:
         return "Error expected";
      default:
         return "Unknown";
      }
   }

   struct test_result {
      test_case_kind aType{};
      std::string    strName{};
      std::string    strStartedAt{};
      long long      iDurationMs{};
      bool           boPassed{};
      std::string    strStatusText{};
      std::string    strExceptionText{};
   };

   template <std::size_t I>
   decltype(auto) get(test_result& aResult) noexcept {
      if constexpr (I == 0) {
         return (aResult.aType);
      }
      else if constexpr (I == 1) {
         return (aResult.strName);
      }
      else if constexpr (I == 2) {
         return (aResult.strStartedAt);
      }
      else if constexpr (I == 3) {
         return (aResult.iDurationMs);
      }
      else if constexpr (I == 4) {
         return (aResult.boPassed);
      }
      else if constexpr (I == 5) {
         return (aResult.strStatusText);
      }
      else if constexpr (I == 6) {
         return (aResult.strExceptionText);
      }
      else {
         static_assert(I < 7, "test_result has only seven tuple-like fields");
      }
   }

   template <std::size_t I>
   decltype(auto) get(test_result const& aResult) noexcept {
      if constexpr (I == 0) {
         return (aResult.aType);
      }
      else if constexpr (I == 1) {
         return (aResult.strName);
      }
      else if constexpr (I == 2) {
         return (aResult.strStartedAt);
      }
      else if constexpr (I == 3) {
         return (aResult.iDurationMs);
      }
      else if constexpr (I == 4) {
         return (aResult.boPassed);
      }
      else if constexpr (I == 5) {
         return (aResult.strStatusText);
      }
      else if constexpr (I == 6) {
         return (aResult.strExceptionText);
      }
      else {
         static_assert(I < 7, "test_result has only seven tuple-like fields");
      }
   }

   template <std::size_t I>
   decltype(auto) get(test_result&& aResult) noexcept {
      if constexpr (I == 0) {
         return std::move(aResult.aType);
      }
      else if constexpr (I == 1) {
         return std::move(aResult.strName);
      }
      else if constexpr (I == 2) {
         return std::move(aResult.strStartedAt);
      }
      else if constexpr (I == 3) {
         return std::move(aResult.iDurationMs);
      }
      else if constexpr (I == 4) {
         return std::move(aResult.boPassed);
      }
      else if constexpr (I == 5) {
         return std::move(aResult.strStatusText);
      }
      else if constexpr (I == 6) {
         return std::move(aResult.strExceptionText);
      }
      else {
         static_assert(I < 7, "test_result has only seven tuple-like fields");
      }
   }

   class test_assertion_error : public std::runtime_error {
   public:
      explicit test_assertion_error(std::string const& strMessage)
         : std::runtime_error{ strMessage } {
      }
   };

   inline std::string EscapeResultText(std::string_view const svText) {
      std::string strResult;
      strResult.reserve(svText.size());

      for (char const ch : svText) {
         switch (ch) {
         case '\\':
            strResult += "\\\\";
            break;
         case '\t':
            strResult += "\\t";
            break;
         case '\r':
            strResult += "\\r";
            break;
         case '\n':
            strResult += "\\n";
            break;
         default:
            strResult += ch;
            break;
         }
      }

      return strResult;
   }

   class test_context {
   public:
      explicit test_context(std::ostream& out) : out{ out } {
      }

      test_context(test_context const&) = delete;
      test_context& operator=(test_context const&) = delete;

      test_context(test_context&&) = default;
      test_context& operator=(test_context&&) = default;

      void Begin(test_case_kind const aKind, std::string_view const svName) {
         vecResults.push_back(test_result{
            .aType = aKind,
            .strName = std::string{ svName },
            .strStartedAt = FormatTimestamp_(std::chrono::system_clock::now()),
            .iDurationMs = 0,
            .boPassed = false,
            .strStatusText = {},
            .strExceptionText = {}
            });

         optActiveIndex = vecResults.size() - 1;
         optActiveStart = std::chrono::steady_clock::now();
      }

      void Complete(bool const boPassed, std::string_view const svStatusText, std::string_view const svExceptionText = {}) {
         if (!optActiveIndex) {
            return;
         }

         auto& aResult = vecResults.at(*optActiveIndex);
         aResult.boPassed = boPassed;
         aResult.iDurationMs = ActiveDurationMs_();

         if (!svStatusText.empty()) {
            AppendStatusText_(aResult, svStatusText);
         }

         aResult.strExceptionText = std::string{ svExceptionText };
         optActiveIndex.reset();
         optActiveStart.reset();
      }

      void Log(std::string_view const svText) {
         if (optActiveIndex) {
            AppendStatusText_(vecResults.at(*optActiveIndex), svText);
         }
         else {
            std::println(out, "{}", svText);
         }
      }

      void Success(std::string_view const svName) {
         Log(std::format("OK: {}", svName));
      }

      [[noreturn]] void Failure(std::string_view const svName, std::string const& strMessage) {
         auto const strFullMessage = std::format("ERROR: {}\n{}", svName, strMessage);
         Log(strFullMessage);
         throw test_assertion_error{ strFullMessage };
      }

      void ExpectedError(std::string_view const svName) {
         Log(std::format("Expected error captured in {}", svName));
      }

      std::vector<test_result> const& Results() const noexcept {
         return vecResults;
      }

      std::vector<test_result> TakeResults()&& {
         return std::move(vecResults);
      }

      int PassedCount() const {
         return static_cast<int>(std::ranges::count_if(vecResults, [](test_result const& aResult) {
            return aResult.boPassed;
            }));
      }

      int FailedCount() const {
         return static_cast<int>(std::ranges::count_if(vecResults, [](test_result const& aResult) {
            return !aResult.boPassed;
            }));
      }

      void PrintResults(std::ostream& theOut) const {
         for (auto const& aResult : vecResults) {
            std::println(theOut, "{}\t{}\t{}\t{}\t{}",
               ToText(aResult.aType),
               aResult.strName,
               aResult.strStartedAt,
               aResult.iDurationMs,
               aResult.boPassed ? "yes" : "no"
               );
         }
      }

      void PrintExceptionResults(std::ostream& theOut) const {
         bool const boHasExceptions = std::ranges::any_of(vecResults, [](test_result const& aResult) {
            return !aResult.strExceptionText.empty();
            });

         if (!boHasExceptions) {
            return;
         }

         std::println(theOut, "");
         std::println(theOut, "Exception details:");

         for (auto const& aResult : vecResults) {
            if (aResult.strExceptionText.empty()) {
               continue;
            }

            std::println(theOut, "");
            std::println(theOut, "Type: {}", ToText(aResult.aType));
            std::println(theOut, "Name: {}", aResult.strName);
            std::println(theOut, "Started at: {}", aResult.strStartedAt);
            std::println(theOut, "Duration ms: {}", aResult.iDurationMs);
            std::println(theOut, "Passed: {}", aResult.boPassed ? "yes" : "no");
            std::println(theOut, "Exception:");
            std::println(theOut, "{}", aResult.strExceptionText);
         }
      }

      void Summary(bool const boPrintExceptionResults = false) const {
         std::println(out, "");
         std::println(out, "Test run resolved.");
         std::println(out, "Passed tests: {}", PassedCount());
         std::println(out, "Failed tests: {}", FailedCount());
         std::println(out, "");

         using stream_policy_type = adecc::AnsiStreamPolicy;
         using streambuf_type = adecc::stream::TextGridStreamBuf<stream_policy_type>;

         adecc::vecCaptions<adecc::AnsiStreamPolicy> vecTestResultCaps = {
            { "Type",        15, adecc::EAlignmentType::left  },
            { "Name",        90, adecc::EAlignmentType::left  },
            { "Started",     24, adecc::EAlignmentType::left  },
            { "Duration ms", 12, adecc::EAlignmentType::right },
            { "Passed",       9, adecc::EAlignmentType::left  }
            };

         auto theBuf = streambuf_type{
            out,
            vecTestResultCaps,
            streambuf_type::grid_backend_type::TableSeparators()
            };

         {
            adecc::StreamRedirect<char> theRedirect{
               out,
               theBuf
               };

            PrintResults(out);
         }

         if (boPrintExceptionResults) {
            PrintExceptionResults(out);
         }
      }

   private:
      static std::string FormatTimestamp_(std::chrono::system_clock::time_point const aTimePoint) {
         auto const aTime = std::chrono::system_clock::to_time_t(aTimePoint);
         auto const iMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
            aTimePoint.time_since_epoch()).count() % 1000;

         std::tm aTimeParts{};

#ifdef _WIN32
         localtime_s(&aTimeParts, &aTime);
#else
         localtime_r(&aTime, &aTimeParts);
#endif

         std::ostringstream strmTimestamp;
         strmTimestamp << std::put_time(&aTimeParts, "%Y-%m-%d %H:%M:%S");
         strmTimestamp << std::format(".{:03}", iMillis);
         return strmTimestamp.str();
      }

      [[nodiscard]] long long ActiveDurationMs_() const {
         if (!optActiveStart) {
            return 0;
         }

         return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - *optActiveStart).count();
      }

      static void AppendStatusText_(test_result& aResult, std::string_view const svText) {
         if (!aResult.strStatusText.empty()) {
            aResult.strStatusText += '\n';
         }

         aResult.strStatusText += svText;
      }

   public:
      std::ostream& out;

   private:
      std::vector<test_result> vecResults{};
      std::optional<std::size_t> optActiveIndex{};
      std::optional<std::chrono::steady_clock::time_point> optActiveStart{};
   };

   template <size_t Index, class value_ty>
   decltype(auto) ExtractScalar(value_ty&& aValue) {
      if constexpr (adecc::is_tuple_like_v<adecc::remove_cvref_t<value_ty>>) {
         return std::get<Index>(std::forward<value_ty>(aValue));
      }
      else {
         return std::forward<value_ty>(aValue);
      }
   }

   template <adecc::db::logical_database_type db_ty>
   void ExecuteStatement(db_ty& db, std::string const& strSql, adecc::db_params const& vecParams = {}) {
      if constexpr (requires { db.ExecuteCommand(strSql, vecParams); }) {
         db.ExecuteCommand(strSql, vecParams);
      }
      else if constexpr (requires { db.ExecuteCommand(strSql); }) {
         if (!vecParams.empty()) {
            auto aResult = db.Execute(strSql, vecParams);

            if constexpr (requires { aResult.begin(); aResult.end(); }) {
               for (auto const& aRow : aResult) {
                  (void)aRow;
               }
            }
            else {
               (void)aResult;
            }
         }
         else {
            db.ExecuteCommand(strSql);
         }
      }
      else {
         auto aResult = db.Execute(strSql, vecParams);

         if constexpr (requires { aResult.begin(); aResult.end(); }) {
            for (auto const& aRow : aResult) {
               (void)aRow;
            }
         }
         else {
            (void)aResult;
         }
      }
   }

   template <class result_ty, adecc::db::logical_database_type db_ty>
   std::vector<result_ty> SelectValues(db_ty& db, std::string const& strSql, adecc::db_params const& vecParams = {}) {
      std::vector<result_ty> vecValues;

      for (auto const& aRow : db.template Execute<result_ty>(strSql, vecParams)) {
         vecValues.emplace_back(ExtractScalar<0>(aRow));
      }

      return vecValues;
   }

   template <class result_ty, adecc::db::logical_database_type db_ty>
   result_ty SelectSingleValue(db_ty& db, std::string const& strSql, adecc::db_params const& vecParams = {}) {
      auto vecValues = SelectValues<result_ty>(db, strSql, vecParams);

      if (vecValues.size() != 1) {
         throw std::runtime_error{
            std::format("Expected exactly one result row, got {}", vecValues.size())
            };
      }

      return vecValues.front();
   }

   template <adecc::db::logical_database_type db_ty>
   int SelectCount(db_ty& db, std::string const& strSql, adecc::db_params const& vecParams = {}) {
      return SelectSingleValue<int>(db, strSql, vecParams);
   }

   template <class fn_ty>
   void ExpectError(test_context& ctx, std::string_view const svName, fn_ty&& fnTest) {
      ctx.Begin(test_case_kind::expected_error, svName);

      try {
         fnTest();
         ctx.Complete(false, "Expected an exception, but the operation completed successfully.");
      }
      catch (std::exception const& ex) {
         ctx.ExpectedError(svName);
         ctx.Complete(true, "Expected exception was thrown.", ex.what());
      }
   }

   template <class fn_ty>
   void ExpectSuccess(test_context& ctx, std::string_view const svName, fn_ty&& fnTest) {
      ctx.Begin(test_case_kind::expected_success, svName);

      try {
         fnTest();
         ctx.Complete(true, "Operation completed successfully.");
      }
      catch (std::exception const& ex) {
         ctx.Complete(false, "Unexpected exception was thrown.", ex.what());
      }
   }

   template <class fn_ty>
   void ExpectDbError(test_context& ctx, std::string_view const svName, fn_ty&& fnTest) {
      ExpectError(ctx, svName, std::forward<fn_ty>(fnTest));
   }

   template <class fn_ty>
   void ExpectDbSuccess(test_context& ctx, std::string_view const svName, fn_ty&& fnTest) {
      ExpectSuccess(ctx, svName, std::forward<fn_ty>(fnTest));
   }

   template <class value_ty>
   constexpr bool IsSingleTupleValue_() {
      using clean_ty = std::remove_cvref_t<value_ty>;

      if constexpr (adecc::is_tuple_like_v<clean_ty>) {
         return std::tuple_size_v<clean_ty> == 1;
      }
      else {
         return false;
      }
   }

   template <class value_ty>
   decltype(auto) NormalizeExpectedValue(value_ty const& aValue) {
      if constexpr (IsSingleTupleValue_<value_ty>()) {
         return std::get<0>(aValue);
      }
      else {
         return (aValue);
      }
   }

   template <class left_ty, class right_ty>
   void ExpectEqual(test_context& ctx,
      std::string_view const svName,
      left_ty const& aLeft,
      right_ty const& aRight) {
      auto const& aNormLeft = NormalizeExpectedValue(aLeft);
      auto const& aNormRight = NormalizeExpectedValue(aRight);

      if (aNormLeft == aNormRight) {
         ctx.Success(svName);
      }
      else {
         ctx.Failure(svName, std::format("Expected '{}', got '{}'", aNormRight, aNormLeft));
      }
   }

} // namespace adecc::test

namespace database_error_scenario = adecc::test;

namespace std {

   template <>
   struct tuple_size<adecc::test::test_result> : integral_constant<size_t, 7> {
   };

   template <>
   struct tuple_element<0, adecc::test::test_result> {
      using type = adecc::test::test_case_kind;
   };

   template <>
   struct tuple_element<1, adecc::test::test_result> {
      using type = string;
   };

   template <>
   struct tuple_element<2, adecc::test::test_result> {
      using type = string;
   };

   template <>
   struct tuple_element<3, adecc::test::test_result> {
      using type = long long;
   };

   template <>
   struct tuple_element<4, adecc::test::test_result> {
      using type = bool;
   };

   template <>
   struct tuple_element<5, adecc::test::test_result> {
      using type = string;
   };

   template <>
   struct tuple_element<6, adecc::test::test_result> {
      using type = string;
   };

} // namespace std