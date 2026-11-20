#pragma once

#include "BarcodeSelection.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/**
\brief Metadata for a confirmed ISBN and its externally sourced cover.
\details Empty fields mean that the queried provider did not supply them.
   A source URL is retained to make each metadata assertion verifiable.
*/
struct BookMetadata {
   std::string strISBN;
   std::string strTitle;
   std::string strSubtitle;
   std::string strAuthors;
   std::string strPublisher;
   std::string strPublished;
   std::string strProvider;
   std::string strSourceURL;
   std::string strCoverURL;
   std::string strStatus;
   cv::Mat theCover;

   [[nodiscard]] bool Found() const noexcept { return !strTitle.empty(); }
};

/**
\brief Transports HTTPS responses with the installed shared libcurl.
\details Uses verified TLS, bounded response memory, redirects, short
   connect/read timeouts and the default BuildEngine curl trust configuration.
*/
class BookHttp final {
   struct CurlDeleter {
      void operator()(CURL* const pCurl) const noexcept {
         if(pCurl) curl_easy_cleanup(pCurl);
      }
   };
   static constexpr std::size_t uMaximumBytes=3*1024*1024;

   static std::size_t Write(char* const pBytes,std::size_t const uSize,
      std::size_t const uCount,void* const pUser) {
      auto& strData=*static_cast<std::string*>(pUser);
      std::size_t const uLength=uSize*uCount;
      if(uLength>uMaximumBytes || strData.size()>uMaximumBytes-uLength)
         return 0;
      strData.append(pBytes,uLength);
      return uLength;
   }

public:
   /**
   \brief Fetches one HTTPS resource using installed libcurl.
   \details No HTTP downgrade or unchecked TLS is permitted. The application
      invokes this only for explicit ISBN lookup requests.
   \param strURL Complete allowlisted HTTPS endpoint.
   \returns Response bytes on HTTP 200 or empty on 404.
   \throw std::runtime_error On transport errors, unsafe URL or HTTP failure.
   */
   [[nodiscard]] static std::string Get(std::string const& strURL) {
      bool const bAllowed=strURL.starts_with("https://openlibrary.org/") ||
         strURL.starts_with("https://covers.openlibrary.org/") ||
         strURL.starts_with("https://www.googleapis.com/") ||
         strURL.starts_with("https://books.google.com/");
      if(!bAllowed) throw std::runtime_error("Unapproved ISBN metadata URL");
      static bool const bCurlInitialized=[]() {
         if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)
            throw std::runtime_error("libcurl initialization failed");
         return true;
      }();
      (void)bCurlInitialized;
      std::unique_ptr<CURL,CurlDeleter> const pCurl(curl_easy_init());
      if(!pCurl) throw std::runtime_error("Cannot create libcurl session");
      std::string strData;
      curl_easy_setopt(pCurl.get(),CURLOPT_URL,strURL.c_str());
      curl_easy_setopt(pCurl.get(),CURLOPT_WRITEFUNCTION,&Write);
      curl_easy_setopt(pCurl.get(),CURLOPT_WRITEDATA,&strData);
      curl_easy_setopt(pCurl.get(),CURLOPT_CONNECTTIMEOUT_MS,2500L);
      curl_easy_setopt(pCurl.get(),CURLOPT_TIMEOUT_MS,6500L);
      curl_easy_setopt(pCurl.get(),CURLOPT_FOLLOWLOCATION,0L);
      curl_easy_setopt(pCurl.get(),CURLOPT_SSL_VERIFYPEER,1L);
      curl_easy_setopt(pCurl.get(),CURLOPT_SSL_VERIFYHOST,2L);
      curl_easy_setopt(pCurl.get(),CURLOPT_USERAGENT,
         "BuildEngine-Tests ISBN Showcase/1.0 (OpenLibrary/GoogleBooks)");
      CURLcode const eResult=curl_easy_perform(pCurl.get());
      if(eResult!=CURLE_OK)
         throw std::runtime_error(std::string("ISBN REST request failed: ")+
            curl_easy_strerror(eResult));
      long iStatus {};
      curl_easy_getinfo(pCurl.get(),CURLINFO_RESPONSE_CODE,&iStatus);
      if(iStatus==404) return {};
      if(iStatus!=200)
         throw std::runtime_error("ISBN REST HTTP "+std::to_string(iStatus));
      return strData;
   }
};

/**
\brief Fetches ISBN edition data from Open Library with Google Books fallback.
\details The lookup checks exact ISBN identifiers, never substitutes a fuzzy
   title search. An unavailable entry is returned as not found, not invented.
*/
class BookLookup final {
   [[nodiscard]] static std::string Field(nlohmann::json const& theData,
      char const* const szName) {
      if(!theData.is_object() || !theData.contains(szName) ||
         !theData.at(szName).is_string()) return {};
      return theData.at(szName).get<std::string>();
   }

   [[nodiscard]] static bool ContainsISBN(nlohmann::json const& theData,
      std::string const& strISBN) {
      if(!theData.is_array()) return false;
      for(auto const& theItem:theData)
         if(Field(theItem,"identifier")==strISBN) return true;
      return false;
   }

   static void LoadCover(BookMetadata& theBook) {
      if(theBook.strCoverURL.empty()) return;
      try {
         std::string const strBytes=BookHttp::Get(theBook.strCoverURL);
         if(strBytes.empty()) return;
         std::vector<unsigned char> const vecBytes(strBytes.begin(),strBytes.end());
         theBook.theCover=cv::imdecode(vecBytes,cv::IMREAD_COLOR);
      }
      catch(std::exception const&) {
         // Optional cover failures must not invalidate verified book metadata.
         theBook.strCoverURL.clear();
      }
   }

public:
   /**
   \brief Resolves a genuine ISBN-13 against publicly documented REST APIs.
   \details Tries Open Library's edition-level Books API first. When no match
      is returned, tries Google Books' ISBN field query. Results retain their
      provider and source URL. Covers are optional and may not be catalogued.
   \param strISBN Normalized 13-digit ISBN.
   \returns Book metadata, an explicit not-found result or an error status.
   \throw std::runtime_error For invalid ISBNs or JSON/HTTP errors.
   */
   [[nodiscard]] static BookMetadata Lookup(std::string const& strISBN) {
      if(!IsIsbn13(strISBN))
         throw std::runtime_error("ISBN REST request rejected: invalid ISBN-13");
      BookMetadata theBook;
      theBook.strISBN=strISBN;
      std::string const strOLURL="https://openlibrary.org/api/books?bibkeys=ISBN:"+
         strISBN+"&jscmd=data&format=json";
      try {
         std::string const strPayload=BookHttp::Get(strOLURL);
         if(!strPayload.empty()) {
            nlohmann::json const theRoot=nlohmann::json::parse(strPayload);
            std::string const strKey="ISBN:"+strISBN;
            if(theRoot.is_object() && theRoot.contains(strKey) &&
               theRoot.at(strKey).is_object()) {
               auto const& theEntry=theRoot.at(strKey);
               theBook.strTitle=Field(theEntry,"title");
               theBook.strSubtitle=Field(theEntry,"subtitle");
               theBook.strPublished=Field(theEntry,"publish_date");
               theBook.strSourceURL=Field(theEntry,"url");
               if(theBook.strSourceURL.empty())
                  theBook.strSourceURL="https://openlibrary.org/isbn/"+strISBN;
               if(theEntry.contains("authors") &&
                  theEntry.at("authors").is_array())
                  for(auto const& theAuthor:theEntry.at("authors")) {
                     std::string const strName=Field(theAuthor,"name");
                     if(!strName.empty()) {
                        if(!theBook.strAuthors.empty()) theBook.strAuthors+=", ";
                        theBook.strAuthors+=strName;
                     }
                  }
               if(theEntry.contains("publishers") &&
                  theEntry.at("publishers").is_array())
                  for(auto const& thePublisher:theEntry.at("publishers")) {
                     std::string const strName=Field(thePublisher,"name");
                     if(!strName.empty()) {
                        if(!theBook.strPublisher.empty()) theBook.strPublisher+=", ";
                        theBook.strPublisher+=strName;
                     }
                  }
               if(theEntry.contains("cover") &&
                  theEntry.at("cover").is_object())
                  theBook.strCoverURL=Field(theEntry.at("cover"),"medium");
               if(theBook.Found()) {
                  theBook.strProvider="Open Library";
                  theBook.strStatus="Book metadata found";
                  LoadCover(theBook);
                  return theBook;
               }
            }
         }
      }
      catch(std::exception const& theError) {
         theBook.strStatus="Open Library: "+std::string(theError.what());
      }

      std::string const strGoogleURL=
         "https://www.googleapis.com/books/v1/volumes?q=isbn:"+strISBN+"&maxResults=5";
      try {
         std::string const strPayload=BookHttp::Get(strGoogleURL);
         if(!strPayload.empty()) {
            nlohmann::json const theRoot=nlohmann::json::parse(strPayload);
            if(theRoot.is_object() && theRoot.contains("items") &&
               theRoot.at("items").is_array()) {
               for(auto const& theItem:theRoot.at("items")) {
                  if(!theItem.is_object() || !theItem.contains("volumeInfo") ||
                     !theItem.at("volumeInfo").is_object()) continue;
                  auto const& theInfo=theItem.at("volumeInfo");
                  if(!theInfo.contains("industryIdentifiers") ||
                     !ContainsISBN(theInfo.at("industryIdentifiers"),strISBN))
                     continue;
                  theBook.strTitle=Field(theInfo,"title");
                  if(theBook.strTitle.empty()) continue;
                  theBook.strSubtitle=Field(theInfo,"subtitle");
                  theBook.strPublisher=Field(theInfo,"publisher");
                  theBook.strPublished=Field(theInfo,"publishedDate");
                  theBook.strSourceURL=Field(theInfo,"infoLink");
                  if(theBook.strSourceURL.empty())
                     theBook.strSourceURL=strGoogleURL;
                  if(theInfo.contains("authors") &&
                     theInfo.at("authors").is_array())
                     for(auto const& theAuthor:theInfo.at("authors"))
                        if(theAuthor.is_string()) {
                           if(!theBook.strAuthors.empty()) theBook.strAuthors+=", ";
                           theBook.strAuthors+=theAuthor.get<std::string>();
                        }
                  if(theInfo.contains("imageLinks") &&
                     theInfo.at("imageLinks").is_object()) {
                     auto const& theImages=theInfo.at("imageLinks");
                     theBook.strCoverURL=Field(theImages,"thumbnail");
                     if(theBook.strCoverURL.empty())
                        theBook.strCoverURL=Field(theImages,"smallThumbnail");
                     if(theBook.strCoverURL.starts_with("http://"))
                        theBook.strCoverURL.replace(0,7,"https://");
                  }
                  theBook.strProvider="Google Books";
                  theBook.strStatus="Book metadata found";
                  LoadCover(theBook);
                  return theBook;
               }
            }
         }
      }
      catch(std::exception const& theError) {
         if(!theBook.strStatus.empty()) theBook.strStatus+="; ";
         theBook.strStatus+="Google Books: "+std::string(theError.what());
      }
      if(theBook.strStatus.empty())
         theBook.strStatus="No exact ISBN record in Open Library or Google Books";
      return theBook;
   }
};
