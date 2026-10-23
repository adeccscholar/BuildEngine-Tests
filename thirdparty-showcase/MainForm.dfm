object MainForm: TMainForm
  Left = 0
  Top = 0
  Caption = 'adecc Scholar - BCC64X Third-Party Showcase'
  ClientHeight = 720
  ClientWidth = 1120
  Color = clBtnFace
  Constraints.MinHeight = 650
  Constraints.MinWidth = 980
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poScreenCenter
  OnShow = FormShow
  object HeaderPanel: TPanel
    Left = 0
    Top = 0
    Width = 1120
    Height = 112
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    object TitleLabel: TLabel
      Left = 24
      Top = 18
      Width = 441
      Height = 28
      Caption = 'BCC64X Third-Party Integration Showcase'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -23
      Font.Name = 'Segoe UI'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object SubtitleLabel: TLabel
      Left = 24
      Top = 53
      Width = 690
      Height = 17
      Caption = 'Boost.Beast + OpenSSL | libcurl | nlohmann/json | zlib | libzip - ein realer, sichtbarer End-to-End Lauf'
    end
    object RunButton: TButton
      Left = 24
      Top = 78
      Width = 170
      Height = 30
      Caption = 'Showcase starten'
      Default = True
      TabOrder = 0
      OnClick = RunButtonClick
    end
    object OpenResultButton: TButton
      Left = 205
      Top = 78
      Width = 190
      Height = 30
      Caption = 'Ergebnis-ZIP anzeigen'
      Enabled = False
      TabOrder = 1
      OnClick = OpenResultButtonClick
    end
    object ProgressBar: TProgressBar
      Left = 420
      Top = 84
      Width = 676
      Height = 18
      TabOrder = 2
    end
  end
  object StatusGrid: TStringGrid
    Left = 0
    Top = 112
    Width = 1120
    Height = 262
    Align = alTop
    ColCount = 5
    DefaultRowHeight = 32
    FixedCols = 0
    RowCount = 7
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goRowSelect]
    TabOrder = 1
  end
  object JsonLabel: TLabel
    Left = 16
    Top = 390
    Width = 219
    Height = 20
    Caption = 'Erzeugte JSON Zusammenfassung'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -16
    Font.Name = 'Segoe UI'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object JsonMemo: TMemo
    Left = 16
    Top = 418
    Width = 1088
    Height = 270
    Anchors = [akLeft, akTop, akRight, akBottom]
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Consolas'
    Font.Style = []
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssBoth
    TabOrder = 2
    WordWrap = False
  end
  object StatusBar: TStatusBar
    Left = 0
    Top = 701
    Width = 1120
    Height = 19
    Panels = <>
    SimplePanel = True
  end
end
