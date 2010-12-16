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
// IPProto MCPR
// 
//

/**
 @file
 @internalComponent
*/


#include <comms-infras/ss_log.h>
//#include <ss_nodestates.h>
#include <comms-infras/corecprstates.h>
#include <comms-infras/coremcprstates.h>
#include <comms-infras/coremcpractivities.h>
#include <commsdattypesv1_1.h> // CommsDat
#include <comms-infras/ss_tiermanager.h> //TierManagerUtils
#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/netcfgextprov.h>
#include "IPProtoMCpr.h"
#include "idletimer.h"


#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KIPProtoMCprTag KESockMetaConnectionTag
_LIT8(KIPProtoMCprSubTag, "ipprotomcpr");
#endif

#include <comms-infras/ss_msgintercept.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace CommsDat;
using namespace MCprActivities;
using namespace IPProtoMCprStates;
//
//CIPProtoMetaConnectionProvider
CIPProtoMetaConnectionProvider* CIPProtoMetaConnectionProvider::NewL(CMetaConnectionProviderFactoryBase& aFactory,
                                                                     const TProviderInfo& aProviderInfo)
	{
	CIPProtoMetaConnectionProvider* self = new (ELeave) CIPProtoMetaConnectionProvider(aFactory,aProviderInfo,IPProtoMCprActivities::ipProtoActivitiesMCpr::Self());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CIPProtoMetaConnectionProvider::CIPProtoMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory,
                                                               const TProviderInfo& aProviderInfo,
                                                               const MeshMachine::TNodeActivityMap& aActivityMap)
	:	CCoreMetaConnectionProvider(aFactory,aProviderInfo,aActivityMap), iIapLocked(EFalse)
	{
	LOG_NODE_CREATE(KIPProtoMCprTag, CIPProtoMetaConnectionProvider);
	}

CIPProtoMetaConnectionProvider::~CIPProtoMetaConnectionProvider()
	{
	LOG_NODE_DESTROY(KIPProtoMCprTag, CIPProtoMetaConnectionProvider);
	}


void CIPProtoMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	__CFLOG_VAR((KIPProtoMCprTag, KIPProtoMCprSubTag, _L8("CIPProtoMetaConnectionProvider %08x:\tReceived() aCFMessage=%d"),
	   this, aMessage.MessageId().MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CIPProtoMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
    CCoreMetaConnectionProvider::Received(ctx);

    User::LeaveIfError(ctx.iReturn);
	}

void CIPProtoMetaConnectionProvider::ConstructL()
    {
    CMetaConnectionProviderBase::ConstructL(); //This initialises the AccessPointInfo

  	//For construction of IpProto providers (construction only) we use the instance field of TProviderInfo
   	//to carry the IAP id. The IAP id is necessary for the initialisation if IpProto specific data.
    TInt iap = (TInt)ProviderInfo().Instance();
	SetProviderInfo(TProviderInfo(ProviderInfo().TierId(),iap));
    SetConfigL();
    }

void CIPProtoMetaConnectionProvider::SetConfigL()
/**
Setup the provisioning information for the IPProto layer.
*/
    {
    RMetaExtensionContainer mec;
    mec.Open(AccessPointConfig());
    CleanupClosePushL(mec);

    //At this moment our ProviderInfo().APId() points to the IAP this provider represents
    TUint configurationIap = ProviderInfo().APId();
    CCommsDatIapView* iapView = CCommsDatIapView::NewLC(configurationIap);

    // Read Idle timer values
    TIdleTimerValues* timerValues = new (ELeave) TIdleTimerValues;
    CleanupStack::PushL(timerValues);
    iapView->GetTimeoutValuesL(timerValues->iShortTimer, timerValues->iMediumTimer, timerValues->iLongTimer);
    mec.AppendExtensionL(timerValues); //iAccessPointConfig owns timerValues from now on
    CleanupStack::Pop(timerValues);

    // Read NetworkId
    TUint32 net = 0;
    iapView->GetIntL(KCDTIdIAPNetwork, net);

	// Read Protocol list (from IfNetworks field).  This contains a comma seperated list of
	// protocol names and determines the binders instantiated in the lower CFProtocol (typically
	// "ip" or "ip,ip6").

	HBufC* protocolList = NULL;

	if (iapView->GetServiceTableType() == KCDTIdVPNServiceRecord)
		{
		// *** HACK ***
		// Normally, the IfNetworks field is read via the Agent ReadDes() methods.  This
		// allow, for example, the VPNConnAgt to fake up the IfNetworks field to "ip" when,
		// in fact, it does not exist in the CommsDat.  We should somehow be arranging for the
		// provisioning of this field to be from the underlying Agent, but the Agent is
		// only accessible from the Agent Adapter in the AgentSCPr layer.  Perhaps the
		// IPProtoSCPr can do the provisioning of the IfNetworks field, but it would need
		// to get the contents of the field from the AgentSCPr below it.
		_LIT(KVPNIfNetworks, "ip");
		protocolList = HBufC::NewMaxL(4);
		*protocolList = KVPNIfNetworks();
		}
	else
		{
		iapView->GetTableCommonTextFieldL(CCommsDatIapView::EIfNetworks, protocolList);
		}


	HBufC* configdaemonmanager = NULL;
	TRAPD(err, iapView->GetTableCommonTextFieldL(ESock::CCommsDatIapView::EConfigDaemonManagerName,
												  configdaemonmanager));
	if ((err == KErrNone) && (*configdaemonmanager != KNullDesC))
		{
		CleanupStack::PushL(configdaemonmanager);
		CNetCfgExtProvision* netcfg = CNetCfgExtProvision::NewL();
		CleanupStack::PushL(netcfg);
		netcfg->InitialiseConfigL(iapView);
		mec.AppendExtensionL(netcfg);
		CleanupStack::Pop(netcfg);
		CleanupStack::PopAndDestroy(configdaemonmanager);
		}

	CleanupStack::PopAndDestroy(iapView);
	CleanupStack::PushL(protocolList);

    TItfInfoConfigExt* itfInfoConfig = new (ELeave) TItfInfoConfigExt(TConnectionInfo(configurationIap,net));
    CleanupStack::PushL(itfInfoConfig);

	if (itfInfoConfig->iProtocolList.MaxSize() >= protocolList->Size())
		{
		itfInfoConfig->iProtocolList.Copy(*protocolList);
		}
	else
		{
		User::Leave(KErrOverflow);
		}

    mec.AppendExtensionL(itfInfoConfig);
    CleanupStack::Pop(itfInfoConfig);
    CleanupStack::PopAndDestroy(protocolList);

    XInterfaceNames* itfNames = XInterfaceNames::NewL();
    CleanupStack::PushL(itfNames);
    mec.AppendExtensionL(itfNames);
    CleanupStack::Pop(itfNames);

    iAccessPointConfig.Close();
	iAccessPointConfig.Open(mec);
	CleanupStack::PopAndDestroy(&mec);
    }

// -----------------------------------------------------------------------------
// TAwaitingControlClientJoinDuringStop::Accept
// -----------------------------------------------------------------------------
//
DEFINE_SMELEMENT( TAwaitingControlClientJoinDuringStop, NetStateMachine::MState, TContext )
TBool TAwaitingControlClientJoinDuringStop::Accept()
    {
    if ( iContext.iMessage.IsMessage<TCFPeer::TJoinRequest>() )
        {
        // Check if Stop activity is ongoing
        if ( iContext.Node().CountActivities( ECFActivityStop ) > 0 )
            {
            // We are stopping this IAP, so don't accept new connections
            // Accept message to trigger special handling, otherwise
            // let this flow through to core join activity
            return ETrue;
            }
        }
    return EFalse;
    }

// -----------------------------------------------------------------------------
// THandleNoBearerDuringGoneDownRecovery::DoL
// -----------------------------------------------------------------------------
//
DEFINE_SMELEMENT( TRespondErrorToControlClientJoin, NetStateMachine::MStateTransition, TContext )
void TRespondErrorToControlClientJoin::DoL()
    {
    iContext.Node().DecrementBlockingDestroy();
    // Single triple activity, so live with context info.
    TEBase::TError errorMsg ( iContext.iMessage.MessageId(), KErrNotReady );
    iContext.PostToSender( errorMsg );
    }
namespace IPProtoMCprSelectConnPrefListActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivitySelect, IPProtoMCprSelectConnPrefList, TCFSelector::TSelect, CSelectNextLayerActivity::NewL)
	//Reply from TAwaitingSelectNextLayer if no choices, otherwise accept
	FIRST_NODEACTIVITY_ENTRY(MCprStates::TAwaitingSelectNextLayerSuper, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TProcessSimpleSelectionPolicySuper, MCprStates::TSelectedProvider)
	//Start the selection main loop
	NODEACTIVITY_ENTRY(MCprStates::KSelectedProvider, CSelectNextLayerActivity::TFindOrCreateTierManagerSuper, MCprStates::TAwaitingTierManagerCreated, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TJoinTierManager, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
	//Select next provider and enter the selection internal loop if provider received. Break if SelectComplete(NULL).
	NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TSelectNextLayerSuper, MCprStates::TAwaitingSelectComplete, CSelectNextLayerActivity::TNoTagOrSelectedProviderIsNull)
    NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TAddProviderInfo, MCprStates::TAwaitingSelectComplete, CSelectNextLayerActivity::TNoTagBackwardsOrJoinServiceProvider)
    //Break the selection internal loop if SelectComplete(NULL), otherwise stay in this tripple
    NODEACTIVITY_ENTRY(MCprStates::KJoinServiceProvider, CSelectNextLayerActivity::TJoinServiceProvider, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
    THROUGH_NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TSendSelectComplete, CSelectNextLayerActivity::TSelectedProviderIsNullOrJoinServiceProviderBackward)
	//Break the selection main loop if no more choices, otherwise go back again
	THROUGH_NODEACTIVITY_ENTRY(MCprStates::KSelectedProviderIsNull, CSelectNextLayerActivity::TLeaveTierManager, CSelectNextLayerActivity::TNoTagOrSelectedProviderBackwardSuper)
	//Finish the activity
	LAST_NODEACTIVITY_ENTRY(KNoTag, MCprStates::TSendFinalSelectComplete)
NODEACTIVITY_END()
}

// An activity to prevent reusage of the IAP when it is being stopped
namespace IPProtoMCprJoinDuringStopActivity
{
// Waiting only join request, as we are handling a case where this
// node is already created and we want to prevent its reusage
DECLARE_DEFINE_NODEACTIVITY( ECFActivityNoBearer, IPProtoMCprJoinDuringStop, TCFPeer::TJoinRequest )
    SINGLE_NODEACTIVITY_ENTRY( IPProtoMCprStates::TRespondErrorToControlClientJoin, 
                               IPProtoMCprStates::TAwaitingControlClientJoinDuringStop )
NODEACTIVITY_END()
}
namespace IPProtoMCprActivities
{
DEFINE_ACTIVITY_MAP(ipProtoActivitiesMCpr)
	ACTIVITY_MAP_ENTRY(IPProtoMCprJoinDuringStopActivity, IPProtoMCprJoinDuringStop)
	ACTIVITY_MAP_ENTRY(IPProtoMCprSelectConnPrefListActivity, IPProtoMCprSelectConnPrefList)
ACTIVITY_MAP_END_BASE(MCprActivities, coreMCprActivities)
}
