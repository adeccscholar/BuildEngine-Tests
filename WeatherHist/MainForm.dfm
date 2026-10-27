object WeatherHistForm: TWeatherHistForm
  Left = 0
  Top = 0
  Caption = 'WeatherHist - Historische Wetterdaten'
  ClientHeight = 700
  ClientWidth = 970
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -14
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poScreenCenter
  object topPanel_: TPanel
    Left = 0
    Top = 0
    Width = 970
    Height = 148
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    object addressLabel_: TLabel
      Left = 16
      Top = 13
      Width = 150
      Height = 17
      Caption = 'Adresse / Standort'
    end
    object dateLabel_: TLabel
      Left = 655
      Top = 13
      Width = 132
      Height = 17
      Caption = 'Datum (TT.MM.JJJJ)'
    end
    object addressEdit_: TEdit
      Left = 16
      Top = 37
      Width = 622
      Height = 28
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 0
      TextHint = 'Strasse, Hausnummer, Postleitzahl und Ort'
    end
    object dateEdit_: TEdit
      Left = 655
      Top = 37
      Width = 156
      Height = 28
      Anchors = [akTop, akRight]
      TabOrder = 1
      Text = '03.08.2026'
    end
    object loadButton_: TButton
      Left = 829
      Top = 35
      Width = 124
      Height = 33
      Anchors = [akTop, akRight]
      Caption = 'Abrufen'
      Default = True
      TabOrder = 2
      OnClick = LoadClick
    end
    object sourceLabel_: TLabel
      Left = 16
      Top = 91
      Width = 930
      Height = 20
      AutoSize = False
      Caption = ''
      EllipsisPosition = epEndEllipsis
    end
    object noteLabel_: TLabel
      Left = 16
      Top = 116
      Width = 930
      Height = 20
      AutoSize = False
      Caption = '15-Minuten-Raster: F15 Prognose original, A60 Archiv original, A~ interpoliert'
    end
  end
  object bottomPanel_: TPanel
    Left = 0
    Top = 661
    Width = 970
    Height = 39
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 1
    object statusLabel_: TLabel
      Left = 16
      Top = 9
      Width = 930
      Height = 22
      AutoSize = False
      Caption = 'Bereit.'
      EllipsisPosition = epEndEllipsis
    end
  end
  object pages_: TPageControl
    Left = 0
    Top = 148
    Width = 970
    Height = 513
    Align = alClient
    ActivePage = summaryTab_
    TabOrder = 2
    object summaryTab_: TTabSheet
      Caption = 'Uebersicht'
      object resultsList_: TListView
        Left = 0
        Top = 148
        Width = 970
        Height = 513
        Align = alClient
        Columns = <
          item
            Caption = 'Merkmal'
            Width = 405
          end
          item
            Caption = 'Wert'
            Width = 515
          end>
        ReadOnly = True
        RowSelect = True
        GridLines = True
        ColumnClick = False
        ViewStyle = vsReport
        TabOrder = 2
      end

    end
    object timelineTab_: TTabSheet
      Caption = 'Tagesverlauf (15 Min.)'
      object timelineList_: TListView
        Left = 0
        Top = 0
        Width = 962
        Height = 481
        Align = alClient
        Columns = <
          item
            Caption = 'Uhrzeit'
            Width = 80
          end
          item
            Caption = 'Temp C'
            Width = 130
          end
          item
            Caption = 'Gefuehlt C'
            Width = 130
          end
          item
            Caption = 'Feuchte %'
            Width = 130
          end
          item
            Caption = 'Tau C'
            Width = 130
          end
          item
            Caption = 'Druck hPa'
            Width = 130
          end
          item
            Caption = 'Wind km/h'
            Width = 130
          end
          item
            Caption = 'Boeen km/h'
            Width = 130
          end
          item
            Caption = 'Richtung'
            Width = 130
          end
          item
            Caption = 'Wolken %'
            Width = 130
          end
          item
            Caption = 'Nied. mm'
            Width = 130
          end
          item
            Caption = 'Regen mm'
            Width = 130
          end
          item
            Caption = 'UV'
            Width = 130
          end
          item
            Caption = 'WMO'
            Width = 130
          end>
        ReadOnly = True
        RowSelect = True
        GridLines = True
        ColumnClick = False
        ViewStyle = vsReport
        TabOrder = 0
      end
    end
  end
  object resultTimer_: TTimer
    Enabled = False
    Interval = 150
    OnTimer = ResultTick
    Left = 48
    Top = 179
  end
end
