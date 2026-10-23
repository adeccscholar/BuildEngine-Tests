// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: MIT

/**
\file MainForm.h
\brief VCL main form for the BCC64X third-party showcase.
*/

#ifndef MainFormH
#define MainFormH

#include "ShowcaseEngine.h"

#include <System.Classes.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.StdCtrls.hpp>

#include <filesystem>
#include <string>
#include <string_view>

/**
\brief Visible VCL dashboard for the integrated third-party demonstration.
\details The form shows each library as a separate row while one complete workflow
         executes real HTTPS requests, JSON processing, zlib compression and ZIP archiving.
*/
class TMainForm : public TForm {
__published:
   TPanel* HeaderPanel;
   TLabel* TitleLabel;
   TLabel* SubtitleLabel;
   TButton* RunButton;
   TButton* OpenResultButton;
   TProgressBar* ProgressBar;
   TStringGrid* StatusGrid;
   TLabel* JsonLabel;
   TMemo* JsonMemo;
   TStatusBar* StatusBar;

   /**
   \brief Initializes the dashboard after the DFM form becomes visible.
   \details No network request is started automatically; the user explicitly starts the showcase.
   \param Sender VCL event sender.
   \returns Nothing.
   \throw Nothing.
   */
   void __fastcall FormShow(TObject* Sender);

   /**
   \brief Executes the complete integrated showcase.
   \details The button remains disabled during execution and is restored after success or failure.
   \param Sender VCL event sender.
   \returns Nothing.
   \throw Nothing; exceptions are converted into a visible error dialog.
   */
   void __fastcall RunButtonClick(TObject* Sender);

   /**
   \brief Opens Explorer at the generated result archive.
   \details The action is enabled only after a successful showcase run.
   \param Sender VCL event sender.
   \returns Nothing.
   \throw Nothing.
   */
   void __fastcall OpenResultButtonClick(TObject* Sender);

private:
   std::filesystem::path theArchivePath;
   int iCompletedRows {};

   /**
   \brief Resets the visible status matrix to its initial state.
   \details Header captions, component names and progress state are initialized consistently.
   \returns Nothing.
   \throw Nothing.
   */
   void ResetDashboard();

   /**
   \brief Applies one engine update to the corresponding grid row.
   \details The VCL message queue is processed afterwards so each stage becomes visible immediately.
   \param theUpdate Engine progress update.
   \returns Nothing.
   \throw Nothing.
   */
   void ApplyUpdate(adecc::showcase::Showcase_Update const& theUpdate);

   /**
   \brief Converts UTF-8 library text to a VCL UnicodeString.
   \details GitHub JSON data and library diagnostics are treated as UTF-8.
   \param svText UTF-8 text.
   \returns VCL Unicode string.
   \throw Nothing.
   */
   [[nodiscard]]
   UnicodeString ToUnicode(std::string_view svText) const;

   /**
   \brief Resolves the grid row belonging to one component.
   \details Component names are stable identifiers shared with ShowcaseEngine.
   \param svComponent Component identifier.
   \returns One-based grid row or zero if no row is known.
   \throw Nothing.
   */
   [[nodiscard]]
   int RowForComponent(std::string_view svComponent) const noexcept;

   /**
   \brief Returns the output directory beside the executable.
   \details Result artifacts are written to a showcase-output subdirectory.
   \returns Absolute output directory.
   \throw std::runtime_error If the executable path cannot be determined.
   */
   [[nodiscard]]
   std::filesystem::path OutputDirectory() const;

public:
   /**
   \brief Constructs the VCL main form.
   \details Standard VCL ownership semantics are preserved.
   \param Owner Owning VCL component.
   \returns Constructed form instance.
   \throw Exception If VCL construction fails.
   */
   __fastcall TMainForm(TComponent* Owner);
   };

extern TMainForm* MainForm;

#endif
