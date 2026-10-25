// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Project: adecc Scholar

#include <windows.h>
#include <wincodec.h>
#include <objbase.h>

#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTileMode.h"
#include "include/effects/SkGradient.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

namespace
{
   constexpr float g_fDesignWidth = 2048.0f;
   constexpr float g_fDesignHeight = 1150.0f;
   constexpr UINT_PTR g_uiAnimationTimer = 1;
   constexpr UINT g_uiAnimationIntervalMs = 16;
   constexpr UINT g_uiStressFrameMessage = WM_APP + 1U;
   constexpr std::size_t g_uiSceneCount = 7;
   constexpr std::array<std::size_t, 5> g_StressDrawCounts { 2000U, 5000U, 10000U, 20000U, 40000U };
   constexpr std::size_t g_uiDefaultStressLevel = 3U;

   enum class Scene : std::size_t
   {
      Images = 0,
      Primitives,
      Paths,
      Gradients,
      TransformsAndClip,
      BlendModes,
      Stress
   };

   struct ImagePlacement
   {
      float fX;
      float fY;
      float fWidth;
      float fHeight;
   };

   using GlyphRows = std::array<std::uint8_t, 7>;

   GlyphRows GlyphFor(char chValue)
   {
      switch(chValue)
      {
         case '0': return { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E };
         case '1': return { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E };
         case '2': return { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F };
         case '3': return { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E };
         case '4': return { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 };
         case '5': return { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E };
         case '6': return { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E };
         case '7': return { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 };
         case '8': return { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E };
         case '9': return { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E };
         case 'A': return { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
         case 'D': return { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E };
         case 'E': return { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F };
         case 'F': return { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 };
         case 'M': return { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 };
         case 'N': return { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 };
         case 'P': return { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 };
         case 'R': return { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 };
         case 'S': return { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E };
         case 'U': return { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
         case 'W': return { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A };
         case '.': return { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06 };
         case ':': return { 0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00 };
         case '-': return { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 };
         default:  return { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      }
   }


   std::size_t AppendText(char* pBuffer, std::size_t uiCapacity,
      std::size_t uiPosition, std::string_view strText)
   {
      for(char const chValue : strText)
      {
         if(uiPosition + 1U >= uiCapacity)
         {
            break;
         }
         pBuffer[uiPosition++] = chValue;
      }
      return uiPosition;
   }

   std::size_t AppendUnsigned(char* pBuffer, std::size_t uiCapacity,
      std::size_t uiPosition, std::uint64_t uiValue)
   {
      char szDigits[24] {};
      std::size_t uiDigitCount = 0U;
      do
      {
         szDigits[uiDigitCount++] = static_cast<char>('0' + (uiValue % 10U));
         uiValue /= 10U;
      }
      while(uiValue != 0U && uiDigitCount < sizeof(szDigits));

      while(uiDigitCount > 0U && uiPosition + 1U < uiCapacity)
      {
         pBuffer[uiPosition++] = szDigits[--uiDigitCount];
      }
      return uiPosition;
   }

   std::size_t FormatFixedMetric(char* pBuffer, std::size_t uiCapacity,
      std::string_view strLabel, double dValue, unsigned int uiDecimals)
   {
      if(uiCapacity == 0U)
      {
         return 0U;
      }

      if(dValue < 0.0)
      {
         dValue = 0.0;
      }

      std::uint64_t uiScale = 1U;
      for(unsigned int uiIndex = 0; uiIndex < uiDecimals; ++uiIndex)
      {
         uiScale *= 10U;
      }

      std::uint64_t const uiScaled = static_cast<std::uint64_t>(
         dValue * static_cast<double>(uiScale) + 0.5);
      std::uint64_t const uiInteger = uiScaled / uiScale;
      std::uint64_t const uiFraction = uiScaled % uiScale;

      std::size_t uiPosition = AppendText(pBuffer, uiCapacity, 0U, strLabel);
      uiPosition = AppendUnsigned(pBuffer, uiCapacity, uiPosition, uiInteger);
      if(uiDecimals > 0U && uiPosition + 1U < uiCapacity)
      {
         pBuffer[uiPosition++] = '.';
         std::uint64_t uiDivisor = uiScale / 10U;
         while(uiDivisor > 0U && uiPosition + 1U < uiCapacity)
         {
            pBuffer[uiPosition++] = static_cast<char>('0' + ((uiFraction / uiDivisor) % 10U));
            uiDivisor /= 10U;
         }
      }
      pBuffer[uiPosition] = '\0';
      return uiPosition;
   }

   std::size_t FormatUnsignedMetric(char* pBuffer, std::size_t uiCapacity,
      std::string_view strLabel, std::uint64_t uiValue)
   {
      if(uiCapacity == 0U)
      {
         return 0U;
      }

      std::size_t uiPosition = AppendText(pBuffer, uiCapacity, 0U, strLabel);
      uiPosition = AppendUnsigned(pBuffer, uiCapacity, uiPosition, uiValue);
      pBuffer[uiPosition] = '\0';
      return uiPosition;
   }

   void DrawBitmapText(SkCanvas* pCanvas, std::string_view strText,
      float fX, float fY, float fPixelSize, SkColor Color)
   {
      if(pCanvas == nullptr)
      {
         return;
      }

      SkPaint Paint;
      Paint.setAntiAlias(false);
      Paint.setColor(Color);

      float fCursorX = fX;
      for(char const chValue : strText)
      {
         if(chValue == ' ')
         {
            fCursorX += fPixelSize * 4.0f;
            continue;
         }

         GlyphRows const Rows = GlyphFor(chValue);
         for(std::size_t uiRow = 0; uiRow < Rows.size(); ++uiRow)
         {
            for(std::size_t uiColumn = 0; uiColumn < 5U; ++uiColumn)
            {
               std::uint8_t const uiMask = static_cast<std::uint8_t>(1U << (4U - uiColumn));
               if((Rows[uiRow] & uiMask) != 0U)
               {
                  pCanvas->drawRect(SkRect::MakeXYWH(
                     fCursorX + static_cast<float>(uiColumn) * fPixelSize,
                     fY + static_cast<float>(uiRow) * fPixelSize,
                     fPixelSize, fPixelSize), Paint);
               }
            }
         }

         fCursorX += fPixelSize * 6.0f;
      }
   }

   // Positions reconstructed from the supplied 2048 x 1150 control image.
   constexpr ImagePlacement g_BackgroundPlacement { 0.0f, 0.0f, 2048.0f, 1150.0f };
   constexpr ImagePlacement g_LogoPlacement       { 137.0f, 423.0f, 518.0f, 347.0f };
   constexpr ImagePlacement g_BuilderPlacement    { 562.0f, 110.0f, 1193.0f, 796.0f };
   constexpr ImagePlacement g_CommunityPlacement  { 621.0f, 321.0f, 1078.0f, 719.0f };

   template <typename T>
   class ComPtr final
   {
   public:
      ComPtr() = default;

      ~ComPtr()
      {
         Reset();
      }

      ComPtr(ComPtr const&) = delete;
      ComPtr& operator=(ComPtr const&) = delete;

      ComPtr(ComPtr&& theOther) noexcept
         : m_pValue(std::exchange(theOther.m_pValue, nullptr))
      {
      }

      ComPtr& operator=(ComPtr&& theOther) noexcept
      {
         if(this != &theOther)
         {
            Reset();
            m_pValue = std::exchange(theOther.m_pValue, nullptr);
         }
         return *this;
      }

      T* Get() const noexcept
      {
         return m_pValue;
      }

      T** Put() noexcept
      {
         Reset();
         return &m_pValue;
      }

      void Reset() noexcept
      {
         if(m_pValue != nullptr)
         {
            m_pValue->Release();
            m_pValue = nullptr;
         }
      }

      T* operator->() const noexcept
      {
         return m_pValue;
      }

      explicit operator bool() const noexcept
      {
         return m_pValue != nullptr;
      }

   private:
      T* m_pValue = nullptr;
   };

   struct DemoImages
   {
      sk_sp<SkImage> spBackground;
      sk_sp<SkImage> spLogo;
      sk_sp<SkImage> spBuilder;
      sk_sp<SkImage> spCommunity;
   };

   std::wstring GetExecutableDirectory()
   {
      std::array<wchar_t, 32768> wszBuffer {};
      DWORD const dwLength = GetModuleFileNameW(nullptr, wszBuffer.data(),
         static_cast<DWORD>(wszBuffer.size()));
      if(dwLength == 0 || dwLength >= wszBuffer.size())
      {
         return {};
      }

      std::wstring strPath(wszBuffer.data(), dwLength);
      std::wstring::size_type const uiSlash = strPath.find_last_of(L"\\/");
      if(uiSlash == std::wstring::npos)
      {
         return L".";
      }
      return strPath.substr(0, uiSlash);
   }

   std::wstring MakeAssetPath(std::wstring const& strExecutableDirectory,
      wchar_t const* wszFileName)
   {
      return strExecutableDirectory + L"\\assets\\" + wszFileName;
   }

   sk_sp<SkImage> LoadPngAsSkImage(IWICImagingFactory* pFactory,
      std::wstring const& strPath)
   {
      if(pFactory == nullptr)
      {
         return nullptr;
      }

      ComPtr<IWICBitmapDecoder> pDecoder;
      HRESULT hResult = pFactory->CreateDecoderFromFilename(strPath.c_str(), nullptr,
         GENERIC_READ, WICDecodeMetadataCacheOnLoad, pDecoder.Put());
      if(FAILED(hResult))
      {
         return nullptr;
      }

      ComPtr<IWICBitmapFrameDecode> pFrame;
      hResult = pDecoder->GetFrame(0, pFrame.Put());
      if(FAILED(hResult))
      {
         return nullptr;
      }

      UINT uiWidth = 0;
      UINT uiHeight = 0;
      hResult = pFrame->GetSize(&uiWidth, &uiHeight);
      if(FAILED(hResult) || uiWidth == 0 || uiHeight == 0)
      {
         return nullptr;
      }

      ComPtr<IWICFormatConverter> pConverter;
      hResult = pFactory->CreateFormatConverter(pConverter.Put());
      if(FAILED(hResult))
      {
         return nullptr;
      }

      hResult = pConverter->Initialize(pFrame.Get(), GUID_WICPixelFormat32bppPBGRA,
         WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
      if(FAILED(hResult))
      {
         return nullptr;
      }

      std::size_t const uiStride = static_cast<std::size_t>(uiWidth) * 4U;
      std::size_t const uiByteCount = uiStride * static_cast<std::size_t>(uiHeight);
      if(uiByteCount > static_cast<std::size_t>(std::numeric_limits<UINT>::max()))
      {
         return nullptr;
      }

      std::vector<std::uint8_t> vPixels(uiByteCount);
      hResult = pConverter->CopyPixels(nullptr, static_cast<UINT>(uiStride),
         static_cast<UINT>(uiByteCount), vPixels.data());
      if(FAILED(hResult))
      {
         return nullptr;
      }

      SkImageInfo const ImageInfo = SkImageInfo::MakeN32Premul(
         static_cast<int>(uiWidth), static_cast<int>(uiHeight));
      SkPixmap const Pixmap(ImageInfo, vPixels.data(), uiStride);
      return SkImages::RasterFromPixmapCopy(Pixmap);
   }

   void DrawPlacedImage(SkCanvas* pCanvas, sk_sp<SkImage> const& spImage,
      ImagePlacement const& Placement, SkPaint const* pPaint = nullptr)
   {
      if(pCanvas == nullptr || !spImage)
      {
         return;
      }

      float const fScaleX = Placement.fWidth / static_cast<float>(spImage->width());
      float const fScaleY = Placement.fHeight / static_cast<float>(spImage->height());

      pCanvas->save();
      pCanvas->translate(Placement.fX, Placement.fY);
      pCanvas->scale(fScaleX, fScaleY);
      pCanvas->drawImage(spImage, 0.0f, 0.0f,
         SkSamplingOptions(SkFilterMode::kLinear), pPaint);
      pCanvas->restore();
   }

   void DrawSceneIndicator(SkCanvas* pCanvas, Scene SceneValue)
   {
      SkPaint Paint;
      Paint.setAntiAlias(true);
      Paint.setColor(SkColorSetARGB(180, 4, 8, 18));
      pCanvas->drawRoundRect(SkRect::MakeXYWH(24.0f, 24.0f, 302.0f, 48.0f),
         18.0f, 18.0f, Paint);

      std::size_t const uiSelected = static_cast<std::size_t>(SceneValue);
      for(std::size_t uiIndex = 0; uiIndex < g_uiSceneCount; ++uiIndex)
      {
         bool const bSelected = uiIndex == uiSelected;
         Paint.setStyle(bSelected ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
         Paint.setStrokeWidth(bSelected ? 1.0f : 3.0f);
         Paint.setColor(bSelected ? SkColorSetARGB(255, 0, 205, 255)
                                  : SkColorSetARGB(210, 185, 195, 225));
         pCanvas->drawCircle(54.0f + static_cast<float>(uiIndex) * 40.0f,
            48.0f, bSelected ? 11.0f : 9.0f, Paint);
      }
   }

   class SkiaShowcase final
   {
   public:
      bool LoadImages()
      {
         ComPtr<IWICImagingFactory> pFactory;
         HRESULT const hResult = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
            reinterpret_cast<void**>(pFactory.Put()));
         if(FAILED(hResult))
         {
            return false;
         }

         std::wstring const strDirectory = GetExecutableDirectory();
         if(strDirectory.empty())
         {
            return false;
         }

         m_Images.spBackground = LoadPngAsSkImage(pFactory.Get(),
            MakeAssetPath(strDirectory, L"Hintergrund.png"));
         m_Images.spBuilder = LoadPngAsSkImage(pFactory.Get(),
            MakeAssetPath(strDirectory, L"C++Builder13.png"));
         m_Images.spLogo = LoadPngAsSkImage(pFactory.Get(),
            MakeAssetPath(strDirectory, L"CB-Logo.png"));
         m_Images.spCommunity = LoadPngAsSkImage(pFactory.Get(),
            MakeAssetPath(strDirectory, L"Community Edition.png"));

         return m_Images.spBackground && m_Images.spBuilder &&
            m_Images.spLogo && m_Images.spCommunity;
      }

      int Run(HINSTANCE hInstance, int nCmdShow)
      {
         (void)nCmdShow;

         LARGE_INTEGER Frequency {};
         if(QueryPerformanceFrequency(&Frequency) != FALSE && Frequency.QuadPart > 0)
         {
            m_i64PerformanceFrequency = Frequency.QuadPart;
         }
         wchar_t const* wszClassName = L"adecc.SkiaDirectShowcase";

         WNDCLASSEXW WindowClass {};
         WindowClass.cbSize = sizeof(WindowClass);
         WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
         WindowClass.lpfnWndProc = &SkiaShowcase::WindowProc;
         WindowClass.hInstance = hInstance;
         WindowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
         WindowClass.lpszClassName = wszClassName;

         if(RegisterClassExW(&WindowClass) == 0)
         {
            return 20;
         }

         POINT const PrimaryPoint { 0, 0 };
         HMONITOR const hMonitor = MonitorFromPoint(PrimaryPoint, MONITOR_DEFAULTTOPRIMARY);
         MONITORINFO MonitorInfo {};
         MonitorInfo.cbSize = sizeof(MonitorInfo);
         if(GetMonitorInfoW(hMonitor, &MonitorInfo) == FALSE)
         {
            UnregisterClassW(wszClassName, hInstance);
            return 21;
         }

         RECT const& MonitorRect = MonitorInfo.rcMonitor;
         int const iWidth = MonitorRect.right - MonitorRect.left;
         int const iHeight = MonitorRect.bottom - MonitorRect.top;

         HWND const hWindow = CreateWindowExW(WS_EX_APPWINDOW, wszClassName,
            L"Skia Direct BCC64X Raster Tests", WS_POPUP,
            MonitorRect.left, MonitorRect.top, iWidth, iHeight,
            nullptr, nullptr, hInstance, this);
         if(hWindow == nullptr)
         {
            UnregisterClassW(wszClassName, hInstance);
            return 22;
         }

         ShowWindow(hWindow, SW_SHOW);
         UpdateWindow(hWindow);
         SetForegroundWindow(hWindow);
         SetFocus(hWindow);

         MSG Message {};
         while(GetMessageW(&Message, nullptr, 0, 0) > 0)
         {
            TranslateMessage(&Message);
            DispatchMessageW(&Message);
         }

         UnregisterClassW(wszClassName, hInstance);
         return static_cast<int>(Message.wParam);
      }

   private:
      void RenderImages(SkCanvas* pCanvas)
      {
         DrawPlacedImage(pCanvas, m_Images.spBackground, g_BackgroundPlacement);
         DrawPlacedImage(pCanvas, m_Images.spLogo, g_LogoPlacement);
         DrawPlacedImage(pCanvas, m_Images.spBuilder, g_BuilderPlacement);
         DrawPlacedImage(pCanvas, m_Images.spCommunity, g_CommunityPlacement);
      }

      void RenderPrimitives(SkCanvas* pCanvas)
      {
         pCanvas->clear(SkColorSetRGB(8, 12, 28));

         SkPaint Paint;
         Paint.setAntiAlias(true);
         Paint.setColor(SkColorSetRGB(0, 185, 255));
         pCanvas->drawRect(SkRect::MakeXYWH(170.0f, 180.0f, 430.0f, 260.0f), Paint);

         Paint.setColor(SkColorSetRGB(255, 32, 165));
         pCanvas->drawRoundRect(SkRect::MakeXYWH(690.0f, 180.0f, 470.0f, 260.0f),
            64.0f, 64.0f, Paint);

         Paint.setColor(SkColorSetRGB(255, 205, 50));
         pCanvas->drawCircle(1450.0f, 310.0f, 150.0f, Paint);

         Paint.setStyle(SkPaint::kStroke_Style);
         Paint.setStrokeWidth(28.0f);
         Paint.setStrokeCap(SkPaint::kRound_Cap);
         Paint.setColor(SkColorSetRGB(110, 255, 155));
         pCanvas->drawLine(190.0f, 620.0f, 900.0f, 940.0f, Paint);

         Paint.setStrokeWidth(12.0f);
         Paint.setColor(SkColorSetRGB(175, 105, 255));
         pCanvas->drawOval(SkRect::MakeXYWH(1060.0f, 610.0f, 690.0f, 330.0f), Paint);

         Paint.setAntiAlias(false);
         Paint.setStrokeWidth(5.0f);
         Paint.setColor(SK_ColorWHITE);
         for(int iLine = 0; iLine < 9; ++iLine)
         {
            float const fY = 520.0f + static_cast<float>(iLine) * 38.0f;
            pCanvas->drawLine(180.0f, fY, 820.0f, fY, Paint);
         }
      }

      void RenderPaths(SkCanvas* pCanvas)
      {
         pCanvas->clear(SkColorSetRGB(11, 9, 26));

         SkPathBuilder WaveBuilder;
         WaveBuilder.moveTo(120.0f, 640.0f)
            .cubicTo(360.0f, 160.0f, 620.0f, 1080.0f, 900.0f, 560.0f)
            .cubicTo(1180.0f, 60.0f, 1450.0f, 1040.0f, 1920.0f, 430.0f)
            .lineTo(1920.0f, 980.0f)
            .lineTo(120.0f, 980.0f)
            .close();
         SkPath const WavePath = WaveBuilder.detach();

         SkPaint FillPaint;
         FillPaint.setAntiAlias(true);
         FillPaint.setColor(SkColorSetARGB(220, 0, 170, 255));
         pCanvas->drawPath(WavePath, FillPaint);

         SkPathBuilder RibbonBuilder;
         RibbonBuilder.moveTo(160.0f, 720.0f)
            .quadTo(450.0f, 280.0f, 760.0f, 690.0f)
            .quadTo(1060.0f, 1060.0f, 1370.0f, 590.0f)
            .quadTo(1630.0f, 230.0f, 1900.0f, 690.0f);
         SkPath const RibbonPath = RibbonBuilder.detach();

         SkPaint StrokePaint;
         StrokePaint.setAntiAlias(true);
         StrokePaint.setStyle(SkPaint::kStroke_Style);
         StrokePaint.setStrokeWidth(34.0f);
         StrokePaint.setStrokeCap(SkPaint::kRound_Cap);
         StrokePaint.setColor(SkColorSetRGB(255, 35, 185));
         pCanvas->drawPath(RibbonPath, StrokePaint);

         SkPathBuilder EvenOddBuilder(SkPathFillType::kEvenOdd);
         EvenOddBuilder.addCircle(540.0f, 350.0f, 190.0f)
            .addCircle(540.0f, 350.0f, 95.0f);
         SkPath const EvenOddPath = EvenOddBuilder.detach();
         FillPaint.setColor(SkColorSetRGB(255, 210, 60));
         pCanvas->drawPath(EvenOddPath, FillPaint);

         SkPathBuilder BezierBuilder;
         BezierBuilder.moveTo(1050.0f, 360.0f)
            .cubicTo(1180.0f, 80.0f, 1500.0f, 80.0f, 1640.0f, 360.0f)
            .cubicTo(1510.0f, 620.0f, 1180.0f, 620.0f, 1050.0f, 360.0f)
            .close();
         SkPath const BezierPath = BezierBuilder.detach();
         FillPaint.setColor(SkColorSetARGB(230, 120, 90, 255));
         pCanvas->drawPath(BezierPath, FillPaint);
      }

      void RenderGradients(SkCanvas* pCanvas)
      {
         pCanvas->clear(SkColorSetRGB(6, 8, 20));

         SkPoint const LinearPoints[2] = {
            SkPoint::Make(130.0f, 190.0f),
            SkPoint::Make(1910.0f, 190.0f)
         };
         SkColor4f const LinearColors[5] = {
            SkColor4f::FromColor(SkColorSetRGB(0, 205, 255)),
            SkColor4f::FromColor(SkColorSetRGB(30, 90, 255)),
            SkColor4f::FromColor(SkColorSetRGB(170, 55, 255)),
            SkColor4f::FromColor(SkColorSetRGB(255, 25, 175)),
            SkColor4f::FromColor(SkColorSetRGB(255, 95, 55))
         };
         SkScalar const LinearStops[5] = { 0.0f, 0.22f, 0.48f, 0.73f, 1.0f };

         SkPaint Paint;
         Paint.setAntiAlias(true);
         Paint.setShader(SkShaders::LinearGradient(LinearPoints,
            {{LinearColors, LinearStops, SkTileMode::kClamp}, {}}));
         pCanvas->drawRoundRect(SkRect::MakeXYWH(130.0f, 190.0f, 1780.0f, 270.0f),
            82.0f, 82.0f, Paint);

         SkColor4f const RadialColors[4] = {
            SkColor4f::FromColor(SkColorSetARGB(255, 255, 255, 255)),
            SkColor4f::FromColor(SkColorSetARGB(255, 0, 210, 255)),
            SkColor4f::FromColor(SkColorSetARGB(230, 80, 40, 220)),
            SkColor4f::FromColor(SkColorSetARGB(0, 10, 8, 30))
         };
         SkScalar const RadialStops[4] = { 0.0f, 0.28f, 0.62f, 1.0f };
         Paint.setShader(SkShaders::RadialGradient(SkPoint::Make(600.0f, 780.0f),
            360.0f, {{RadialColors, RadialStops, SkTileMode::kClamp}, {}}));
         pCanvas->drawCircle(600.0f, 780.0f, 360.0f, Paint);

         SkPoint const RepeatPoints[2] = {
            SkPoint::Make(1050.0f, 590.0f),
            SkPoint::Make(1380.0f, 930.0f)
         };
         SkColor4f const RepeatColors[3] = {
            SkColor4f::FromColor(SkColorSetRGB(255, 35, 175)),
            SkColor4f::FromColor(SkColorSetRGB(20, 225, 255)),
            SkColor4f::FromColor(SkColorSetRGB(255, 215, 50))
         };
         SkScalar const RepeatStops[3] = { 0.0f, 0.5f, 1.0f };
         Paint.setShader(SkShaders::LinearGradient(RepeatPoints,
            {{RepeatColors, RepeatStops, SkTileMode::kMirror}, {}}));
         pCanvas->drawRoundRect(SkRect::MakeXYWH(1080.0f, 610.0f, 720.0f, 360.0f),
            46.0f, 46.0f, Paint);
      }

      void RenderTransformsAndClip(SkCanvas* pCanvas, float fAngle)
      {
         pCanvas->clear(SkColorSetRGB(7, 11, 25));

         SkPaint BackdropPaint;
         BackdropPaint.setColor(SkColorSetRGB(18, 38, 75));
         pCanvas->drawRoundRect(SkRect::MakeXYWH(170.0f, 160.0f, 1708.0f, 830.0f),
            72.0f, 72.0f, BackdropPaint);

         pCanvas->save();
         pCanvas->clipRect(SkRect::MakeXYWH(215.0f, 205.0f, 1618.0f, 740.0f));

         for(int iItem = 0; iItem < 7; ++iItem)
         {
            float const fX = 390.0f + static_cast<float>(iItem % 4) * 410.0f;
            float const fY = 390.0f + static_cast<float>(iItem / 4) * 390.0f;
            float const fScale = 0.42f + static_cast<float>(iItem) * 0.055f;
            float const fRotation = fAngle + static_cast<float>(iItem) * 21.0f;

            pCanvas->save();
            pCanvas->translate(fX, fY);
            pCanvas->rotate(fRotation);
            pCanvas->scale(fScale, fScale);
            pCanvas->drawImage(m_Images.spLogo,
               -static_cast<float>(m_Images.spLogo->width()) * 0.5f,
               -static_cast<float>(m_Images.spLogo->height()) * 0.5f,
               SkSamplingOptions(SkFilterMode::kLinear), nullptr);
            pCanvas->restore();
         }

         pCanvas->restore();

         SkPaint BorderPaint;
         BorderPaint.setAntiAlias(true);
         BorderPaint.setStyle(SkPaint::kStroke_Style);
         BorderPaint.setStrokeWidth(8.0f);
         BorderPaint.setColor(SkColorSetRGB(0, 205, 255));
         pCanvas->drawRoundRect(SkRect::MakeXYWH(170.0f, 160.0f, 1708.0f, 830.0f),
            72.0f, 72.0f, BorderPaint);
      }

      void RenderBlendModes(SkCanvas* pCanvas)
      {
         pCanvas->clear(SkColorSetRGB(9, 10, 24));

         constexpr std::array<SkBlendMode, 6> BlendModes {
            SkBlendMode::kSrcOver,
            SkBlendMode::kMultiply,
            SkBlendMode::kScreen,
            SkBlendMode::kOverlay,
            SkBlendMode::kPlus,
            SkBlendMode::kDifference
         };

         SkPaint BasePaint;
         BasePaint.setAntiAlias(true);
         SkPaint BlendPaint;
         BlendPaint.setAntiAlias(true);

         for(std::size_t uiIndex = 0; uiIndex < BlendModes.size(); ++uiIndex)
         {
            std::size_t const uiColumn = uiIndex % 3;
            std::size_t const uiRow = uiIndex / 3;
            float const fX = 180.0f + static_cast<float>(uiColumn) * 610.0f;
            float const fY = 185.0f + static_cast<float>(uiRow) * 455.0f;

            BasePaint.setColor(SkColorSetRGB(28, 42, 76));
            pCanvas->drawRoundRect(SkRect::MakeXYWH(fX, fY, 500.0f, 360.0f),
               42.0f, 42.0f, BasePaint);

            BasePaint.setColor(SkColorSetARGB(235, 0, 205, 255));
            pCanvas->drawCircle(fX + 195.0f, fY + 178.0f, 125.0f, BasePaint);

            BlendPaint.setBlendMode(BlendModes[uiIndex]);
            BlendPaint.setColor(SkColorSetARGB(225, 255, 30, 175));
            pCanvas->drawCircle(fX + 305.0f, fY + 178.0f, 125.0f, BlendPaint);

            BasePaint.setStyle(SkPaint::kStroke_Style);
            BasePaint.setStrokeWidth(5.0f);
            BasePaint.setColor(SkColorSetARGB(210, 225, 230, 255));
            pCanvas->drawRoundRect(SkRect::MakeXYWH(fX, fY, 500.0f, 360.0f),
               42.0f, 42.0f, BasePaint);
            BasePaint.setStyle(SkPaint::kFill_Style);
         }
      }

      void RenderStress(SkCanvas* pCanvas, float fAngle)
      {
         pCanvas->clear(SkColorSetRGB(5, 7, 16));

         std::size_t const uiDrawCalls = g_StressDrawCounts[m_uiStressLevel];
         std::uint32_t const uiPhase = static_cast<std::uint32_t>(fAngle * 4096.0f);

         SkPaint Paint;
         Paint.setAntiAlias(true);
         Paint.setStyle(SkPaint::kFill_Style);

         for(std::size_t uiIndex = 0; uiIndex < uiDrawCalls; ++uiIndex)
         {
            std::uint32_t const uiSeed = static_cast<std::uint32_t>(uiIndex) * 2654435761U + uiPhase;
            float const fX = 28.0f + static_cast<float>((uiSeed >> 1U) % 1980U);
            float const fY = 92.0f + static_cast<float>((uiSeed >> 12U) % 1010U);
            float const fSize = 4.0f + static_cast<float>((uiSeed >> 24U) & 31U);
            SkColor const Color = SkColorSetARGB(
               70U + ((uiSeed >> 25U) & 0x7FU),
               (uiSeed >> 16U) & 0xFFU,
               (uiSeed >> 8U) & 0xFFU,
               uiSeed & 0xFFU);
            Paint.setColor(Color);

            switch(uiIndex & 3U)
            {
               case 0U:
                  pCanvas->drawRect(SkRect::MakeXYWH(fX, fY, fSize * 1.7f, fSize), Paint);
                  break;
               case 1U:
                  pCanvas->drawCircle(fX, fY, fSize * 0.62f, Paint);
                  break;
               case 2U:
                  Paint.setStyle(SkPaint::kStroke_Style);
                  Paint.setStrokeWidth(1.0f + static_cast<float>((uiSeed >> 20U) & 7U));
                  pCanvas->drawLine(fX, fY,
                     fX + static_cast<float>((uiSeed >> 7U) & 63U) - 31.0f,
                     fY + static_cast<float>((uiSeed >> 14U) & 63U) - 31.0f, Paint);
                  Paint.setStyle(SkPaint::kFill_Style);
                  break;
               default:
                  pCanvas->drawRoundRect(SkRect::MakeXYWH(fX, fY, fSize * 1.5f, fSize * 0.9f),
                     fSize * 0.25f, fSize * 0.25f, Paint);
                  break;
            }
         }

         SkPaint PanelPaint;
         PanelPaint.setAntiAlias(true);
         PanelPaint.setColor(SkColorSetARGB(220, 4, 8, 18));
         pCanvas->drawRoundRect(SkRect::MakeXYWH(110.0f, 820.0f, 970.0f, 250.0f),
            28.0f, 28.0f, PanelPaint);

         char szLine[96] {};
         std::size_t uiLineLength = FormatFixedMetric(szLine, sizeof(szLine),
            "FPS ", m_dFpsSmoothed, 1U);
         DrawBitmapText(pCanvas, std::string_view(szLine, uiLineLength),
            145.0f, 858.0f, 7.0f, SK_ColorWHITE);

         uiLineLength = FormatFixedMetric(szLine, sizeof(szLine),
            "MS ", m_dRenderMsSmoothed, 2U);
         DrawBitmapText(pCanvas, std::string_view(szLine, uiLineLength),
            145.0f, 922.0f, 6.0f, SkColorSetRGB(0, 205, 255));

         uiLineLength = FormatUnsignedMetric(szLine, sizeof(szLine),
            "DRAWS ", static_cast<std::uint64_t>(uiDrawCalls));
         DrawBitmapText(pCanvas, std::string_view(szLine, uiLineLength),
            520.0f, 858.0f, 6.0f, SkColorSetRGB(255, 210, 60));
         DrawBitmapText(pCanvas, m_bAnimate ? "RUN" : "PAUSE",
            520.0f, 922.0f, 6.0f, m_bAnimate ? SkColorSetRGB(100, 255, 150)
                                             : SkColorSetRGB(255, 100, 120));
      }

      void RenderScene(SkCanvas* pCanvas, float fAngle)
      {
         switch(m_Scene)
         {
            case Scene::Images:
               RenderImages(pCanvas);
               break;
            case Scene::Primitives:
               RenderPrimitives(pCanvas);
               break;
            case Scene::Paths:
               RenderPaths(pCanvas);
               break;
            case Scene::Gradients:
               RenderGradients(pCanvas);
               break;
            case Scene::TransformsAndClip:
               RenderTransformsAndClip(pCanvas, fAngle);
               break;
            case Scene::BlendModes:
               RenderBlendModes(pCanvas);
               break;
            case Scene::Stress:
               RenderStress(pCanvas, fAngle);
               break;
         }

         DrawSceneIndicator(pCanvas, m_Scene);
      }

      void Paint(HWND hWindow, HDC hDeviceContext)
      {
         LARGE_INTEGER FrameStart {};
         QueryPerformanceCounter(&FrameStart);
         UpdateFps(FrameStart.QuadPart);

         RECT ClientRect {};
         if(GetClientRect(hWindow, &ClientRect) == FALSE)
         {
            return;
         }

         int const iWidth = ClientRect.right - ClientRect.left;
         int const iHeight = ClientRect.bottom - ClientRect.top;
         if(iWidth <= 0 || iHeight <= 0)
         {
            return;
         }

         SkImageInfo const SurfaceInfo = SkImageInfo::MakeN32Premul(iWidth, iHeight);
         sk_sp<SkSurface> const spSurface = SkSurfaces::Raster(SurfaceInfo);
         if(!spSurface)
         {
            return;
         }

         SkCanvas* const pCanvas = spSurface->getCanvas();
         pCanvas->clear(SK_ColorBLACK);

         float const fFitScale = std::min(static_cast<float>(iWidth) / g_fDesignWidth,
            static_cast<float>(iHeight) / g_fDesignHeight);
         float const fScale = m_bFit ? fFitScale : 1.0f;
         float const fOffsetX = (static_cast<float>(iWidth) - g_fDesignWidth * fScale) * 0.5f;
         float const fOffsetY = (static_cast<float>(iHeight) - g_fDesignHeight * fScale) * 0.5f;
         float const fAngle = m_bAnimate
            ? static_cast<float>(GetTickCount64() % 12000ULL) * (360.0f / 12000.0f)
            : 18.0f;

         pCanvas->save();
         pCanvas->translate(fOffsetX, fOffsetY);
         pCanvas->scale(fScale, fScale);
         RenderScene(pCanvas, fAngle);
         pCanvas->restore();

         LARGE_INTEGER RenderEnd {};
         QueryPerformanceCounter(&RenderEnd);
         UpdateRenderTime(FrameStart.QuadPart, RenderEnd.QuadPart);

         SkPixmap Pixmap;
         if(!spSurface->peekPixels(&Pixmap))
         {
            return;
         }

         std::size_t const uiTightStride = static_cast<std::size_t>(iWidth) * 4U;
         void const* pBits = Pixmap.addr();
         if(Pixmap.rowBytes() != uiTightStride)
         {
            m_vTightPixels.resize(uiTightStride * static_cast<std::size_t>(iHeight));
            auto const* pSource = static_cast<std::uint8_t const*>(Pixmap.addr());
            for(int iRow = 0; iRow < iHeight; ++iRow)
            {
               std::memcpy(m_vTightPixels.data() + uiTightStride * static_cast<std::size_t>(iRow),
                  pSource + Pixmap.rowBytes() * static_cast<std::size_t>(iRow), uiTightStride);
            }
            pBits = m_vTightPixels.data();
         }

         BITMAPINFO BitmapInfo {};
         BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
         BitmapInfo.bmiHeader.biWidth = iWidth;
         BitmapInfo.bmiHeader.biHeight = -iHeight;
         BitmapInfo.bmiHeader.biPlanes = 1;
         BitmapInfo.bmiHeader.biBitCount = 32;
         BitmapInfo.bmiHeader.biCompression = BI_RGB;

         StretchDIBits(hDeviceContext, 0, 0, iWidth, iHeight,
            0, 0, iWidth, iHeight, pBits, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);

         if(m_Scene == Scene::Stress && m_bAnimate)
         {
            PostMessageW(hWindow, g_uiStressFrameMessage, 0, 0);
         }
      }

      void ResetPerformanceMetrics()
      {
         m_i64LastFrameStart = 0;
         m_dFpsSmoothed = 0.0;
         m_dRenderMsSmoothed = 0.0;
         m_uiPerformanceSamples = 0U;
      }

      void UpdateFps(LONGLONG i64FrameStart)
      {
         if(m_i64PerformanceFrequency <= 0)
         {
            return;
         }

         if(m_i64LastFrameStart != 0 && i64FrameStart > m_i64LastFrameStart)
         {
            double const dSeconds = static_cast<double>(i64FrameStart - m_i64LastFrameStart) /
               static_cast<double>(m_i64PerformanceFrequency);
            double const dFps = dSeconds > 0.0 ? 1.0 / dSeconds : 0.0;
            double const dAlpha = m_uiPerformanceSamples < 10U ? 0.35 : 0.10;
            m_dFpsSmoothed = m_uiPerformanceSamples == 0U
               ? dFps
               : m_dFpsSmoothed + (dFps - m_dFpsSmoothed) * dAlpha;
            ++m_uiPerformanceSamples;
         }

         m_i64LastFrameStart = i64FrameStart;
      }

      void UpdateRenderTime(LONGLONG i64FrameStart, LONGLONG i64RenderEnd)
      {
         if(m_i64PerformanceFrequency <= 0 || i64RenderEnd <= i64FrameStart)
         {
            return;
         }

         double const dMilliseconds = static_cast<double>(i64RenderEnd - i64FrameStart) * 1000.0 /
            static_cast<double>(m_i64PerformanceFrequency);
         double const dAlpha = m_uiPerformanceSamples < 10U ? 0.35 : 0.10;
         m_dRenderMsSmoothed = m_dRenderMsSmoothed == 0.0
            ? dMilliseconds
            : m_dRenderMsSmoothed + (dMilliseconds - m_dRenderMsSmoothed) * dAlpha;
      }

      void UpdateAnimationDriver(HWND hWindow)
      {
         KillTimer(hWindow, g_uiAnimationTimer);
         if(!m_bAnimate)
         {
            return;
         }

         if(m_Scene == Scene::Stress)
         {
            InvalidateRect(hWindow, nullptr, FALSE);
         }
         else
         {
            SetTimer(hWindow, g_uiAnimationTimer, g_uiAnimationIntervalMs, nullptr);
         }
      }

      void SelectScene(HWND hWindow, std::size_t uiScene)
      {
         Scene const PreviousSceneValue = m_Scene;
         m_Scene = static_cast<Scene>(uiScene % g_uiSceneCount);

         if(m_Scene == Scene::Stress)
         {
            m_bAnimate = true;
            ResetPerformanceMetrics();
         }
         else if(PreviousSceneValue == Scene::Stress)
         {
            m_bAnimate = false;
         }

         UpdateAnimationDriver(hWindow);
         InvalidateRect(hWindow, nullptr, FALSE);
      }

      void PreviousScene(HWND hWindow)
      {
         std::size_t const uiCurrent = static_cast<std::size_t>(m_Scene);
         SelectScene(hWindow, (uiCurrent + g_uiSceneCount - 1U) % g_uiSceneCount);
      }

      void NextScene(HWND hWindow)
      {
         std::size_t const uiCurrent = static_cast<std::size_t>(m_Scene);
         SelectScene(hWindow, (uiCurrent + 1U) % g_uiSceneCount);
      }

      void ToggleAnimation(HWND hWindow)
      {
         m_bAnimate = !m_bAnimate;
         if(m_Scene == Scene::Stress)
         {
            ResetPerformanceMetrics();
         }
         UpdateAnimationDriver(hWindow);
         InvalidateRect(hWindow, nullptr, FALSE);
      }

      void AdjustStressLoad(HWND hWindow, int iDirection)
      {
         if(m_Scene != Scene::Stress)
         {
            return;
         }

         if(iDirection > 0 && m_uiStressLevel + 1U < g_StressDrawCounts.size())
         {
            ++m_uiStressLevel;
         }
         else if(iDirection < 0 && m_uiStressLevel > 0U)
         {
            --m_uiStressLevel;
         }

         ResetPerformanceMetrics();
         InvalidateRect(hWindow, nullptr, FALSE);
      }

      void Reset(HWND hWindow)
      {
         m_Scene = Scene::Images;
         m_bFit = true;
         m_bAnimate = false;
         m_uiStressLevel = g_uiDefaultStressLevel;
         ResetPerformanceMetrics();
         KillTimer(hWindow, g_uiAnimationTimer);
         InvalidateRect(hWindow, nullptr, FALSE);
      }

      void HandleKey(HWND hWindow, WPARAM wParam)
      {
         switch(wParam)
         {
            case VK_ESCAPE:
               DestroyWindow(hWindow);
               break;
            case VK_LEFT:
               PreviousScene(hWindow);
               break;
            case VK_RIGHT:
               NextScene(hWindow);
               break;
            case VK_UP:
               AdjustStressLoad(hWindow, 1);
               break;
            case VK_DOWN:
               AdjustStressLoad(hWindow, -1);
               break;
            case 'F':
               m_bFit = !m_bFit;
               InvalidateRect(hWindow, nullptr, FALSE);
               break;
            case VK_SPACE:
               ToggleAnimation(hWindow);
               break;
            case 'R':
               Reset(hWindow);
               break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
               SelectScene(hWindow, static_cast<std::size_t>(wParam - '1'));
               break;
         }
      }

      static LRESULT CALLBACK WindowProc(HWND hWindow, UINT uiMessage,
         WPARAM wParam, LPARAM lParam)
      {
         SkiaShowcase* pThis = reinterpret_cast<SkiaShowcase*>(
            GetWindowLongPtrW(hWindow, GWLP_USERDATA));

         if(uiMessage == WM_NCCREATE)
         {
            auto const* pCreate = reinterpret_cast<CREATESTRUCTW const*>(lParam);
            pThis = static_cast<SkiaShowcase*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hWindow, GWLP_USERDATA,
               reinterpret_cast<LONG_PTR>(pThis));
         }

         switch(uiMessage)
         {
            case WM_ERASEBKGND:
               return 1;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
               if(pThis != nullptr)
               {
                  pThis->HandleKey(hWindow, wParam);
                  return 0;
               }
               break;

            case g_uiStressFrameMessage:
               if(pThis != nullptr && pThis->m_Scene == Scene::Stress && pThis->m_bAnimate)
               {
                  InvalidateRect(hWindow, nullptr, FALSE);
                  return 0;
               }
               break;

            case WM_TIMER:
               if(pThis != nullptr && wParam == g_uiAnimationTimer)
               {
                  InvalidateRect(hWindow, nullptr, FALSE);
                  return 0;
               }
               break;

            case WM_SIZE:
               InvalidateRect(hWindow, nullptr, FALSE);
               return 0;

            case WM_PAINT:
               if(pThis != nullptr)
               {
                  PAINTSTRUCT PaintStruct {};
                  HDC const hDeviceContext = BeginPaint(hWindow, &PaintStruct);
                  pThis->Paint(hWindow, hDeviceContext);
                  EndPaint(hWindow, &PaintStruct);
                  return 0;
               }
               break;

            case WM_CLOSE:
               DestroyWindow(hWindow);
               return 0;

            case WM_DESTROY:
               KillTimer(hWindow, g_uiAnimationTimer);
               PostQuitMessage(0);
               return 0;
         }

         return DefWindowProcW(hWindow, uiMessage, wParam, lParam);
      }

      DemoImages m_Images;
      Scene m_Scene = Scene::Images;
      bool m_bFit = true;
      bool m_bAnimate = false;
      std::size_t m_uiStressLevel = g_uiDefaultStressLevel;
      LONGLONG m_i64PerformanceFrequency = 1;
      LONGLONG m_i64LastFrameStart = 0;
      double m_dFpsSmoothed = 0.0;
      double m_dRenderMsSmoothed = 0.0;
      std::size_t m_uiPerformanceSamples = 0U;
      std::vector<std::uint8_t> m_vTightPixels;
   };
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance,
   LPSTR pszCommandLine, int nCmdShow)
{
   (void)hPreviousInstance;
   (void)pszCommandLine;

   SetProcessDPIAware();

   HRESULT const hComResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
   bool const bComInitialized = SUCCEEDED(hComResult);
   if(!bComInitialized && hComResult != RPC_E_CHANGED_MODE)
   {
      return 10;
   }

   SkiaShowcase Showcase;
   int iResult = 11;
   if(Showcase.LoadImages())
   {
      iResult = Showcase.Run(hInstance, nCmdShow);
   }

   if(bComInitialized)
   {
      CoUninitialize();
   }
   return iResult;
}
