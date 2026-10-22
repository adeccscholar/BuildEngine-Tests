#pragma once

#include <Windows.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace adecc::compare {
   inline std::filesystem::path AnsiPath(std::string_view const svText) {
      if (svText.empty()) return {};
      int const iLength = MultiByteToWideChar(1252u, 0u, svText.data(),
         static_cast<int>(svText.size()), nullptr, 0);
      if (iLength == 0) throw std::runtime_error("CP1252 path conversion failed");
      std::wstring strUnicode(static_cast<std::size_t>(iLength), L'\0');
      if (MultiByteToWideChar(1252u, 0u, svText.data(),
         static_cast<int>(svText.size()), strUnicode.data(), iLength) == 0) {
         throw std::runtime_error("CP1252 path conversion failed");
         }
      return std::filesystem::path(std::move(strUnicode));
      }

   inline std::string AnsiName(std::filesystem::path const& aPath) {
      std::wstring const strUnicode = aPath.stem().wstring();
      if (strUnicode.empty()) return {};
      int const iLength = WideCharToMultiByte(1252u, WC_NO_BEST_FIT_CHARS,
         strUnicode.data(), static_cast<int>(strUnicode.size()), nullptr, 0, nullptr, nullptr);
      if (iLength == 0) throw std::runtime_error("CP1252 filename conversion failed");
      std::string strAnsi(static_cast<std::size_t>(iLength), '\0');
      BOOL boSubstituted = FALSE;
      if (WideCharToMultiByte(1252u, WC_NO_BEST_FIT_CHARS, strUnicode.data(),
         static_cast<int>(strUnicode.size()), strAnsi.data(), iLength,
         nullptr, &boSubstituted) == 0 || boSubstituted) {
         throw std::runtime_error("Filename cannot be represented in CP1252");
         }
      return strAnsi;
      }
}
