// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "Nd_dlgsv.h"

#include <metadatabase.h>
#include <commsdattypesv1_1.h>

_LIT(KNDDialogMutexName,"NdDialogStartupMutex");
_LIT(KNDDialogSemaphoreName,"NdDialogStartupSem");
_LIT(KDialogServerFileName,"DummyAGENTDIALOG.DLL");

//
// CNetDialDialogServer
//

CNetDialDialogServer::CNetDialDialogServer(TInt aPriority)
	: CServer2(aPriority)
	{}

CNetDialDialogServer* CNetDialDialogServer::NewL()
//
// Create and start a new count server
//
	{
	CNetDialDialogServer* pS=new(ELeave) CNetDialDialogServer(EPrioritySupervisor);
	CleanupStack::PushL(pS);
	pS->StartL(DIALOG_SERVER_NAME);
	CleanupStack::Pop();
	return pS;
	}

CSession2* CNetDialDialogServer::NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const
//
// Create a new server session
//
	{
	// check we're the right version
	TVersion v(KDialogServMajorVersionNumber,KDialogServMinorVersionNumber,KDialogServBuildVersionNumber);
	if (!User::QueryVersionSupported(v,aVersion))
		User::Leave(KErrNotSupported);
	// make new session
	RThread client;
	aMessage.Client(client);
	return CNetDialDialogSession::NewL(client, (CNetDialDialogServer*)this);
	}

//
// CNetDialDialogSession
//

CNetDialDialogSession* CNetDialDialogSession::NewL(RThread aClient, CNetDialDialogServer* aServer)
	{
	CNetDialDialogSession* r=new(ELeave) CNetDialDialogSession(aClient,aServer);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}

CNetDialDialogSession::CNetDialDialogSession(RThread /*aClient*/, CNetDialDialogServer* aServer)
	: CSession2(),
	iNetDialDialogSvr(aServer),
	iIAP(0),
	iModemId(0),
	iLocationId(0),
	iConNames(TIspConnectionNames()),
	iPrefs(TConnectionPrefs()),
	iBool(EFalse),
	iBufPtr(NULL,0),
	iPctWriteBuffer(NULL,0)
	{}

void CNetDialDialogSession::ConstructL()
	{
	iBusy=EFalse;
	iPctOpen=EFalse;
	iReadPctOutstanding=EFalse;
	iDestroyPctNotfcnOutstanding=EFalse;

	iBuffer=HBufC::NewL(KMaxWriteBufferSize);
	TPtr temp=iBuffer->Des();
	iBufPtr.Set(temp);
	iPctWriteBuffer.Set(temp);
	
	iNetDialDialogSvr->iSessionCount++;
	}

CNetDialDialogSession::~CNetDialDialogSession()
	{
	delete iBuffer;
	CancelEverything();

	//
	// Shutdown if this is the last session disconnecting...
	//
	iNetDialDialogSvr->iSessionCount--;
	if (iNetDialDialogSvr->iSessionCount == 0)
		{
		CActiveScheduler::Stop();
		}
	}

void CNetDialDialogSession::ServiceL(const RMessage2& aMessage)
	{
	TRAPD(err,DispatchMessageL(aMessage));
	if(err!=KErrNone)
		aMessage.Complete(err);
	}

void CNetDialDialogSession::DispatchMessageL(const RMessage2& aMessage)
	{
	switch (aMessage.Function())
        {
	case EGetIAP:
		CheckBusyL();
		iMessage=aMessage;
		iState=EGetIAP;
		GetIAPL();
		break;
	case EGetModemAndLocation:
		CheckBusyL();
		iMessage=aMessage;
		iState=EGetModemAndLocation;
		GetModemAndLocationL();
		break;
	case EWarnNewIAP:
		CheckBusyL();
		iMessage=aMessage;
		iState=EWarnIAP;
		WarnIAPL();
		break;
	case EGetLogin:
		CheckBusyL();
		iMessage=aMessage;
		iState=ELogin;
		GetLoginL();
		break;
	case EGetAuthentication:
		CheckBusyL();
		iMessage=aMessage;
		iState=EAuthentication;
		GetAuthenticationL();
		break;
	case EGetReconnectReq:
		CheckBusyL();
		iMessage=aMessage;
		iState=EReconnectReq;
		ReconnectReqL();
		break;
	case EOpenPct:
		CheckBusyL();
		CheckPctL(aMessage);
		iMessage=aMessage;
		iState=EPct;
		OpenPctL();
		break;
	case EWritePct:
		CheckPctL(aMessage);
		iMessage=aMessage;
		iState=EPct;
		WritePct();
		break;
	case EReadPct:
		CheckPctL(aMessage);
		iReadPctMessage=aMessage;
		iState=EPct;
		ReadPctL();
		break;
	case EDestroyPctNotification:
		CheckPctL(aMessage);
		iDestroyNotificationMessage=aMessage;
		iState=EPct;
		DestroyPctNotificationL();
		break;
	case EClosePct:
		CheckPctL(aMessage);
		iMessage=aMessage;
		iState=EPct;
		ClosePct();
		break;
	case EWarnQoS:
		CheckBusyL();
		iMessage=aMessage;
		iState=EWarnQoSState;
		WarnQoSL();
		break;
	case ECancelGetIAP:
	case ECancelGetModemAndLocation:
	case ECancelWarnIAP:
	case ECancelLogin:
	case ECancelAuthenticate:
	case ECancelReconnect:
	case ECancelReadPct:
	case ECancelDestroyPctNotification:
	case ECancelWarnQoS:
		Cancel((TDialogServRqst)aMessage.Function());
		aMessage.Complete(KErrNone);
		break;
	case EObserveDialogs:
		__ASSERT_DEBUG(iNetDialDialogSvr->iDialogObserver==NULL, User::Invariant());
		iNetDialDialogSvr->iDialogObserver = new(ELeave)CDialogObserverReq(aMessage);
		break;
	case ESetReconnectResponse:
		iNetDialDialogSvr->iReconnectResponse = (TBool)aMessage.Int0();
		aMessage.Complete(KErrNone);
		break;
	case ESetQoSWarnResponse:
		iNetDialDialogSvr->iQoSWarnResponse = (TBool)aMessage.Int0();
		aMessage.Complete(KErrNone);
		break;
	case ECancelObserveDialogs:
		if(iNetDialDialogSvr->iDialogObserver)
			{
			iNetDialDialogSvr->iDialogObserver->Cancel();
			delete iNetDialDialogSvr->iDialogObserver;
			iNetDialDialogSvr->iDialogObserver = NULL;
			}
		aMessage.Complete(KErrNone);
		break;
	case ESetTestNumber:
		iNetDialDialogSvr->iTestNumber = aMessage.Int0();
		aMessage.Complete(KErrNone);
		break;
	default:
		PanicClient(EClientBadRequest);
		break;
        }
	}

void CNetDialDialogSession::CheckBusyL()
	{
	if(iBusy)
		User::Leave(KErrInUse);

	iBusy=ETrue;
	}

TInt CNetDialDialogSession::GeneralCallBack(TAny* aSession)
	{
	__ASSERT_ALWAYS(aSession,PanicServer(ESvrGeneralCallBackNoSession));

//	CNetDialDialogSession* self = STATIC_CAST(CNetDialDialogSession*,aSession);

//	switch (self->iState)
//       {
//	case EGetIAP:
//		self->GetIAPCompleteL(self->iIAPSelection->Status());
//		break;
//	case EGetModemAndLocation:
//		self->GetModemAndLocationCompleteL(self->iModemAndLocationSelection->Status());
//		break;
//	case EWarnIAP:
//		self->WarnIAPCompleteL(self->iIAPWarning->Status());
//		break;
//	case ELogin:
//		self->CompleteLogin(self->iLogin->Status());
//		break;
//	case EAuthentication:
//		self->CompleteAuthentication(self->iAuthenticate->Status());
//		break;
//	case EReconnectReq:
//		self->CompleteReconnectReq(self->iReconnect->Status());
//		break;
//	case EWarnQoSState:
//		self->WarnQoSCompleteL(self->iQoSWarning->Status());
//		break;
//	default:
//		PanicServer(ESvrGeneralCallBackError);
//		break;
//		}

	return KErrNone;
	}

//
//GetModemAndLocation
//
void CNetDialDialogSession::GetModemAndLocationL()
	{
	// The dummy dialog server works only with TS_GenConn
	TPckgBuf<TUint32> modemIdPckg;
	TUint32& modemId = ( modemIdPckg )();
	TInt ret=KErrNone;

	TPckgBuf<TUint32> locationIdPckg;
	TUint32& locationId = ( locationIdPckg )();

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* session = CMDBSession::NewL(KCDVersion1_2);
#else
	CMDBSession* session = CMDBSession::NewL(KCDVersion1_1);
#endif
	CleanupStack::PushL(session);

	CCDIAPRecord* iap = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	CleanupStack::PushL(iap);

	// rather than using a record set and loading the whole modem bearer table just manually find the first one
	// assumption here that we will find one quickly (this is test code)
	TInt err = KErrGeneral;
	for (TUint i=0;i<10 && err!=KErrNone;i++)
		{
		iap->SetRecordId(i);
		TRAP(err,iap->LoadL(*session));
		}

	if (err != KErrNone)
		{
		User::Leave(KErrArgument);
		}

	modemId = iap->RecordId();

	locationId = iap->iLocation;

	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);

	TRAP(ret,iMessage.WriteL(0,modemIdPckg)); 
	TRAP(ret,iMessage.WriteL(1,locationIdPckg)); 

	iBusy=EFalse;

	iMessage.Complete(KErrNone);
	}

void CNetDialDialogSession::GetModemAndLocationCompleteL(TInt )
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	}


//
// Get IAP
//

void CNetDialDialogSession::GetIAPL()
	{
	// The dummy dialog server works only with TS_GenConn
	TPckgBuf<TConnectionPrefs> prefsPckg;
	TRAPD(ret,iMessage.ReadL(1,prefsPckg));
	__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadDescriptor));

	TConnectionPrefs& prefs = ( prefsPckg )();

	
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* session = CMDBSession::NewL(KCDVersion1_2);
#else
	CMDBSession* session = CMDBSession::NewL(KCDVersion1_1);
#endif
	CleanupStack::PushL(session);

	CCDConnectionPrefsRecord* pref = static_cast<CCDConnectionPrefsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdConnectionPrefsRecord));
	CleanupStack::PushL(pref);
	pref->iBearerSet = ECommDbBearerWcdma;
	pref->iDirection = prefs.iDirection;
	if (!pref->FindL(*session))
		{
		User::Leave(KErrArgument);
		}

	TPckgBuf<TUint32> IAPIdPckg;
	TUint32& IAPId = ( IAPIdPckg )();
	IAPId = pref->iDefaultIAP;
	TRAP_IGNORE(iMessage.WriteL(0,IAPIdPckg)); 

	CleanupStack::PopAndDestroy(pref);
	CleanupStack::PopAndDestroy(session);
	if(iNetDialDialogSvr->iTestNumber == 20201)
		{
		if(prefs.iDirection != ECommDbConnectionDirectionOutgoing &&
		   prefs.iBearerSet != ECommDbBearerWcdma)
			User::Leave(KErrArgument);
		}
	else if(iNetDialDialogSvr->iTestNumber == 20202)
		{
		if(prefs.iDirection == ECommDbConnectionDirectionOutgoing &&
		   prefs.iBearerSet == 1)
			User::Leave(KErrArgument);
		}

	iBusy=EFalse;

	iMessage.Complete(KErrNone);
/*
	TRAPD(ret,iMessage.ReadL(iMessage.Ptr1(),iPrefs));
	__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadDescriptor));

	const TInt lastError = iMessage.Int2();

	iDb=CCommsDatabase::NewL(EDatabaseTypeIAP);
	iIAPSelection=CIAPSelection::NewL(iDb,this);

	TCallBack callBack(GeneralCallBackL,this);
	iIAPSelection->GetIAPL(iIAP(),iPrefs(),lastError,callBack);
*/
	}

void CNetDialDialogSession::GetIAPCompleteL(TInt )
	{
/*
	if (aStatus == KErrNone)
		{
		TRAPD(ret,WriteL(iMessage.Ptr0(),iIAP)); 
		__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadRecordId));
		}

	delete iIAPSelection;
	iIAPSelection = NULL;
	delete iDb;
	iDb = NULL;

	iBusy=EFalse;

	iMessage.Complete(aStatus);
*/
	}

//
// Warn IAP
//
void CNetDialDialogSession::WarnIAPL()
	{
	// The dummy dialog server works only with TS_GenConn
	TPckgBuf<TBool> boolFlagPckg;
	TBool& flag = ( boolFlagPckg )();
	TInt ret=KErrNone;

	if(iNetDialDialogSvr->iTestNumber == 20258 ||
	   iNetDialDialogSvr->iTestNumber == 20262 )
		{
		flag=EFalse;
		}
	else
		{
		flag=ETrue;
		}

	TRAP(ret,iMessage.WriteL(3,boolFlagPckg));

	iMessage.Complete(KErrNone);

/*
	TRAPD(ret,iMessage.ReadL(iMessage.Ptr0(),iPrefs));
	__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadDescriptor));
	const TInt lastError = iMessage.Int1();
	TRAP(ret,iMessage.ReadL(iMessage.Ptr2(),iNewIAPName));
	__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadDescriptor));

	iDb = CCommsDatabase::NewL(EDatabaseTypeIAP);
	iIAPWarning = CIAPWarning::NewL(this);

	TCallBack callBack(GeneralCallBack,this);
	iIAPWarning->WarnIAPL(iPrefs(),lastError,iNewIAPName,iBool(),callBack);
*/
	}

void CNetDialDialogSession::WarnIAPCompleteL(TInt )
	{
/*	if (aStatus == KErrNone)
		{
		TRAPD(ret,WriteL(iMessage.Ptr3(),iBool)); 
		__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadRecordId));
		}

	delete iIAPWarning;
	iIAPWarning = NULL;
	delete iDb;
	iDb = NULL;

	iBusy = EFalse;

	iMessage.Complete(aStatus);
*/
	}

//
// Login Dialog
//

void CNetDialDialogSession::GetLoginL()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	iLogin=CLogin::NewL(this);

	TRAPD(ret,iMessage.ReadL(iMessage.Ptr0(),iUsername));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
	TRAP(ret,iMessage.ReadL(iMessage.Ptr1(),iPassword));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
//	const TBool isReconnect = iMessage.Int2();		// this will be unused in this implementation

	TCallBack callBack(GeneralCallBack,this);
	iLogin->GetUserPass(iUsername,iPassword,callBack);
*/
	}

void CNetDialDialogSession::CompleteLogin(TInt )
	{
/*	if(aStatus==KErrNone)
		{
		TRAPD(ret,WriteL(iMessage.Ptr0(),iUsername));
		if (ret!=KErrNone)
			PanicClient(EClientBadDescriptor);
		TRAP(ret,WriteL(iMessage.Ptr1(),iPassword));
		if (ret!=KErrNone)
			PanicClient(EClientBadDescriptor);
		}

	delete iLogin;
	iLogin=NULL;
	iBusy=EFalse;

	iMessage.Complete(aStatus); */
	}

//
// Authentication Dialog
//

void CNetDialDialogSession::GetAuthenticationL()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	iAuthenticate=CAuthenticate::NewL(this);

	TRAPD(ret,iMessage.ReadL(iMessage.Ptr0(),iUsername));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
	TRAP(ret,iMessage.ReadL(iMessage.Ptr1(),iPassword));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
//	const TBool isReconnect = iMessage.Int2();	// this will be unused in this implementation

	TCallBack callBack(GeneralCallBack,this);
	iAuthenticate->GetUserPassL(iUsername,iPassword,callBack);
*/
	}

void CNetDialDialogSession::CompleteAuthentication(TInt )
	{
/*	if(aStatus==KErrNone)
		{
		TRAPD(ret,WriteL(iMessage.Ptr0(),iUsername));
		if (ret!=KErrNone)
			PanicClient(EClientBadDescriptor);
		TRAP(ret,WriteL(iMessage.Ptr1(),iPassword));
		if (ret!=KErrNone)
			PanicClient(EClientBadDescriptor);
		}

	delete iAuthenticate;
	iAuthenticate=NULL;
	iBusy=EFalse;

	iMessage.Complete(aStatus); */
	}

//
// Reconnection Request
//

void CNetDialDialogSession::ReconnectReqL()
	{
//	iReconnect=CReconnect::NewL(this);

//	TCallBack callBack(GeneralCallBack,this);
//	iReconnect->ReconnectL(callBack);

	TBool response = iNetDialDialogSvr->iReconnectResponse;
	TPtr boolDes((TText*)&response,sizeof(response),sizeof(response));
	TRAPD(ret,iMessage.WriteL(0,boolDes));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
	User::After(100000); // 0.1 sec
	iBusy=EFalse;
	iMessage.Complete(KErrNone);
	// Inform server that a dialog has been fired
	// Note have to allow the client of the dialog to run 
	// before the observer of dialogs so wait on a short timer
	// before informing the server
	User::After(100000); // 0.1 sec
	if(iNetDialDialogSvr->iDialogObserver)
		{
		iNetDialDialogSvr->iDialogObserver->Complete();
		delete iNetDialDialogSvr->iDialogObserver;
		iNetDialDialogSvr->iDialogObserver = NULL;
		}
	}

void CNetDialDialogSession::CompleteReconnectReq(TBool )
	{
/*
	TPtr boolDes((TText*)&aBool,sizeof(aBool),sizeof(aBool));
	TRAPD(ret,WriteL(iMessage.Ptr0(),boolDes));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);

	delete iReconnect;
	iReconnect=NULL;
	iBusy=EFalse;

	iMessage.Complete(KErrNone);
*/
	}

//
// Post Connection Terminal
//

void CNetDialDialogSession::CheckPctL(const RMessage2& )
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	if (aMessage.Function()==EOpenPct)
		{
		if (iPctOpen)
			PanicClient(EClientPctAlreadyOpen);
		}
	else
		{
		if (!iPctOpen)
			PanicClient(EClientPctNotOpen);
		if (!iBusy)
			User::Leave(KErrGeneral);
		} */
	}

void CNetDialDialogSession::OpenPctL()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	iPct=CPct::NewL(this);
	iPctOpen=ETrue;
	iMessage.Complete(KErrNone); */
	}

void CNetDialDialogSession::WritePct()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	__ASSERT_DEBUG(iPct, PanicServer(ESvrPctNotOpen));

	TPtr temp(iBufPtr);
	temp.Copy(iPctWriteBuffer);

	TBuf<KMaxWriteBufferSize> newBuffer;
	TRAPD(ret,iMessage.ReadL(iMessage.Ptr0(),newBuffer));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
	else
		{
		if (newBuffer.Length()<=KMaxWriteBufferSize-3)
			{
			_LIT(KEllipsis,"...");
			newBuffer.Append(KEllipsis);
			}
		TInt len=temp.Length()+newBuffer.Length();
		if (len>KMaxWriteBufferSize)
			{
			temp.Copy(temp.Right(temp.Length()-(len-KMaxWriteBufferSize)));
			len=KMaxWriteBufferSize;
			}
		iPctWriteBuffer.Set((TText*)temp.Ptr(),temp.Length(),len);
		iPctWriteBuffer.Append(newBuffer);
		}

	iPct->WritePct(iPctWriteBuffer);
	iMessage.Complete(KErrNone);
*/
	}

void CNetDialDialogSession::ReadPctL()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	__ASSERT_DEBUG(iPct, PanicServer(ESvrPctNotOpen));

	iPctReadBuffer.Zero();
	TRAPD(ret,iReadPctMessage.ReadL(iReadPctMessage.Ptr0(),iPctReadBuffer));
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);
	
	TCallBack readPctCallBack(ReadPctCallBack,this);
	iPct->ReadPctL(iPctReadBuffer,readPctCallBack);
	iReadPctOutstanding=ETrue;
*/
	}
	
TInt CNetDialDialogSession::ReadPctCallBack(TAny* )
	{
/*	
	CNetDialDialogSession* This=(CNetDialDialogSession*)aContext;
	if (This)
		This->ReadPctComplete(This->iPct->Status());
*/
	return KErrNone;
	}	
	
void CNetDialDialogSession::ReadPctComplete(TInt )
	{
/*	
	__ASSERT_DEBUG(iReadPctOutstanding, PanicServer(ESvrNoReadPctOutstanding));

	if(aStatus==KErrNone)
		{
		TInt pos=iPctReadBuffer.Locate('.');
		if (pos>=0)
			{
			TPtrC carriageRet(&KCarriageReturn);
			iPctReadBuffer.Insert(pos,carriageRet);
			}
		TRAPD(ret,WriteL(iReadPctMessage.Ptr0(),iPctReadBuffer));
		if (ret!=KErrNone)
			PanicClient(EClientBadDescriptor);
		}

	iReadPctMessage.Complete(aStatus);
	iReadPctOutstanding=EFalse;
*/
	}
	
void CNetDialDialogSession::DestroyPctNotificationL()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	__ASSERT_DEBUG(iPct, PanicServer(ESvrPctNotOpen));

	TCallBack destroyPctCallBack(DestroyPctCallBack,this);
	iPct->DestroyPctNotificationL(destroyPctCallBack);
	iDestroyPctNotfcnOutstanding = ETrue; */
	}
	
TInt CNetDialDialogSession::DestroyPctCallBack(TAny* )
	{
/*
	CNetDialDialogSession* This=(CNetDialDialogSession*)aContext;
	if (This)
		This->DestroyPctComplete(This->iPct->Status());
*/
	return KErrNone;
	}	
	
void CNetDialDialogSession::DestroyPctComplete(TInt )
	{
/*
	__ASSERT_DEBUG(iDestroyPctNotfcnOutstanding, PanicServer(ESvrNoDestroyPctNotfcnOutstanding));

	iDestroyNotificationMessage.Complete(aStatus);
	iDestroyPctNotfcnOutstanding = EFalse; */
	}
	
void CNetDialDialogSession::ClosePct()
	{
// The dummy dialog server should do nothing for this call as it expects
// tests to run with commdb setup to not prompt
	__ASSERT_DEBUG(EFalse, PanicClient(EClientBadRequest));
/*
	__ASSERT_DEBUG(iPct, PanicServer(ESvrPctNotOpen));
	__ASSERT_ALWAYS(!iReadPctOutstanding, PanicClient(EClientReadPctOutstandingOnClose));
	__ASSERT_ALWAYS(!iDestroyPctNotfcnOutstanding, PanicClient(EClientDestroyPctNotfcnOutstandingOnClose));

	iPctReadBuffer.Zero();
	iPctWriteBuffer.Zero();
	iPct->ClosePct();
	iBusy=EFalse;
	iPctOpen=EFalse;
	iMessage.Complete(KErrNone);
*/
	}

//
// Warn QoS
//

void CNetDialDialogSession::WarnQoSL()
	{
//	iQoSWarning = CQoSWarning::NewL(this);

//	TCallBack callBack(GeneralCallBack,this);
//	iQoSWarning->WarnQoSL(iBool(),callBack);

	// Inform server that a dialog has been fired
	// Note have to allow the client of the dialog to run 
	// before the observer of dialogs so wait on a short timer
	// before informing the server
	if(iNetDialDialogSvr->iDialogObserver)
		{
		iNetDialDialogSvr->iDialogObserver->Complete();
		delete iNetDialDialogSvr->iDialogObserver;
		iNetDialDialogSvr->iDialogObserver = NULL;
		}

	User::After(100000); // 0.1 sec
	
	// Inform AGX of dialog result
	iBool()=iNetDialDialogSvr->iQoSWarnResponse;
	TRAPD(ret,iMessage.WriteL(0,iBool)); 
	if (ret!=KErrNone)
		PanicClient(EClientBadDescriptor);

	iMessage.Complete(KErrNone);
	iBusy=EFalse;
	}

void CNetDialDialogSession::WarnQoSCompleteL(TInt )
	{
//	if (aStatus == KErrNone)
//		{
//		TRAPD(ret,WriteL(iMessage.Ptr0(),iBool)); 
//		__ASSERT_ALWAYS(ret == KErrNone, PanicClient(EClientBadRecordId));
//		}

//	delete iQoSWarning;
//	iQoSWarning = NULL;
	
//	iBusy = EFalse;

//	iMessage.Complete(aStatus);
	}
//=================

void CNetDialDialogSession::Cancel(TDialogServRqst aRequest)
//
// Call DoCancel() not Cancel() because there is no cancel for the notifier
// so we must set a flag and and cancel when it next returns to the RunL()
//
	{
	if(!iBusy)
		return;

	switch (aRequest)
		{
	case ECancelGetIAP:
		if (iState==EGetIAP)
			{
//			if (iIAPSelection)
//				iIAPSelection->DoCancel();
			iBusy=EFalse;
			}
		break;
	case ECancelGetModemAndLocation:
		if (iState==EGetModemAndLocation)
			{
//			if (iModemAndLocationSelection)
//				iModemAndLocationSelection->DoCancel();
			iBusy=EFalse;
			}
		break;
	case ECancelWarnIAP:
		if (iState==EWarnIAP)
			{
//			if (iIAPWarning)
//				iIAPWarning->DoCancel();
			iBusy=EFalse;
			}
		break;
	case ECancelLogin:
		if (iState==ELogin)
			{
//			if (iLogin)
//				iLogin->Cancel();
			iBusy=EFalse;
			}
		break;
	case ECancelAuthenticate:
		if (iState==EAuthentication)
			{	
//			if (iAuthenticate)
//				iAuthenticate->DoCancel();
			iBusy=EFalse;
			}
		break;
	case ECancelReconnect:
		if (iState==EReconnectReq)
			{
//			if (iReconnect)
//				iReconnect->DoCancel();
			iBusy=EFalse;
			}
		break;
	case ECancelReadPct:
		if (iState==EPct && iPctOpen && iReadPctOutstanding)
			{
//			if (iPct)
//				iPct->DoCancel();		// cancels read only
			iReadPctOutstanding=EFalse;
			}
		break;
	case ECancelDestroyPctNotification:
		if (iState==EPct && iDestroyPctNotfcnOutstanding)
			{
//			if (iPct)
//				iPct->DoNotificationCancel();
			}
		break;
	case ECancelWarnQoS:
		if(iState == EWarnQoSState)
			{
//			if (iQoSWarning)
//				iQoSWarning->DoCancel();
			iBusy=EFalse;
			}
		break;
	default:
		break;
		}
	}

void CNetDialDialogSession::CancelEverything()
	{	
	switch (iState)
		{
	case EGetIAP:
		Cancel(ECancelGetIAP);
		break;
	case EGetModemAndLocation:
		Cancel(ECancelGetModemAndLocation);
		break;
	case EWarnIAP:
		Cancel(ECancelWarnIAP);
		break;
	case ELogin:
		Cancel(ECancelLogin);
		break;
	case EAuthentication:
		Cancel(ECancelAuthenticate);
		break;
	case EReconnectReq:
		Cancel(ECancelReconnect);
		break;
	case EPct:
		Cancel(ECancelReadPct);
		Cancel(ECancelDestroyPctNotification);
		break;
	case EWarnQoSState:
		Cancel(ECancelWarnQoS);
		break;
	default:
		break;
		}
	}

void CNetDialDialogSession::PanicClient(TInt aPanic) const
//
// Panic the client
//
	{
	_LIT(KPanicClientText,"AgentDialog Client");
	iMessage.Panic(KPanicClientText,aPanic);
	}

//
// Global functions
//

GLDEF_C TInt CNetDialDialogServer::ThreadFunction(TAny* /*aStarted*/)
	{
	RSemaphore s;
	TInt semRet=s.OpenGlobal(KNDDialogSemaphoreName);
	if (semRet!=KErrNone)
		{
		s.Signal();
		s.Close();
		PanicServer(ESvrCreateServer);
		}

	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();

// Keep the DLL Load reference counter above zero when the app ends
	RLibrary library;
	TInt ret=library.Load(KDialogServerFileName);				// Library is automatically closed when thread exits
	if(ret!=KErrNone)
		{
		s.Signal();
		s.Close();
		PanicServer(ESvrDllFileNameNotFound);
		}

	CActiveScheduler* pA=new CActiveScheduler;
	__ASSERT_ALWAYS(pA,PanicServer(ESvrMainSchedulerError));
	CActiveScheduler::Install(pA);

	CNetDialDialogServer* dlgServer=NULL;
	TRAP(ret,(dlgServer=CNetDialDialogServer::NewL()));
	
	s.Signal();
	s.Close();
	
	if (ret!=KErrNone)
		PanicServer(ESvrCreateServer);

	CActiveScheduler::Start();

// finished ... so, destroy clean-up stack
	delete dlgServer;
	delete pA;
	delete cleanup;
	__UHEAP_MARKEND;

	return KErrNone;
	}


GLDEF_C void PanicServer(TAgentDialogPanic aPanic)
//
// Panic the server
//
	{
	_LIT(KPanicServerText,"ND_DLSV Server");
	User::Panic(KPanicServerText,aPanic);
	}

EXPORT_C TInt StartDialogThread()
//
// Attempt to start a thread for the socket server in the C32 process.
//
    {
#if defined (__EPOC32__)
	TFindServer findDL(DIALOG_SERVER_NAME);
	TFullName name;
	if (findDL.Next(name)==KErrNone)
		return KErrAlreadyExists;
#endif

// Set up a mutex to stop thread collision
	RMutex mutex;
	TInt ret=KErrNone;
    if(mutex.CreateGlobal(KNDDialogMutexName) != KErrNone)
		ret=mutex.OpenGlobal(KNDDialogMutexName);
	
// Set up semaphore to signal that the new thread has started
	RSemaphore s;
	if(ret==KErrNone && s.CreateGlobal(KNDDialogSemaphoreName,0)!=KErrNone)
		ret=s.OpenGlobal(KNDDialogSemaphoreName);
	
	if(ret==KErrNone)
		{
		mutex.Wait(); // the exclusion ensures the socket server is started atomically
		RThread t;
		// Create dialog server thread in current process
		// This assumes that dialog server is always started from within the C32 process.
		ret=t.Create(DIALOG_SERVER_NAME,CNetDialDialogServer::ThreadFunction,KDialogStackSize,KDialogMinHeapSize,KDialogMaxHeapSize,NULL);
		if (ret==KErrNone)
			{
			t.Resume();
			s.Wait();		    
			}
		else if(ret==KErrAlreadyExists) // Created by someone else or shutting down ???
			ret=KErrNone;
		User::SetCritical(User::ENotCritical);
		t.Close();
		mutex.Signal();
		}

    s.Close();
    mutex.Close();

    return ret;
    }

//===================================================================
// Additional test code
CDialogObserverReq::CDialogObserverReq(const RMessage2& aMessage) : iOutstandingReq(aMessage)
	{
	}
CDialogObserverReq::~CDialogObserverReq()
	{
	}

void CDialogObserverReq::Complete()
	{
	iOutstandingReq.Complete(KErrNone);
	}

void CDialogObserverReq::Cancel()
	{
	iOutstandingReq.Complete(KErrCancel);
	}
