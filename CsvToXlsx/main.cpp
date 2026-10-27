#include "Convert.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace fs = std::filesystem;
namespace {

/**
\brief Paths and suffix specified on the command line.
\details Empty optional paths retain the documented defaults.
*/
struct Options {
   fs::path aInputDirectory;
   fs::path aExtension{L".csv"};
   fs::path aArchiveDirectory;
   fs::path aOutputDirectory;
   bool bMoveFiles{};
};

/**
\brief Obtain path arguments without losing Windows Unicode text.
\details Uses the native wide Windows command line rather than locale-dependent argv.
\returns Every parameter after the executable name.
\throw std::runtime_error If Windows argument decoding fails.
*/
std::vector<fs::path> CommandLineArguments(int iArgc, char const* const* pszArgv) {
   std::vector<fs::path> vecArguments;
#ifdef _WIN32
   int iCount = 0;
   LPWSTR* pszWideArgs = ::CommandLineToArgvW(::GetCommandLineW(), &iCount);
   if(pszWideArgs == nullptr) {
      throw std::runtime_error("CommandLineToArgvW failed");
      }
   auto aDeleter = [](LPWSTR* pszMemory) -> void {
      ::LocalFree(pszMemory);
      };
   std::unique_ptr<LPWSTR, decltype(aDeleter)> aGuard(pszWideArgs, aDeleter);
   for(int iIndex = 1; iIndex < iCount; ++iIndex) {
      vecArguments.emplace_back(pszWideArgs[iIndex]);
      }
#else
   for(int iIndex = 1; iIndex < iArgc; ++iIndex) {
      vecArguments.emplace_back(pszArgv[iIndex]);
      }
#endif
   return vecArguments;
   }

/**
\brief Canonicalize an existing directory or prepare a destination directory.
\details Normalizes paths to allow safe equality comparisons.
\param aPath Input directory path.
\param bCreate True to create the directory when needed.
\returns Normalized absolute directory path.
\throw std::filesystem::filesystem_error On path failure.
*/
fs::path DirectoryPath(fs::path const& aPath, bool bCreate) {
   if(bCreate) {
      fs::create_directories(aPath);
      }
   if(!fs::is_directory(aPath)) {
      throw std::runtime_error("Directory does not exist or is inaccessible");
      }
   return fs::weakly_canonical(aPath);
   }

/**
\brief Parse up to four positional command-line arguments.
\details Unspecified extension is .csv; omitted output path uses the input path.
\param vecArguments Positional arguments excluding executable name.
\returns Validated and normalized configuration.
\throw std::exception On an invalid argument or directory.
*/
Options ParseOptions(std::vector<fs::path> const& vecArguments) {
   if(vecArguments.empty() || vecArguments.size() > 4 || vecArguments[0].empty()) {
      throw std::invalid_argument("Usage: adecc_csv_to_xlsx <input-dir> [extension] [move-dir] [xlsx-dir]");
      }
   Options aResult;
   aResult.aInputDirectory = DirectoryPath(vecArguments[0], false);
   if(vecArguments.size() >= 2 && !vecArguments[1].empty()) {
      aResult.aExtension = vecArguments[1];
      }
   if(aResult.aExtension.native().front() != L'.') {
      fs::path aPrefix{L"."};
      aPrefix += aResult.aExtension;
      aResult.aExtension = std::move(aPrefix);
      }
   if(vecArguments.size() >= 3 && !vecArguments[2].empty()) {
      aResult.aArchiveDirectory = DirectoryPath(vecArguments[2], true);
      aResult.bMoveFiles = (aResult.aArchiveDirectory != aResult.aInputDirectory);
      }
   aResult.aOutputDirectory = (vecArguments.size() >= 4 && !vecArguments[3].empty())
      ? DirectoryPath(vecArguments[3], true)
      : aResult.aInputDirectory;
   return aResult;
   }

/**
\brief Compare file extensions case-insensitively for ASCII extension names.
\details Windows-native paths are compared without altering actual filenames.
\param aFile Source filename.
\param aExpected Desired suffix, including the leading dot.
\returns true when both normalized extensions match.
\throw None.
*/
bool HasExtension(fs::path const& aFile, fs::path const& aExpected) {
   std::wstring strFile = aFile.extension().wstring();
   std::wstring strExpected = aExpected.wstring();
   auto ToAsciiLower = [](wchar_t ch) -> wchar_t {
      return (ch >= L'A' && ch <= L'Z') ? static_cast<wchar_t>(ch - L'A' + L'a') : ch;
      };
   std::ranges::transform(strFile, strFile.begin(), ToAsciiLower);
   std::ranges::transform(strExpected, strExpected.begin(), ToAsciiLower);
   return strFile == strExpected;
   }

/**
\brief Move a source file to its archive destination without overwriting.
\details Uses rename on the same volume and verified copy-then-remove if the
volumes differ; never removes the original before a successful copy.
\param aSource Current source filename.
\param aDestination Requested archive filename.
\returns Nothing.
\throw std::exception When a move or copy fails.
*/
void MoveSource(fs::path const& aSource, fs::path const& aDestination) {
   if(fs::exists(aDestination)) {
      throw std::runtime_error("Archive destination already exists");
      }
   std::error_code aError;
   fs::rename(aSource, aDestination, aError);
   if(!aError) {
      return;
      }
   bool bCrossDevice = (aError == std::errc::cross_device_link);
#ifdef _WIN32
   bCrossDevice = bCrossDevice || (aError.value() == ERROR_NOT_SAME_DEVICE);
#endif
   if(!bCrossDevice) {
      throw fs::filesystem_error("Cannot move source file", aSource, aDestination, aError);
      }
   fs::copy_file(aSource, aDestination, fs::copy_options::none);
   try {
      if(fs::file_size(aSource) != fs::file_size(aDestination)) {
         throw std::runtime_error("Archive size verification failed");
         }
      std::ifstream aOriginal(aSource, std::ios::binary);
      std::ifstream aCopy(aDestination, std::ios::binary);
      if(!aOriginal || !aCopy) {
         throw std::runtime_error("Cannot verify archive contents");
         }
      std::array<char, 64 * 1024> aOriginalBuffer{};
      std::array<char, 64 * 1024> aCopyBuffer{};
      while(true) {
         aOriginal.read(aOriginalBuffer.data(), aOriginalBuffer.size());
         aCopy.read(aCopyBuffer.data(), aCopyBuffer.size());
         std::streamsize const iSize = aOriginal.gcount();
         if(iSize != aCopy.gcount() ||
            std::memcmp(aOriginalBuffer.data(), aCopyBuffer.data(),
                        static_cast<std::size_t>(iSize)) != 0 ||
            aOriginal.bad() || aCopy.bad()) {
            throw std::runtime_error("Archive byte verification failed");
            }
         if(iSize == 0) {
            break;
            }
         }
      fs::remove(aSource);
      }
   catch(...) {
      std::error_code aCleanupError;
      fs::remove(aDestination, aCleanupError);
      throw;
      }
   }

} // namespace

/**
\brief Convert all matching input files into XLSX workbooks.
\details Handles errors per file; input files are moved only after their XLSX
was successfully saved, and existing workbooks are never deliberately replaced.
\param iArgc Number of command-line arguments.
\param pszArgv Argument strings (non-Windows fallback only).
\returns 0 on complete success, 1 on failed files, 2 for invalid arguments.
\throw None; errors are reported on stderr.
*/
int main(int iArgc, char** pszArgv) {
   try {
      Options const aOptions = ParseOptions(CommandLineArguments(iArgc, pszArgv));
      std::vector<fs::path> vecFiles;
      for(fs::directory_entry const& aEntry : fs::directory_iterator(aOptions.aInputDirectory)) {
         if(aEntry.is_regular_file() && HasExtension(aEntry.path(), aOptions.aExtension)) {
            vecFiles.push_back(aEntry.path());
            }
         }
      std::ranges::sort(vecFiles);
      std::size_t uSuccess = 0;
      std::size_t uFailures = 0;
      for(fs::path const& aInput : vecFiles) {
         fs::path aOutput = aOptions.aOutputDirectory / aInput.filename();
         aOutput.replace_extension(L".xlsx");
         fs::path const aArchive = aOptions.aArchiveDirectory / aInput.filename();
         try {
            if(fs::exists(aOutput)) {
               throw std::runtime_error("XLSX already exists; input will not be moved");
               }
            if(aOptions.bMoveFiles && fs::exists(aArchive)) {
               throw std::runtime_error("Archive file already exists; input will not be moved");
               }
            auto const aResult = adecc::csv_to_xlsx::ConvertFile(aInput, aOutput);
            if(aOptions.bMoveFiles) {
               MoveSource(aInput, aArchive);
               }
            ++uSuccess;
            std::println("[PASS] {} -> {} | rows={} max-columns={} delimiter={}",
               aInput.string(), aOutput.string(), aResult.uRows, aResult.uMaxColumns,
               aResult.chDelimiter == '\t' ? "TAB" : std::string(1, aResult.chDelimiter));
            }
         catch(std::exception const& aError) {
            ++uFailures;
            std::cerr << std::format("[FAIL] {}: {}", aInput.string(), aError.what()) << '\n';
            }
         }
      std::println("[RESULT] files={} succeeded={} failed={}", vecFiles.size(), uSuccess, uFailures);
      return uFailures == 0 ? 0 : 1;
      }
   catch(std::exception const& aError) {
      std::cerr << std::format("[ERROR] {}", aError.what()) << '\n';
      return 2;
      }
   }
