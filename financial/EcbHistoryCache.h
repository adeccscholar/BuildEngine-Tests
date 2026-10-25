// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbHistoryCache.h
\brief Non-template file operations for the ECB XML cache.

\details
This component performs only file-system operations. Parsing the ECB XML and
therefore instantiating the floating-point exchange-rate representation remains
inside the calling template in EcbData.h.
*/
#pragma once

#include "FinancialBasics.h"
#include "FinancialModel.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace adecc::financial::ecb {

   [[nodiscard]]
   std::filesystem::path DefaultEcbCacheFile();

   [[nodiscard]]
   std::optional<std::string> ReadCachedEcbHistoryXml(std::filesystem::path const& cacheFile);

   void ReplaceCachedEcbHistoryXml(std::filesystem::path const& cacheFile, std::string_view xml);

   [[nodiscard]]
   bool WasPublicationChecked(std::filesystem::path const& cacheFile, date_ty publicationDate);

   void MarkPublicationChecked(std::filesystem::path const& cacheFile, date_ty publicationDate);

} // namespace adecc::financial::ecb
