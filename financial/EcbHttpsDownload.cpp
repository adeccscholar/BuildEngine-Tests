// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file EcbHttpsDownload.cpp
\brief Downloads ECB exchange-rate history through the reusable HTTPS client.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2026 adecc Systemhaus GmbH

  \licenseblock{MIT}
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
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
