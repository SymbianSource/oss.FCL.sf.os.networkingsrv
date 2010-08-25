// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IP SubConnection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/corescpractivities.h>
#include <comms-infras/ss_corepractivities.h>
#include "ipdeftscpr.h"
#include "IPMessages.h"
#include <comms-infras/ss_log.h>
#include "IPCpr.h"

#include <comms-infras/ss_msgintercept.h>

#ifdef SYMBIAN_TRACE_ENABLE
	#define KIPSCprTag KESockSubConnectionTag
	//_LIT8(KIPSCprSubTag, "ipqosdeftscpr");
#endif // SYMBIAN_TRACE_ENABLE


using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace IPDeftBaseSCprActivities;
using namespace PRActivities;
using namespace CoreNetStates;

//-=========================================================
//
// States
//
//-=========================================================
namespace IPDeftSCprStates
{
DEFINE_SMELEMENT(TPolicyChecking, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext)
void TPolicyChecking::DoL()
	{
	IPBaseSCprStates::TStoreAddressUpdate storeAddressUpdate(iContext);
	storeAddressUpdate.DoL();

	TCFIPMessage::TDataClientRouted& addressUpdateMsg = message_cast<TCFIPMessage::TDataClientRouted>(iContext.iMessage);

	// check for src and dst address presence
	if( addressUpdateMsg.iAddrUpdate.iSrcSockAddr.Family() != KAFUnspec &&
		addressUpdateMsg.iAddrUpdate.iDestSockAddr.Family() != KAFUnspec )
		{
		RNodeInterface* ctrlProvider = iContext.Node().ControlProvider();
		User::LeaveIfError(ctrlProvider? KErrNone : KErrCorrupt);

	    RIPDataClientNodeInterface* client = static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);

		ctrlProvider->PostMessage(
			TNodeCtxId(iContext.ActivityId(), iContext.NodeId()),
			TCFIPMessage::TPolicyParams(
				addressUpdateMsg.iAddrUpdate,
				iContext.NodeId(),
				address_cast<TNodeId>(iContext.iSender),
				client->iAppSid
				).CRef()
			);
		}
	}
}//namespace 
//-=========================================================
//
// Activities
//
//-=========================================================
namespace IPDeftSCprAddressUpdate
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityAddressUpdate, IPDeftSCprAddressUpdate, TCFIPMessage::TDataClientRouted)
	NODEACTIVITY_ENTRY(KNoTag, IPDeftSCprStates::TPolicyChecking, IPBaseSCprStates::TAwaitingAddressUpdate, MeshMachine::TNoTag)
NODEACTIVITY_END()
}


DEFINE_SMELEMENT(CCommsIPBinderActivity::TFetchClientUids, NetStateMachine::MStateTransition, TContext)
void CCommsIPBinderActivity::TFetchClientUids::DoL()
	{
	MPlatsecApiExt* platsec(NULL);
   	TRAP_IGNORE(platsec = reinterpret_cast<MPlatsecApiExt*>(address_cast<TNodeId>(iContext.iSender).Node().FetchNodeInterfaceL(MPlatsecApiExt::KInterfaceId)));
   	if (platsec)
   		{
		TSecureId secureId = 0;
   		if(platsec->SecureId(secureId) != KErrNone)
   			{
   			return;
   			}

		CCommsIPBinderActivity* intf = reinterpret_cast<CCommsIPBinderActivity*>(iContext.iNodeActivity->FetchExtInterface(CCommsIPBinderActivity::KInterfaceId));
		if(intf)
			{
			intf->SetUid(TUid::Uid(secureId.iId));
			}
   		}
	}

DEFINE_SMELEMENT(CCommsIPBinderActivity::TUpdateClientUids, NetStateMachine::MStateTransition, TContext)
void CCommsIPBinderActivity::TUpdateClientUids::DoL()
	{
	/* Data client joined is recieved from the factory, which isn't the peer. Therefore we must look up
	 * the client using it's comms id which we can get from the dcjoined message
	 */
    RIPDataClientNodeInterface* client = static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);
    if (!client)
        {
        return;
        }

	CCommsIPBinderActivity* intf = reinterpret_cast<CCommsIPBinderActivity*>(iContext.iNodeActivity->FetchExtInterface(CCommsIPBinderActivity::KInterfaceId));
	if(intf)
		{
	    client->iAppSid = intf->GetUid();
		}
	}

namespace IPDeftSCprBinderRequestActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityBinderRequest, IPDeftSCprBinderRequest, TCFServiceProvider::TCommsBinderRequest, CCommsIPBinderActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingBinderRequest, MeshMachine::TNoTag )

	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CCommsIPBinderActivity::TFetchClientUids, IPDeftBaseSCprBinderRequestActivity::TNoTagOrUseExistingOrPermissionDenied)
	NODEACTIVITY_ENTRY(KNoTag, PRStates::TCreateDataClient, CoreNetStates::TAwaitingDataClientJoin, MeshMachine::TNoTag)
	// Below this point we need to modify the error handling approach. If we're getting a TError on TBinderResponse,
	// this means the client requesting the binder couldn't bind to it. As far as the client is concerned, this
	// activity is finished (it has flagged an error). The standard error handling will result in erroring
	// the originator. In this case we shouoldn't error the originator, instead, wrap up quietly.
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CCommsIPBinderActivity::TProcessDataClientCreationAndUpdateClientUids, MeshMachine::TTag<CoreStates::KUseExisting>)

	NODEACTIVITY_ENTRY(CoreStates::KUseExisting, CCommsBinderActivity::TSendBinderResponse, CCommsBinderActivity::TAwaitingBindToComplete, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TClearError)
	LAST_NODEACTIVITY_ENTRY(IPDeftBaseSCprBinderRequestActivity::KPermissionDenied, MeshMachine::TRaiseAndClearActivityError)
NODEACTIVITY_END()
}

namespace IPDeftSCprActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(activityMap)
	ACTIVITY_MAP_ENTRY(IPDeftSCprAddressUpdate, IPDeftSCprAddressUpdate)
	ACTIVITY_MAP_ENTRY(IPDeftSCprBinderRequestActivity, IPDeftSCprBinderRequest)
ACTIVITY_MAP_END_BASE(IPDeftBaseSCprActivities, ipdeftbasescprActivityMap)
}

//-=========================================================
//
// CIpDefaultSubConnectionProvider methods
//
//-=========================================================
CIpDefaultSubConnectionProvider::~CIpDefaultSubConnectionProvider()
    {
    LOG_NODE_DESTROY(KIPSCprTag, CIpDefaultSubConnectionProvider);
    }

CIpDefaultSubConnectionProvider::CIpDefaultSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory)
	: CIpDefaultBaseSubConnectionProvider(aFactory, IPDeftSCprActivities::activityMap::Self())
    {
    LOG_NODE_CREATE(KIPSCprTag, CIpDefaultSubConnectionProvider);
    }

CIpDefaultSubConnectionProvider* CIpDefaultSubConnectionProvider::NewL(ESock::CSubConnectionProviderFactoryBase& aFactory)
    {
    CIpDefaultSubConnectionProvider* provider = new (ELeave) CIpDefaultSubConnectionProvider(aFactory);
    CleanupStack::PushL(provider);
    provider->ConstructL();

    CleanupStack::Pop(provider);
    return provider;
    }

void CIpDefaultSubConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
   	TNodeContext<CIpDefaultSubConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	CCoreSubConnectionProvider::ReceivedL(aSender, aRecipient, aMessage);
	User::LeaveIfError(ctx.iReturn);
	}

MeshMachine::CNodeActivityBase* CCommsIPBinderActivity::NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
    {
	TUint c = GetNextActivityCountL(aActivitySig,aNode);
    return new(ELeave)CCommsIPBinderActivity(aActivitySig, aNode, c);
    }

CCommsIPBinderActivity::CCommsIPBinderActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount)
:	CCommsBinderActivity(aActivitySig, aNode, aNextActivityCount)
	{
	iAppSid = TUid::Null();
	}
