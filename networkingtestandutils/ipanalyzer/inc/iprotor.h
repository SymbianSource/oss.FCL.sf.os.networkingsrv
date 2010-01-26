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
// iprotor.h - protocol analyzer main module header
//



/**
 @internalComponent
*/

#ifndef __IPROTOR_H
#define __IPROTOR_H

#include <coecntrl.h>
#include <coeccntx.h>
#include <coemain.h>

#if EPOC_SDK < 0x06000000
#include <eikcmds.hrh>
#include <eiktbar.h>
#else
#include <eiklabel.h>
#include <eikmenup.h>
#endif

#include <eikfnlab.h>
#include <eikenv.h>
#include <eikdoc.h>
#include <eikappui.h>
#include <eikconso.h>
#include <eikapp.h>
#include <eikdialg.h>

#include "help.h"
#include "engine.h"
#include <iprotor.rsg>
#include "iprotor.hrh"


const TUid KUidRotorApp={ 0x10000894 };
const TUid KUidRotorAppUid={ 0x10000894 };

#define MAX_NUM_BLADES	10
#define DEFAULT_NUM_BLADES	3

//
//	CRotor
//
// A control for the Rotor. It'll be created from the main view (container)
// CRotor doesn't need a ConstructL() because it's a simple control.

class CRotor : public CCoeControl
{
public:
	CRotor(TInt *aNumBlades);
	~CRotor();
	void ConstructL(const TRect& aRect);
	void UpdateRotor();

private:
	void DrawRotor() const;
	void DrawBlades() const;
	void DrawBlade(const TPoint& aCenter, TReal aAngle) const;
	void Draw(const TRect& aRect) const;

// members
private:
	TRect iRotorRect;	//Use to minimize operations in redrawings
	TInt *iNumBlades;	//is a * to the engine to be updated at the same time.
	TInt32 iRadi;
	TReal iAngle;
};



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
	//void ConstructL(TInt aFlags);
	//void ConstructL(TPoint aLeftTop,const TSize& aSize,TInt aFlags,CCoeControl *aParent);
	void ConstructL(const TPoint& aTopLeft, const TSize& aSize, TInt aFlags);
    //TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
    //void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	//void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
    void HandleCommandL(TInt aCommand);
    void ActivateL();
	void SetScrollBarVisibilityL(CEikScrollBarFrame::TScrollBarVisibility aHBarVisibility, CEikScrollBarFrame::TScrollBarVisibility aVBarVisibility);
	void DrawCursor();
	void Write(const TDesC &aDes);
	CEikConsoleControl *ConsoleControl() const;
	TBool UpdateScrollBars();
	void ClearScreen();
	void Redraw(const TRect &aRect);
	void Lf();
	TSize ScreenSize() const;
protected:
    void FocusChanged(TDrawNow aDrawNow);
private:
    void ToggleFontStyleAndRedrawL(TMessageControlFontStyle aStyleElement);
	
private:
	CEikConsoleScreen* iConsole;
	TInt iHistory;
	//TInt iAllPrintable,iScrollLock,iIgnoreCursor,iHideCursor;
	//TDesC* iSelBufPtr;
	//TInt iSmallScreen;
	//TInt iHighCursor;
	};








class CRotorDocument;
class CRotorAppUi;
//
// class CRotorAppView
//

class CRotorAppView : public CCoeControl, public MCoeControlBrushContext
    {
public:
	~CRotorAppView();
    void ConstructL(const TRect& aRect, const CRotorAppUi *aRotorUi);
	void UpdateRotor();
	void UpdateNetworkL(TBool aUp);
	void UpdateSpeedL(TReal aSpeed);
	void UpdateStatisticsL();
	void StartL();
	void StopL();
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Write(const TDesC &aDes);
	inline CRotorEngine *Model() 
		{ return iModel;}
	inline CConsoleControl *Console() 
		{ return iConsole;}
	
	void SwitchRotorL();
	void ClearScreenL();
	void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane) const;
	void ShowError(TInt aId);
	void ShowError(const TDes& msg, TInt aErr);
	void Status(TInt aId);
	void HandleCommandL(TInt aCommand);
	TSize ScreenSize() const;

private: // from CCoeControl
	void Draw(const TRect& /*aRect*/) const;


	//Font used in the labels
	void InitFontL();

	//Component Controls
	void CreateRotorL();
	void CreateFamilyLabelL();
	void CreateNetLabelL();
	void CreateStatLabelsL();
	void CreateProtocolLabelL();
	void CreateRecvPacketsLabelL();
	void CreateIPv4PacketsLabelL();
	void CreateIPv6PacketsLabelL();
	void CreateTCPPacketsLabelL();
	void CreateUDPPacketsLabelL();
	void CreateICMPPacketsLabelL();
	void CreateExtv6PacketsLabelL();
	void CreateSpeedLabelL();
	void CreateConsoleL(TInt aFlags);
	void CreateBigConsoleL(TInt aFlags);

	void UpdateFamilyLabelL();
	void UpdateProtocolLabelL();
	void ActivateMonitoringL();
	void ResetMonitoringL();
	void ShowError(const TDes &msg);
	void DestroyControls();
	

private:
	CRotor *iRotor;
	CEikLabel *iFamilyLabel;	//displays the type of the packets being monitored
	CEikLabel *iNetLabel;	//displays the network state
	CEikLabel *iSpeedLabel;	//displays the speed of the packets being monitored
	CEikLabel *iProtocolLabel;	//displays the type of the packets being monitored
	CEikLabel *iRecvPacketsLabel;	//displays the number of the packets received
	CEikLabel *iIPv4PacketsLabel;	//displays the number of IPv4  packets received
	CEikLabel *iIPv6PacketsLabel;	//displays the number of IPv6 packets received
	CEikLabel *iTCPPacketsLabel;	//displays the number of TCP packets received
	CEikLabel *iUDPPacketsLabel;	//displays the number of UDP packets received
	CEikLabel *iICMPPacketsLabel;	//displays the number of ICMP packets received
	CEikLabel *iExtv6PacketsLabel;	//displays the number of Extension headers (IPv6) packets received

	CFont *iFont;				//Font used in all the labels;
	CConsoleControl* iConsole;
	
	CRotorEngine *iModel;
	TBool iRunning;
/*
	CEikRichTextEditor *iEdit;	//For the console
	CRichText *iRtxt;
	HBufC *iData;
*/

//	CFont*	iRotorFont;
//	TDesC*  iRotorText;
	
    };

//
// CRotorAppUi
//

class CRotorAppUi : public CEikAppUi
    {
public:
    void ConstructL();
	~CRotorAppUi();
	void RestorePreferencesL(TPreferences &aPreferences) const;
	void StorePreferencesL(const TPreferences &aPreferences) const;

private: // from CEikAppUi
	void HandleCommandL(TInt aCommand);
	TBool LaunchOptionsDialogL(CRotorEngine* aModel) const;
	TBool LaunchIPv4ViewDialogL(SMonIPv4Info *aMonInfo) const;
	TBool LaunchIPv6ViewDialogL(SMonIPv6Info *aMonInfo) const;
	TBool LaunchIPv6ExtViewDialogL(SMonIPv6Info *aMonInfo) const;

	void LaunchAboutDialogL() const;
	void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
private:
    CRotorAppView* iAppView;
    };

//
// CRotorDocument
//

class CRotorDocument : public CEikDocument
	{
public:
	// construct/destruct
	CRotorDocument(CEikApplication& aApp);
private: // from CEikDocument
	CEikAppUi* CreateAppUiL();
	};

//
// CRotorApplication
//

class CRotorApplication : public CEikApplication
	{
private: // from CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};

// DIALOG CLASSES


class COptionsDialog : public CEikDialog
{
public:
	COptionsDialog(CRotorEngine* aModel);
private:
	~COptionsDialog();
	void HandleControlStateChangeL(TInt aControlId);
	//TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	//void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	//void ProcessPointerEventL(const TPointerEvent& aPointerEvent);
	//void ReportEventL(MCoeControlObserver::TCoeEvent aEvent);
	//void PrepareForFocusTransitionL();
	TBool OkToExitL(TInt);
	void PreLayoutDynInitL();
private:
	CRotorEngine* iModel;
};


class CIPv4ViewDialog : public CEikDialog
{
public:
	//CIPViewDialog(SIPView *AIPView);
	CIPv4ViewDialog(SMonIPv4Info *aMonInfo);
private:
	~CIPv4ViewDialog();
	TBool OkToExitL(TInt aButton);
	void PreLayoutDynInitL();
	void SetPage();
	void ClearPage();

private:
	SMonIPv4Info *iMonInfo;
};


class CIPv6ViewDialog : public CEikDialog
{
public:
	//CIPViewDialog(SIPView *AIPView);
	CIPv6ViewDialog(SMonIPv6Info *aMonInfo);
private:
	~CIPv6ViewDialog();
	TBool OkToExitL(TInt aButton);
	void PreLayoutDynInitL();
	void SetPage();
	void ClearPage();

private:
	SMonIPv6Info *iMonInfo;
};

class CIPv6ExtViewDialog : public CEikDialog
{
public:
	CIPv6ExtViewDialog(SMonIPv6Info *aMonInfo);
private:
	~CIPv6ExtViewDialog();
	TBool OkToExitL(TInt aButton);
	void PreLayoutDynInitL();
	void SetPage();
	void ClearPage();

private:
	SMonIPv6Info *iMonInfo;
};

class CHistoryDialog : public CEikDialog
{
public:
	CHistoryDialog(TInt *aHistory);
private:
	TBool OkToExitL(TInt);
	void PreLayoutDynInitL();

private:
	TInt *iHistory;
};

class CAHPacketViewDialog : public CEikDialog
{
public:
	//CIPViewDialog(SIPView *AIPView);
	CAHPacketViewDialog(SMonIPv4Info *aMonInfo);
private:
	~CAHPacketViewDialog();
	TBool OkToExitL(TInt);
	void PreLayoutDynInitL();

private:
	SMonIPv4Info *iMonInfo;
};

#endif
