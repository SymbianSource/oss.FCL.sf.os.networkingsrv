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
// udpecho.cpp - udp echo main module
//

#include <coemain.h>
#include <eikenv.h>
#include <eikdef.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdialg.h>
#include <eikdoc.h>

#ifdef MAKE_EXE_APPLICATION
#include <eikstart.h>
#endif

#include <eiktbar.h>
#include <eikedwin.h>

#if EPOC_SDK >= 0x06000000
#       include <techview/eikon.hrh>
#else
#       include <eikdialg.hrh>
#       include <eikedwin.hrh>
#       include <eikcmds.hrh>
#endif

//#include <eikdial.h>

#include "udpecho.h"

#ifdef CALYPSO
#include <AknTextSettingPage.h>
#endif

//
// EXPORTed functions
//

EXPORT_C CApaApplication* NewApplication()
	{
	return new CUdpEchoApplication;
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
// Application class, CUdpEchoApplication
//
//

TUid CUdpEchoApplication::AppDllUid() const
	{
	return KUidEchoApp;
	}

CApaDocument* CUdpEchoApplication::CreateDocumentL()
	{
	return new (ELeave) CUdpEchoDocument(*this);
	}


//
//
// Document class, CUdpEchoDocument
//
//

CUdpEchoDocument::CUdpEchoDocument(CEikApplication& aApp)
		: CEikDocument(aApp)
	{
	}

CEikAppUi* CUdpEchoDocument::CreateAppUiL()
	{
    return new(ELeave) CUdpEchoAppUi;
	}


//
//
// App UI class, CUdpEchoAppUi
//
//

void CUdpEchoAppUi::ConstructL()
    {
    BaseConstructL();

    iAppView=new(ELeave) CUdpEchoView;
	CleanupStack::PushL(iAppView);
    iAppView->ConstructL(ClientRect());
	CleanupStack::Pop();

#if EPOC_SDK < 0x06000000
//#if !defined(CALYPSO) && !defined(CRYSTAL)
    //AddToStackL(iAppView->Console());
	// Display the application name at the top of the toolbar
	CEikFileNameLabel* filenameLabel=STATIC_CAST(CEikFileNameLabel*, iToolBar->ControlById(EUdpEchoCmdFileName));
	filenameLabel->UpdateL();
#endif

    // add app view to stack; enables key event handling.
    AppViewToStackL();

#ifdef CALYPSO
    iAppView->ActivateL();

    iSettingView = new (ELeave) CSettingView(iAppView->Engine());
    iSettingView->ConstructL(ClientRect());
    iSettingView->SetComponentsToInheritVisibility( ETrue );

    SettingViewToStackL();

    iSettingView->ActivateL();

    ShowAppViewL();

#endif
    }

CUdpEchoAppUi::~CUdpEchoAppUi()
	{
	AppViewFromStack();
    delete iAppView;
#ifdef CALYPSO
    delete iSettingView;
#endif
	}

/*
void CUdpEchoAppUi::DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane)
{
	iAppView->DynInitMenuPaneL(aMenuId, aMenuPane);
}
*/

void CUdpEchoAppUi::HandleCommandL(TInt aCommand)
	{
	switch (aCommand)
		{
	case EUdpEchoStart:
		iAppView->StartL();
		break;
	case EUdpEchoStop:
		iAppView->StopL();
		break;
	case EUdpEchoOptions:
#ifdef CALYPSO
        ShowSettingViewL();        
#else
        LaunchOptionsDialogL(iAppView->Engine());
#endif        
		break;
	case EUdpEchoClearScreen:
		iAppView->ClearScreenL();
		break;
	case EUdpEchoAbout:
		LaunchAboutDialogL();
		break;
	case EEikCmdExit:
		iAppView->StopL();
        AppViewFromStack();
#ifdef CALYPSO
        SettingViewFromStack();
#endif        
		Exit();
		break;
#ifdef CALYPSO
    case EUdpEchoSave:
        // Try to save settings data
        if ( iSettingView->SaveSettingsL() )
        {
            // Switch back to the normal view
            ShowAppViewL();
        }
        break;
    case EUdpEchoCancel:
        // Simply switch back to the normal view
        ShowAppViewL();
        break;
#endif // #ifdef CALYPSO
    default:	//Console Specific Commands
		iAppView->Console()->HandleCommandL(aCommand);
		}
	}

#ifdef CALYPSO

void CUdpEchoAppUi::ShowAppViewL()
{
    iSettingView->MakeVisible( EFalse );
    SettingViewFromStack();
    AppViewToStackL();
    iAppView->MakeVisible( ETrue );
    iAppView->DrawNow();

    CEikButtonGroupContainer *cba = ((CAknAppUi*)CEikonEnv::Static()->EikAppUi())->Cba();
    cba->SetCommandSetL(R_ECHO_CBA);
    cba->DrawDeferred();

// Make sure that setting view's scroll indicators are not shown in the app view
    iSettingView->ListBox()->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOff);
    iSettingView->ListBox()->UpdateScrollBarsL();
}

void CUdpEchoAppUi::ShowSettingViewL()
{
    iAppView->MakeVisible( EFalse );
    AppViewFromStack();
    SettingViewToStackL();
    iSettingView->MakeVisible( ETrue );
    iSettingView->DrawNow();

    CEikButtonGroupContainer *cba = ((CAknAppUi*)CEikonEnv::Static()->EikAppUi())->Cba();
    cba->SetCommandSetL(R_SETTINGS_CBA);
    cba->DrawDeferred();

// Make sure that scroll bars are shown if needed
    iSettingView->ListBox()->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);
    iSettingView->ListBox()->UpdateScrollBarsL();
}

void CUdpEchoAppUi::SettingViewToStackL()
{
    if (!iSettingViewOnStack)
    {
        AddToStackL(iSettingView);
        iSettingViewOnStack = ETrue;
    }
}

void CUdpEchoAppUi::SettingViewFromStack()
{
    if (iSettingViewOnStack)
    {
        RemoveFromStack(iSettingView);
        iSettingViewOnStack = EFalse;
    }
}

#endif // #ifdef CALYPSO
    
void CUdpEchoAppUi::AppViewToStackL()
{
    if (!iAppViewOnStack)
    {
        AddToStackL(iAppView);
        iAppViewOnStack = ETrue;
    }
}

void CUdpEchoAppUi::AppViewFromStack()
{
    if (iAppViewOnStack) 
    {
        RemoveFromStack(iAppView);
        iAppViewOnStack = EFalse;
    }
}

#ifndef CALYPSO
    
// Launches a dialog to ask for new options
TBool CUdpEchoAppUi::LaunchOptionsDialogL(CUdpEchoEngine* aEngine) const
{
	COptionsDialog *dialog=new (ELeave) COptionsDialog(aEngine);
	TInt button=dialog->ExecuteLD(R_ECHO_OPTIONS_DIALOG);	//Final D means the dialog is destructed by itself
	return (button==EEikBidOk);	// If button is CANCEL then the algorithm is not added
}

#endif // #ifndef CALYPSO

// Launches a dialog to show an about box
void CUdpEchoAppUi::LaunchAboutDialogL() const
	{
	CEikDialog* dialog = new (ELeave) CEikDialog();
	dialog->ExecuteLD(R_ECHO_ABOUT);	//Final D means the dialog is destructed by itself
	}

//
//
// Application view class, CUdpEchoView
//
//

void CUdpEchoView::ConstructL(const TRect& aRect)
{
    CreateWindowL();
#if EPOC_SDK >= 0x06000000
    SetRect(aRect);
#else
    SetRectL(aRect);
#endif
    iContext=this;
	iBrushStyle=CGraphicsContext::ESolidBrush;
    iBrushColor=KRgbWhite;


	//Engine
	iEngine= new (ELeave) CUdpEchoEngine;
	iEngine->ConstructL(this);
	
	CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);
	
    ActivateL();
}




void CUdpEchoView::CreateBigConsoleL(TInt aFlags)
{

	iConsole=new(ELeave) CConsoleControl;
	//iConsole->ConstructL(aFlags);

#ifndef CALYPSO
    
    iConsole->ConstructL(Position(),Size(),aFlags);
    
#else
    
    TRect rect=Rect();
    TSize size;
    size.iHeight = rect.iBr.iY - rect.iTl.iY - 4; // -4 to not overwrite CBA texts
    size.iWidth = rect.iBr.iX - rect.iTl.iX - 4;  // -4 to leave some space on the right
                                                  // (based on trial and error)
    TPoint position;
    position.iY = 48; // To skip the status pane (based on trial and error)
    position.iX = 2;  // 2 to leave some space on the left (based on trial and error)

    iConsole->ConstructL(position, size, aFlags);
    
#endif // #ifndef CALYPSO
}


CUdpEchoView::~CUdpEchoView()
	{
	delete iConsole;
	delete iEngine;

	}


void CUdpEchoView::Draw(const TRect& ) const
{
	CWindowGc& gc = SystemGc();

#ifndef CALYPSO
    
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	TSize penSizeBold(3,3);
	gc.SetPenSize(penSizeBold);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	
	TRect rect=iConsole->Rect();	//Lines around the Console
	rect.Grow(3,3);
	gc.DrawRect(rect);

#else
    
    gc.Clear();
    
#endif // #ifndef CALYPSO
}


void CUdpEchoView::StartL()
{
	TInt err;

    if (iEngine->Running())
	{
		CEikonEnv::Static()->InfoMsg(_L("Already Running!"));
		return;
	}
		
	err = iEngine->Start();
	if (err != KErrNone)
	{
		Write(_L("Error initializing echo engine\n"));
	}
    UpdateBusyMsgL();
}


void CUdpEchoView::Write(const TDesC &aMsg)
{
	iConsole->Write(aMsg);

}


void CUdpEchoView::StopL()
{
	if (iEngine->Running())
		iEngine->StopL();
    else
		CEikonEnv::Static()->InfoMsg(_L("Already Stopped!"));
    UpdateBusyMsgL();
}


void CUdpEchoView::ClearScreenL()
{
#ifdef CALYPSO
    iConsole->ClearScreen();
#else
	delete iConsole;
	iConsole=NULL;
	CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);	
#endif
}

void CUdpEchoView::UpdateBusyMsgL()
{
	if (iEngine->Running())
    	CEikonEnv::Static()->BusyMsgL(R_BUSY);
    else
    	CEikonEnv::Static()->BusyMsgCancel();
}

#ifndef CALYPSO

// DIALOG CLASSES

COptionsDialog::COptionsDialog(CUdpEchoEngine* aEngine)
{
	iEngine=aEngine;
}


TBool COptionsDialog::OkToExitL(TInt)
{
	iEngine->iProtocol=ChoiceListCurrentItem(EUdpEchoProtocol);
        iEngine->iAction=ChoiceListCurrentItem(EUdpEchoAction);
    iEngine->iPort = NumberEditorValue(EUdpEchoPort);
 	iEngine->iPerPacketTrace = (CheckBoxState(EUdpEchoPacketTrace)== CEikButtonBase::ESet);
 
	// Take optional multicast group address and convert it to IPv6 if it is IPv4
	TBuf<64> groupstring(_L(""));
	GetEdwinText(groupstring, EUdpEchoMcastGroup);
	if (groupstring.Length() > 0)
		iEngine->iMcastGroup.Input(groupstring);
	else
		iEngine->iMcastGroup.SetAddress(0);

	if (iEngine->iMcastGroup.Family() == KAfInet)
		iEngine->iMcastGroup.ConvertToV4Mapped();

	return ETrue;
}

void COptionsDialog::PreLayoutDynInitL()
{
	SetChoiceListCurrentItem(EUdpEchoProtocol,iEngine->iProtocol);
	SetChoiceListCurrentItem(EUdpEchoAction,iEngine->iAction);
	SetNumberEditorValue(EUdpEchoPort, iEngine->iPort);
   	SetCheckBoxState(EUdpEchoPacketTrace, iEngine->iPerPacketTrace  ? CEikButtonBase::ESet : CEikButtonBase::EClear);

	TBuf<64> groupstring(_L(""));	
	if (iEngine->iMcastGroup.IsMulticast()) {
		iEngine->iMcastGroup.Output(groupstring);
		SetEdwinTextL(EUdpEchoMcastGroup, &groupstring);
	}
}

#endif // #ifndef CALYPSO

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

void CConsoleControl::HandleCommandL(TInt /* aCommand */)
    {
    }

void CConsoleControl::FocusChanged(TDrawNow aDrawNow)
	{
	iConsole->ConsoleControl()->SetFocus(IsFocused(), aDrawNow);
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


#ifdef CALYPSO

//------------------------------------------------------------------
// CEchoBinaryPopupSettingItem
//
// NOTE. This class is implemented only to circumvent
// a bug (?) in CAknBinaryPopupSettingItem::Load
//------------------------------------------------------------------

CEchoBinaryPopupSettingItem::CEchoBinaryPopupSettingItem( TInt aIdentifier, TBool& aBinaryValue ) :
   CAknBinaryPopupSettingItem( aIdentifier, aBinaryValue )
{
}

void CEchoBinaryPopupSettingItem::LoadL()
{
    // Call CAknEnumeratedTextPopupSettingItem::LoadL to copy
    // the external boolean value to an internal value
    // NOTE. CAknBinaryPopupSettingItem::LoadL must NOT be called
    CAknEnumeratedTextPopupSettingItem::LoadL();
}

//-----------------------------------------------------------------
// CEchoTextSettingItem
//
// NOTE. This is a re-implementation/copy of CAknTextSettingItem
// (the only difference is in the LoadL method) due to a bug/
// feature in CAknTextSettingItem::LoadL()
//-----------------------------------------------------------------

CEchoTextSettingItem::CEchoTextSettingItem( TInt aIdentifier, TDes& aText ) : 
   CAknSettingItem(aIdentifier), iExternalText( aText ), iInternalTextPtr(0,0)
{
}	

CEchoTextSettingItem::~CEchoTextSettingItem()
{
    delete iInternalText;
}

void CEchoTextSettingItem::StoreL()
{
    // Copy the internal to the external
    iExternalText.Copy(iInternalText->Des() );
}

// NOTE. This is the only method that has changed from CAknTextSettingItem
void CEchoTextSettingItem::LoadL()
{
    delete iInternalText;
    iInternalText = 0;
    // NOTE. The difference is below
    // OLD CODE:
    // iInternalText = iExternalText.AllocL();
    // END OF OLD CODE
    // NEW CODE:
    iInternalText = HBufC::NewL( iExternalText.MaxLength() );
    TPtr ptrText = iInternalText->Des();
    ptrText.Copy( iExternalText );
    // END OF NEW CODE
    // Use Set so that the whole TPtr is just overwritten - and gets a new buffer 
    iInternalTextPtr.Set(iInternalText->Des() );
}

const TDesC& CEchoTextSettingItem::SettingTextL()
{
    return *iInternalText;
}

void CEchoTextSettingItem::EditItemL( TBool /* aCalledFromMenu */)
{
    CAknSettingPage* dlg = new( ELeave )CAknTextSettingPage( SettingPageResourceId(), iInternalTextPtr, 0 );
    SetSettingPage( dlg );
    SettingPage()->SetSettingPageObserver(this);
    SetUpStandardSettingPageL();
    SettingPage()->ExecuteLD(CAknSettingPage::EUpdateWhenChanged);
}

TPtr& CEchoTextSettingItem::InternalTextPtr()
{
    return iInternalTextPtr;
}

TPtrC CEchoTextSettingItem::ExternalText()
{
    return (TPtrC)(iExternalText);
}

void CEchoTextSettingItem::SetExternalText( TDesC& aNewExternalText )
{
    iExternalText.Copy( aNewExternalText );
}

//-----------------------------------------------------------------
// CSettingView
//-----------------------------------------------------------------

void CSettingView::ConstructL(const TRect& /*aRect*/)
{
    // Copy data from the model to member variables
    iProtocol = iEngine->iProtocol;
    iAction = iEngine->iAction;
    iPort.Num( iEngine->iPort );

    CAknSettingItemList::ConstructFromResourceL( R_ECHO_SETTINGS );
}

CSettingView::CSettingView(CUdpEchoEngine *aEngine)
{
    iEngine = aEngine;
}

CSettingView::~CSettingView()
{
}

void CSettingView::ProcessCommandL(TInt /*aCommand*/)
{
}

CAknSettingItem* CSettingView::CreateSettingItemL( TInt identifier )
{
    CAknSettingItem* settingItem;

    switch (identifier)
    {
        case 1:
            settingItem = new (ELeave) CEchoBinaryPopupSettingItem( identifier, iProtocol );
            return settingItem;
            break;
        case 2:
            settingItem = new (ELeave) CEchoTextSettingItem( identifier, iPort );
            return settingItem;
            break;
        default:
            settingItem = new (ELeave) CAknSettingItem(identifier);
            return settingItem;
            break;
    }
}

TBool CSettingView::SaveSettingsL()
{
    // Save settings to the member variables
    StoreSettingsL();

    // Validate input
    TLex lex;

    TInt port;
    lex.Assign( iPort );
    if ( lex.Val( port ) != KErrNone ) 
    {
        CEikonEnv::Static()->InfoMsg(_L("Port number must be numeric"));
        return EFalse;
    }

    // Validation OK, so save settings to the model
    iEngine->iProtocol = iProtocol;
    iEngine->iPort = port;

    return ETrue;
}

#endif // #ifdef CALYPSO

