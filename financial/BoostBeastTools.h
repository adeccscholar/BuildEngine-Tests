// SPDX-FileCopyrightText: 2020-2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file BoostBeastTools.h
\brief Synchronous HTTP client utilities based on Boost.Beast and Boost.Asio.

\version 1.0
\date    02.08.2026
\author  Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2020-2026 adecc Systemhaus GmbH

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

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <string>
#include <format>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp = asio::ip::tcp;      

/**
\brief Simple synchronous HTTP client using Boost.Beast and Boost.Asio.

\details
This class establishes a TCP connection to a specified HTTP server (host and port) and enables synchronous HTTP GET requests.
It provides basic error handling and supports automatic reconnection if the connection is lost (e.g. due to timeout or server reset).
Typical usage includes requesting external APIs such as Open Meteo in server or state machine contexts.

\warning
This client operates synchronously and is not suitable for high-concurrency scenarios. For non-blocking use cases, consider asynchronous APIs.

*/
class HttpRequest {
private:
   asio::io_context ioContext_; ///< Internal IO context for network operations
   tcp::resolver resolver_;     ///< DNS resolver
   tcp::socket socket_;         ///< TCP socket for the connection
   std::string host_;           ///< Target host
   std::string port_;           ///< Target port
public:
   /**
     \brief Constructs and connects the HTTP client to a given host and port.

     \details
      Resolves the host and establishes a TCP connection. Throws if resolution or connection fails.

     \param host  The remote server hostname (e.g. "api.open-meteo.com").
     \param port  The remote server port as string (default "80").

     \throw std::runtime_error on resolution or connection failure.
   */
   HttpRequest(std::string host, std::string port = "80") : ioContext_(), resolver_(ioContext_), socket_(ioContext_),
               host_(std::move(host)), port_(std::move(port)) {
      boost::system::error_code ec;

     // const auto results = resolver_.resolve(host_, port_, ec);
      const auto results = resolver_.resolve(boost::asio::ip::tcp::v4(), host_, port_, ec);
      if (ec) {
         throw std::runtime_error(std::format("Failed to resolve {}:{} - {}", host_, port_, ec.message()));
         }

      boost::asio::connect(socket_, results.begin(), results.end(), ec);
      if (ec) {
         throw std::runtime_error(std::format("Failed to connect to {}:{} - {}", host_, port_, ec.message()));
         }
      }

   /**
      \brief Destructor: shuts down and closes the TCP socket if open.
   */
   ~HttpRequest() {
      if (socket_.is_open()) {
         boost::system::error_code ec;
         socket_.shutdown(tcp::socket::shutdown_both, ec);
         socket_.close(ec); // optional
         }
      }


   /**
      \brief Attempts to reconnect the TCP socket to the configured host and port.

      \details
       Closes the existing socket (if open), resolves the host, and re-establishes the connection.
       Throws on failure.

      \throw std::runtime_error if resolution or connection fails.
   */
   void reconnect() {
      boost::system::error_code ec;
      socket_.close(ec);
      // const auto results = resolver_.resolve(host_, port_, ec);
      const auto results = resolver_.resolve(boost::asio::ip::tcp::v4(), host_, port_, ec);
      if (ec) throw std::runtime_error("Reconnect: resolve failed: " + ec.message());
      boost::asio::connect(socket_, results.begin(), results.end(), ec);
      if (ec) throw std::runtime_error("Reconnect: connect failed: " + ec.message());
      }


   /**
     \brief Performs a synchronous HTTP GET request to the specified endpoint.
 
     \details
      Sends a GET request and returns the response body as a string.
      On typical lost connection errors (EOF, connection reset, broken pipe, end of stream), the client 
      attempts to reconnect and retries the request once.

     \param endpoint_path The HTTP resource path (e.g. "/v1/forecast?...").
     \returns The HTTP response body as string.

     \throw std::runtime_error on unrecoverable error.

     \note Example for simple implementation without reconnect
     \code{.cpp}
      std::string perform_get(std::string const& endpoint_path) {
         http::request<http::string_body> req{ http::verb::get, endpoint_path, 11 };
         req.set(http::field::host, host_);
         req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

         http::write(socket_, req);

         beast::flat_buffer buffer;
         http::response<http::string_body> res;

         http::read(socket_, buffer, res);
         return res.body();
         }
     \endcode
   */
   std::string perform_get(std::string const& endpoint_path) {
      http::request<http::string_body> req{ http::verb::get, endpoint_path, 11 };
      req.set(http::field::host, host_);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

      for (uint32_t attempt = 0; attempt < 2; ++attempt) {
         boost::system::error_code ec;
         try {
            http::write(socket_, req, ec);
            if (ec) throw boost::system::system_error(ec);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(socket_, buffer, res, ec);
            if (ec) throw boost::system::system_error(ec);
            return res.body();
            }
         catch (const boost::system::system_error& ex) {
            auto code = ex.code();           
            if (attempt == 0 && // check for typical lost of connections problems 
                (code == asio::error::eof         || code == asio::error::connection_reset ||
                 code == asio::error::broken_pipe || code == http::error::end_of_stream)
               ) {
               reconnect();
               continue; // retry once again after successful reconnect!
               }
            else {
               throw; // other error or second try to get the data
               }
            }
         }
      throw std::runtime_error("perform_get: unreachable code reached");
      }


};