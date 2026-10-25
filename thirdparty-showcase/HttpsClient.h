// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file HttpsClient.h
\brief Public interface for the reusable synchronous HTTPS client.

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

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace adecc::net {

using https_headers =
   std::vector<std::pair<std::string, std::string>>;

struct https_response {
   unsigned status_code {};
   std::string reason;
   std::string body;
   };

class HttpsClient {
public:
   explicit HttpsClient(std::string host, std::string port = "443");

   [[nodiscard]]
   https_response Get(std::string_view target, https_headers const& headers = {}) const;

   [[nodiscard]]
   std::string const& Host() const noexcept { return host; }

   [[nodiscard]]
   std::string const& Port() const noexcept { return port; }

private:
   std::string host;
   std::string port;
};

} // namespace irr_example::net
