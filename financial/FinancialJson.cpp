// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file IrrJson.cpp
\brief JSON file loading for IRR test data.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#include "FinancialJson.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include <array>

#include <boost/json/parse.hpp>
#include <boost/system/system_error.hpp>

#include <fstream>
#include <iterator>

namespace adecc::financial::detail {

boost::json::value ReadJson(std::filesystem::path const& filename) {
   std::filesystem::path source = filename;
#ifdef _WIN32
   if (!std::filesystem::exists(source) && !filename.has_parent_path()) {
      std::array<wchar_t, 32768> exePath{};
      auto const length = GetModuleFileNameW(nullptr, exePath.data(),
         static_cast<DWORD>(exePath.size()));
      if (length > 0 && length < exePath.size()) {
         auto const candidate = std::filesystem::path(
            std::wstring_view(exePath.data(), length)).parent_path()
            / "Tests" / filename;
         if (std::filesystem::exists(candidate)) source = candidate;
         }
      }
#endif
   std::ifstream input { source, std::ios::binary };
   if (!input)
      throw std::runtime_error(std::format(
         "file '{}' could not be opened", filename.string()));

   std::string const json_text { std::istreambuf_iterator<char> { input },
                                 std::istreambuf_iterator<char> {} };
   try {
      return boost::json::parse(json_text);
      }
   catch (boost::system::system_error const& error) {
      throw std::runtime_error(std::format(
         "invalid JSON in '{}': {}", filename.string(), error.what()));
      }
   }

} // namespace irr_example::detail
