#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

#include <ZXing/CreateBarcode.h>
#include <ZXing/WriteBarcode.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ImageView.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/objdetect/barcode.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <format>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct Result {
   cv::Mat theImage;
   cv::Mat theAnnotated;
   std::vector<std::string> vecText;
   std::vector<std::string> vecTypes;
   bool bDetected {};
   bool bVerified {};
   double flMilliseconds {};
};

/**
\brief Converts a true ZXing/Zint generated symbol into an OpenCV image.
\details No barcode rendering is reimplemented; every source pixel comes from
   ZXing::WriteBarcodeToImage and the managed external Zint writer backend.
\param svText EAN-13 or QR payload.
\param bQR Select QR Code instead of EAN-13.
\returns CV_8UC3 source image.
\throw std::runtime_error On encoder failure.
*/
cv::Mat MakeSymbol(std::string_view const svText,bool const bQR) {
   ZXing::CreatorOptions const theOptions {
      bQR ? ZXing::BarcodeFormat::QRCode : ZXing::BarcodeFormat::EAN13
   };
   ZXing::Barcode const theBarcode=ZXing::CreateBarcodeFromText(svText,theOptions);
   if(!theBarcode.isValid())
      throw std::runtime_error("ZXing could not encode the selected payload");
   ZXing::WriterOptions theWriter;
   theWriter.scale(8);
   theWriter.addQuietZones(true);
   ZXing::Image const theBitmap=ZXing::WriteBarcodeToImage(theBarcode,theWriter);
   if(theBitmap.width() <= 0 || theBitmap.height() <= 0)
      throw std::runtime_error("ZXing writer returned an empty symbol");
   cv::Mat theImage(theBitmap.height()+120,
      std::max(900,theBitmap.width()+120),CV_8UC3,cv::Scalar(255,255,255));
   int const iLeft=(theImage.cols-theBitmap.width())/2;
   int const iTop=(theImage.rows-theBitmap.height())/2;
   for(int iY=0;iY<theBitmap.height();++iY)
      for(int iX=0;iX<theBitmap.width();++iX) {
         unsigned char const uValue=theBitmap.data(iX,iY)[0];
         theImage.at<cv::Vec3b>(iTop+iY,iLeft+iX)=cv::Vec3b{uValue,uValue,uValue};
      }
   return theImage;
}

/**
\brief Runs native OpenCV 5.1 barcode or QR detection on real image pixels.
\details The CV detector supplies all positions and decoded text. ZXing is
   used only to create a reproducible input fixture, never as a decode fallback.
\param theImage BGR image, either imported from disk or ZXing generated.
\param bQR Select cv::QRCodeDetector rather than cv::barcode::BarcodeDetector.
\param svExpected Optional exact expected payload for the round-trip smoke.
\returns Annotated input image and actual OpenCV detection evidence.
\throw cv::Exception On invalid computer-vision operations.
*/
Result Analyze(cv::Mat const& theImage,bool const bQR,
   std::string_view const svExpected={}) {
   if(theImage.empty()) throw std::runtime_error("Input image is empty");
   Result theResult;
   theResult.theImage=theImage.clone();
   theResult.theAnnotated=theImage.clone();
   auto const theStart=std::chrono::steady_clock::now();
   cv::Mat theCorners;
   if(bQR) {
      cv::QRCodeDetector theDetector;
      std::string const strText=theDetector.detectAndDecode(theImage,theCorners);
      if(!strText.empty()) {
         theResult.vecText.push_back(strText);
         theResult.vecTypes.push_back("QR Code");
      }
   }
   else {
      cv::barcode::BarcodeDetector theDetector;
      theResult.bDetected=theDetector.detectAndDecodeWithType(theImage,
         theResult.vecText,theResult.vecTypes,theCorners);
   }
   theResult.flMilliseconds=std::chrono::duration<double,std::milli>(
      std::chrono::steady_clock::now()-theStart).count();
   theResult.bDetected=theResult.bDetected || !theResult.vecText.empty();
   theResult.bVerified=std::ranges::any_of(theResult.vecText,
      [svExpected](std::string const& strValue) {
         return !svExpected.empty() && strValue == svExpected;
      });
   if(theCorners.total() >= 4) {
      cv::Mat thePoints;
      theCorners.convertTo(thePoints,CV_32F);
      cv::Mat theFlat=thePoints.reshape(2,static_cast<int>(thePoints.total()));
      for(int i=0;i+3<theFlat.rows;i+=4) {
         for(int j=0;j<4;++j) {
            cv::Point2f const a=theFlat.at<cv::Point2f>(i+j);
            cv::Point2f const b=theFlat.at<cv::Point2f>(i+(j+1)%4);
            cv::line(theResult.theAnnotated,a,b,cv::Scalar(27,215,68),3,cv::LINE_AA);
         }
      }
   }
   return theResult;
}

/** \brief Draws a downsampled OpenCV result as native ImGui geometry. */
void DrawImage(cv::Mat const& theImage,ImVec2 const aSize) {
   ImVec2 const aOrigin=ImGui::GetCursorScreenPos();
   ImDrawList* const pDraw=ImGui::GetWindowDrawList();
   pDraw->AddRectFilled(aOrigin,{aOrigin.x+aSize.x,aOrigin.y+aSize.y},
      IM_COL32(246,248,251,255),12.0F);
   if(!theImage.empty()) {
      int const iW=std::clamp(static_cast<int>(aSize.x/3),1,520);
      int const iH=std::clamp(static_cast<int>(aSize.y/3),1,210);
      cv::Mat theSmall;
      cv::resize(theImage,theSmall,cv::Size{iW,iH},0,0,cv::INTER_AREA);
      float const flS=std::min(aSize.x/iW,aSize.y/iH);
      float const flX=aOrigin.x+(aSize.x-iW*flS)*0.5F;
      float const flY=aOrigin.y+(aSize.y-iH*flS)*0.5F;
      for(int iY=0;iY<iH;++iY)
         for(int iX=0;iX<iW;++iX) {
            cv::Vec3b const aColor=theSmall.at<cv::Vec3b>(iY,iX);
            ImU32 const uColor=IM_COL32(aColor[2],aColor[1],aColor[0],255);
            pDraw->AddRectFilled({flX+iX*flS,flY+iY*flS},
               {flX+(iX+1)*flS+0.3F,flY+(iY+1)*flS+0.3F},uColor);
         }
   }
   ImGui::Dummy(aSize);
}

bool Verify() {
   struct Case { char const* szLabel; char const* szPayload; bool bQR; };
   constexpr std::array<Case,2> arrCases {{
      {"EAN-13","5901234123457",false},
      {"QR Code","BUILDENGINE-OPENCV-5.1",true}
   }};
   bool bPass=true;
   for(Case const& theCase:arrCases) {
      cv::Mat const theImage=MakeSymbol(theCase.szPayload,theCase.bQR);
      Result const theResult=Analyze(theImage,theCase.bQR,theCase.szPayload);
      bool const bCase=theResult.bVerified;
      std::println("SHOWCASE|CHECK|opencv-{}|{}|detected={} decoded={} elapsed-ms={:.2f}",
         theCase.szLabel,bCase?"PASS":"FAIL",theResult.bDetected,
         theResult.vecText.size(),theResult.flMilliseconds);
      bPass=bPass && bCase;
   }
   std::println("SHOWCASE|RESULT|{}|Native OpenCV barcode and QR detectors with ZXing/Zint fixtures",
      bPass?"PASS":"FAIL");
   return bPass;
}

void Interactive() {
   NativeShowcase aApp{"BuildEngine | OPENCV BARCODE VISION",1460,880};
   bool bQR=false;
   std::array<char,512> arrPath {};
   std::array<char,256> arrPayload {};
   std::string strText="5901234123457";
   std::copy(strText.begin(),strText.end(),arrPayload.begin());
   Result theResult;
   std::string strError;
   auto const Run=[&](bool const bFile) {
      try {
         cv::Mat const theImage=bFile
            ? cv::imread(arrPath.data(),cv::IMREAD_COLOR)
            : MakeSymbol(arrPayload.data(),bQR);
         theResult=Analyze(theImage,bQR,bFile?"":arrPayload.data());
         strError.clear();
      }
      catch(std::exception const& theError) {
         theResult={};
         strError=theError.what();
      }
   };
   Run(false);
   while(aApp.Running()) {
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("OPENCV BARCODE VISION",nullptr,
         ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("OPENCV / BARCODE VISION",
         "OpenCV 5.1 native detection + ZXing/Zint symbols + ImGui/SDL2");
      if(ImGui::RadioButton("EAN-13 / UPC barcode",!bQR)) {
         bQR=false; arrPayload.fill(0);
         strText="5901234123457";
         std::copy(strText.begin(),strText.end(),arrPayload.begin());
         Run(false);
      }
      ImGui::SameLine();
      if(ImGui::RadioButton("QR Code",bQR)) {
         bQR=true; arrPayload.fill(0);
         strText="BUILDENGINE-OPENCV-5.1";
         std::copy(strText.begin(),strText.end(),arrPayload.begin());
         Run(false);
      }
      ImGui::SetNextItemWidth(560);
      ImGui::InputText("Payload",arrPayload.data(),arrPayload.size());
      if(ImGui::Button("GENERATE + DETECT")) Run(false);
      ImGui::SeparatorText("Import an existing symbol image");
      ImGui::SetNextItemWidth(720);
      ImGui::InputText("Image path (PNG/JPEG)",arrPath.data(),arrPath.size());
      ImGui::SameLine();
      if(ImGui::Button("DETECT FILE")) Run(true);
      if(!strError.empty())
         ImGui::TextColored({1,.3F,.4F,1},"Error: %s",strError.c_str());
      else {
         ImGui::Text("OpenCV detected: %s | Results: %d | Processing: %.2f ms",
            theResult.bDetected?"yes":"no",
            static_cast<int>(theResult.vecText.size()),theResult.flMilliseconds);
         if(theResult.bVerified)
            ImGui::TextColored({.3F,.96F,.65F,1},"ROUND-TRIP: PASS");
         for(std::size_t u=0;u<theResult.vecText.size();++u)
            ImGui::Text("[%s] %s",
               u<theResult.vecTypes.size()?theResult.vecTypes[u].c_str():"code",
               theResult.vecText[u].c_str());
      }
      DrawImage(theResult.theAnnotated,ImVec2{
         std::max(360.0F,ImGui::GetContentRegionAvail().x),
         std::max(240.0F,ImGui::GetContentRegionAvail().y-12)});
      ImGui::End();
      aApp.EndFrame();
   }
}
}
int main(int const iArgc,char const* const* const pszArgv) {
   try {
      if(iArgc>1 && std::string_view{pszArgv[1]}=="--verify")
         return Verify()?0:1;
      Interactive();
      return 0;
   }
   catch(std::exception const& theError) {
      std::println(stderr,"SHOWCASE|ERROR|{}",theError.what());
      return 2;
   }
}
