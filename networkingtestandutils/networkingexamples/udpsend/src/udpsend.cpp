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
// udpsend.cpp - udp packet sender main module
//

#include <coemain.h>
#include <eikenv.h>
#include <eikdef.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdialg.h>
#include <eikdoc.h>

#if EPOC_SDK < 0x06000000
#include <eikchkbx.h>
#include <eikfontd.h>
#endif

#include <eiktbar.h>
#include <eikedwin.h>

#ifdef MAKE_EXE_APPLICATION
#include <eikstart.h>
#endif

#if EPOC_SDK >= 0x06000000
#   include <techview/eikon.hrh>
#else
#   include <eikdialg.hrh>
#   include <eikedwin.hrh>
#   include <eikcmds.hrh>
#endif

#if EPOC_SDK < 0x06000000
#include <eikdial.h>
#endif

#include "udpsend.h"

//
// EXPORTed functions
//

EXPORT_C CApaApplication* NewApplication()
    {
    return new CRotorApplication;
    }


#ifdef MAKE_EXE_APPLICATION

GLDEF_C TInt E32Main()
	{
	return EikStart::RunApplication(NewApplication);
	}

#else

    
#endif // MAKE_EXE_APPLICATION


//
//
// Application class, CRotorApplication
//
//

TUid CRotorApplication::AppDllUid() const
    {
    return KUidRotorApp;
    }

CApaDocument* CRotorApplication::CreateDocumentL()
    {
    return new (ELeave) CRotorDocument(*this);
    }


//
//
// Document class, CRotorDocument
//
//

CRotorDocument::CRotorDocument(CEikApplication& aApp)
        : CEikDocument(aApp)
    {
    }

CEikAppUi* CRotorDocument::CreateAppUiL()
    {
    return new(ELeave) CRotorAppUi;
    }


//
//
// App UI class, CRotorAppUi
//
//

void CRotorAppUi::ConstructL()
    {
    BaseConstructL();

    iAppView=new(ELeave) CUDPSendView;
    CleanupStack::PushL(iAppView);
    iAppView->ConstructL(ClientRect());
    CleanupStack::Pop();
    //AddToStackL(iAppView->Console());
    // Display the application name at the top of the toolbar
    // First get the control to display it in.
#if EPOC_SDK >= 0x06000000
#else
    CEikFileNameLabel* filenameLabel=STATIC_CAST(CEikFileNameLabel*, iToolBar->ControlById(EUDPSendCmdFileName));
    // Then display the application name. UpdateL() displays the application's
    // main document file name in the filename label control. However, in this
    // application, there is no main document file (because the application is
    // not file based). In this case, UpdateL() displays the application name by
    // default.
    filenameLabel->UpdateL();
#endif
    }

CRotorAppUi::~CRotorAppUi()
    {
    delete iAppView;
    }

/*
void CRotorAppUi::DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane)
{
    iAppView->DynInitMenuPaneL(aMenuId, aMenuPane);
}
*/

void CRotorAppUi::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
        {
    // Each command in the toolbar/Tools menu prints an info message
    // using the EIKON InfoMsg() function.
    case EUDPSendStart:
        //iEikonEnv->InfoMsg(R_ROTOR_COMMAND_1);
        iAppView->StartL();
        break;
    case EUDPSendStop:
        iAppView->Stop();
        //iEikonEnv->InfoMsg(R_EXAMPLE_COMMAND_2);
        break;

    case EUDPSendOptions:
        //iEikonEnv->InfoMsg(R_NOT_IMPLEMENTED);
        LaunchOptionsDialogL(iAppView->Model());
        break;

    case EUDPSendClearScreen:
        iAppView->ClearScreenL();
        break;
#if 0    
    case EUDPSendHelp:
        iEikonEnv->InfoMsg(R_NOT_IMPLEMENTED);
        //iHelp->DisplayL();
        break;
#endif // 0
    case EUDPSendAbout:
        LaunchAboutDialogL();
        break;
    case EEikCmdExit:
        iAppView->Stop();
        Exit();
        break;
    default:    //Console Specific Commands
        iAppView->Console()->HandleCommandL(aCommand);
        }
    }

// Launches a dialog to ask for new options
TBool CRotorAppUi::LaunchOptionsDialogL(CUDPSendEngine* aModel) const
{
    COptionsDialog *dialog=new (ELeave) COptionsDialog(aModel);
    TInt button=dialog->ExecuteLD(R_ROTOR_OPTIONS_DIALOG);  //Final D means the dialog is destructed by itself
    return (button==EEikBidOk); // If button is CANCEL then the algorithm is not added
}

// Launches a dialog to show an about box
void CRotorAppUi::LaunchAboutDialogL() const
    {
    CEikDialog* dialog = new (ELeave) CEikDialog();
    dialog->ExecuteLD(R_ROTOR_ABOUT);   //Final D means the dialog is destructed by itself
    }

//
//
// Application view class, CUDPSendView
//
//

void CUDPSendView::ConstructL(const TRect& aRect)
{
    CreateWindowL();
#if EPOC_SDK >= 0x06000000
    SetRect(aRect);
#else
    SetRectL(aRect);
#endif

#if 0
    Window().SetShadowDisabled(ETrue);
#endif
    iContext=this;
    iBrushStyle=CGraphicsContext::ESolidBrush;
    iBrushColor=KRgbWhite;


    //Model or engine
    iModel= new (ELeave) CUDPSendEngine;
    iModel->ConstructL(this);
    
    CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);
    
    ActivateL();
}




void CUDPSendView::CreateBigConsoleL(TInt aFlags)
{

    iConsole=new(ELeave) CConsoleControl;
    //iConsole->ConstructL(aFlags);
    //TRect rect=Rect();
    //rect.Shrink(3,3);
    iConsole->ConstructL(Position(),Size(),aFlags);
    iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
    
}



CUDPSendView::~CUDPSendView()
    {
    delete iConsole;
    delete iModel;

    }


// The following two functions have to be implemented for all compound controls.
TInt CUDPSendView::CountComponentControls() const
{
    return 1;   //With or without rotor
}

CCoeControl* CUDPSendView::ComponentControl(TInt aIndex) const
{
    switch (aIndex)
    {
    case 0:
        return iConsole;

    default:
        return NULL;
    }
}


void CUDPSendView::Draw(const TRect& ) const
{
    CWindowGc& gc = SystemGc();
    
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    TSize penSizeBold(3,3);
    gc.SetPenSize(penSizeBold);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    
    TRect rect=iConsole->Rect();    //Lines around the Console
    rect.Grow(3,3);
    gc.DrawRect(rect);


}


void CUDPSendView::StartL()
{
    TBuf<50> msg;
    TInt err;
    if (iModel->Running())
    {
        CEikonEnv::Static()->InfoMsg(_L("Already Running!"));
        return;
    }
    
    
    CEikonEnv::Static()->BusyMsgL(R_BUSY);
    
    
    TRAP(err,iModel->StartL());
    if (err!=KErrNone)
    {
        msg.Format(_L("Error initializing: %d"), err);
        ShowError(msg);
        Stop();
    }
    CEikonEnv::Static()->BusyMsgCancel();
    
}


void CUDPSendView::Write(const TDesC &aMsg)
{
    iConsole->Write(aMsg);

}

void CUDPSendView::ShowError(TInt aId)
{
    iEikonEnv->InfoMsg(aId);
}

void CUDPSendView::ShowError(const TDes &msg)
{
    iEikonEnv->InfoMsg(msg);
}

void CUDPSendView::ShowError(const TDes &msg, TInt aErr)
{
    TBuf<100> txt;
    TBuf<100> txt2;

    txt.Format(msg);
    iEikonEnv->GetErrorText(txt2,aErr);
    txt.AppendFormat(txt2);
    iEikonEnv->InfoMsg(txt);
}

void CUDPSendView::Stop()
{
    if (iModel->Running())
    {
        iModel->Stop();
    }
}


void CUDPSendView::ClearScreenL()
{
    delete iConsole;
    iConsole=NULL;
    CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor); 
}





// DIALOG CLASSES

//  printf ( "%s <hostname> <dst port> <src port> <delay [ms]> <packet size> <priority> <flowlabel>\n", argv[0] );
COptionsDialog::COptionsDialog(CUDPSendEngine* aModel)
{
    iModel=aModel;
}


TBool COptionsDialog::OkToExitL(TInt)
{
    iModel->iProtocol=ChoiceListCurrentItem(EUDPSendProtocol);

    iModel->iDestPort = NumberEditorValue(EUDPSendDestPort);
    //iModel->iSrcPort = NumberEditorValue(EUDPSendSrcPort);
    iModel->iTotalPackets = NumberEditorValue(EUDPSendPacketNum);
    iModel->iDelay = NumberEditorValue(EUDPSendDelay);
    iModel->iPacketSize = NumberEditorValue(EUDPSendPacketSize);
 	iModel->iSendSynch = (CheckBoxState(EUDPSendSynch)== CEikButtonBase::ESet);
    iModel->iPriority = NumberEditorValue(EUDPSendPriority);
    iModel->iFlowLabel= NumberEditorValue(EUDPSendFlowLabel);
    iModel->iTcpLinger = NumberEditorValue(ETCPLinger);
    iModel->iTcpNagling = ChoiceListCurrentItem(ETCPNagling);

    return ETrue;
}

void COptionsDialog::PreLayoutDynInitL()
{
    SetChoiceListCurrentItem(EUDPSendProtocol,iModel->iProtocol);
    SetNumberEditorValue(EUDPSendDestPort, iModel->iDestPort);
    //SetNumberEditorValue(EUDPSendSrcPort, iModel->iSrcPort);
    SetNumberEditorValue(EUDPSendPacketNum, iModel->iTotalPackets);
    SetNumberEditorValue(EUDPSendDelay, iModel->iDelay);
    SetNumberEditorValue(EUDPSendPacketSize, iModel->iPacketSize);
   	SetCheckBoxState(EUDPSendSynch, iModel->iSendSynch  ? CEikButtonBase::ESet : CEikButtonBase::EClear);
    SetNumberEditorValue(EUDPSendPriority, iModel->iPriority);
    SetNumberEditorValue(EUDPSendFlowLabel, iModel->iFlowLabel);
    SetNumberEditorValue(ETCPLinger, iModel->iTcpLinger);
    SetChoiceListCurrentItem(ETCPNagling, iModel->iTcpNagling);
    
    /*
    TBool b=(iModel->iProtocol==IPv6);

    SetLineDimmedNow(EUDPSendPriority, !b);
    MakeLineVisible(EUDPSendPriority,b);
    SetLineDimmedNow(EUDPSendFlowLabel,!b);
    MakeLineVisible(EUDPSendFlowLabel,b);
    */

}

/*
void COptionsDialog::HandleControlStateChangeL(TInt aControlId)
{
    TBool b;
    switch (aControlId)
    {
    case EUDPSendProtocol:
        //Dimms/Undimms some lines depending on the IP protocol
        b = (ChoiceListCurrentItem(EUDPSendProtocol)==IPv6);
        SetLineDimmedNow(EUDPSendPriority, !b);
        MakeLineVisible(EUDPSendPriority,b);
        SetLineDimmedNow(EUDPSendFlowLabel,!b);
        MakeLineVisible(EUDPSendFlowLabel,b);
        break;

    }

}
*/

//
// CHostNameDialog
//

CHostNameDialog::CHostNameDialog(CUDPSendEngine* aSender):CEikDialog(),iSender(aSender)
{
}

// Checks if data entered by user is correct
// returns ETrue to exit dialog anf EFalse to not exit it (no name entered)
TBool CHostNameDialog::OkToExitL(TInt aButton)
{   
    if (aButton==EEikBidCancel) //CANCEL BUTTON
    {
        iOKPressed=EFalse;
        return ETrue;
    }

    TBuf<80> hostname;
    GetEdwinText(hostname,EUDPSendHostName);
    
    if (hostname.Length())
    {
        iSender->SetHostName(hostname);
        return ETrue;
    }
    else
        return EFalse;  //If no hostname specified can't continue
}


//To initialize dialog data
void CHostNameDialog::PreLayoutDynInitL()
{
    //TBuf<KHostNameLimit> hostname(_L(""));

    SetEdwinTextL(EUDPSendHostName,iSender->GetHostName());
}

//
// CHistoryDialog
//



CHistoryDialog::CHistoryDialog(TInt *aHistory)
{
    iHistory=aHistory;
}

TBool CHistoryDialog::OkToExitL(TInt)
{
    *iHistory=NumberEditorValue(ERotorHistory); 
    return ETrue;
}

void CHistoryDialog::PreLayoutDynInitL()
{
    SetNumberEditorValue(ERotorHistory, *iHistory);
}


//
// CConsoleControl
//


CConsoleControl::~CConsoleControl()
    {
    //delete iSelBufPtr; // forget selection
    delete iConsole;
    }

void CConsoleControl::ConstructL(const TPoint& aTopLeft, const TSize& aSize, TInt aFlags)
    {

    TRect rect(aTopLeft,aTopLeft + aSize.AsPoint());
#if EPOC_SDK >= 0x06000000
    SetRect(rect);
#else
    SetRectL(rect);
#endif
    
    iConsole=new(ELeave) CEikConsoleScreen;
    iConsole->ConstructL(_L("TEST"),aTopLeft,aSize,aFlags,EEikConsWinInPixels);
    iConsole->SetHistorySizeL(200,0);
    iHistory=200;
    }

void CConsoleControl::ActivateL()
    {
    CCoeControl::ActivateL();
    iConsole->SetKeepCursorInSight(TRUE);
    iConsole->DrawCursor();
    iConsole->SetAtt(ATT_NORMAL);
    }

#if EPOC_SDK >= 0x06000000
void CConsoleControl::HandleCommandL(TInt /*aCommand*/)
    {
    }
#else
void CConsoleControl::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
    {
#if 0
    case ERotorFontDialog:
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
        break;
#endif

    case ERotorHistory:
        {
            CHistoryDialog* dialog2=new(ELeave) CHistoryDialog(&iHistory);
            if (dialog2->ExecuteLD(R_ROTOR_HISTORY_DIALOG))
            {
                iConsole->SetHistorySizeL(iHistory,0);
            }
        }
        break;

    case ERotorScrollNone:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EOff);
        break;
    case ERotorScrollHor:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EAuto,CEikScrollBarFrame::EOff);
        break;
    case ERotorScrollVert:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
        break;
    case ERotorScrollBoth:
        iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EAuto,CEikScrollBarFrame::EAuto);
        break;


        }
    }
#endif  

void CConsoleControl::FocusChanged(TDrawNow aDrawNow)
    {
    iConsole->ConsoleControl()->SetFocus(IsFocused(), aDrawNow);
    }

#if EPOC_SDK >= 0x06000000
#else
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
#endif
    
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

CEikConsoleControl *CConsoleControl::ConsoleControl() const
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
