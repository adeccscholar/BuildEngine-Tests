#pragma once

#include <SDL.h>
#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <cstdint>
#include <stdexcept>

/**
\brief Native SDL2 streaming viewport for real OpenCV camera pixels.
\details A shared video display primitive for BuildEngine SDL2/ImGui
   showcases. Pixel data is uploaded as a streaming SDL texture rather than
   painted one rectangle per pixel. The texture is released before the
   owning NativeShowcase renderer is destroyed.
*/
class CameraFrameView final {
   SDL_Renderer* pRenderer {};
   SDL_Texture* pTexture {};
   int iWidth {};
   int iHeight {};

public:
   /**
   \brief Acquires the SDL2 renderer of a native showcase window.
   \details No separate renderer, window or camera is created.
   \param pWindow Existing live SDL2 window.
   \throw std::runtime_error When the showcase has no SDL2 renderer.
   */
   explicit CameraFrameView(SDL_Window* const pWindow)
      : pRenderer(SDL_GetRenderer(pWindow)) {
      if(!pRenderer) throw std::runtime_error("SDL2 showcase renderer missing");
   }

   CameraFrameView(CameraFrameView const&)=delete;
   CameraFrameView& operator=(CameraFrameView const&)=delete;

   ~CameraFrameView() {
      if(pTexture) SDL_DestroyTexture(pTexture);
   }

   /**
   \brief Uploads the latest captured frame.
   \details Accepts 8-bit grayscale, BGR and BGRA OpenCV images.
   \param theFrame Current frame from the native videoio backend.
   \throw std::runtime_error On unsupported input or SDL transfer failure.
   */
   void Update(cv::Mat const& theFrame) {
      if(theFrame.empty()) return;
      cv::Mat theBGRA;
      if(theFrame.type()==CV_8UC3)
         cv::cvtColor(theFrame,theBGRA,cv::COLOR_BGR2BGRA);
      else if(theFrame.type()==CV_8UC1)
         cv::cvtColor(theFrame,theBGRA,cv::COLOR_GRAY2BGRA);
      else if(theFrame.type()==CV_8UC4)
         theBGRA=theFrame;
      else
         throw std::runtime_error("Unsupported camera frame pixel layout");
      if(iWidth!=theBGRA.cols || iHeight!=theBGRA.rows) {
         if(pTexture) SDL_DestroyTexture(pTexture);
         pTexture=SDL_CreateTexture(pRenderer,SDL_PIXELFORMAT_BGRA32,
            SDL_TEXTUREACCESS_STREAMING,theBGRA.cols,theBGRA.rows);
         if(!pTexture) throw std::runtime_error(SDL_GetError());
         iWidth=theBGRA.cols;
         iHeight=theBGRA.rows;
      }
      if(SDL_UpdateTexture(pTexture,nullptr,theBGRA.data,
         static_cast<int>(theBGRA.step))!=0)
         throw std::runtime_error(SDL_GetError());
   }

   /**
   \brief Draws the latest frame inside ImGui without stretching it.
   \details The original image aspect ratio is preserved.
   \param flMaxWidth Available ImGui viewport width.
   \param flMaxHeight Available ImGui viewport height.
   \throw Nothing intentionally.
   */
   void Draw(float const flMaxWidth,float const flMaxHeight) const {
      if(!pTexture || iWidth<=0 || iHeight<=0) {
         ImGui::TextDisabled("No camera frame is available.");
         return;
      }
      float const flScale=std::min(flMaxWidth/iWidth,flMaxHeight/iHeight);
      ImGui::Image(static_cast<ImTextureID>(
         reinterpret_cast<std::intptr_t>(pTexture)),
         ImVec2{std::max(1.0F,flScale*iWidth),
                std::max(1.0F,flScale*iHeight)});
   }
};

/**
\brief Explicitly selected DirectShow source using managed OpenCV videoio.
\details No device is opened by the constructor or device enumeration.
   A source is opened only on a deliberate operator action.
*/
class DirectShowCapture final {
   cv::VideoCapture theCapture;
   int iIndex {-1};

public:
   [[nodiscard]] bool Connected() const { return theCapture.isOpened(); }
   [[nodiscard]] int Index() const noexcept { return iIndex; }

   /**
   \brief Opens one operator-selected DirectShow capture index.
   \details Existing capture resources are released before connection.
   \param iDeviceIndex Index from the current DirectShow device enumeration.
   \returns True only when OpenCV successfully opens the source.
   \throw cv::Exception On a native driver failure.
   */
   bool Connect(int const iDeviceIndex,int const iRequestedWidth=0,
      int const iRequestedHeight=0) {
      Disconnect();
      if(iDeviceIndex<0) return false;
      if(!theCapture.open(iDeviceIndex,cv::CAP_DSHOW)) return false;
      if(iRequestedWidth>0 && iRequestedHeight>0) {
         // Device negotiation may select a different supported mode. Always
         // report actual captured frame dimensions instead of claiming 4K.
         theCapture.set(cv::CAP_PROP_FRAME_WIDTH,iRequestedWidth);
         theCapture.set(cv::CAP_PROP_FRAME_HEIGHT,iRequestedHeight);
      }
      iIndex=iDeviceIndex;
      return true;
   }

   /** \brief Releases the selected camera without affecting other devices. */
   void Disconnect() {
      theCapture.release();
      iIndex=-1;
   }

   /**
   \brief Reads the next real video frame.
   \param theFrame Receives the OpenCV image.
   \returns True only when a nonempty frame was obtained.
   \throw cv::Exception On a native capture failure.
   */
   bool Read(cv::Mat& theFrame) {
      return Connected() && theCapture.read(theFrame) && !theFrame.empty();
   }
};
