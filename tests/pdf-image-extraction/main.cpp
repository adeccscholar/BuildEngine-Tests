// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

#include <cctype>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <print>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace std::literals;

/**
\brief Reads an integer value from a PDF dictionary.
\details Returns -1 when the key is absent or does not contain an integer value.
\param theDictionary PDF dictionary to inspect.
\param svKey PDF dictionary key including its leading slash.
\returns Integer value or -1.
\throw Does not intentionally throw.
*/
long long ReadInteger(QPDFObjectHandle const& theDictionary, std::string_view const svKey) noexcept {
   try {
      QPDFObjectHandle const theValue = theDictionary.getKey(std::string(svKey));
      return theValue.isInteger() ? theValue.getIntValue() : -1LL;
      }
   catch(...) {
      return -1LL;
      }
   }

/**
\brief Converts a PDF value into a compact diagnostic string.
\details Null values are represented by a single dash.
\param theValue PDF value to convert.
\returns PDF syntax produced by QPDF or a dash for null.
\throw std::runtime_error If QPDF cannot serialize the value.
*/
std::string Describe(QPDFObjectHandle const& theValue) {
   return theValue.null() ? "-" : theValue.unparse();
   }

/**
\brief Creates a file-system-safe part of an output file name.
\details Non-alphanumeric characters are replaced by underscores.
\param svValue Text to sanitize.
\returns Sanitized string.
\throw std::bad_alloc If memory allocation fails.
*/
std::string Sanitize(std::string_view const svValue) {
   std::string strResult;
   strResult.reserve(svValue.size());

   for(unsigned char const uChar : svValue) {
      if(std::isalnum(uChar) != 0) {
         strResult.push_back(static_cast<char>(uChar));
         }
      else {
         strResult.push_back('_');
         }
      }

   return strResult;
   }

/**
\brief Selects the extension for an unchanged raw PDF image stream.
\details Only single DCTDecode and JPXDecode streams are labelled as directly usable image files.
\param strFilter Serialized PDF filter value.
\returns File extension including the leading dot.
\throw Does not throw.
*/
std::string_view RawExtension(std::string const& strFilter) noexcept {
   if(strFilter == "/DCTDecode") {
      return ".jpg"sv;
      }
   else if(strFilter == "/JPXDecode") {
      return ".jp2"sv;
      }

   return ".bin"sv;
   }

/**
\brief Writes an unchanged PDF stream buffer to a file.
\details No decoding, rendering, color conversion or resampling is performed.
\param theImage Image stream object.
\param theOutput Output path.
\returns Number of bytes written.
\throw std::runtime_error If the output file cannot be created or written.
*/
std::size_t WriteRawStream(QPDFObjectHandle& theImage, std::filesystem::path const& theOutput) {
   std::shared_ptr<Buffer> const pBuffer = theImage.getRawStreamData();
   if(pBuffer == nullptr) {
      throw std::runtime_error("QPDF returned no raw stream buffer");
      }

   std::ofstream theFile(theOutput, std::ios::binary | std::ios::trunc);
   if(!theFile) {
      throw std::runtime_error("Unable to create output file: " + theOutput.string());
      }

   std::size_t const uSize = pBuffer->getSize();
   theFile.write(
      reinterpret_cast<char const*>(pBuffer->getBuffer()),
      static_cast<std::streamsize>(uSize));

   if(!theFile) {
      throw std::runtime_error("Unable to write output file: " + theOutput.string());
      }

   return uSize;
   }

/**
\brief Runs the PDF image extraction repro.
\details Enumerates image XObjects recursively through page and form resources and exports each
unique indirect stream once.
\param strInput Input PDF file.
\param theOutputDirectory Directory receiving image streams and images.tsv.
\returns EXIT_SUCCESS when the PDF was inspected successfully.
\throw std::exception Propagates QPDF and file-system failures to main.
*/
int Run(
   std::string const& strInput,
   std::filesystem::path const& theOutputDirectory) {

   std::filesystem::create_directories(theOutputDirectory);

   QPDF theDocument;
   theDocument.processFile(strInput.c_str());

   std::vector<QPDFPageObjectHelper> vecPages =
      QPDFPageDocumentHelper::get(theDocument).getAllPages();

   std::ofstream theManifest(theOutputDirectory / "images.tsv", std::ios::trunc);
   if(!theManifest) {
      throw std::runtime_error("Unable to create images.tsv");
      }

   theManifest
      << "page\tresource\tobject\twidth\theight\tbpc\tcolor-space\tfilter"
         "\timage-mask\tmask\tsoft-mask\traw-bytes\tfile\n";

   std::set<std::string> setExtractedObjects;
   std::size_t uOccurrences = 0U;
   std::size_t uUniqueImages = 0U;

   std::println(
      "PDF|version={}|pages={}",
      theDocument.getPDFVersion(),
      vecPages.size());

   for(std::size_t uPage = 0U; uPage < vecPages.size(); ++uPage) {
      vecPages[uPage].forEachImage(
         true,
         [&](QPDFObjectHandle& theImage,
             QPDFObjectHandle&,
             std::string const& strResourceName) {

            ++uOccurrences;

            QPDFObjectHandle const theDictionary = theImage.getDict();
            long long const iWidth = ReadInteger(theDictionary, "/Width");
            long long const iHeight = ReadInteger(theDictionary, "/Height");
            long long const iBitsPerComponent = ReadInteger(theDictionary, "/BitsPerComponent");

            std::string const strColorSpace = Describe(theDictionary.getKey("/ColorSpace"));
            std::string const strFilter = Describe(theDictionary.getKey("/Filter"));
            std::string const strObject = theImage.getObjGen().unparse(',');

            bool const bImageMask = theDictionary.getKey("/ImageMask").isBool() &&
                                    theDictionary.getKey("/ImageMask").getBoolValue();
            bool const bMask = !theDictionary.getKey("/Mask").null();
            bool const bSoftMask = !theDictionary.getKey("/SMask").null();

            std::string const strObjectFilePart = Sanitize(strObject);
            std::string const strFileName =
               "image-" + strObjectFilePart + std::string(RawExtension(strFilter));
            std::filesystem::path const theImagePath = theOutputDirectory / strFileName;

            std::size_t uRawBytes = 0U;
            if(setExtractedObjects.insert(strObject).second) {
               uRawBytes = WriteRawStream(theImage, theImagePath);
               ++uUniqueImages;
               }
            else {
               std::error_code theError;
               uRawBytes = static_cast<std::size_t>(
                  std::filesystem::file_size(theImagePath, theError));
               if(theError) {
                  uRawBytes = 0U;
                  }
               }

            std::println(
               "IMAGE|page={}|name={}|object={}|width={}|height={}|bpc={}|color={}|filter={}|"
               "image-mask={}|mask={}|soft-mask={}|raw={}|file={}",
               uPage + 1U,
               strResourceName,
               strObject,
               iWidth,
               iHeight,
               iBitsPerComponent,
               strColorSpace,
               strFilter,
               bImageMask,
               bMask,
               bSoftMask,
               uRawBytes,
               strFileName);

            theManifest
               << (uPage + 1U) << '\t'
               << strResourceName << '\t'
               << strObject << '\t'
               << iWidth << '\t'
               << iHeight << '\t'
               << iBitsPerComponent << '\t'
               << strColorSpace << '\t'
               << strFilter << '\t'
               << bImageMask << '\t'
               << bMask << '\t'
               << bSoftMask << '\t'
               << uRawBytes << '\t'
               << strFileName << '\n';
            });
      }

   std::println(
      "RESULT|images={}|unique={}|output={}",
      uOccurrences,
      uUniqueImages,
      theOutputDirectory.string());

   return EXIT_SUCCESS;
   }

} // namespace

/**
\brief Program entry point for the PDF image extraction repro.
\details Requires an input PDF and an output directory.
\param iArgc Number of command-line arguments.
\param ppszArgv Command-line arguments.
\returns EXIT_SUCCESS on success, EXIT_FAILURE for processing errors, or 2 for invalid usage.
\throw Does not propagate exceptions.
*/
int main(int const iArgc, char const* const* const ppszArgv) {
   if(iArgc != 3) {
      std::println(stderr, "Usage: pdf-image-extraction <input.pdf> <output-directory>");
      return 2;
      }

   try {
      return Run(ppszArgv[1], std::filesystem::path(ppszArgv[2]));
      }
   catch(std::exception const& theException) {
      std::println(stderr, "ERROR|{}", theException.what());
      return EXIT_FAILURE;
      }
   }
