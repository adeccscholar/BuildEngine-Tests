#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

/**
\brief Corrects an operator-estimated vertical bow in a photographed book page.
\details Builds a genuine floating-point OpenCV inverse mapping for cv::remap.
   The horizontal camera coordinate is unchanged. The signed curvature
   parameter gives the vertical baseline displacement at the page centre as
   a fraction of the input height. The model deliberately does not claim to
   recover a three-dimensional page surface or flattened line spacing.
\param thePage Original or perspective-rectified document image.
\param flCurvature Signed vertical bow relative to page height, [-0.18, 0.18].
\returns A new geometrically dewarped document image.
\throw std::invalid_argument When the page is empty or curvature is not finite.
\note Apply before thresholding or OCR, not to a reduced preview.
*/
[[nodiscard]] inline cv::Mat DewarpBookPage(
   cv::Mat const& thePage, float const flCurvature) {
   if(thePage.empty() || !std::isfinite(flCurvature))
      throw std::invalid_argument {"Invalid book-page dewarping input"};
   if(std::abs(flCurvature) < 0.00001F)
      return thePage.clone();

   float const flAmplitude = std::clamp(flCurvature, -0.18F, 0.18F) *
      static_cast<float>(thePage.rows);
   cv::Mat theMapX(thePage.size(), CV_32FC1);
   cv::Mat theMapY(thePage.size(), CV_32FC1);
   for(int iX = 0; iX < thePage.cols; ++iX) {
      float const flNormalizedX = static_cast<float>(iX) /
         static_cast<float>(std::max(1, thePage.cols - 1));
      float const flOffset = flAmplitude *
         std::sin(std::numbers::pi_v<float> * flNormalizedX);
      for(int iY = 0; iY < thePage.rows; ++iY) {
         theMapX.at<float>(iY, iX) = static_cast<float>(iX);
         theMapY.at<float>(iY, iX) = static_cast<float>(iY) + flOffset;
      }
   }
   cv::Mat theOutput;
   cv::remap(thePage, theOutput, theMapX, theMapY,
      cv::INTER_LINEAR, cv::BORDER_REPLICATE);
   return theOutput;
}

/**
\brief Builds a deterministic curved page fixture for testing the correction.
\details A straight, synthetic ink baseline is displaced by the same analytic
   signed bow used by DewarpBookPage, with no OCR text substitution.
\param iWidth Fixture width in pixels.
\param iHeight Fixture height in pixels.
\param flCurvature Signed vertical curvature relative to height.
\returns A grayscale image with one synthetic curved black baseline.
\throw std::invalid_argument If dimensions are too small.
*/
[[nodiscard]] inline cv::Mat MakeCurvedBookFixture(
   int const iWidth, int const iHeight, float const flCurvature) {
   if(iWidth < 16 || iHeight < 16)
      throw std::invalid_argument {"Curved fixture dimensions are too small"};
   cv::Mat theImage(iHeight, iWidth, CV_8UC1, cv::Scalar {255});
   int const iBase = iHeight / 2;
   for(int iX = 0; iX < iWidth; ++iX) {
      float const flX = static_cast<float>(iX) /
         static_cast<float>(iWidth - 1);
      int const iY = iBase + static_cast<int>(std::round(
         flCurvature * iHeight * std::sin(std::numbers::pi_v<float> * flX)));
      if(iY >= 2 && iY + 2 < iHeight) {
         for(int iDelta = -2; iDelta <= 2; ++iDelta)
            theImage.at<unsigned char>(iY + iDelta, iX) = 0;
      }
   }
   return theImage;
}

/**
\brief Measures the maximum signed ink displacement of a synthetic baseline.
\details Used only by the independent geometry regression; a production page
   requires text-line detection or operator-guided visual judgement.
\param theImage Grayscale fixture image.
\returns Maximum absolute detected baseline deviation from image centre.
\throw std::invalid_argument If the image is empty.
*/
[[nodiscard]] inline int MeasureBookFixtureBow(cv::Mat const& theImage) {
   if(theImage.empty() || theImage.type() != CV_8UC1)
      throw std::invalid_argument {"Expected grayscale fixture"};
   int iMaximum {};
   for(int iX = 0; iX < theImage.cols; iX += 8) {
      int iBest = theImage.rows / 2;
      int iInk = 256;
      for(int iY = theImage.rows / 4; iY < (3 * theImage.rows) / 4; ++iY) {
         int const iPixel = theImage.at<unsigned char>(iY, iX);
         if(iPixel < iInk) {
            iInk = iPixel;
            iBest = iY;
         }
      }
      iMaximum = std::max(iMaximum, std::abs(iBest - theImage.rows / 2));
   }
   return iMaximum;
}
