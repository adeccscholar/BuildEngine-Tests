// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include <vcl.h>
#pragma hdrstop
#include "MainForm.h"
#include <curl/curl.h>
#include <tchar.h>
#include <exception>

int WINAPI _tWinMain(HINSTANCE,HINSTANCE,LPTSTR,int) {
   if(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
      ::MessageBoxW(nullptr,L"libcurl konnte nicht initialisiert werden.",
         L"WeatherHist",MB_ICONERROR|MB_OK);
      return 1;
      }
   int iReturn{};
   try {
      Application->Initialize();
      Application->MainFormOnTaskBar = true;
      Application->CreateForm(__classid(TWeatherHistForm), &WeatherHistForm);
      Application->Run();
      }
   catch(Exception& theError) {
      Application->ShowException(&theError);
      iReturn = 1;
      }
   catch(std::exception const&) {
      ::MessageBoxW(nullptr,L"WeatherHist: unbehandelter Laufzeitfehler.",
         L"WeatherHist",MB_ICONERROR|MB_OK);
      iReturn = 1;
      }
   curl_global_cleanup();
   return iReturn;
   }
