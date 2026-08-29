#include <curl/curl.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool HasProtocol(curl_version_info_data const& theInfo, std::string_view const svProtocol) {
   if(theInfo.protocols == nullptr) return false;
   for(char const* const* pszProtocol = theInfo.protocols; *pszProtocol != nullptr; ++pszProtocol)
      if(svProtocol == *pszProtocol) return true;
   return false;
   }

bool HasFeature(curl_version_info_data const& theInfo, int const iFeature) noexcept {
   return (theInfo.features & iFeature) != 0;
   }

bool HasFeatureName(curl_version_info_data const& theInfo, std::string_view const svFeature) {
   if(theInfo.feature_names == nullptr) return false;
   for(char const* const* pszFeature = theInfo.feature_names; *pszFeature != nullptr; ++pszFeature)
      if(svFeature == *pszFeature) return true;
   return false;
   }

bool Require(bool const bCondition, std::string_view const svDescription) {
   std::cout << (bCondition ? "PASS: " : "FAIL: ") << svDescription << '\n';
   return bCondition;
   }

}

int main() {
   curl_version_info_data const* const pInfo = curl_version_info(CURLVERSION_NOW);
   if(pInfo == nullptr) {
      std::cerr << "FAIL: curl_version_info returned null\n";
      return EXIT_FAILURE;
      }

   std::cout << "libcurl: " << (pInfo->version != nullptr ? pInfo->version : "<unknown>") << '\n';
   std::cout << "ssl: " << (pInfo->ssl_version != nullptr ? pInfo->ssl_version : "<none>") << '\n';

   bool bSuccess = true;
   bSuccess = Require(HasProtocol(*pInfo, "http"), "HTTP protocol") && bSuccess;
   bSuccess = Require(HasProtocol(*pInfo, "https"), "HTTPS protocol") && bSuccess;
   bSuccess = Require(HasProtocol(*pInfo, "ldap"), "LDAP protocol") && bSuccess;
   bSuccess = Require(HasProtocol(*pInfo, "ldaps"), "LDAPS protocol") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_SSL), "SSL/TLS") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_IPV6), "IPv6") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_ASYNCHDNS), "asynchronous/threaded resolver") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_IDN), "IDN") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_SSPI), "Windows SSPI") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_NTLM), "NTLM") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_LIBZ), "zlib") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_BROTLI), "Brotli") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_ZSTD), "Zstd") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_UNICODE), "Unicode Windows API") && bSuccess;
   bSuccess = Require(HasFeature(*pInfo, CURL_VERSION_THREADSAFE), "thread-safe libcurl") && bSuccess;
   bSuccess = Require(HasFeatureName(*pInfo, "NativeCA"), "Windows native CA store") && bSuccess;

   std::string_view const svSsl = pInfo->ssl_version != nullptr ? pInfo->ssl_version : "";
   bSuccess = Require(svSsl.find("OpenSSL") != std::string_view::npos, "OpenSSL TLS backend") && bSuccess;

   std::cout << (bSuccess ? "curl installed-consumer runtime: PASS\n" : "curl installed-consumer runtime: FAIL\n");
   return bSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
   }
