#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"
#include "../common/CameraDevices.h"
#include "../common/CameraFrameView.h"
#include "DocumentRegion.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <filesystem>
#include <memory>
#include <format>
#include <cstdint>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct Recognition {
   cv::Mat theOriginal;
   cv::Mat theProcessed;
   std::string strRecognized;
   int iConfidence {};
   double flMilliseconds {};
};

/**
\brief Generates a deterministic synthetic OCR document using OpenCV.
\details This image is not precomputed or supplied as the recognized result.
\param svText English test text to paint.
\returns A grayscale image to pass through Leptonica and Tesseract.
\throw std::exception If OpenCV cannot allocate the image.
*/
cv::Mat MakeDocument(std::string_view const svText) {
   constexpr int iFont=cv::FONT_HERSHEY_SIMPLEX;
   constexpr double flFontScale=2.15;
   constexpr int iThickness=5;
   constexpr int iMargin=52;
   constexpr int iBaseline=160;
   // OpenCV's Hershey renderer has narrow default ASCII-space advances.
   // Draw measured words independently with a visible inter-word gap.
   // Never infer or insert spaces into Tesseract's recognition result.
   std::vector<std::string> vecWords;
   std::size_t uPos=0;
   while(uPos<svText.size()) {
      std::size_t const uStart=svText.find_first_not_of(' ',uPos);
      if(uStart==std::string_view::npos) break;
      std::size_t const uEnd=svText.find(' ',uStart);
      vecWords.emplace_back(svText.substr(uStart,
         uEnd==std::string_view::npos ? uEnd : uEnd-uStart));
      if(uEnd==std::string_view::npos) break;
      uPos=uEnd+1;
   }
   int iUnusedBaseline {};
   cv::Size const aReference=cv::getTextSize("TEST",iFont,
      flFontScale,iThickness,&iUnusedBaseline);
   int const iWordGap=std::max(70,aReference.height);
   int iTextWidth {};
   for(std::string const& strWord:vecWords)
      iTextWidth+=cv::getTextSize(strWord,iFont,
         flFontScale,iThickness,&iUnusedBaseline).width;
   if(vecWords.size()>1)
      iTextWidth+=static_cast<int>(vecWords.size()-1)*iWordGap;
   cv::Mat theImage(280,std::max(1120,iTextWidth+2*iMargin),
      CV_8UC1,cv::Scalar(255));
   int iX=iMargin;
   for(std::string const& strWord:vecWords) {
      cv::putText(theImage,strWord,cv::Point{iX,iBaseline},
         iFont,flFontScale,cv::Scalar(14),iThickness,cv::LINE_AA);
      iX+=cv::getTextSize(strWord,iFont,
         flFontScale,iThickness,&iUnusedBaseline).width+iWordGap;
   }
   return theImage;
}

/**
\brief Recognizes a real image using managed OpenCV, Leptonica and Tesseract.
\details OpenCV handles grayscale/threshold preprocessing. Leptonica owns a
   PIX copy of the resulting pixels; TessBaseAPI performs LSTM OCR using the
   separately installed English language model.
\param theInput Original grayscale or BGR image.
\param bThreshold Whether to apply Otsu binarization.
\param iPageMode Tesseract text segmentation mode.\n\param strLanguage Explicit managed traineddata selection, eng/deu/deu+eng.
\returns Original, processed image, recognized UTF-8 text, confidence and time.
\throw std::runtime_error When recognition initialization or processing fails.
*/
Recognition Recognize(cv::Mat const& theInput,bool const bThreshold,
   tesseract::PageSegMode const iPageMode,std::string const& strLanguage="eng") {
   if(theInput.empty())
      throw std::runtime_error("No OCR input image");
   Recognition theResult;
   theResult.theOriginal=theInput.clone();
   auto const theStart=std::chrono::steady_clock::now();
   cv::Mat theGray;
   if(theInput.channels()==3)
      cv::cvtColor(theInput,theGray,cv::COLOR_BGR2GRAY);
   else if(theInput.channels()==4)
      cv::cvtColor(theInput,theGray,cv::COLOR_BGRA2GRAY);
   else if(theInput.channels()==1)
      theGray=theInput.clone();
   else
      throw std::runtime_error("Unsupported number of input image channels");
   if(bThreshold)
      cv::threshold(theGray,theResult.theProcessed,0,255,
         cv::THRESH_BINARY|cv::THRESH_OTSU);
   else
      theResult.theProcessed=theGray;
   if(strLanguage!="eng" && strLanguage!="deu" && strLanguage!="deu+eng")
      throw std::runtime_error("Unapproved OCR language selection");
   std::filesystem::path const theDataDir{ADECC_TESSDATA_PATH};
   if(!std::filesystem::is_regular_file(theDataDir/"eng.traineddata"))
      throw std::runtime_error("Managed Tesseract English traineddata missing");
   if(strLanguage!="eng" &&
      !std::filesystem::is_regular_file(theDataDir/"deu.traineddata"))
      throw std::runtime_error("German OCR model is missing from managed Tesseract package: deu.traineddata");
   tesseract::TessBaseAPI theAPI;
   if(theAPI.Init(ADECC_TESSDATA_PATH,strLanguage.c_str(),tesseract::OEM_LSTM_ONLY)!=0)
      throw std::runtime_error("Tesseract English model initialization failed");
   theAPI.SetPageSegMode(iPageMode);
   // Leptonica owns the PIX; pixDestroy is required after the OCR call.
   auto const fnDestroy=[](PIX* pPix) { pixDestroy(&pPix); };
   using pix_owner=std::unique_ptr<PIX,decltype(fnDestroy)>;
   pix_owner pImage(pixCreate(theResult.theProcessed.cols,
      theResult.theProcessed.rows,8),fnDestroy);
   if(!pImage)
      throw std::runtime_error("Leptonica failed to allocate a PIX");
   for(int iY=0;iY<theResult.theProcessed.rows;++iY) {
      unsigned char const* pLine=theResult.theProcessed.ptr<unsigned char>(iY);
      for(int iX=0;iX<theResult.theProcessed.cols;++iX)
         pixSetPixel(pImage.get(),iX,iY,pLine[iX]);
   }
   theAPI.SetImage(pImage.get());
   if(theAPI.Recognize(nullptr)!=0)
      throw std::runtime_error("Tesseract failed to recognize the image");
   using text_owner=std::unique_ptr<char[]>;
   text_owner pText(theAPI.GetUTF8Text());
   if(!pText)
      throw std::runtime_error("Tesseract returned no UTF-8 output");
   theResult.strRecognized=pText.get();
   while(!theResult.strRecognized.empty() &&
      (theResult.strRecognized.back()=='\n' ||
       theResult.strRecognized.back()=='\r' ||
       theResult.strRecognized.back()==' '))
      theResult.strRecognized.pop_back();
   theResult.iConfidence=theAPI.MeanTextConf();
   theResult.flMilliseconds=std::chrono::duration<double,std::milli>(
      std::chrono::steady_clock::now()-theStart).count();
   return theResult;
}

bool Verify() {
   std::string const strExpected="OCR TEST 4711";
   cv::Mat const theFixture=MakeDocument(strExpected);
   // Verify that the source really contains whitespace between adjacent words.
   // Otherwise a recognized OCRTEST would be a rendering regression rather
   // than a defect in Tesseract's text-space segmentation.
   cv::Mat theInk;
   cv::threshold(theFixture,theInk,128,255,cv::THRESH_BINARY_INV);
   cv::Mat theColumnInk;
   cv::reduce(theInk,theColumnInk,0,cv::REDUCE_MAX,CV_8U);
   int iLargestGap {};
   int iCurrentGap {};
   bool bSeenInk {};
   for(int iX=0;iX<theColumnInk.cols;++iX) {
      bool const bInk=theColumnInk.at<unsigned char>(0,iX)!=0;
      if(bInk) {
         if(bSeenInk) iLargestGap=std::max(iLargestGap,iCurrentGap);
         iCurrentGap=0;
         bSeenInk=true;
      }
      else if(bSeenInk) {
         ++iCurrentGap;
      }
   }
   bool const bWordSeparation=iLargestGap>=55;
   std::println("SHOWCASE|CHECK|ocr-fixture-word-spacing|{}|largest-white-gap={}px",
      bWordSeparation?"PASS":"FAIL",iLargestGap);
   Recognition const theResult=Recognize(theFixture,true,
      tesseract::PSM_SINGLE_LINE);
   bool const bExact=theResult.strRecognized==strExpected;
   bool const bConfidence=theResult.iConfidence>=0 &&
      theResult.iConfidence<=100;
   std::println("SHOWCASE|CHECK|tesseract-exact|{}|expected={} actual={}",
      bExact?"PASS":"FAIL",strExpected,theResult.strRecognized);
   std::println("SHOWCASE|CHECK|tesseract-confidence|{}|confidence={} elapsed-ms={:.2f}",
      bConfidence?"PASS":"FAIL",theResult.iConfidence,theResult.flMilliseconds);
   // Validate document extraction independently from an attached camera.
   cv::Mat thePage(540,760,CV_8UC1,cv::Scalar{255});
   cv::rectangle(thePage,cv::Rect{120,90,450,330},cv::Scalar{0},4);
   cv::Mat const theSelected=ExtractDocument(thePage,
      DocumentRegion::Mode::ManualROI,{{.15F,.12F,.65F,.70F}}).theDocument;
   DocumentRegion const theAutoPage=ExtractDocument(thePage,
      DocumentRegion::Mode::AutoPage,{{.0F,.0F,1.0F,1.0F}});
   bool const bManualRegion=theSelected.cols>=450 &&
      theSelected.rows>=350;
   bool const bAutoRegion=theAutoPage.bFound &&
      theAutoPage.theDocument.cols>=350 &&
      theAutoPage.theDocument.rows>=250;
   std::println("SHOWCASE|CHECK|ocr-document-manual-region|{}|size={}x{}",
      bManualRegion?"PASS":"FAIL",theSelected.cols,theSelected.rows);
   std::println("SHOWCASE|CHECK|ocr-document-perspective|{}|detected={} size={}x{}",
      bAutoRegion?"PASS":"FAIL",theAutoPage.bFound,
      theAutoPage.theDocument.cols,theAutoPage.theDocument.rows);
   std::println("SHOWCASE|RESULT|{}|OpenCV preprocessing, Leptonica PIX and native Tesseract LSTM",
      bExact&&bConfidence&&bWordSeparation&&bManualRegion&&bAutoRegion?"PASS":"FAIL");
   return bExact&&bConfidence&&bWordSeparation&&bManualRegion&&bAutoRegion;
}

void Interactive() {
   NativeShowcase aApp{"BuildEngine | TESSERACT OCR STUDIO",1500,950};
   CameraFrameView theCameraView{aApp.Window()};
   CameraFrameView theSourceView{aApp.Window()};
   CameraFrameView theProcessedView{aApp.Window()};
   DirectShowCapture theCapture;
   std::vector<CameraDevice> vecCameras;
   std::string strEnumerationError;
   auto const RefreshCameras=[&]() {
      try {
         vecCameras=EnumerateCameras();
         strEnumerationError.clear();
      }
      catch(std::exception const& theError) {
         vecCameras.clear();
         strEnumerationError=theError.what();
      }
   };
   RefreshCameras();

   int iSelectedCamera=vecCameras.empty()?-1:0;
   int iManualIndex=0;
   int iCameraResolution=2;
   std::array<char,256> arrText {};
   std::string const strDefault="OCR TEST 4711";
   std::copy(strDefault.begin(),strDefault.end(),arrText.begin());
   std::array<char,512> arrFile {};
   bool bThreshold=true;
   int iPageMode=0;
   bool const bGermanModelInstalled=std::filesystem::is_regular_file(
      std::filesystem::path{ADECC_TESSDATA_PATH}/"deu.traineddata");
   int iLanguage=bGermanModelInstalled?2:0;
   int iRegionMode=2;
   std::array<float,4> arrROI {.10F,.10F,.80F,.80F};
   bool bAutoUpscale=true;
   bool bScanOnRelease=false;
   bool bDraggingSelection=false;
   ImVec2 aSelectionStart {};
   DocumentRegion theLastRegion;
   bool bFrozen=false;
   cv::Mat theLatestFrame;
   cv::Mat theFrozenFrame;
   Recognition theResult;
   std::string strError;
   std::string strCaptureError;
   std::uint64_t uLastCapture {};
   std::uint64_t uFrames {};
   int iReadFailures {};

   auto const ReleaseCamera=[&]() {
      theCapture.Disconnect();
      bFrozen=false;
      bDraggingSelection=false;
      theFrozenFrame.release();
   };
   auto const Run=[&](cv::Mat const& theInput,bool const bCameraInput) {
      try {
         DocumentRegion theRegion=ExtractDocument(theInput,
            bCameraInput?static_cast<DocumentRegion::Mode>(iRegionMode):
               DocumentRegion::Mode::FullFrame,arrROI);
         cv::Mat theDocument=theRegion.theDocument;
         if(bCameraInput && bAutoUpscale) {
            double const flScale=std::min(2.5,
               std::max(1.0,1800.0/std::max(theDocument.cols,theDocument.rows)));
            if(flScale>1.01)
               cv::resize(theDocument,theDocument,cv::Size{},
                  flScale,flScale,cv::INTER_CUBIC);
         }
         tesseract::PageSegMode const ePSM=iPageMode==0
            ? tesseract::PSM_AUTO
            : iPageMode==1?tesseract::PSM_SINGLE_BLOCK:tesseract::PSM_SINGLE_LINE;
         Recognition theNewResult=Recognize(theDocument,bThreshold,ePSM,
            iLanguage==0?"eng":iLanguage==1?"deu":"deu+eng");
         theLastRegion=std::move(theRegion);
         theResult=std::move(theNewResult);
         theSourceView.Update(theResult.theOriginal);
         theProcessedView.Update(theResult.theProcessed);
         strError.clear();
      }
      catch(std::exception const& theError) {
         strError=theError.what();
      }
   };
   Run(MakeDocument(arrText.data()),false);
   while(aApp.Running()) {
      if(theCapture.Connected() && !bFrozen) {
         std::uint64_t const uNow=SDL_GetTicks64();
         if(uNow-uLastCapture>=70) {
            uLastCapture=uNow;
            try {
               cv::Mat theFrame;
               if(theCapture.Read(theFrame)) {
                  theLatestFrame=theFrame.clone();
                  theCameraView.Update(theLatestFrame);
                  ++uFrames;
                  iReadFailures=0;
                  strCaptureError.clear();
               }
               else if(++iReadFailures>=10) {
                  ReleaseCamera();
                  strCaptureError="Camera returned no frames. Select and reconnect a device.";
               }
            }
            catch(std::exception const& theError) {
               ReleaseCamera();
               strCaptureError=theError.what();
            }
         }
      }

      aApp.BeginFrame();
      ImGui::SetNextWindowPos({0,0});
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::Begin("TESSERACT OCR STUDIO",nullptr,
         ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("TESSERACT / OCR STUDIO",
         "OpenCV 5.1 camera and preprocessing / Leptonica PIX / Tesseract LSTM");

      ImGui::SeparatorText("1. DirectShow camera");
      if(ImGui::Button("REFRESH CAMERAS")) {
         std::string strSelectedPath;
         if(iSelectedCamera>=0 &&
            iSelectedCamera<static_cast<int>(vecCameras.size()))
            strSelectedPath=vecCameras[static_cast<std::size_t>(iSelectedCamera)].strDevicePath;
         ReleaseCamera();
         RefreshCameras();
         iSelectedCamera=vecCameras.empty()?-1:0;
         if(!strSelectedPath.empty())
            for(std::size_t u=0;u<vecCameras.size();++u)
               if(vecCameras[u].strDevicePath==strSelectedPath) {
                  iSelectedCamera=static_cast<int>(u);
                  break;
               }
      }
      ImGui::SameLine();
      ImGui::TextDisabled("Registered cameras: %d",
         static_cast<int>(vecCameras.size()));
      if(!strEnumerationError.empty())
         ImGui::TextColored({1,.4F,.4F,1},
            "Enumeration: %s",strEnumerationError.c_str());
      std::string strSelection="Select registered camera";
      if(iSelectedCamera>=0 &&
         iSelectedCamera<static_cast<int>(vecCameras.size())) {
         CameraDevice const& theDevice=vecCameras[static_cast<std::size_t>(iSelectedCamera)];
         strSelection=std::format("[{}] {}",theDevice.iIndex,theDevice.strName);
      }
      ImGui::SetNextItemWidth(740);
      if(ImGui::BeginCombo("Camera",strSelection.c_str())) {
         for(std::size_t u=0;u<vecCameras.size();++u) {
            CameraDevice const& theDevice=vecCameras[u];
            std::string const strLabel=std::format("[{}] {}##ocrCamera{}",
               theDevice.iIndex,theDevice.strName,u);
            if(ImGui::Selectable(strLabel.c_str(),
               iSelectedCamera==static_cast<int>(u))) {
               ReleaseCamera();
               iSelectedCamera=static_cast<int>(u);
            }
         }
         ImGui::EndCombo();
      }
      ImGui::SetNextItemWidth(110);
      ImGui::InputInt("Manual DirectShow index",&iManualIndex);
      iManualIndex=std::clamp(iManualIndex,0,31);
      ImGui::SetNextItemWidth(250);
      ImGui::Combo("Requested camera resolution",&iCameraResolution,
         "Device default\0HD 1280 x 720\0Full HD 1920 x 1080\0UHD 3840 x 2160\0");
      auto const ConnectCamera=[&](int const iIndex) {
         try {
            ReleaseCamera();
            theLatestFrame.release();
            uLastCapture=0;
            uFrames=0;
            iReadFailures=0;
            constexpr std::array<std::array<int,2>,4> arrResolutions {{
               {{0,0}},{{1280,720}},{{1920,1080}},{{3840,2160}}
            }};
            auto const& arrResolution=arrResolutions[static_cast<std::size_t>(iCameraResolution)];
            if(theCapture.Connect(iIndex,arrResolution[0],arrResolution[1]))
               strCaptureError.clear();
            else
               strCaptureError=std::format("DirectShow camera {} could not be opened.",iIndex);
         }
         catch(std::exception const& theError) {
            ReleaseCamera();
            strCaptureError=theError.what();
         }
      };
      if(ImGui::Button("CONNECT SELECTED")) {
         if(iSelectedCamera>=0 &&
            iSelectedCamera<static_cast<int>(vecCameras.size()))
            ConnectCamera(vecCameras[static_cast<std::size_t>(iSelectedCamera)].iIndex);
         else
            strCaptureError="Select a registered DirectShow camera first.";
      }
      ImGui::SameLine();
      if(ImGui::Button("CONNECT BY INDEX")) ConnectCamera(iManualIndex);
      ImGui::SameLine();
      if(ImGui::Button("DISCONNECT")) ReleaseCamera();
      ImGui::SameLine();
      ImGui::TextDisabled("%s / index %d / frames %llu",
         theCapture.Connected()?(bFrozen?"FROZEN":"LIVE"):"DISCONNECTED",
         theCapture.Index(),static_cast<unsigned long long>(uFrames));
      if(!strCaptureError.empty())
         ImGui::TextColored({1,.4F,.4F,1},"Capture: %s",strCaptureError.c_str());

      if(ImGui::Button(bFrozen?"RESUME LIVE":"FREEZE FRAME") &&
         theCapture.Connected()) {
         if(bFrozen) {
            bFrozen=false;
            theFrozenFrame.release();
            uLastCapture=0;
         }
         else if(!theLatestFrame.empty()) {
            theFrozenFrame=theLatestFrame.clone();
            bFrozen=true;
         }
      }
      ImGui::SameLine();
      if(ImGui::Button("RECOGNIZE FRAME")) {
         cv::Mat const& theInput=bFrozen?theFrozenFrame:theLatestFrame;
         if(!theInput.empty()) {
            // Recognition operates on an immutable captured frame; do not
            // invoke Tesseract on every SDL video or ImGui refresh.
            cv::Mat const theSnapshot=theInput.clone();
            Run(theSnapshot,true);
         }
         else
            strError="No camera frame is available for OCR.";
      }
      ImGui::SameLine();
      if(ImGui::Button("SCAN SELECTION")) {
         if(iRegionMode!=1)
            strError="Select a rectangle by dragging on the camera image first.";
         else {
            cv::Mat const& theInput=bFrozen?theFrozenFrame:theLatestFrame;
            if(theInput.empty())
               strError="No camera frame is available for scanning.";
            else
               Run(theInput.clone(),true);
         }
      }
      if(!theLatestFrame.empty())
         ImGui::Text("Actual captured pixels: %d x %d",
            theLatestFrame.cols,theLatestFrame.rows);
      ImGui::TextDisabled("Drag a rectangle on the camera image, then SCAN SELECTION.");

      ImGui::SeparatorText("2. Recognition settings and alternative inputs");
      ImGui::SetNextItemWidth(260);
      ImGui::Combo("Recognition language",&iLanguage,
         "English (eng)\0German (deu)\0German + English\0");
      if(!bGermanModelInstalled)
         ImGui::TextColored({1.0F,.7F,.3F,1},
            "German model not installed in active Tesseract package: only English OCR is available.");
      ImGui::Checkbox("Otsu threshold",&bThreshold);
      ImGui::SameLine();
      ImGui::Checkbox("Upscale small camera text",&bAutoUpscale);
      ImGui::SetNextItemWidth(270);
      ImGui::Combo("Tesseract text layout",&iPageMode,
         "Automatic page (PSM 3)\0Single text block (PSM 6)\0Single line (PSM 7)\0");
      ImGui::SetNextItemWidth(290);
      ImGui::Combo("Camera document area",&iRegionMode,
         "Whole camera frame\0Manual text rectangle\0Detect document quadrilateral\0");
      if(iRegionMode==1) {
         ImGui::SliderFloat("Crop left",&arrROI[0],0.0F,.95F);
         ImGui::SliderFloat("Crop top",&arrROI[1],0.0F,.95F);
         ImGui::SliderFloat("Crop width",&arrROI[2],.05F,1.0F-arrROI[0]);
         ImGui::SliderFloat("Crop height",&arrROI[3],.05F,1.0F-arrROI[1]);
      }
      if(iRegionMode==2)
         ImGui::TextDisabled("If no page border exists, full frame is used; use Manual rectangle for a text block.");
      ImGui::Checkbox("Scan immediately on mouse release",&bScanOnRelease);
      ImGui::SetNextItemWidth(560);
      ImGui::InputText("Synthetic document",arrText.data(),arrText.size());
      if(ImGui::Button("RENDER + RECOGNIZE")) {
         Run(MakeDocument(arrText.data()),false);
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(640);
      ImGui::InputText("Image file (PNG/JPEG)",arrFile.data(),arrFile.size());
      ImGui::SameLine();
      if(ImGui::Button("OCR FILE")) {
         cv::Mat const theInput=cv::imread(arrFile.data(),cv::IMREAD_COLOR);
         Run(theInput,false);
      }

      ImGui::SeparatorText("3. Camera image and recognized text");
      if(!strError.empty())
         ImGui::TextColored({1,.35F,.44F,1},"OCR error: %s",strError.c_str());
      else {
         ImGui::Text("Confidence: %d%% | Processing: %.2f ms | OCR image: %d x %d",
            theResult.iConfidence,theResult.flMilliseconds,
            theResult.theProcessed.cols,theResult.theProcessed.rows);
         ImGui::TextDisabled("Document region: %s",
            theLastRegion.eMode==DocumentRegion::Mode::AutoPage
               ? (theLastRegion.bFound?"Detected and rectified":"Not detected (full frame)")
               : theLastRegion.eMode==DocumentRegion::Mode::ManualROI
                  ? "Selected text rectangle":"Full frame");
      }
      ImGui::Text("Recognized UTF-8 (scrollable, copyable):");
      if(ImGui::Button("COPY RECOGNIZED TEXT"))
         ImGui::SetClipboardText(theResult.strRecognized.c_str());
      ImGui::InputTextMultiline("##ocrText",
         theResult.strRecognized.data(),theResult.strRecognized.size()+1,
         ImVec2{std::max(320.0F,ImGui::GetContentRegionAvail().x),180},
         ImGuiInputTextFlags_ReadOnly);
      ImGui::SeparatorText("4. Live camera / document source / processed pixels");
      if(theCapture.Connected()) {
         ImGui::Text("Live camera frame:");
         float const flCameraWidth=std::max(350.0F,ImGui::GetContentRegionAvail().x);
         if(iRegionMode==1)
            theCameraView.DrawWithROI(flCameraWidth,310.0F,arrROI);
         else
            theCameraView.Draw(flCameraWidth,310.0F);
         ImVec2 const aMin=ImGui::GetItemRectMin();
         ImVec2 const aMax=ImGui::GetItemRectMax();
         bool const bHovered=ImGui::IsItemHovered();
         ImVec2 const aMouse=ImGui::GetIO().MousePos;
         auto const Normalize=[&](ImVec2 const aPosition) -> ImVec2 {
            return {
               std::clamp((aPosition.x-aMin.x)/(aMax.x-aMin.x),0.0F,1.0F),
               std::clamp((aPosition.y-aMin.y)/(aMax.y-aMin.y),0.0F,1.0F)};
         };
         if(bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !theLatestFrame.empty()) {
            // Freeze the exact image being selected, preventing ROI/frame drift.
            if(!bFrozen) {
               theFrozenFrame=theLatestFrame.clone();
               bFrozen=true;
            }
            bDraggingSelection=true;
            aSelectionStart=Normalize(aMouse);
         }
         if(bDraggingSelection) {
            ImVec2 const aEnd=Normalize(aMouse);
            float const flLeft=std::min(aSelectionStart.x,aEnd.x);
            float const flTop=std::min(aSelectionStart.y,aEnd.y);
            float const flWidth=std::abs(aEnd.x-aSelectionStart.x);
            float const flHeight=std::abs(aEnd.y-aSelectionStart.y);
            if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
               ImGui::GetWindowDrawList()->AddRect(
                  {aMin.x+flLeft*(aMax.x-aMin.x),aMin.y+flTop*(aMax.y-aMin.y)},
                  {aMin.x+(flLeft+flWidth)*(aMax.x-aMin.x),
                   aMin.y+(flTop+flHeight)*(aMax.y-aMin.y)},
                  IM_COL32(80,220,140,255),0.0F,0,3.0F);
            }
            else {
               bDraggingSelection=false;
               if(flWidth>=.005F && flHeight>=.005F) {
                  arrROI={flLeft,flTop,flWidth,flHeight};
                  iRegionMode=1;
                  if(bScanOnRelease && !theFrozenFrame.empty())
                     Run(theFrozenFrame.clone(),true);
               }
            }
         }
      }
      else
         ImGui::TextDisabled("Camera disconnected; synthetic and file OCR remain usable.");
      float const flW=std::max(300.0F,ImGui::GetContentRegionAvail().x);
      ImGui::Text("Selected document / rectified source:");
      theSourceView.Draw(flW,450.0F);
      ImGui::Text("Actual OpenCV preprocessing supplied to Tesseract:");
      theProcessedView.Draw(flW,450.0F);
      ImGui::End();
      aApp.EndFrame();
   }
   ReleaseCamera();
}
}
int main(int const iArgc,char const* const* const pszArgv) {
   try {
      if(iArgc>1 && std::string_view{pszArgv[1]}=="--list-cameras") {
         std::vector<CameraDevice> const vecCameras=EnumerateCameras();
         std::println("SHOWCASE|CAMERAS|{}|Registered DirectShow devices",vecCameras.size());
         for(CameraDevice const& theCamera:vecCameras)
            std::println("SHOWCASE|CAMERA|{}|{}|{}",theCamera.iIndex,
               theCamera.strName,theCamera.strDevicePath);
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
