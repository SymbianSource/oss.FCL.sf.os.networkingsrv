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
// The objects of the class do all work to start & finish PPP session
// 
//

#include "DummyAgtRef.h"
#include "nifman.h"
#include "cdbcols.h"
#include "TestMgr.h"
#include "dummyprotocol.h"
#include "ss_pman.h"

CDummyNifAgentRef::CDummyNifAgentRef(const TDesC& aName,CTestMgr& aTheMgr, CTestExecuteLogger& aLogger):iName(aName),iTheMgr(aTheMgr), iLogger(aLogger),
	iState(EStopped) // When the AgentRef comes up, the NIF is disconnected.
{
}

CDummyNifAgentRef::~CDummyNifAgentRef()
{
	delete ipDummyPrt;
	DestroyIniReader();
}

CDummyNifAgentRef*
CDummyNifAgentRef::NewL(const TDesC& aName,CTestMgr& aTheMgr, CTestExecuteLogger& aLogger)
{

	CDummyNifAgentRef* ref = new (ELeave) CDummyNifAgentRef(aName,aTheMgr, aLogger);
	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, ref));
	ref->ConstructL();
	CleanupStack::Pop();
	return ref;
}


void CDummyNifAgentRef::ConstructL()
{
	ipDummyPrt=CDummyProtocol::NewL();
	CreateIniReader();
}

//this member's called when the PPP negotiation is over.
void CDummyNifAgentRef::LinkLayerUp()
{
	LOG_INFO_PRINTF1(_L("DummyAgtRef::LinkLayerUp"));
		
	iTheMgr.Notify(CTestMgr::ETestFinished);
	ServiceClosed();//cleanup
}

TInt CDummyNifAgentRef::Authenticate(TDes& /*aUsername*/, TDes& /*aPassword*/)
{
	//ask the test mgr for authentication information
	return KErrNone;
}
//
TInt CDummyNifAgentRef::Notification(TNifToAgentEventType /*aEvent*/, void* /*aInfo*/)
{
	//log them all ???
	return KErrNone;
}

//
//MNifIfNotify overrides -------------------------------------------------------
//
TInt CDummyNifAgentRef::DoReadInt(const TDesC&  aField, TUint32&  aValue,const RMessagePtr2* /*aMessage*/)
{
	TInt result = KErrNotFound;
	if (ipIniFileReader !=0)
	{
		TInt value=0;
		if (ipIniFileReader->FindVar(iData.KCommDbSectionName,aField,value))
		{
			aValue=(TInt32)value;
			result=KErrNone;
		}
	}
	//use hardcoded only if the ini file read or search failed
	if (KErrNotFound==result)
	{
		result=KErrNone;

		if (TPtrC(KCDTypeNameIfServerMode)==aField)
			aValue=iData.KPppIsServerMode;
		else if (TPtrC(iData.KModemCommRoleString)==aField)
			aValue=iData.KPppCommRole;
		else if (iData.KIapIdString==aField)
			aValue=iData.KPppIapId;
		else
			result=KErrNotFound;
	}
	//
	LOG_ERR_PRINTF4(_L("--------------- Config param used: %S = %d (err %d)"),&aField,aValue,result);
	//
	return result;
}

TInt CDummyNifAgentRef::DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* /*aMessage*/)
{
	TInt result = KErrNotFound;
	if (ipIniFileReader !=0)
	{
		TPtrC value;
		if (ipIniFileReader->FindVar(iData.KCommDbSectionName,aField,value))
		{
			aValue.Copy(value);
			result=KErrNone;
		}
	}
	//use hardcoded only if the ini file read or search failed
	if (KErrNotFound==result)
	{
		result=KErrNone;
		//
		if (TPtrC(KCDTypeNameRecordName)==aField)
			aValue=iData.KPppCommdbName;
		else if (iData.KModemPortString==aField)
			aValue=iData.KPppPortString;
		else if (iData.KModemCsyString==aField)
			aValue=iData.KPppCsyString;
		else
			result=KErrNotFound;
	}
	//
	LOG_ERR_PRINTF4(_L("--------------- Config param used: %S = %S (err %d)"),&aField,&aValue,result);
	//
	return result;
}

TInt CDummyNifAgentRef::DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* /*aMessage*/)
{
	TInt result = KErrNotFound; //optimistic approach
	if (ipIniFileReader !=0)
	{
		TInt value=0;
		if (ipIniFileReader->FindVar(iData.KCommDbSectionName,aField,value))
		{
			aValue=(value != 0);
			result=KErrNone;
		}
	}
	//use hardcoded only if the ini file read or search failed
	if (KErrNotFound==result)
	{
		result=KErrNone;

		if (TPtrC(KCDTypeNameEnableLcpExtensions) == aField)
			aValue=iData.KPppIsLcpExtEnabled;
		else if (TPtrC(KCDTypeNameEnableSwComp) == aField)
			aValue=iData.KPppIsSwCompEnabled;
		else if (TPtrC(KCDTypeNameIfCallbackEnabled) == aField)
			aValue=iData.KPppIsCallbackEnabled;
		else
			result=KErrNotFound;
	}
	//
	LOG_ERR_PRINTF4(_L("--------------- Config param used: %S = %d (err %d)"),&aField,(TInt)aValue,result);
	//
	return result;
}
//
//MNifAgentNotify overrides -------------------------------------------------------
//

//Nasty classes to sneak into protected sections
class CCheekyNifIfBase:public CNifIfBase
{
};

class CCheekyNifIfFactory : public CNifIfFactory
{
public:
	CNifIfBase*
		CreateInterfaceL(const TDesC& aName, MNifIfNotify* aNotify){ return NewInterfaceL(aName, aNotify);}
};

void CDummyNifAgentRef::ServiceStartedL()
{
	LOG_INFO_PRINTF1(_L("DummyAgtRef::ServiceStartedL"));
		
	// load the required interface
	TInt errCode=KErrNone;
	//create the interface as nifman does. The code taken from NifMan (with custom error check)
	TAutoClose<RLibrary> lib;
	errCode=lib.iObj.Load(iData.KPppNifFileName);
	if (errCode!=KErrNone)
	{
		LOG_ERR_PRINTF2(_L("Can't load tppp.nif (error=%d) -> Leaving"),errCode);
		User::Leave(errCode);
	}
	lib.PushL();
	// The Uid check
#ifdef _UNICODE
	TUid uid(TUid::Uid(KUidUnicodeNifmanInterface));
#else
	TUid uid(TUid::Uid(KUidNifmanInterface));
#endif
	if(lib.iObj.Type()[1]!=uid)
	{
		LOG_INFO_PRINTF1(_L("Wrong UID of ppp.nif -> Leaving"));
		User::Leave(KErrCorrupt);
	}
	//get the entry point
	typedef CNifFactory* (*TNifFactoryNewL)();
	TNifFactoryNewL libEntry=(TNifFactoryNewL)lib.iObj.Lookup(1);//the factory function has ordinal ==1
	if (libEntry==NULL)
	{
		LOG_INFO_PRINTF1(_L("No factory method in ppp.nif -> Leaving"));
		User::Leave(KErrCorrupt);
	}

	CNifFactory* pFactory =(*libEntry)(); // Opens CObject
	if (!pFactory)
	{
		LOG_INFO_PRINTF1(_L("Can't create factory in ppp.nif -> Leaving"));
		User::Leave(KErrCorrupt);
	}

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, pFactory));
	CObjectCon* pContainer=CObjectCon::NewL();
	CleanupStack::PushL(pContainer);
	pFactory->InitL(lib.iObj, *pContainer); // Transfers the library object if successful
	// Can pop the library now - auto close will have no effect because handle is null
	CleanupStack::Pop();//pContainer
	CleanupStack::Pop();//pFactory
	lib.Pop();

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, pFactory));
	TName name=_L("ppp");
	//another dirty hack
	CCheekyNifIfFactory* pFactoryExt=(CCheekyNifIfFactory*)pFactory;
    iInterface = reinterpret_cast<CNifIfLink*>(pFactoryExt->CreateInterfaceL(name, this));
    CleanupStack::PopAndDestroy(); // close extra reference on Factory

	if(!iInterface)
	{
		LOG_INFO_PRINTF1(_L("Failed to create interface -> Leaving"));
		User::Leave(KErrCorrupt);
	}
	
	//chose ip or ip6
	TText prtName_[64];
	TPtrC    prtName(prtName_,64);
	 if (ipIniFileReader ==0 || //problem with ppp.ini reading
		(!ipIniFileReader->FindVar(iData.KCommDbSectionName,TPtrC(KCDTypeNameIfNetworks),prtName)))//or no corresponding item
	{
		prtName.Set(_L("ip6"));//use default then
	}
	//use hardcoded only if the ini file read or search failed
	iInterfaceBound=iInterface->GetBinderL(TPtrC(prtName));//Do as if we bind to the ipv6 protocol
	iInterface->BindL(ipDummyPrt);     //and instead of the protocol put just a dummy
}

void
CDummyNifAgentRef::ServiceStarted()
{
	TRAPD(errCode,ServiceStartedL());
	if (errCode!=KErrNone)
	{
		LOG_INFO_PRINTF1(_L("Can't continue -> Stopping the test"));
		iTheMgr.Notify(CTestMgr::EStopTest);//and it's enough in our case
	}
	LOG_INFO_PRINTF2(_L("DummyAgtRef::ServiceStarted. ServiceStartedL returned err[%d]"), errCode);		
}


void CDummyNifAgentRef::ServiceClosed()
{
	LOG_INFO_PRINTF1(_L("DummyAgtRef::ServiceClosed"));
		
	delete iInterfaceBound;
	iInterfaceBound=0;
	delete iInterface;
	iInterface=0;

}

void CDummyNifAgentRef::ConnectComplete(TInt aStatus)
{
	LOG_INFO_PRINTF2(_L("DummuAgtRef::ConnectComplete aStatus[%d]"), aStatus);
		
	if (aStatus==KErrNone)
	{
		aStatus=iInterface->Start();
		iState = ELinkUp;
		LOG_INFO_PRINTF2(_L("DummyAgtRef::ConnectComplete: Interface started: state[%d] (ELinkUp)"), iState);
	}
	if(aStatus!=KErrNone)
	{
	//	iAgent->Disconnect(aStatus);
	}
}

void CDummyNifAgentRef::DisconnectComplete()
{
	LOG_INFO_PRINTF1(_L("DummyAgtRef::DisconnectComplete"));
		
	ServiceClosed();
}
void CDummyNifAgentRef::Stop()
{
	LOG_INFO_PRINTF2(_L("DummyAgtRef::Stop iState[%d]"), iState);
		
	// Simplified replica of real CNifAgentRef::Stop
	// Stop can be called while the NIF is negotiating termination. PPP NIF does not support correct handling
	// of repeated Stop calls (i.e. Close events) - it will not behave as specified in RFC1661. Therefore, PPP must be
	// "shielded."
	switch(iState)
		{
		case ELinkUp: // NIF is Up. It is OK to stop it.
			iState = EStopping; // Negotiating termination.
			iInterface->Stop(KErrCancel,MNifIfNotify::EDisconnect); // Start to negotiate termination.
			break;
			
		case EStopping: // NIF is negotiating termination already
		case EStopped:  // NIF has informed us that it is finished.
			break; 
			
		default:			
			User::Panic(_L("CDummyNifAgentRef::Stop: Illegal AgentRef state."),KErrGeneral);			
		}
	
}

void CDummyNifAgentRef::LinkLayerDown(TInt  aReason, TAction  aAction)
{
	LOG_INFO_PRINTF4(_L("DummyAgtRef::LinkLayerDown iState[%d] Reason[%d] Action[%d] "), iState, aReason, aAction);
	iState = EStopped;		
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
TInt CDummyNifAgentRef::DoWriteInt( const TDesC& /*aField*/, TUint32 /*aValue*/,const RMessagePtr2* /*aMessage*/)
{
	__ASSERT_ALWAYS(ETrue,User::Panic(_L("CDummyNifAgentRef::WriteInt called"),KErrGeneral));
	return KErrNone;
}
TInt CDummyNifAgentRef::DoReadDes(const TDesC& /*aField*/, TDes8& /*aValue*/,const RMessagePtr2* /*aMessage*/)
{
	__ASSERT_ALWAYS(ETrue,User::Panic(_L("CDummyNifAgentRef::ReadDes called"),KErrGeneral));
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
void CDummyNifAgentRef::IfProgress(TInt aStage, TInt aError)
{
LOG_INFO_PRINTF4(_L("DummyAgtRef::IfProgress iState[%d] Stage[%d] Error[%d] "), iState, aStage, aError);
}

void CDummyNifAgentRef::IfProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
{
}

void CDummyNifAgentRef::OpenRoute()
{
}
void CDummyNifAgentRef::CloseRoute()
{
}

void CDummyNifAgentRef::ReconnectComplete(TInt /*aStatus*/)
{
}
void CDummyNifAgentRef::AuthenticateComplete(TInt /*aStatus*/)
{
}
void CDummyNifAgentRef::AgentProgress(TInt /*aStage*/, TInt /*aError*/)
{
}

void CDummyNifAgentRef::AgentProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
{
}

TInt
CDummyNifAgentRef:: Notification(TAgentToNifEventType /*aEvent*/, TAny* /*aInfo*/)
{
	return KErrNone;
}
TInt CDummyNifAgentRef::IncomingConnectionReceived()
{
	return KErrNone;
}

void CDummyNifAgentRef::AgentEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
{
}

TName CDummyNifAgentRef::Name() const
{
	return iName;
}
void CDummyNifAgentRef::Close()
{
}

TInt CDummyNifAgentRef::PacketActivity(TDataTransferDirection /*aDirection*/, TUint /*aBytes*/, TBool /*aResetTimer*/)
{
	return KErrNone;
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
