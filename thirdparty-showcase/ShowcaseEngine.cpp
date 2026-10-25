// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file ShowcaseEngine.cpp
\brief Implementation of the BCC64X third-party integration showcase.
*/

#include "ShowcaseEngine.h"
#include "HttpsClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/crypto.h>
#include <zip.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace adecc::showcase {

   namespace {

      using json = nlohmann::json;

      struct Repository_Info {
         std::string strFullName;
         std::string strDescription;
         std::string strDefaultBranch;
         std::string strUpdatedAt;
         std::string strFetchedBy;
         std::uint64_t uStars {};
         std::uint64_t uForks {};
         std::uint64_t uOpenIssues {};
         };

      struct Curl_Result {
         long iStatusCode {};
         std::string strBody;
         std::string strContentType;
         std::string strCurlVersion;
         std::string strTlsBackend;
         };

      struct Curl_Handle_Deleter {
         /**
         \brief Releases a libcurl easy handle.
         \details The deleter is used by std::unique_ptr for exception-safe cleanup.
         \param pHandle Handle to release.
         \returns Nothing.
         \throw Nothing.
         */
         void operator()(CURL* pHandle) const noexcept {
            if (pHandle != nullptr) {
               curl_easy_cleanup(pHandle);
               }
            }
         };

      struct Curl_Header_Deleter {
         /**
         \brief Releases a libcurl header list.
         \details The deleter is used by std::unique_ptr for exception-safe cleanup.
         \param pHeaders Header list to release.
         \returns Nothing.
         \throw Nothing.
         */
         void operator()(curl_slist* pHeaders) const noexcept {
            if (pHeaders != nullptr) {
               curl_slist_free_all(pHeaders);
               }
            }
         };

      using Curl_Handle = std::unique_ptr<CURL, Curl_Handle_Deleter>;
      using Curl_Headers = std::unique_ptr<curl_slist, Curl_Header_Deleter>;

      class Curl_Global final {
      public:
         /**
         \brief Initializes the process-wide libcurl state.
         \details CURL_GLOBAL_DEFAULT is sufficient for the HTTPS client used by this demo.
         \returns Nothing.
         \throw std::runtime_error If libcurl initialization fails.
         */
         Curl_Global() {
            CURLcode const iResult = curl_global_init(CURL_GLOBAL_DEFAULT);
            if (iResult != CURLE_OK) {
               throw std::runtime_error(std::format("curl_global_init failed: {}",
                                                    curl_easy_strerror(iResult)));
               }
            }

         /**
         \brief Releases the process-wide libcurl state.
         \details The cleanup is paired with the constructor initialization.
         \returns Nothing.
         \throw Nothing.
         */
         ~Curl_Global() {
            curl_global_cleanup();
            }

         Curl_Global(Curl_Global const&) = delete;
         Curl_Global& operator=(Curl_Global const&) = delete;
         };

      /**
      \brief Appends libcurl response bytes to a std::string.
      \details libcurl calls this function for every received body block.
      \param pData Received byte block.
      \param uSize Size of one element.
      \param uCount Number of elements.
      \param pUserData Pointer to the destination std::string.
      \returns Number of consumed bytes.
      \throw Nothing.
      */
      std::size_t CurlWrite(char* pData, std::size_t uSize, std::size_t uCount,
                            void* pUserData) noexcept {
         std::size_t const uBytes = uSize * uCount;
         auto* pBody = static_cast<std::string*>(pUserData);

         try {
            pBody->append(pData, uBytes);
            return uBytes;
            }
         catch (...) {
            return 0;
            }
         }

      /**
      \brief Reads one public GitHub repository endpoint through libcurl.
      \details The request uses HTTPS, follows redirects, applies finite network timeouts and keeps
               peer and hostname verification enabled. On Windows the OpenSSL backend additionally
               uses the native Windows CA store through CURLSSLOPT_NATIVE_CA.
      \param svUrl Absolute HTTPS URL.
      \returns HTTP status, response body and content type.
      \throw std::runtime_error If libcurl fails or response metadata cannot be obtained.
      */
      [[nodiscard]]
      Curl_Result CurlGet(std::string_view svUrl) {
         Curl_Global theGlobal;
         Curl_Handle theHandle { curl_easy_init() };
         if (!theHandle) {
            throw std::runtime_error("curl_easy_init returned nullptr.");
            }

         Curl_Headers theHeaders;
         curl_slist* pHeaders = nullptr;
         pHeaders = curl_slist_append(pHeaders, "Accept: application/vnd.github+json");
         if (pHeaders == nullptr) {
            throw std::runtime_error("curl_slist_append failed.");
            }
         theHeaders.reset(pHeaders);

         std::string strBody;
         std::string strUrl { svUrl };
         std::array<char, CURL_ERROR_SIZE> aErrorBuffer {};

         curl_easy_setopt(theHandle.get(), CURLOPT_URL, strUrl.c_str());
         curl_easy_setopt(theHandle.get(), CURLOPT_HTTPHEADER, theHeaders.get());
         curl_easy_setopt(theHandle.get(), CURLOPT_USERAGENT, "adecc-bcc64x-thirdparty-showcase/1.0");
         curl_easy_setopt(theHandle.get(), CURLOPT_FOLLOWLOCATION, 1L);
         curl_easy_setopt(theHandle.get(), CURLOPT_CONNECTTIMEOUT, 8L);
         curl_easy_setopt(theHandle.get(), CURLOPT_TIMEOUT, 20L);
         curl_easy_setopt(theHandle.get(), CURLOPT_ERRORBUFFER, aErrorBuffer.data());
         curl_easy_setopt(theHandle.get(), CURLOPT_SSL_VERIFYPEER, 1L);
         curl_easy_setopt(theHandle.get(), CURLOPT_SSL_VERIFYHOST, 2L);
         curl_easy_setopt(theHandle.get(), CURLOPT_SSL_OPTIONS,
                          static_cast<long>(CURLSSLOPT_NATIVE_CA));
         curl_easy_setopt(theHandle.get(), CURLOPT_PROXY_SSL_OPTIONS,
                          static_cast<long>(CURLSSLOPT_NATIVE_CA));
         curl_easy_setopt(theHandle.get(), CURLOPT_WRITEFUNCTION, &CurlWrite);
         curl_easy_setopt(theHandle.get(), CURLOPT_WRITEDATA, &strBody);

         CURLcode const iPerformResult = curl_easy_perform(theHandle.get());
         if (iPerformResult != CURLE_OK) {
            long iVerifyResult = 0;
            curl_easy_getinfo(theHandle.get(), CURLINFO_SSL_VERIFYRESULT, &iVerifyResult);

            std::string_view const svDetails = aErrorBuffer.front() != '\0'
                                               ? std::string_view { aErrorBuffer.data() }
                                               : std::string_view { "no additional libcurl diagnostic" };

            throw std::runtime_error(
               std::format("libcurl HTTPS request failed: {}; TLS verify result: {}; details: {}",
                           curl_easy_strerror(iPerformResult), iVerifyResult, svDetails));
            }

         long iStatusCode = 0;
         curl_easy_getinfo(theHandle.get(), CURLINFO_RESPONSE_CODE, &iStatusCode);

         char* szContentType = nullptr;
         curl_easy_getinfo(theHandle.get(), CURLINFO_CONTENT_TYPE, &szContentType);

         curl_version_info_data const* const pVersion = curl_version_info(CURLVERSION_NOW);

         return Curl_Result {
            .iStatusCode = iStatusCode,
            .strBody = std::move(strBody),
            .strContentType = szContentType != nullptr ? szContentType : "",
            .strCurlVersion = pVersion != nullptr && pVersion->version != nullptr
                                 ? pVersion->version : "loaded",
            .strTlsBackend = pVersion != nullptr && pVersion->ssl_version != nullptr
                                ? pVersion->ssl_version : "TLS backend unknown"
            };
         }

      /**
      \brief Parses repository metadata from a GitHub REST response.
      \details Only fields used by the visible demonstration are retained.
      \param strBody JSON response body.
      \param strFetchedBy Human-readable transport identifier.
      \returns Typed repository metadata.
      \throw nlohmann::json::exception If mandatory JSON fields are absent or invalid.
      */
      [[nodiscard]]
      Repository_Info ParseRepository(std::string const& strBody,
                                      std::string strFetchedBy) {
         json const theJson = json::parse(strBody);

         return Repository_Info {
            .strFullName = theJson.at("full_name").get<std::string>(),
            .strDescription = theJson.value("description", std::string {}),
            .strDefaultBranch = theJson.at("default_branch").get<std::string>(),
            .strUpdatedAt = theJson.at("updated_at").get<std::string>(),
            .strFetchedBy = std::move(strFetchedBy),
            .uStars = theJson.at("stargazers_count").get<std::uint64_t>(),
            .uForks = theJson.at("forks_count").get<std::uint64_t>(),
            .uOpenIssues = theJson.at("open_issues_count").get<std::uint64_t>()
            };
         }

      /**
      \brief Converts typed repository metadata to JSON.
      \details The representation is intentionally explicit for the demo archive.
      \param theRepository Typed repository metadata.
      \returns JSON object representing the repository.
      \throw nlohmann::json::exception If JSON construction fails.
      */
      [[nodiscard]]
      json RepositoryToJson(Repository_Info const& theRepository) {
         return json {
            { "full_name", theRepository.strFullName },
            { "description", theRepository.strDescription },
            { "default_branch", theRepository.strDefaultBranch },
            { "updated_at", theRepository.strUpdatedAt },
            { "fetched_by", theRepository.strFetchedBy },
            { "stars", theRepository.uStars },
            { "forks", theRepository.uForks },
            { "open_issues", theRepository.uOpenIssues }
            };
         }

      /**
      \brief Compresses a text buffer with zlib.
      \details The function uses compress2 with the best compression level.
      \param strText Input text.
      \returns Compressed byte vector.
      \throw std::runtime_error If zlib compression fails.
      */
      [[nodiscard]]
      std::vector<unsigned char> Compress(std::string const& strText) {
         uLongf uCompressedSize = compressBound(static_cast<uLong>(strText.size()));
         std::vector<unsigned char> vecCompressed(static_cast<std::size_t>(uCompressedSize));

         int const iResult = compress2(vecCompressed.data(), &uCompressedSize,
                                       reinterpret_cast<Bytef const*>(strText.data()),
                                       static_cast<uLong>(strText.size()), Z_BEST_COMPRESSION);
         if (iResult != Z_OK) {
            throw std::runtime_error(std::format("zlib compress2 failed with code {}.", iResult));
            }

         vecCompressed.resize(static_cast<std::size_t>(uCompressedSize));
         return vecCompressed;
         }

      /**
      \brief Decompresses a zlib buffer to the expected text size.
      \details Used to verify the compressed artifact before and after ZIP storage.
      \param vecCompressed Compressed bytes.
      \param uExpectedSize Expected uncompressed text size.
      \returns Restored text.
      \throw std::runtime_error If zlib decompression fails.
      */
      [[nodiscard]]
      std::string Decompress(std::vector<unsigned char> const& vecCompressed,
                             std::size_t uExpectedSize) {
         std::string strResult(uExpectedSize, '\0');
         uLongf uResultSize = static_cast<uLongf>(uExpectedSize);

         int const iResult = uncompress(reinterpret_cast<Bytef*>(strResult.data()), &uResultSize,
                                        vecCompressed.data(),
                                        static_cast<uLong>(vecCompressed.size()));
         if (iResult != Z_OK) {
            throw std::runtime_error(std::format("zlib uncompress failed with code {}.", iResult));
            }

         strResult.resize(static_cast<std::size_t>(uResultSize));
         return strResult;
         }

      /**
      \brief Adds one memory buffer to an open libzip archive.
      \details libzip borrows the supplied buffer until zip_close completes.
      \param pZip Open archive handle.
      \param szName Entry name inside the archive.
      \param pData Buffer start.
      \param uSize Buffer size in bytes.
      \returns Nothing.
      \throw std::runtime_error If source creation or file insertion fails.
      */
      void AddZipBuffer(zip_t* pZip, char const* szName, void const* pData,
                        std::size_t uSize) {
         zip_source_t* pSource = zip_source_buffer(pZip, pData, uSize, 0);
         if (pSource == nullptr) {
            throw std::runtime_error(std::format("libzip cannot create source for '{}': {}",
                                                 szName, zip_strerror(pZip)));
            }

         if (zip_file_add(pZip, szName, pSource, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
            std::string const strError = zip_strerror(pZip);
            zip_source_free(pSource);
            throw std::runtime_error(std::format("libzip cannot add '{}': {}",
                                                 szName, strError));
            }
         }

      /**
      \brief Reads one complete archive entry.
      \details Entry size is obtained from zip_stat before allocating the result buffer.
      \param pZip Open archive handle.
      \param szName Entry name.
      \returns Entry bytes.
      \throw std::runtime_error If the entry cannot be located or read completely.
      */
      [[nodiscard]]
      std::vector<unsigned char> ReadZipEntry(zip_t* pZip, char const* szName) {
         zip_stat_t theStat {};
         zip_stat_init(&theStat);
         if (zip_stat(pZip, szName, ZIP_FL_ENC_UTF_8, &theStat) != 0) {
            throw std::runtime_error(std::format("libzip cannot stat '{}': {}",
                                                 szName, zip_strerror(pZip)));
            }

         zip_file_t* pFile = zip_fopen(pZip, szName, ZIP_FL_ENC_UTF_8);
         if (pFile == nullptr) {
            throw std::runtime_error(std::format("libzip cannot open '{}': {}",
                                                 szName, zip_strerror(pZip)));
            }

         std::vector<unsigned char> vecData(static_cast<std::size_t>(theStat.size));
         zip_int64_t const iRead = zip_fread(pFile, vecData.data(), theStat.size);
         zip_fclose(pFile);

         if (iRead != static_cast<zip_int64_t>(theStat.size)) {
            throw std::runtime_error(std::format("libzip read size mismatch for '{}'.", szName));
            }

         return vecData;
         }

      /**
      \brief Creates and verifies the final demonstration ZIP archive.
      \details Raw HTTP responses, the generated summary and its zlib representation are stored,
               then the archive is reopened and both summary representations are compared.
      \param theArchivePath Destination ZIP path.
      \param strBeastBody Raw Boost.Beast response.
      \param strCurlBody Raw libcurl response.
      \param strSummary Pretty-printed generated summary.
      \param vecCompressed zlib-compressed summary.
      \returns Number of verified archive entries.
      \throw std::runtime_error If any libzip or verification operation fails.
      */
      [[nodiscard]]
      std::size_t CreateAndVerifyArchive(std::filesystem::path const& theArchivePath,
                                         std::string const& strBeastBody,
                                         std::string const& strCurlBody,
                                         std::string const& strSummary,
                                         std::vector<unsigned char> const& vecCompressed) {
         std::string const strReadme =
            "adecc Scholar BCC64X Third-Party Showcase\n"
            "\n"
            "beast-zlib-repository.json : HTTPS via Boost.Beast + OpenSSL\n"
            "curl-libzip-repository.json: HTTPS via libcurl\n"
            "summary.json               : parsed and combined with nlohmann/json\n"
            "summary.json.zlib          : summary compressed with zlib\n";

         int iZipError = 0;
         std::string const strArchivePath = theArchivePath.string();
         zip_t* pZip = zip_open(strArchivePath.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &iZipError);
         if (pZip == nullptr) {
            zip_error_t theError;
            zip_error_init_with_code(&theError, iZipError);
            std::string const strError = zip_error_strerror(&theError);
            zip_error_fini(&theError);
            throw std::runtime_error(std::format("libzip cannot create archive: {}", strError));
            }

         try {
            AddZipBuffer(pZip, "beast-zlib-repository.json", strBeastBody.data(), strBeastBody.size());
            AddZipBuffer(pZip, "curl-libzip-repository.json", strCurlBody.data(), strCurlBody.size());
            AddZipBuffer(pZip, "summary.json", strSummary.data(), strSummary.size());
            AddZipBuffer(pZip, "summary.json.zlib", vecCompressed.data(), vecCompressed.size());
            AddZipBuffer(pZip, "README.txt", strReadme.data(), strReadme.size());
            }
         catch (...) {
            zip_discard(pZip);
            throw;
            }

         if (zip_close(pZip) != 0) {
            std::string const strError = zip_strerror(pZip);
            zip_discard(pZip);
            throw std::runtime_error(std::format("libzip cannot close archive: {}", strError));
            }

         pZip = zip_open(strArchivePath.c_str(), ZIP_RDONLY, &iZipError);
         if (pZip == nullptr) {
            throw std::runtime_error("libzip cannot reopen the generated archive.");
            }

         try {
            zip_int64_t const iEntries = zip_get_num_entries(pZip, 0);
            if (iEntries != 5) {
               throw std::runtime_error(std::format("Expected 5 ZIP entries, found {}.", iEntries));
               }

            std::vector<unsigned char> const vecSummary = ReadZipEntry(pZip, "summary.json");
            std::string const strSummaryFromZip(vecSummary.begin(), vecSummary.end());
            if (strSummaryFromZip != strSummary) {
               throw std::runtime_error("summary.json changed during ZIP roundtrip.");
               }

            std::vector<unsigned char> const vecCompressedFromZip =
               ReadZipEntry(pZip, "summary.json.zlib");
            std::string const strInflated = Decompress(vecCompressedFromZip, strSummary.size());
            if (strInflated != strSummary) {
               throw std::runtime_error("zlib payload changed during ZIP roundtrip.");
               }

            zip_close(pZip);
            return static_cast<std::size_t>(iEntries);
            }
         catch (...) {
            zip_close(pZip);
            throw;
            }
         }

      /**
      \brief Emits one visible progress row.
      \details Empty callbacks are accepted for non-visual reuse.
      \param fnUpdate Progress callback.
      \param theUpdate Update value to emit.
      \returns Nothing.
      \throw Any exception thrown by the callback.
      */
      void Emit(Showcase_Callback const& fnUpdate, Showcase_Update theUpdate) {
         if (fnUpdate) {
            fnUpdate(theUpdate);
            }
         }

   } // namespace

   Showcase_Result RunShowcase(std::filesystem::path const& theOutputDirectory,
                               Showcase_Callback const& fnUpdate) {
      std::filesystem::create_directories(theOutputDirectory);

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "Boost.Beast",
         .strTask = "HTTPS GET api.github.com/repos/madler/zlib",
         .strStatus = "RUNNING",
         .strResult = "TLS handshake + HTTP/1.1",
         .strDetails = "Using the supplied adecc::net::HttpsClient"
         });

      adecc::net::HttpsClient theHttpsClient { "api.github.com" };
      adecc::net::https_response const theBeastResponse = theHttpsClient.Get(
         "/repos/madler/zlib",
         {
            { "Accept", "application/vnd.github+json" }
         });

      if (theBeastResponse.status_code != 200U) {
         throw std::runtime_error(std::format("Boost.Beast request returned HTTP {} {}.",
                                              theBeastResponse.status_code,
                                              theBeastResponse.reason));
         }

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "Boost.Beast",
         .strTask = "HTTPS GET",
         .strStatus = "OK",
         .strResult = std::format("HTTP 200, {} bytes", theBeastResponse.body.size()),
         .strDetails = "zlib repository metadata received"
         });

      char const* const szOpenSslVersion = OpenSSL_version(OPENSSL_VERSION);
      Emit(fnUpdate, Showcase_Update {
         .strComponent = "OpenSSL",
         .strTask = "TLS peer verification",
         .strStatus = "OK",
         .strResult = szOpenSslVersion != nullptr ? szOpenSslVersion : "OpenSSL runtime loaded",
         .strDetails = "Beast TLS handshake completed with peer verification"
         });

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "libcurl",
         .strTask = "HTTPS GET api.github.com/repos/nih-at/libzip",
         .strStatus = "RUNNING",
         .strResult = "Independent HTTP stack",
         .strDetails = "libcurl/OpenSSL with Windows native CA verification"
         });

      Curl_Result const theCurlResponse = CurlGet("https://api.github.com/repos/nih-at/libzip");
      if (theCurlResponse.iStatusCode != 200) {
         throw std::runtime_error(std::format("libcurl request returned HTTP {}.",
                                              theCurlResponse.iStatusCode));
         }

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "libcurl",
         .strTask = "HTTPS GET",
         .strStatus = "OK",
         .strResult = std::format("HTTP 200, {} bytes", theCurlResponse.strBody.size()),
         .strDetails = std::format("libcurl {} / {}", theCurlResponse.strCurlVersion,
                                   theCurlResponse.strTlsBackend + " / native Windows CA")
         });

      Repository_Info const theZlibRepository = ParseRepository(
         theBeastResponse.body, "Boost.Beast + OpenSSL");
      Repository_Info const theLibzipRepository = ParseRepository(
         theCurlResponse.strBody, "libcurl");

      json const theSummary = {
         { "showcase", "adecc Scholar BCC64X Third-Party Showcase" },
         { "repositories", json::array({
            RepositoryToJson(theZlibRepository),
            RepositoryToJson(theLibzipRepository)
            }) },
         { "libraries", {
            { "boost_beast", "HTTPS transport A" },
            { "openssl", szOpenSslVersion != nullptr ? szOpenSslVersion : "loaded" },
            { "libcurl", theCurlResponse.strCurlVersion },
            { "nlohmann_json", std::format("{}.{}.{}", NLOHMANN_JSON_VERSION_MAJOR,
                                                 NLOHMANN_JSON_VERSION_MINOR,
                                                 NLOHMANN_JSON_VERSION_PATCH) },
            { "zlib", zlibVersion() },
            { "libzip", zip_libzip_version() }
            } }
         };

      std::string const strSummary = theSummary.dump(2);

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "nlohmann/json",
         .strTask = "Parse + typed projection + combined JSON",
         .strStatus = "OK",
         .strResult = std::format("2 documents -> {} byte summary", strSummary.size()),
         .strDetails = std::format("{}: {} stars | {}: {} stars",
                                   theZlibRepository.strFullName, theZlibRepository.uStars,
                                   theLibzipRepository.strFullName, theLibzipRepository.uStars)
         });

      std::vector<unsigned char> const vecCompressed = Compress(strSummary);
      std::string const strInflated = Decompress(vecCompressed, strSummary.size());
      if (strInflated != strSummary) {
         throw std::runtime_error("zlib memory roundtrip verification failed.");
         }

      double const flRatio = strSummary.empty()
         ? 0.0
         : 100.0 * static_cast<double>(vecCompressed.size()) /
           static_cast<double>(strSummary.size());

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "zlib",
         .strTask = "Compress + uncompress verification",
         .strStatus = "OK",
         .strResult = std::format("{} -> {} bytes ({:.1f}%)",
                                  strSummary.size(), vecCompressed.size(), flRatio),
         .strDetails = zlibVersion()
         });

      std::filesystem::path const theArchivePath =
         theOutputDirectory / "thirdparty-showcase-result.zip";
      std::size_t const uArchiveEntries = CreateAndVerifyArchive(
         theArchivePath, theBeastResponse.body, theCurlResponse.strBody,
         strSummary, vecCompressed);

      Emit(fnUpdate, Showcase_Update {
         .strComponent = "libzip",
         .strTask = "Create + reopen + verify result archive",
         .strStatus = "OK",
         .strResult = std::format("{} entries", uArchiveEntries),
         .strDetails = theArchivePath.string()
         });

      return Showcase_Result {
         .theArchivePath = theArchivePath,
         .strSummaryJson = strSummary,
         .uSummaryBytes = strSummary.size(),
         .uCompressedBytes = vecCompressed.size(),
         .uArchiveEntries = uArchiveEntries
         };
      }

} // namespace adecc::showcase
