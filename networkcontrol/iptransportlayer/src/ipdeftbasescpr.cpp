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
// IP Default Base SubConnection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/

#define SYMBIAN_NETWORKING_UPS

#include "IPMessages.h"

#include <comms-infras/corescpractivities.h>
#include <comms-infras/ss_corepractivities.h>
#include <comms-infras/ss_nodemessages_rejoiningprovider.h>
#include <comms-infras/ss_nodemessages_dataclient.h>
#include "ipdeftbasescpr.h"
#include <comms-infras/ss_log.h>
#include <comms-infras/ss_platsec_apiext.h>
#include <networking/qos3gpp_subconparams.h>

#include <comms-infras/ss_msgintercept.h>
#ifdef SYMBIAN_NETWORKING_UPS
#include <comms-infras/upspractivities.h>
#endif

#ifdef SYMBIAN_TRACE_ENABLE
	#define KIPDeftSCprTag KESockSubConnectionTag
#endif // SYMBIAN_TRACE_ENABLE


using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace IPDeftSCprBaseActivities;
using namespace PRActivities;
using namespace CoreNetStates;

//-=========================================================
//
// States
//
//-=========================================================
namespace IPBaseSCprStates
{
DEFINE_SMELEMENT(TAwaitingAddressUpdate, NetStateMachine::MState, IPBaseSCprStates::TContext)
TBool TAwaitingAddressUpdate::Accept()
	{
	return iContext.iMessage.IsMessage<TCFIPMessage::TDataClientRouted>();
	}

DEFINE_SMELEMENT(TStoreAddressUpdate, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext)
void TStoreAddressUpdate::DoL()
	{
    RIPDataClientNodeInterface* client = static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);
    if (NULL == client)
        {
        User::Leave(KErrNotFound);
        }

    TCFIPMessage::TDataClientRouted& addressUpdateMsg = message_cast<TCFIPMessage::TDataClientRouted>(iContext.iMessage);
    client->iCliDstAddr = addressUpdateMsg.iAddrUpdate.iDestSockAddr;
    client->iCliSrcAddr = addressUpdateMsg.iAddrUpdate.iSrcSockAddr;
    client->iProtocolId = addressUpdateMsg.iAddrUpdate.iProtocolId;
    if (iContext.Node().iIapId == CIpSubConnectionProviderBase::KInvalidIapId)
        {
        iContext.Node().iIapId = addressUpdateMsg.iAddrUpdate.iIapId;
        }
	}

DEFINE_SMELEMENT(TRejoinDataClient, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext)
void TRejoinDataClient::DoL()
	{
    PRActivities::CRejoinDataClientActivity::TRejoinDataClient sendRejoinReq(iContext);
    sendRejoinReq.DoL();
	ASSERT(iContext.iNodeActivity);
	PRActivities::CRejoinDataClientActivity* rejoinActivity =
	    static_cast<PRActivities::CRejoinDataClientActivity*>(iContext.iNodeActivity);
    RIPDataClientNodeInterface& dc =
        static_cast<RIPDataClientNodeInterface&>(
        rejoinActivity->iDataClients[rejoinActivity->iDataClients.Count()-1].iDataClient);
    TNodeId& newOwner = rejoinActivity->iDataClients[rejoinActivity->iDataClients.Count()-1].iNewOwner;

	RClientInterface::OpenPostMessageClose(dc.RecipientId(), newOwner,
		TCFIPMessage::TDataClientRouted(TAddrUpdate(dc.iCliSrcAddr, dc.iCliDstAddr, dc.iProtocolId, iContext.Node().iIapId)).CRef());
	}
	

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
State waiting for TTransportNotification message which carries TCP Receive window.
*/
DEFINE_SMELEMENT(TAwaitingTransportNotification, NetStateMachine::MState, IPBaseSCprStates::TContext)
TBool IPBaseSCprStates::TAwaitingTransportNotification::Accept()
	{
	return iContext.iMessage.IsMessage<TCFMessage::TTransportNotification>();
	}
/*
StateTransition which will iterate thru all the data clients, send the TCP receive window size to them.
*/
DEFINE_SMELEMENT(TSendTransportNotificationToDataClients, NetStateMachine::MStateTransition,IPBaseSCprStates::TContext)
void IPBaseSCprStates::TSendTransportNotificationToDataClients::DoL()
	{
	
	//data client iterator.
	//Exclude data clients that are ELeaving otherwise the PostMessage() below will panic.
	TClientIter<TDefaultClientMatchPolicy> iter = iContext.Node().GetClientIter<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData), TClientType(0, TCFClientType::ELeaving));
   	//Data Client number
   	TInt dataCliNum =NULL;
  	//Send message to all data clients.
   	RNodeInterface* dataClient = iter[dataCliNum];
   	
   	TCFMessage::TTransportNotification message;
  	while (dataClient)
   		{
		//Post messages to the data client. Message will flow to all the data clients i.e. socket flows.
 		dataClient->PostMessage(iContext.NodeId(), message);
 		dataCliNum++;
 		dataClient = iter[dataCliNum];
   		}
	}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

}


//-=========================================================
//
// Activities
//
//-=========================================================

namespace IPDeftBaseSCprAddressUpdate
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityAddressUpdate, IPDeftBaseSCprAddressUpdate, TCFIPMessage::TDataClientRouted)
	NODEACTIVITY_ENTRY(KNoTag, IPBaseSCprStates::TStoreAddressUpdate, IPBaseSCprStates::TAwaitingAddressUpdate, MeshMachine::TNoTag)
NODEACTIVITY_END()
}

namespace IPDeftSCprRejoinDataClient
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityRejoin, IPDeftSCprRejoin, TCFRejoiningProvider::TRejoinDataClientRequest, PRActivities::CRejoinDataClientActivity::NewL)
	// If an SCprNoBearer is being processed (e.g. RSocket::Connect()) when IPDeftSCprRejoin is started (RSubConnection::AddSocket()),
	// park until the SCprNoBearer completes.  This ensures that the EStarting flag (set temporarily during SCprNoBearer)
	// is not inadvertantly propagated (via the DataClientJoiningRequest message) to the receiving SCpr.  If this happens,
	// EStarting will never get reset on the receiving SCpr and can cause activities (like PRDataClientStop) to park
	// permanently on that SCpr.
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientRejoin, CoreNetStates::TNoTagBlockedByNoBearer)
	NODEACTIVITY_ENTRY(KNoTag,               IPBaseSCprStates::TRejoinDataClient, PRActivities::CRejoinDataClientActivity::TAwaitingJoinComplete, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(CoreStates::KLoopTag, IPBaseSCprStates::TRejoinDataClient, PRActivities::CRejoinDataClientActivity::TAwaitingJoinComplete, MeshMachine::TNoTag)

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendRejoinComplete, CoreNetStates::TAwaitingDataClientRejoinOrApplyOrCancel, PRActivities::CRejoinDataClientActivity::TRejoinLoopTag)
	NODEACTIVITY_ENTRY(KNoTag, PRActivities::CRejoinDataClientActivity::TApplyRejoin, CoreNetStates::TAwaitingApplyResponse, MeshMachine::TNoTag)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendApplyResponse)
#else
	LAST_NODEACTIVITY_ENTRY(KNoTag, SCprStates::TSendApplyResponse)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
NODEACTIVITY_END()
}

namespace IPDeftBaseSCprBinderRequestActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityBinderRequest, IPDeftBaseSCprBinderRequest, TCFServiceProvider::TCommsBinderRequest, CCommsBinderActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingBinderRequest, TNoTagOrUseExistingOrPermissionDenied)

	NODEACTIVITY_ENTRY(KNoTag, PRStates::TCreateDataClient, CoreNetStates::TAwaitingDataClientJoin, MeshMachine::TNoTag)

	// Below this point we need to modify the error handling approach. If we're getting a TError on TBinderResponse,
	// this means the client requesting the binder couldn't bind to it. As far as the client is concerned, this
	// activity is finished (it has flagged an error). The standard error handling will result in erroring
	// the originator. In this case we shouoldn't error the originator, instead, wrap up quietly.
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CCommsBinderActivity::TProcessDataClientCreation, MeshMachine::TTag<CoreStates::KUseExisting>)

	NODEACTIVITY_ENTRY(CoreStates::KUseExisting, CCommsBinderActivity::TSendBinderResponse, CCommsBinderActivity::TAwaitingBindToComplete, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TClearError)
	LAST_NODEACTIVITY_ENTRY(KPermissionDenied, MeshMachine::TRaiseAndClearActivityError)
NODEACTIVITY_END()
}

namespace IPDeftBaseSCprBinderRequestActivity
{
const TUint32 KSIPSecureId  = 270490934;
const TUint32 KDHCPSecureId = 270522821;
const TUint32 KDNSSecureId  = 268437634;

DEFINE_SMELEMENT(TNoTagOrUseExistingOrPermissionDenied, NetStateMachine::MStateFork, TContext)
TInt TNoTagOrUseExistingOrPermissionDenied::TransitionTag()
	{
	ASSERT(iContext.iNodeActivity);

	// have to cast the context sicne we're inheriting
	NetStateMachine::TContextAccessor<IPDeftBaseSCprBinderRequestActivity::TContext,CCommsBinderActivity::TContext> ctxAccessor(iContext);

	if (ctxAccessor.Context().Node().ImsFlag())
		{
		TSecureId secureId = 0;
		MPlatsecApiExt* extn = NULL;
		TRAPD(err, extn = reinterpret_cast<MPlatsecApiExt*>(address_cast<TNodeId>(iContext.iSender).Node().FetchNodeInterfaceL(MPlatsecApiExt::KInterfaceId)));
		if (err == KErrNone || extn != NULL)
			{
			if ((extn->SecureId(secureId) != KErrNone) ||
				(secureId.iId != KSIPSecureId && secureId.iId != KDHCPSecureId && secureId.iId != KDNSSecureId))
				{
				iContext.iNodeActivity->SetError(KErrPermissionDenied);
				return KPermissionDenied;
				}
			}
		}

	return PRActivities::CCommsBinderActivity::TNoTagOrUseExisting::TransitionTag();
	}
}

namespace IPDeftBaseSCprDataClientStartActivity
{
DEFINE_SMELEMENT(TGetParams, NetStateMachine::MStateTransition, TContext)
void TGetParams::DoL()
	{
	ASSERT(iContext.iNodeActivity);
	if(iContext.Node().ServiceProvider() == NULL)
    	{
    	//The service provider could have dissapeared by now.
    	User::Leave(KErrDied);
    	}
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	iContext.iNodeActivity->PostRequestTo(*iContext.Node().ServiceProvider(),
		TCFScpr::TSetParamsRequest(RCFParameterFamilyBundleC()).CRef());
#else
 	iContext.iNodeActivity->PostRequestTo(*iContext.Node().ServiceProvider(),
		TCFScpr::TParamsRequest(RCFParameterFamilyBundleC()).CRef());
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	}
}


namespace IPDeftBaseSCprDataClientStartActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityStartDataClient, IPDeftBaseSCprDataClientStart, TCFDataClient::TStart )
    FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientStart, MeshMachine::TNoTag)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	//NODEACTIVITY_ENTRY(KNoTag, IPDeftBaseSCprDataClientStartActivity::TGetParams, CoreNetStates::TAwaitingParamResponse, CoreNetStates::TNoTagOrNoDataClients)
	NODEACTIVITY_ENTRY(KNoTag, IPDeftBaseSCprDataClientStartActivity::TGetParams, CoreNetStates::TAwaitingParamResponse, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, PRStates::TStoreParams, CoreNetStates::TNoTagOrNoDataClients)
#else
	NODEACTIVITY_ENTRY(KNoTag, IPDeftBaseSCprDataClientStartActivity::TGetParams, CoreNetStates::TAwaitingParamResponse, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, SCprStates::TStoreParams, CoreNetStates::TNoTagOrNoDataClients)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW	
	NODEACTIVITY_ENTRY(KNoTag, PRStates::TStartDataClients, CoreNetStates::TAwaitingDataClientsStarted, MeshMachine::TTag<CoreNetStates::KNoDataClients>)
	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KNoDataClients, PRStates::TSendDataClientStarted)
NODEACTIVITY_END()
}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

/*
Custom activity which waits for TTransportNotification message from IPCPR.
Stores the TCP receive window size in pointer appended to TProvisionConfig
and sends the window to all data clients.
*/
namespace IPDeftBaseSCPRBearerCharActivity
{
DECLARE_DEFINE_NODEACTIVITY(IPDeftSCprBaseActivities::ECFActivityReceiveWin, IPDeftBaseSCPRBearerCharActivity, TCFMessage::TTransportNotification)
	FIRST_NODEACTIVITY_ENTRY(IPBaseSCprStates::TAwaitingTransportNotification, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IPBaseSCprStates::TSendTransportNotificationToDataClients, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace IPDeftSCprBaseActivities
{
DEFINE_ACTIVITY_MAP(ipscprbaseActivityMap)
	ACTIVITY_MAP_ENTRY(IPDeftSCprRejoinDataClient, IPDeftSCprRejoin)

#ifdef SYMBIAN_NETWORKING_UPS
ACTIVITY_MAP_END_BASE(UpsActivities, upsActivitiesSCpr)
#else
ACTIVITY_MAP_END_BASE(SCprActivities, coreSCprActivities)
#endif
}


namespace IPDeftBaseSCprActivities
{
DEFINE_ACTIVITY_MAP(ipdeftbasescprActivityMap)
	ACTIVITY_MAP_ENTRY(IPDeftBaseSCprAddressUpdate, IPDeftBaseSCprAddressUpdate)
	ACTIVITY_MAP_ENTRY(IPDeftBaseSCprDataClientStartActivity, IPDeftBaseSCprDataClientStart)
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(IPDeftBaseSCPRBearerCharActivity, IPDeftBaseSCPRBearerCharActivity)
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	
	ACTIVITY_MAP_ENTRY(IPDeftBaseSCprBinderRequestActivity, IPDeftBaseSCprBinderRequest)
ACTIVITY_MAP_END_BASE(IPDeftSCprBaseActivities, ipscprbaseActivityMap)
}


//-=========================================================
//
// CIpSubConnectionProviderBase methods
//
//-=========================================================
CIpSubConnectionProviderBase::CIpSubConnectionProviderBase(ESock::CSubConnectionProviderFactoryBase& aFactory,
                             const MeshMachine::TNodeActivityMap& aActivityMap)
:CCoreSubConnectionProvider(aFactory, aActivityMap),
 iIapId(KInvalidIapId)
    {
    }

RNodeInterface* CIpSubConnectionProviderBase::NewClientInterfaceL(const TClientType& aClientType, TAny* aClientInfo)
    {
    if (aClientType.Type() & TCFClientType::EData)
        {
        return new (ELeave) RIPDataClientNodeInterface();
        }
    return CCoreSubConnectionProvider::NewClientInterfaceL(aClientType, aClientInfo);
    }


//-=========================================================
//
// CIpDefaultBaseSubConnectionProvider methods
//
//-=========================================================
CIpDefaultBaseSubConnectionProvider::~CIpDefaultBaseSubConnectionProvider()
    {
    LOG_NODE_DESTROY(KIPDeftSCprTag, CIpDefaultBaseSubConnectionProvider);
    }

CIpDefaultBaseSubConnectionProvider::CIpDefaultBaseSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory,
                             const MeshMachine::TNodeActivityMap& aActivityMap)
    : CIpSubConnectionProviderBase(aFactory, aActivityMap)
    {
    LOG_NODE_CREATE(KIPDeftSCprTag, CIpDefaultBaseSubConnectionProvider);
    }

CIpDefaultBaseSubConnectionProvider* CIpDefaultBaseSubConnectionProvider::NewL(ESock::CSubConnectionProviderFactoryBase& aFactory)
    {
    CIpDefaultBaseSubConnectionProvider* provider = new (ELeave) CIpDefaultBaseSubConnectionProvider(aFactory, IPDeftBaseSCprActivities::ipdeftbasescprActivityMap::Self());
    CleanupStack::PushL(provider);
    provider->ConstructL();

    CleanupStack::Pop(provider);
    return provider;
    }

void CIpDefaultBaseSubConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
   	TNodeContext<CIpDefaultBaseSubConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
	CCoreSubConnectionProvider::ReceivedL(aSender, aRecipient, aMessage);
	User::LeaveIfError(ctx.iReturn);
	}

TBool CIpDefaultBaseSubConnectionProvider::ImsFlag()
	{
	TBool imsFlagSet = EFalse;
	if ( iParameterBundle.IsNull() )
		{
		return imsFlagSet;
		}

	RParameterFamily family=iParameterBundle.FindFamily(KSubConnContextDescrParamsFamily);
	if( ! family.IsNull())
		{
		CSubConImsExtParamSet* imsExtGranted = static_cast<CSubConImsExtParamSet*>(
			family.FindParameterSet(STypeId::CreateSTypeId(KSubCon3GPPExtParamsFactoryUid,KSubConImsExtParamsType),
									 RParameterFamily::EGranted));

		if (imsExtGranted)
			imsFlagSet = imsExtGranted->GetImsSignallingIndicator();
		}

	return imsFlagSet;
	}
	
