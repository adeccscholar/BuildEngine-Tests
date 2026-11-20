#pragma once

#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

/**
\brief Selects a page region without changing the source camera pixels.
\details Supports the whole image, an operator-defined rectangular ROI, or
   a four-corner document detected from the real OpenCV image geometry.
*/
struct DocumentRegion {
   enum class Mode { FullFrame, ManualROI, AutoPage };
   Mode eMode {Mode::AutoPage};
   std::array<float,4> arrROI {.10F,.10F,.80F,.80F};
   bool bFound {};
   cv::Mat theDocument;
   std::array<cv::Point2f,4> arrCorners {};
};

/**
\brief Extracts a document area for genuine multiparagraph OCR.
\details AutoPage uses Canny, findContours, approxPolyDP and
   getPerspectiveTransform/warpPerspective from the managed OpenCV package.
   If no reliable quadrilateral is found, the unchanged full image is
   returned and bFound=false; there is no invented page geometry.
\param theFrame Captured OpenCV BGR or grayscale frame.
\param eMode Full frame, manual ROI or automatic page quadrilateral.
\param arrROI Normalized left,top,width,height for the manual crop.
\returns Selected high-resolution document and original-image corners.
\throw std::runtime_error When no input is supplied.
*/
inline DocumentRegion ExtractDocument(cv::Mat const& theFrame,
   DocumentRegion::Mode const eMode,
   std::array<float,4> const& arrROI) {
   if(theFrame.empty())
      throw std::runtime_error("Cannot extract an empty OCR document");
   DocumentRegion theRegion;
   theRegion.eMode=eMode;
   theRegion.arrROI=arrROI;
   theRegion.theDocument=theFrame.clone();
   if(eMode==DocumentRegion::Mode::FullFrame) return theRegion;

   if(eMode==DocumentRegion::Mode::ManualROI) {
      int const iLeft=std::clamp(static_cast<int>(arrROI[0]*theFrame.cols),
         0,theFrame.cols-1);
      int const iTop=std::clamp(static_cast<int>(arrROI[1]*theFrame.rows),
         0,theFrame.rows-1);
      int const iWidth=std::clamp(static_cast<int>(arrROI[2]*theFrame.cols),
         1,theFrame.cols-iLeft);
      int const iHeight=std::clamp(static_cast<int>(arrROI[3]*theFrame.rows),
         1,theFrame.rows-iTop);
      cv::Rect const aROI {iLeft,iTop,iWidth,iHeight};
      theRegion.theDocument=theFrame(aROI).clone();
      theRegion.arrCorners = {{
         {static_cast<float>(aROI.x),static_cast<float>(aROI.y)},
         {static_cast<float>(aROI.x+aROI.width),static_cast<float>(aROI.y)},
         {static_cast<float>(aROI.x+aROI.width),static_cast<float>(aROI.y+aROI.height)},
         {static_cast<float>(aROI.x),static_cast<float>(aROI.y+aROI.height)}
      }};
      theRegion.bFound=true;
      return theRegion;
   }

   cv::Mat theGray;
   if(theFrame.channels()==3)
      cv::cvtColor(theFrame,theGray,cv::COLOR_BGR2GRAY);
   else if(theFrame.channels()==4)
      cv::cvtColor(theFrame,theGray,cv::COLOR_BGRA2GRAY);
   else
      theGray=theFrame;

   // Detect at reduced size, but rectify the original full-resolution frame.
   double const flScale=std::min(1.0,1000.0/std::max(theFrame.cols,theFrame.rows));
   cv::Mat theSmall;
   cv::resize(theGray,theSmall,cv::Size{},flScale,flScale,cv::INTER_AREA);
   cv::GaussianBlur(theSmall,theSmall,cv::Size{5,5},0);
   cv::Mat theEdges;
   cv::Canny(theSmall,theEdges,45,125);
   cv::morphologyEx(theEdges,theEdges,cv::MORPH_CLOSE,
      cv::getStructuringElement(cv::MORPH_RECT,cv::Size{5,5}));
   std::vector<std::vector<cv::Point>> vecContours;
   cv::findContours(theEdges,vecContours,cv::RETR_LIST,cv::CHAIN_APPROX_SIMPLE);
   double flBestArea {};
   std::vector<cv::Point> vecBest;
   double const flImageArea=static_cast<double>(theSmall.cols)*theSmall.rows;
   for(auto const& vecContour:vecContours) {
      double const flPerimeter=cv::arcLength(vecContour,true);
      if(flPerimeter<1) continue;
      std::vector<cv::Point> vecApprox;
      cv::approxPolyDP(vecContour,vecApprox,0.025*flPerimeter,true);
      if(vecApprox.size()!=4 || !cv::isContourConvex(vecApprox)) continue;
      double const flArea=std::abs(cv::contourArea(vecApprox));
      if(flArea>flBestArea && flArea>flImageArea*.08 &&
         flArea<flImageArea*.97) {
         flBestArea=flArea;
         vecBest=std::move(vecApprox);
      }
   }
   if(vecBest.size()!=4) return theRegion;

   std::array<cv::Point2f,4> arrCorners {};
   cv::Point2f aCenter {};
   for(int i=0;i<4;++i) {
      arrCorners[static_cast<std::size_t>(i)]=cv::Point2f{
         static_cast<float>(vecBest[static_cast<std::size_t>(i)].x/flScale),
         static_cast<float>(vecBest[static_cast<std::size_t>(i)].y/flScale)};
      aCenter+=arrCorners[static_cast<std::size_t>(i)];
   }
   aCenter*=.25F;
   std::sort(arrCorners.begin(),arrCorners.end(),
      [&](cv::Point2f const& a,cv::Point2f const& b) {
         return std::atan2(a.y-aCenter.y,a.x-aCenter.x) <
            std::atan2(b.y-aCenter.y,b.x-aCenter.x);
      });
   // Rotate circular order so the first corner is top-left.
   auto const pTopLeft=std::min_element(arrCorners.begin(),arrCorners.end(),
      [](cv::Point2f const& a,cv::Point2f const& b) {
         return a.x+a.y<b.x+b.y;
      });
   std::rotate(arrCorners.begin(),pTopLeft,arrCorners.end());
   // After rotation winding can be TL,TR,BR,BL or TL,BL,BR,TR.
   if(arrCorners[1].x<arrCorners[3].x)
      std::swap(arrCorners[1],arrCorners[3]);

   auto const Distance=[](cv::Point2f const& a,cv::Point2f const& b) {
      return cv::norm(a-b);
   };
   int const iWidth=static_cast<int>(std::max(
      Distance(arrCorners[0],arrCorners[1]),
      Distance(arrCorners[3],arrCorners[2])));
   int const iHeight=static_cast<int>(std::max(
      Distance(arrCorners[0],arrCorners[3]),
      Distance(arrCorners[1],arrCorners[2])));
   if(iWidth<100 || iHeight<100) return theRegion;
   std::array<cv::Point2f,4> const arrTarget {{
      {0,0},{static_cast<float>(iWidth-1),0},
      {static_cast<float>(iWidth-1),static_cast<float>(iHeight-1)},
      {0,static_cast<float>(iHeight-1)}
   }};
   cv::Mat const theTransform=cv::getPerspectiveTransform(
      arrCorners.data(),arrTarget.data());
   cv::warpPerspective(theFrame,theRegion.theDocument,theTransform,
      cv::Size{iWidth,iHeight},cv::INTER_LINEAR,cv::BORDER_REPLICATE);
   theRegion.arrCorners=arrCorners;
   theRegion.bFound=true;
   return theRegion;
}
