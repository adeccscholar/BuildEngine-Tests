// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file HttpsClient.h
\brief Public interface for the reusable synchronous HTTPS client.

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
