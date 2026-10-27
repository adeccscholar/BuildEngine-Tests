// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once
/** \file MainForm.h
\brief WeatherHist native VCL form.
\details All VCL interaction remains on the main thread; HTTPS executes on a worker.
*/
#include "HistoryModel.h"
#include <vcl.h>
#include <Vcl.Forms.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ComCtrls.hpp>
#include <System.Classes.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

/** \brief WeatherHist: historical weather at a freely entered postal address.
\details A timer transfers results from the background worker to the VCL thread.
*/
class TWeatherHistForm final : public TForm {
   struct TaskState {
      std::mutex aMutex;
      bool bCompleted{};
      std::optional<weather_hist::Report> theReport;
      std::string strError;
   };

   std::shared_ptr<TaskState> m_pTask;
   std::jthread m_aWorker;

   /** \brief Present the report in the list.
   \details All strings are UTF-8 and must be decoded for the VCL.
   \param theReport Result from the archive service.
   \returns Nothing.
   \throw Exception If a VCL control operation fails.
   */
   void DisplayReport(weather_hist::Report const& theReport);

__published:
   TPanel* topPanel_;
   TPanel* bottomPanel_;
   TLabel* addressLabel_;
   TLabel* dateLabel_;
   TLabel* sourceLabel_;
   TLabel* noteLabel_;
   TLabel* statusLabel_;
   TEdit* addressEdit_;
   TEdit* dateEdit_;
   TButton* loadButton_;
   TListView* resultsList_;
   TTimer* resultTimer_;

   /** \brief Start a user-requested historical lookup.
   \details Validates both inputs and starts a single background thread.
   \param aSender VCL sender.
   \returns Nothing.
   \throw None; errors are displayed in the status panel.
   */
   void __fastcall LoadClick(TObject* aSender);

   /** \brief Transfer finished worker data to VCL.
   \details No VCL object is ever touched from the background thread.
   \param aSender Timer sender.
   \returns Nothing.
   \throw None; display errors are reported in the status panel.
   */
   void __fastcall ResultTick(TObject* aSender);

public:
   /** \brief Create the form and initialize its result list.
   \details The DFM defines controls, captions, anchors and event bindings.
   \param aOwner Component owner.
   \returns An initialized form.
   \throw Exception On failed form initialization.
   */
   __fastcall TWeatherHistForm(TComponent* aOwner);

   /** \brief Stop polling and join an outstanding request.
   \details CURL timeouts bound the shutdown wait; no detached worker survives.
   \returns Nothing.
   \throw None.
   */
   __fastcall ~TWeatherHistForm();
};

extern TWeatherHistForm* WeatherHistForm;
