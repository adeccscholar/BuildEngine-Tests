// Copyright (c) 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include "MainForm.h"

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
   try {
      Application->Initialize();
      Application->MainFormOnTaskBar = true;
      Application->CreateForm(__classid(TViewerForm), &ViewerForm);
      Application->Run();
   }
   catch(Exception& error) {
      Application->ShowException(&error);
      return 1;
   }
   catch(std::exception const& error) {
      UnicodeString const message(error.what());
      Application->MessageBox(message.c_str(), L"PDFium / VCL", MB_OK | MB_ICONERROR);
      return 1;
   }
   return 0;
}
