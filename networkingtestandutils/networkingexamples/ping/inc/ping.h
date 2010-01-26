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
// ping.h - icmp echo client main module
// This software has been implemented in the 6PACK
// project at the Mobile Networks Laboratory (MNW)
// http://www.research.nokia.com/projects/6pack/
//

#if !defined __PINGAPP2_H
#define __PINGAPP2_H

#include <coecntrl.h>
#include <coeccntx.h>
#include <eiklabel.h>
#include <txtrich.h>
#include <eikrted.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdialg.h>
#include <eikdoc.h>
#include <eikconso.h>
#ifdef CALYPSO
#include <AknQueryDialog.h>
#include <AknSettingItemList.h>
#include <AknAppUi.h>
#endif

#define IPv4	0
#define IPv6	1

//#define NEWLINE	"\b"	// \b is used instead of the typical \n

// UID of app


const TUid KUidPingApp = { 0x10000361 } ;
const TUid KUidPingVersionUid = { 0x10000362 } ;

#ifdef CALYPSO

class CPingBinaryPopupSettingItem : public CAknBinaryPopupSettingItem
{
    public:
        CPingBinaryPopupSettingItem( TInt aIdentifier, TBool& aBinaryValue );

        virtual void LoadL();
};

class CPingTextSettingItem : public CAknSettingItem
{
    public:
        CPingTextSettingItem( TInt aIdentifier, TDes& aText );
        CPingTextSettingItem::~CPingTextSettingItem();
        virtual void StoreL();
        virtual void LoadL(); // Called from 2nd stage constructors
        virtual const TDesC& SettingTextL();
        virtual void EditItemL( TBool aCalledFromMenu );

    protected:
        TPtr& InternalTextPtr();
        TPtrC ExternalText();
        void SetExternalText( TDesC& aNewExternalText );

    private:
        TDes& iExternalText;
        HBufC* iInternalText;
        TPtr iInternalTextPtr;
};

class CSettingView : public CAknSettingItemList, public MEikCommandObserver
{
    public:
        CSettingView(CPing *aPingModel);
        ~CSettingView();
        void ConstructL(const TRect& aRect);
        void ProcessCommandL(TInt aCommand);
        CAknSettingItem* CreateSettingItemL( TInt identifier );
        TBool SaveSettingsL();

    private:
        TBool    iLimitPacketCount;        // Indicates if unlimited packet number
        TBuf<5>  iMaxPacketCount;          // Number of ICMP Echo Request packets to send
        TBuf<5>  iPacketDataSize;          // Default data size (not including ICMP header)
        TBuf<5>  iWaitTime;                // Time between sent packets (default 1 second)
        TBuf<5>  iLastWaitTime;            // Time to wait for the last packet (default 2 seconds)
        TBuf<MAX_PATTERN_LENGTH> iPattern; // Pattern to fill the packet (default 0xFF)
        TBool    iQuiet;                   // No packet info, just statistics
        TBool    iVerbose;                 // Verbose Output. All ICMP packets, not only Echo reply
        TBool    iDebug;                   // Set the SO_DEBUG flag in the socket
		
#ifdef IAPSETTING
		TBuf<MAX_IAP_LENGTH> iIAP;		
#endif
        CPing *iPingModel;
};

#endif // #ifdef CALYPSO

// 
// CSimpleConsole
//
/*
enum TMessageControlFontStyle
{
    EStyleElementBold=EMenuCommandBold,
    EStyleElementItalic=EMenuCommandItalic,
    EStyleElementInverse=EMenuCommandInverse,
    EStyleElementUnderline=EMenuCommandUnderline,
    EStyleElementColor=EMenuCommandColor
};
*/
class CConsoleControl : public CCoeControl
{
public:
	CConsoleControl() {}
	~CConsoleControl();
	//void ConstructL(TInt aFlags);
	//void ConstructL(TPoint aLeftTop,const TSize& aSize,TInt aFlags,CCoeControl *aParent);
	void ConstructL(TPoint aLeftTop,const TSize& aSize,TInt aFlags);
    //TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
    //void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	//void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
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
	TSize ScreenSize();
protected:
    void FocusChanged(TDrawNow aDrawNow);
private:
    //void ToggleFontStyleAndRedrawL(TMessageControlFontStyle aStyleElement);
	
private:
	CEikConsoleScreen* iConsole;
	TInt iHistory;
	//TInt iAllPrintable,iScrollLock,iIgnoreCursor,iHideCursor;
	//TDesC* iSelBufPtr;
	//TInt iSmallScreen;
	//TInt iHighCursor;
};


class CPingContainer: public CCoeControl, public MCoeControlObserver
{
public:
	  // Construction
	CPingContainer(CPing* aPingModel);
	void ConstructL(const TRect& aRect);

	  // Destruction
	~CPingContainer();
	//void ConstructFromResourceL(TResourceReader& aReader);
	//void CSmileyContainer::PrepareForFocusLossL();
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	void WriteHostL(TDes& aHostname);
	void UpdateStatisticsL();
	void WriteLine(const TDesC& abuf);
	void ResetScreen();

//protected:
	//void FocusChanged(TDrawNow aDrawNow);
private:
	  // Virtual, defined by CCoeControl; replaces the default implementation
	  // provided by CCoeControl.
	void         Draw(const TRect& aRect) const;
	
	  // Virtual, defined by CCoeControl; replaces the default implementation
	  // provided by CCoeControl. 
    TInt         CountComponentControls() const;

	  // Virtual, defined by CCoeControl; replaces the default implementation
	  // provided by CCoeControl.
	CCoeControl* ComponentControl(TInt aIndex) const;

	  // Virtual, defined by CCoeControl; empty implementation provided by
	  // CCoeControl; full implementation provided by this class
	//void         SizeChangedL();
	
	  // Defined as pure virtual by the mixin class MCoeControlObserver 
	  // inherited by CCoeControl. An empty implementation provided by 
	  // this class (its containees do not report events).
	void         HandleControlEventL(CCoeControl* aControl,
		                             TCoeEvent aEventType);

	void CreateConsoleL(TRect aRect);
private:
	  // Member functions defined and used by this class
	//void		SwapFocus(CCoeControl* aControl);
private:
      // Data members defined and used by this class.
	CPing *iPingModel;
	CEikLabel* iLabel; // label for status messages
	CEikLabel* iLabel2; // label for status messages
	
	CConsoleControl* iConsole;
	//CEikRichTextEditor *iEdit;
	//CRichText *iRtxt;
	};






//
// class CPingView
//

class CPingView : public CCoeControl, public MCoeControlBrushContext
    {
public:
	CPingView(CPing *aPingModel);
    void ConstructL(const TRect& aRect);
    ~CPingView();
	// changing view
	void ConstructViewL();
	void ResetScreen();
	// various types of update


private: // from CCoeControl
	void Draw(const TRect& /*aRect*/) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;
private: // phased update stuff
	
public: // also from MGraphicsExampleObserver
	//void NotifyStatus(const TDesC& aMessage);
	//void NotifyStatus2(const TDesC& aMessage);
private: // new function
	void CreateLabelL();
private: // data
	//CPingAppView* iView; // view of boss puzzle
	//CEikLabel* iLabel; // label for status messages
	//CEikEdwin* iEdit; // label for status console
	//TBossPuzzle* iModel; // model we're working with
	CPing *iPingModel;
	CPingContainer* iContainer;
    };

#ifdef CALYPSO
class CHostNameDialog : public CAknTextQueryDialog
#else
class CHostNameDialog : public CEikDialog
#endif
{
public:
#ifdef CALYPSO
    CHostNameDialog(TDes& aHostname);
#else    
    CHostNameDialog(CPing *aPingModel);
#endif
	
private:
	TBool OkToExitL(TInt aButton);
	void PreLayoutDynInitL();
private:
	TBool iOKPressed;
	CPing *iPingModel;
};

#ifndef CALYPSO

class COptionsDialog : public CEikDialog
{
public:
	COptionsDialog(CPing *aPingModel);
private:
	~COptionsDialog();
	void HandleControlStateChangeL(TInt aControlId);
	TBool OkToExitL(TInt);
	void PreLayoutDynInitL();
private:
	CPing *iPingModel;
};

#endif // #ifndef CALYPSO

//
// CPingAppUi
//

#ifdef CALYPSO
class CPingAppUi : public CAknAppUi
#else
class CPingAppUi : public CEikAppUi
#endif
    {
public:
    void ConstructL();
	~CPingAppUi();

#ifdef CALYPSO

private:
    TBool iAppViewOnStack;
    TBool iSettingViewOnStack;
    void AppViewToStackL();
    void AppViewFromStack();
    void SettingViewToStackL();
    void SettingViewFromStack();
    void ShowAppViewL();
    void ShowSettingViewL();
    CSettingView* iSettingView;

#endif // #ifdef CALYPSO
    
private: // from CEikAppUi
	static TInt Launcher(TAny* x);
	void InitModelL();
	void HandleCommandL(TInt aCommand);
	void CreateOptionsDialogL();
	TBool CreateHostNameDialogL();
	void CreateAboutDialogL();
	void RestorePreferencesL(TPreferences &aPreferences);
	void StorePreferencesL(const TPreferences &aPreferences);
	//void UnDimStop();
	//void DimStop();

private:
    CPingView* iAppView;
	CPing *iPingModel;	//contains all Ping related Data
	//TBossPuzzle* iModel;
    };

//
// CExampleShellDocument
//

class CPingDocument : public CEikDocument
	{
public:
	CPingDocument(CEikApplication& aApp);
	//CPingDocument(CEikApplication& aApp): CEikDocument(aApp) { }
	//TBossPuzzle* Model() { return(&iModel); }
private: // from CEikDocument
	CEikAppUi* CreateAppUiL();
private:
	//TBossPuzzle iModel;
	};

//
// CPingApplication
//

class CPingApplication : public CEikApplication
	{
private: // from CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};

#endif
