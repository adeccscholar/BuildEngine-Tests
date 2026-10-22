object ViewerForm: TViewerForm
  Left = 0
  Top = 0
  Caption = 'PDFium / VCL'
  ClientHeight = 850
  ClientWidth = 1100
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poScreenCenter
  OnResize = FormResized
  OnMouseWheel = FormMouseWheel
  object toolbar_: TPanel
    Left = 0
    Top = 0
    Width = 1100
    Height = 62
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    object openButton_: TButton
      Left = 12
      Top = 14
      Width = 105
      Height = 34
      Caption = 'PDF öffnen...'
      TabOrder = 0
      OnClick = OpenClick
    end
    object previousButton_: TButton
      Left = 130
      Top = 14
      Width = 100
      Height = 34
      Caption = 'Zurück'
      TabOrder = 1
      OnClick = PreviousClick
    end
    object pageEdit_: TEdit
      Left = 244
      Top = 18
      Width = 66
      Height = 26
      TabOrder = 2
      OnKeyDown = PageKeyDown
    end
    object goButton_: TButton
      Left = 321
      Top = 14
      Width = 60
      Height = 34
      Caption = 'Gehe zu'
      TabOrder = 3
      OnClick = GoClick
    end
    object nextButton_: TButton
      Left = 394
      Top = 14
      Width = 100
      Height = 34
      Caption = 'Weiter'
      TabOrder = 4
      OnClick = NextClick
    end
    object counterLabel_: TLabel
      Left = 510
      Top = 23
      Width = 60
      Height = 17
      Caption = 'Seite 0 / 0'
    end
    object zoomPanel_: TPanel
      Left = 835
      Top = 0
      Width = 265
      Height = 62
      Align = alRight
      BevelOuter = bvNone
      TabOrder = 5
      object zoomOutButton_: TButton
        Left = 10
        Top = 14
        Width = 38
        Height = 34
        Caption = '-'
        Hint = '25 % verkleinern'
        ShowHint = True
        TabOrder = 0
        OnClick = ZoomOutClick
      end
      object zoomFitButton_: TButton
        Left = 58
        Top = 14
        Width = 68
        Height = 34
        Caption = 'Fit'
        Hint = 'Ganze Seite anzeigen'
        ShowHint = True
        TabOrder = 1
        OnClick = ZoomFitClick
      end
      object zoomInButton_: TButton
        Left = 136
        Top = 14
        Width = 38
        Height = 34
        Caption = '+'
        Hint = '25 % vergrößern'
        ShowHint = True
        TabOrder = 2
        OnClick = ZoomInClick
      end
      object zoomLabel_: TLabel
        Left = 186
        Top = 23
        Width = 45
        Height = 17
        Caption = '100 %'
      end
    end
  end
  object statusBar_: TPanel
    Left = 0
    Top = 822
    Width = 1100
    Height = 28
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 1
    object statusLabel_: TLabel
      Left = 12
      Top = 7
      Width = 225
      Height = 17
      Caption = 'Bitte eine PDF-Datei öffnen.'
    end
  end
  object pageCanvas_: TPaintBox
    Left = 0
    Top = 62
    Width = 1100
    Height = 760
    Align = alClient
    OnPaint = PaintPage
    OnMouseDown = CanvasMouseDown
    OnMouseMove = CanvasMouseMove
    OnMouseUp = CanvasMouseUp
  end
  object openDialog_: TOpenDialog
    DefaultExt = 'pdf'
    Filter = 'PDF-Dateien (*.pdf)|*.pdf|Alle Dateien (*.*)|*.*'
    Options = [ofHideReadOnly, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 600
    Top = 112
  end
end
