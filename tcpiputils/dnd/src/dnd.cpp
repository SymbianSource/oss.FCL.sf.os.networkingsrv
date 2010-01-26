// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// dnd.cpp - name resolver daemon main module
//

#include <eikfnlab.h>
#include <eikfontd.h>

#include <eikenv.h>


#include <coeccntx.h>

#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikconso.h>

#include "dnd.hrh"
#include "demon.h"
#include <dndapp.rsg>

const TUid KUidDndApp = {0x10000882};
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
	void ConstructL(const TPoint& aLeftTop, const TSize& aSize, TInt aFlags);
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
// class CDndView
//
class CDndEngine;
class CDndView : public CCoeControl, public MCoeControlBrushContext, public MDemonMain
    {
public:
	~CDndView();
    void ConstructL(const TRect& aRect);
	
	void Start();
	void Stop();
	void Dump();
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Write(const TDesC &aDes);
	void WriteList(const TDesC &aFmt, VA_LIST aList);
	void ClearScreen();
	void ShowError(TInt aId);
	TInt CheckResult(const TDesC &aText, TInt aResult);

	void HandleCommandL(TInt aCommand);
private: 
	void Draw(const TRect& /*aRect*/) const;

	//Component Controls
	void CreateBigConsoleL(TInt aFlags);

	void ShowError(TDes &msg);
	void ShowError(TDes &msg, TInt aErr);

private:
	CConsoleControl* iConsole;
	
	MDemonEngine *iModel;
	TBool iRunning;
	TBuf<1024> iBuf;	//< A work buffer for formatting messsages (Writef)
    };


//
//	CDndAppUi
//
class CDndAppUi : public CEikAppUi
    {
public:
    void ConstructL();
	~CDndAppUi();

private:
	void HandleCommandL(TInt aCommand);
	//	TBool LaunchOptionsDialog(CUDPSendEngine* aModel);
	// void LaunchAboutDialog();
private:
    CDndView* iAppView;
//	CHelpTask *iHelp;
    };

//
// CDndDocument
//
class CDndDocument : public CEikDocument
	{
public:
	CDndDocument(CEikApplication& aApp);
private:
	CEikAppUi* CreateAppUiL();
	};

//
// CDndAppUi
//
class CDndApplication : public CEikApplication
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

void CConsoleControl::ConstructL(const TPoint& aTopLeft,const TSize& aSize,TInt aFlags)
	{
	TRect rect(aTopLeft,aTopLeft + aSize.AsPoint());
	SetRect(rect);	
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
		{
		TCharFormat charFormat;
		charFormat.iFontSpec = iConsole->Font();
		TCharFormatMask dummy;
		if (CEikFontDialog::RunDlgLD(charFormat, dummy))
			{
			//charFormat.iFontSpec.iTypeface.SetIsProportional(EFalse);
			iConsole->SetFontL(charFormat.iFontSpec);
			}
		}
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

	default:
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
		if ( iConsole->Att() & ATT_COLORMASK )	// color?
			iConsole->SetAtt(ATT_NORMAL);	// then set normal
		else								// else
			iConsole->SetAtt(4,11);			// set 4 (darkgray) on 11 (lightgray)
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
void CDndView::ConstructL(const TRect& aRect)
    {
	CreateWindowL();
    SetRect(aRect);
 
    iContext = this;
	iBrushStyle = CGraphicsContext::ESolidBrush;
    iBrushColor = KRgbWhite;
	CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);
	
    ActivateL();
    }

void CDndView::CreateBigConsoleL(TInt aFlags)
	{
	iConsole =new(ELeave) CConsoleControl;
//	TRect rect=Rect();
//	rect.Shrink(3,3);
	iConsole->ConstructL(Position(), Rect().Size(),aFlags);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
	}

CDndView::~CDndView()
	{
	delete iConsole;
	delete iModel;
	}
    
TInt CDndView::CountComponentControls() const
	{
	return 1;
	}

CCoeControl* CDndView::ComponentControl(TInt aIndex) const
	{
	switch (aIndex)
		{
	case 0:
		return iConsole;
	default:
		return 0;
		}
	}

void CDndView::Draw(const TRect& /*aRect*/) const
	{
	}

void CDndView::Start()
	{
	TInt err;

	if (!iModel)
		{

		TRAP(err, iModel = MDemonEngine::NewL(*this));
		if (err == KErrNone && iModel)
			{
			CEikonEnv::Static()->BusyMsgL(R_BUSY);
			TRAP(err, iModel->ConstructL());
			CEikonEnv::Static()->BusyMsgCancel();
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
		CEikonEnv::Static()->InfoMsg(_L("Already started!"));
	}

void CDndView::Write(const TDesC &aMsg)
	{
	iConsole->Write(aMsg);
	iConsole->Write(_L("\n"));
	}

void CDndView::WriteList(const TDesC &aFmt, VA_LIST aList)
	{
	iBuf.FormatList(aFmt, aList);
	iConsole->Write(iBuf);
	iConsole->Write(_L("\n"));
	}


void CDndView::ShowError(TInt aId)
	{
	iEikonEnv->InfoMsg(aId);
	}

void CDndView::ShowError(TDes &msg)
	{
	iEikonEnv->InfoMsg(msg);
	}

void CDndView::ShowError(TDes &msg, TInt aErr)
	{
	TBuf<100> txt;
	TBuf<100> txt2;

	txt.Format(msg);
	iEikonEnv->GetErrorText(txt2,aErr);
	txt.AppendFormat(txt2);
	iEikonEnv->InfoMsg(txt);
	}

TInt CDndView::CheckResult(const TDesC &aText, TInt aResult)
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


void CDndView::Stop()
	{
	TInt err;
	if (iModel)
		{
		TRAP(err, CEikonEnv::Static()->BusyMsgL(R_BUSY));
		delete iModel;	
		iModel = NULL;
		CEikonEnv::Static()->BusyMsgCancel();
		Write(_L("*Stopped*"));
		}
	else
		{
		CEikonEnv::Static()->InfoMsg(_L("Not started!"));
		}
	}

void CDndView::ClearScreen()
	{
	TInt err;
	delete iConsole;
	iConsole = NULL;
	TRAP(err, CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor));	
	}

void CDndView::HandleCommandL(TInt aCommand)
	{
	switch (aCommand)
		{
	case EDndStart:
		Start();
		break;
	case EDndStop:
		Stop();
		break;
	case EDndClearScreen:
		ClearScreen();
		break;
	case EDndDump:
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
		ASSERT(iConsole != NULL);
		iConsole->HandleCommandL(aCommand);
		if (iModel)
			iModel->HandleCommandL(aCommand);
		}
	}

// **************
// APPLICATION UI
// **************
//
void CDndAppUi::ConstructL()
    {
    BaseConstructL();

    iAppView= new (ELeave) CDndView;
    iAppView->ConstructL(ClientRect());
    }

void CDndAppUi::HandleCommandL(TInt aCommand)
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

CDndAppUi::~CDndAppUi()
	{
    delete iAppView;
	}

// ********
// DOCUMENT
// ********
CDndDocument::CDndDocument(CEikApplication& aApp)
		: CEikDocument(aApp)
	{
	}

CEikAppUi* CDndDocument::CreateAppUiL()
	{
    return new (ELeave) CDndAppUi;
	}

// ***********
// APPLICATION
// ***********
//

TUid CDndApplication::AppDllUid() const
	{
	return KUidDndApp;
	}

CApaDocument* CDndApplication::CreateDocumentL()
	{
	return new(ELeave) CDndDocument(*this);
	}

// ****
// MAIN
// ****
//
EXPORT_C CApaApplication* NewApplication()
	{
	return new CDndApplication;
	}


GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}
