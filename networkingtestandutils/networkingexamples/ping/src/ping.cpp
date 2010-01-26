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
// ping.cpp - icmp echo client main module
// This software has been implemented in the 6PACK
// project at the Mobile Networks Laboratory (MNW)
// http://www.research.nokia.com/projects/6pack/
//

#include <e32keys.h>
#include <coemain.h>
#include <eikenv.h>
#include <eikdef.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdialg.h>
#include <eikdoc.h>
#include <eikchkbx.h>
#include <eiktbar.h>
#include <eikfnlab.h>
#include <eikedwin.h>
#include <eikstart.h>

#if EPOC_SDK >= 0x06000000
#   include <techview/eikon.hrh>
#else
#   include <eikdialg.hrh>
#   include <eikedwin.hrh>
#   include <eikcmds.hrh>
#endif

#include <pingapp.rsg>
#include "pingmodel.h"
#include "ping.hrh"
#include "ping.h"

#ifdef CALYPSO
#include <AknTextSettingPage.h>
#endif

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


void CConsoleControl::SetScrollBarVisibilityL(CEikScrollBarFrame::TScrollBarVisibility aHBarVisibility, CEikScrollBarFrame::TScrollBarVisibility aVBarVisibility)
{
    iConsole->SetScrollBarVisibilityL(aHBarVisibility,aVBarVisibility);
    iConsole->ConsoleControl()->UpdateArea();
    iConsole->UpdateScrollBars();
    iConsole->ConsoleControl()->UpdateArea();
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


TSize CConsoleControl::ScreenSize()
{
    return iConsole->ScreenSize();
}


CPingView::CPingView(CPing *aPingModel)
{
    iPingModel=aPingModel;
}

void CPingView::ConstructL(const TRect& aRect)
{
    CreateWindowL();
    Window().SetShadowDisabled(ETrue);
    iContext=this;
    iBrushStyle=CGraphicsContext::ESolidBrush;
    iBrushColor=KRgbWhite;
#if EPOC_SDK >= 0x06000000
    SetRect(aRect);
#else
    SetRectL(aRect);
#endif
    
    // Create its only component, a CPingContainer
    
    iContainer = new(ELeave) CPingContainer(iPingModel);
    iContainer->SetContainerWindowL(*this);
    TRect containerRect = Rect();
    iContainer->ConstructL(containerRect);

    ActivateL();
    DrawNow();
}

CPingView::~CPingView()
    {

    delete iContainer;
    }
    
TInt CPingView::CountComponentControls() const
    {   
    return 1; //Just the container
    }

CCoeControl* CPingView::ComponentControl(TInt /*aIndex*/) const
    {
    return iContainer;
    }


void CPingView::ResetScreen()
{
    iContainer->ResetScreen();
}


TKeyResponse CPingView::OfferKeyEventL(const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
    return EKeyWasNotConsumed;
}

void CPingView::Draw(const TRect& /*aRect*/) const
    {

    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(Rect());

    }


//
// CPingcontainer
//
const TInt KLabelHeight=25;


 // Construction
CPingContainer::CPingContainer(CPing* aPingModel)
{
    iPingModel=aPingModel;
}


// Destruction
CPingContainer::~CPingContainer()
{
    delete iLabel;
    delete iLabel2;
    delete iConsole;
/*
    delete iEdit;
    delete iRtxt;
*/
}

void CPingContainer::ConstructL(const TRect& aRect)
{
    iPingModel->SetConsole(this);

#if EPOC_SDK >= 0x06000000
    SetRect(aRect);
#else
    SetRectL(aRect);
#endif

    TRect rect=Rect();
    rect.iTl.iY += KLabelHeight;    // make way for top label
    rect.iBr.iY -= KLabelHeight;    // make way for bottom label

    CreateConsoleL(rect);

    //Bottom label
    iLabel=new (ELeave) CEikLabel;
    rect=Rect();
    rect.iTl.iY=rect.iBr.iY-KLabelHeight; // make it bottom 20 pixels
    iLabel->SetContainerWindowL(*this);
#if EPOC_SDK >= 0x06000000
    iLabel->SetRect(rect);
#else
    iLabel->SetRectL(rect);
#endif
    iLabel->SetAlignment(EHLeftVCenter); // center text
    iLabel->SetBufferReserveLengthL(500); // nice long buffer
    UpdateStatisticsL();
    
    //Top label
    iLabel2=new (ELeave) CEikLabel;
    rect=Rect();
    rect.iBr.iY=rect.iTl.iY+KLabelHeight; // make it top 20 pixels
    iLabel2->SetContainerWindowL(*this);
#if EPOC_SDK >= 0x06000000
    iLabel2->SetRect(rect);
#else
    iLabel2->SetRectL(rect);
#endif
    iLabel2->SetAlignment(EHLeftVCenter); // center text
    iLabel2->SetBufferReserveLengthL(500); // nice long buffer
    TBuf<6> hostname(_L("<None>"));
    WriteHostL(hostname);
/*  
    iEdit = new(ELeave) CEikRichTextEditor;
    //iEdit->ConstructL(NULL,0,0,EEikEdwinOwnsWindow|EEikEdwinInclusiveSizeFixed|EEikEdwinKeepDocument|EEikEdwinUserSuppliedText|EEikEdwinLineCursor|EEikEdwinJustAutoCurEnd);
    iEdit->ConstructL(NULL,0,0,EEikEdwinOwnsWindow|EEikEdwinInclusiveSizeFixed|EEikEdwinKeepDocument|EEikEdwinUserSuppliedText|EEikEdwinLineCursor);
    //iEdit->ConstructL(NULL,0,0,EEikEdwinOwnsWindow|EEikEdwinInclusiveSizeFixed|EEikEdwinKeepDocument|EEikEdwinUserSuppliedText);
    
    iEdit->SetObserver(this);
    iEdit->CreateScrollBarFrameL();
    iEdit->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOn);
    
    iRtxt= CRichText::NewL(iEikonEnv->SystemParaFormatLayerL(),iEikonEnv->SystemCharFormatLayerL());    
    ResetScreen();
*/  

/*
#if EPOC_SDK >= 0x06000000
    iEdit->SetRect(rect);
#else
    iEdit->SetRectL(rect);
#endif
*/
    ActivateL();
}


void CPingContainer::CreateConsoleL(TRect aRect)
{
    iConsole = new(ELeave) CConsoleControl;
#if defined(CRYSTAL)  //This is a patch to put the console in the right place. 
                //For some unknown reason appears displaced.
    aRect.iBr.iX += 92;
    aRect.iTl.iX += 92;
    iConsole->ConstructL(aRect.iTl, aRect.Size(), CEikConsoleScreen::ENoInitialCursor);
#elif defined(CALYPSO)
    TSize size;
    size.iHeight = aRect.iBr.iY - aRect.iTl.iY;
    size.iWidth = aRect.iBr.iX - aRect.iTl.iX - 10; // -10 to leave some space on the right
                                                    // (based on trial and error)
    TPoint position;
    position.iY = 70; // To skip the status pane and host name field (based on trial and error)
    position.iX = 5;  // 5 to leave some space on the left (based on trial and error)

    iConsole->ConstructL(position, size, CEikConsoleScreen::ENoInitialCursor);
#else
    iConsole->ConstructL(aRect.iTl, aRect.Size(), CEikConsoleScreen::ENoInitialCursor);    
#endif
    
    iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
}

void CPingContainer::ResetScreen()
{
    iConsole->ClearScreen();
}

void CPingContainer::WriteHostL(TDes& aHostname)
{
    TBuf<100> aux;
    
    aux.Append(aHostname);

    iLabel2->SetTextL(aux);
    iLabel2->DrawNow();
}

void CPingContainer::UpdateStatisticsL()
{
    TBuf<48> aux;

    aux.Format(_L("Sent: %d Recvd: %d"),iPingModel->iSentPackets, iPingModel->iRecvPackets);
    iLabel->SetTextL(aux);
    iLabel->DrawNow();
}

void CPingContainer::WriteLine(const TDesC& abuf)
{
    iConsole->Write(abuf);
}

TKeyResponse CPingContainer::OfferKeyEventL(const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
    return EKeyWasNotConsumed;
}

// Virtual, defined by CCoeControl; replaces the default implementation
// provided by CCoeControl.
void CPingContainer::Draw(const TRect& aRect) const
{
    CWindowGc& gc = SystemGc();
    gc.SetClippingRect(aRect);
    gc.DrawRect(Rect());

}
    
// Virtual, defined by CCoeControl; replaces the default implementation
// provided by CCoeControl. 
TInt CPingContainer::CountComponentControls() const
{
    return 3;
}

// Virtual, defined by CCoeControl; replaces the default implementation
// provided by CCoeControl.
CCoeControl* CPingContainer::ComponentControl(TInt aIndex) const
{
    switch (aIndex)
    {
    case 0: return iLabel;
    case 1: return iLabel2;
    case 2: return iConsole;
    //case 2: return iEdit;
    };

    return NULL;
}

// Virtual, defined by CCoeControl; empty implementation provided by
// CCoeControl; full implementation provided by this class
void SizeChangedL()
{
    TInt a=2;
    a=a;
}
    
// Defined as pure virtual by the mixin class MCoeControlObserver 
// inherited by CCoeControl. An empty implementation provided by 
// this class (its containees do not report events).
void CPingContainer::HandleControlEventL(CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{


}

#ifndef CALYPSO

COptionsDialog::COptionsDialog(CPing *aPingModel):CEikDialog()
{
    iPingModel=aPingModel;

}


COptionsDialog::~COptionsDialog()
{

}

void COptionsDialog::HandleControlStateChangeL(TInt aControlId)
{
    if (aControlId==EPingNumPackCheckBox)
    {
        //Dimms/Undimms the control PacketLimitNum if needed
        CEikButtonBase *checkbox = (CEikButtonBase *)Control(EPingNumPackCheckBox);
        TBool b = checkbox->State()==CEikCheckBox::EClear;
        SetLineDimmedNow(EPingNumPackNum,b);    
        MakeLineVisible(EPingNumPackNum,!b);
    }
    else if (aControlId==EPingHopLimitCheckBox)
    {
        CEikButtonBase *checkbox = (CEikButtonBase *)Control(EPingHopLimitCheckBox);
        TBool b = checkbox->State()==CEikCheckBox::EClear;
        SetLineDimmedNow(EPingHopLimitNum,b);   
        MakeLineVisible(EPingHopLimitNum,!b);
    }
}


// Checks if data entered by user is correct
TBool COptionsDialog::OkToExitL(TInt)
{
    if (CheckBoxState(EPingNumPackCheckBox)==CEikButtonBase::ESet)
    {
        iPingModel->iTotalPackets=NumberEditorValue(EPingNumPackNum);
        iPingModel->iPackLimit=ETrue;
    }
    else
        iPingModel->iPackLimit=EFalse;  //Indicates Unlimited Packets

    // This option is disabled by now. Code is commented pingmodel.cpp
    /* DO NOT REMOVE!!!
    if (CheckBoxState(EPingHopLimitCheckBox)==CEikButtonBase::ESet)
        iPingModel->iHopLimit=NumberEditorValue(EPingHopLimitNum);
    else
        iPingModel->iHopLimit=0;    //Indicates Hop Limit won't be set
    
    iPingModel->iNumericOutput=(CheckBoxState(EPingNoResolvCheckBox)==CEikButtonBase::ESet);
    */
    iPingModel->iQuiet=(CheckBoxState(EPingQuietCheckBox)== CEikButtonBase::ESet);
    iPingModel->iVerbose=(CheckBoxState(EPingVerboseCheckBox)== CEikButtonBase::ESet);
    iPingModel->iDebug=(CheckBoxState(EPingDebugCheckBox)== CEikButtonBase::ESet);

    iPingModel->iSecWait=NumberEditorValue(EPingSecNum);    //Time between packets
    iPingModel->iLastSecWait=NumberEditorValue(EPingLastSecNum);    //Time between packets  
    iPingModel->iPacketDataSize=NumberEditorValue(EPingPackSizeNum);
    GetEdwinText(iPingModel->iPattern,EPingPatternNum);
    //Check the pattern is hexadecimal
    TLex lex(iPingModel->iPattern);
    while (!lex.Eos())
    {
        if (!lex.Get().IsHexDigit())
        {
            CEikonEnv::Static()->InfoMsg(_L("Bad Pattern (Must be Hexadecimal)"));
            TryChangeFocusToL(EPingPatternNum);
            return EFalse;
        }

    }
    return ETrue;
}

//To initialize dialog data
void COptionsDialog::PreLayoutDynInitL()
{
    if (!(iPingModel->iPackLimit))
    {
        SetCheckBoxState(EPingNumPackCheckBox, CEikButtonBase::EClear);
        MakeLineVisible(EPingNumPackNum,EFalse);
        SetLineDimmedNow(EPingNumPackNum,ETrue);
    }
    else
    {
        SetCheckBoxState(EPingNumPackCheckBox, CEikButtonBase::ESet);
    }
    SetNumberEditorValue(EPingNumPackNum, iPingModel->iTotalPackets);
    
    // This lines should be disabled if next commented code is active
    SetLineDimmedNow(EPingNoResolvCheckBox,ETrue);
    MakeLineVisible(EPingNoResolvCheckBox,EFalse);
    /*
    if (iPingModel->iNumericOutput)
        SetCheckBoxState(EPingNoResolvCheckBox, CEikButtonBase::ESet);
    else
        SetCheckBoxState(EPingNoResolvCheckBox, CEikButtonBase::EClear);
    */

    if (iPingModel->iDebug)
        SetCheckBoxState(EPingDebugCheckBox, CEikButtonBase::ESet);
    else
        SetCheckBoxState(EPingDebugCheckBox, CEikButtonBase::EClear);

    if (iPingModel->iQuiet)
        SetCheckBoxState(EPingQuietCheckBox, CEikButtonBase::ESet);
    else
        SetCheckBoxState(EPingQuietCheckBox, CEikButtonBase::EClear);

    // This lines should be disabled if next commented code is active
    SetLineDimmedNow(EPingHopLimitCheckBox,ETrue);
    MakeLineVisible(EPingHopLimitCheckBox,EFalse);
    SetLineDimmedNow(EPingHopLimitNum,ETrue);
    MakeLineVisible(EPingHopLimitNum,EFalse);
    // This option is disabled by now. Code is commented in .rss and pingmodel.cpp
/*DO NOT REMOVE!!!
    if (iPingModel->iHopLimit==0)
    {
        SetCheckBoxState(EPingHopLimitCheckBox, CEikButtonBase::EClear);
        MakeLineVisible(EPingHopLimitNum,EFalse);
        SetLineDimmedNow(EPingHopLimitNum,ETrue);
        SetNumberEditorValue(EPingHopLimitNum, 255);    //Max TTL (in hops)
    }
    else
    {
        SetCheckBoxState(EPingNumPackCheckBox, CEikButtonBase::ESet);
        SetNumberEditorValue(EPingHopLimitNum, iPingModel->iHopLimit);
    }
*/

    if (iPingModel->iVerbose)
        SetCheckBoxState(EPingVerboseCheckBox, CEikButtonBase::ESet);
    else
        SetCheckBoxState(EPingVerboseCheckBox, CEikButtonBase::EClear);


    SetNumberEditorValue(EPingSecNum, iPingModel->iSecWait);
    SetNumberEditorValue(EPingLastSecNum, iPingModel->iLastSecWait);
    SetNumberEditorValue(EPingPackSizeNum, iPingModel->iPacketDataSize);
    SetEdwinTextL(EPingPatternNum,&iPingModel->iPattern);
}

#endif // #ifndef CALYPSO

//
// CHostNameDialog
//
#ifdef CALYPSO
CHostNameDialog::CHostNameDialog(TDes& aHostname) : CAknTextQueryDialog(aHostname)
{
}
#else
CHostNameDialog::CHostNameDialog(CPing *aPingModel):CEikDialog()
{
    iPingModel=aPingModel;
}
#endif

// Checks if data entered by user is correct
// returns ETrue to exit dialog anf EFalse to not exit it (no name entered)

#ifdef CALYPSO
TBool CHostNameDialog::OkToExitL(TInt aButton)
{   
    return CAknTextQueryDialog::OkToExitL(aButton);
}
#else
TBool CHostNameDialog::OkToExitL(TInt aButton)
{   
    
    TBuf<KHostNameLimit> hostname(_L(""));
    if (aButton==EEikBidCancel) //CANCEL BUTTON
    {
        iOKPressed=EFalse;
        return ETrue;
    }
    
    GetEdwinText(hostname,EPingHostName);
    
    if (hostname.Length()!=0)
    {
        iPingModel->SetHostName(hostname);
        return ETrue;
    }
    else
        return EFalse;  //If no hostname specified can't continue
    
}
#endif // CALYPSO

//To initialize dialog data
#ifdef CALYPSO
void CHostNameDialog::PreLayoutDynInitL()
{
    CAknTextQueryDialog::PreLayoutDynInitL();
}
#else
void CHostNameDialog::PreLayoutDynInitL()
{
    //TBuf<KHostNameLimit> hostname(_L(""));

    SetEdwinTextL(EPingHostName,iPingModel->GetHostName());
}
#endif



//
// CPingAppUi
//

void CPingAppUi::ConstructL()
{
    BaseConstructL();
    
    InitModelL();
    iAppView=new(ELeave) CPingView(iPingModel);
    iAppView->ConstructL(ClientRect());

#if EPOC_SDK < 0x06000000
    //Just to watch the name of the file on the top of the toolbar
    CEikFileNameLabel* pingLabel=(CEikFileNameLabel*)iToolBar->ControlById(EPingCmdFileName);
    pingLabel->UpdateL();
#endif

#ifndef CALYPSO
    
    // add app view to stack; enables key event handling.
    AddToStackL(iAppView);
    
#else // if CALYPSO defined
    
    AppViewToStackL();

    iAppView->ActivateL();

    iSettingView = new (ELeave) CSettingView(iPingModel);
    iSettingView->ConstructL(ClientRect());
    iSettingView->SetComponentsToInheritVisibility( ETrue );

    SettingViewToStackL();

    iSettingView->ActivateL();

    ShowAppViewL();
    
#endif
}

#ifdef CALYPSO

void CPingAppUi::ShowAppViewL()
{
    iSettingView->MakeVisible( EFalse );
    SettingViewFromStack();
    AppViewToStackL();
    iAppView->MakeVisible( ETrue );
    iAppView->DrawNow();

    CEikButtonGroupContainer *cba = ((CAknAppUi*)CEikonEnv::Static()->EikAppUi())->Cba();
    cba->SetCommandSetL(R_PING_CBA);
    cba->DrawDeferred();

    // Make sure that setting view's scroll indicators are not shown in the app view
    iSettingView->ListBox()->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOff);
    iSettingView->ListBox()->UpdateScrollBarsL();
}

void CPingAppUi::ShowSettingViewL()
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

void CPingAppUi::AppViewToStackL()
{
    if (!iAppViewOnStack)
    {
        AddToStackL(iAppView);
        iAppViewOnStack = ETrue;
    }
}

void CPingAppUi::SettingViewToStackL()
{
    if (!iSettingViewOnStack)
    {
        AddToStackL(iSettingView);
        iSettingViewOnStack = ETrue;
    }
}

void CPingAppUi::AppViewFromStack()
{
    if (iAppViewOnStack) 
    {
        RemoveFromStack(iAppView);
        iAppViewOnStack = EFalse;
    }
}

void CPingAppUi::SettingViewFromStack()
{
    if (iSettingViewOnStack)
    {
        RemoveFromStack(iSettingView);
        iSettingViewOnStack = EFalse;
    }
}

#endif // #ifdef CALYPSO

void CPingAppUi::InitModelL()
{
    TPreferences param;

    RestorePreferencesL(param); //Reads the .ini file

    iPingModel= new (ELeave) CPing();
    iPingModel->ConstructL(param);
}

void CPingAppUi::HandleCommandL(TInt aCommand)
{
    TPreferences param;
    
    switch (aCommand)
    {
    case EPingStart:
        if (!(iPingModel->IsRunning()))
            if (CreateHostNameDialogL()) {
                iPingModel->BeginL();
			}
        break;

    case EPingStop: //Stop ping
        if (iPingModel->IsRunning())
            iPingModel->EndPingL();
        break;

    case EPingapp2Options:
        if (!(iPingModel->IsRunning()))
#ifdef CALYPSO
            ShowSettingViewL();
#else
            CreateOptionsDialogL();
#endif        
        break;

    case EPingReset:
        iAppView->ResetScreen();
        break;

    case EPingAbout:
        CreateAboutDialogL();
        break;

    case EEikCmdExit:
        if (iPingModel->IsRunning())
            iPingModel->EndPingL();
        iPingModel->GetPreferences(param);
        StorePreferencesL(param);
#ifdef CALYPSO
        AppViewFromStack();
        SettingViewFromStack();
#endif        
        Exit();
        return;

#ifdef CALYPSO
        
    case EPingSave:
        // Try to save settings data
        if ( iSettingView->SaveSettingsL() )
        {
            // Switch back to the normal view
            ShowAppViewL();
        }
        break;

    case EPingCancel:
        // Simply switch back to the normal view
        ShowAppViewL();
        break;

#endif // #ifdef CALYPSO
    }
}

#ifndef CALYPSO

// Launches a dialog to ask for new options
void CPingAppUi::CreateOptionsDialogL()
    {
    CEikDialog* dialog = new (ELeave) COptionsDialog(iPingModel);
    dialog->ExecuteLD(R_PING_OPTIONS);  //Final D means the dialog is destructed by itself
    }

#endif // #ifndef CALYPSO
    
// Launches a dialog to show an about box
void CPingAppUi::CreateAboutDialogL()
    {
    CEikDialog* dialog = new (ELeave) CEikDialog();
    dialog->ExecuteLD(R_PING_ABOUT);    //Final D means the dialog is destructed by itself
    }


//Launches a dialog to specify a hostname and returns if Ok has been pressed to
//
#ifdef CALYPSO
TBool CPingAppUi::CreateHostNameDialogL()
{
    TBuf<KHostNameLimit> hostname;

    hostname.Copy(*(iPingModel->GetHostName()));

    CAknTextQueryDialog* dialog = new (ELeave) CHostNameDialog(hostname);

    TInt button = dialog->ExecuteLD(R_PING_HOSTNAME_AVKON);

    if ((button == EAknSoftkeyOk) && (hostname.Length() != 0))
    {
        iPingModel->SetHostName(hostname);
        return ETrue;
    }
    else
    {
        return EFalse;
    }
}
#else
TBool CPingAppUi::CreateHostNameDialogL()
    {
    CEikDialog* dialog = new (ELeave) CHostNameDialog(iPingModel);
    TInt button=dialog->ExecuteLD(R_PING_HOSTNAME); //Final D means the dialog is destructed by itself
    return (button==EEikBidOk); // If button is CANCEL then the ping is not executed
    }
#endif // CALYPSO

void CPingAppUi::RestorePreferencesL(TPreferences &aPreferences)
{

    CDictionaryStore *iniFile = Application()->OpenIniFileLC(iCoeEnv->FsSession());
    TBool found = iniFile->IsPresentL(KUidPingApp); //replace XXUid with uid of prefs - usually the UID of the main app
    TInt error = KErrNone;

    if (found) 
    {
        RDictionaryReadStream readStream;
        readStream.OpenLC (*iniFile, KUidPingApp);
        // ignore any reads off the end of the file etc - clear later on

        TRAP(error, aPreferences.iFlags = readStream.ReadUint8L());
        //if (error!=KErrNone)
        //  aPreferences.iFlags=0;
        TRAP(error, aPreferences.iLastSecWait = readStream.ReadUint16L());
        //if (error!=KErrNone)
        TRAP(error, aPreferences.iPacketDataSize = readStream.ReadUint16L());
        //if (error!=KErrNone)
        TRAP(error, aPreferences.iSecWait = readStream.ReadUint16L());
        //if (error!=KErrNone)
        TRAP(error, aPreferences.iTotalPackets = readStream.ReadUint16L());
        //if (error!=KErrNone)
        TInt length=0;
        TRAP(error, length = readStream.ReadInt16L());
        TRAP(error, readStream.ReadL(aPreferences.iHostname,length));
        length=0;
        TRAP(error, length = readStream.ReadInt16L());
        TRAP(error, readStream.ReadL(aPreferences.iPattern,length));

#ifdef IAPSETTING
		TRAP(error, aPreferences.iIAP = readStream.ReadInt16L());
#endif
        //if (error!=KErrNone)
        //........ // read in all the fields as appropriate
        CleanupStack::PopAndDestroy(); // readStream
    }

    CleanupStack::PopAndDestroy(); // iniFile

    if (error!=KErrNone || !found) 
    {
        // missing stream initialise
        CPing::DefaultPreferences(aPreferences);
        /*
        aPreferences.iFlags = 0;
        
        aPreferences.iLastSecWait=2;
        aPreferences.iPacketDataSize=56;
        aPreferences.iSecWait=1;
        aPreferences.iTotalPackets=10;
        aPreferences.iPattern=_L("FF");
        */

        //.... // and whatever is appropriate for all the other fields
        StorePreferencesL(aPreferences); // store the default ones - update inifile
    }
}


void CPingAppUi::StorePreferencesL(const TPreferences &aPreferences)
{
    CDictionaryStore *iniFile = Application()->OpenIniFileLC(iCoeEnv->FsSession());
    RDictionaryWriteStream writeStream;
    writeStream.AssignLC(*iniFile, KUidPingApp);
    writeStream.WriteUint8L (aPreferences.iFlags);
    writeStream.WriteUint16L (aPreferences.iLastSecWait);
    writeStream.WriteUint16L (aPreferences.iPacketDataSize);
    writeStream.WriteUint16L (aPreferences.iSecWait);
    writeStream.WriteUint16L (aPreferences.iTotalPackets);
    writeStream.WriteInt16L (aPreferences.iHostname.Length());
    writeStream.WriteL (aPreferences.iHostname);
    writeStream.WriteInt16L (aPreferences.iPattern.Length());
    writeStream.WriteL (aPreferences.iPattern);
   
#ifdef IAPSETTING
	writeStream.WriteUint16L (aPreferences.iIAP);
#endif
    //.... // and whatever

    writeStream.CommitL();
    CleanupStack::PopAndDestroy(); // write stream

    // in this replace XXVersionUid with another unique UID - usually the next one up from XXUid
    writeStream.AssignLC(*iniFile, KUidPingVersionUid); // write version 1.0 (major.minor)
    writeStream.WriteInt8L(1); // major
    writeStream.WriteInt8L(0); // minor
    writeStream.CommitL(); // flush
    CleanupStack::PopAndDestroy(); // writeStream;

    // commit changes to the store
    if (iniFile->Commit()!=KErrNone)
        iniFile->RevertL();

    CleanupStack::PopAndDestroy();
}



CPingAppUi::~CPingAppUi()
    {
#ifndef CALYPSO
	RemoveFromStack(iAppView);
#endif

    delete iAppView;
#ifdef CALYPSO
    delete iSettingView;
#endif
    delete iPingModel;
    }

//
// CPingDocument
//

CPingDocument::CPingDocument(CEikApplication& aApp)
        : CEikDocument(aApp)
    {
    }

CEikAppUi* CPingDocument::CreateAppUiL()
    {
    return(new(ELeave) CPingAppUi);
    }

//
// CPingApplication
//

TUid CPingApplication::AppDllUid() const
    {
    return KUidPingApp;
    }


CApaDocument* CPingApplication::CreateDocumentL()
    {
    return new(ELeave) CPingDocument(*this);
    }

//
// EXPORTed functions
//

LOCAL_C CApaApplication* NewApplication()
    {
    return new CPingApplication;
    }

#if defined(__WINS__) && !defined(EKA2)

GLDEF_C TInt E32Dll(TDllReason)
	{
	return KErrNone;
	}
EXPORT_C TInt WinsMain(TDesC* aCmdLine)
	{
	return EikStart::RunApplication(NewApplication, aCmdLine);
	}

#else

GLDEF_C TInt E32Main()
	{
	return EikStart::RunApplication(NewApplication);
	}

#endif

#ifdef CALYPSO

//------------------------------------------------------------------
// CPingBinaryPopupSettingItem
//
// NOTE. This class is implemented only to circumvent
// a bug (?) in CAknBinaryPopupSettingItem::Load
//------------------------------------------------------------------

CPingBinaryPopupSettingItem::CPingBinaryPopupSettingItem( TInt aIdentifier, TBool& aBinaryValue ) :
   CAknBinaryPopupSettingItem( aIdentifier, aBinaryValue )
{
}

void CPingBinaryPopupSettingItem::LoadL()
{
    // Call CAknEnumeratedTextPopupSettingItem::LoadL to copy
    // the external boolean value to an internal value
    // NOTE. CAknBinaryPopupSettingItem::LoadL must NOT be called
    CAknEnumeratedTextPopupSettingItem::LoadL();
}

//-----------------------------------------------------------------
// CPingTextSettingItem
//
// NOTE. This is a re-implementation/copy of CAknTextSettingItem
// (the only difference is in the LoadL method) due to a bug/
// feature in CAknTextSettingItem::LoadL()
//-----------------------------------------------------------------

CPingTextSettingItem::CPingTextSettingItem( TInt aIdentifier, TDes& aText ) : 
   CAknSettingItem(aIdentifier), iExternalText( aText ), iInternalTextPtr(0,0)
{
}   

CPingTextSettingItem::~CPingTextSettingItem()
{
    delete iInternalText;
}

void CPingTextSettingItem::StoreL()
{
    // Copy the internal to the external
    iExternalText.Copy(iInternalText->Des() );
}

// NOTE. This is the only method that has changed from CAknTextSettingItem
void CPingTextSettingItem::LoadL()
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

const TDesC& CPingTextSettingItem::SettingTextL()
{
    return *iInternalText;
}

void CPingTextSettingItem::EditItemL( TBool /* aCalledFromMenu */)
{
    CAknSettingPage* dlg = new( ELeave )CAknTextSettingPage( SettingPageResourceId(), iInternalTextPtr, 0 );
    SetSettingPage( dlg );
    SettingPage()->SetSettingPageObserver(this);
    SetUpStandardSettingPageL();
    SettingPage()->ExecuteLD(CAknSettingPage::EUpdateWhenChanged);
}

TPtr& CPingTextSettingItem::InternalTextPtr()
{
    return iInternalTextPtr;
}

TPtrC CPingTextSettingItem::ExternalText()
{
    return (TPtrC)(iExternalText);
}

void CPingTextSettingItem::SetExternalText( TDesC& aNewExternalText )
{
    iExternalText.Copy( aNewExternalText );
}

//-----------------------------------------------------------------
// CSettingView
//-----------------------------------------------------------------

void CSettingView::ConstructL(const TRect& /*aRect*/)
{
    // Copy data from the model to member variables

    iLimitPacketCount = iPingModel->iPackLimit ? ETrue : EFalse;
    iMaxPacketCount.Num( iPingModel->iTotalPackets );
    iPacketDataSize.Num( iPingModel->iPacketDataSize );
    iWaitTime.Num( iPingModel->iSecWait );
    iLastWaitTime.Num( iPingModel->iLastSecWait );

    iPattern.Copy( iPingModel->iPattern );
    iQuiet = iPingModel->iQuiet ? ETrue : EFalse;
    iVerbose = iPingModel->iVerbose ? ETrue : EFalse;
    iDebug = iPingModel->iDebug ? ETrue : EFalse;

#ifdef IAPSETTING
	iIAP.Num( iPingModel->iIAP );	 
#endif

    CAknSettingItemList::ConstructFromResourceL( R_PING_SETTINGS );
}

CSettingView::CSettingView(CPing *aPingModel)
{
    iPingModel = aPingModel;
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
            settingItem = new (ELeave) CPingBinaryPopupSettingItem( identifier, iLimitPacketCount );
            return settingItem;
            break;
        case 2:
            settingItem = new (ELeave) CPingTextSettingItem( identifier, iMaxPacketCount );
            return settingItem;
            break;
        case 3:
            settingItem = new (ELeave) CPingTextSettingItem(identifier, iPacketDataSize);
            return settingItem;
            break;
        case 4:
            settingItem = new (ELeave) CPingTextSettingItem(identifier, iWaitTime);
            return settingItem;
            break;
        case 5:
            settingItem = new (ELeave) CPingTextSettingItem(identifier, iLastWaitTime);
            return settingItem;
            break;
        case 6:
            settingItem = new (ELeave) CPingTextSettingItem(identifier, iPattern);
            return settingItem;
            break;
        case 7:
            settingItem = new (ELeave) CPingBinaryPopupSettingItem( identifier, iQuiet );
            return settingItem;
            break;
        case 8:
            settingItem = new (ELeave) CPingBinaryPopupSettingItem( identifier, iVerbose );
            return settingItem;
            break;
        case 9:
            settingItem = new (ELeave) CPingBinaryPopupSettingItem( identifier, iDebug );
            return settingItem;
            break;
#ifdef IAPSETTING
        case 10:
			settingItem = new (ELeave) CPingTextSettingItem( identifier, iIAP );
			return settingItem;
            break;
#endif        
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

    TInt maxPacketCount;
    lex.Assign( iMaxPacketCount );
    if ( lex.Val( maxPacketCount ) != KErrNone ) 
    {
        CEikonEnv::Static()->InfoMsg(_L("Packet count must be numeric"));
        return EFalse;
    }

    TInt packetDataSize;
    lex.Assign( iPacketDataSize );
    if ( lex.Val( packetDataSize ) != KErrNone ) 
    {
        CEikonEnv::Static()->InfoMsg(_L("Packet size must be numeric"));
        return EFalse;
    }

    TInt waitTime;
    lex.Assign( iWaitTime );
    if ( lex.Val( waitTime ) != KErrNone ) 
    {
        CEikonEnv::Static()->InfoMsg(_L("Wait time must be numeric"));
        return EFalse;
    }

    TInt lastWaitTime;
    lex.Assign( iLastWaitTime );
    if ( lex.Val( lastWaitTime ) != KErrNone ) 
    {
        CEikonEnv::Static()->InfoMsg(_L("Last wait time must be numeric"));
        return EFalse;
    }

    lex.Assign( iPattern );
    while (!lex.Eos())
    {
        if (!lex.Get().IsHexDigit())
        {
            CEikonEnv::Static()->InfoMsg(_L("Pattern must be hexadecimal"));
            return EFalse;
        }
    }

    // Validation OK, so save settings to the model

    iPingModel->iPackLimit = iLimitPacketCount;
    iPingModel->iTotalPackets = maxPacketCount;
    iPingModel->iPacketDataSize = packetDataSize;
    iPingModel->iSecWait = waitTime;
    iPingModel->iLastSecWait = lastWaitTime;
    iPingModel->iPattern.Copy(iPattern);
    iPingModel->iQuiet = iQuiet;
    iPingModel->iVerbose = iVerbose;
    iPingModel->iDebug = iDebug;

#ifdef IAPSETTING
	TInt iap;
	lex.Assign( iIAP );
	lex.Val( iap );
	iPingModel->iIAP = iap;
#endif

    return ETrue;
}
    
#endif // #ifdef CALYPSO
