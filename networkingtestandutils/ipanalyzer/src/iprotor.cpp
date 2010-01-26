// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// iprotor.cpp - protocol analyzer main module
// This software has been implemented in the 6PACK
// project at the Mobile Networks Laboratory (MNW)
// http://www.research.nokia.com/projects/6pack/
// This file contains the C++ source code for the iprotor
// example program. This builds on the iprotor program, but has the 
// following additions:
// - the resource file, iprotor.rss, defines a toolbar for the
// application
// - the constructor for the App UI contains code that displays the
// application name at the top of the toolbar
// - the resource file defines an additional menu pane, labeled 
// "Tools" and containing the four commands that can be invoked
// from the toolbar 
// - the .hrh file that defines the commands invoked by the Tools
// menu/toolbar.
// - HandleCommandL() is expanded to handle the additional commands
// (they do nothing but print out an error message).
//

#include <basched.h>
#include <e32math.h>

#if EPOC_SDK < 0x06000000
#include <eikenv.h>
#include <coecntrl.h>
#include <eikappui.h>
#include <e32keys.h>
#include <techview/eikmenup.h>
#include <eikmenu.hrh>
#include <eikdef.h>
#include <techview/eikon.rsg>
#include <techview/eikpgsel.h>

#include <techview/eiklabel.h>
#include <eikdialg.hrh>
#endif

#ifdef MAKE_EXE_APPLICATION
#include <eikstart.h>
#endif

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <s32stor.h>
#endif

#include <techview/eikfontd.h>
#include <techview/eikchlst.h>
#include "iprotor.h"


#define PI 3.141592654
#define KLabelHeight	25

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

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)

	{
	return KErrNone;
	}
#endif

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

    iAppView=new(ELeave) CRotorAppView;
	CleanupStack::PushL(iAppView);
    iAppView->ConstructL(ClientRect(), this);
	iAppView->SwitchRotorL();
	CleanupStack::Pop();
	//AddToStackL(iAppView->Console());
	// Display the application name at the top of the toolbar
	// First get the control to display it in.
#if EPOC_SDK < 0x06000000
	CEikFileNameLabel* filenameLabel=STATIC_CAST(CEikFileNameLabel*, iToolBar->ControlById(ERotorCmdFileName));
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
#ifndef CALYPSO
	RemoveFromStack(iAppView);
#endif
    delete iAppView;
	}


void CRotorAppUi::DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane)
{
	iAppView->DynInitMenuPaneL(aMenuId, aMenuPane);
}

void CRotorAppUi::RestorePreferencesL(TPreferences &aPreferences) const
{
	CDictionaryStore *iniFile = Application()->OpenIniFileLC(iCoeEnv->FsSession());
	TBool found = iniFile->IsPresentL(KUidRotorApp); //replace XXUid with uid of prefs - usually the UID of the main app
	TInt error = KErrNone;

	if (found) 
	{
		RDictionaryReadStream readStream;
		readStream.OpenLC (*iniFile, KUidRotorApp);
		// ignore any reads off the end of the file etc - clear later on

		TRAP(error, aPreferences.iDumpIPv4 = readStream.ReadUint8L());
		TRAP(error, aPreferences.iDumpIPv6 = readStream.ReadUint8L());
		TRAP(error, aPreferences.iDumpIPSEC = readStream.ReadUint8L());
		TRAP(error, aPreferences.iProtocol = readStream.ReadInt16L());
		TRAP(error, aPreferences.iPort = readStream.ReadUint16L());
		TRAP(error, aPreferences.iViewIPHdr = readStream.ReadUint8L());
		TRAP(error, aPreferences.iNumBlades = readStream.ReadInt16L());

		CleanupStack::PopAndDestroy(); // readStream
	}

	CleanupStack::PopAndDestroy(); // iniFile

	if (error!=KErrNone || !found) 
	{
		// missing stream initialise
		CRotorEngine::DefaultPreferences(aPreferences);

		//.... // and whatever is appropriate for all the other fields
		StorePreferencesL(aPreferences); // store the default ones - update inifile
	}
}

void CRotorAppUi::StorePreferencesL(const TPreferences &aPreferences) const
{
	CDictionaryStore *iniFile = Application()->OpenIniFileLC(iCoeEnv->FsSession());
	RDictionaryWriteStream writeStream;
	writeStream.AssignLC(*iniFile, KUidRotorApp);

	writeStream.WriteUint8L(aPreferences.iDumpIPv4);
	writeStream.WriteUint8L(aPreferences.iDumpIPv6);
	writeStream.WriteUint8L(aPreferences.iDumpIPSEC);
	writeStream.WriteInt16L(aPreferences.iProtocol);
	writeStream.WriteUint16L(aPreferences.iPort);
	writeStream.WriteUint8L(aPreferences.iViewIPHdr);
	writeStream.WriteInt16L(aPreferences.iNumBlades);
	//.... // and whatever

	writeStream.CommitL();
	CleanupStack::PopAndDestroy(); // write stream

	// in this replace XXVersionUid with another unique UID - usually the next one up from XXUid
	writeStream.AssignLC(*iniFile, KUidRotorAppUid); // write version 1.0 (major.minor)
	writeStream.WriteInt8L(1); // major
	writeStream.WriteInt8L(0); // minor
	writeStream.CommitL(); // flush
	CleanupStack::PopAndDestroy(); // writeStream;

	// commit changes to the store
	if (iniFile->Commit()!=KErrNone)
		iniFile->RevertL();

	CleanupStack::PopAndDestroy();	//inifile
}

void CRotorAppUi::HandleCommandL(TInt aCommand)
	{
	TPreferences param;

	switch (aCommand)
		{
	// Each command in the toolbar/Tools menu prints an info message
	// using the EIKON InfoMsg() function.
	case ERotorStart:
		//iEikonEnv->InfoMsg(R_ROTOR_COMMAND_1);
		iAppView->StartL();
		break;
	case ERotorStop:
		iAppView->StopL();
		//iEikonEnv->InfoMsg(R_EXAMPLE_COMMAND_2);
		break;
	case ERotorNoRotor:
		iAppView->SwitchRotorL();
		break;

	case ERotorOptions:
		//iEikonEnv->InfoMsg(R_NOT_IMPLEMENTED);
		LaunchOptionsDialogL(iAppView->Model());
		break;
	case ERotorIPv4View:
		LaunchIPv4ViewDialogL(iAppView->Model()->MonIPv4Info());
		break;
	case ERotorIPv6View:
		LaunchIPv6ViewDialogL(iAppView->Model()->MonIPv6Info());
		break;
	case ERotorIPv6ExtView:
		LaunchIPv6ExtViewDialogL(iAppView->Model()->MonIPv6Info());
		break;
/*	case ERotorAHPacketView:
		LaunchAHPacketViewDialog(iAppView->Model()->MonInfo());
		break;*/
	case ERotorClearScreen:
		//iConsole->ClearScreen();	//Only clears visible part of console
		iAppView->ClearScreenL();
		break;
	case ERotorAbout:
		LaunchAboutDialogL();
		break;
	case EEikCmdExit: 
		iAppView->Model()->GetPreferences(param);
		StorePreferencesL(param);
		Exit();
		break;
	default:	//Console Specific Commands
		iAppView->HandleCommandL(aCommand);
		}
	}



// Launches a dialog to ask for new options
TBool CRotorAppUi::LaunchOptionsDialogL(CRotorEngine* aModel) const
{
	COptionsDialog *dialog=new (ELeave) COptionsDialog(aModel);
	TInt button=dialog->ExecuteLD(R_ROTOR_OPTIONS_DIALOG);	//Final D means the dialog is destructed by itself
	return (button==EEikBidOk);	// If button is CANCEL then the algorithm is not added
}

// Launches a dialog to ask for IPv4 view options
TBool CRotorAppUi::LaunchIPv4ViewDialogL(SMonIPv4Info *aMonInfo) const
{
	CIPv4ViewDialog *dialog=new (ELeave) CIPv4ViewDialog(aMonInfo);
	TInt button=dialog->ExecuteLD(R_ROTOR_IPV4_VIEW_DIALOG);	//Final D means the dialog is destructed by itself
	return (button==EEikBidOk);	// If button is CANCEL then the algorithm is not added
}

// Launches a dialog to ask for IPv6 view options
TBool CRotorAppUi::LaunchIPv6ViewDialogL(SMonIPv6Info *aMonInfo) const
{
	CIPv6ViewDialog *dialog=new (ELeave) CIPv6ViewDialog(aMonInfo);
	TInt button=dialog->ExecuteLD(R_ROTOR_IPV6_VIEW_DIALOG);	//Final D means the dialog is destructed by itself
	return (button==EEikBidOk);	// If button is CANCEL then the algorithm is not added
}

// Launches a dialog to ask for IPv6 Extensions view options
TBool CRotorAppUi::LaunchIPv6ExtViewDialogL(SMonIPv6Info *aMonInfo) const
{
	CIPv6ExtViewDialog *dialog=new (ELeave) CIPv6ExtViewDialog(aMonInfo);
	TInt button=dialog->ExecuteLD(R_ROTOR_IPV6EXT_VIEW_DIALOG);	//Final D means the dialog is destructed by itself
	return (button==EEikBidOk);	// If button is CANCEL then the algorithm is not added
}


// Launches a dialog to show an about box
void CRotorAppUi::LaunchAboutDialogL() const
	{
	CEikDialog* dialog = new (ELeave) CEikDialog();
	dialog->ExecuteLD(R_ROTOR_ABOUT);	//Final D means the dialog is destructed by itself
	}
//
//
// Application view class, CRotorAppView
//
//

void CRotorAppView::ConstructL(const TRect& aRect, const CRotorAppUi *aRotorUi)
{
    CreateWindowL();

#if EPOC_SDK < 0x06000000
    SetRectL(aRect);
#else
	SetRect(aRect);
#endif
    iContext=this;
	iBrushStyle=CGraphicsContext::ESolidBrush;
    iBrushColor=KRgbWhite;

    //TFontSpec spec(_L("Swiss"),213);
    //iRotorFont=iCoeEnv->CreateScreenFontL(spec);
	//iRotorText=iCoeEnv->AllocReadResourceL(R_EXAMPLE_TEXT);

	// Component controls initialization
	
	
	//CreateConsoleL(CEikConsoleScreen::ENoInitialCursor);
	InitFontL();
	CreateRotorL();
	CreateFamilyLabelL();
	CreateProtocolLabelL();
	CreateNetLabelL();
	CreateSpeedLabelL();
	CreateStatLabelsL();
	
	iRunning=EFalse;	//engine NOT running
	//Model or engine
	TPreferences param;
	aRotorUi->RestorePreferencesL(param);	//Reads the .ini file
	iModel= new (ELeave) CRotorEngine(this);
	iModel->ConstructL(param);
	iModel->iShowPackets=EFalse;	//Tells the engine to avoid using the console
    ActivateL();
}



void CRotorAppView::InitFontL()
{
	//TFontSpec spec(_L("Swiss"),180);
	TFontSpec spec(_L("Times"),175);
	spec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
	spec.iTypeface.SetIsProportional(EFalse);
	spec.iTypeface.SetIsSerif(ETrue);
	iFont=iCoeEnv->CreateScreenFontL(spec);
}

void CRotorAppView::CreateRotorL()
{
	
	TRect rect=Rect();
	iRotor = new(ELeave) CRotor(&iModel->iPref.iNumBlades);
	CleanupStack::PushL(iRotor);
	/*
	rect.iTl.iX=4*rect.iBr.iX/6;	//  right 1/3 of the screen
	rect.iTl.iY=KLabelHeight; // make it 20 pixels
	rect.iBr.iY-=2*KLabelHeight;
	*/
	rect.iBr.iX=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=KLabelHeight;		// make it 20 pixels
	rect.iBr.iY-=KLabelHeight;

	rect.Shrink(3,3);
	iRotor->ConstructL(rect);
	iRotor->SetContainerWindowL(*this);
	CleanupStack::Pop();
	
}


void CRotorAppView::CreateConsoleL(TInt aFlags)
{
	/*
	iConsole=new(ELeave) CConsoleControl;
	CleanupStack::PushL(iConsole);
	TRect rect=Rect();
	rect.iBr.iX=4*rect.iBr.iX/6 - 2;	//left 4/6 of screen -2 To put a separation on the right of the console
	rect.Shrink(3,3);
	//TSize size(rect.Size());
	//size.iWidth-=50;
	iConsole->SetRectL(rect);
	iConsole->ConstructL(rect.iTl, rect.Size(),aFlags);
	//iConsole->SetContainerWindowL(*this);
	iConsole->SetRectL(rect);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EOn);
	CleanupStack::Pop();
	TBool b=iConsole->UpdateScrollBars();
	iConsole->ConsoleControl()->UpdateArea();
	b=iConsole->UpdateScrollBars();
	iConsole->DrawCursor();
	*/
	iConsole=new(ELeave) CConsoleControl;
	//iConsole->ConstructL(aFlags);
	TRect rect=Rect();
	rect.iBr.iX=4*rect.iBr.iX/6 - 2;	//left 4/6 of screen -2 To put a separation on the right of the console
	rect.Shrink(3,3);
	//rect.iBr.iX=4*rect.iBr.iX/6 - 2;	//left 4/6 of screen -2 To put a separation on the right of the console
	//iConsole->SetRect()
	iConsole->ConstructL(rect.iTl,rect.Size(),aFlags);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
	
}


void CRotorAppView::CreateBigConsoleL(TInt aFlags)
{
	/*
	iConsole=new(ELeave) CConsoleControl;
	CleanupStack::PushL(iConsole);
	TRect rect=Rect();
	rect.iBr.iX=4*rect.iBr.iX/6 - 2;	//left 4/6 of screen -2 To put a separation on the right of the console
	rect.Shrink(3,3);
	//TSize size(rect.Size());
	//size.iWidth-=50;
	iConsole->SetRectL(rect);
	iConsole->ConstructL(rect.iTl, rect.Size(),aFlags);
	//iConsole->SetContainerWindowL(*this);
	iConsole->SetRectL(rect);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EOn);
	CleanupStack::Pop();
	TBool b=iConsole->UpdateScrollBars();
	iConsole->ConsoleControl()->UpdateArea();
	b=iConsole->UpdateScrollBars();
	iConsole->DrawCursor();
	*/
	iConsole=new(ELeave) CConsoleControl;
	//iConsole->ConstructL(aFlags);
	iConsole->ConstructL(Position(),Size(),aFlags);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
	
}

void CRotorAppView::CreateProtocolLabelL()
{
	iProtocolLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();
/*
	rect.iTl.iX=4*rect.iBr.iX/6;	
	rect.iBr.iY=KLabelHeight; // make it 20 pixels
	
*/
	rect.iBr.iX=rect.iBr.iX/5;	
	rect.iBr.iY=KLabelHeight; // make it 20 pixels

	rect.Shrink(3,3);
	iProtocolLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iProtocolLabel->SetRectL(rect);
#else
	iProtocolLabel->SetRect(rect);
#endif
	iProtocolLabel->SetAlignment(EHCenterVCenter); // center text
	iProtocolLabel->SetBufferReserveLengthL(26);
	iProtocolLabel->SetTextL(_L("Not Active\n")); // nice long buffer

	/*
	CFont *font;
	TFontSpec fontSpec=iTypeLabel->Font()->FontSpecInTwips();
	//CTypefaceStore *faceStore= new CTypefaceStore();
	//faceStore->ConstructL();
	//TInt err=GetNearestFontInTwips(font,fontSpec);
	CGraphicsDevice* screenDevice=(iCoeEnv->ScreenDevice());
    screenDevice->GetNearestFontInTwips(font,fontSpec);
	iTypeLabel->SetFont(font);
*/
	//iTypeLabel->SetFontL(); // nice long buffer
	
	//iTypeLabel->ActivateL(); // now ready
}

void CRotorAppView::CreateNetLabelL()
{
	iNetLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();
/*
	rect.iTl.iX=4*rect.iBr.iX/6;	
	rect.iBr.iY=KLabelHeight; // make it 20 pixels
	
*/
	rect.iTl.iX+=rect.iBr.iX/5;		
	rect.iBr.iX=2*rect.iBr.iX/5;	
	rect.iBr.iY=KLabelHeight; // make it 20 pixels

	rect.Shrink(3,3);
	iNetLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iNetLabel->SetRectL(rect);
#else
	iNetLabel->SetRect(rect);
#endif
	iNetLabel->SetAlignment(EHCenterVCenter); // center text
	iNetLabel->SetBufferReserveLengthL(26);
	//iTypeLabel->SetFontL(); // nice long buffer
	iNetLabel->SetTextL(_L("Net DOWN")); // nice long buffer
	//iTypeLabel->ActivateL(); // now ready
}


void CRotorAppView::CreateFamilyLabelL()
{
	iFamilyLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();
/*
	rect.iTl.iX=4*rect.iBr.iX/6;	
	rect.iBr.iY=KLabelHeight; // make it 20 pixels
	
*/
	rect.iTl.iX+=2*rect.iBr.iX/5;
	rect.iBr.iY=KLabelHeight;

	rect.Shrink(3,3);
	iFamilyLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iFamilyLabel->SetRectL(rect);
#else
	iFamilyLabel->SetRect(rect);
#endif
	iFamilyLabel->SetAlignment(EHCenterVCenter); // center text
	iFamilyLabel->SetBufferReserveLengthL(31);
	iFamilyLabel->SetTextL(_L("---")); // nice long buffer
	//iTypeLabel->SetFontL(); // nice long buffer
	
	//iTypeLabel->ActivateL(); // now ready
}

void CRotorAppView::CreateRecvPacketsLabelL()
{
	/*
	iRecvPacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();
	
	//rect.iTl.iX=3*rect.iBr.iX/5;	// Label is in right 1/3 of the screen
	//rect.iTl.iY+=2*KLabelHeight;
	//rect.iBr.iY=3*KLabelHeight; // make it 20 pixels
	
	rect.iBr.iX=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=rect.iBr.iY-KLabelHeight;
	rect.Shrink(3,3);
	
	iRecvPacketsLabel->SetContainerWindowL(*this);
	iRecvPacketsLabel->SetRectL(rect);
	iRecvPacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iRecvPacketsLabel->SetBufferReserveLengthL(26);
	iRecvPacketsLabel->SetTextL(_L(" Packets received: 0")); // nice long buffer
	*/

	iRecvPacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=6*KLabelHeight;
	rect.iBr.iY=7*KLabelHeight;
	rect.Shrink(3,3);
	
	iRecvPacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iRecvPacketsLabel->SetRectL(rect);
#else
	iRecvPacketsLabel->SetRect(rect);
#endif
	iRecvPacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iRecvPacketsLabel->SetBufferReserveLengthL(26);
	iRecvPacketsLabel->SetFont(iFont);
	iRecvPacketsLabel->SetTextL(_L(" Total:  0 packets")); // nice long buffer
}

void CRotorAppView::CreateIPv4PacketsLabelL()
{
	iIPv4PacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=KLabelHeight;
	rect.iBr.iY=2*KLabelHeight;
	rect.Shrink(3,3);
	
	iIPv4PacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iIPv4PacketsLabel->SetRectL(rect);
#else
	iIPv4PacketsLabel->SetRect(rect);
#endif
	iIPv4PacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iIPv4PacketsLabel->SetBufferReserveLengthL(26);
	iIPv4PacketsLabel->SetFont(iFont);
	iIPv4PacketsLabel->SetTextL(_L(" IPv4 :  0 packets")); // nice long buffer
}

void CRotorAppView::CreateIPv6PacketsLabelL()
{
	iIPv6PacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=2*KLabelHeight;
	rect.iBr.iY=3*KLabelHeight;
	rect.Shrink(3,3);
	
	iIPv6PacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iIPv6PacketsLabel->SetRectL(rect);
#else
	iIPv6PacketsLabel->SetRect(rect);
#endif
	iIPv6PacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iIPv6PacketsLabel->SetBufferReserveLengthL(26);
	iIPv6PacketsLabel->SetFont(iFont);
	iIPv6PacketsLabel->SetTextL(_L(" IPv6 :  0 packets")); // nice long buffer
}

void CRotorAppView::CreateTCPPacketsLabelL()
{
	iTCPPacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=3*KLabelHeight;
	rect.iBr.iY=4*KLabelHeight;
	rect.Shrink(3,3);
	
	iTCPPacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iTCPPacketsLabel->SetRectL(rect);
#else
	iTCPPacketsLabel->SetRect(rect);
#endif
	iTCPPacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iTCPPacketsLabel->SetBufferReserveLengthL(26);
	iTCPPacketsLabel->SetFont(iFont);
	iTCPPacketsLabel->SetTextL(_L(" TCP  :  0 packets")); // nice long buffer
}

void CRotorAppView::CreateUDPPacketsLabelL()
{
	iUDPPacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=4*KLabelHeight;
	rect.iBr.iY=5*KLabelHeight;
	rect.Shrink(3,3);
	
	iUDPPacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iUDPPacketsLabel->SetRectL(rect);
#else
	iUDPPacketsLabel->SetRect(rect);
#endif
	iUDPPacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iUDPPacketsLabel->SetBufferReserveLengthL(26);
	iUDPPacketsLabel->SetFont(iFont);
	iUDPPacketsLabel->SetTextL(_L(" UDP  :  0 packets")); // nice long buffer
}

void CRotorAppView::CreateICMPPacketsLabelL()
{
	iICMPPacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=5*KLabelHeight;
	rect.iBr.iY=6*KLabelHeight;
	rect.Shrink(3,3);
	
	iICMPPacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iICMPPacketsLabel->SetRectL(rect);
#else
	iICMPPacketsLabel->SetRect(rect);
#endif
	iICMPPacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iICMPPacketsLabel->SetBufferReserveLengthL(26);
	iICMPPacketsLabel->SetFont(iFont);
	iICMPPacketsLabel->SetTextL(_L(" ICMP :  0 packets")); // nice long buffer
}

void CRotorAppView::CreateExtv6PacketsLabelL()
{
	iExtv6PacketsLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();

	rect.iTl.iX+=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=5*KLabelHeight;
	rect.iBr.iY=6*KLabelHeight;
	rect.Shrink(3,3);
	
	iExtv6PacketsLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iExtv6PacketsLabel->SetRectL(rect);
#else
	iExtv6PacketsLabel->SetRect(rect);
#endif
	iExtv6PacketsLabel->SetAlignment(EHLeftVCenter); // center text
	iExtv6PacketsLabel->SetBufferReserveLengthL(26);
	iExtv6PacketsLabel->SetFont(iFont);
	iExtv6PacketsLabel->SetTextL(_L(" Extv6:  0 packets")); // nice long buffer
}

void CRotorAppView::CreateStatLabelsL()
{
	CreateRecvPacketsLabelL();
	CreateIPv4PacketsLabelL();
	CreateIPv6PacketsLabelL();
	CreateTCPPacketsLabelL();
	CreateUDPPacketsLabelL();
	CreateICMPPacketsLabelL();
	CreateExtv6PacketsLabelL();
	//UpdateStatistics();
}


void CRotorAppView::CreateSpeedLabelL()
{
	iSpeedLabel=new (ELeave) CEikLabel;
	TRect rect=Rect();
	/*
	rect.iTl.iX=3*rect.iBr.iX/5;	// Label is in right 1/3 of the screen
	rect.iTl.iY+=2*KLabelHeight;
	rect.iBr.iY=3*KLabelHeight; // make it 20 pixels
	*/
	rect.iBr.iX=2*rect.iBr.iX/5;	//left 1/3 of the screen
	rect.iTl.iY=rect.iBr.iY-KLabelHeight;
	//rect.iTl.iY=rect.iBr.iY-2*KLabelHeight;
	//rect.iBr.iY-=KLabelHeight; // make it 20 pixels
	rect.Shrink(3,3);
	//rect.Move(0,-3);
	iSpeedLabel->SetContainerWindowL(*this);
#if EPOC_SDK < 0x06000000
    iSpeedLabel->SetRectL(rect);
#else
	iSpeedLabel->SetRect(rect);
#endif
	iSpeedLabel->SetAlignment(EHCenterVCenter); // center text
	iSpeedLabel->SetBufferReserveLengthL(26);
	iSpeedLabel->SetTextL(_L(" Speed: 0.00 (pack/s)")); // nice long buffer
}



void CRotorAppView::ActivateMonitoringL()
{
	iRunning=ETrue;

	if (iRotor)
	{
		UpdateStatisticsL();
		UpdateFamilyLabelL();
		UpdateProtocolLabelL();
	}
	else
	{
		TBuf<50> msg;
		switch (iModel->iPref.iProtocol)
		{
		case ICMP:
			msg=_L("ICMP level");
			break;
		case IP:
			msg=_L("IP level"); 
			break;
		case TCP:
			msg=_L("TCP level");
			break;
		case UDP:
			msg=_L("UDP level");
			break;
		case ESP:
			msg=_L("ESP level");
			break;
		case AH:
			msg=_L("AH level"); 
			break;
		default: 
			msg=_L("Unknown type");
		}

		iConsole->ClearScreen();
		if (iModel->iPref.iDumpIPv4)		//IPv4
		{
			msg.AppendFormat(_L(" (IPv4)"));
		}
		if (iModel->iPref.iDumpIPv6)		//IPv6
		{
			msg.AppendFormat(_L(" (IPv6)"));
		}
		if (iModel->iPref.iDumpIPSEC)		//IPSEC
		{
			msg.AppendFormat(_L(" (IPSEC)"));
		}
		msg.AppendFormat(_L("\n"));
		iConsole->Write(msg);
	}

}

void CRotorAppView::ResetMonitoringL()
{
	iRunning=EFalse;
	if (iRotor)
	{
		UpdateProtocolLabelL();
		UpdateFamilyLabelL();
	}
	else
		iConsole->Write(_L("Not Active"));
}


CRotorAppView::~CRotorAppView()
{

	delete iModel;
	DestroyControls();
	iCoeEnv->ReleaseScreenFont(iFont);
}


// The following two functions have to be implemented for all compound controls.
TInt CRotorAppView::CountComponentControls() const
{
	return (iRotor) ? 12 : 1;	//With or without rotor
}

CCoeControl* CRotorAppView::ComponentControl(TInt aIndex) const
{
	if (iRotor)
	{
		switch (aIndex)
		{
		case 0:
			return iSpeedLabel;
		case 1:
			return iProtocolLabel;	//displays the protocol level scanned
		case 2:
			return iRotor;
		case 3:
			return iNetLabel;
		case 4:
			return iRecvPacketsLabel;
		case 5:
			return iIPv4PacketsLabel;	//displays the number of IPv4  packets received
		case 6 :
			return iIPv6PacketsLabel;	//displays the number of IPv6 packets received
		case 7 :
			return iTCPPacketsLabel;	//displays the number of TCP packets received
		case 8 :
			return iUDPPacketsLabel;	//displays the number of UDP packets received
		case 9 :
			return iICMPPacketsLabel;	//displays the number of ICMP packets received
		case 10 :
			return iICMPPacketsLabel;	//displays the number of ICMP packets received
		case 11 :
			return iFamilyLabel;		//Displays the families/sockets scanned
		default:
			return NULL;
		}
	}
	else
		return iConsole;
}


void CRotorAppView::Draw(const TRect& ) const
{
	CWindowGc& gc = SystemGc();
	
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	TSize penSizeBold(3,3);
	gc.SetPenSize(penSizeBold);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);

	TRect rect;
	/*
	TRect rect=iConsole->Rect();	//Lines around the Console
	rect.Grow(3,3);
	gc.DrawRect(rect);
	*/
	if (iRotor)	//Lines around the Rotor and Label
	{
		rect=iProtocolLabel->Rect();
		rect.Grow(3,3);
		gc.DrawRect(rect);

		rect=iNetLabel->Rect();
		rect.Grow(3,3);
		gc.DrawRect(rect);

		rect=iSpeedLabel->Rect();
		rect.Grow(3,3);
		gc.DrawRect(rect);

		rect = iRotor->Rect();
		rect.Grow(3,3);
		gc.DrawRect(rect);

		rect=Rect();	//around stats
		rect.iTl.iX+=2*rect.iBr.iX/5;
		gc.DrawRect(rect);

		rect=iFamilyLabel->Rect();
		rect.Grow(3,3);
		gc.DrawRect(rect);

	}
	else
	{
		rect=iConsole->Rect();	//Lines around the Console
		rect.Grow(3,3);
		gc.DrawRect(rect);
	}

}


void CRotorAppView::StartL()
{
	TBuf<50> msg;
	TInt err;
	if (iRunning)
	{
		if (!CEikonEnv::QueryWinL(_L("Already Running"),_L("Restart Monitoring?")))
			return;
		else
			StopL();
	}
	
	
	CEikonEnv::Static()->BusyMsgL(R_BUSY);
	
	
	ActivateMonitoringL();
	
	TRAP(err,iModel->StartL());
	if (err!=KErrNone)
	{
		msg.Format(_L("Error initializing: %d"), err);
		ShowError(msg);
		StopL();
	}
	CEikonEnv::Static()->BusyMsgCancel();
}

void CRotorAppView::Status(TInt aId)
{
	iEikonEnv->InfoMsg(aId);
}


void CRotorAppView::Write(const TDesC &aMsg)
{
	if (!iRotor)	//If there's any console
		iConsole->Write(aMsg);
	
}

void CRotorAppView::ShowError(TInt aId)
{
	iEikonEnv->InfoMsg(aId);
}

void CRotorAppView::ShowError(const TDes& msg)
{
	iEikonEnv->InfoMsg(msg);
}

void CRotorAppView::ShowError(const TDes& msg, TInt aErr)
{
	TBuf<100> txt;
	TBuf<100> txt2;

	txt.Format(msg);
	iEikonEnv->GetErrorText(txt2,aErr);
	txt.AppendFormat(txt2);
	iEikonEnv->InfoMsg(txt);
}

void CRotorAppView::StopL()
{
	if (iRunning)
	{
		iModel->Stop();
		ResetMonitoringL();
	}
}

void CRotorAppView::UpdateRotor()
{
	if (iRotor)	//may be "Hidden" (Full screen console)
		iRotor->UpdateRotor();
}

void CRotorAppView::UpdateNetworkL(TBool aUp)
{
	if (iRotor)
	{
		if (aUp)
			iNetLabel->SetTextL(_L("Net UP"));
		else
			iNetLabel->SetTextL(_L("Net DOWN"));
		
		iSpeedLabel->DrawNow();
	}

}

void CRotorAppView::UpdateSpeedL(TReal aSpeed)
{
	if (iRotor)	//may be "Hidden" (Full screen console)
	{
		TBuf<25> buf;
		buf.Format(_L(" Speed: %.2f (pack/s)"), aSpeed);
		iSpeedLabel->SetTextL(buf); 
		iSpeedLabel->DrawNow();
	}

}


void CRotorAppView::UpdateFamilyLabelL()
{
	if (iRotor)	//may be "Hidden" (Full screen console)
	{
		TBuf<50> buf;
		TBool prev=EFalse;
		
		if (iRunning)
		{
			buf.Format(_L("Listening "));
			if (iModel->iPref.iDumpIPv4)
			{
				buf.AppendFormat(_L("IPv4"));
				prev=ETrue;
			}

			if (iModel->iPref.iDumpIPv6)
			{
				if (prev)
					buf.AppendFormat(_L(" + "));
				buf.AppendFormat(_L("IPv6"));
				prev=ETrue;
			}
			if (iModel->iPref.iDumpIPSEC)
			{
				if (prev)
					buf.AppendFormat(_L(" + "));
				buf.AppendFormat(_L(" IPSEC"));
			}
			iFamilyLabel->SetTextL(buf); 
		}
		else
			iFamilyLabel->SetTextL(_L("---")); 

		iFamilyLabel->DrawNow();
	}
}

void CRotorAppView::UpdateProtocolLabelL()
{
	if (iRotor)	//may be "Hidden" (Full screen console)
	{

		TBuf<50> msg;
		if (iRunning)
		{
			switch (iModel->iPref.iProtocol)
			{
			case ICMP:
				msg=_L("ICMP level");
				break;
			case IP:
				msg=_L("IP level"); // nice long buffer
				break;
			case TCP:
				msg=_L("TCP level"); // nice long buffer
				break;
			case UDP:
				msg=_L("UDP level"); // nice long buffer
				break;
			case ESP:
				msg=_L("ESP level"); // nice long buffer
				break;
			case AH:
				msg=_L("AH level"); // nice long buffer
				break;
			default: 
				msg=_L("Unknown type"); // nice long buffer
			}
		}
		else
			msg.Format(_L("Not Active"));
		iProtocolLabel->SetTextL(msg); 
		iProtocolLabel->DrawNow();
	}
}

void CRotorAppView::UpdateStatisticsL()
{
	if (iRotor)	//Only Shown in the RotorView
	{
		TBuf<25> buf;
		buf.Format(_L(" Total:  %d packets"),iModel->iStatInfo.iTotalPackets);
		iRecvPacketsLabel->SetTextL(buf); 
		iRecvPacketsLabel->DrawNow();

		buf.Format(_L(" IPv4 :  %d packets"),iModel->iStatInfo.iIPv4Packets);
		iIPv4PacketsLabel->SetTextL(buf); 
		iIPv4PacketsLabel->DrawNow();

		buf.Format(_L(" IPv6 :  %d packets"),iModel->iStatInfo.iIPv6Packets);
		iIPv6PacketsLabel->SetTextL(buf); 
		iIPv6PacketsLabel->DrawNow();

		buf.Format(_L(" TCP  :  %d packets"),iModel->iStatInfo.iTCPPackets);
		iTCPPacketsLabel->SetTextL(buf); 
		iTCPPacketsLabel->DrawNow();

		buf.Format(_L(" UDP  :  %d packets"),iModel->iStatInfo.iUDPPackets);
		iUDPPacketsLabel->SetTextL(buf); 
		iUDPPacketsLabel->DrawNow();

		buf.Format(_L(" ICMP :  %d packets"),iModel->iStatInfo.iICMPPackets);
		iICMPPacketsLabel->SetTextL(buf); 
		iICMPPacketsLabel->DrawNow();

		buf.Format(_L(" Extv6:  %d packets"),iModel->iStatInfo.iICMPPackets);
		iICMPPacketsLabel->SetTextL(buf); 
		iICMPPacketsLabel->DrawNow();
		
	}

}

void CRotorAppView::DestroyControls()
{
	delete iRotor;
	iRotor=NULL;
	delete iFamilyLabel;
	iFamilyLabel=NULL;
	delete iNetLabel;
	iNetLabel=NULL;
	delete iSpeedLabel;
	iSpeedLabel=NULL;
	delete iProtocolLabel;
	iProtocolLabel=NULL;
	delete iRecvPacketsLabel;
	iRecvPacketsLabel=NULL;
	delete iIPv4PacketsLabel;
	iIPv4PacketsLabel=NULL;
	delete iIPv6PacketsLabel;
	iIPv6PacketsLabel=NULL;
	delete iTCPPacketsLabel;
	iTCPPacketsLabel=NULL;
	delete iUDPPacketsLabel;
	iUDPPacketsLabel=NULL;
	delete iICMPPacketsLabel;
	iICMPPacketsLabel=NULL;
	delete iExtv6PacketsLabel;
	iExtv6PacketsLabel=NULL;

	delete iConsole;
	iConsole=NULL;
}

void CRotorAppView::SwitchRotorL()
{
	if (iRotor)	//If initialized then is on the screen
	{
		DestroyControls();		
		CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);	
		iModel->iShowPackets=ETrue;
	}
	else
	{
		DestroyControls();
		CreateRotorL();
		CreateProtocolLabelL();
		CreateNetLabelL();
		CreateFamilyLabelL();
		CreateStatLabelsL();
		CreateSpeedLabelL();
		UpdateStatisticsL();
		UpdateProtocolLabelL();
		UpdateFamilyLabelL();	
		iModel->iShowPackets=EFalse;
	}
	ActivateL();	//To reactivate all the controls ready to draw
	DrawDeferred();		//To draw the new configuration of the controls
}

void CRotorAppView::ClearScreenL()
{
	if (!iRotor)	//If there's any console
	{
		delete iConsole;
		iConsole=NULL;
		CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);	
	}
	
	/*
	if (iRotor)
		CreateConsoleL(CEikConsoleScreen::ENoInitialCursor);
	else
		CreateBigConsoleL(CEikConsoleScreen::ENoInitialCursor);	
	*/

}

void CRotorAppView::DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane) const
{
	//if (!iRotor)
	//	aMenuPane
	if (aMenuId==R_ROTOR_CONSOLE_MENU)
	{
		if (!iRotor)
			aMenuPane->SetItemButtonState(ERotorNoRotor,EEikMenuItemSymbolOn);
			//aMenuPane->SetItemTextL(ERotorNoRotor,R_SHOW_ROTOR);
	}
}

void CRotorAppView::HandleCommandL(TInt aCommand)
{
	if (iConsole)
		iConsole->HandleCommandL(aCommand);
}

TSize CRotorAppView::ScreenSize() const
{
	__ASSERT_ALWAYS(iConsole != NULL, User::Panic(_L("iConsole"), 0));
	return iConsole->ScreenSize();
}

//
//	CRotor
//
// A control for the Rotor. It'll be created from the main view (container)
// CRotor doesn't need a ConstructL() because it's a simple control.

CRotor::CRotor(TInt *aNumBlades)
{
	iNumBlades=aNumBlades;
}


CRotor::~CRotor()
{
	
}


void CRotor::ConstructL(const TRect& aRect)
{
#if EPOC_SDK < 0x06000000
    SetRectL(aRect);
#else
	SetRect(aRect);
#endif
	TRect rect=aRect;
	rect.Shrink(0,20);	//Shrink a little to fit better to the screen
	//rect.Heigth();
	//rect.Width();
	//Math::Int(iRadi,(rect.iBr.iY-rect.iTl.iY) / 2 );	//Pre calculates the radi to use it later
	iRadi = (rect.Height() > rect.Width()) ? rect.Width()/2 : rect.Height()/2;

	iRotorRect=aRect;
	TPoint center(rect.Center());
	iRotorRect.iTl.iX=center.iX - iRadi;	//To minimize the redrawn part
	iRotorRect.iTl.iY=center.iY - iRadi;
	iRotorRect.iBr.iX=center.iX + iRadi;
	iRotorRect.iBr.iY=center.iY + iRadi;
}


void CRotor::DrawRotor() const
{
	CWindowGc& gc=SystemGc(); // graphics context we draw to

	TRect rect=Rect();
	
	TPoint center=rect.Center();
	

	TPoint start(center.iX,center.iY - iRadi );
	TPoint end(start);
	TRect arcRect(center.iX-iRadi, center.iY-iRadi, center.iX+iRadi, center.iY+iRadi);
	TSize penSizeBold(4,4);
	gc.SetPenSize(penSizeBold);
	//gc.SetPenColor(KRgbGray);
	gc.DrawArc(arcRect, start, end);
	gc.SetPenColor(KRgbBlack);
	penSizeBold.SetSize(10,10);
	gc.SetPenSize(penSizeBold);
	gc.Plot(center);
	DrawBlades();

}


void CRotor::DrawBlades() const
{
	TReal angle=iAngle;	//in degrees
	TReal incr=2*PI / (*iNumBlades);	//radians between each blade
	for (TInt i=0; i< *iNumBlades; i++)
	{
		DrawBlade(Rect().Center(), angle);
		angle+=incr;
	}
}


void CRotor::DrawBlade(const TPoint& aCenter, TReal aAngle) const
{
	CWindowGc& gc=SystemGc(); // graphics context we draw to
	TReal lSin, lCos;
	TInt32 x,y;
	TSize penSizeBold(2,2);
	gc.SetPenSize(penSizeBold);

	gc.SetBrushColor(KRgbBlack);
	
	Math::Sin(lSin,aAngle-0.12);
	Math::Cos(lCos,aAngle-0.12);
	Math::Int(x, iRadi*lCos);
	Math::Int(y, iRadi*lSin);
	TPoint end1(aCenter.iX + x, aCenter.iY + y); 

	Math::Sin(lSin,aAngle+0.12);
	Math::Cos(lCos,aAngle+0.12);
	Math::Int(x, iRadi*lCos);
	Math::Int(y, iRadi*lSin);
	TPoint end2(aCenter.iX + x, aCenter.iY + y); 

	TRect arcRect(aCenter.iX-iRadi, aCenter.iY-iRadi, aCenter.iX+iRadi, aCenter.iY+iRadi);
	gc.DrawPie(arcRect,end2,end1);

	//gc.DrawLine(aCenter, end1);
	//gc.DrawLine(aCenter, end2);
	//gc.DrawLine(end1, end2);
	
	//gc.DrawArc(arcRect, start, end);
}


void CRotor::Draw(const TRect& /*aRect*/) const
{
	CWindowGc& gc=SystemGc(); // graphics context we draw to
	TSize penSizeBold(3,3);
	gc.SetPenSize(penSizeBold);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	//gc.SetBrushColor(KRgbDitheredLightGray);
	//gc.SetBrushColor(TRgb(0xefefef));
	/*
	TRect rect = Rect();
	rect.Grow(3,3);
	gc.DrawRect(rect);
	*/
	DrawRotor();

	/*
	CWindowGc& gc=SystemGc();
	if (IsFocused())
		{
		gc.SetPenColor(KRgbBlack);
		}
	else
		{
		gc.SetPenColor(KRgbWhite);
		}
	gc.DrawRect(Rect());

	gc.SetClippingRect(aRect);

	// Draw the smiley face, smiling or looking sad
	gc.SetPenColor(KRgbBlack);
	// Draw a circle for the face
	gc.DrawEllipse(iSmileyRect);
	// Draw the eyes
	TPoint leftEye(iSmileyWidth/3, iSmileyHeight/3);
	TPoint rightEye(iSmileyWidth*2/3, iSmileyHeight/3);
	gc.SetPenSize(TSize(5,5));
	gc.Plot(iSmileyRect.iTl+leftEye);
	gc.Plot(iSmileyRect.iTl+rightEye);
	//Draw the mouth, smiling or looking sad.
	gc.SetPenSize(TSize(1,1));
	gc.SetPenColor(KRgbWhite);
	if (iSmiling)
		gc.DrawArc(iFrownRect, iFrownRect.iTl+TPoint(iSmileyWidth/2,iFrownRect.Height()/2), 
							  iFrownRect.iTl+TPoint(0,iFrownRect.Height()/2));
	else
		gc.DrawArc(iSmileRect, iSmileRect.iTl+TPoint(0,iSmileRect.Height()/2), 
							  iSmileRect.iTl+TPoint(iSmileyWidth/2,iSmileRect.Height()/2));
	gc.SetPenColor(KRgbBlack);
	if (iSmiling)
		gc.DrawArc(iSmileRect, iSmileRect.iTl+TPoint(0,iSmileRect.Height()/2), 
							  iSmileRect.iTl+TPoint(iSmileyWidth/2,iSmileRect.Height()/2));
	else
		gc.DrawArc(iFrownRect, iFrownRect.iTl+TPoint(iSmileyWidth/2,iFrownRect.Height()/2), 
							  iFrownRect.iTl+TPoint(0,iFrownRect.Height()/2));
*/
}


void CRotor::UpdateRotor()
{
	iAngle+=0.2;
	if (iAngle > 2 *PI)
		iAngle-= 2*PI;
	
	ActivateGc();
	CWindowGc& gc = SystemGc();
	gc.SetClippingRect(iRotorRect);
	//gc.Clear(rect); // clear to brush color
	DrawDeferred();
	//DrawBlades();
	DeactivateGc();
	
}







// DIALOG CLASSES


COptionsDialog::COptionsDialog(CRotorEngine* aModel)
{
	iModel=aModel;
}

COptionsDialog::~COptionsDialog()
{

}
	//TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	//void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	//void ProcessPointerEventL(const TPointerEvent& aPointerEvent);
	//void ReportEventL(MCoeControlObserver::TCoeEvent aEvent);
	//void PrepareForFocusTransitionL();
	//void HandleControlStateChangeL(TInt aControlId);

TBool COptionsDialog::OkToExitL(TInt)
{
	if ((CheckBoxState(ERotorDumpIPv4)==CEikButtonBase::EClear) && 
		(CheckBoxState(ERotorDumpIPv6)==CEikButtonBase::EClear) && 
		(CheckBoxState(ERotorDumpIPSEC)==CEikButtonBase::EClear))
	{
		CEikonEnv::Static()->InfoMsg(_L("Must dump some traffic"));
		TryChangeFocusToL(ERotorDumpIPv4);
		return EFalse;
	}

	iModel->iPref.iProtocol = ChoiceListCurrentItem(ERotorProtocolChoiceList);
	iModel->iPref.iPort = NumberEditorValue(ERotorPortNumEd);
	iModel->iPref.iViewIPHdr = (CheckBoxState(ERotorViewIPHdr)==CEikButtonBase::ESet);
	iModel->iPref.iDumpIPv4 = (CheckBoxState(ERotorDumpIPv4)==CEikButtonBase::ESet);
	iModel->iPref.iDumpIPv6 = (CheckBoxState(ERotorDumpIPv6)==CEikButtonBase::ESet);
	iModel->iPref.iDumpIPSEC = (CheckBoxState(ERotorDumpIPSEC)==CEikButtonBase::ESet);
	iModel->iPref.iNumBlades = NumberEditorValue(ERotorBladesNumEd);

	return ETrue;
}

void COptionsDialog::PreLayoutDynInitL()
{
	if (!iModel->iIPv4Active)
	{
		SetCheckBoxState(ERotorDumpIPv4, CEikButtonBase::EClear);
		SetLineDimmedNow(ERotorDumpIPv4, ETrue);
	}
	else
	{
		if (iModel->iPref.iDumpIPv4)
			SetCheckBoxState(ERotorDumpIPv4, CEikButtonBase::ESet);
		else
			SetCheckBoxState(ERotorDumpIPv4, CEikButtonBase::EClear);
	}
	if (!iModel->iIPv6Active)
	{
		SetCheckBoxState(ERotorDumpIPv6, CEikButtonBase::EClear);
		SetLineDimmedNow(ERotorDumpIPv6, ETrue);
	}
	else
	{
		if (iModel->iPref.iDumpIPv6)
			SetCheckBoxState(ERotorDumpIPv6, CEikButtonBase::ESet);
		else
			SetCheckBoxState(ERotorDumpIPv6, CEikButtonBase::EClear);
	}

	if (!iModel->iIPSECActive)
	{
		SetCheckBoxState(ERotorDumpIPSEC, CEikButtonBase::EClear);
		SetLineDimmedNow(ERotorDumpIPSEC, ETrue);
	}
	else
	{
		if (iModel->iPref.iDumpIPSEC)
			SetCheckBoxState(ERotorDumpIPSEC, CEikButtonBase::ESet);
		else
			SetCheckBoxState(ERotorDumpIPSEC, CEikButtonBase::EClear);
	}
	((CEikChoiceList *)Control(ERotorProtocolChoiceList))->SetArrayL(R_ROTOR_IPV4_HDR_LIST);

	SetChoiceListCurrentItem(ERotorProtocolChoiceList,iModel->iPref.iProtocol);
	SetNumberEditorValue(ERotorPortNumEd, iModel->iPref.iPort);

	if (iModel->iPref.iViewIPHdr)
		SetCheckBoxState(ERotorViewIPHdr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorViewIPHdr, CEikButtonBase::EClear);

	//SetChoiceListCurrentItem(ERotorMode,iModel->iMode);

	SetNumberEditorValue(ERotorBladesNumEd, iModel->iPref.iNumBlades);

	TBool b = ((iModel->iPref.iProtocol!=TCP) && (iModel->iPref.iProtocol!=UDP) && (iModel->iPref.iProtocol!=IP));
	SetLineDimmedNow(ERotorPortNumEd,b);
	MakeLineVisible(ERotorPortNumEd,!b);
}


void COptionsDialog::HandleControlStateChangeL(TInt aControlId)
{
	TInt protocol;	//,mode
	TBool b;
	switch (aControlId)
	{
	case ERotorProtocolChoiceList:
		//Dimms/Undimms the control PacketLimitNum if needed
		protocol=ChoiceListCurrentItem(ERotorProtocolChoiceList);
		b = ((protocol!=TCP) && (protocol!=UDP) && (protocol!=IP));
		SetLineDimmedNow(ERotorPortNumEd,b);	
		MakeLineVisible(ERotorPortNumEd,!b);
		break;
		/*
	case ERotorMode:
		mode=ChoiceListCurrentItem(ERotorMode);
		protocol=ChoiceListCurrentItem(ERotorProtocolChoiceList);
		if (mode==IPv4)	//To change the list of headers to sniff
		{
			((CEikChoiceList *)Control(ERotorProtocolChoiceList))->SetArrayL(R_ROTOR_IPV4_HDR_LIST);
			SetChoiceListCurrentItem(ERotorProtocolChoiceList,protocol);	//Set to the same protocol
		}
		else	//IPv6
		{	
			mode=((CEikChoiceList *)Control(ERotorProtocolChoiceList))->Array()->MdcaCount();
			((CEikChoiceList *)Control(ERotorProtocolChoiceList))->SetArrayL(R_ROTOR_IPV6_HDR_LIST);
			if (protocol>=((CEikChoiceList *)Control(ERotorProtocolChoiceList))->Array()->MdcaCount())
				protocol=0;	
			SetChoiceListCurrentItem(ERotorProtocolChoiceList,protocol);	//Set to the same protocol
		}
		break;
		*/
	default:
		break;
	}
}


//
//	CIPv4ViewDialog
//


CIPv4ViewDialog::CIPv4ViewDialog(SMonIPv4Info *aMonInfo)
{
	iMonInfo=aMonInfo;
}

CIPv4ViewDialog::~CIPv4ViewDialog()
{

}
	//TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	//void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	//void ProcessPointerEventL(const TPointerEvent& aPointerEvent);
	//void ReportEventL(MCoeControlObserver::TCoeEvent aEvent);
	//void PrepareForFocusTransitionL();
	//void HandleControlStateChangeL(TInt aControlId);

void CIPv4ViewDialog::SetPage()
{
#if EPOC_SDK < 0x06000000
	switch (PageSelector()->CurrentPageControlId())
	{
	case ERotorIPPage:
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPHdrLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPTOS, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPTotalLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPId, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPFlags, CEikButtonBase::ESet);
		break;
	case ERotorIPPage2:
		SetCheckBoxState(ERotorIPOffset, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPTTL, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPProtocol, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPChksum, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::ESet);
		break;
	case ERotorICMPPage:
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::ESet);
		break;
	case ERotorTCPPage:
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::ESet);
		break;
	case ERotorTCPPage2:
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::ESet);
		break;
	case ERotorUDPPage:
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::ESet);
		break;
	case ERotorAHPage:
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::ESet);
		break;
	case ERotorESPPage:
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::ESet);
		break;
	}
#endif
}	//lint !e1762 // could probably not made const for ER5

void CIPv4ViewDialog::ClearPage()
{
#if EPOC_SDK < 0x06000000
	switch (PageSelector()->CurrentPageControlId())
	{
	case ERotorIPPage:
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPHdrLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPTOS, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPTotalLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPId, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPFlags, CEikButtonBase::EClear);
		break;
	case ERotorIPPage2:
		SetCheckBoxState(ERotorIPOffset, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPTTL, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPProtocol, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPChksum, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::EClear);
		break;
	case ERotorICMPPage:
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::EClear);
		break;
	case ERotorTCPPage:
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::EClear);
		break;
	case ERotorTCPPage2:
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::EClear);
		break;
	case ERotorUDPPage:
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::EClear);
		break;
	case ERotorAHPage:
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::EClear);
		break;
	case ERotorESPPage:
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::EClear);
		break;
	}
#endif
}	//lint !e1762 // could probably not made const for ER5

TBool CIPv4ViewDialog::OkToExitL(TInt aButton)
{	
	switch (aButton)
	{
	case ESetButton:
		SetPage();
		return EFalse;
	case EClearButton:
		ClearPage();
		return EFalse;
	case EEikBidOk:
	//IPFields

	iMonInfo->iIPVersion =(CheckBoxState(ERotorIPVersion)==CEikButtonBase::ESet);
	iMonInfo->iIPHdrLen =(CheckBoxState(ERotorIPHdrLen)==CEikButtonBase::ESet);
	iMonInfo->iIPTOS =(CheckBoxState(ERotorIPTOS)==CEikButtonBase::ESet);
	iMonInfo->iIPTotalLen =(CheckBoxState(ERotorIPTotalLen)==CEikButtonBase::ESet);
	iMonInfo->iIPId =(CheckBoxState(ERotorIPId)==CEikButtonBase::ESet);
	iMonInfo->iIPFlags =(CheckBoxState(ERotorIPFlags)==CEikButtonBase::ESet);
	iMonInfo->iIPOffset =(CheckBoxState(ERotorIPOffset)==CEikButtonBase::ESet);
	iMonInfo->iIPTTL =(CheckBoxState(ERotorIPTTL)==CEikButtonBase::ESet);
	iMonInfo->iIPProtocol=(CheckBoxState(ERotorIPProtocol)==CEikButtonBase::ESet);
	iMonInfo->iIPChksum=(CheckBoxState(ERotorIPChksum)==CEikButtonBase::ESet);
	iMonInfo->iIPSrcAddr=(CheckBoxState(ERotorIPSrcAddr)==CEikButtonBase::ESet);
	iMonInfo->iIPDstAddr=(CheckBoxState(ERotorIPDstAddr)==CEikButtonBase::ESet);


	//ICMP Fields

	iMonInfo->iICMPType=(CheckBoxState(ERotorICMPType)==CEikButtonBase::ESet);
	iMonInfo->iICMPCode=(CheckBoxState(ERotorICMPCode)==CEikButtonBase::ESet);
	iMonInfo->iICMPChksum=(CheckBoxState(ERotorICMPChksum)==CEikButtonBase::ESet);


	// TCP Fields
	iMonInfo->iTCPSrcPort=(CheckBoxState(ERotorTCPSrcPort)==CEikButtonBase::ESet);
	iMonInfo->iTCPDstPort=(CheckBoxState(ERotorTCPDstPort)==CEikButtonBase::ESet);
	iMonInfo->iTCPSeq=(CheckBoxState(ERotorTCPSeq)==CEikButtonBase::ESet);
	iMonInfo->iTCPAckNum=(CheckBoxState(ERotorTCPAckNum)==CEikButtonBase::ESet);
	iMonInfo->iTCPHdrLen=(CheckBoxState(ERotorTCPHdrLen)==CEikButtonBase::ESet);
	iMonInfo->iTCPFlags=(CheckBoxState(ERotorTCPFlags)==CEikButtonBase::ESet);
	iMonInfo->iTCPHdrWinSize=(CheckBoxState(ERotorTCPHdrWinSize)==CEikButtonBase::ESet);
	iMonInfo->iTCPChksum=(CheckBoxState(ERotorTCPChksum)==CEikButtonBase::ESet);
	iMonInfo->iTCPHdrUrgPtr=(CheckBoxState(ERotorTCPHdrUrgPtr)==CEikButtonBase::ESet);


	//UDP Fields
	iMonInfo->iUDPSrcPort=(CheckBoxState(ERotorUDPSrcPort)==CEikButtonBase::ESet);
	iMonInfo->iUDPDstPort=(CheckBoxState(ERotorUDPDstPort)==CEikButtonBase::ESet);
	iMonInfo->iUDPLen=(CheckBoxState(ERotorUDPLen)==CEikButtonBase::ESet);
	iMonInfo->iUDPChksum=(CheckBoxState(ERotorUDPChksum)==CEikButtonBase::ESet);	
	
	//AH Fields
	iMonInfo->iAHProtocol =(CheckBoxState(ERotorAHProtocol)==CEikButtonBase::ESet);
	iMonInfo->iAHHdrLen =(CheckBoxState(ERotorAHHdrLen)==CEikButtonBase::ESet);
	iMonInfo->iAHSPI =(CheckBoxState(ERotorAHSPI)==CEikButtonBase::ESet);
	iMonInfo->iAHSeq =(CheckBoxState(ERotorAHSeq)==CEikButtonBase::ESet);

	//ESP Fields
	iMonInfo->iESPSPI =(CheckBoxState(ERotorESPSPI)==CEikButtonBase::ESet);
	iMonInfo->iESPSeq =(CheckBoxState(ERotorESPSeq)==CEikButtonBase::ESet);
	return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}

void CIPv4ViewDialog::PreLayoutDynInitL()
{

	//IP Fields

	if (iMonInfo->iIPVersion)
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::EClear);

	if (iMonInfo->iIPHdrLen)
		SetCheckBoxState(ERotorIPHdrLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPHdrLen, CEikButtonBase::EClear);

	if (iMonInfo->iIPTOS)
		SetCheckBoxState(ERotorIPTOS, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPTOS, CEikButtonBase::EClear);

	if (iMonInfo->iIPTotalLen)
		SetCheckBoxState(ERotorIPTotalLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPTotalLen, CEikButtonBase::EClear);

	if (iMonInfo->iIPId)
		SetCheckBoxState(ERotorIPId, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPId, CEikButtonBase::EClear);
	
	if (iMonInfo->iIPFlags)
		SetCheckBoxState(ERotorIPFlags, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPFlags, CEikButtonBase::EClear);

	if (iMonInfo->iIPOffset)
		SetCheckBoxState(ERotorIPOffset, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPOffset, CEikButtonBase::EClear);

	if (iMonInfo->iIPTTL)
		SetCheckBoxState(ERotorIPTTL, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPTTL, CEikButtonBase::EClear);

	if (iMonInfo->iIPProtocol)
		SetCheckBoxState(ERotorIPProtocol, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPProtocol, CEikButtonBase::EClear);

	if (iMonInfo->iIPChksum)
		SetCheckBoxState(ERotorIPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPChksum, CEikButtonBase::EClear);

	if (iMonInfo->iIPSrcAddr)
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::EClear);

	if (iMonInfo->iIPDstAddr)
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::EClear);
	
	//ICMP Fields

	if (iMonInfo->iICMPType)
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::EClear);

	if (iMonInfo->iICMPCode)
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::EClear);
	
	if (iMonInfo->iICMPChksum)
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::EClear);


	//TCP Fields
	
	if (iMonInfo->iTCPSrcPort)
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::EClear);

	if (iMonInfo->iTCPDstPort)
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::EClear);

	if (iMonInfo->iTCPSeq)
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::EClear);

	if (iMonInfo->iTCPAckNum)
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::EClear);
	
	if (iMonInfo->iTCPHdrLen)
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::EClear);
	
	if (iMonInfo->iTCPFlags)
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::EClear);

	if (iMonInfo->iTCPHdrWinSize)
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::EClear);

	if (iMonInfo->iTCPChksum)
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::EClear);

	if (iMonInfo->iTCPHdrUrgPtr)
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::EClear);


	//UDP Fields

	if (iMonInfo->iUDPSrcPort)
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::EClear);

	if (iMonInfo->iUDPDstPort)
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::EClear);

	if (iMonInfo->iUDPLen)
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::EClear);

	if (iMonInfo->iUDPChksum)
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::EClear);
	
	//AH Fields
	if (iMonInfo->iAHProtocol)
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::EClear);

	if (iMonInfo->iAHHdrLen)
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::EClear);

	if (iMonInfo->iAHSPI)
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::EClear);

	if (iMonInfo->iAHSeq)
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::EClear);


	//ESP Fields
	if (iMonInfo->iESPSPI)
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::EClear);

	if (iMonInfo->iESPSeq)
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::EClear);
}


//
//	CAHPacketViewDialog
//

/*
CAHPacketViewDialog::CAHPacketViewDialog(SMonInfo *aMonInfo)
{
	iMonInfo=aMonInfo;
}

CAHPacketViewDialog::~CAHPacketViewDialog()
{

}

TBool CAHPacketViewDialog::OkToExitL(TInt)
{	
	//AH Fields

	iMonInfo->iAHProtocol =(CheckBoxState(ERotorAHProtocol)==CEikButtonBase::ESet);
	iMonInfo->iAHHdrLen =(CheckBoxState(ERotorAHHdrLen)==CEikButtonBase::ESet);
	iMonInfo->iAHSPI =(CheckBoxState(ERotorAHSPI)==CEikButtonBase::ESet);
	iMonInfo->iAHSeq =(CheckBoxState(ERotorAHSeq)==CEikButtonBase::ESet);

	return ETrue;
}

void CAHPacketViewDialog::PreLayoutDynInitL()
{

	//AH Fields

	if (iMonInfo->iAHProtocol)
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::EClear);

	if (iMonInfo->iAHHdrLen)
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::EClear);

	if (iMonInfo->iAHSPI)
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::EClear);

	if (iMonInfo->iAHSeq)
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::EClear);

}
*/

//
//	CIPv6ViewDialog
//


CIPv6ViewDialog::CIPv6ViewDialog(SMonIPv6Info *aMonInfo)
{
	iMonInfo=aMonInfo;
}

CIPv6ViewDialog::~CIPv6ViewDialog()
{

}
	//TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	//void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	//void ProcessPointerEventL(const TPointerEvent& aPointerEvent);
	//void ReportEventL(MCoeControlObserver::TCoeEvent aEvent);
	//void PrepareForFocusTransitionL();
	//void HandleControlStateChangeL(TInt aControlId);

void CIPv6ViewDialog::SetPage()
{
#if EPOC_SDK < 0x06000000
	switch (PageSelector()->CurrentPageControlId())
	{
	case ERotorIPPage:
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPTraffic, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPFlowLabel, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPPayloadLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPNextHdr, CEikButtonBase::ESet);
		break;
	case ERotorIPPage2:
		SetCheckBoxState(ERotorIPHopLimit, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::ESet);
		break;
	case ERotorICMPPage:
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorICMPParameter, CEikButtonBase::ESet);
		break;
	case ERotorTCPPage:
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::ESet);
		break;
	case ERotorTCPPage2:
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorTCPOptions, CEikButtonBase::ESet);
		break;
	case ERotorUDPPage:
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::ESet);
		break;
	}
#endif
}	//lint !e1762 // could probably not made const for ER5

void CIPv6ViewDialog::ClearPage()
{
#if EPOC_SDK < 0x06000000
	switch (PageSelector()->CurrentPageControlId())
	{
	case ERotorIPPage:
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPTraffic, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPFlowLabel, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPPayloadLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPNextHdr, CEikButtonBase::EClear);
		break;
	case ERotorIPPage2:
		SetCheckBoxState(ERotorIPHopLimit, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::EClear);
		break;
	case ERotorICMPPage:
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorICMPParameter, CEikButtonBase::EClear);
		break;
	case ERotorTCPPage:
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::EClear);
		break;
	case ERotorTCPPage2:
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorTCPOptions, CEikButtonBase::EClear);
		break;
	case ERotorUDPPage:
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::EClear);
		break;
	}
#endif
}	//lint !e1762 // could probably not made const for ER5

TBool CIPv6ViewDialog::OkToExitL(TInt aButton)
{	
	switch (aButton)
	{
	case ESetButton:
		SetPage();
		return EFalse;
	case EClearButton:
		ClearPage();
		return EFalse;
	case EEikBidOk:
	//IPFields

	iMonInfo->iIPVersion =(CheckBoxState(ERotorIPVersion)==CEikButtonBase::ESet);
	iMonInfo->iIPTraffic =(CheckBoxState(ERotorIPTraffic)==CEikButtonBase::ESet);
	iMonInfo->iIPFlowLabel =(CheckBoxState(ERotorIPFlowLabel)==CEikButtonBase::ESet);
	iMonInfo->iIPPayloadLen =(CheckBoxState(ERotorIPPayloadLen)==CEikButtonBase::ESet);
	iMonInfo->iIPNextHdr =(CheckBoxState(ERotorIPNextHdr)==CEikButtonBase::ESet);
	iMonInfo->iIPHopLimit =(CheckBoxState(ERotorIPHopLimit)==CEikButtonBase::ESet);
	iMonInfo->iIPSrcAddr =(CheckBoxState(ERotorIPSrcAddr)==CEikButtonBase::ESet);
	iMonInfo->iIPDstAddr=(CheckBoxState(ERotorIPDstAddr)==CEikButtonBase::ESet);


	//ICMP Fields

	iMonInfo->iICMPType=(CheckBoxState(ERotorICMPType)==CEikButtonBase::ESet);
	iMonInfo->iICMPCode=(CheckBoxState(ERotorICMPCode)==CEikButtonBase::ESet);
	iMonInfo->iICMPChksum=(CheckBoxState(ERotorICMPChksum)==CEikButtonBase::ESet);
	iMonInfo->iICMPParameter=(CheckBoxState(ERotorICMPParameter)==CEikButtonBase::ESet);
	

	// TCP Fields
	iMonInfo->iTCPSrcPort=(CheckBoxState(ERotorTCPSrcPort)==CEikButtonBase::ESet);
	iMonInfo->iTCPDstPort=(CheckBoxState(ERotorTCPDstPort)==CEikButtonBase::ESet);
	iMonInfo->iTCPSeq=(CheckBoxState(ERotorTCPSeq)==CEikButtonBase::ESet);
	iMonInfo->iTCPAckNum=(CheckBoxState(ERotorTCPAckNum)==CEikButtonBase::ESet);
	iMonInfo->iTCPHdrLen=(CheckBoxState(ERotorTCPHdrLen)==CEikButtonBase::ESet);
	iMonInfo->iTCPFlags=(CheckBoxState(ERotorTCPFlags)==CEikButtonBase::ESet);
	iMonInfo->iTCPHdrWinSize=(CheckBoxState(ERotorTCPHdrWinSize)==CEikButtonBase::ESet);
	iMonInfo->iTCPChksum=(CheckBoxState(ERotorTCPChksum)==CEikButtonBase::ESet);
	iMonInfo->iTCPHdrUrgPtr=(CheckBoxState(ERotorTCPHdrUrgPtr)==CEikButtonBase::ESet);
	iMonInfo->iTCPOptions=(CheckBoxState(ERotorTCPOptions)==CEikButtonBase::ESet);


	//UDP Fields
	iMonInfo->iUDPSrcPort=(CheckBoxState(ERotorUDPSrcPort)==CEikButtonBase::ESet);
	iMonInfo->iUDPDstPort=(CheckBoxState(ERotorUDPDstPort)==CEikButtonBase::ESet);
	iMonInfo->iUDPLen=(CheckBoxState(ERotorUDPLen)==CEikButtonBase::ESet);
	iMonInfo->iUDPChksum=(CheckBoxState(ERotorUDPChksum)==CEikButtonBase::ESet);
	return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}

void CIPv6ViewDialog::PreLayoutDynInitL()
{

	//IP Fields
	if (iMonInfo->iIPVersion)
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPVersion, CEikButtonBase::EClear);

	if (iMonInfo->iIPTraffic)
		SetCheckBoxState(ERotorIPTraffic, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPTraffic, CEikButtonBase::EClear);

	if (iMonInfo->iIPFlowLabel)
		SetCheckBoxState(ERotorIPFlowLabel, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPFlowLabel, CEikButtonBase::EClear);

	if (iMonInfo->iIPPayloadLen)
		SetCheckBoxState(ERotorIPPayloadLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPPayloadLen, CEikButtonBase::EClear);

	if (iMonInfo->iIPNextHdr)
		SetCheckBoxState(ERotorIPNextHdr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPNextHdr, CEikButtonBase::EClear);
	
	if (iMonInfo->iIPHopLimit)
		SetCheckBoxState(ERotorIPHopLimit, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPHopLimit, CEikButtonBase::EClear);

	if (iMonInfo->iIPSrcAddr)
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPSrcAddr, CEikButtonBase::EClear);

	if (iMonInfo->iIPDstAddr)
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorIPDstAddr, CEikButtonBase::EClear);
	
	//ICMP Fields

	if (iMonInfo->iICMPType)
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPType, CEikButtonBase::EClear);

	if (iMonInfo->iICMPCode)
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPCode, CEikButtonBase::EClear);
	
	if (iMonInfo->iICMPChksum)
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPChksum, CEikButtonBase::EClear);

	if (iMonInfo->iICMPParameter)
		SetCheckBoxState(ERotorICMPParameter, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorICMPParameter, CEikButtonBase::EClear);

	//TCP Fields
	
	if (iMonInfo->iTCPSrcPort)
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPSrcPort, CEikButtonBase::EClear);

	if (iMonInfo->iTCPDstPort)
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPDstPort, CEikButtonBase::EClear);

	if (iMonInfo->iTCPSeq)
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPSeq, CEikButtonBase::EClear);

	if (iMonInfo->iTCPAckNum)
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPAckNum, CEikButtonBase::EClear);
	
	if (iMonInfo->iTCPHdrLen)
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPHdrLen, CEikButtonBase::EClear);
	
	if (iMonInfo->iTCPFlags)
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPFlags, CEikButtonBase::EClear);

	if (iMonInfo->iTCPHdrWinSize)
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPHdrWinSize, CEikButtonBase::EClear);

	if (iMonInfo->iTCPChksum)
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPChksum, CEikButtonBase::EClear);

	if (iMonInfo->iTCPHdrUrgPtr)
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPHdrUrgPtr, CEikButtonBase::EClear);

	if (iMonInfo->iTCPOptions)
		SetCheckBoxState(ERotorTCPOptions, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorTCPOptions, CEikButtonBase::EClear);

	//UDP Fields

	if (iMonInfo->iUDPSrcPort)
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPSrcPort, CEikButtonBase::EClear);

	if (iMonInfo->iUDPDstPort)
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPDstPort, CEikButtonBase::EClear);

	if (iMonInfo->iUDPLen)
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPLen, CEikButtonBase::EClear);

	if (iMonInfo->iUDPChksum)
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorUDPChksum, CEikButtonBase::EClear);

}


//
//	CIPv6ExtViewDialog:	IPv6 Extensions Monitoring Preferences Dialog
//


CIPv6ExtViewDialog::CIPv6ExtViewDialog(SMonIPv6Info *aMonInfo)
{
	iMonInfo=aMonInfo;
}

CIPv6ExtViewDialog::~CIPv6ExtViewDialog()
{

}

void CIPv6ExtViewDialog::SetPage()
{
#if EPOC_SDK < 0x06000000
#define SETCHECKBOX(x) SetCheckBoxState(x, CEikButtonBase::ESet)
	switch (PageSelector()->CurrentPageControlId())
	{
	case ERotorHOPPage:
		SetCheckBoxState(ERotorHOPNextHdr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorHOPHdrExtLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorHOPOptionType, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorHOPOptionLen, CEikButtonBase::ESet);
		break;
	case ERotorDSTPage:
		SetCheckBoxState(ERotorDSTNextHdr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorDSTHdrExtLen, CEikButtonBase::ESet);
		SETCHECKBOX(ERotorDSTHomeAddr);
		SETCHECKBOX(ERotorDSTBindingUpdate);
		SETCHECKBOX(ERotorDSTBindingRequest);
		SETCHECKBOX(ERotorDSTBindingAck);
		SETCHECKBOX(ERotorDSTPad);
		SETCHECKBOX(ERotorDSTUnknown);
		break;
	case ERotorRTPage:
		SetCheckBoxState(ERotorRTNextHdr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorRTHdrExtLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorRTRoutingType, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorRTSegLeft, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorRTSLBitMap, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorRTAddresses, CEikButtonBase::ESet);
		break;
	case ERotorFRAGPage:
		SetCheckBoxState(ERotorFRAGNextHdr, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorFRAGFragOffset, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorFRAGMFlag, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorFRAGId, CEikButtonBase::ESet);
		break;
	case ERotorAHPage:
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::ESet);
		break;
	case ERotorESPPage:
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::ESet);
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::ESet);
		break;
	}
#undef SETCHECKBOX
#endif
}	//lint !e1762 // could probably not made const for ER5

void CIPv6ExtViewDialog::ClearPage()
{
#if EPOC_SDK < 0x06000000
#define CLEARCHECKBOX(x) SetCheckBoxState(x, CEikButtonBase::EClear)
	switch (PageSelector()->CurrentPageControlId())
	{
	case ERotorHOPPage:
		SetCheckBoxState(ERotorHOPNextHdr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorHOPHdrExtLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorHOPOptionType, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorHOPOptionLen, CEikButtonBase::EClear);
		break;
	case ERotorDSTPage:
		SetCheckBoxState(ERotorDSTNextHdr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorDSTHdrExtLen, CEikButtonBase::EClear);
		CLEARCHECKBOX(ERotorDSTHomeAddr);
		CLEARCHECKBOX(ERotorDSTBindingUpdate);
		CLEARCHECKBOX(ERotorDSTBindingRequest);
		CLEARCHECKBOX(ERotorDSTBindingAck);
		CLEARCHECKBOX(ERotorDSTPad);
		CLEARCHECKBOX(ERotorDSTUnknown);
		break;
	case ERotorRTPage:
		SetCheckBoxState(ERotorRTNextHdr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorRTHdrExtLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorRTRoutingType, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorRTSegLeft, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorRTSLBitMap, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorRTAddresses, CEikButtonBase::EClear);
		break;
	case ERotorFRAGPage:
		SetCheckBoxState(ERotorFRAGNextHdr, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorFRAGFragOffset, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorFRAGMFlag, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorFRAGId, CEikButtonBase::EClear);
		break;
	case ERotorAHPage:
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::EClear);
		break;
	case ERotorESPPage:
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::EClear);
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::EClear);
		break;
	}
#undef CLEARCHECKBOX
#endif
}	//lint !e1762 // could probably not made const for ER5

TBool CIPv6ExtViewDialog::OkToExitL(TInt aButton)
{	
	switch (aButton)
	{
	case ESetButton:
		SetPage();
		return EFalse;
	case EClearButton:
		ClearPage();
		return EFalse;
	case EEikBidOk:
	//Hop By Hop Fields

	iMonInfo->iHOPNextHdr =(CheckBoxState(ERotorHOPNextHdr)==CEikButtonBase::ESet);
	iMonInfo->iHOPHdrExtLen =(CheckBoxState(ERotorHOPHdrExtLen)==CEikButtonBase::ESet);
	iMonInfo->iHOPOptionType =(CheckBoxState(ERotorHOPOptionType)==CEikButtonBase::ESet);
	iMonInfo->iHOPOptionLen =(CheckBoxState(ERotorHOPOptionLen)==CEikButtonBase::ESet);

	// Destination Options Hdr
	iMonInfo->iDSTNextHdr =(CheckBoxState(ERotorDSTNextHdr)==CEikButtonBase::ESet);
	iMonInfo->iDSTHdrExtLen =(CheckBoxState(ERotorDSTHdrExtLen)==CEikButtonBase::ESet);
#define CHECKBOXSET(x) (iMonInfo->i##x = (CheckBoxState(ERotor##x)==CEikButtonBase::ESet))
	CHECKBOXSET(DSTHomeAddr);
	CHECKBOXSET(DSTBindingUpdate);
	CHECKBOXSET(DSTBindingRequest);
	CHECKBOXSET(DSTBindingAck);
	CHECKBOXSET(DSTPad);
	CHECKBOXSET(DSTUnknown);
#undef CHECKBOXSET

	//Routing Hdr
	iMonInfo->iRTNextHdr =(CheckBoxState(ERotorRTNextHdr)==CEikButtonBase::ESet);
	iMonInfo->iRTHdrExtLen=(CheckBoxState(ERotorRTHdrExtLen)==CEikButtonBase::ESet);
	iMonInfo->iRTRoutingType=(CheckBoxState(ERotorRTRoutingType)==CEikButtonBase::ESet);
	iMonInfo->iRTSegLeft=(CheckBoxState(ERotorRTSegLeft)==CEikButtonBase::ESet);
	iMonInfo->iRTSLBitMap=(CheckBoxState(ERotorRTSLBitMap)==CEikButtonBase::ESet);
	iMonInfo->iRTAddresses=(CheckBoxState(ERotorRTAddresses)==CEikButtonBase::ESet);

	// Fragment Hdr Fields
	iMonInfo->iFRAGNextHdr=(CheckBoxState(ERotorFRAGNextHdr)==CEikButtonBase::ESet);
	iMonInfo->iFRAGFragOffset=(CheckBoxState(ERotorFRAGFragOffset)==CEikButtonBase::ESet);
	iMonInfo->iFRAGMFlag=(CheckBoxState(ERotorFRAGMFlag)==CEikButtonBase::ESet);
	iMonInfo->iFRAGId=(CheckBoxState(ERotorFRAGId)==CEikButtonBase::ESet);

	
	//AH Fields	
	iMonInfo->iAHProtocol =(CheckBoxState(ERotorAHProtocol)==CEikButtonBase::ESet);
	iMonInfo->iAHHdrLen =(CheckBoxState(ERotorAHHdrLen)==CEikButtonBase::ESet);
	iMonInfo->iAHSPI =(CheckBoxState(ERotorAHSPI)==CEikButtonBase::ESet);
	iMonInfo->iAHSeq =(CheckBoxState(ERotorAHSeq)==CEikButtonBase::ESet);

	//ESP Fields
	iMonInfo->iESPSPI =(CheckBoxState(ERotorESPSPI)==CEikButtonBase::ESet);
	iMonInfo->iESPSeq =(CheckBoxState(ERotorESPSeq)==CEikButtonBase::ESet);
	return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}

}

void CIPv6ExtViewDialog::PreLayoutDynInitL()
{

	//HopByHop Fields

	if (iMonInfo->iHOPNextHdr)
		SetCheckBoxState(ERotorHOPNextHdr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorHOPNextHdr, CEikButtonBase::EClear);

	if (iMonInfo->iHOPHdrExtLen)
		SetCheckBoxState(ERotorHOPHdrExtLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorHOPHdrExtLen, CEikButtonBase::EClear);

	if (iMonInfo->iHOPOptionType)
		SetCheckBoxState(ERotorHOPOptionType, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorHOPOptionType, CEikButtonBase::EClear);

	if (iMonInfo->iHOPOptionLen)
		SetCheckBoxState(ERotorHOPOptionLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorHOPOptionLen, CEikButtonBase::EClear);


	// Destination Options Hdr

	if (iMonInfo->iDSTNextHdr)
		SetCheckBoxState(ERotorDSTNextHdr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorDSTNextHdr, CEikButtonBase::EClear);
	
	if (iMonInfo->iDSTHdrExtLen)
		SetCheckBoxState(ERotorDSTHdrExtLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorDSTHdrExtLen, CEikButtonBase::EClear);
#define CHECKBOXSET(x) \
	if (iMonInfo->i##x) \
	SetCheckBoxState(ERotor##x, CEikButtonBase::ESet); \
	else \
	SetCheckBoxState(ERotor##x, CEikButtonBase::EClear)

	CHECKBOXSET(DSTHomeAddr);
	CHECKBOXSET(DSTBindingUpdate);
	CHECKBOXSET(DSTBindingRequest);
	CHECKBOXSET(DSTBindingAck);
	CHECKBOXSET(DSTPad);
	CHECKBOXSET(DSTUnknown);
#undef CHECKBOXSET

	//Routing Hdr

	if (iMonInfo->iRTNextHdr)
		SetCheckBoxState(ERotorRTNextHdr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorRTNextHdr, CEikButtonBase::EClear);

	if (iMonInfo->iRTHdrExtLen)
		SetCheckBoxState(ERotorRTHdrExtLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorRTHdrExtLen, CEikButtonBase::EClear);
	
	if (iMonInfo->iRTRoutingType)
		SetCheckBoxState(ERotorRTRoutingType, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorRTRoutingType, CEikButtonBase::EClear);

	if (iMonInfo->iRTSegLeft)
		SetCheckBoxState(ERotorRTSegLeft, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorRTSegLeft, CEikButtonBase::EClear);
	
	if (iMonInfo->iRTSLBitMap)
		SetCheckBoxState(ERotorRTSLBitMap, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorRTSLBitMap, CEikButtonBase::EClear);
	
	if (iMonInfo->iRTAddresses)
		SetCheckBoxState(ERotorRTAddresses, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorRTAddresses, CEikButtonBase::EClear);

	// Fragment Hdr Fields
	if (iMonInfo->iFRAGNextHdr)
		SetCheckBoxState(ERotorFRAGNextHdr, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorFRAGNextHdr, CEikButtonBase::EClear);

	if (iMonInfo->iFRAGFragOffset)
		SetCheckBoxState(ERotorFRAGFragOffset, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorFRAGFragOffset, CEikButtonBase::EClear);

	if (iMonInfo->iFRAGMFlag)
		SetCheckBoxState(ERotorFRAGMFlag, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorFRAGMFlag, CEikButtonBase::EClear);

	if (iMonInfo->iFRAGId)
		SetCheckBoxState(ERotorFRAGId, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorFRAGId, CEikButtonBase::EClear);




	//AH Fields
	if (iMonInfo->iAHProtocol)
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHProtocol, CEikButtonBase::EClear);

	if (iMonInfo->iAHHdrLen)
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHHdrLen, CEikButtonBase::EClear);

	if (iMonInfo->iAHSPI)
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHSPI, CEikButtonBase::EClear);

	if (iMonInfo->iAHSeq)
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorAHSeq, CEikButtonBase::EClear);


	//ESP Fields
	if (iMonInfo->iESPSPI)
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorESPSPI, CEikButtonBase::EClear);

	if (iMonInfo->iESPSeq)
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::ESet);
	else
		SetCheckBoxState(ERotorESPSeq, CEikButtonBase::EClear);

}


CHistoryDialog::CHistoryDialog(TInt *aHistory)
{
	iHistory=aHistory;
}
/*
CHistoryDialog::~CHistoryDialog()
{

}
*/
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
/*
void CConsoleControl::ConstructL(TInt aFlags)
	{
	*/
	/*
    CreateWindowL();
    Window().SetShadowDisabled(ETrue);
    Window().SetBackgroundColor(KRgbGray);
    EnableDragEvents();
    SetExtentToWholeScreenL();
	SetBlank();
	*/
/*
    iConsole=new(ELeave) CEikConsoleScreen;
	iConsole->ConstructL(_L("TEST"),aFlags);
	iConsole->SetHistorySizeL(100,0);
	iHistory=100;
	}
*/

void CConsoleControl::ConstructL(const TPoint& aTopLeft, const TSize& aSize, TInt aFlags)
	{
	/*
    CreateWindowL();
    Window().SetShadowDisabled(ETrue);
    Window().SetBackgroundColor(KRgbGray);
    EnableDragEvents();
    SetExtentToWholeScreenL();
	SetBlank();
	*/
	TRect rect(aTopLeft,aTopLeft + aSize.AsPoint());
#if EPOC_SDK < 0x06000000
    SetRectL(rect);
#else
	SetRect(rect);
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
/*
void CConsoleControl::DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane)
	{
	if (aMenuId==R_CONS_OPTIONS_MENU)
		{
		if ( iConsole->Att() & ATT_COLORMASK )
			aMenuPane->SetItemButtonState(EMenuCommandColor,EEikMenuItemSymbolOn);
		else
			{
			if ( iConsole->Att() & ATT_BOLD )
				aMenuPane->SetItemButtonState(EMenuCommandBold,EEikMenuItemSymbolOn);
			if ( iConsole->Att() & ATT_INVERSE )
				aMenuPane->SetItemButtonState(EMenuCommandInverse,EEikMenuItemSymbolOn);
			if ( iConsole->Att() & ATT_ITALIC )
				aMenuPane->SetItemButtonState(EMenuCommandItalic,EEikMenuItemSymbolOn);
			if ( iConsole->Att() & ATT_UNDERLINE )
				aMenuPane->SetItemButtonState(EMenuCommandUnderline,EEikMenuItemSymbolOn);
			}
		}
	
	if (aMenuId==R_CONS_SPECIAL_MENU)
		{
		if (iHighCursor)
			aMenuPane->SetItemButtonState(EMenuCursorSize,EEikMenuItemSymbolOn);
		if (iSmallScreen)
			aMenuPane->SetItemButtonState(EMenuScreenSize,EEikMenuItemSymbolOn);
		}

	if (aMenuId==R_CONS_TOOLS_MENU)
		{
		if (iHideCursor)
			aMenuPane->SetItemButtonState(EMenuCommandHideCursor,EEikMenuItemSymbolOn);
		if (iIgnoreCursor)
			aMenuPane->SetItemButtonState(EMenuCommandIgnoreCursor,EEikMenuItemSymbolOn);
		if (iScrollLock)
			aMenuPane->SetItemButtonState(EMenuCommandScrollLock,EEikMenuItemSymbolOn);
		if (iAllPrintable)
			aMenuPane->SetItemButtonState(EMenuCommandPrintable,EEikMenuItemSymbolOn);
		}

	}
*/
void CConsoleControl::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
        {
		/*
	case ERotorClearScreen:
		//iConsole->ClearScreen();	//Only clears visible part of console

		break;

	case EMenuCommandEditCopy			:
		{
		TRect range = iConsole->Selection();	// get current selected range		
		if (iSelBufPtr) delete iSelBufPtr;		// forget previous selection
		iSelBufPtr = iConsole->RetrieveL(range);
		if (iSelBufPtr)
			{
			TBuf<32> msg;
			msg.Format(_L("%d bytes copied"),iSelBufPtr->Length());
			iEikonEnv->InfoMsg(msg);
			}
		else 
			iEikonEnv->InfoMsg(_L("Nothing to copy..."));
		}
		break;
	case EMenuCommandEditPaste			:
		iConsole->SelectCursor(); // forget current selection...
		if (iSelBufPtr)
			{
			iConsole->Write(*iSelBufPtr);
			iConsole->FlushChars();
			TBuf<32> msg;
			msg.Format(_L("%d bytes pasted"),iSelBufPtr->Length());
			iEikonEnv->InfoMsg(msg);
			}
		else
			{
			iEikonEnv->InfoMsg(_L("Nothing to paste..."));
			}
		break;
		
	case EMenuCommandBold:
    case EMenuCommandItalic:
    case EMenuCommandUnderline:
	case EMenuCommandInverse:
    case EMenuCommandColor:
        ToggleFontStyleAndRedrawL((TMessageControlFontStyle)aCommand);
        break;

	case EMenuScreenSize:
		{
		iSmallScreen = !iSmallScreen;
		if (iSmallScreen)
			iConsole->ConsoleControl()->SetExtentL( TPoint(40,20), TSize(560,200) );
		else
			iConsole->ConsoleControl()->SetExtentL( TPoint(0,0), TSize(640,240) );
		}
		break;
	case EMenuCursorSize:
		{
		iHighCursor = !iHighCursor;
		if (iHighCursor)
			iConsole->SetCursorHeight(100);
		else
			iConsole->SetCursorHeight(20);
		}
		break;
*/
#if EPOC_SDK < 0x06000000
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

/*
	case EMenuCommandHideCursor:
		iHideCursor=!iHideCursor;
		if (iHideCursor)
			iConsole->HideCursor();
		else
			iConsole->DrawCursor();
		break;
    case EMenuCommandIgnoreCursor:
		iConsole->SetKeepCursorInSight(iIgnoreCursor);
		iIgnoreCursor=!iIgnoreCursor;
		break;
    case EMenuCommandScrollLock:
		iScrollLock=!iScrollLock;
		iConsole->SetScrollLock(iScrollLock);
		break;
    case EMenuCommandPrintable:
		iAllPrintable=!iAllPrintable;
		iConsole->SetAllPrintable(iAllPrintable);
		break;
*/		

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

/*
	case EMenuCommandLongLine			:
		TBuf<256> str;
		for (TInt i=0; i<9; i++)
			{
			TBuf<32> tmp;
			tmp.Format(_L("%d abcdefghijklmnopqrstuvwxyz"),i);
			str+=tmp;
			}
		iConsole->Write(str);
		iConsole->FlushChars();
        break;
*/
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
/*
void CConsoleControl::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
	TBuf<128> iMessage;
    iEikonEnv->Format128(iMessage,R_CONS_POINTER_EVENT,aPointerEvent.iType,aPointerEvent.iPosition.iX,aPointerEvent.iPosition.iY);
	iConsole->Write(iMessage);
	iConsole->FlushChars();
    }
*/
/*
TKeyResponse CConsoleControl::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	if (aType!=EEventKey)
		return(EKeyWasConsumed);
    TInt modifiers=aKeyEvent.iModifiers;
    TInt code=aKeyEvent.iCode;
    if (code==CTRL('e'))
        CBaActiveScheduler::Exit();


	TRect range = iConsole->Selection(); // get current selected range
	switch (code)
		{
		case EKeyUpArrow:
			iConsole->Up();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyDownArrow:
			iConsole->Down();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyLeftArrow:
			iConsole->Left();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyRightArrow:
			iConsole->Right();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyEnter: 
			if (!iAllPrintable)
				{
				iConsole->Cr();
				iConsole->Lf();
				}
			break;
		default:
			{
			iConsole->SelectCursor();	// forget previous selection
			TBuf<1> chr;
			chr.Format(_L("%c"),code);
			iConsole->Write(chr);
			iConsole->FlushChars();
			}
			break;
		}
    return(EKeyWasConsumed);
}
*/

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
	/*
	iConsole->UpdateScrollBars();
	iConsole->ConsoleControl()->UpdateArea();
	iConsole->UpdateScrollBars();
	iConsole->Redraw(Rect());
	*/
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


TSize CConsoleControl::ScreenSize() const
{
	return iConsole->ScreenSize();
}
