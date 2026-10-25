// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file ShowcaseEngine.h
\brief Integration engine for the BCC64X third-party showcase.
\details The engine combines Boost.Beast/OpenSSL, libcurl, nlohmann/json,
         zlib and libzip in one reproducible demonstration workflow.
*/

#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace adecc::showcase {

   /**
   \brief Visible progress information for one showcase component.
   \details Each update addresses one row in the VCL status matrix.
   */
   struct Showcase_Update {
      std::string strComponent;
      std::string strTask;
      std::string strStatus;
      std::string strResult;
      std::string strDetails;
      };

   /**
   \brief Final result of the complete showcase run.
   \details The result contains the generated JSON summary and the verified ZIP archive path.
   */
   struct Showcase_Result {
      std::filesystem::path theArchivePath;
      std::string strSummaryJson;
      std::size_t uSummaryBytes {};
      std::size_t uCompressedBytes {};
      std::size_t uArchiveEntries {};
      };

   using Showcase_Callback = std::function<void(Showcase_Update const&)>;

   /**
   \brief Executes the complete third-party integration showcase.
   \details The method performs two real HTTPS requests through independent stacks,
            parses both responses, compresses the combined summary, writes a ZIP archive
            and reopens that archive for verification.
   \param theOutputDirectory Directory receiving the generated ZIP archive.
   \param fnUpdate Callback receiving visible progress updates after every successful stage.
   \returns The verified showcase result.
   \throw std::runtime_error If any HTTPS, JSON, zlib or libzip operation fails.
   */
   [[nodiscard]]
   Showcase_Result RunShowcase(std::filesystem::path const& theOutputDirectory,
                               Showcase_Callback const& fnUpdate);

} // namespace adecc::showcase
