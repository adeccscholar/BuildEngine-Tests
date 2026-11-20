#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"
#include "CameraDevices.h"
#include "LiveBarcodeTracker.h"
#include "BarcodeSelection.h"
#include "BookLookup.h"

#include <ZXing/CreateBarcode.h>
#include <ZXing/WriteBarcode.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ImageView.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/objdetect/barcode.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <cstdint>
#include <memory>
#include <format>
#include <future>
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
   std::vector<std::array<cv::Point2f,4>> vecDetectedCorners;
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
         std::array<cv::Point2f,4> arrCorners {};
         for(int j=0;j<4;++j)
            arrCorners[static_cast<std::size_t>(j)]=theFlat.at<cv::Point2f>(i+j);
         theResult.vecDetectedCorners.push_back(arrCorners);
         for(int j=0;j<4;++j)
            cv::line(theResult.theAnnotated,arrCorners[static_cast<std::size_t>(j)],
               arrCorners[static_cast<std::size_t>((j+1)%4)],
               cv::Scalar(27,215,68),3,cv::LINE_AA);
      }
   }
   return theResult;
}

/**
\brief Decodes a selected symbology with managed OpenCV and ZXing engines.
\details Automatic mode tries native OpenCV QR and linear barcode detection
   before using the broader readable ZXing format set. Manually selected
   modes are passed to ZXing directly; they are not silently narrowed to
   the two formats supported by the current OpenCV detector.
\param theImage Source camera frame or imported BGR image.
\param iSelection Zero for automatic, otherwise one-based ZXing catalog index.
\param theSelection Catalog generated from the actual ZXing package.
\returns Real decoded text, format names and detector quadrilateral positions.
\throw std::runtime_error For incompatible input pixel layout.
*/
Result AnalyzeSelected(cv::Mat const& theImage,int const iSelection,
   BarcodeSelection const& theSelection) {
   if(iSelection==0) {
      Result theQR=Analyze(theImage,true);
      if(theQR.bDetected && !theQR.vecText.empty())
         return theQR;
      Result theLinear=Analyze(theImage,false);
      if(theLinear.bDetected && !theLinear.vecText.empty()) {
         for(std::size_t u=0;u<theLinear.vecText.size();++u) {
            if(IsIsbn13(theLinear.vecText[u]) && u<theLinear.vecTypes.size()) {
               theLinear.vecTypes[u]="ISBN-13 (EAN-13)";
            }
         }
         return theLinear;
      }
   }
   auto const theStart=std::chrono::steady_clock::now();
   ZXing::Barcodes const vecCodes=DecodeZXing(theImage,
      theSelection.GetFormat(iSelection));
   Result theResult;
   theResult.theImage=theImage.clone();
   theResult.theAnnotated=theImage.clone();
   for(ZXing::Barcode const& theCode:vecCodes) {
      if(!theCode.isValid()) continue;
      std::string const strRaw=theCode.text();
      if(strRaw.empty()) continue;
      std::string strType=std::string(ZXing::Name(theCode.format()));
      if(IsIsbn13(strRaw))
         strType="ISBN-13 ("+strType+")";
      theResult.vecText.push_back(strRaw);
      theResult.vecTypes.push_back(strType);
      std::array<cv::Point2f,4> arrCorners {};
      for(std::size_t u=0;u<4;++u) {
         auto const& thePoint=theCode.position()[u];
         arrCorners[u]={static_cast<float>(thePoint.x),
            static_cast<float>(thePoint.y)};
      }
      theResult.vecDetectedCorners.push_back(arrCorners);
      for(int i=0;i<4;++i)
         cv::line(theResult.theAnnotated,
            arrCorners[static_cast<std::size_t>(i)],
            arrCorners[static_cast<std::size_t>((i+1)%4)],
            cv::Scalar{27,215,68},3,cv::LINE_AA);
   }
   theResult.bDetected=!theResult.vecText.empty();
   theResult.flMilliseconds=std::chrono::duration<double,std::milli>(
      std::chrono::steady_clock::now()-theStart).count();
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

/**
\brief Owns a native SDL streaming texture for continuous OpenCV image display.
\details The image is transferred once per updated camera frame, not converted
   into thousands of ImGui drawing primitives. The SDL renderer owns the GPU
   resource, which is released before its NativeShowcase parent shuts down.
*/
class CameraViewport final {
   SDL_Renderer* pRenderer {};
   SDL_Texture* pTexture {};
   int iWidth {};
   int iHeight {};

public:
   explicit CameraViewport(SDL_Window* const pWindow)
      : pRenderer(SDL_GetRenderer(pWindow)) {
      if(!pRenderer) throw std::runtime_error("SDL renderer unavailable");
   }
   CameraViewport(CameraViewport const&)=delete;
   CameraViewport& operator=(CameraViewport const&)=delete;
   ~CameraViewport() { if(pTexture) SDL_DestroyTexture(pTexture); }

   /**
   \brief Updates the live image from the actual annotated OpenCV pixels.
   \details Uses an SDL2 BGRA streaming texture and OpenCV conversion.
   \param theImage BGR camera image with optional detector annotations.
   \returns Nothing.
   \throw std::runtime_error On SDL texture allocation or transfer failure.
   */
   void SetImage(cv::Mat const& theImage) {
      if(theImage.empty()) return;
      if(theImage.cols!=iWidth || theImage.rows!=iHeight) {
         if(pTexture) SDL_DestroyTexture(pTexture);
         pTexture=SDL_CreateTexture(pRenderer,SDL_PIXELFORMAT_BGRA32,
            SDL_TEXTUREACCESS_STREAMING,theImage.cols,theImage.rows);
         if(!pTexture) throw std::runtime_error(SDL_GetError());
         iWidth=theImage.cols;
         iHeight=theImage.rows;
      }
      cv::Mat theBGRA;
      cv::cvtColor(theImage,theBGRA,cv::COLOR_BGR2BGRA);
      if(SDL_UpdateTexture(pTexture,nullptr,theBGRA.data,
         static_cast<int>(theBGRA.step))!=0)
         throw std::runtime_error(SDL_GetError());
   }

   /**
   \brief Displays the current frame with its native aspect ratio.
   \details Calls SDL2 texture-backed ImGui rendering, preserving all pixels.
   \param flAvailableWidth Available viewport width.
   \param flAvailableHeight Available viewport height.
   \returns Nothing.
   \throw Nothing intentionally.
   */
   void Draw(float const flAvailableWidth,float const flAvailableHeight) const {
      if(!pTexture || iWidth<=0 || iHeight<=0) {
         ImGui::TextDisabled("No camera frames received.");
         return;
      }
      float const flScale=std::min(flAvailableWidth/static_cast<float>(iWidth),
         flAvailableHeight/static_cast<float>(iHeight));
      ImVec2 const aSize {
         std::max(1.0F,flScale*iWidth),std::max(1.0F,flScale*iHeight)
      };
      ImGui::Image(static_cast<ImTextureID>(
         reinterpret_cast<std::intptr_t>(pTexture)),aSize);
   }
};

/**
\brief Holds an explicitly selected OpenCV DirectShow capture.
\details Neither camera discovery nor the headless smoke opens hardware.
   The selected camera is released on every disconnect or selection change.
*/
class CameraCapture final {
   cv::VideoCapture theCapture;
   int iIndex {-1};

public:
   [[nodiscard]] bool Connected() const { return theCapture.isOpened(); }
   [[nodiscard]] int Index() const { return iIndex; }

   /**
   \brief Opens exactly one selected DirectShow camera.
   \details Uses the existing OpenCV CAP_DSHOW backend; does not enumerate by
      repeatedly opening camera indices, so inactive cameras remain harmless.
   \param iDeviceIndex Current DirectShow enumeration index.
   \returns True when the driver successfully opens the selected source.
   \throw cv::Exception If OpenCV reports a capture failure.
   */
   bool Connect(int const iDeviceIndex) {
      Disconnect();
      if(iDeviceIndex<0) return false;
      if(!theCapture.open(iDeviceIndex,cv::CAP_DSHOW)) return false;
      iIndex=iDeviceIndex;
      return true;
   }
   void Disconnect() {
      theCapture.release();
      iIndex=-1;
   }
   [[nodiscard]] bool Read(cv::Mat& theFrame) {
      return Connected() && theCapture.read(theFrame) && !theFrame.empty();
   }
};

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
   bool const bLookupISBN=IsIsbn13("9783696710095") &&
      ToIsbn10("9783696710095")==std::optional<std::string>{"3696710091"};
   std::println("SHOWCASE|CHECK|isbn-requested-9783696710095|{}|isbn10={}",
      bLookupISBN?"PASS":"FAIL",
      ToIsbn10("9783696710095").value_or("n/a"));
   bPass=bPass&&bLookupISBN;
   // Catalog entries and ISBN metadata are independent of webcam availability.
   BarcodeSelection const theSelection;
   bool const bHasISBN=std::ranges::find(theSelection.vecFormats,
      ZXing::BarcodeFormat::ISBN)!=theSelection.vecFormats.end();
   bool const bHasQR=std::ranges::find(theSelection.vecFormats,
      ZXing::BarcodeFormat::QRCode)!=theSelection.vecFormats.end();
   bool const bISBN=IsIsbn13("9780306406157") &&
      !IsIsbn13("9780306406158") &&
      !IsIsbn13("4006381333931") &&
      ToIsbn10("9780306406157")==std::optional<std::string>{"0306406152"} &&
      !ToIsbn10("9791234567896");
   cv::Mat const theISBNImage=MakeSymbol("9780306406157",false);
   Result const theISBNResult=AnalyzeSelected(theISBNImage,0,theSelection);
   bool const bISBNDetected=std::ranges::find(theISBNResult.vecText,
      std::string("9780306406157"))!=theISBNResult.vecText.end();
   cv::Mat const theSelectedQR=MakeSymbol("BUILDENGINE-OPENCV-5.1",true);
   int const iQRCodeOption=static_cast<int>(
      std::ranges::find(theSelection.vecFormats,ZXing::BarcodeFormat::QRCode)-
      theSelection.vecFormats.begin())+1;
   Result const theSelectedResult=bHasQR ?
      AnalyzeSelected(theSelectedQR,iQRCodeOption,theSelection):Result{};
   bool const bManualQR=std::ranges::find(theSelectedResult.vecText,
      std::string("BUILDENGINE-OPENCV-5.1"))!=theSelectedResult.vecText.end();
   std::println("SHOWCASE|CHECK|barcode-formats|{}|readable={} isbn={} qrcode={}",
      bHasISBN&&bHasQR?"PASS":"FAIL",theSelection.vecFormats.size(),bHasISBN,bHasQR);
   std::println("SHOWCASE|CHECK|isbn-13|{}|ean978={} isbn10={} checksum={}",
      bISBN&&bISBNDetected?"PASS":"FAIL",bISBNDetected,ToIsbn10("9780306406157").value_or("n/a"),bISBN);
   std::println("SHOWCASE|CHECK|barcode-manual-qr|{}|decoded={}",
      bManualQR?"PASS":"FAIL",bManualQR);
   bPass=bPass&&bHasISBN&&bHasQR&&bISBN&&bISBNDetected&&bManualQR;
   // A confirmed QR code must remain locked on unchanged live frames.
   // No decoder invocation is necessary while the tracker reports presence.
   cv::Mat const theFixture=MakeSymbol("BUILDENGINE-OPENCV-5.1",true);
   Result const theQR=Analyze(theFixture,true,"BUILDENGINE-OPENCV-5.1");
   LiveBarcodeTracker theTracker;
   bool bAcquired=theQR.bVerified &&
      !theQR.vecDetectedCorners.empty() &&
      theTracker.Acquire(theFixture,theQR.vecDetectedCorners.front());
   bool bStable=bAcquired;
   if(bStable) {
      for(int i=0;i<12;++i)
         bStable=bStable && theTracker.Update(theFixture);
   }
   cv::Mat const theAbsent(theFixture.size(),theFixture.type(),cv::Scalar(255,255,255));
   bool bReleased=bStable;
   if(bReleased) {
      for(int i=0;i<6;++i)
         theTracker.Update(theAbsent);
      bReleased=!theTracker.Active();
   }
   std::println("SHOWCASE|CHECK|live-code-lock|{}|acquired={} still-tracked={}",
      bStable?"PASS":"FAIL",bAcquired,theTracker.Active());
   std::println("SHOWCASE|CHECK|live-code-rearm|{}|removed symbol frees new decode",
      bReleased?"PASS":"FAIL");
   bPass=bPass && bStable && bReleased;
   std::println("SHOWCASE|RESULT|{}|Native OpenCV barcode and QR detectors with ZXing/Zint fixtures",
      bPass?"PASS":"FAIL");
   return bPass;
}

void Interactive() {
   NativeShowcase aApp{"BuildEngine | OPENCV BARCODE VISION",1460,880};
   CameraViewport theViewport{aApp.Window()};
   CameraViewport theBookCover{aApp.Window()};
   CameraCapture theCapture;
   LiveBarcodeTracker theTracker;
   BarcodeSelection const theSelection;
   int iSelectedFormat=0;
   std::vector<CameraDevice> vecDevices;
   std::string strDiscoveryError;
   auto const RefreshCameras=[&]() {
      try {
         vecDevices=EnumerateCameras();
         strDiscoveryError.clear();
      }
      catch(std::exception const& theError) {
         vecDevices.clear();
         strDiscoveryError=theError.what();
      }
   };
   RefreshCameras();
   int iSelected=vecDevices.empty()?-1:0;
   int iManualIndex=0;
   bool bQR=false;
   bool bLive=false;
   bool bScan=true;
   std::array<char,512> arrPath {};
   std::array<char,256> arrPayload {};
   std::string strText="5901234123457";
   std::copy(strText.begin(),strText.end(),arrPayload.begin());
   Result theResult;
   Result theLastConfirmed;
   std::uint64_t uDecodeCount {};
   std::array<char,40> arrLookupISBN {};
   BookMetadata theBook;
   std::future<BookMetadata> theBookRequest;
   bool bBookPending=false;
   bool bAutoBookLookup=true;
   std::string strError;
   std::string strCaptureError;
   std::uint64_t uLastCapture {};
   std::uint64_t uLastScan {};
   std::uint64_t uFrames {};
   int iDroppedFrames {};
   auto const Run=[&](bool const bFile) {
      try {
         if(bLive) {
            theCapture.Disconnect();
            bLive=false;
         }
         theTracker.Reset();
         cv::Mat const theImage=bFile
            ? cv::imread(arrPath.data(),cv::IMREAD_COLOR)
            : MakeSymbol(arrPayload.data(),bQR);
         theResult=AnalyzeSelected(theImage,iSelectedFormat,theSelection);
         if(!bFile)
            theResult.bVerified=std::ranges::any_of(theResult.vecText,
               [&](std::string const& strValue) { return strValue==arrPayload.data(); });
         theViewport.SetImage(theResult.theAnnotated);
         strError.clear();
      }
      catch(std::exception const& theError) {
         theResult={};
         strError=theError.what();
      }
   };
   Run(false);
   auto const BeginBookLookup=[&](std::string const& strISBN) {
      if(bBookPending) return;
      theBook={};
      theBook.strISBN=strISBN;
      theBook.strStatus="Searching public catalogs...";
      bBookPending=true;
      theBookRequest=std::async(std::launch::async,
         [strISBN]() { return BookLookup::Lookup(strISBN); });
   };
   auto const SetLookupISBN=[&](std::string_view const svText) {
      std::string strDigits;
      for(char const c:svText)
         if(c>='0' && c<='9') strDigits.push_back(c);
      if(!IsIsbn13(strDigits) || strDigits==arrLookupISBN.data()) return;
      arrLookupISBN.fill(0);
      std::copy(strDigits.begin(),strDigits.end(),arrLookupISBN.begin());
      theBook={};
      theBook.strStatus="ISBN recognized. Public catalog query available.";
      if(bAutoBookLookup)
         BeginBookLookup(strDigits);
   };
   while(aApp.Running()) {
      if(bBookPending && theBookRequest.wait_for(std::chrono::milliseconds{0})==
         std::future_status::ready) {
         try {
            theBook=theBookRequest.get();
            if(!theBook.theCover.empty()) theBookCover.SetImage(theBook.theCover);
         }
         catch(std::exception const& theError) {
            theBook.strStatus=theError.what();
         }
         bBookPending=false;
      }
      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("OPENCV BARCODE VISION",nullptr,
         ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("OPENCV / BARCODE VISION",
         "OpenCV 5.1 DirectShow camera + native barcode/QR detection + ZXing/Zint fixtures");
      ImGui::SeparatorText("1. Native camera selection");
      if(ImGui::Button("REFRESH REGISTERED CAMERAS")) {
         theTracker.Reset();
         theCapture.Disconnect();
         bLive=false;
         std::string strPreviousPath;
         if(iSelected>=0 && iSelected<static_cast<int>(vecDevices.size()))
            strPreviousPath=vecDevices[static_cast<std::size_t>(iSelected)].strDevicePath;
         RefreshCameras();
         iSelected=vecDevices.empty()?-1:0;
         if(!strPreviousPath.empty()) {
            for(std::size_t u=0;u<vecDevices.size();++u) {
               if(vecDevices[u].strDevicePath==strPreviousPath) {
                  iSelected=static_cast<int>(u);
                  break;
               }
            }
         }
         strCaptureError.clear();
      }
      ImGui::SameLine();
      ImGui::TextDisabled("Registered DirectShow devices: %d",
         static_cast<int>(vecDevices.size()));
      if(!strDiscoveryError.empty())
         ImGui::TextColored({1,.35F,.42F,1},"Enumeration: %s",strDiscoveryError.c_str());

      std::string strSelected="No enumerated camera";
      if(iSelected>=0 && iSelected<static_cast<int>(vecDevices.size())) {
         CameraDevice const& aCamera=vecDevices[static_cast<std::size_t>(iSelected)];
         strSelected=std::format("[{}] {}",aCamera.iIndex,aCamera.strName);
      }
      ImGui::SetNextItemWidth(std::min(920.0F,ImGui::GetContentRegionAvail().x));
      if(ImGui::BeginCombo("DirectShow device",strSelected.c_str())) {
         for(std::size_t u=0;u<vecDevices.size();++u) {
            CameraDevice const& aCamera=vecDevices[u];
            std::string const strLabel=std::format("[{}] {}##camera{}",
               aCamera.iIndex,aCamera.strName,u);
            if(ImGui::Selectable(strLabel.c_str(),iSelected==static_cast<int>(u))) {
               theTracker.Reset();
               theCapture.Disconnect();
               bLive=false;
               iSelected=static_cast<int>(u);
               strCaptureError.clear();
            }
         }
         ImGui::EndCombo();
      }
      ImGui::TextDisabled(
         "Enumeration does not open devices; unavailable cameras may remain listed.");
      ImGui::SetNextItemWidth(125);
      ImGui::InputInt("DirectShow index (manual)",&iManualIndex);
      iManualIndex=std::clamp(iManualIndex,0,31);
      if(ImGui::Button("CONNECT SELECTED")) {
         int const iIndex=iSelected>=0 &&
            iSelected<static_cast<int>(vecDevices.size())
            ? vecDevices[static_cast<std::size_t>(iSelected)].iIndex:-1;
         try {
            if(theCapture.Connect(iIndex)) {
               bLive=true;
               uLastCapture=uLastScan=0;
               theTracker.Reset();
               theResult={};
               uFrames=0;
               iDroppedFrames=0;
               strCaptureError.clear();
               strError.clear();
            }
            else {
               bLive=false;
               strCaptureError="Selected DirectShow camera could not be opened.";
            }
         }
         catch(std::exception const& theError) {
            bLive=false;
            strCaptureError=theError.what();
         }
      }
      ImGui::SameLine();
      if(ImGui::Button("CONNECT BY INDEX")) {
         try {
            if(theCapture.Connect(iManualIndex)) {
               bLive=true;
               uLastCapture=uLastScan=0;
               theTracker.Reset();
               theResult={};
               uFrames=0;
               iDroppedFrames=0;
               strCaptureError.clear();
               strError.clear();
            }
            else {
               bLive=false;
               strCaptureError=std::format("DirectShow index {} is unavailable.",iManualIndex);
            }
         }
         catch(std::exception const& theError) {
            bLive=false;
            strCaptureError=theError.what();
         }
      }
      ImGui::SameLine();
      if(ImGui::Button("DISCONNECT")) {
         theTracker.Reset();
         theCapture.Disconnect();
         bLive=false;
      }
      ImGui::SameLine();
      if(ImGui::Checkbox("Scan live frames",&bScan)) {
         theTracker.Reset();
         theResult={};
      }
      ImGui::SameLine();
      if(ImGui::Button("RESET / SCAN NEXT")) {
         theTracker.Reset();
         theResult={};
         uLastScan=0;
      }
      ImGui::Text("Capture status: %s | index: %d | frames: %llu | read failures: %d",
         bLive?"CONNECTED":"DISCONNECTED",theCapture.Index(),
         static_cast<unsigned long long>(uFrames),iDroppedFrames);
      if(!strCaptureError.empty())
         ImGui::TextColored({1,.4F,.4F,1},"Capture: %s",strCaptureError.c_str());

      ImGui::SeparatorText("2. Recognition mode");
      if(ImGui::RadioButton("EAN-13 / UPC barcode",!bQR)) {
         bQR=false; arrPayload.fill(0);
         strText="5901234123457";
         std::copy(strText.begin(),strText.end(),arrPayload.begin());
         theTracker.Reset();
         theResult={};
         if(!bLive) Run(false);
      }
      ImGui::SameLine();
      if(ImGui::RadioButton("QR Code",bQR)) {
         bQR=true; arrPayload.fill(0);
         strText="BUILDENGINE-OPENCV-5.1";
         std::copy(strText.begin(),strText.end(),arrPayload.begin());
         theTracker.Reset();
         theResult={};
         if(!bLive) Run(false);
      }
      ImGui::SetNextItemWidth(560);
      std::string const strFormatLabel=theSelection.Label(iSelectedFormat);
      if(ImGui::BeginCombo("Detection format",strFormatLabel.c_str())) {
         if(ImGui::Selectable("Automatic (OpenCV + ZXing)",iSelectedFormat==0)) {
            iSelectedFormat=0;
            theTracker.Reset();
            theResult={};
            uLastScan=0;
         }
         for(std::size_t u=0;u<theSelection.vecFormats.size();++u) {
            int const iOption=static_cast<int>(u)+1;
            ZXing::BarcodeFormat const eFormat=theSelection.vecFormats[u];
            std::string const strLabel=std::string(ZXing::Name(eFormat))+
               "##format"+std::to_string(u);
            if(ImGui::Selectable(strLabel.c_str(),iSelectedFormat==iOption)) {
               iSelectedFormat=iOption;
               theTracker.Reset();
               theResult={};
               uLastScan=0;
            }
         }
         ImGui::EndCombo();
      }
      ImGui::TextDisabled("All readable formats come from installed ZXing-C++ 3.1.1; OpenCV handles QR and EAN/UPC in automatic mode.");
      ImGui::SetNextItemWidth(560);
      ImGui::InputText("Payload",arrPayload.data(),arrPayload.size());
      if(ImGui::Button("GENERATE + DETECT")) {
         Run(false);
         for(std::string const& strDecoded:theResult.vecText)
            SetLookupISBN(strDecoded);
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(570);
      ImGui::InputText("Image path",arrPath.data(),arrPath.size());
      ImGui::SameLine();
      if(ImGui::Button("DETECT FILE")) {
         Run(true);
         for(std::string const& strDecoded:theResult.vecText)
            SetLookupISBN(strDecoded);
      }

      if(bLive) {
         std::uint64_t const uNow=SDL_GetTicks64();
         if(uNow-uLastCapture>=80) {
            uLastCapture=uNow;
            try {
               cv::Mat theFrame;
               if(theCapture.Read(theFrame)) {
                  ++uFrames;
                  // A confirmed barcode is tracked visually, without calling
                  // the barcode or QR decoder again while the symbol persists.
                  if(theTracker.Active() && !theTracker.Update(theFrame)) {
                     theResult={};
                     uLastScan=0;
                  }
                  if(bScan && !theTracker.Active() && uNow-uLastScan>=180) {
                     uLastScan=uNow;
                     Result theScan=AnalyzeSelected(theFrame,iSelectedFormat,theSelection);
                     if(theScan.bDetected && !theScan.vecText.empty() &&
                        !theScan.vecDetectedCorners.empty() &&
                        theTracker.Acquire(theFrame,theScan.vecDetectedCorners.front())) {
                        theResult=std::move(theScan);
                        theLastConfirmed=theResult;
                        for(std::string const& strDecoded:theResult.vecText)
                           SetLookupISBN(strDecoded);
                        ++uDecodeCount;
                     }
                     else {
                        theResult={};
                     }
                  }
                  cv::Mat theLive=theFrame.clone();
                  if(theTracker.Active()) {
                     cv::Rect const aBox=theTracker.Box();
                     cv::rectangle(theLive,aBox,cv::Scalar{28,220,65},3,cv::LINE_AA);
                  }
                  theViewport.SetImage(theLive);
                  strCaptureError.clear();
               }
               else {
                  ++iDroppedFrames;
                  if(iDroppedFrames>=10) {
                     theCapture.Disconnect();
                     bLive=false;
                     strCaptureError="Camera stopped returning frames; reconnect or select another device.";
                  }
               }
            }
            catch(std::exception const& theError) {
               theCapture.Disconnect();
               bLive=false;
               strCaptureError=theError.what();
            }
         }
      }

      ImGui::SeparatorText("3. Native OpenCV output");
      if(!strError.empty())
         ImGui::TextColored({1,.3F,.4F,1},"Error: %s",strError.c_str());
      else {
         ImGui::Text("Detected: %s | Decoded: %d | Analysis: %.2f ms",
            theResult.bDetected?"yes":"no",
            static_cast<int>(theResult.vecText.size()),theResult.flMilliseconds);
         if(bLive && theTracker.Active())
            ImGui::TextColored({.35F,.95F,.70F,1},
               "LOCKED: symbol still present - no repeat decoding");
         ImGui::Text("Confirmed live decodes: %llu",
            static_cast<unsigned long long>(uDecodeCount));
         if(!bLive && theResult.bVerified)
            ImGui::TextColored({.3F,.96F,.65F,1},"ROUND-TRIP: PASS");
         for(std::size_t u=0;u<theResult.vecText.size();++u) {
            if(std::optional<std::string> const strISBN10=ToIsbn10(theResult.vecText[u]))
               ImGui::Text("ISBN-10: %s",strISBN10->c_str());
            ImGui::Text("[%s] %s",
               u<theResult.vecTypes.size()?theResult.vecTypes[u].c_str():"code",
               theResult.vecText[u].c_str());
         }
      }
      ImGui::SeparatorText("4. ISBN book metadata (public REST)");
      ImGui::TextWrapped("Book metadata is optional and fetched only on request. "
         "Open Library is queried first; Google Books supplies a fallback. "
         "The detected ISBN remains visible independently of catalog availability.");
      ImGui::Checkbox("Auto lookup after confirmed ISBN", &bAutoBookLookup);
      ImGui::SetNextItemWidth(290);
      ImGui::InputText("ISBN-13 (hyphens accepted)",arrLookupISBN.data(),
         arrLookupISBN.size());
      ImGui::SameLine();
      if(bBookPending) {
         ImGui::TextDisabled("Searching catalogs...");
      }
      else if(ImGui::Button("LOOK UP BOOK")) {
         std::string strNormalized;
         for(char const c:std::string_view{arrLookupISBN.data()})
            if(c>='0' && c<='9') strNormalized.push_back(c);
         if(!IsIsbn13(strNormalized)) {
            theBook={};
            theBook.strStatus="Enter a valid 978/979 ISBN-13 with correct checksum.";
         }
         else
            BeginBookLookup(strNormalized);
      }
      if(!theBook.strStatus.empty())
         ImGui::TextWrapped("%s",theBook.strStatus.c_str());
      if(theBook.Found()) {
         ImGui::Text("Title: %s",theBook.strTitle.c_str());
         if(!theBook.strSubtitle.empty())
            ImGui::TextWrapped("Subtitle: %s",theBook.strSubtitle.c_str());
         if(!theBook.strAuthors.empty())
            ImGui::TextWrapped("Authors: %s",theBook.strAuthors.c_str());
         if(!theBook.strPublisher.empty())
            ImGui::TextWrapped("Publisher: %s",theBook.strPublisher.c_str());
         if(!theBook.strPublished.empty())
            ImGui::Text("Published: %s",theBook.strPublished.c_str());
         ImGui::Text("Source: %s",theBook.strProvider.c_str());
         if(!theBook.strSourceURL.empty()) {
            ImGui::TextWrapped("Catalog: %s",theBook.strSourceURL.c_str());
            if(ImGui::Button("COPY CATALOG LINK"))
               ImGui::SetClipboardText(theBook.strSourceURL.c_str());
         }
         if(!theBook.theCover.empty()) {
            ImGui::Text("Cover");
            theBookCover.Draw(180.0F,260.0F);
         }
         else
            ImGui::TextDisabled("No cover available from this catalog.");
      }
            if(!theLastConfirmed.vecText.empty()) {
         ImGui::SeparatorText("Last confirmed reading (retained)");
         ImGui::TextDisabled("Code format: %s",theLastConfirmed.vecTypes.front().c_str());
         for(std::size_t u=0;u<theLastConfirmed.vecText.size();++u)
            ImGui::TextWrapped("[%s] %s",
               u<theLastConfirmed.vecTypes.size()
                  ? theLastConfirmed.vecTypes[u].c_str() : "code",
               theLastConfirmed.vecText[u].c_str());
         for(std::string const& strValue:theLastConfirmed.vecText)
            if(std::optional<std::string> const strISBN10=ToIsbn10(strValue))
               ImGui::Text("ISBN-10: %s",strISBN10->c_str());
      }
      theViewport.Draw(std::max(300.0F,ImGui::GetContentRegionAvail().x),
         std::max(180.0F,ImGui::GetContentRegionAvail().y-12));
      ImGui::End();
      aApp.EndFrame();
   }
   theCapture.Disconnect();
}
}
int main(int const iArgc,char const* const* const pszArgv) {
   try {
      if(iArgc>1 && std::string_view{pszArgv[1]}=="--lookup-isbn") {
         if(iArgc<3) throw std::runtime_error("Usage: --lookup-isbn <ISBN-13>");
         std::string strISBN;
         for(char const c:std::string_view{pszArgv[2]})
            if(c>='0' && c<='9') strISBN.push_back(c);
         if(!IsIsbn13(strISBN))
            throw std::runtime_error("Invalid ISBN-13 checksum or 978/979 prefix");
         BookMetadata const theBook=BookLookup::Lookup(strISBN);
         std::println("SHOWCASE|BOOK|ISBN|{}",strISBN);
         std::println("SHOWCASE|BOOK|STATUS|{}",theBook.strStatus);
         if(theBook.Found()) {
            std::println("SHOWCASE|BOOK|TITLE|{}",theBook.strTitle);
            std::println("SHOWCASE|BOOK|SUBTITLE|{}",theBook.strSubtitle);
            std::println("SHOWCASE|BOOK|AUTHOR|{}",theBook.strAuthors);
            std::println("SHOWCASE|BOOK|PUBLISHER|{}",theBook.strPublisher);
            std::println("SHOWCASE|BOOK|PUBLISHED|{}",theBook.strPublished);
            std::println("SHOWCASE|BOOK|SOURCE|{}|{}",theBook.strProvider,
               theBook.strSourceURL);
            std::println("SHOWCASE|BOOK|COVER|{}|{}",
               theBook.theCover.empty()?"UNAVAILABLE":"AVAILABLE",theBook.strCoverURL);
         }
         else
            std::println("SHOWCASE|BOOK|NOT_FOUND|No confirmed entry in queried catalogs");
         return theBook.Found()?0:3;
      }
      if(iArgc>1 && std::string_view{pszArgv[1]}=="--list-cameras") {
         std::vector<CameraDevice> const vecDevices=EnumerateCameras();
         std::println("SHOWCASE|CAMERAS|{}|DirectShow registered devices",vecDevices.size());
         for(CameraDevice const& aCamera:vecDevices)
            std::println("SHOWCASE|CAMERA|{}|{}|{}",aCamera.iIndex,
               aCamera.strName,aCamera.strDevicePath);
         return 0;
      }
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
