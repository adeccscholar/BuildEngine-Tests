// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file EcbHttpsDownload.cpp
\brief Downloads ECB exchange-rate history through the reusable HTTPS client.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2026 adecc Systemhaus GmbH

  \licenseblock{PolyForm-Noncommercial-1.0.0}
  Licensed under PolyForm Noncommercial License 1.0.0; see repository LICENSE.
  \endlicenseblock

  \note This file is part of the adecc Scholar project - Free educational
  materials for modern C++.
*/

#include "EcbHttpsDownload.h"
#include "HttpsClient.h"

#include <format>
#include <stdexcept>
#include <utility>

namespace adecc::financial::ecb {

namespace {

constexpr char ecb_host[]   = "www.ecb.europa.eu";
constexpr char ecb_target[] = "/stats/eurofxref/eurofxref-hist.xml";

} // namespace



std::string DownloadEcbHistoryXml() {
   adecc::net::HttpsClient client { ecb_host };

   auto response = client.Get(ecb_target, { { "Accept", "application/xml,text/xml,*/*" }  });

   if (response.status_code != 200) {
      throw std::runtime_error(std::format("ECB request failed: HTTP {} {}", response.status_code, response.reason));
      }

   if (response.body.empty()) {
      throw std::runtime_error("ECB returned an empty response.");
      }

   return std::move(response.body);
   }

} // namespace irr_example::ecb
