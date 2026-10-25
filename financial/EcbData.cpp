// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbData.cpp
\brief Implements non-template ECB XML acquisition and source reporting.

\details
This translation unit contains only the non-template cache, download,
validation and reporting logic. The typed XML conversion is defined in
\c EcbData.h and is instantiated by the application for the type it uses.

No external template declarations and no explicit template instantiations are
used in this file.

\version 1.2
\date    03.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#include "EcbData.h"

#include "EcbHistoryCache.h"

#include <chrono>
#include <filesystem>
#include <print>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace adecc::financial::ecb {

   namespace detail {

      /**
      \brief Verifies that the cached response has the expected ECB envelope.

      \param xml XML document obtained from the cache component.
      \throw std::runtime_error if the document is not an ECB XML response.
      */
      void ValidateEcbDocument(std::string_view xml) {
         constexpr std::string_view expected{ "<gesmes:Envelope" };

         if (xml.find(expected) == std::string_view::npos)
            throw std::runtime_error("Cached ECB response is not the expected XML document.");
         }


      /**
      \brief Writes the origin and state of the selected ECB XML document.

      \param output Stream receiving the status information.
      \param source Result returned by the cache component.
      */
      void PrintCacheResult(std::ostream& output, ecb_cache_result const& source) {

         std::println(output, "ECB XML cache: {}", source.cache_file.string());

         if (source.downloaded) {
            std::println(output, "ECB XML source: downloaded and stored in cache");
            }
         else if (source.used_fallback) {
            std::println(output, "ECB XML source: cache fallback after download failure");
            }
         else {
            std::println(output, "ECB XML source: existing cache file");
            }

         std::println(output,
            "ECB latest publication: {:%F}",
            std::chrono::sys_days{ source.latest_publication_date });
      }

   } // namespace details


   std::string LoadEcbXml(std::filesystem::path const& cache_file) {
      auto source = LoadCurrentEcbHistory(cache_file);
      ValidateEcbDocument(source.xml);
      return std::move(source.xml);
      }


   std::string LoadEcbXml(std::ostream& output, std::filesystem::path const& cache_file) {
      auto source = LoadCurrentEcbHistory(cache_file);
      PrintCacheResult(output, source);
      ValidateEcbDocument(source.xml);
      return std::move(source.xml);
      }

} // namespace adecc::ecb