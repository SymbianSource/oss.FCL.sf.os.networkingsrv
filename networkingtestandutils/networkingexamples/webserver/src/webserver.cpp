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
// webserver.cpp - http server main module
//

#ifdef MAKE_EXE_APPLICATION
#include <eikstart.h>
#endif

#include "webserver.h"

//
// EXPORTed functions
//

EXPORT_C CApaApplication* NewApplication()
	{
	return new CWebServerApplication;
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
// Application class, CWebServerApplication
//
//

TUid CWebServerApplication::AppDllUid() const
	{
	return KUidWebServer;
	}

CApaDocument* CWebServerApplication::CreateDocumentL()
	{
	return CWebServerDocument::NewL(*this);	// Construct the document
	}


//
//
// Document class, CWebServerDocument
//
//

// C++ constructor
CWebServerDocument::CWebServerDocument(CEikApplication& aApp)
		: CEikDocument(aApp)
	{
	}

// Implement two-phase construction for document
CWebServerDocument* CWebServerDocument::NewL(CEikApplication& aApp)
	{
	CWebServerDocument* self=new (ELeave) CWebServerDocument(aApp);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

void CWebServerDocument::ConstructL()
	{
	iModel = CWebServerEng::NewL();		// Construct the model
	}

CWebServerDocument::~CWebServerDocument()
	{
	delete iModel;						// delete the model
	}

CEikAppUi* CWebServerDocument::CreateAppUiL()
	{
    return new(ELeave) CWebServerAppUi;
	}


//
//
// App UI class, CWebServerAppUi
//
//

void CWebServerAppUi::ConstructL()
{
    BaseConstructL();
	iModel=STATIC_CAST(CWebServerDocument*,iDocument)->Model();
    iAppView=new(ELeave) CWebServerAppView;
    iAppView->ConstructL(ClientRect(),iModel);
#if EPOC_SDK < 0x06000000
	CEikFileNameLabel* filenameLabel=STATIC_CAST(CEikFileNameLabel*, iToolBar->ControlById(EWSCmdFileName));
	filenameLabel->UpdateL();
#endif
	// Read Config File.
	TInt err;
	TRAP(err,iModel->iServerEnv->ReadConfigFileL(CFG_FILENAME));
	if ((err == KErrNotFound) || (err == KErrPathNotFound))
	{
		iAppView->ShowWarningL(R_CONFIG_FILE_MISSING_WAR);
		iModel->iServerEnv->CheckAndSetDefaultL();
	}
	else
	{
		User::LeaveIfError(err);
	}
	TRAP(err,iModel->iHttpTypes.ReadHttpTypesFileL(HTTP_TYPES_FILENAME));
	if ((err == KErrNotFound) || (err == KErrPathNotFound))
	{
		iAppView->ShowWarningL(R_TYPES_FILE_MISSING_WAR);
		iModel->iHttpTypes.SetDefaultHttpTypesL();
	}
	else
	{
		User::LeaveIfError(err);
	}
}

CWebServerAppUi::~CWebServerAppUi()
	{
#ifndef CALYPSO
		RemoveFromStack(iAppView);
#endif
    delete iAppView;
	}


#ifndef CALYPSO
void CWebServerAppUi::AboutDialogL() const
{
	CEikDialog* dialog = new (ELeave) CEikDialog();
	dialog->ExecuteLD(R_WS_ABOUT_WEBSERVER);	//Final D means the dialog is destructed by itself
}

void CWebServerAppUi::MimeTypesDialogL()
{

	CEikDialog* dialog = new (ELeave) CWebServerMimeTypesDialog(iModel);
	if (!dialog->ExecuteLD(R_WS_MIME_TYPES_DIALOG))	
		return;
}

void CWebServerAppUi::MiscDataDialogL()
{

	CEikDialog* dialog = new (ELeave) CWebServerMiscDataDialog(iModel);
	if (!dialog->ExecuteLD(R_WS_MISC_DATA_DIALOG))	
		return;
}

void CWebServerAppUi::HttpMethodsDialogL()
{

	CEikDialog* dialog = new (ELeave) CWebServerHttpMethodsDialog(iModel);
	if (!dialog->ExecuteLD(R_WS_HTTP_METHODS_DIALOG))	
		return;
}

void CWebServerAppUi::DirDialogL()
{

	CEikDialog* dialog = new (ELeave) CWebServerDirDialog(iModel);
	if (!dialog->ExecuteLD(R_WS_DIR_DIALOG))	
		return;
}

void CWebServerAppUi::ConsoleDialogL()
{
	CEikDialog* dialog = new (ELeave) CWebServerConsoleDialog(iModel);
	if (!dialog->ExecuteLD(R_WS_CONSOLE_DIALOG))	
		return;
}

void CWebServerAppUi::SetHistorySizeDialogL()
{
	CEikDialog* dialog = new (ELeave) CWebServerSetHistorySizeDialog(iModel);
	if (!dialog->ExecuteLD(R_WS_SET_HISTORY_SIZE_DIALOG))
	{
		iAppView->Console()->SetHistorySizeL(iModel->iServerEnv->iHistorySize);
		return;
	}
}
#endif //CALYPSO

void CWebServerAppUi::ResetTotalCounterL()
{
	iModel->iTotalNumberConnections = 0;
	iAppView->UpdateNumberConL();
}

void CWebServerAppUi::StartWebServer()
{
	if (iModel->iWebServerState == CWebServerEng::ENotStarted)
	{	
		TRAPD(err,iModel->StartWebServerL());
		if (err < KErrNone)
		{
			iAppView->ShowError(err);
		}
	}
}


void CWebServerAppUi::HandleCommandL(TInt aCommand)
	{
	switch (aCommand)
		{
	// From Epoc-WS menu.
	case EWSCmdStart:
		StartWebServer();
		break;
	case EWSCmdCancel:
		iModel->Reset();
		//iModel->Cancel();
		break;
	case EEikCmdExit:
#ifdef CALYPSO
	case EAknSoftkeyExit:
	case EAknCmdExit:
#endif
		iModel->iServerEnv->SaveConfigFileL(CFG_FILENAME);
		iModel->iHttpTypes.SaveHttpTypesFileL(HTTP_TYPES_FILENAME);
		//iModel->Cancel();
		iModel->Reset();
		Exit();
		break;
	// View Menu.
	case EWSCmdClearConsole:
		iAppView->ClearScreenL();
		break;
	case EWSCmdResetTotalCounter:
		ResetTotalCounterL();
		break;
#ifndef CALYPSO		// Dialogs
	case EWSCmdSetHistorySize:
		SetHistorySizeDialogL();
		 break;
	case EWSCmdConsole:
		ConsoleDialogL();
		break;
	// Options Menu.
	case EWSCmdDir:
		DirDialogL();
		break;
	case EWSCmdHttpMethods:
		HttpMethodsDialogL();
		break;
	case EWSCmdMiscData:
		MiscDataDialogL();
		break;
	case EWSCmdMimeTypes:
		MimeTypesDialogL();
		break;
	case EWSCmdAbout:
		AboutDialogL();
		break;
#else
	case EWSCmdOption:
		//OptionView();
		break;
#endif	//CALYPSO
	default:
		break;
		}
	}


void CWebServerAppUi::DynInitMenuPaneL(TInt aMenuId,CEikMenuPane* aMenuPane)
{
	switch(aMenuId)
	{
		case R_WS_OPTIONS:
			if (iModel->iWebServerState != CWebServerEng::ENotStarted)
			{
			
				aMenuPane->SetItemDimmed(EWSCmdDir,ETrue);
				aMenuPane->SetItemDimmed(EWSCmdHttpMethods,ETrue);
				aMenuPane->SetItemDimmed(EWSCmdMiscData,ETrue);
				aMenuPane->SetItemDimmed(EWSCmdMimeTypes,ETrue);
			}
			else
			{
				aMenuPane->SetItemDimmed(EWSCmdDir,EFalse);
				aMenuPane->SetItemDimmed(EWSCmdHttpMethods,EFalse);
				aMenuPane->SetItemDimmed(EWSCmdMiscData,EFalse);
				aMenuPane->SetItemDimmed(EWSCmdMimeTypes,EFalse);
			}
			break;
		case R_WS_FILE:
			if (iModel->iWebServerState != CWebServerEng::ENotStarted)
			{
				aMenuPane->SetItemDimmed(EWSCmdStart,ETrue);
				
			}
			else
			{
				aMenuPane->SetItemDimmed(EWSCmdStart,EFalse);
			}
			break;
		default:
			break;
	}
}


//
//
// Application view class, CWebServerAppView
//
//

void CWebServerAppView::ConstructL(const TRect& aRect, CWebServerEng* aModel)
    {
	iModel=aModel; // Give the app view a pointer to the model
	iModel->SetObserver(this);
	CreateWindowL();
#if EPOC_SDK >= 0x06000000
    SetRect(aRect);
#else
    SetRectL(aRect);
#endif
	iContext = this;
	iBrushStyle = CGraphicsContext::ESolidBrush;
	iBrushColor = KRgbWhite;
	CreateTopLabelL();
	CreateBottomLabelL();
	CreateBigConsoleL();
    ActivateL();
    }

void CWebServerAppView::CreateBigConsoleL()
{
	iConsole = new (ELeave) CConsoleControl;
        TPoint pos  = Position();
        TSize  size = Size();
        pos.iY += LABEL_HEIGHT;
        size.iHeight -= 2 * LABEL_HEIGHT;
	iConsole->ConstructL(pos,size,CEikConsoleScreen::ENoInitialCursor);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
	iConsole->SetHistorySizeL(iModel->iServerEnv->iHistorySize);
}

void CWebServerAppView::CreateTopLabelL()
{
	iTopLabel = new (ELeave) CEikLabel;
	iTopLabel->SetContainerWindowL(*this);
	iTopLabel->SetBufferReserveLengthL(LABEL_BUFFER_SIZE);
	iTopLabel->SetAlignment(EHCenterVCenter);
	

	// Get the current window rectagle	
	TRect rect = Rect();
	rect.iBr.iY = rect.iTl.iY + LABEL_HEIGHT; // Set the label window rectangle.
#if EPOC_SDK >= 0x06000000
	iTopLabel->SetRect(rect);
#else
	iTopLabel->SetRectL(rect);
#endif
//	iTopLabel->SetTextL(HEADER_TITLE);
    TBuf<LABEL_BUFFER_SIZE> name;
    CEikonEnv::Static()->ReadResource( name, R_TOP_TITLE ); // uses text from resource file
    iTopLabel->SetTextL(name);

	iTopLabel->DrawNow();
}

void CWebServerAppView::CreateBottomLabelL()
{
	iBottomLabel = new (ELeave) CEikLabel;
	iBottomLabel->SetBufferReserveLengthL(LABEL_BUFFER_SIZE);
	iBottomLabel->SetContainerWindowL(*this);
	
	// Get the current window rectagle	
	TRect rect = Rect();
	rect.iTl.iY = rect.iBr.iY - LABEL_HEIGHT; // Set the label window rectangle.
#if EPOC_SDK >= 0x06000000
	iBottomLabel->SetRect(rect);
#else
	iBottomLabel->SetRectL(rect);
#endif
	iBottomLabel->SetAlignment(EHCenterVCenter);

	TBuf<LABEL_BUFFER_SIZE> name;
    CEikonEnv::Static()->ReadResource( name, R_BOTTOM_TITLE );

	TBuf<LABEL_BUFFER_SIZE> msg;
	msg.Format(name,iModel->iCurrentNumberConnections,iModel->iTotalNumberConnections);
	iBottomLabel->SetTextL(msg);

//	iBottomLabel->SetTextL(FOOTER_TITLE); //Get the Text from the Resource file

}



CWebServerAppView::~CWebServerAppView()
	{
	delete iConsole;
	delete iTopLabel;
	delete iBottomLabel;
	}

TInt CWebServerAppView::CountComponentControls() const
	{
	return 3;
	}


CCoeControl* CWebServerAppView::ComponentControl(TInt aIndex) const
	{
		switch(aIndex)
		{
		case 0:
			return iConsole;
		case 1:
			return iTopLabel;
		case 2:
			return iBottomLabel;
		default:
			return NULL;
		}
	}


void CWebServerAppView::Draw(const TRect& /*aRect*/) const
	{
		CWindowGc& gc = SystemGc();
		TRect consolerect = iConsole->Rect();
		consolerect.Grow(1,1);
		gc.DrawRect(consolerect);
		gc.DrawRect(iTopLabel->Rect());
		gc.DrawRect(iBottomLabel->Rect());
	}

void CWebServerAppView::HandleProgressEvent()
{
	iConsole->Write(iModel->iStatusText);
}

void CWebServerAppView::Write(const TDesC &aMsg)
{
	iConsole->Write(aMsg);
}

void CWebServerAppView::ShowError(TInt aId)
{
	TBuf<256> aux;
	iEikonEnv->GetErrorText(aux,aId);
	iEikonEnv->AlertWin(_L("Socket initialization ERROR"),aux);
}

void CWebServerAppView::ShowWarningL(const TInt aWarning)
{
	iEikonEnv->InfoWinL(R_WARNING,aWarning);
}

void CWebServerAppView::UpdateNumberConL()
{
	TBuf<LABEL_BUFFER_SIZE> name;
    CEikonEnv::Static()->ReadResource( name, R_BOTTOM_TITLE );  // uses text from resource file

	TBuf<LABEL_BUFFER_SIZE> msg;
	// msg.Format(_L("Current Connections: %4d Total Connections: %4d"),iModel->iCurrentNumberConnections,iModel->iTotalNumberConnections);	
	msg.Format(name,iModel->iCurrentNumberConnections,iModel->iTotalNumberConnections);
	iBottomLabel->SetTextL(msg);
	iBottomLabel->DrawNow();
}

void CWebServerAppView::ShowError(const TDes &msg)
{
	iEikonEnv->InfoMsg(msg);
}

void CWebServerAppView::ShowError(const TDes &msg, TInt aErr)
{
	TBuf<100> txt;
	TBuf<100> txt2;

	txt.Format(msg);
	iEikonEnv->GetErrorText(txt2,aErr);
	txt.AppendFormat(txt2);
	iEikonEnv->InfoMsg(txt);
}


void CWebServerAppView::ClearScreenL()
{
	delete iConsole;
	iConsole=NULL;
	CreateBigConsoleL();	
}



//void CWebServerAppView::HandlePointerEventL(const TPointerEvent& /*aPointerEvent*/)
//	{
//	}

//
//
// Dialog Classes.
//
//

#ifndef CALYPSO
// // //
// CWebServerSetHistorySizeDialog class. //
// // //


CWebServerSetHistorySizeDialog::CWebServerSetHistorySizeDialog(CWebServerEng* aModel)
{
	iModel = aModel;
}


void CWebServerSetHistorySizeDialog::PreLayoutDynInitL()
{
	SetNumberEditorValue(EWSSetHistorySize,iModel->iServerEnv->iHistorySize);
}


TBool CWebServerSetHistorySizeDialog::OkToExitL(TInt aButton)
{

	switch (aButton)
	{
	case EEikBidOk:
		iModel->iServerEnv->iHistorySize = NumberEditorValue(EWSSetHistorySize);
		return ETrue;
	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}


// // //
// CWebServerDirDialog class.			 //
// // //


CWebServerDirDialog::CWebServerDirDialog(CWebServerEng* aModel)
{
	iModel = aModel;
}


void CWebServerDirDialog::PreLayoutDynInitL()
{

	SetEdwinTextL(EWSServerPath,&iModel->iServerEnv->ServerPath());
	SetEdwinTextL(EWSCGIPath,&iModel->iServerEnv->CgiPath());
	SetEdwinTextL(EWSBackupPath,&iModel->iServerEnv->BackupPath());
	SetEdwinTextL(EWSErrorPath,&iModel->iServerEnv->ErrorPath());
}


TBool CWebServerDirDialog::OkToExitL(TInt aButton)
{

	TBuf<256> buffer[4];
	TEntry entry;
	TInt err;

	switch (aButton)
	{
	case EEikBidOk:
	
		GetEdwinText(buffer[1],EWSServerPath);
		if (buffer[1].Length()<=0)
		{
			TryChangeFocusToL(EWSServerPath);
			iEikonEnv->LeaveWithInfoMsg(R_WS_NO_SERVER_PATH_SPECIFIED);

		}
		if ((err=iCoeEnv->FsSession().Entry(buffer[1],entry))!=KErrNone)
		{
			TryChangeFocusToL(EWSServerPath);
			if (err !=KErrNotFound)
				iEikonEnv->LeaveWithInfoMsg(R_WS_SERVER_PATH_INVALID);	
			else
				iEikonEnv->LeaveWithInfoMsg(R_WS_SERVER_PATH_DOES_NOT_EXIST);
		}
	
		GetEdwinText(buffer[0],EWSCGIPath);
		if (buffer[0].Length()<=0)
		{
			TryChangeFocusToL(EWSCGIPath);
			iEikonEnv->LeaveWithInfoMsg(R_WS_NO_CGI_PATH_SPECIFIED);

		}
		if ((err =iCoeEnv->FsSession().Entry(buffer[0],entry))!=KErrNone)
		{
			TryChangeFocusToL(EWSCGIPath);
			if (err !=KErrNotFound)
				iEikonEnv->LeaveWithInfoMsg(R_WS_CGI_PATH_INVALID);
			else
				iEikonEnv->LeaveWithInfoMsg(R_WS_CGI_PATH_DOES_NOT_EXIST);
		}
			
		GetEdwinText(buffer[2],EWSBackupPath);
		if (buffer[2].Length()<=0)
		{
			TryChangeFocusToL(EWSBackupPath);
			iEikonEnv->LeaveWithInfoMsg(R_WS_NO_BACKUP_PATH_SPECIFIED);

		}
		if ((err=iCoeEnv->FsSession().Entry(buffer[2],entry))!=KErrNone)
		{
			TryChangeFocusToL(EWSBackupPath);
			if (err !=KErrNotFound)
				iEikonEnv->LeaveWithInfoMsg(R_WS_BACKUP_PATH_INVALID);		
			else
				iEikonEnv->LeaveWithInfoMsg(R_WS_BACKUP_PATH_DOES_NOT_EXIST);
		}
		
		GetEdwinText(buffer[3],EWSErrorPath);
		if (buffer[3].Length()<=0)
		{
			TryChangeFocusToL(EWSErrorPath);
			iEikonEnv->LeaveWithInfoMsg(R_WS_NO_ERROR_PATH_SPECIFIED);

		}
		if ((err=iCoeEnv->FsSession().Entry(buffer[3],entry))!=KErrNone)
		{
			TryChangeFocusToL(EWSErrorPath);
			if (err !=KErrNotFound)
				iEikonEnv->LeaveWithInfoMsg(R_WS_ERROR_PATH_INVALID);
			else
				iEikonEnv->LeaveWithInfoMsg(R_WS_ERROR_PATH_DOES_NOT_EXIST);
			
		}
		
		iModel->iServerEnv->SetServerPathL(buffer[1]);
		iModel->iServerEnv->SetCgiPathL(buffer[0]);
		iModel->iServerEnv->SetBackupPathL(buffer[2]);
		iModel->iServerEnv->SetErrorPathL(buffer[3]);

		iModel->iServerEnv->SaveConfigFileL(CFG_FILENAME);
		return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}


// // //
// CWebServerHttpMethodsDialog class.    //
// // //


CWebServerHttpMethodsDialog::CWebServerHttpMethodsDialog(CWebServerEng* aModel)
{
	iModel = aModel;
}


void CWebServerHttpMethodsDialog::PreLayoutDynInitL()
{
	if(!iModel->iServerEnv->iDeleteMethod)
	{
		SetCheckBoxState(EWSDeleteMethod, CEikButtonBase::EClear);
	}
	else
	{
		SetCheckBoxState(EWSDeleteMethod, CEikButtonBase::ESet);
	}
	if(!iModel->iServerEnv->iPutMethod)
	{
		SetCheckBoxState(EWSPutMethod, CEikButtonBase::EClear);
	}
	else
	{
		SetCheckBoxState(EWSPutMethod, CEikButtonBase::ESet);
	}
}


TBool CWebServerHttpMethodsDialog::OkToExitL(TInt aButton)
{

	switch (aButton)
	{
	case EEikBidOk:
		iModel->iServerEnv->iDeleteMethod = (CheckBoxState(EWSDeleteMethod) == CEikButtonBase::ESet);
		iModel->iServerEnv->iPutMethod = (CheckBoxState(EWSPutMethod) == CEikButtonBase::ESet);
		iModel->iServerEnv->SaveConfigFileL(CFG_FILENAME);
		return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}

// // //
// CWebServerMiscDataDialog class.    //
// // //


CWebServerMiscDataDialog::CWebServerMiscDataDialog(CWebServerEng* aModel)
{
	iModel = aModel;
}


void CWebServerMiscDataDialog::PreLayoutDynInitL()
{
	SetEdwinTextL(EWSDefaultResource,&iModel->iServerEnv->DefaultResource());
	SetNumberEditorValue(EWSPort,iModel->iServerEnv->iPort);
}


TBool CWebServerMiscDataDialog::OkToExitL(TInt aButton)
{
	TBuf<256> buffer;

	switch (aButton)
	{
	case EEikBidOk:
		GetEdwinText(buffer,EWSDefaultResource);
		if (buffer.Length() >0)
		{
			if (iCoeEnv->FsSession().IsValidName(buffer))
			{
				iModel->iServerEnv->iPort = NumberEditorValue(EWSPort);		
				iModel->iServerEnv->SetDefaultResourceL(buffer);
				iModel->iServerEnv->SaveConfigFileL(CFG_FILENAME);
			}
			else
			{
				TryChangeFocusToL(EWSDefaultResource);
				iEikonEnv->LeaveWithInfoMsg(R_WS_DEFAULT_RESOURCE_FILENAME_INVALID);
			}
		}
		else
		{
			TryChangeFocusToL(EWSDefaultResource);
			iEikonEnv->LeaveWithInfoMsg(R_WS_NO_DEFAULT_RESOURCE_SPECIFIED);
		}
		return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}

// // //
// CWebServerMimeTypesDialog class.		 //
// // //

	
CWebServerMimeTypesDialog::CWebServerMimeTypesDialog(CWebServerEng* aModel)
{
	iModel = aModel;
	iModelModified = EFalse;
}

void CWebServerMimeTypesDialog::PostLayoutDynInitL()
{
}

void CWebServerMimeTypesDialog::PreLayoutDynInitL()
{
	iListBox = STATIC_CAST(CEikColumnListBox*,Control(EWSMimeTypesList));
	CDesCArrayFlat* currentlist = iModel->iHttpTypes.HttpTypesArray();

	iListBox->Model()->SetItemTextArray(currentlist);
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);

#if EPOC_SDK >= 0x06000000
	CColumnListBoxData* columnData = iListBox->ItemDrawer()->ColumnData();
#else
	CColumnListBoxData* columnData = iListBox->Model()->ColumnData();
#endif
	columnData->SetColumnFontL(0, iEikonEnv->NormalFont());
	columnData->SetColumnFontL(1, iEikonEnv->NormalFont());

	// Find the correct Width for each column.
	const CFont* font[2];
	TPtrC text;
	TInt width[2],len;
	
	width[0] = 0;
	width[1] = 0;	
	font [0] = columnData->ColumnFont(0);
	font [1] = columnData->ColumnFont(1);

	len = currentlist->Count();

	for (TInt index=0; index<len; index++)
	{
		TPtrC aux1 = currentlist->MdcaPoint(index);
		TextUtils::ColumnText(text,0,&aux1);
		width[0] = Max(width[0],font[0]->TextWidthInPixels(text) + 20);
		
		TextUtils::ColumnText(text,1,&aux1);
		width[1] = Max(width[1],font[1]->TextWidthInPixels(text) + 10);		
	}

	columnData->SetColumnWidthPixelL(0,width[0]);
	columnData->SetColumnWidthPixelL(1,width[1]);

	columnData->SetColumnAlignmentL(0,CGraphicsContext::ELeft);
	columnData->SetColumnAlignmentL(1,CGraphicsContext::ELeft);

	CEikScrollBarFrame* scrollbarframe = iListBox->CreateScrollBarFrameL();
	scrollbarframe->SetScrollBarVisibilityL(CEikScrollBarFrame::EAuto,CEikScrollBarFrame::EAuto);

}


TBool CWebServerMimeTypesDialog::OkToExitL(TInt aButton)
{

	CEikDialog* dialog;
	TInt index=0;

	switch (aButton)
	{
	case EEikBidOk:
		if (iModelModified)
		{
			iModel->iHttpTypes.SaveHttpTypesFileL(HTTP_TYPES_FILENAME);		
		}
		return ETrue;

	case EEikBidCancel:
		return ETrue;
	case EWSBidAdd:
		dialog = new (ELeave) CWebServerAddMimeTypeDialog((CDesCArrayFlat*)iListBox->Model()->ItemTextArray(),&index);
		if (dialog->ExecuteLD(R_WS_ADD_MIME_TYPES_DIALOG))
		{
			iListBox->HandleItemAdditionL();
			iListBox->SetCurrentItemIndex(iListBox->CurrentItemIndex());
			iListBox->DrawNow();
			iModelModified = ETrue;
		}
		return EFalse;
	case EWSBidDel:
		if (((CDesCArray*)iListBox->Model()->ItemTextArray())->Count() > 0)
		{
			if (iEikonEnv->QueryWinL(R_WS_DELETE_MIMETYPE_CONFIRM))
			{
				index = iListBox->CurrentItemIndex();
				((CDesCArray*)iListBox->Model()->ItemTextArray())->Delete(index);
				iListBox->HandleItemRemovalL();
				index = Max(0,index - 1);
				iListBox->SetCurrentItemIndex(index);
				iListBox->DrawNow();
				iModelModified = ETrue;
			}
		}
		return EFalse;
	default:
		return EFalse;
	}
}

// // //
// CWebServerConsoleDialog class.		 //
// // //

CWebServerConsoleDialog::CWebServerConsoleDialog(CWebServerEng* aModel)
{
	iModel = aModel;
}

void CWebServerConsoleDialog::PreLayoutDynInitL()
{
	if(!iModel->iServerEnv->iHttpReq)
	{
		SetCheckBoxState(EWSHttpReq, CEikButtonBase::EClear);
	}
	else
	{
		SetCheckBoxState(EWSHttpReq, CEikButtonBase::ESet);
	}
	if(!iModel->iServerEnv->iHttpResp)
	{
		SetCheckBoxState(EWSHttpResp, CEikButtonBase::EClear);
	}
	else
	{
		SetCheckBoxState(EWSHttpResp, CEikButtonBase::ESet);
	}
	if(!iModel->iServerEnv->iMiscData)
	{
		SetCheckBoxState(EWSMiscData, CEikButtonBase::EClear);
	}
	else
	{
		SetCheckBoxState(EWSMiscData, CEikButtonBase::ESet);
	}
}

TBool CWebServerConsoleDialog::OkToExitL(TInt aButton)
{

	switch (aButton)
	{
	case EEikBidOk:
		iModel->iServerEnv->iHttpReq = (CheckBoxState(EWSHttpReq) == CEikButtonBase::ESet);
		iModel->iServerEnv->iHttpResp = (CheckBoxState(EWSHttpResp) == CEikButtonBase::ESet);
		iModel->iServerEnv->iMiscData =  (CheckBoxState(EWSMiscData) == CEikButtonBase::ESet);
		iModel->iServerEnv->SaveConfigFileL(CFG_FILENAME);
		return ETrue;

	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}

// // //
// CWebServerAddMimeTypeDialog class.    //
// // //


CWebServerAddMimeTypeDialog::CWebServerAddMimeTypeDialog(CDesCArrayFlat* aModel,TInt* aPos)
{
	iModel = aModel;
	iPos = aPos;
}


void CWebServerAddMimeTypeDialog::PreLayoutDynInitL()
{
}


TBool CWebServerAddMimeTypeDialog::OkToExitL(TInt aButton)
{
	TBuf<64> buffer;
	TBuf<128> newrow;
	switch (aButton)
	{
	case EEikBidOk:
		GetEdwinText(buffer,EWSExtension);
		if (buffer.Length()>0)
		{
			buffer.Trim();
			newrow = buffer;
			GetEdwinText(buffer,EWSMimeType);
			if (buffer.Length()>0)
			{
		
				if (!FindExtension(newrow))
				{
					newrow.Append(KColumnListSeparator); // KColumnListSeparator = '\t';
					buffer.Trim();
					newrow.Append(buffer);
					*iPos = iModel->InsertIsqL(newrow);
				}
				else
				{
					TryChangeFocusToL(EWSExtension);
					iEikonEnv->LeaveWithInfoMsg(R_WS_EXTENSION_ALREADY_EXISTS);
				}
			}
			else
			{
				TryChangeFocusToL(EWSMimeType);
				iEikonEnv->LeaveWithInfoMsg(R_WS_NO_MIMETYPE_SPECIFIED);
			}
		}
		else
		{
			TryChangeFocusToL(EWSExtension);
			iEikonEnv->LeaveWithInfoMsg(R_WS_NO_EXTENSION_SPECIFIED);
		}
		return ETrue;
	case EEikBidCancel:
		return ETrue;
	default:
		return EFalse;
	}
}


TBool CWebServerAddMimeTypeDialog::FindExtension(const TDesC& aExt) const
{
	TBool res = EFalse;
	TInt len = iModel->MdcaCount();
	TBuf<9> extension = aExt;

	extension.Append(KColumnListSeparator);
	
	for (TInt index = 0; ((index < len) && (!res)); index++)
	{
		if ((*iModel)[index].Find(extension) == 0) res = ETrue;
	}

	return res;
}
#endif //CALYPSO

//
//	CConsoleControl
//

CConsoleControl::~CConsoleControl()
	{
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

void CConsoleControl::SetHistorySizeL(const TInt aSize)  // Carlos addition
{
	iHistory = aSize;
	iConsole->SetHistorySizeL(iHistory,0);		
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
