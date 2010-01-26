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
// NETMCPR_ACTIVITIES.H
// 
//

#define SYMBIAN_NETWORKING_UPS

#include "netmcpr.h"
#include "netmcprstates.h"
#include "netmcpractivities.h"
#include "IPMessages.h"
#include <comms-infras/ss_nodemessages_serviceprovider.h>
#include <comms-infras/ss_nodemessages_selector.h>
#include <comms-infras/ss_nodemessages_factory.h>
#include <comms-infras/ss_nodemessages_tiermanagerfactory.h>
#include <comms-infras/coremcprstates.h>
#include <comms-infras/coremcpractivities.h>
#include <ss_glob.h>
#include <elements/nm_messages_errorrecovery.h>
#include <elements/nm_messages_child.h>

//#ifdef SYMBIAN_NETWORKING_UPS
#include "netmcprups_activities.h"
//#include <comms-infras/upsprstates.h>
//#include <comms-infras/upsmessages.h>
//using namespace UpsMCprActivities;
//using namespace NetMCprUpsActivities;
//#endif //SYMBIAN_NETWORKING_UPS

using namespace Messages;
using namespace MeshMachine;
using namespace NetStateMachine;
using namespace ESock;
using namespace MCprActivities;
using namespace NetMCprActivities;
using namespace NetMCprLegacyActivities;

namespace NetMCprDeferredSelectActivity
{
/** Deferred selection activity to support legacy user prompt behaviour.

	This activity forms an extension to the basic ConnectionStartRecovery activity (in Core).
	It starts before the basic ConnectionStartRecovery activity, remembers the original recovery
	request, and triggers the basic ConnectionStartRecovery activity after a successful completion.

	Along with ConnectionStartRecovery activity, it belongs to a group of Error Recovery Activities.
	Error Recovery Activities need to handle their own errors (generated as well as returned).

	The legacy selection may not be fully completed (i.e. certain service providers
	could have been skipped during selection). This activity completes the selection,
	potentially providing more connection choices after an unsuccessful attempt
	to start a connection (with previously selected choices).
*/
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityConnectionStartRecovery, MCprDeferredSelect, TEErrorRecovery::TErrorRecoveryRequest, CDeferredSelectionActivity::NewL)
	//Intercept the reconnection request if appropriate
	FIRST_NODEACTIVITY_ENTRY(CDeferredSelectionActivity::TAwaitingConnectionStartRecoveryRequest, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CDeferredSelectionActivity::TParkReConnectRequestAndFindOrCreateTierManager, CDeferredSelectionActivity::TState<MCprStates::TAwaitingTierManagerCreated>, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CDeferredSelectionActivity::TJoinTierManager, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CDeferredSelectionActivity::TCompleteDeferredSelection, CDeferredSelectionActivity::TState<MCprStates::TAwaitingSelectComplete>, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CDeferredSelectionActivity::TProcessSelectComplete, CDeferredSelectionActivity::TState<MCprStates::TAwaitingSelectComplete>, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CDeferredSelectionActivity::TJoinServiceProvider, CDeferredSelectionActivity::TState<CoreStates::TAwaitingJoinComplete>, MeshMachine::TNoTagOrErrorTag)
	//Now run the basic reconnection which will fail if our selection did not give us any more choices
	LAST_NODEACTIVITY_ENTRY(KNoTag, CDeferredSelectionActivity::TReDispatchReConnectRequest)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

namespace NetMcprPromptingReSelectActivity
{
/* This activity is started when there is a need to re-connect and the network
 * MCPr contains any information regarding to this. The behaviour of this activity
 * is almost the same as the DeferredSelection activity but it uses different
 * Extensions and different connection preferences. Basically this activity is used
 * if prompt dialog shold be invoked during re-selection and we have to use the
 * so called 399 selection instead of the legacy one.
 * 
 * NOTE: THIS ACTIVITY HAS TO BE REMOVED AS THIS KIND OF PROMTING FUNCTIONALITY IS NOT
 * ENOUGH GENERIC. THE SAME BEHAVIOUR SHOULD WE HAVE INDEPENDENTLY OF THE GIVEN SELECTION
 * MECHANISM AND SELECTORS... THERE WILL BE A DEFECT WHICH WILL COVER AND DESCRIBE THE PROBLEM.   
*/
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityConnectionStartRecovery, MCprPromptingReSelect, TEErrorRecovery::TErrorRecoveryRequest, CPromptingReSelectActivity::NewL)
	//Intercept the reconnection request if appropriate
	FIRST_NODEACTIVITY_ENTRY(CPromptingReSelectActivity::TAwaitingConnectionStartRecoveryRequest, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CPromptingReSelectActivity::TParkReConnectRequestAndFindOrCreateTierManager, CPromptingReSelectActivity::TState<MCprStates::TAwaitingTierManagerCreated>, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CPromptingReSelectActivity::TJoinTierManager, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CPromptingReSelectActivity::TCompletePromptingReSelection, CPromptingReSelectActivity::TState<MCprStates::TAwaitingSelectComplete>, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CPromptingReSelectActivity::TProcessSelectComplete, CPromptingReSelectActivity::TState<MCprStates::TAwaitingSelectComplete>, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CPromptingReSelectActivity::TJoinServiceProvider, CPromptingReSelectActivity::TState<CoreStates::TAwaitingJoinComplete>, MeshMachine::TNoTagOrErrorTag)
	//Now run the basic reconnection which will fail if our selection did not give us any more choices
	LAST_NODEACTIVITY_ENTRY(KNoTag, CPromptingReSelectActivity::TReDispatchReConnectRequest)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

#ifdef DUMMY_MOBILITY_MCPR
namespace NetMCprDummyMobilityActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityDummyMobilityActivity, MCprDummyMobility, TCFMessage::TMigrationAvailable)
	FIRST_NODEACTIVITY_ENTRY(NetMCprStates::TDummyAwaitingMigrationAvailable, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, NetMCprStates::TSendMigrationAvailable, NetMCprStates::TAwaitingMigrationRequestedOrMigrationRejected, NetMCprStates::TMigrationRequestedOrMigrationRejected)

	LAST_NODEACTIVITY_ENTRY(NetMCprStates::KMigrationRejected, MeshMachine::TDoNothing)

	NODEACTIVITY_ENTRY(NetMCprStates::KMigrationRequested, NetMCprStates::TSendMigrateToAccessPoint, NetMCprStates::TAwaitingMigrationRejectedOrMigrationAccepted, MeshMachine::TNoTag) // we dont actually care

	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}
#endif

namespace NetMCprProcessPolicyParamsActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityCustom, MCprProcessPolicyParams, TCFIPMessage::TPolicyParams)
	NODEACTIVITY_ENTRY(KNoTag, NetMCprStates::TProcessPolicyParams, NetMCprStates::TAwaitingPolicyParams, MeshMachine::TNoTag)
NODEACTIVITY_END()
}

namespace NetMCprActivities
{
DEFINE_ACTIVITY_MAP(netMCprActivities)
	ACTIVITY_MAP_ENTRY(NetMCprDeferredSelectActivity, MCprDeferredSelect)
	ACTIVITY_MAP_ENTRY(NetMcprPromptingReSelectActivity, MCprPromptingReSelect)
	ACTIVITY_MAP_ENTRY(NetMCprProcessPolicyParamsActivity, MCprProcessPolicyParams)
#ifdef SYMBIAN_NETWORKING_UPS
	ACTIVITY_MAP_ENTRY(NetMCprPolicyCheckRequestActivity, NetMCprPolicyCheckRequest)		 // UPS support
	ACTIVITY_MAP_ENTRY(NetMCprUpsNoBearerActivity, NetMCprUpsNoBearer)						 // UPS support
	ACTIVITY_MAP_ENTRY(NetMCprMonitorProviderStatusActivity, NetMCprUpsProviderStatusChange) // UPS support					
	ACTIVITY_MAP_ENTRY(NetMCprUpsStatusChangeActivity, NetMCprUpsStatusChange)				 // UPS support
	#endif
ACTIVITY_MAP_END_BASE(MobilityMCprActivities, mobilityMCprActivities)
}

//
//Re Connection - CDeferredSelectionActivity
DEFINE_SMELEMENT(CDeferredSelectionActivity::TAwaitingConnectionStartRecoveryRequest, NetStateMachine::MState, CDeferredSelectionActivity::TContext)
TBool CDeferredSelectionActivity::TAwaitingConnectionStartRecoveryRequest::Accept()
	{
	//If this is a reconnect request plus we can obtain some more options from the deferred selection, start this activity.
	//If we can not obtan any more choices do not bother with starting, go straight to the standard reconnection.
	if (MCprStates::TAwaitingConnectionStartRecoveryRequest::Accept()
		&& iContext.Node().AccessPointConfig().FindExtension(TDeferredSelectionPrefsExt::TypeId()))
		{
		ASSERT(iContext.Node().ProviderInfo().Instance()); //We only support deferred selection for legacy providers on this layer
		return ETrue;
		}
	return EFalse;
	}

DEFINE_SMELEMENT(CDeferredSelectionActivity::TParkReConnectRequestAndFindOrCreateTierManager, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext)
void CDeferredSelectionActivity::TParkReConnectRequestAndFindOrCreateTierManager::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CDeferredSelectionActivity& activity = static_cast<CDeferredSelectionActivity&>(*iContext.iNodeActivity);
	activity.iOriginalErrContext = message_cast<TEErrorRecovery::TErrorRecoveryRequest>(iContext.iMessage).iErrContext;
	activity.ParkReConnectRequestL(iContext);

	//Not leaving getter plus asserted
	const TDeferredSelectionPrefsExt* ext = static_cast<const TDeferredSelectionPrefsExt*>(iContext.Node().AccessPointConfig().FindExtension(TDeferredSelectionPrefsExt::TypeId()));
	ASSERT(ext);
	ASSERT(ext->iTierId.iUid!=0);
	TAlwaysFindFactoryQuery query;
	iContext.iNodeActivity->PostRequestTo(SockManGlobals::Get()->GetPlaneFC(TCFPlayerRole(TCFPlayerRole::ETierMgrPlane)),
		TCFFactory::TFindOrCreatePeer(TCFPlayerRole::ETierMgrPlane, ext->iTierId, &query).CRef());
	}

DEFINE_SMELEMENT(CDeferredSelectionActivity::TCompleteDeferredSelection, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext)
void CDeferredSelectionActivity::TCompleteDeferredSelection::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CDeferredSelectionActivity& ac = static_cast<CDeferredSelectionActivity&>(*iContext.iNodeActivity);

	const TDeferredSelectionPrefsExt& ext = static_cast<const TDeferredSelectionPrefsExt&>(iContext.Node().AccessPointConfig().FindExtensionL(
	        TDeferredSelectionPrefsExt::TypeId()));
	ac.PostRequestTo(ac.iTierManager, TCFSelector::TSimpleSelect(TSelectionPrefs(ext.iPrefs)).CRef());
	}

DEFINE_SMELEMENT(CDeferredSelectionActivity::TProcessSelectComplete, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext)
void CDeferredSelectionActivity::TProcessSelectComplete::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CDeferredSelectionActivity& activity = static_cast<CDeferredSelectionActivity&>(*iContext.iNodeActivity);
	activity.iSelectedMcpr = message_cast<TCFSelector::TSelectComplete>(iContext.iMessage).iNodeId;

	//The provider must be valid and different from what we already have
	if (iContext.Node().FindClient(activity.iSelectedMcpr))
		{
		iContext.iNodeActivity->SetError(
		    static_cast<CDeferredSelectionActivity*>(iContext.iNodeActivity)->iOriginalErrContext.iStateChange.iError);
		}
	else if (activity.iSelectedMcpr.IsNull())
    	{
    	User::Leave(
    	    static_cast<CDeferredSelectionActivity*>(iContext.iNodeActivity)->iOriginalErrContext.iStateChange.iError);
    	}
	}

DEFINE_SMELEMENT(CDeferredSelectionActivity::TJoinServiceProvider, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext)
void CDeferredSelectionActivity::TJoinServiceProvider::DoL()
	{
	//Final select complete
	ASSERT(message_cast<TCFSelector::TSelectComplete>(iContext.iMessage).iNodeId.IsNull());

	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CDeferredSelectionActivity& activity = static_cast<CDeferredSelectionActivity&>(*iContext.iNodeActivity);

	//Join the new service provider
	RNodeInterface* client = iContext.Node().AddClientL(activity.iSelectedMcpr, TClientType(TCFClientType::EServProvider));

	//Join the selected provider as a control client, send select complete message.
	//There is no need to remember the channel (SetSentTo()) because we do not expect any answer.
	activity.PostRequestTo(*client,
		TCFServiceProvider::TJoinRequest(iContext.NodeId(), TCFClientType::ECtrl).CRef());
	}

DEFINE_SMELEMENT(CDeferredSelectionActivity::TReDispatchReConnectRequest, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext)
void CDeferredSelectionActivity::TReDispatchReConnectRequest::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CDeferredSelectionActivity& activity = static_cast<CDeferredSelectionActivity&>(*iContext.iNodeActivity);
	activity.ReDispatchReConnectRequestL(iContext);
	}

DEFINE_SMELEMENT(CDeferredSelectionActivity::TJoinTierManager, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext)
void CDeferredSelectionActivity::TJoinTierManager::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CDeferredSelectionActivity& ac = static_cast<CDeferredSelectionActivity&>(*iContext.iNodeActivity);
	ac.iTierManager = message_cast<TCFFactory::TPeerFoundOrCreated>(iContext.iMessage).iNodeId;
    ASSERT(!ac.iTierManager.IsNull()); //Must always be valid.
    ac.PostRequestTo(ac.iTierManager, TCFPeer::TJoinRequest(iContext.NodeId(), TClientType(TCFClientType::ECtrl)).CRef());
	}

MeshMachine::CNodeActivityBase* CDeferredSelectionActivity::NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
    {
    return new (ELeave) CDeferredSelectionActivity(aActivitySig, aNode);
    }

CDeferredSelectionActivity::CDeferredSelectionActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
:	MeshMachine::CNodeActivityBase(aActivitySig, aNode)
	{
	}

CDeferredSelectionActivity::~CDeferredSelectionActivity()
	{
	if (!iTierManager.IsNull())
		{
		RClientInterface::OpenPostMessageClose(iNode.Id(), iTierManager, TEChild::TLeft().CRef());
		iTierManager.SetNull();
		}
	}

void CDeferredSelectionActivity::ParkReConnectRequestL(const TNodeContextBase& aContext)
	{
	User::LeaveIfError(StoreContext(aContext));
	}

void CDeferredSelectionActivity::ReDispatchReConnectRequestL(const TNodeContextBase& aContext)
	{
	TBuf8<__Align8(sizeof(TNodeContextBase))> ctxBuf;
	TBuf8<__Align8(TSignalBase::KMaxInlineMessageSize + TSignalBase::KMaxUnstoredOverhead)> msgBuf;
	TNodeCtxId dummy;

	TNodeContextBase* storedContext = LoadContext(aContext.iNode, aContext.iNodeActivity, ctxBuf, msgBuf, dummy);

	//We should never be here if parking of the original request failed!
	__ASSERT_ALWAYS(storedContext, User::Panic(KNetMCprPanic, KPanicNoContext));
	PostToOriginators(storedContext->iMessage);
	iContextDesc.Zero();
	}

void CDeferredSelectionActivity::ReplyToOriginators(TEErrorRecovery::TErrorRecoveryResponse& aCFMessageSig)
	{
//TODO[PROD] - logging
   	//MESH_LOG_MESSAGE(KESockComponentTag, KESockMeshMachine, aCFMessageSig, this,_S8("CConnectionRecoveryActivity:\tPostToOriginators"));
	for (TInt n = iOriginators.Count() - 1;n>=0; n--)
		{
		Messages::TNodePeerId& peerId = iOriginators[n];
		//aCFMessageSig.SetActivity(peerId.iActivityId);
		TCFSafeMessage::TResponseCarrierWest<TEErrorRecovery::TErrorRecoveryResponse> resp(aCFMessageSig, peerId.Peer().RecipientId());
		PostToOriginator(peerId, resp);
		}
	}


//
//Re Connection - CPromptingReSelectActivity
DEFINE_SMELEMENT(CPromptingReSelectActivity::TAwaitingConnectionStartRecoveryRequest, NetStateMachine::MState, CPromptingReSelectActivity::TContext)
TBool CPromptingReSelectActivity::TAwaitingConnectionStartRecoveryRequest::Accept()
	{
	//If this is a reconnect request plus we can obtain some more options from the deferred selection, start this activity.
	//If we can not obtan any more choices do not bother with starting, go straight to the standard reconnection.
	if (MCprStates::TAwaitingConnectionStartRecoveryRequest::Accept())
	    {
	    const TPromptingSelectionPrefsExt* ext = static_cast<const TPromptingSelectionPrefsExt*>(
	            iContext.Node().AccessPointConfig().FindExtension(TPromptingSelectionPrefsExt::TypeId()));
		if (ext)
            {
    		/* it possible that the extension contains a list with 0 element in it. The reason for that:
    		 * As the 'CIpProtoProviderSelector' selector receives one-by-one the preferences not as a list
    		 * it cannot know when the selection on that given layer will be finished. So when there is a 
    		 * prompting AP the RunL of that selector will append an extension to the Network MCPr, indicating
    		 * that there was already a prompt, with an empty list in that extension. However if the given 
    		 * preference was the last one in the list which needed to prompt the RunL will append the given
    		 * extension to the Network MCPr with an empty list just to indicate that the prompting has happened
    		 * so no other dialog should be invoked. So during re-selection there will be an extension appended 
    		 * to the Network MCPr with an empty list. Here is the code which checks this situation and acts
    		 * according to the situation.
    		 * 
    		 * NOTE: this whole activity has to be removed and the prompting logic should be re-worked in every
    		 * IPProto level selector by _NOT_ using the Network level MCPR at all!!!!!!! There will be a defect for
    		 * this problem!!!
    		 * 
    		 */
            const TConnPref& pref = ext->iPrefs;
            const TConnIdList& list = static_cast<const TConnIdList&>(pref);
		
    		if (list.Count() == 1)
    			{
    			return ETrue;
    			}
    		else
    			{
    			//remove the empty list and return with EFalse so the default errorhandling will take place...
    			const_cast<TPromptingSelectionPrefsExt*>(ext)->iPromptingInProgress = EFalse;
    			}
            }
	    }
	return EFalse;
	}

DEFINE_SMELEMENT(CPromptingReSelectActivity::TParkReConnectRequestAndFindOrCreateTierManager, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext)
void CPromptingReSelectActivity::TParkReConnectRequestAndFindOrCreateTierManager::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CPromptingReSelectActivity& activity = static_cast<CPromptingReSelectActivity&>(*iContext.iNodeActivity);
	activity.iOriginalErrContext = message_cast<TEErrorRecovery::TErrorRecoveryRequest>(iContext.iMessage).iErrContext;
	activity.ParkReConnectRequestL(iContext);

	//Not leaving getter plus asserted
	const TPromptingSelectionPrefsExt* ext = static_cast<const TPromptingSelectionPrefsExt*>(iContext.Node().AccessPointConfig().FindExtension(TPromptingSelectionPrefsExt::TypeId()));
	ASSERT(ext);
	ASSERT(ext->iTierId.iUid!=0);
	TAlwaysFindFactoryQuery query;
	iContext.iNodeActivity->PostRequestTo(SockManGlobals::Get()->GetPlaneFC(TCFPlayerRole(TCFPlayerRole::ETierMgrPlane)),
		TCFFactory::TFindOrCreatePeer(TCFPlayerRole::ETierMgrPlane, ext->iTierId, &query).CRef());
	}

DEFINE_SMELEMENT(CPromptingReSelectActivity::TCompletePromptingReSelection, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext)
void CPromptingReSelectActivity::TCompletePromptingReSelection::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CPromptingReSelectActivity& ac = static_cast<CPromptingReSelectActivity&>(*iContext.iNodeActivity);

	const TPromptingSelectionPrefsExt& ext = static_cast<const TPromptingSelectionPrefsExt&>(iContext.Node().AccessPointConfig().FindExtensionL(
	        TPromptingSelectionPrefsExt::TypeId()));
	ac.PostRequestTo(ac.iTierManager, TCFSelector::TSimpleSelect(TSelectionPrefs(ext.iPrefs)).CRef());
	}

DEFINE_SMELEMENT(CPromptingReSelectActivity::TProcessSelectComplete, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext)
void CPromptingReSelectActivity::TProcessSelectComplete::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CPromptingReSelectActivity& activity = static_cast<CPromptingReSelectActivity&>(*iContext.iNodeActivity);
	activity.iSelectedMcpr = message_cast<TCFSelector::TSelectComplete>(iContext.iMessage).iNodeId;

	//The provider must be valid and different from what we already have
	if (iContext.Node().FindClient(activity.iSelectedMcpr))
		{
		iContext.iNodeActivity->SetError(
		    static_cast<CPromptingReSelectActivity*>(iContext.iNodeActivity)->iOriginalErrContext.iStateChange.iError);
		}
	else if (activity.iSelectedMcpr.IsNull())
    	{
    	User::Leave(
    	    static_cast<CPromptingReSelectActivity*>(iContext.iNodeActivity)->iOriginalErrContext.iStateChange.iError);
    	}
	}

DEFINE_SMELEMENT(CPromptingReSelectActivity::TJoinServiceProvider, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext)
void CPromptingReSelectActivity::TJoinServiceProvider::DoL()
	{
	//Final select complete
	ASSERT(message_cast<TCFSelector::TSelectComplete>(iContext.iMessage).iNodeId.IsNull());

	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CPromptingReSelectActivity& activity = static_cast<CPromptingReSelectActivity&>(*iContext.iNodeActivity);

	//Join the new service provider
	RNodeInterface* client = iContext.Node().AddClientL(activity.iSelectedMcpr, TClientType(TCFClientType::EServProvider));

	//Join the selected provider as a control client, send select complete message.
	//There is no need to remember the channel (SetSentTo()) because we do not expect any answer.
	activity.PostRequestTo(*client,
		TCFServiceProvider::TJoinRequest(iContext.NodeId(), TCFClientType::ECtrl).CRef());
	}

DEFINE_SMELEMENT(CPromptingReSelectActivity::TReDispatchReConnectRequest, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext)
void CPromptingReSelectActivity::TReDispatchReConnectRequest::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CPromptingReSelectActivity& activity = static_cast<CPromptingReSelectActivity&>(*iContext.iNodeActivity);
	activity.ReDispatchReConnectRequestL(iContext);
	}

DEFINE_SMELEMENT(CPromptingReSelectActivity::TJoinTierManager, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext)
void CPromptingReSelectActivity::TJoinTierManager::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KNetMCprPanic, KPanicNoActivity));
	CPromptingReSelectActivity& ac = static_cast<CPromptingReSelectActivity&>(*iContext.iNodeActivity);
	ac.iTierManager = message_cast<TCFFactory::TPeerFoundOrCreated>(iContext.iMessage).iNodeId;
    ASSERT(!ac.iTierManager.IsNull()); //Must always be valid.
    ac.PostRequestTo(ac.iTierManager, TCFPeer::TJoinRequest(iContext.NodeId(), TClientType(TCFClientType::ECtrl)).CRef());
	}

MeshMachine::CNodeActivityBase* CPromptingReSelectActivity::NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
    {
    return new (ELeave) CPromptingReSelectActivity(aActivitySig, aNode);
    }

CPromptingReSelectActivity::CPromptingReSelectActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
:	MeshMachine::CNodeActivityBase(aActivitySig, aNode)
	{
	}

CPromptingReSelectActivity::~CPromptingReSelectActivity()
	{
	if (!iTierManager.IsNull())
		{
		RClientInterface::OpenPostMessageClose(iNode.Id(), iTierManager, TEChild::TLeft().CRef());
		iTierManager.SetNull();
		}
	}

void CPromptingReSelectActivity::ParkReConnectRequestL(const TNodeContextBase& aContext)
	{
	User::LeaveIfError(StoreContext(aContext));
	}

void CPromptingReSelectActivity::ReDispatchReConnectRequestL(const TNodeContextBase& aContext)
	{
	TBuf8<__Align8(sizeof(TNodeContextBase))> ctxBuf;
	TBuf8<__Align8(TSignalBase::KMaxInlineMessageSize + TSignalBase::KMaxUnstoredOverhead)> msgBuf;
	TNodeCtxId dummy;
	TNodeContextBase* storedContext = LoadContext(aContext.iNode, aContext.iNodeActivity, ctxBuf, msgBuf, dummy);

	//We should never be here if parking of the original request failed!
	__ASSERT_ALWAYS(storedContext, User::Panic(KNetMCprPanic, KPanicNoContext));
	PostToOriginators(storedContext->iMessage);
	iContextDesc.Zero();
	}

void CPromptingReSelectActivity::ReplyToOriginators(TEErrorRecovery::TErrorRecoveryResponse& aCFMessageSig)
	{
//TODO[PROD] - logging
   	//MESH_LOG_MESSAGE(KESockComponentTag, KESockMeshMachine, aCFMessageSig, this,_S8("CConnectionRecoveryActivity:\tPostToOriginators"));
	for (TInt n = iOriginators.Count() - 1;n>=0; n--)
		{
		Messages::TNodePeerId& peerId = iOriginators[n];
		//aCFMessageSig.SetActivity(peerId.iActivityId);
		TCFSafeMessage::TResponseCarrierWest<TEErrorRecovery::TErrorRecoveryResponse> resp(aCFMessageSig, peerId.Peer().RecipientId());
		PostToOriginator(peerId, resp);
		}
	}


