// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
\file MainForm.cpp
\brief VCL dashboard implementation for the BCC64X third-party showcase.
*/

#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"

#include <System.SysUtils.hpp>
#include <Winapi.ShellAPI.hpp>
#include <Winapi.Windows.hpp>

#include <array>
#include <exception>
#include <format>
#include <limits>
#include <stdexcept>

#pragma package(smart_init)
#pragma resource "*.dfm"

TMainForm* MainForm = nullptr;

__fastcall TMainForm::TMainForm(TComponent* Owner)
   : TForm(Owner) {
   }

void __fastcall TMainForm::FormShow(TObject*) {
   ResetDashboard();
   }

UnicodeString TMainForm::ToUnicode(std::string_view svText) const {
   if (svText.empty()) {
      return UnicodeString();
      }

   // ShowcaseEngine/std::format/nlohmann::json supply UTF-8 byte strings.
   // Decode bytes explicitly: UTF8String(char*) can otherwise depend on the
   // source/runtime codepage and corrupt German umlauts in the VCL UI.
   if (svText.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
      throw std::length_error("UTF-8 text exceeds the Windows conversion limit.");
      }
   int const iBytes = static_cast<int>(svText.size());
   int const iCharacters = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, svText.data(), iBytes, nullptr, 0);
   if (iCharacters == 0) {
      throw std::runtime_error("Invalid UTF-8 text for VCL display.");
      }
   std::wstring strUnicode(static_cast<std::size_t>(iCharacters), L'\0');
   if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, svText.data(),
       iBytes, strUnicode.data(), iCharacters) != iCharacters) {
      throw std::runtime_error("UTF-8 conversion for VCL display failed.");
      }
   return UnicodeString(strUnicode.c_str(), iCharacters);
   }

int TMainForm::RowForComponent(std::string_view svComponent) const noexcept {
   if (svComponent == "Boost.Beast") {
      return 1;
      }
   else if (svComponent == "OpenSSL") {
      return 2;
      }
   else if (svComponent == "libcurl") {
      return 3;
      }
   else if (svComponent == "nlohmann/json") {
      return 4;
      }
   else if (svComponent == "zlib") {
      return 5;
      }
   else if (svComponent == "libzip") {
      return 6;
      }

   return 0;
   }

void TMainForm::ResetDashboard() {
   StatusGrid->ColCount = 5;
   StatusGrid->RowCount = 7;
   StatusGrid->FixedRows = 1;

   StatusGrid->Cells[0][0] = L"Bibliothek";
   StatusGrid->Cells[1][0] = L"Aufgabe";
   StatusGrid->Cells[2][0] = L"Status";
   StatusGrid->Cells[3][0] = L"Ergebnis";
   StatusGrid->Cells[4][0] = L"Details";

   std::array<UnicodeString, 6> const arrComponents {
      L"Boost.Beast", L"OpenSSL", L"libcurl", L"nlohmann/json", L"zlib", L"libzip"
      };

   for (int iRow = 1; iRow <= 6; ++iRow) {
      StatusGrid->Cells[0][iRow] = arrComponents[static_cast<std::size_t>(iRow - 1)];
      StatusGrid->Cells[1][iRow] = L"wartet";
      StatusGrid->Cells[2][iRow] = L"READY";
      StatusGrid->Cells[3][iRow] = L"";
      StatusGrid->Cells[4][iRow] = L"";
      }

   StatusGrid->ColWidths[0] = 120;
   StatusGrid->ColWidths[1] = 285;
   StatusGrid->ColWidths[2] = 80;
   StatusGrid->ColWidths[3] = 220;
   StatusGrid->ColWidths[4] = 340;

   ProgressBar->Min = 0;
   ProgressBar->Max = 6;
   ProgressBar->Position = 0;
   iCompletedRows = 0;

   JsonMemo->Lines->Clear();
   JsonMemo->Lines->Add(L"Nach erfolgreichem Lauf erscheint hier die von nlohmann/json erzeugte Zusammenfassung.");

   OpenResultButton->Enabled = false;
   theArchivePath.clear();
   StatusBar->SimpleText = L"Bereit. Die Demo f\u00fchrt zwei reale HTTPS Requests \u00fcber unabh\u00e4ngige Stacks aus.";
   }

void TMainForm::ApplyUpdate(adecc::showcase::Showcase_Update const& theUpdate) {
   int const iRow = RowForComponent(theUpdate.strComponent);
   if (iRow == 0) {
      return;
      }

   UnicodeString const strOldStatus = StatusGrid->Cells[2][iRow];
   StatusGrid->Cells[1][iRow] = ToUnicode(theUpdate.strTask);
   StatusGrid->Cells[2][iRow] = ToUnicode(theUpdate.strStatus);
   StatusGrid->Cells[3][iRow] = ToUnicode(theUpdate.strResult);
   StatusGrid->Cells[4][iRow] = ToUnicode(theUpdate.strDetails);

   if (theUpdate.strStatus == "OK" && strOldStatus != L"OK") {
      ++iCompletedRows;
      ProgressBar->Position = iCompletedRows;
      }

   StatusBar->SimpleText = ToUnicode(std::format("{}: {}", theUpdate.strComponent,
                                                 theUpdate.strStatus));
   Application->ProcessMessages();
   }

std::filesystem::path TMainForm::OutputDirectory() const {
   std::array<wchar_t, 32768> wszModule {};
   DWORD const uLength = GetModuleFileNameW(nullptr, wszModule.data(),
                                            static_cast<DWORD>(wszModule.size()));
   if (uLength == 0 || uLength >= wszModule.size()) {
      throw std::runtime_error("Cannot determine executable path.");
      }

   return std::filesystem::path { std::wstring { wszModule.data(), uLength } }
      .parent_path() / L"showcase-output";
   }

void __fastcall TMainForm::RunButtonClick(TObject*) {
   RunButton->Enabled = false;
   OpenResultButton->Enabled = false;
   Screen->Cursor = crHourGlass;
   ResetDashboard();
   RunButton->Enabled = false;

   try {
      adecc::showcase::Showcase_Result const theResult = adecc::showcase::RunShowcase(
         OutputDirectory(),
         [this](adecc::showcase::Showcase_Update const& theUpdate) {
            ApplyUpdate(theUpdate);
            });

      theArchivePath = theResult.theArchivePath;
      JsonMemo->Lines->Text = ToUnicode(theResult.strSummaryJson);
      OpenResultButton->Enabled = true;
      StatusBar->SimpleText = UnicodeString(std::format(
         L"Fertig: {} Bibliotheksstufen gr\u00fcn, ZIP mit {} Eintr\u00e4gen verifiziert.",
         iCompletedRows, theResult.uArchiveEntries).c_str());
      }
   catch (std::exception const& theException) {
      StatusBar->SimpleText = L"FEHLER: Showcase wurde abgebrochen.";
      MessageDlg(ToUnicode(std::format("Showcase fehlgeschlagen:\r\n\r\n{}",
                                       theException.what())),
                 mtError, TMsgDlgButtons() << mbOK, 0);
      }
   catch (...) {
      StatusBar->SimpleText = L"FEHLER: unbekannte Ausnahme.";
      MessageDlg(L"Showcase fehlgeschlagen: unbekannte Ausnahme.",
                 mtError, TMsgDlgButtons() << mbOK, 0);
      }

   Screen->Cursor = crDefault;
   RunButton->Enabled = true;
   }

void __fastcall TMainForm::OpenResultButtonClick(TObject*) {
   if (theArchivePath.empty()) {
      return;
      }

   std::wstring const strArchive = theArchivePath.wstring();
   ShellExecuteW(Handle, L"open", L"explorer.exe",
                 std::format(L"/select,\"{}\"", strArchive).c_str(),
                 nullptr, SW_SHOWNORMAL);
   }
