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
// udpsend.h - udp packet sender main module
//



/**
 @internalComponent
*/
#ifndef __UDPSEND_H
#define __UDPSEND_H

#include <coecntrl.h>
#include <coeccntx.h>
#include <coemain.h>

#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>

//#include <eikcmds.hrh>

#if EPOC_SDK >= 0x06000000
#       include <techview/eikon.hrh>
#else
//#       include <eikdialg.hrh>
//#       include <eikedwin.hrh>
#       include <eikcmds.hrh>
#endif


#include <eikenv.h>
#include <eikfnlab.h>
#include <eiktbar.h>
#include <eikdialg.h>
#include <eikconso.h>

#if EPOC_SDK < 0x06000000
#include <netdial.h>
#endif

#include "help.h"
#include "engine.h"
#include <udpsend.rsg>
#include "udpsend.hrh"


const TUid KUidRotorApp={ 0x01000534 };




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

//
// class CUDPSendView
//

class CUDPSendView : public CCoeControl, public MCoeControlBrushContext
    {
public:
    ~CUDPSendView();
    void ConstructL(const TRect& aRect);
    
    void StartL();
    void Stop();
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;
    void Write(const TDesC &aDes);
    inline CUDPSendEngine *Model()
        { return iModel;}
    inline CConsoleControl *Console()
        { return iConsole;}
    
    void ClearScreenL();
    //void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
    void ShowError(TInt aId);

private: // from CCoeControl
    void Draw(const TRect& /*aRect*/) const;

    //Component Controls
    void CreateBigConsoleL(TInt aFlags);

    void ShowError(const TDes &msg);
    void ShowError(const TDes &msg, TInt aErr);
    

private:

    CConsoleControl* iConsole;
    
    CUDPSendEngine *iModel;
    TBool iRunning;

    };

//
// CRotorAppUi
//

class CRotorAppUi : public CEikAppUi
    {
public:
    void ConstructL();
    ~CRotorAppUi();

private: // from CEikAppUi
    void HandleCommandL(TInt aCommand);
    TBool LaunchOptionsDialogL(CUDPSendEngine* aModel) const;
    void LaunchAboutDialogL() const;
    //void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
private:
    CUDPSendView* iAppView;
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
    COptionsDialog(CUDPSendEngine* aModel);
private:
    
    //void HandleControlStateChangeL(TInt aControlId);

    TBool OkToExitL(TInt);
    void PreLayoutDynInitL();
private:
    CUDPSendEngine* iModel;
};


class CHostNameDialog : public CEikDialog
{
public:
    CHostNameDialog(CUDPSendEngine* aSender);
    
private:
    TBool OkToExitL(TInt aButton);
    void PreLayoutDynInitL();
private:
    TBool iOKPressed;
    CUDPSendEngine* iSender;
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

#endif
