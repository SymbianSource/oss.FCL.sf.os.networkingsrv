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

#include <comms-infras/nifif.h>
#include <nifvar.h>
#include <nifutl.h>
#include "tnifprog.h"

class CSimpleIfFactory : public CNifIfFactory
	{
protected:
	virtual void InstallL();
	virtual CNifIfBase* NewInterfaceL(const TDesC& aName);
	virtual TInt Info(TNifIfInfo& aInfo, TInt aIndex) const;
	};

class CSimpleIf;
class CSimpleIfLink : public CNifIfLink, public MTimer
	{
public:
	CSimpleIfLink(CNifIfFactory& aFactory, TBool aMulti);
	~CSimpleIfLink();
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual TInt Start();
	virtual void Stop(TInt aReason, MNifIfNotify::TAction aAction);		
   // virtual void Stop(TInt aReason);
	virtual void AuthenticateComplete(TInt aResult);
	virtual void BindL(TAny *aId);
    virtual CNifIfBase* GetBinderL(const TDesC& aName);
	virtual void TimerComplete(TInt aStatus);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	virtual void Restart(CNifIfBase* aIf);

	static void FillInInfo(TNifIfInfo& aInfo, TBool aMulti);
public:
	TBool iMulti;
	CProtocolBase* iProtocol;
	CSimpleIf* iLastCreated;
	TUint32 iTestno;
	TBool iRestartPending;
	};

class CSimpleIf : public CNifIfBase
	{

public:
	CSimpleIf(CSimpleIfLink& aLink);
    virtual void BindL(TAny *aId);
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	void Recv(RMBufChain& aPdu);
	static void FillInInfo(TNifIfInfo& aInfo);

	CProtocolBase* iProtocol;
	CSimpleIfLink* iLink;
	};

extern "C"
    {
    IMPORT_C CNifFactory * NewInterfaceFactoryL(void);	// Force export 

	EXPORT_C CNifFactory* NewInterfaceFactoryL(void)
		{
		return new (ELeave) CSimpleIfFactory;
		}
	}

void CSimpleIfFactory::InstallL()
	{
	}


CNifIfBase* CSimpleIfFactory::NewInterfaceL(const TDesC& aName)
	{

	CSimpleIfLink* s=0;

	if(!aName.CompareF(_L("testsglif")))
	    s = new (ELeave) CSimpleIfLink(*this, EFalse);
	else if(aName.CompareF(_L("testmulif")))
		User::Leave(KErrNotFound);
	else
        s = new (ELeave) CSimpleIfLink(*this, ETrue);
	CleanupStack::PushL(s);
	s->TimerConstructL(ESocketTimerPriority);
	CleanupStack::Pop();
	return s;
	}

TInt CSimpleIfFactory::Info(TNifIfInfo& aInfo, TInt aIndex) const
	{
	
	switch(aIndex)
		{
	case 0:	CSimpleIfLink::FillInInfo(aInfo, EFalse); break;
	case 1:	CSimpleIfLink::FillInInfo(aInfo, ETrue); break;
	default: CSimpleIf::FillInInfo(aInfo);
		}
	return 3;
	}

CSimpleIfLink::CSimpleIfLink(CNifIfFactory& aFactory, TBool aMulti)
	: CNifIfLink(aFactory)
	{

	iMulti=aMulti;
	}

CSimpleIfLink::~CSimpleIfLink()
	{

	TimerDelete();
	}

void CSimpleIfLink::Info(TNifIfInfo& aInfo) const
	{
	FillInInfo(aInfo, iMulti);
	}

void CSimpleIfLink::FillInInfo(TNifIfInfo& aInfo, TBool aMulti)
	{
	aInfo.iProtocolSupported=0;
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfIsLink | KNifIfUsesNotify | KNifIfCreatedByFactory;
	if(aMulti)
		{
	    aInfo.iName = _L("testmulif");
		aInfo.iFlags |= KNifIfCreatesBinder;
		}
	else
		aInfo.iName = _L("testsglif");
	}

TInt CSimpleIfLink::Send(RMBufChain& aPdu, TAny* aSource)
	{

	if(iMulti)
	    ((CSimpleIf*)aSource)->Recv(aPdu);
	else
	    iProtocol->Process(aPdu);
	return 1;
	}

TInt CSimpleIfLink::Start()
	{
	
	if(iNotify->ReadInt(_L("testno"), iTestno)  != KErrNone)
		User::Panic(_L("CSimpleIfLink"), 1);

	if(iTestno==12)
		{
		iNotify->IfProgress(ETNifmanProg3, KErrLocked);
		return KErrLocked;
		}
	else if(iTestno>=13 && iTestno<20)
		{
		TimerAfter(500000);
		iNotify->Notification(ENifToAgentEventTypePPPCallbackGranted);
		}
	else if(iTestno==8)
		{
		iNotify->NegotiationFailed(iLastCreated, KErrNotSupported);
		}
	else if(iTestno==20)
		{
		TimerAfter(500000);
		}

	iNotify->LinkLayerUp();
	iNotify->IfProgress(KLinkLayerOpen, KErrNone);

	return KErrNone;
	}

void CSimpleIfLink::TimerComplete(TInt)
	{
	
	if(iTestno==13 || iRestartPending)
		{
	    iNotify->IfProgress(ETNifmanProg3, KErrTimedOut);
	    iNotify->LinkLayerDown(KErrTimedOut, MNifIfNotify::EDisconnect);
		}
	else if(iTestno==16)
		{
		TName username, password;
		iNotify->Authenticate(username, password);
		}
	else if(iTestno==14 || iTestno==15)
		{
		iNotify->IfProgress(ETNifmanProg4, KErrCommsLineFail);
		iRestartPending=ETrue;
		iNotify->LinkLayerDown(KErrCommsLineFail, MNifIfNotify::EReconnect);
		}
	else if(iTestno==20)
		{
		iNotify->IfProgress(ETNifmanProg7, KErrCommsLineFail);
		iNotify->BinderLayerDown(iLastCreated, KErrCommsLineFail, MNifIfNotify::EReconnect);
		}
	}

void CSimpleIfLink::AuthenticateComplete(TInt aResult)
	{
	iNotify->IfProgress(ETNifmanProg5, aResult);
	}

//void CSimpleIfLink::Stop(TInt aError)
void CSimpleIfLink::Stop(TInt aError, MNifIfNotify::TAction aAction)		
	{
	
	// Disconections unbinding has been done by now
	iNotify->IfProgress(ETNifmanProg3, aError);
	iNotify->LinkLayerDown(aError, aAction);
	}

void CSimpleIfLink::BindL(TAny *aId)
	{
	if(iProtocol)
		User::Leave(KErrAlreadyExists);
	iProtocol = (CProtocolBase*)aId;	
	}

CNifIfBase* CSimpleIfLink::GetBinderL(const TDesC&)
	{

	if(iMulti)
		{
	    iLastCreated = new (ELeave) CSimpleIf(*this);
		return iLastCreated;
		}
	
	return 0;
	}

TInt CSimpleIfLink::Notification(TAgentToNifEventType aEvent,void* aInfo)
	{

	if (aEvent!=EAgentToNifEventTypeDisableTimers)
		return KErrUnknown;
	if (aInfo!=NULL)
		return KErrUnknown;

	return KErrNone;
	}

void CSimpleIfLink::Restart(CNifIfBase* aIf)
	{
	
	if(iTestno==20 && aIf==iLastCreated)
		{
		iNotify->IfProgress(ETNifmanProg8, KErrNone);
		}
	}

CSimpleIf::CSimpleIf(CSimpleIfLink& aLink)
	: CNifIfBase(aLink)
	{
	iLink = &aLink;	
	}

void CSimpleIf::BindL(TAny *aId)
	{
	if(iProtocol)
		User::Leave(KErrAlreadyExists);
	iProtocol = (CProtocolBase*)aId;	
	}

TInt CSimpleIf::Send(RMBufChain& aPdu, TAny*)
	{
	
	return iLink->Send(aPdu, this);
	}

void CSimpleIf::Recv(RMBufChain& aPdu)
	{
	
	iProtocol->Process(aPdu, (CProtocolBase*)this);
	}

void CSimpleIf::Info(TNifIfInfo& aInfo) const
	{
	FillInInfo(aInfo);
	}

void CSimpleIf::FillInInfo(TNifIfInfo& aInfo)
	{
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
	aInfo.iName = _L("simpleif");
	aInfo.iProtocolSupported=0;
	}

TInt CSimpleIf::Notification(TAgentToNifEventType /*aEvent*/, void * /*aInfo*/)
	{

	return KErrNone;
	}





