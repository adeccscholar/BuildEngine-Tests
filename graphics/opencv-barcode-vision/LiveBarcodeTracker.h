#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

/**
\brief Tracks one confirmed barcode image patch without decoding it again.
\details After an initial successful OpenCV decode the original grayscale
   region becomes a template. Subsequent frames use OpenCV matchTemplate in
   a bounded search window around the last position. New decoding is allowed
   only after consecutive failed matches prove the code left the image.
   Tracking is visual evidence, not a repeated barcode detection.
\note Scale, perspective or major rotation changes can terminate tracking;
   they never create a false confirmed decode.
*/
class LiveBarcodeTracker final {
   cv::Mat theTemplate;
   cv::Rect theBox {};
   int iMissed {};
   static constexpr int iLostThreshold=5;
   static constexpr double flMatchThreshold=0.72;

   [[nodiscard]] static cv::Mat MakeGray(cv::Mat const& theImage) {
      cv::Mat theGray;
      if(theImage.channels()==3)
         cv::cvtColor(theImage,theGray,cv::COLOR_BGR2GRAY);
      else if(theImage.channels()==4)
         cv::cvtColor(theImage,theGray,cv::COLOR_BGRA2GRAY);
      else if(theImage.channels()==1)
         theGray=theImage;
      else
         throw std::runtime_error("Unsupported camera image for tracking");
      return theGray;
   }

public:
   /**
   \brief Starts tracking from a confirmed OpenCV detector quadrilateral.
   \details The bounding rectangle includes the original barcode texture;
      tracking is rejected if the detected box is too small or outside frame.
   \param theImage Source frame used for actual decoding.
   \param arrCorners Four corners returned by OpenCV.
   \returns True if a sufficient source template was captured.
   \throw cv::Exception On invalid image data.
   */
   bool Acquire(cv::Mat const& theImage,
      std::array<cv::Point2f,4> const& arrCorners) {
      cv::Rect theRect=cv::boundingRect(std::vector<cv::Point2f>{
         arrCorners.begin(),arrCorners.end()});
      theRect &= cv::Rect{0,0,theImage.cols,theImage.rows};
      if(theRect.width<24 || theRect.height<24) return false;
      cv::Mat const theGray=MakeGray(theImage);
      // Include a narrow environmental border to improve template stability.
      int const iPadding=std::max(4,std::min(theRect.width,theRect.height)/12);
      theRect=cv::Rect{
         theRect.x-iPadding,theRect.y-iPadding,
         theRect.width+2*iPadding,theRect.height+2*iPadding
      } & cv::Rect{0,0,theImage.cols,theImage.rows};
      theTemplate=theGray(theRect).clone();
      theBox=theRect;
      iMissed=0;
      return true;
   }

   /** \brief Clears visual tracking and permits a new decode. */
   void Reset() {
      theTemplate.release();
      theBox={};
      iMissed=0;
   }

   [[nodiscard]] bool Active() const noexcept {
      return !theTemplate.empty();
   }
   [[nodiscard]] cv::Rect Box() const noexcept {
      return theBox;
   }

   /**
   \brief Advances visual tracking without invoking a barcode decoder.
   \details A local normalized template match moves the region with the
      camera. Five consecutive missing matches release this code. A single
      weak frame does not create an artificial disappearance.
   \param theFrame Current BGR or grayscale camera image.
   \returns True while the known code is still tracked or within miss grace.
   \throw cv::Exception On OpenCV template matching failure.
   */
   bool Update(cv::Mat const& theFrame) {
      if(!Active()) return false;
      cv::Mat const theGray=MakeGray(theFrame);
      cv::Rect const aBounds{0,0,theGray.cols,theGray.rows};
      int const iMargin=std::max(32,
         std::max(theBox.width,theBox.height)/2);
      cv::Rect const aSearch{
         theBox.x-iMargin,theBox.y-iMargin,
         theBox.width+2*iMargin,theBox.height+2*iMargin
      };
      cv::Rect const aROI=aSearch & aBounds;
      bool bMatched=false;
      if(aROI.width>=theTemplate.cols && aROI.height>=theTemplate.rows) {
         cv::Mat theScores;
         cv::matchTemplate(theGray(aROI),theTemplate,
            theScores,cv::TM_CCOEFF_NORMED);
         double flMax {};
         cv::Point aLocation {};
         cv::minMaxLoc(theScores,nullptr,&flMax,nullptr,&aLocation);
         if(std::isfinite(flMax) && flMax>=flMatchThreshold) {
            theBox=cv::Rect{
               aROI.x+aLocation.x,aROI.y+aLocation.y,
               theTemplate.cols,theTemplate.rows
            };
            iMissed=0;
            bMatched=true;
         }
      }
      if(!bMatched && ++iMissed>=iLostThreshold) {
         Reset();
         return false;
      }
      return true;
   }
};
