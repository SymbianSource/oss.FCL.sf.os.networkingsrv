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
// udpecho.h - udp echo main module
//



/**
 @internalComponent
*/
#ifndef __UDPECHO_H
#define __UDPECHO_H

#include <coecntrl.h>
#include <coeccntx.h>
#include <coemain.h>

#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikenv.h>
#include <eikfnlab.h>
#include <eiktbar.h>
#include <eikconso.h>
#include <eikdialg.h>
#if EPOC_SDK >= 0x06000000
#       include <techview/eikon.hrh>
#else
#       include <eikdialg.hrh>
#       include <eikedwin.hrh>
#       include <eikcmds.hrh>
#endif

#include "help.h"
#include <udpecho.rsg>
#include "udpecho.hrh"
#include "engine.h"

#ifdef CALYPSO
#include <AknQueryDialog.h>
#include <AknSettingItemList.h>
#include <AknAppUi.h>
#endif

const TUid KUidEchoApp={ 0x01022001 };

#ifdef CALYPSO

class CEchoBinaryPopupSettingItem : public CAknBinaryPopupSettingItem
{
    public:
        CEchoBinaryPopupSettingItem( TInt aIdentifier, TBool& aBinaryValue );

        virtual void LoadL();
};

class CEchoTextSettingItem : public CAknSettingItem
{
    public:
        CEchoTextSettingItem( TInt aIdentifier, TDes& aText );
        ~CEchoTextSettingItem();
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
        CSettingView(CUdpEchoEngine *aEngine);
        ~CSettingView();
        void ConstructL(const TRect& aRect);
        void ProcessCommandL(TInt aCommand);
        CAknSettingItem* CreateSettingItemL( TInt identifier );
        TBool SaveSettingsL();

    private:
        TBool    iProtocol;
	TUint	iAction;
	TBuf<5>  iPort;     

        CUdpEchoEngine *iEngine;
};

#endif // #ifdef CALYPSO

class CConsoleControl : public CCoeControl
	{
public:
	CConsoleControl() {}
	~CConsoleControl();
	void ConstructL(const TPoint& aTopLeft, const TSize& aSize, TInt aFlags);
    void HandleCommandL(TInt aCommand);
    void ActivateL();
	void DrawCursor();
	void Write(const TDesC &aDes);
	CEikConsoleControl *ConsoleControl() const;
	TBool UpdateScrollBars();
	void ClearScreen();
	void Redraw(const TRect &aRect);
	void Lf();
protected:
    void FocusChanged(TDrawNow aDrawNow);
private:
	CEikConsoleScreen* iConsole;
	TInt iHistory;
	};


//
// class CUdpEchoView
//

class CUdpEchoView : public CCoeControl, public MCoeControlBrushContext
    {
public:
	~CUdpEchoView();
    void ConstructL(const TRect& aRect);
	
	void StartL();
	void StopL();
	void Write(const TDesC &aDes);
	inline CUdpEchoEngine *Engine()
		{ return iEngine;}
	inline CConsoleControl *Console()
		{ return iConsole;}
	void ClearScreenL();
    void UpdateBusyMsgL();

private: // from CCoeControl
	void Draw(const TRect& /*aRect*/) const;

	//Component Controls
	void CreateBigConsoleL(TInt aFlags);

private:

	CConsoleControl *iConsole;
	CUdpEchoEngine *iEngine;

    };

//
// CUdpEchoAppUi
//

#ifdef CALYPSO
class CUdpEchoAppUi : public CAknAppUi
#else
class CUdpEchoAppUi : public CEikAppUi
#endif
    {
public:
    void ConstructL();
	~CUdpEchoAppUi();


private:
    TBool iAppViewOnStack;
    TBool iSettingViewOnStack;
    void AppViewToStackL();
    void AppViewFromStack();

#ifdef CALYPSO
    void SettingViewToStackL();
    void SettingViewFromStack();
    void ShowAppViewL();
    void ShowSettingViewL();
    CSettingView* iSettingView;
#endif // #ifdef CALYPSO
    
private: // from CEikAppUi
	void HandleCommandL(TInt aCommand);
	TBool LaunchOptionsDialogL(CUdpEchoEngine* aEngine) const;
	void LaunchAboutDialogL() const;
	//void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
private:
    CUdpEchoView* iAppView;
    };


//
// CUdpEchoDocument
//

class CUdpEchoDocument : public CEikDocument
	{
public:
	// construct/destruct
	CUdpEchoDocument(CEikApplication& aApp);
private: // from CEikDocument
	CEikAppUi* CreateAppUiL();
	};

//
// CUdpEchoApplication
//

class CUdpEchoApplication : public CEikApplication
	{
private: // from CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};


#ifndef CALYPSO

// DIALOG CLASSES

class COptionsDialog : public CEikDialog
{
public:
	COptionsDialog(CUdpEchoEngine* aEngine);
private:
	TBool OkToExitL(TInt);
	void PreLayoutDynInitL();
private:
	CUdpEchoEngine* iEngine;
};

#endif // #ifndef CALYPSO

#endif
