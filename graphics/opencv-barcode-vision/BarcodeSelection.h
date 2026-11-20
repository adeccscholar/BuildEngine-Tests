#pragma once

#include <ZXing/BarcodeFormat.h>
#include <ZXing/Barcode.h>
#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ReaderOptions.h>

#include <opencv2/core.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/**
\brief A codec selection backed by the actual ZXing-C++ readable format set.
\details The first option is automatic detection; remaining options are
   enumerated by the installed ZXing library. No unsupported formats are
   invented by the application.
*/
struct BarcodeSelection {
   std::vector<ZXing::BarcodeFormat> vecFormats;

   BarcodeSelection() {
      ZXing::BarcodeFormats const theReadable =
         ZXing::BarcodeFormats::list(ZXing::BarcodeFormat::AllReadable);
      vecFormats.assign(theReadable.begin(),theReadable.end());
   }

   /**
   \brief Returns a format selected in the GUI.
   \details Index zero represents automatic mode.
   \param iSelection Zero for automatic, otherwise a one-based format index.
   \returns The selected ZXing format, or All for automatic scanning.
   \throw std::out_of_range For an invalid index.
   */
   ZXing::BarcodeFormat GetFormat(int const iSelection) const {
      if(iSelection==0) return ZXing::BarcodeFormat::All;
      return vecFormats.at(static_cast<std::size_t>(iSelection-1));
   }

   /**
   \brief Human-readable selection caption.
   \details Names come directly from the installed ZXing format database.
   \param iSelection Zero for automatic or one-based format index.
   \returns Display name of the selected format.
   \throw std::out_of_range For an invalid index.
   */
   std::string Label(int const iSelection) const {
      return iSelection==0 ? "Automatic (OpenCV + ZXing)" :
         std::string(ZXing::Name(GetFormat(iSelection)));
   }
};

/**
\brief Validates the international EAN-13 check digit and ISBN prefix.
\details Printed ISBN-13 book symbols use the 978/979 EAN prefixes.
\param svDigits Exactly 13 ASCII decimal digits.
\returns True only for a valid Bookland EAN-13 ISBN.
\throw Nothing intentionally.
*/
inline bool IsIsbn13(std::string_view const svDigits) {
   if(svDigits.size()!=13 ||
      (svDigits.substr(0,3)!="978" && svDigits.substr(0,3)!="979"))
      return false;
   int iSum=0;
   for(std::size_t u=0;u<13;++u) {
      if(svDigits[u]<'0' || svDigits[u]>'9') return false;
      iSum+=(svDigits[u]-'0')*(u%2==0?1:3);
   }
   return iSum%10==0;
}

/**
\brief Converts a 978-prefixed ISBN-13 to an ISBN-10.
\details The 979 prefix cannot generally be represented as ISBN-10.
\param svDigits A complete ISBN-13 number.
\returns Valid ISBN-10 if conversion is mathematically possible.
\throw Nothing intentionally.
*/
inline std::optional<std::string> ToIsbn10(std::string_view const svDigits) {
   if(!IsIsbn13(svDigits) || svDigits.substr(0,3)!="978")
      return std::nullopt;
   std::string strISBN {svDigits.substr(3,9)};
   int iChecksum=0;
   for(int i=0;i<9;++i)
      iChecksum+=(strISBN[static_cast<std::size_t>(i)]-'0')*(10-i);
   int const iRemainder=(11-iChecksum%11)%11;
   strISBN+=iRemainder==10?'X':static_cast<char>('0'+iRemainder);
   return strISBN;
}

/**
\brief Reads native ZXing barcodes using a concrete format filter.
\details Input points and text are sourced solely from the installed ZXing
   decoding engine; no application-local symbology decoder is provided.
\param theImage BGR, BGRA or grayscale OpenCV frame.
\param eFormat ZXing format or All for automatic detection.
\returns Verified native ZXing results.
\throw std::runtime_error For unsupported pixel layouts.
*/
inline ZXing::Barcodes DecodeZXing(cv::Mat const& theImage,
   ZXing::BarcodeFormat const eFormat) {
   if(theImage.empty()) return {};
   ZXing::ImageFormat eImage=ZXing::ImageFormat::None;
   if(theImage.type()==CV_8UC1) eImage=ZXing::ImageFormat::Lum;
   else if(theImage.type()==CV_8UC3) eImage=ZXing::ImageFormat::BGR;
   else if(theImage.type()==CV_8UC4) eImage=ZXing::ImageFormat::BGRA;
   else throw std::runtime_error("Unsupported OpenCV pixel type for ZXing");
   ZXing::ImageView const theView {theImage.data,theImage.cols,theImage.rows,
      eImage,static_cast<int>(theImage.step)};
   ZXing::ReaderOptions theOptions;
   theOptions.setTryHarder(true);
   theOptions.setTryRotate(true);
   theOptions.setTryInvert(true);
   if(eFormat!=ZXing::BarcodeFormat::All)
      theOptions.setFormats(ZXing::BarcodeFormats{eFormat});
   return ZXing::ReadBarcodes(theView,theOptions);
}
