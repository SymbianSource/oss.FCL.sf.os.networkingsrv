// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include <eikfnlab.h>
#ifndef CALYPSO
#include <eikfontd.h>
#endif
#include <eikenv.h>

#if EPOC_SDK >= 0x06000000
#else
#include <e32keys.h>
#include <coemain.h>
#include <eikdef.h>
#include <eikcmds.hrh>
#include <techview/eikon.rsg>
#include <eiklabel.h>
#include <eikfnlab.h>
#include <eiktbar.h>
#include <eikchlst.h>
#include <eikpgsel.h>

#include <eikfontd.h>
#include <eiklabel.h>
#endif

#include <coeccntx.h>

#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikconso.h>

#include "anvltest.h"
#include "demon.h"
#include <anvltest.rsg>

#ifdef MAKE_EXE_APPLICATION
#include <eikstart.h>
#endif


const TUid KUidAnvltestApp = {0x10000883};

// 
// CSimpleConsole
//
enum TMessageControlFontStyle
    {
    EStyleElementBold=EMenuCommandBold,
    EStyleElementItalic=EMenuCommandItalic,
    EStyleElementInverse=EMenuCommandInverse,
    EStyleElementUnderline=EMenuCommandUnderline,
    EStyleElementColor=EMenuCommandColor
    };

class CConsoleControl : public CCoeControl
    {
public:
    CConsoleControl() {}
    ~CConsoleControl();
    void ConstructL(TPoint aLeftTop,const TSize& aSize,TInt aFlags);
    void HandleCommandL(TInt aCommand);
    void ActivateL();
    void SetScrollBarVisibilityL(CEikScrollBarFrame::TScrollBarVisibility aHBarVisibility, CEikScrollBarFrame::TScrollBarVisibility aVBarVisibility);
    void DrawCursor();
    void Write(const TDesC &aDes);
    CEikConsoleControl *ConsoleControl();
    TBool UpdateScrollBars();
    void ClearScreen();
    void Redraw(const TRect &aRect);
    void Lf();
protected:
    void FocusChanged(TDrawNow aDrawNow);
private:
    void ToggleFontStyleAndRedrawL(TMessageControlFontStyle aStyleElement);
    
private:
    CEikConsoleScreen* iConsole;
    TInt iHistory;
    };


//
// class CAnvltestView
//
class CAnvltestEngine;
class CAnvltestView : public CCoeControl, public MCoeControlBrushContext, public MDemonMain
    {
public:
    ~CAnvltestView();
    void ConstructL(const TRect& aRect);
    
	void Start(int);
    void Stop();
    void Dump();
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;
    void Write(const TDesC &aDes);
    void ClearScreen();
    void ShowError(TInt aId);
    TInt CheckResult(const TDesC &aText, TInt aResult);

    void HandleCommandL(TInt aCommand);

	int GetIpVersion(void);

private: 
    void Draw(const TRect& /*aRect*/) const;

    void CreateBigConsoleL(TInt aFlags);

    void ShowError(TDes &msg);
    void ShowError(TDes &msg, TInt aErr);

private:

	int ipVersion;

    CConsoleControl* iConsole;
    
    CDemonEngineBase *iModel;

    TBool iRunning;

    };


//
//  CAnvltestAppUi
//
class CAnvltestAppUi : public CEikAppUi
    {
public:
    void ConstructL();
    ~CAnvltestAppUi();

private:
    void HandleCommandL(TInt aCommand);
    //  TBool LaunchOptionsDialog(CUDPSendEngine* aModel);
    // void LaunchAboutDialog();
private:
    CAnvltestView* iAppView;
//  CHelpTask *iHelp;
    };

//
// CAnvltestDocument
//
class CAnvltestDocument : public CEikDocument
    {
public:
    CAnvltestDocument(CEikApplication& aApp);
private:
    CEikAppUi* CreateAppUiL();
    };

//
// CAnvltestAppUi
//
class CAnvltestApplication : public CEikApplication
    {
private: // from CApaApplication
    CApaDocument* CreateDocumentL();
    TUid AppDllUid() const;
    };

// 
// CConsoleControl
//
CConsoleControl::~CConsoleControl()
    {
    delete iConsole;
    }

void CConsoleControl::ConstructL(TPoint aTopLeft,const TSize& aSize,TInt aFlags)
    {
    TRect rect(aTopLeft,aTopLeft + aSize.AsPoint());
#if EPOC_SDK >= 0x06000000
    SetRect(rect);  
#else
    SetRectL(rect); 
#endif
    iConsole= new (ELeave) CEikConsoleScreen;
    iConsole->ConstructL(_L("TEST"),aTopLeft,aSize,aFlags,EEikConsWinInPixels);
    iConsole->SetHistorySizeL(200,0);
    //iConsole->SetAllPrintable(ETrue);
    iHistory=200;
    }

void CConsoleControl::ActivateL()
    {
    CCoeControl::ActivateL();
    iConsole->SetKeepCursorInSight(TRUE);
    iConsole->DrawCursor();
    iConsole->SetAtt(ATT_NORMAL);
    }

void CConsoleControl::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
        {
    case EConsolFontDialog:
#if EPOC_SDK >= 0x06000000
        {
#ifndef CALYPSO
        TCharFormat charFormat;
        charFormat.iFontSpec = iConsole->Font();
        TCharFormatMask dummy;
        if (CEikFontDialog::RunDlgLD(charFormat, dummy))
            {
            //charFormat.iFontSpec.iTypeface.SetIsProportional(EFalse);
            iConsole->SetFontL(charFormat.iFontSpec);
            }
#endif
        }
#else
        {
        TCharFormat charFormat;
        charFormat.iFontSpec = iConsole->Font();
        TCharFormatMask dummy;
        CEikFontDialog* dialog=new(ELeave) CEikFontDialog(charFormat,dummy);
        if (dialog->ExecuteLD(R_EIK_DIALOG_FONT))
            {
            //charFormat.iFontSpec.iTypeface.SetIsProportional(EFalse);
            iConsole->SetFontL(charFormat.iFontSpec);
            }
        }
#endif
        break;
#if 0
    case EConsolHistory:
        {
        CHistoryDialog* dialog2 = new(ELeave) CHistoryDialog(&iHistory);
        if (dialog2->ExecuteLD(R_KMD_HISTORY_DIALOG))
            iConsole->SetHistorySizeL(iHistory,0);
        }
        break;
#endif
    case EConsolScrollNone:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EOff);
        break;
    case EConsolScrollHor:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EAuto,CEikScrollBarFrame::EOff);
        break;
    case EConsolScrollVert:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
        break;
    case EConsolScrollBoth:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EAuto,CEikScrollBarFrame::EAuto);
        break;


        }
    }

void CConsoleControl::FocusChanged(TDrawNow aDrawNow)
    {
    iConsole->ConsoleControl()->SetFocus(IsFocused(), aDrawNow); 
    }

void CConsoleControl::ToggleFontStyleAndRedrawL(TMessageControlFontStyle aStyleElement)
    {
    switch (aStyleElement)
        {
    case EStyleElementColor:
        if ( iConsole->Att() & ATT_COLORMASK )  // color?
            iConsole->SetAtt(ATT_NORMAL);   // then set normal
        else                                // else
            iConsole->SetAtt(4,11);         // set 4 (darkgray) on 11 (lightgray)
        break;
    case EStyleElementBold:
        // clear color flag (just to be sure) and switch bold flag
        iConsole->SetAtt( (iConsole->Att()&(~ATT_COLORMASK)) ^ ATT_BOLD );
        break;
    case EStyleElementItalic:
        // clear color flag (just to be sure) and switch italic flag
        iConsole->SetAtt( (iConsole->Att()&(~ATT_COLORMASK)) ^ ATT_ITALIC );
        break;
    case EStyleElementInverse:
        // clear color flag (just to be sure) and switch inverse flag
        iConsole->SetAtt( (iConsole->Att()&(~ATT_COLORMASK)) ^ ATT_INVERSE );
        break;
    case EStyleElementUnderline:
        // clear color flag (just to be sure) and switch underline flag
        iConsole->SetAtt( (iConsole->Att()&(~ATT_COLORMASK)) ^ ATT_UNDERLINE );
        break;
        }
    }

void CConsoleControl::SetScrollBarVisibilityL(CEikScrollBarFrame::TScrollBarVisibility aHBarVisibility, CEikScrollBarFrame::TScrollBarVisibility aVBarVisibility)
    {
    iConsole->SetScrollBarVisibilityL(aHBarVisibility,aVBarVisibility);
    iConsole->ConsoleControl()->UpdateArea();
    iConsole->UpdateScrollBars();
    iConsole->ConsoleControl()->UpdateArea();
    //TBool b=iConsole->RecalculateSize();
    }


void CConsoleControl::DrawCursor()
    {
    iConsole->DrawCursor();
    }


void CConsoleControl::Write(const TDesC &aMsg)
    {
    iConsole->Write(aMsg);
    iConsole->FlushChars();
    }


CEikConsoleControl *CConsoleControl::ConsoleControl()
    {
    return iConsole->ConsoleControl();
    }

TBool CConsoleControl::UpdateScrollBars()
    {
    return iConsole->UpdateScrollBars();
    }

void CConsoleControl::ClearScreen()
    {
    iConsole->ClearScreen();
    }


void CConsoleControl::Redraw(const TRect &aRect)
    {
    iConsole->Redraw(aRect);
    }

void CConsoleControl::Lf()
    {
    iConsole->Lf();
    }


// ****************
// APPLICATION VIEW
// ****************
//
void CAnvltestView::ConstructL(const TRect& aRect)
    {
    CreateWindowL();
#if EPOC_SDK >= 0x06000000
    SetRect(aRect);
#else
    SetRectL(aRect);
#endif
 
    iContext = this;
    iBrushStyle = CGraphicsContext::ESolidBrush;
    iBrushColor = KRgbWhite;
    CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);
    
    ActivateL();
    }

void CAnvltestView::CreateBigConsoleL(TInt aFlags)
    {
    iConsole =new(ELeave) CConsoleControl;
//  TRect rect=Rect();
//  rect.Shrink(3,3);
    iConsole->ConstructL(Position(), Rect().Size(),aFlags);
    iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
    }

CAnvltestView::~CAnvltestView()
    {
    delete iConsole;
    delete iModel;
    }
    
TInt CAnvltestView::CountComponentControls() const
    {
    return 1;
    }

CCoeControl* CAnvltestView::ComponentControl(TInt aIndex) const
    {
    switch (aIndex)
        {
    case 0:
        return iConsole;
    default:
        return 0;
        };
    }

void CAnvltestView::Draw(const TRect& /*aRect*/) const
    {
#if 0
    CWindowGc& gc = SystemGc();
    
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    TSize penSizeBold(3,3);
    gc.SetPenSize(penSizeBold);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    
    TRect rect=iConsole->Rect();    //Lines around the Console
    rect.Grow(3,3);
    gc.DrawRect(rect);
#endif
    }

int CAnvltestView::GetIpVersion(void)
{
 return ipVersion;
}


void CAnvltestView::Start(int ipVersionSelected)
    {
    TInt err;

	ipVersion = ipVersionSelected;

    if (!iModel)
        {
        iEikonEnv->BusyMsgL(R_BUSY);
        
        TRAP(err, iModel = ENGINE::NewL(this,ipVersionSelected));
       
		if (err == KErrNone && iModel)
            {
            TRAP(err, iModel->ConstructL());
            iEikonEnv->BusyMsgCancel();
            if (err!=KErrNone)
                {
                TBuf<50> msg;
                msg.Format(_L("Error initializing: %d"), err);
                ShowError(msg);
                Stop();
                }
            }
        }
    else
        iEikonEnv->InfoMsg(_L("Already started!"));
    }

void CAnvltestView::Write(const TDesC &aMsg)
    {
    iConsole->Write(aMsg);
    }

void CAnvltestView::ShowError(TInt aId)
    {
    iEikonEnv->InfoMsg(aId);
    }

void CAnvltestView::ShowError(TDes &msg)
    {
    iEikonEnv->InfoMsg(msg);
    }

void CAnvltestView::ShowError(TDes &msg, TInt aErr)
    {
    TBuf<100> txt;
    TBuf<100> txt2;

    txt.Format(msg);
    iEikonEnv->GetErrorText(txt2,aErr);
    txt.AppendFormat(txt2);
    iEikonEnv->InfoMsg(txt);
    }

TInt CAnvltestView::CheckResult(const TDesC &aText, TInt aResult)
    {
    if (aResult == KErrNone)
        return KErrNone;

    TBuf<100> err;
    iEikonEnv->GetErrorText(err, aResult);

    TBuf<200> str(aText);
    str.AppendFormat(_L(" returned with [%d: %s] "), aResult,err.PtrZ());
    Write(str);
    return aResult;
    }


void CAnvltestView::Stop()
    {
#if 1
    if (iModel)
        {
        CEikonEnv::Static()->BusyMsgL(R_BUSY);
        delete iModel;  
        iModel = NULL;
        CEikonEnv::Static()->BusyMsgCancel();
        Write(_L("\n ANVL Test Stub STOPPED"));
        }
    else
        {
        CEikonEnv::Static()->InfoMsg(_L("Not started!"));
        }
#else
    if (iModel)
        {
        CEikonEnv::Static()->InfoMsg(_L("Stop not supported!"));
        }
    else
        {
        CEikonEnv::Static()->InfoMsg(_L("Not started!"));
        }
#endif
    }

void CAnvltestView::ClearScreen()
    {
    delete iConsole;
    iConsole = NULL;
    CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor); 
    }

void CAnvltestView::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
        {
	case EAnvltestStartIPv4:
        Start(IPV4);
        break;
	case EAnvltestStartIPv6:
        Start(IPV6);
        break;
    case EAnvltestStop:
        Stop();
        break;
    case EAnvltestClearScreen:
        ClearScreen();
        break;
    case EAnvltestDump:
        if (iModel)
            {
            CEikonEnv::Static()->BusyMsgL(R_BUSY);
            iModel->HandleCommandL(aCommand);
            CEikonEnv::Static()->BusyMsgCancel();
            }
        else
            CEikonEnv::Static()->InfoMsg(_L("Not started!"));
        break;

    default:
        iConsole->HandleCommandL(aCommand);
        if (iModel)
            iModel->HandleCommandL(aCommand);
        }
    }

// **************
// APPLICATION UI
// **************
//
void CAnvltestAppUi::ConstructL()
    {
    BaseConstructL();

    iAppView= new (ELeave) CAnvltestView;
    iAppView->ConstructL(ClientRect());
#if EPOC_SDK >= 0x06000000
#else
    CEikFileNameLabel* filenameLabel=STATIC_CAST(CEikFileNameLabel*, iToolBar->ControlById(EAnvltestFilename));
    filenameLabel->UpdateL();
#endif
    }

void CAnvltestAppUi::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
        {
    case EEikCmdExit:
        Exit();
        return;
    default:
        iAppView->HandleCommandL(aCommand);
        }
    }

CAnvltestAppUi::~CAnvltestAppUi()
    {
#ifndef CALYPSO
	RemoveFromStack(iAppView);
#endif
    delete iAppView;
    }

// ********
// DOCUMENT
// ********
CAnvltestDocument::CAnvltestDocument(CEikApplication& aApp)
        : CEikDocument(aApp)
    {
    }

CEikAppUi* CAnvltestDocument::CreateAppUiL()
    {
    return new (ELeave) CAnvltestAppUi;
    }

// ***********
// APPLICATION
// ***********
//

TUid CAnvltestApplication::AppDllUid() const
    {
    return KUidAnvltestApp;
    }

CApaDocument* CAnvltestApplication::CreateDocumentL()
    {
    return new(ELeave) CAnvltestDocument(*this);
    }

// ****
// MAIN
// ****
//

EXPORT_C CApaApplication* NewApplication()
      {
      return new CAnvltestApplication;
      }

#ifdef MAKE_EXE_APPLICATION

GLDEF_C TInt E32Main()
	{
	return EikStart::RunApplication(NewApplication);
	}

#else

#ifdef __SECURE_API__
GLDEF_C TInt E32Dll()
#else
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
#endif
    {
    return KErrNone;
    }
    
#endif // MAKE_EXE_APPLICATION
