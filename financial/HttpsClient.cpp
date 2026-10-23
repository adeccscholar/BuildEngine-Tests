// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file HttpsClient.cpp
\brief Reusable synchronous HTTPS client based on Boost.Beast and OpenSSL.

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

#include "HttpsClient.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/system/system_error.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#ifdef _WIN32
#include <Windows.h>
#include <wincrypt.h>
#endif

#include <array>
#include <format>
#include <memory>
#include <stdexcept>
#include <utility>
#include <string>
#include <vector>


namespace adecc::net {

   namespace asio = boost::asio;
   namespace beast = boost::beast;
   namespace http = beast::http;
   namespace ssl = asio::ssl;

   using tcp = asio::ip::tcp;

   namespace details {

      [[nodiscard]]
      std::string OpenSslErrorText(unsigned long error) {
         if (error == 0) {
            return "no OpenSSL error information";
         }

         std::array<char, 256> text{};
         ERR_error_string_n(error, text.data(), text.size());
         return text.data();
      }

#ifdef _WIN32

      struct certificate_store_deleter {
         void operator()(void* value) const noexcept {
            if (value != nullptr) {
               CertCloseStore(
                  static_cast<HCERTSTORE>(value),
                  0);
            }
         }
      };

      using certificate_store_ptr =
         std::unique_ptr<void, certificate_store_deleter>;

      struct certificate_context_deleter {
         void operator()(CERT_CONTEXT const* value) const noexcept {
            if (value != nullptr) {
               CertFreeCertificateContext(value);
            }
         }
      };

      using certificate_context_ptr =
         std::unique_ptr<CERT_CONTEXT const, certificate_context_deleter>;

      struct certificate_chain_deleter {
         void operator()(CERT_CHAIN_CONTEXT const* value) const noexcept {
            if (value != nullptr) {
               CertFreeCertificateChain(value);
            }
         }
      };

      using certificate_chain_ptr =
         std::unique_ptr<CERT_CHAIN_CONTEXT const, certificate_chain_deleter>;

      struct windows_verify_data {
         std::wstring hostname;
         DWORD policy_error{};
         DWORD system_error{};
      };

      [[nodiscard]]
      std::wstring ToWideString(char const* text) {
         auto const length = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text,
            -1,
            nullptr,
            0);

         if (length <= 0) {
            throw std::runtime_error(
               std::format(
                  "Cannot convert hostname to UTF-16: Windows error {}",
                  GetLastError()));
         }

         std::wstring result(
            static_cast<std::size_t>(length),
            L'\0');

         if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text,
            -1,
            result.data(),
            length) <= 0) {
            throw std::runtime_error(
               std::format(
                  "Cannot convert hostname to UTF-16: Windows error {}",
                  GetLastError()));
         }

         result.pop_back();
         return result;
      }

      [[nodiscard]]
      certificate_context_ptr ToWindowsCertificate(X509* certificate) {
         auto const encoded_size = i2d_X509(
            certificate,
            nullptr);

         if (encoded_size <= 0) {
            return {};
         }

         std::vector<unsigned char> encoded(
            static_cast<std::size_t>(encoded_size));

         auto* position = encoded.data();
         if (i2d_X509(certificate, &position) != encoded_size) {
            return {};
         }

         return certificate_context_ptr{
            CertCreateCertificateContext(
               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
               encoded.data(),
               static_cast<DWORD>(encoded.size()))
         };
      }

      [[nodiscard]]
      certificate_store_ptr CreateAdditionalCertificateStore(
         X509_STORE_CTX* openssl_context,
         X509* leaf_certificate) {
         certificate_store_ptr store{
            CertOpenStore(
               CERT_STORE_PROV_MEMORY,
               0,
               0,
               CERT_STORE_CREATE_NEW_FLAG,
               nullptr)
         };

         if (!store) {
            return {};
         }

         auto* untrusted =
            X509_STORE_CTX_get0_untrusted(openssl_context);

         if (untrusted == nullptr) {
            return store;
         }

         auto const count = sk_X509_num(untrusted);
         for (int index = 0; index < count; ++index) {
            auto* certificate = sk_X509_value(
               untrusted,
               index);

            if (certificate == nullptr ||
               certificate == leaf_certificate) {
               continue;
            }

            auto windows_certificate =
               ToWindowsCertificate(certificate);

            if (!windows_certificate) {
               continue;
            }

            if (!CertAddCertificateContextToStore(
               static_cast<HCERTSTORE>(store.get()),
               windows_certificate.get(),
               CERT_STORE_ADD_USE_EXISTING,
               nullptr)) {
               return {};
            }
         }

         return store;
      }

      int VerifyWithWindows(
         X509_STORE_CTX* openssl_context,
         void* argument) noexcept {
         auto* verify_data =
            static_cast<windows_verify_data*>(argument);

         verify_data->policy_error = 0;
         verify_data->system_error = 0;

         try {
            auto* leaf_certificate =
               X509_STORE_CTX_get0_cert(openssl_context);

            if (leaf_certificate == nullptr) {
               X509_STORE_CTX_set_error(
                  openssl_context,
                  X509_V_ERR_APPLICATION_VERIFICATION);
               return 0;
            }

            auto windows_leaf =
               ToWindowsCertificate(leaf_certificate);

            if (!windows_leaf) {
               X509_STORE_CTX_set_error(openssl_context, X509_V_ERR_UNSPECIFIED);
               return 0;
               }

            auto additional_store = CreateAdditionalCertificateStore(openssl_context, leaf_certificate);

            if (!additional_store) {
               verify_data->system_error = GetLastError();
               X509_STORE_CTX_set_error(openssl_context, X509_V_ERR_UNSPECIFIED);
               return 0;
               }

            LPSTR usages[] = { const_cast<LPSTR>(szOID_PKIX_KP_SERVER_AUTH)  };

            CERT_CHAIN_PARA chain_parameters{};
            chain_parameters.cbSize = sizeof(chain_parameters);
            chain_parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
            chain_parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
            chain_parameters.RequestedUsage.Usage.rgpszUsageIdentifier = usages;

            CERT_CHAIN_CONTEXT const* raw_chain = nullptr;
            if (!CertGetCertificateChain(nullptr, windows_leaf.get(), nullptr, static_cast<HCERTSTORE>(additional_store.get()),
                                         &chain_parameters, CERT_CHAIN_CACHE_END_CERT | CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT,
                                         nullptr, &raw_chain)) {
               verify_data->system_error = GetLastError();
               X509_STORE_CTX_set_error(openssl_context, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
               return 0;
               }

            certificate_chain_ptr chain { raw_chain };

            SSL_EXTRA_CERT_CHAIN_POLICY_PARA ssl_parameters{};
            ssl_parameters.cbSize = sizeof(ssl_parameters);
            ssl_parameters.dwAuthType = AUTHTYPE_SERVER;
            ssl_parameters.fdwChecks = 0;
            ssl_parameters.pwszServerName = const_cast<wchar_t*>(verify_data->hostname.c_str());

            CERT_CHAIN_POLICY_PARA policy_parameters{ };
            policy_parameters.cbSize = sizeof(policy_parameters);
            policy_parameters.pvExtraPolicyPara = &ssl_parameters;

            CERT_CHAIN_POLICY_STATUS policy_status{};
            policy_status.cbSize = sizeof(policy_status);

            if (!CertVerifyCertificateChainPolicy(
               CERT_CHAIN_POLICY_SSL,
               chain.get(),
               &policy_parameters,
               &policy_status)) {
               verify_data->system_error = GetLastError();
               X509_STORE_CTX_set_error(
                  openssl_context,
                  X509_V_ERR_APPLICATION_VERIFICATION);
               return 0;
            }

            if (policy_status.dwError != 0) {
               verify_data->policy_error =
                  policy_status.dwError;
               X509_STORE_CTX_set_error(
                  openssl_context,
                  X509_V_ERR_CERT_REJECTED);
               return 0;
            }

            X509_STORE_CTX_set_error(
               openssl_context,
               X509_V_OK);
            return 1;
         }
         catch (...) {
            X509_STORE_CTX_set_error(
               openssl_context,
               X509_V_ERR_UNSPECIFIED);
            return 0;
         }
      }

#endif

   } // namespace

   HttpsClient::HttpsClient(
      std::string host,
      std::string port)
      : host { std::move(host) },
        port { std::move(port) } {

      if (this->host.empty()) {
         throw std::invalid_argument(
            "HTTPS host must not be empty.");
      }

      if (this->port.empty()) {
         throw std::invalid_argument(
            "HTTPS port must not be empty.");
      }
   }

   https_response HttpsClient::Get(std::string_view target, https_headers const& headers) const {

      if (target.empty() || target.front() != '/') {
         throw std::invalid_argument("HTTPS request target must begin with '/'.");
         }

      asio::io_context io_context;

#ifdef _WIN32
      details::windows_verify_data verify_data {
         .hostname = details::ToWideString(host.c_str())
         };
#endif

      ssl::context ssl_context { ssl::context::tls_client };
      ssl_context.set_options(ssl::context::default_workarounds);
      ssl_context.set_verify_mode(ssl::verify_peer);

#ifdef _WIN32
      SSL_CTX_set_cert_verify_callback(ssl_context.native_handle(), &details::VerifyWithWindows, &verify_data);
#else
      ssl_context.set_default_verify_paths();
#endif

      tcp::resolver resolver { io_context };

      beast::ssl_stream<beast::tcp_stream> stream {
         io_context,
         ssl_context
         };

      if (SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()) != 1) {
         auto const error = ERR_get_error();

         throw std::runtime_error(
            std::format("Cannot set TLS SNI hostname '{}': {}", host, details::OpenSslErrorText(error)));
         }

#ifndef _WIN32
      stream.set_verify_callback(
         ssl::host_name_verification{
            host
         });
#endif

      auto const endpoints = resolver.resolve(host, port);

      beast::get_lowest_layer(stream).connect(endpoints);

      try {
         stream.handshake(ssl::stream_base::client);
         }
      catch (boost::system::system_error const& error) {
         auto const verify_result = SSL_get_verify_result(stream.native_handle());
         auto const* verify_text =  X509_verify_cert_error_string(verify_result);

#ifdef _WIN32
         throw std::runtime_error(
            std::format("TLS handshake with '{}' failed: {}; OpenSSL verification: {} ({}); Windows policy error: 0x{:08X}; "
                        "Windows system error: {}", host,  error.what(),
                        verify_text != nullptr ? verify_text : "unknown verification error",
                        verify_result, verify_data.policy_error, verify_data.system_error));
#else
         throw std::runtime_error(std::format("TLS handshake with '{}' failed: {}; certificate verification: {} ({})",
                                              host, error.what(), verify_text != nullptr ? verify_text : "unknown verification error",
                                              verify_result));
#endif
      }

      http::request<http::empty_body> request { http::verb::get, target,  11  };

      request.set(http::field::host, host);
      request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

      for (auto const& [name, value] : headers) {
         request.set(name, value);
         }

      http::write(stream, request);

      beast::flat_buffer buffer;
      http::response<http::string_body> response;

      http::read(stream, buffer, response);

      https_response result {
         .status_code = response.result_int(),
         .reason = std::string { response.reason() },
         .body = std::move(response.body())
         };

      beast::error_code shutdown_error;
      stream.shutdown(shutdown_error);

      if (shutdown_error == asio::error::eof || shutdown_error == ssl::error::stream_truncated) {
         shutdown_error = {};
         }

      if (shutdown_error) {
         throw beast::system_error{ shutdown_error };
         }

      return result;
      }


} // namespace irr_example::net
