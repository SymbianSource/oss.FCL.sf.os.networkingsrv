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
// webserver.h - http server main module
//



/**
 @internalComponent
*/
#ifndef __WEBSERVER_H
#define __WEBSERVER_H
/*
#include <coecntrl.h>

#include <eiksform.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikenv.h>
#include <eikcmds.hrh>
#include <eikfnlab.h>
#include <eiktbar.h>
#include <eiklabel.h>
*/



#if EPOC_SDK >= 0x06000000
#include <coeccntx.h>

#ifdef CALYPSO
#include <aknappui.h>
#else
#include <eikenv.h>
#include <eikappui.h>
#include <eiktbar.h>
#endif	//CALYPSO
#include <eikapp.h>
#include <eikdoc.h>
#include <eikconso.h>

#include <eiktxlbm.h>

#else
#include <basched.h>
#include <eikenv.h>
#include <coecntrl.h>
#include <eikappui.h>
#include <e32keys.h>
#include <eikmenu.hrh>
#include <eikdef.h>
#include <techview/eikon.rsg>
#include <eikchlst.h>
#include <eikpgsel.h>

#include <eikfontd.h>
#include <e32math.h>
#include <eikdialg.hrh>

//*********************
#include <coecntrl.h>
#include <coeccntx.h>
#include <coemain.h>

#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikcmds.hrh>
#include <eikenv.h>
#include <eikfnlab.h>
#include <eiktbar.h>

#include <eikconso.h>


//************ Header introduced by me. **************************
//#include <eikgted.h>			// CEikGlobalTextEditor class.
#include <eikchkbx.h>		    // CEikCtCheckBox class. (Check boxes class)
#include <eikedwin.h>           // CEikEdwin class. (Text boxes class)
#include <eikmfne.h>			// CEikNumberEditor class (Number boxes class)		
#include <eiktxtut.h>		    // TextUtils class. (For mime-type dialog)
#include <coeutils.h>			// ConeUtils class.
#endif

#include <eikdialg.h>
#include <eiklabel.h>			// CEikLabel class.
#include <eikclb.h>				// CEikColumnListBox class.	(For mime-type dialog)
#include <eikmenup.h>
#include <eikclbd.h>	        // CColumnListBoxData class. (For mime-type dialog)

#include <webserver.rsg>
#include "webserver.hrh"

#include "help.h"
#include "ws_eng.h"

#ifdef __SECURE_DATA__
#define HTTP_TYPES_FILENAME _L("wstypes.cfg")
#define CFG_FILENAME _L("ws.cfg")
#else  // __SECURE_DATA__
#define HTTP_TYPES_FILENAME _L("\\data\\wstypes.cfg")
#define CFG_FILENAME _L("\\data\\ws.cfg")
#endif // __SECURE_DATA__


// Definition for Views
#define LABEL_BUFFER_SIZE	256
#define	LABEL_HEIGHT		20
#define HEADER_TITLE		_L("WEBSERVER")
#define FOOTER_TITLE		_L("Current Connections:    0 Total Connections:    0")



const TUid KUidWebServer={ 0x10000884 };


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
	void ConstructL(const TPoint& aLeftTop, const TSize& aSize, TInt aFlags);
//    void HandleCommandL(TInt aCommand);
    void ActivateL();
	void SetScrollBarVisibilityL(CEikScrollBarFrame::TScrollBarVisibility aHBarVisibility, CEikScrollBarFrame::TScrollBarVisibility aVBarVisibility);
	void DrawCursor();
	void Write(const TDesC &aDes);
	CEikConsoleControl *ConsoleControl() const;
	TBool UpdateScrollBars();
	void ClearScreen();
	void Redraw(const TRect &aRect);
	void Lf();
public: // Functions added by Carlos Chinea
	void SetHistorySizeL(const TInt aSize);
protected:
    void FocusChanged(TDrawNow aDrawNow);
//private:
  //  void ToggleFontStyleAndRedrawL(TMessageControlFontStyle aStyleElement);
	
private:
	CEikConsoleScreen* iConsole;
	TInt iHistory;
	};

// End CSimpleConsole from TONINO(c)



//
// CExampleApplication
//

class CWebServerApplication : public CEikApplication
	{
private: // from CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};

//
// CWebServerDocument
//

class CWebServerDocument : public CEikDocument
	{
public:
	~CWebServerDocument();
	static CWebServerDocument* NewL(CEikApplication& aApp);
	void ResetModel();
	CWebServerEng* Model() { return iModel;}
private:
	CWebServerDocument(CEikApplication& aApp);
	void ConstructL();
private: // from CEikDocument
	CEikAppUi* CreateAppUiL();
	CWebServerEng* iModel;
	};

//
// CWebServerAppUi
//

class CWebServerAppView;
#ifndef CALYPSO
class CWebServerAppUi : public CEikAppUi
#else
class CWebServerAppUi : public CAknAppUi
#endif	//CALYPSO
    {
public:
    void ConstructL();
	~CWebServerAppUi();
private: // from CEikAppUi
	void HandleCommandL(TInt aCommand);
	void ConsoleDialogL();
	void StartWebServer();
	void ResetTotalCounterL();
#ifndef CALYPSO
	void SetHistorySizeDialogL();
	void DirDialogL();
	void HttpMethodsDialogL();
	void MiscDataDialogL();
	void MimeTypesDialogL();
	void AboutDialogL() const;
#endif //CALYPSO
	void DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane);
private:
	CWebServerEng*	iModel;
    CWebServerAppView* iAppView;
    };

//
// class CWebServerAppView
//
class CWebServerAppView : public CCoeControl, public MCoeControlBrushContext, public MWebServerObserver
    {
public:
    void ConstructL(const TRect& aRect, CWebServerEng* aModel);
	~CWebServerAppView();
public: // from MCommsTestObserver
	void HandleProgressEvent();
public:
	void ShowWarningL(const TInt aWarning);
	void UpdateNumberConL();
private:// Component Control
	void CreateBigConsoleL();
	void CreateTopLabelL();
	void CreateBottomLabelL();

	void ShowError(const TDes& aMsg);
	void ShowError(const TDes& aMsg,TInt aErr);
private: // from CCoeControl
	//void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	void Draw(const TRect& /*aRect*/) const;
public:
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Write(const TDesC& aDes);
	void ClearScreenL();
	void ShowError(TInt aId);
	inline CWebServerEng *Model() {	return iModel; }
	inline CConsoleControl *Console() const { return iConsole; }
//	void SizeChangedL();
private:
	CWebServerEng* iModel;
	CConsoleControl*	iConsole;
	CEikLabel* iTopLabel;
	CEikLabel* iBottomLabel;
    };


#ifndef CALYPSO
class CWebServerSetHistorySizeDialog: public CEikDialog
{
public:
	CWebServerSetHistorySizeDialog(CWebServerEng* aModel);
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
private:
	CWebServerEng* iModel;
};

class CWebServerConsoleDialog: public CEikDialog
{
public:
	CWebServerConsoleDialog(CWebServerEng* aModel);
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
private:
	CWebServerEng* iModel;
};

class CWebServerDirDialog: public CEikDialog
{
public:
	CWebServerDirDialog(CWebServerEng* aModel);
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
private:
	CWebServerEng* iModel;
};

class CWebServerHttpMethodsDialog: public CEikDialog
{
public:
	CWebServerHttpMethodsDialog(CWebServerEng* aModel);
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
private:
	CWebServerEng* iModel;
};

class CWebServerMiscDataDialog: public CEikDialog
{
public:
	CWebServerMiscDataDialog(CWebServerEng* aModel);
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
private:
	CWebServerEng* iModel;
};


class CWebServerMimeTypesDialog: public CEikDialog
{
public:
	CWebServerMimeTypesDialog(CWebServerEng* aModel);
private:
	void PreLayoutDynInitL();
	void PostLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
private:
	CEikColumnListBox* iListBox;
	CWebServerEng* iModel;
	TBool iModelModified;
};

class CWebServerAddMimeTypeDialog: public CEikDialog
{
public:
	CWebServerAddMimeTypeDialog(CDesCArrayFlat* aModel,TInt* aPos);
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButton);
	TBool FindExtension(const TDesC& aExt) const;
private:
	CDesCArrayFlat*  iModel;
	TInt* iPos;
};
#endif //CALYPSO

#endif
