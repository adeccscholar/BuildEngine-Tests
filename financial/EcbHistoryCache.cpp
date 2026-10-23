// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

#include "EcbHistoryCache.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace adecc::financial::ecb {

namespace details {

#ifdef _WIN32
[[nodiscard]]
std::filesystem::path windowsEnvironmentPath(wchar_t const* variableName) {
   auto const requiredSize = GetEnvironmentVariableW(variableName, nullptr, 0);

   if (requiredSize == 0)
      return {};

   std::vector<wchar_t> buffer(requiredSize);
   auto const written = GetEnvironmentVariableW(variableName, buffer.data(), static_cast<DWORD>(buffer.size()));

   if (written == 0 || written >= buffer.size())
      return {};

   return std::filesystem::path { buffer.data() };
   }

#endif

[[nodiscard]]
std::string readTextFile(std::filesystem::path const& filename) {
   std::ifstream input { filename, std::ios::binary };
   if (!input)
      throw std::runtime_error(std::format("Cannot open ECB cache file '{}'",  filename.string()));

   return { std::istreambuf_iterator<char> { input }, std::istreambuf_iterator<char> {} };
   }


void writeTextFile(std::filesystem::path const& filename, std::string_view content) {
   if (!filename.parent_path().empty())
      std::filesystem::create_directories(filename.parent_path());

   std::ofstream output { filename, std::ios::binary | std::ios::trunc };

   if (!output)
      throw std::runtime_error(std::format(
         "Cannot create ECB cache file '{}'",
         filename.string()));

   output.write(
      content.data(),
      static_cast<std::streamsize>(content.size()));

   if (!output)
      throw std::runtime_error(std::format(
         "Cannot write ECB cache file '{}'",
         filename.string()));
}

[[nodiscard]]
std::filesystem::path stateFile(std::filesystem::path const& cacheFile) {
   auto result = cacheFile;
   result += ".state";
   return result;
   }

[[nodiscard]]
std::string dateText(date_ty date) {
   return std::format("{:%F}", std::chrono::sys_days { date });
   }

} // namespace

std::filesystem::path DefaultEcbCacheFile() {
#ifdef _WIN32
   auto const localAppData = details::windowsEnvironmentPath(L"LOCALAPPDATA");
   if (!localAppData.empty()) {
      return localAppData / "adecc" / "IrrExample" / "ecb" / "eurofxref-hist.xml";
      }
#endif

   return std::filesystem::current_path() / "cache" / "ecb" / "eurofxref-hist.xml";
   }

std::optional<std::string> ReadCachedEcbHistoryXml(std::filesystem::path const& cacheFile) {
   if (!std::filesystem::exists(cacheFile))
      return std::nullopt;

   return details::readTextFile(cacheFile);
   }

void ReplaceCachedEcbHistoryXml(std::filesystem::path const& cacheFile, std::string_view xml) {
   if (!cacheFile.parent_path().empty())
      std::filesystem::create_directories(cacheFile.parent_path());

   auto temporaryFile = cacheFile;
   temporaryFile += ".tmp";

   auto backupFile = cacheFile;
   backupFile += ".bak";

   details::writeTextFile(temporaryFile, xml);

   std::error_code error;
   std::filesystem::remove(backupFile, error);
   error.clear();

   auto const hadCache = std::filesystem::exists(cacheFile);
   if (hadCache) {
      std::filesystem::rename(cacheFile, backupFile, error);
      if (error) {
         std::filesystem::remove(temporaryFile);
         throw std::runtime_error(std::format("Cannot preserve existing ECB cache '{}': {}",
                                  cacheFile.string(), error.message()));
         }
      }

   std::filesystem::rename(temporaryFile, cacheFile, error);
   if (error) {
      if (hadCache) {
         std::error_code restoreError;
         std::filesystem::rename(backupFile, cacheFile, restoreError);
         }

      std::filesystem::remove(temporaryFile);
      throw std::runtime_error(std::format("Cannot replace ECB cache '{}': {}", cacheFile.string(), error.message()));
      }

   std::filesystem::remove(backupFile, error);
   }

bool WasPublicationChecked(std::filesystem::path const& cacheFile, date_ty publicationDate) {
   auto const filename = details::stateFile(cacheFile);
   if (!std::filesystem::exists(filename))  return false;

   std::ifstream input { filename };
   std::string checkedDate;
   std::getline(input, checkedDate);
   return checkedDate == details::dateText(publicationDate);
}

void MarkPublicationChecked(std::filesystem::path const& cacheFile, date_ty publicationDate) {
   details::writeTextFile(details::stateFile(cacheFile), details::dateText(publicationDate));
   }

} // namespace adecc::financial::ecb