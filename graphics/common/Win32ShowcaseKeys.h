// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include <windows.h>
#include <string>
#include <string_view>

namespace adecc::tests::win32 {

/**
\brief Handles common F1 and F11 interactions for a native Win32 showcase.
\details F1 displays a genuine native About dialog. F11 switches to the
         current monitor's fullscreen bounds and restores window placement
         and style on the next press. Key repeat is ignored. A showcase
         application owns one primary window per process.
\param hWindow Native application window.
\param uKey Windows virtual key code.
\param iKeyParameters WM_KEYDOWN lParam containing the repeat flag.
\param svApplication Human-readable application title.
\param svTechnology Technologies and concrete purpose of the application.
\returns True when a supported key was consumed.
\throw Nothing intentionally.
\note This helper does not alter any scene, renderer or simulation.
*/
[[nodiscard]] inline bool HandleShowcaseKey(
   HWND const hWindow, WPARAM const uKey, LPARAM const iKeyParameters,
   std::wstring_view const svApplication, std::wstring_view const svTechnology) {
   if(uKey != VK_F1 && uKey != VK_F11) return false;
   if((static_cast<unsigned long long>(iKeyParameters) & (1ULL << 30)) != 0)
      return true;
   if(uKey == VK_F1) {
      std::wstring strMessage { L"adecc Scholar | " };
      strMessage.append(svApplication);
      strMessage.append(L"\n\n");
      strMessage.append(svTechnology);
      strMessage.append(L"\n\nF1: About\nF11: Fullscreen / windowed");
      strMessage.append(L"\nDocumentation and package evidence: Description.md / SBOM");
      std::wstring const strTitle { svApplication };
      MessageBoxW(hWindow, strMessage.c_str(), strTitle.c_str(),
                  MB_OK | MB_ICONINFORMATION);
      return true;
   }

   struct WindowedState {
      WINDOWPLACEMENT aPlacement { sizeof(WINDOWPLACEMENT) };
      LONG_PTR iStyle {};
      bool bFullscreen {};
   };
   static WindowedState theState;
   if(!theState.bFullscreen) {
      WINDOWPLACEMENT aPlacement { sizeof(WINDOWPLACEMENT) };
      MONITORINFO aMonitor { sizeof(MONITORINFO) };
      if(!GetWindowPlacement(hWindow, &aPlacement) ||
         !GetMonitorInfoW(MonitorFromWindow(hWindow, MONITOR_DEFAULTTONEAREST),
                          &aMonitor))
         return true;
      theState.aPlacement = aPlacement;
      theState.iStyle = GetWindowLongPtrW(hWindow, GWL_STYLE);
      SetWindowLongPtrW(hWindow, GWL_STYLE,
         (theState.iStyle & ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW)) |
         static_cast<LONG_PTR>(WS_POPUP));
      RECT const& aRect = aMonitor.rcMonitor;
      SetWindowPos(hWindow, HWND_TOP, aRect.left, aRect.top,
                   aRect.right - aRect.left, aRect.bottom - aRect.top,
                   SWP_FRAMECHANGED | SWP_SHOWWINDOW);
      theState.bFullscreen = true;
   }
   else {
      SetWindowLongPtrW(hWindow, GWL_STYLE, theState.iStyle);
      SetWindowPlacement(hWindow, &theState.aPlacement);
      SetWindowPos(hWindow, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_FRAMECHANGED | SWP_SHOWWINDOW);
      theState.bFullscreen = false;
   }
   return true;
}

} // namespace adecc::tests::win32
