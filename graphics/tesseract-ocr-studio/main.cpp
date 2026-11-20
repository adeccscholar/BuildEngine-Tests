#include "../common/GraphicsDllImports.h"
#include "../common/NativeShowcase.h"

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
\param iPageMode Tesseract PSM_SINGLE_LINE or PSM_AUTO.
\returns Original, processed image, recognized UTF-8 text, confidence and time.
\throw std::runtime_error When recognition initialization or processing fails.
*/
Recognition Recognize(cv::Mat const& theInput,bool const bThreshold,
   tesseract::PageSegMode const iPageMode) {
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
   if(!std::filesystem::is_regular_file(
      std::filesystem::path{ADECC_TESSDATA_PATH}/"eng.traineddata"))
      throw std::runtime_error("Managed Tesseract English traineddata missing");
   tesseract::TessBaseAPI theAPI;
   if(theAPI.Init(ADECC_TESSDATA_PATH,"eng",tesseract::OEM_LSTM_ONLY)!=0)
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

/** \brief Draws real grayscale pixels as a compact ImGui diagnostic viewport. */
void DrawGray(cv::Mat const& theImage,ImVec2 const aSize) {
   ImVec2 const aStart=ImGui::GetCursorScreenPos();
   ImDrawList* const pDraw=ImGui::GetWindowDrawList();
   pDraw->AddRectFilled(aStart,{aStart.x+aSize.x,aStart.y+aSize.y},
      IM_COL32(250,252,255,255),10.0F);
   if(!theImage.empty()) {
      cv::Mat theGray;
      if(theImage.channels()==1) theGray=theImage;
      else cv::cvtColor(theImage,theGray,cv::COLOR_BGR2GRAY);
      int const iWidth=std::clamp(static_cast<int>(aSize.x/3),1,470);
      int const iHeight=std::clamp(static_cast<int>(aSize.y/3),1,120);
      cv::Mat theSmall;
      cv::resize(theGray,theSmall,cv::Size{iWidth,iHeight},0,0,cv::INTER_AREA);
      float const flScale=std::min(aSize.x/iWidth,aSize.y/iHeight);
      float const flOffsetX=aStart.x+(aSize.x-iWidth*flScale)*.5F;
      float const flOffsetY=aStart.y+(aSize.y-iHeight*flScale)*.5F;
      for(int iY=0;iY<iHeight;++iY) {
         unsigned char const* pLine=theSmall.ptr<unsigned char>(iY);
         int iX=0;
         while(iX<iWidth) {
            int const iStart=iX;
            unsigned char const uValue=pLine[iX];
            while(iX<iWidth && pLine[iX]==uValue) ++iX;
            pDraw->AddRectFilled(
               {flOffsetX+iStart*flScale,flOffsetY+iY*flScale},
               {flOffsetX+iX*flScale+.25F,flOffsetY+(iY+1)*flScale+.25F},
               IM_COL32(uValue,uValue,uValue,255));
         }
      }
   }
   ImGui::Dummy(aSize);
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
   std::println("SHOWCASE|RESULT|{}|OpenCV preprocessing, Leptonica PIX and native Tesseract LSTM",
      bExact&&bConfidence&&bWordSeparation?"PASS":"FAIL");
   return bExact&&bConfidence&&bWordSeparation;
}

void Interactive() {
   NativeShowcase aApp{"BuildEngine | TESSERACT OCR STUDIO",1450,900};
   std::array<char,256> arrText {};
   std::string const strDefault="OCR TEST 4711";
   std::copy(strDefault.begin(),strDefault.end(),arrText.begin());
   std::array<char,512> arrFile {};
   bool bThreshold=true;
   bool bSingleLine=true;
   Recognition theResult;
   std::string strError;
   auto const Run=[&](bool const bFromFile) {
      try {
         cv::Mat const theInput=bFromFile
            ? cv::imread(arrFile.data(),cv::IMREAD_COLOR)
            : MakeDocument(arrText.data());
         theResult=Recognize(theInput,bThreshold,
            bSingleLine?tesseract::PSM_SINGLE_LINE:tesseract::PSM_AUTO);
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
      ImGui::Begin("TESSERACT OCR STUDIO",nullptr,
         ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
      NativeShowcase::Header("TESSERACT / OCR STUDIO",
         "OpenCV grayscale + Otsu / Leptonica PIX / Tesseract 5.5 English LSTM");
      ImGui::SetNextItemWidth(650);
      ImGui::InputText("Synthetic document",arrText.data(),arrText.size());
      ImGui::Checkbox("Otsu threshold",&bThreshold);
      ImGui::SameLine();
      ImGui::Checkbox("Single line (otherwise auto layout)",&bSingleLine);
      if(ImGui::Button("RENDER + RECOGNIZE")) Run(false);
      ImGui::SeparatorText("Recognize a document from disk");
      ImGui::SetNextItemWidth(730);
      ImGui::InputText("Image file (PNG/JPEG)",arrFile.data(),arrFile.size());
      ImGui::SameLine();
      if(ImGui::Button("OCR FILE")) Run(true);
      if(!strError.empty())
         ImGui::TextColored({1,.35F,.44F,1},"OCR error: %s",strError.c_str());
      else {
         ImGui::Text("Tesseract LSTM confidence: %d%% | Processing: %.2f ms",
            theResult.iConfidence,theResult.flMilliseconds);
         ImGui::TextColored({.35F,.95F,.75F,1},"Recognized UTF-8 text:");
         ImGui::TextWrapped("%s",theResult.strRecognized.c_str());
      }
      ImGui::SeparatorText("Original input");
      float const flW=std::max(300.0F,ImGui::GetContentRegionAvail().x);
      DrawGray(theResult.theOriginal,{flW,190});
      ImGui::SeparatorText("OpenCV preprocessing output");
      DrawGray(theResult.theProcessed,{flW,190});
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
