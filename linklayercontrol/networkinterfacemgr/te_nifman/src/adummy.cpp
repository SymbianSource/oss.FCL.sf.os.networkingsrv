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
// Dummy nifman agent
//
//

#include <comms-infras/nifagt.h>
#include <cdbcols.h>
#include <nifutl.h>
#include "tnifprog.h"
#include <test/es_dummy.h>

class CSimpleAgentFactory : public CNifAgentFactory
	{
protected:
	virtual void InstallL();
	virtual CNifAgentBase *NewAgentL(const TDesC& aName);
	virtual TInt Info(TNifAgentInfo& aInfo, TInt aIndex) const;
	};

class CSimpleAgent : public CNifAgentBase, public MTimer
	{

public:
	CSimpleAgent();
	~CSimpleAgent();
	void ConstructL();

	virtual TInt Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption);
	virtual void TimerComplete(TInt aStatus);
	virtual void Connect(TAgentConnectType aType);
	virtual void Connect(TAgentConnectType aType, CStoreableOverrideSettings* aOverrideSettings);
	virtual void CancelConnect();
	virtual void Reconnect();
	virtual void CancelReconnect();
	virtual void Authenticate(TDes& aUsername, TDes& aPassword);
	virtual void CancelAuthenticate();
	virtual void Disconnect(TInt aReason);
	virtual TInt GetExcessData(TDes8& aBuffer);
	virtual TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo);
	virtual TInt IncomingConnectionReceived();
	virtual void GetLastError(TInt& aError);
	virtual TBool IsActive() const;
	virtual TBool IsReconnect() const;
	virtual void SetConnectionSettingsL(const TConnectionSettings& aSettings);
	virtual TConnectionSettings& ConnectionSettingsL();
	virtual void SetOverridesL(CStoreableOverrideSettings* aOverrideSettings);
	virtual CStoreableOverrideSettings* OverridesL();
	virtual void RequestNotificationOfServiceChangeL(MAgentSessionNotify* aSession);
	virtual void CancelRequestNotificationOfServiceChange(MAgentSessionNotify* aSession);

	virtual void Info(TNifAgentInfo& aInfo) const;

	static void FillInInfo(TNifAgentInfo& aInfo);


protected:
	virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage);
	virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage);
	virtual HBufC* DoReadLongDesLC(const TDesC& aField,const RMessagePtr2* aMessage);


public:
	enum TState
		{
		EIdle,
		EConnecting,
		EAwaitingDisconnect,
		EDisconnecting
		};
	TUint32 iTestNo;
	TState iState;
	TAgentConnectType iConType;
	TConnectionSettings iSettings;
	};


extern "C"
    {
    IMPORT_C CNifFactory * NewAgentFactoryL(void);	// Force export

	EXPORT_C CNifFactory* NewAgentFactoryL(void)
		{
		return new (ELeave) CSimpleAgentFactory;
		}
	}

CNifAgentBase *CSimpleAgentFactory::NewAgentL(const TDesC& aName)
	{
	(void)aName;
	CSimpleAgent* p = new (ELeave) CSimpleAgent;
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop();
	return p;
	}

void CSimpleAgentFactory::InstallL()
	{ }

TInt CSimpleAgentFactory::Info(TNifAgentInfo& aInfo, TInt) const
	{
	CSimpleAgent::FillInInfo(aInfo);
	return 1;
	}

CSimpleAgent::CSimpleAgent()
	{ }

CSimpleAgent::~CSimpleAgent()
	{

	TimerDelete();
	}

void CSimpleAgent::Info(TNifAgentInfo& aInfo) const
    {
	FillInInfo(aInfo);

	// ensure that the name of this agent is unique
	aInfo.iName.AppendFormat(_L("[%x]"), this);
	}

void CSimpleAgent::FillInInfo(TNifAgentInfo& aInfo)
	{
	aInfo.iName = _L("testagent");
	aInfo.iVersion = TVersion(1,1,1);
	}

void CSimpleAgent::ConstructL()
	{

	TimerConstructL(ESocketTimerPriority);
	}

TInt CSimpleAgent::Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption)
	{

	if(aOptionLevel!=KCOLAgent)
		return KErrNotSupported;

	switch(aOptionName)
		{
		case KADummySetInitialValue:
			iTestNo = *(REINTERPRET_CAST(const TInt*, aOption.Ptr()));
			return KErrNone;

		case KADummyIssueStop:
			switch (*(REINTERPRET_CAST(const TInt*, aOption.Ptr())))
				{
				case 2:		// not a stop as such - borrowing KADummyIssueStop
					iNotify->AgentEvent(EEtelEvent, ECurrentNetworkChangeEvent, KNullDesC8, NULL);
					break;
				case 1:
					{
					TInt stopCode = KErrEof;
					TPckgC<TInt> stopCodePkg(stopCode);
					iNotify->AgentEvent(EAgentOriginatedConnectionCommand, EAgentConnectionCommandStop, stopCodePkg, NULL);
					return KErrNone;
					break;
					}
				case 0:
					return iNotify->Notification(EAgentToNifEventTypeDisableConnection, NULL);
					break;
				}
		default:
			return KErrNotSupported;
		}
	}

void CSimpleAgent::Connect(TAgentConnectType aType, CStoreableOverrideSettings*)
	{
	iConType=aType;
	iState=EConnecting;
	iNotify->AgentProgress(ETNifmanProg2, KErrNone);
	TimerAfter(500000);
	}

void CSimpleAgent::Connect(TAgentConnectType aConType)
	{
	Connect(aConType, NULL);
	}

void CSimpleAgent::CancelConnect()
	{
	iState = EAwaitingDisconnect;
	TimerCancel();
	}

void CSimpleAgent::TimerComplete(TInt)
	{

	switch(iState)
		{
		case EConnecting:
			{
			if(iTestNo==10)
				{
				iState = EAwaitingDisconnect;
				iNotify->AgentProgress(ETNifmanProg6, KErrCouldNotConnect);
				iNotify->ConnectComplete(KErrCouldNotConnect);
				return;
				}

			if(iConType == EAgentStartDialOut)
				iNotify->ServiceStarted();
			if(iState==EConnecting)
				{
				iState = EAwaitingDisconnect;

				if(iTestNo==11)
					{
					iNotify->AgentProgress(ETNifmanProg6, KErrDisconnected);
					iNotify->ConnectComplete(KErrDisconnected);
					}
				else
					{
					iNotify->AgentProgress(ETNifmanProg6, KErrNone);
					iNotify->ConnectComplete(KErrNone);
					}
				}
			break;
			}

		case EDisconnecting:
			iState=EIdle;
			iNotify->DisconnectComplete();
			break;

		default:
			User::Panic(_L("CSimpleAgent"), 2);
		}
	}

TInt CSimpleAgent::Notification(TNifToAgentEventType aEvent, TAny* aInfo)
	{

	if (aEvent!=ENifToAgentEventTypePPPCallbackGranted)
		return KErrUnknown;
	if (aInfo!=NULL)
		return KErrUnknown;

	return KErrNone;
	}

void CSimpleAgent::Disconnect(TInt)
	{

	__ASSERT_DEBUG(iState==EAwaitingDisconnect, User::Panic(_L("CSimpleAgent"), 0));
	iState=EDisconnecting;

	TimerAfter(500000);
	}

void CSimpleAgent::Reconnect()
	{

	if(iTestNo == 15)
	    iNotify->ReconnectComplete(KErrNone);
	else // 14
		iNotify->ReconnectComplete(KErrBadName);
	}

void CSimpleAgent::CancelReconnect()
	{
	}

void CSimpleAgent::Authenticate(TDes&, TDes&)
	{

	iNotify->AuthenticateComplete(KErrNone);
	}

void CSimpleAgent::CancelAuthenticate()
	{
	}

TInt CSimpleAgent::GetExcessData(TDes8&)
	{
	return KErrNotSupported;
	}

TInt CSimpleAgent::DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2*)
	{

	if(!aField.CompareF(_L("TestNo")))
		{
		aValue = iTestNo;
		return KErrNone;
		}
	else if(!aField.CompareF(_L("ModemBearer\\LastSocketClosedTimeout")))
		{
		switch (iTestNo)
			{
		case 17:
		case 19: aValue = 2; break;
		default: aValue = KMaxTUint32;
			}
		return KErrNone;
		}
	else if(!aField.CompareF(_L("ModemBearer\\LastSessionClosedTimeout")))
		{
		switch (iTestNo)
			{
		case 17:
		case 19: aValue = 2; break;
		default: aValue = KMaxTUint32;
			}
		return KErrNone;
		}
	else if(!aField.CompareF(_L("ModemBearer\\LastSocketActivityTimeout")))
		{
		switch (iTestNo)
			{
		case 18:
		case 19: aValue = 4; break;
		default: aValue = KMaxTUint32;
			}
		return KErrNone;
		}
	return KErrNotSupported;
	}

TInt CSimpleAgent::DoWriteInt(const TDesC&, TUint32,const RMessagePtr2*)
	{
	return KErrNotSupported;
	}

TInt CSimpleAgent::DoWriteBool(const TDesC&, TBool,const RMessagePtr2*)
	{
	return KErrNotSupported;
	}

TInt CSimpleAgent::DoReadBool(const TDesC&, TBool&,const RMessagePtr2*)
	{
	return KErrNotSupported;
	}


TInt CSimpleAgent::DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2*)
	{

	if(!aField.CompareF(TPtrC(ISP_IF_NETWORKS)))
		{
		switch (iTestNo)
			{
        case 7:  aValue = _L8("Bogus Protocol"); break;
		case 4:
		case 8:
		case 3:
			{
			TBuf<200> wideBuf;
			wideBuf = KDummyFourName;
			wideBuf.Append(_L(","));
			wideBuf.Append(KDummyThreeName);
			aValue.Copy(wideBuf);
			break;
			}
		case 2:  aValue.Copy(KDummyFourName); break;
		case 5:  aValue.Copy(KDummyOneName); break;
		default: aValue.Copy(KDummyThreeName);
			}
		}
	else if(!aField.CompareF(TPtrC(IF_NAME)))
		{
		switch (iTestNo)
			{
		case 6: aValue = _L8("bogusif");    break;
		case 4:
		case 8:
		case 20:
		case 2: aValue.Copy(KDummyMulIfName);   break;
		default: aValue.Copy(KDummySglIfName); break;
			}
		}
	else if (!aField.CompareF(TPtrC(SERVICE_CONFIG_DAEMON_MANAGER_NAME)))
		{
		return KErrNotFound;
		}
	else if (!aField.CompareF(TPtrC(SERVICE_CONFIG_DAEMON_NAME)))
		{
		return KErrNotFound;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

TInt CSimpleAgent::DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2*)
	{

	if(!aField.CompareF(TPtrC(ISP_IF_NETWORKS)))
		{
		switch (iTestNo)
			{
        case 7:  aValue = _L16("Bogus Protocol"); break;
		case 4:
		case 8:
		case 3:
			{
			aValue = KDummyFourName;
			aValue.Append(_L(","));
			aValue.Append(KDummyThreeName);
			break;
			}
		case 2:  aValue = KDummyFourName; break;
		case 5:  aValue = KDummyOneName; break;
		default: aValue = KDummyThreeName;
			}
		}
	else if(!aField.CompareF(TPtrC(IF_NAME)))
		{
		switch (iTestNo)
			{
		case 6: aValue = _L16("bogusif");    break;
		case 4:
		case 8:
		case 20:
		case 2: aValue = KDummyMulIfName;   break;
		default: aValue = KDummySglIfName; break;
			}
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}


TInt CSimpleAgent::DoWriteDes(const TDesC&, const TDesC8&,const RMessagePtr2*)
	{

	return KErrNotSupported;
	}

TInt CSimpleAgent::DoWriteDes(const TDesC&, const TDesC16&,const RMessagePtr2*)
	{

	return KErrNotSupported;
	}

TInt CSimpleAgent::IncomingConnectionReceived()
	{

	return iNotify->IncomingConnectionReceived();
	}

void CSimpleAgent::GetLastError(TInt& aError)
	{

	aError=KErrNone;
	}

TBool CSimpleAgent::IsActive() const
	{
	return ETrue;
	}

TBool CSimpleAgent::IsReconnect() const
	{
	return EFalse;
	}

void CSimpleAgent::SetConnectionSettingsL(const TConnectionSettings& aSettings)
	{
	iSettings = aSettings;
	}

TConnectionSettings& CSimpleAgent::ConnectionSettingsL()
	{
	return iSettings;
	}

void CSimpleAgent::SetOverridesL(CStoreableOverrideSettings*)
	{ }

CStoreableOverrideSettings* CSimpleAgent::OverridesL()
	{
	User::Leave(KErrNotSupported);
	return NULL;
	}

void CSimpleAgent::RequestNotificationOfServiceChangeL(MAgentSessionNotify*)
	{
	User::Leave(KErrNotSupported);
	}

void CSimpleAgent::CancelRequestNotificationOfServiceChange(MAgentSessionNotify*)
	{ }

HBufC* CSimpleAgent::DoReadLongDesLC(const TDesC&,const RMessagePtr2*)
	{
	User::Leave(KErrNotSupported);
	return NULL;
	}

