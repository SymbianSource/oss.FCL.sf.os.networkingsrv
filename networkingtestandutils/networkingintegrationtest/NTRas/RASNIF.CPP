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

#include "rasdef.h"
//#include "in_ver.h" Changed for dual stack 

TInt CNtRasFactory::Info(TNifAgentInfo& aInfo, TInt) const
	{
	
	CNtRas::FillInInfo(aInfo);
	return 1;
	}

void CNtRasFactory::InstallL()
	{

	}

CNifAgentBase *CNtRasFactory::NewAgentL(const TDesC& aName)
	{

	if(aName.CompareF(RAS_NIF_NAME))
		User::Leave(KErrBadName);

	CNtRas* ras = new (ELeave) CNtRas;
	CleanupStack::PushL(ras);
	ras->ConstructL();
	CleanupStack::Pop();
	return ras;
	}

CNtRas::CNtRas()
	{

	__DECLARE_NAME(_S("CNtRas"));
	}

CNtRas::~CNtRas()
	{

	__ASSERT_DEBUG(!iDatabase, Panic(ENtRasPanic_ObjectNotDeleted));
	__ASSERT_DEBUG(!iLoginBox, Panic(ENtRasPanic_ObjectNotDeleted));
	__ASSERT_DEBUG(!iConnectBox, Panic(ENtRasPanic_ObjectNotDeleted));
	__ASSERT_DEBUG(!iConnector, Panic(ENtRasPanic_ObjectNotDeleted));
	delete iAsyCb;
	delete iExcessData;
	}

void CNtRas::ConstructL()
	{
	
	TCallBack callback = TCallBack(AsyCbHandler, this);
	iAsyCb = new (ELeave) CAsyncCallBack(callback, KNtRasAsyncCbPriority);

	iDatabase = CRasDatabase::NewL(this);
	}

void CNtRas::Info(TNifAgentInfo& aInfo) const
	{
	
	FillInInfo(aInfo);

	// ensure that the name of this agent is unique
	aInfo.iName.AppendFormat(_L("[%x]"), this);
	}

void CNtRas::FillInInfo(TNifAgentInfo& aInfo)
	{
	
	aInfo.iName=RAS_NIF_NAME;
	}

void CNtRas::Connect(TAgentConnectType aType)
	{
	
	if(aType==EAgentStartDialOut)
	    Process(ERasOpStart);
	else
		Process(ERasOpRestart);
	}

void CNtRas::Connect(TAgentConnectType aType, CStoreableOverrideSettings* /*aOverrideSettings*/)
	{
	
	if(aType==EAgentStartDialOut)
	    Process(ERasOpStart);
	else
		Process(ERasOpRestart);
	}


void CNtRas::CancelConnect()
//
// Cancel all oustanding active calls by deleting objects
// Note a call to disconnect is to follow
//
	{

	iAsyCb->Cancel();
	delete iConnectBox;
	iConnectBox=0;

	// Let following Disconnect call sort everything else out
	}

void CNtRas::Reconnect()
	{
	Process(ERasOpReconnect);
	}

void CNtRas::CancelReconnect()
	{
	CancelConnect();
	}

void CNtRas::Disconnect(TInt /* aReason */)
	{

	Process(ERasOpDisconnect);
	}

void CNtRas::Authenticate(TDes& aUsername, TDes& aPassword)
	{

	__ASSERT_DEBUG(iState==ERasOpen, Panic(ENtRasPanic_ObjectNotDeleted));

	iUserNamePtr = &aUsername;
	iPasswordPtr = &aPassword;
	Process(ERasOpAuthenticate);
	}

void CNtRas::CancelAuthenticate()
	{

	iAsyCb->Cancel();
	delete iLoginBox;
	iLoginBox=0;
	iState = ERasOpen;
	}

void CNtRas::AuthenticateDialogComplete(TInt aStatus)
//
// Upcall from Login dialog
//
	{

	__ASSERT_DEBUG(iState==ERasAuthenticate, Panic(ENtRasPanic_ObjectNotDeleted)); 
	
	delete iLoginBox;
	iLoginBox=0;
	iState = ERasOpen;
	iNotify->AuthenticateComplete(aStatus);
	}

TInt CNtRas::GetExcessData(TDes8& aBuffer)
	{

	if(iState!=ERasOpen && iState != ERasAuthenticate)
	    return KErrNotReady;
	
	if(iExcessData)
	    aBuffer.Copy(*iExcessData);
	else
		aBuffer.SetLength(0);
	return KErrNone;
	}

TInt CNtRas::DoReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* /*aMessage*/ )
	{
	if(!iDatabase)
		return KErrNotReady;
	return iDatabase->ReadField(aField, aValue);
	}

TInt CNtRas::DoReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* /*aMessage*/ )
	{
#if !defined(UNICODE)
	if(!iDatabase)
		return KErrNotReady;
	return iDatabase->ReadField(aField, aValue);
#else
	// Remove warnings 
	aValue = aValue;
	const TUint16*  aTemp;
	aTemp = aField.Ptr(); 
	return KErrNotSupported;
#endif
	}

TInt CNtRas::DoReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* /*aMessage*/ )
	{
#if defined(UNICODE)
	if(!iDatabase)
		return KErrNotReady;

	if (aValue.MaxLength() > 256)
		return KErrTooBig;

	TBuf8<256> TempBuffer;
	TInt	RetCode;
	
	RetCode = iDatabase->ReadField(aField, TempBuffer);

	if ( RetCode == KErrNone )
		{
		TInt i;
		TUint8* source;
		TUint16* dest;
		TInt len;

		len = aValue.MaxLength();

		source = CONST_CAST(TUint8*,TempBuffer.Ptr());
		dest = CONST_CAST(TUint16*, aValue.Ptr());

		for (i=0;i<len;i++)
			{
			*(dest+i) = (TUint16)(*(source+i)&0xFF);
			}
		aValue.SetLength(TempBuffer.Length());
		}
	return RetCode;
#else
	// Remove warnings 
	aValue = aValue;
	const TUint8*  aTemp;
	aTemp = aField.Ptr(); 
	return KErrNotSupported;
#endif
	}

TInt CNtRas::DoReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* /*aMessage*/ )
	{
	TUint32	TempValue;
	TInt	RetCode;

	if(!iDatabase)
		return KErrNotReady;

	RetCode = iDatabase->ReadField(aField, TempValue);

	if (RetCode == KErrNone)
		{
		if (TempValue)
			{
			aValue = ETrue;
			}
		else
			{
			aValue = EFalse;
			}
		}

	return RetCode;
	}

TInt CNtRas::DoWriteInt(const TDesC& /*aField */, TUint32 /* aValue */, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNotSupported;
	}

TInt CNtRas::DoWriteDes(const TDesC& /*aField*/, const TDesC8& /*aValue*/, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNotSupported;
	}

TInt CNtRas::DoWriteDes(const TDesC& /*aField*/, const TDesC16& /*aValue*/, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNotSupported;
	}

TInt CNtRas::DoWriteBool(const TDesC& /*aField */, TBool /* aValue */, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNotSupported;
	}

TInt CNtRas::Notification(TNifToAgentEventType aEvent, TAny* /* aInfo */)
	{
	switch (aEvent)
		{
		case ENifToAgentEventTypeQueryIsDialIn:
			{
			return KErrNotSupported;
			}
		case ENifToAgentEventTypePPPCallbackGranted:
			{
			return KErrNotSupported;
			}
		default:
			{
			return KErrNotSupported;
			}
		}

	}

TInt CNtRas::IncomingConnectionReceived()
	{
	return KErrNotSupported;
	}

void CNtRas::GetLastError(TInt& aError)
	{
	aError=KErrNone;
	}

void CNtRas::ConnectDialogComplete(TInt aStatus)
//
// Upcall from Connect dialog
//
	{

	__ASSERT_DEBUG(iState==ERasSelect||iState==ERasReconnect, Panic(ENtRasPanic_BadState));

	// Cleanup connect dialog
	delete iConnectBox;
	iConnectBox=0;

	TInt res = aStatus;
	iNotify->AgentProgress(ERasFinishedSelection, res);

	if(iState==ERasReconnect)
		{
		iNotify->ReconnectComplete(res);
		return;
		}

	if(res==KErrNone)
		{
        iNotify->ServiceStarted();
		if(iOp==ERasOpDisconnect) // ServiceStarted Failed
			return;
		// Start the connector next
		iState=ERasConnect;
		TRAP(res, iConnector=CRasConnect::NewL(this);)
		if(res!=KErrNone)
			iNotify->AgentProgress(ERasConnectionOpen, res);
		}
	
	if(res!=KErrNone)
	    iNotify->ConnectComplete(res); // Disconnect Next
	}

void CNtRas::ConnectComplete(TInt aStatus)
	{

	__ASSERT_DEBUG(iState==ERasConnect, Panic(ENtRasPanic_BadState));
	iNotify->AgentProgress(ERasConnectionOpen, aStatus);
	if(aStatus==KErrNone)
		{
		delete iExcessData;
		if(iConnector->ExcessData().Length())
		    iExcessData=iConnector->ExcessData().Alloc();
		}
	
	if(aStatus==KErrNone)
		iState=ERasOpen;
	iNotify->ConnectComplete(aStatus);
	}

void CNtRas::Process(TRasOp aOp)
//
// Advance to next state and call schedule DoProcess()
//
	{

	iOp = aOp;
	iAsyCb->Call();
	}

TInt CNtRas::AsyCbHandler(TAny* aPtr)
//
// Async CallBack handler - async state advances
//
	{
	((CNtRas*)aPtr)->DoProcess();
	return 0;
	}

void CNtRas::DoProcess()
//
// Process a request from above -> either Connect, Authenticate or Disconnect request
//
	{
	
	TInt res;
	// Do whatever is necessary for the op
	switch (iOp)
		{
	case ERasOpStart: 
		__ASSERT_DEBUG(iState==ERasClosed, Panic(ENtRasPanic_BadState));
		iState=ERasSelect;
		iNotify->AgentProgress(ERasStartingSelection, KErrNone);
	
		TRAP(res, iConnectBox = CConnectBox::NewL(this, EFalse);)
		if(res!=KErrNone)
			ConnectDialogComplete(res);
		break;

	case ERasOpReconnect:
		__ASSERT_DEBUG(iState==ERasOpen, Panic(ENtRasPanic_BadState));
		iNotify->AgentProgress(ERasStartingSelection, KErrNone);
		delete iConnector; // drop the lines
		iConnector=0;
		iState=ERasReconnect;
		TRAP(res, iConnectBox = CConnectBox::NewL(this, ETrue);)
		if(res!=KErrNone)
			ConnectDialogComplete(res);
		break;

	case ERasOpRestart:
		__ASSERT_DEBUG(iState==ERasReconnect, Panic(ENtRasPanic_BadState));
		iState=ERasConnect;
		TRAP(res, iConnector=CRasConnect::NewL(this);)
		if(res!=KErrNone)
			{
			iNotify->AgentProgress(ERasConnectionOpen, res);
			iNotify->ConnectComplete(res);
			}
		break;

	case ERasOpDisconnect:
		__ASSERT_DEBUG(!iConnectBox, Panic(ENtRasPanic_ObjectNotDeleted));
		__ASSERT_DEBUG(!iLoginBox, Panic(ENtRasPanic_ObjectNotDeleted));

		iNotify->AgentProgress(ERasStartingHangUp, KErrNone);
		if(iDatabase)
		    iNotify->ServiceClosed();
		delete iExcessData;
		iExcessData=0;
		delete iDatabase;
		iDatabase=0;
		delete iConnector;
		iConnector=0;
		iState=ERasClosed;
		iNotify->AgentProgress(ERasFinishedHangUp, KErrNone);
		iNotify->AgentProgress(ERasNoConnection, KErrNone);
		iNotify->DisconnectComplete();
		break;

	case ERasOpAuthenticate:
		__ASSERT_DEBUG(!iLoginBox,  Panic(ENtRasPanic_ObjectNotDeleted));
		__ASSERT_DEBUG(iState==ERasOpen, Panic(ENtRasPanic_BadState));
		
		iState=ERasAuthenticate;
	
		TRAP(res, iLoginBox = CLoginBox::NewL(this, *iUserNamePtr, *iPasswordPtr);)
		if (res!=KErrNone)
			AuthenticateDialogComplete(res);

		break;
		}		
	}

TBool CNtRas::IsActive() const
	{
	
	return ETrue;
	}

TBool CNtRas::IsReconnect() const
	{

	return EFalse;
	}

void CNtRas::SetConnectionSettingsL(const TConnectionSettings& aSettings)
	{

	iSettings = aSettings;
	}

TConnectionSettings& CNtRas::ConnectionSettingsL()
	{

	return iSettings;
	}

void CNtRas::SetOverridesL(CStoreableOverrideSettings*)
	{
	}

CStoreableOverrideSettings* CNtRas::OverridesL()
	{

	User::Leave(KErrNotSupported);
	return NULL;
	}

void CNtRas::RequestNotificationOfServiceChangeL(MAgentSessionNotify*)
	{

	User::Leave(KErrNotSupported);
	}

void CNtRas::CancelRequestNotificationOfServiceChange(MAgentSessionNotify*)
	{
	}

HBufC* CNtRas::DoReadLongDesLC(const TDesC&, const RMessagePtr2* /*aMessage*/ )
	{
	User::Leave(KErrNotSupported);
	return NULL;
	}
