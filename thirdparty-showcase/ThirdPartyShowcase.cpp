// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file ThirdPartyShowcase.cpp
\brief VCL application entry point for the BCC64X third-party showcase.
*/

#include <vcl.h>
#pragma hdrstop

#include <windows.h>

#include "MainForm.h"

#pragma argsused
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
   try {
      Application->Initialize();
      Application->MainFormOnTaskBar = true;
      Application->Title = L"adecc Scholar - BCC64X Third-Party Showcase";
      Application->CreateForm(__classid(TMainForm), &MainForm);
      Application->Run();
      }
   catch (Exception& theException) {
      Application->ShowException(&theException);
      return 1;
      }
   catch (...) {
      try {
         throw Exception(L"Unbekannte Ausnahme im Third-Party Showcase");
         }
      catch (Exception& theException) {
         Application->ShowException(&theException);
         }
      return 1;
      }

   return 0;
   }
