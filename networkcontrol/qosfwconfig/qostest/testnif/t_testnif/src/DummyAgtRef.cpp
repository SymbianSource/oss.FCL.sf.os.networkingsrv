// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// The objects of the class  do all work to start & finish PPP session
// 
//

#include "common.h"
#include "DummyAgtRef.h"
#include "NIFMAN.H"
#include "cdbcols.h"
#include "TestMgr.h"
#include "dummyprotocol.h"
#include "ss_pman.h"
#include "Tlog.h"



CDummyNifAgentRef::CDummyNifAgentRef(const TDesC& aName,CTestMgr& aTheMgr):iName(aName),iTheMgr(aTheMgr)
{
}

CDummyNifAgentRef::~CDummyNifAgentRef()
{
 //	delete ref;
}

CDummyNifAgentRef* CDummyNifAgentRef::NewL(const TDesC& aName,CTestMgr& aTheMgr)
{
	CDummyNifAgentRef* ref = new (ELeave) CDummyNifAgentRef(aName,aTheMgr);
	CleanupStack::PushL(ref);
	ref->ConstructL();
	CleanupStack::Pop();
	return ref;
}

void CDummyNifAgentRef::ConstructL()
{

}

void CDummyNifAgentRef::StartL(CNifAgentFactory* aFactory)
{
	_LIT(RAS_NIF_NAME, "ntras");
	CCheekyNifAgentFactory* pFactoryExt=(CCheekyNifAgentFactory*)aFactory;
	iAgent = pFactoryExt->CreateAgentL(RAS_NIF_NAME);
	CCheekyNifAgentBase* pAgent=(CCheekyNifAgentBase*)iAgent;
	pAgent->SetNotify(this);//pretend to be "normal" agentref
	iAgent->Connect(EAgentStartDialOut/*EAgentStartOutgoingExplicit*/);
}
//
//MNifIfNotify overrides -------------------------------------------------------
//
//this member's called when the PPP negotiation is over.
void CDummyNifAgentRef::LinkLayerUp()
{
	TestLog.Printf(_L("Link Layer Up in the Agent was called\n"));
	iTheMgr.Notify(CTestMgr::ENifLoaded);
}

TInt CDummyNifAgentRef::Authenticate(TDes& aUsername, TDes& aPassword)
{
	TestLog.Printf(_L("Authenticating.....in the Agent was called\n"));
	iAgent->Authenticate(aUsername, aPassword);
	return KErrNone;
}
//
TInt CDummyNifAgentRef::Notification(TNifToAgentEventType /*aEvent*/, void* /*aInfo*/)
{
	return KErrNone;
}

void CDummyNifAgentRef::LinkLayerDown(TInt  /*aReason*/, TAction  /*aAction*/)
{
	TestLog.Printf(_L("Link Layer Down in the Agent was called\n"));
}
void CDummyNifAgentRef::NegotiationFailed(CNifIfBase*  /*aIf*/, TInt  /*aReason*/)
{
}

void CDummyNifAgentRef::BinderLayerDown(CNifIfBase*, TInt, TAction)
	{
	}

void CDummyNifAgentRef::CancelAuthenticate()
{
}
TInt CDummyNifAgentRef::GetExcessData(TDes8&  /*aBuffer*/)
{
	return KErrNone;
}

TInt CDummyNifAgentRef::DoReadInt(const TDesC&  aField, TUint32&  aValue,const RMessagePtr2* aMessage)
{
	return iAgent->ReadInt(aField, aValue,aMessage);
}

TInt CDummyNifAgentRef::DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage)
{
	return iAgent->ReadDes(aField, aValue, aMessage);
}

TInt CDummyNifAgentRef::DoReadDes(const TDesC& aField, TDes8& aValue ,const RMessagePtr2* aMessage )
{
	return iAgent->ReadDes(aField, aValue, aMessage);
}

TInt CDummyNifAgentRef::DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage)
{
	return iAgent->ReadBool(aField, aValue, aMessage);
}

TInt CDummyNifAgentRef::DoWriteInt( const TDesC& /*aField*/, TUint32 /*aValue*/,const RMessagePtr2* /*aMessage*/)
{
	__ASSERT_ALWAYS(ETrue,User::Panic(_L("CDummyNifAgentRef::WriteInt called"),KErrGeneral));
	return KErrNone;
}

TInt CDummyNifAgentRef::DoWriteDes(const TDesC& /*aField*/, const TDesC8& /*aValue*/,const RMessagePtr2* /*aMessage*/)
{
	__ASSERT_ALWAYS(ETrue,User::Panic(_L("CDummyNifAgentRef::WriteDes called"),KErrGeneral));
	return KErrNone;
}
TInt CDummyNifAgentRef::DoWriteDes(const TDesC& /*aField*/, const TDesC16& /*aValue*/,const RMessagePtr2* /*aMessage*/)
{
	__ASSERT_ALWAYS(ETrue,User::Panic(_L("CDummyNifAgentRef::WriteDes called"),KErrGeneral));
	return KErrNone;
}
TInt CDummyNifAgentRef::DoWriteBool( const TDesC& /*aField*/, TBool /*aValue*/,const RMessagePtr2* /*aMessage*/)
{
	__ASSERT_ALWAYS(ETrue,User::Panic(_L("CDummyNifAgentRef::WriteBool called"),KErrGeneral));
	return KErrNone;
}

void CDummyNifAgentRef::IfProgress(TInt /*aStage*/, TInt /*aError*/)
{
}

void CDummyNifAgentRef::OpenRoute()
{
}
void CDummyNifAgentRef::CloseRoute()
{
}

TInt CDummyNifAgentRef::PacketActivity(TDataTransferDirection /*aDirection*/, TUint /*aBytes*/, TBool /*aResetTimer*/)
{
	return KErrNone;
}

//
//MNifAgentNotify overrides -------------------------------------------------------
//
void CDummyNifAgentRef::ServiceStarted()
{
	TRAP_IGNORE(ServiceStartedL());

}
void CDummyNifAgentRef::ServiceStartedL()
{
	// here we deciding if to work with the ip or ip6.
	TName name=_L("qosppp");
	TText prtName_[64];
	TPtrC    prtName(prtName_,64);

	prtName.Set(_L("ip"));
	//prtName.Set(_L("ip6"));//    (currently not running) change by ido

	iTheMgr.StartLoadingNifL(name, TPtrC(prtName));
}

void CDummyNifAgentRef::ServiceClosed()
{
	delete iInterfaceBound;
	iInterfaceBound=0;
	delete iInterface;
	iInterface=0;
}

void CDummyNifAgentRef::ConnectComplete(TInt aStatus)
{
	if (aStatus==KErrNone)
	{
		TestLog.Printf(_L("Starting the Nif\n"));
		aStatus=iInterface->Start();
	}
	if(aStatus!=KErrNone)
	{
		TestLog.Printf(_L("Agent Call ConnectComplete BUT with some problem .\n"));
	//	iAgent->Disconnect(aStatus);
	}
}

void CDummyNifAgentRef::DisconnectComplete()
{
	ServiceClosed();
}

void CDummyNifAgentRef::ReconnectComplete(TInt /*aStatus*/)
{
}

void CDummyNifAgentRef::AuthenticateComplete(TInt aStatus)
{
	iInterface->AuthenticateComplete(aStatus);
}

void CDummyNifAgentRef::AgentProgress(TInt /*aStage*/, TInt /*aError*/)
{
}

TInt CDummyNifAgentRef:: Notification(TAgentToNifEventType /*aEvent*/, TAny* /*aInfo*/)
{
	return KErrNone;
}

TInt CDummyNifAgentRef::IncomingConnectionReceived()
{
	return KErrNone;
}

TName CDummyNifAgentRef::Name() const
{
	return iName;
}

void CDummyNifAgentRef::SetInterface(CNifIfLink* aLink)
{iInterface=aLink;};
void CDummyNifAgentRef::SetNifIfBase(CNifIfBase* aBase)
{	iInterfaceBound=aBase;}

void CDummyNifAgentRef::IfProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
{
}

void CDummyNifAgentRef::NotifyDataSent(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aUplinkVolume*/)
{
}

void CDummyNifAgentRef::NotifyDataReceived(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aDownlinkVolume*/)
{
}

void CDummyNifAgentRef::NifEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
{
}

void CDummyNifAgentRef::AgentProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
{
}

void CDummyNifAgentRef::AgentEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
{
}
