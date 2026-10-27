// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include "HistoryNetwork.h"
#include <System.SysUtils.hpp>
#include <windows.h>
#include <exception>
#include <format>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#pragma package(smart_init)
#pragma resource "*.dfm"

TWeatherHistForm* WeatherHistForm = nullptr;

namespace {

/** \brief Convert a VCL UTF-16 string to UTF-8.
\details Windows conversion is strict and does not substitute code points.
\param strText UTF-16 text.
\returns UTF-8 string.
\throw std::runtime_error On a Windows character conversion failure.
*/
[[nodiscard]] std::string ToUtf8(UnicodeString const& strText) {
   if(strText.IsEmpty()) {
      return {};
      }
   int const iNeeded = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
      strText.c_str(), strText.Length(), nullptr, 0, nullptr, nullptr);
   if(iNeeded <= 0) {
      throw std::runtime_error{"Cannot convert text to UTF-8"};
      }
   std::string strResult(static_cast<std::size_t>(iNeeded), '\0');
   if(::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
      strText.c_str(), strText.Length(), strResult.data(), iNeeded,
      nullptr, nullptr) != iNeeded) {
      throw std::runtime_error{"UTF-8 conversion failed"};
      }
   return strResult;
   }

/** \brief Decode an UTF-8 string for VCL controls.
\details Avoids interpreting Unicode service errors as ANSI text.
\param strText UTF-8 source.
\returns UnicodeString suitable for VCL.
\throw std::runtime_error If UTF-8 is invalid.
*/
[[nodiscard]] UnicodeString ToUnicode(std::string const& strText) {
   if(strText.empty()) {
      return UnicodeString{};
      }
   int const iNeeded = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
      strText.data(), static_cast<int>(strText.size()), nullptr, 0);
   if(iNeeded <= 0) {
      throw std::runtime_error{"Invalid UTF-8 response text"};
      }
   std::wstring strWide(static_cast<std::size_t>(iNeeded), L'\0');
   if(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
      strText.data(), static_cast<int>(strText.size()),
      strWide.data(), iNeeded) != iNeeded) {
      throw std::runtime_error{"UTF-8 response decoding failed"};
      }
   return UnicodeString{strWide.c_str()};
   }
} // namespace

__fastcall TWeatherHistForm::TWeatherHistForm(TComponent* aOwner)
   : TForm(aOwner) {
   resultTimer_->Enabled = false;
   statusLabel_->Caption = L"Adresse und Datum eingeben, dann Abrufen waehlen.";
   }

__fastcall TWeatherHistForm::~TWeatherHistForm() {
   resultTimer_->Enabled = false;
   // The thread owns only a shared worker state; joining avoids shutdown races
   // with curl_global_cleanup at application termination.
   if(m_aWorker.joinable()) {
      m_aWorker.join();
      }
   m_pTask.reset();
   }

void TWeatherHistForm::DisplayReport(weather_hist::Report const& theReport) {
   resultsList_->Items->BeginUpdate();
   try {
      resultsList_->Items->Clear();
      for(weather_hist::Metric const& aMetric : theReport.vecMetrics) {
         TListItem* const pItem = resultsList_->Items->Add();
         pItem->Caption = ToUnicode(aMetric.strLabel);
         pItem->SubItems->Add(ToUnicode(aMetric.strValue));
         }
      }
   catch(...) {
      resultsList_->Items->EndUpdate();
      throw;
      }
   resultsList_->Items->EndUpdate();
   timelineList_->Items->BeginUpdate();
   try {
      timelineList_->Items->Clear();
      for(weather_hist::TimelineRow const& aRow : theReport.vecTimeline) {
         TListItem* const pItem=timelineList_->Items->Add();
         pItem->Caption=ToUnicode(aRow.strTime);
         for(std::string const& strValue : aRow.arValues) {
            pItem->SubItems->Add(ToUnicode(strValue));
            }
         }
      }
   catch(...) {
      timelineList_->Items->EndUpdate();
      throw;
      }
   timelineList_->Items->EndUpdate();
   sourceLabel_->Caption = ToUnicode(std::format(
      "{} ({:.5f}, {:.5f})",theReport.strAddress,
      theReport.flLatitude,theReport.flLongitude));
   }

void __fastcall TWeatherHistForm::LoadClick(TObject*) {
   if(resultTimer_->Enabled) {
      return;
      }
   try {
      std::string const strAddress = ToUtf8(addressEdit_->Text);
      std::string const strDate = weather_hist::ParseDate(ToUtf8(dateEdit_->Text));
      if(strAddress.empty() || strAddress.size() > 300U) {
         throw std::invalid_argument{"Bitte eine Adresse (maximal 300 Byte) eingeben."};
         }
      // Assigning to a completed jthread joins it before the next request.
      std::shared_ptr<TaskState> const pTask = std::make_shared<TaskState>();
      m_pTask = pTask;
      m_aWorker = std::jthread([pTask,strAddress,strDate](std::stop_token) -> void {
         try {
            weather_hist::Report theReport =
               weather_hist::FetchHistory(strAddress,strDate);
            std::scoped_lock aGuard{pTask->aMutex};
            pTask->theReport = std::move(theReport);
            pTask->bCompleted = true;
            }
         catch(std::exception const& aError) {
            std::scoped_lock aGuard{pTask->aMutex};
            pTask->strError = aError.what();
            pTask->bCompleted = true;
            }
         catch(...) {
            std::scoped_lock aGuard{pTask->aMutex};
            pTask->strError = "Unbekannter Fehler im Netzwerk-Thread.";
            pTask->bCompleted = true;
            }
         });
      resultsList_->Items->Clear();
      timelineList_->Items->Clear();
      sourceLabel_->Caption = L"";
      statusLabel_->Caption = L"Adresse wird aufgeloest und das historische Wetter abgerufen...";
      loadButton_->Enabled = false;
      resultTimer_->Enabled = true;
      }
   catch(std::exception const& aError) {
      statusLabel_->Caption = ToUnicode(aError.what());
      loadButton_->Enabled = true;
      }
   }

void __fastcall TWeatherHistForm::ResultTick(TObject*) {
   if(!m_pTask) {
      resultTimer_->Enabled = false;
      loadButton_->Enabled = true;
      return;
      }
   std::optional<weather_hist::Report> theReport;
   std::string strError;
   {
      std::scoped_lock aGuard{m_pTask->aMutex};
      if(!m_pTask->bCompleted) {
         return;
         }
      theReport = std::move(m_pTask->theReport);
      strError = std::move(m_pTask->strError);
   }
   resultTimer_->Enabled = false;
   loadButton_->Enabled = true;
   try {
      if(!strError.empty()) {
         statusLabel_->Caption = ToUnicode(strError);
         return;
         }
      if(!theReport) {
         throw std::runtime_error{"Leere Antwort des Wetterdienstes."};
         }
      DisplayReport(*theReport);
      statusLabel_->Caption =
         L"Open-Meteo: historische Reanalyse; UV ggf. Prognosearchiv. "
         L"Standortdaten: OpenStreetMap / Nominatim.";
      }
   catch(std::exception const& aError) {
      statusLabel_->Caption = ToUnicode(aError.what());
      }
   }
