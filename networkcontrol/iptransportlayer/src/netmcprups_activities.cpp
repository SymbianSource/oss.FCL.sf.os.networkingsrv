// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include "netmcprups.h"
#include "netmcpractivities.h"
#include "netmcprups_activities.h"
#include <comms-infras/upsprstates.h>
#include <comms-infras/upsmessages.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace UpsMCprActivities;
using namespace NetMCprActivities;
using namespace NetMCprUpsActivities;

namespace NetMCprPolicyCheckRequestActivity
{
/**
Process TPolicyCheckRequest messages.

Issue a UPS authorisation request towards the NetUps and send back a TPolicyCheckResponse message
with the result.  This activity assumes that Networking usage of UPS has not been "short-circuited" off
(the originator should not have sent the TPolicyCheckRequest in the first place if so).
*/
DEFINE_CUSTOM_NODEACTIVITY(ECFActivityPolicyCheckRequest, NetMCprPolicyCheckRequest, UpsMessage::TPolicyCheckRequest, CUpsAuthorisationActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(UpsStates::TAwaitingPolicyCheckRequest, CUpsAuthorisationActivityBase::TNoTagOrCompletion)

	NODEACTIVITY_ENTRY(KNoTag, CUpsAuthorisationActivityBase::TIssueRequestToNetUps, CUpsAuthorisationActivityBase::TAwaitingPolicyCheckResponseOrCancel, TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CUpsAuthorisationActivityBase::TForwardPolicyCheckResponseToOriginators)
	LAST_NODEACTIVITY_ENTRY(CUpsAuthorisationActivityBase::KUpsCompletionTag, CUpsAuthorisationActivityBase::TForwardCompletionPolicyCheckResponseToOriginators)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, TDoNothing)	// the activity error member has already been set in CUpsAuthorisationActivityBase::TNoTagOrError
NODEACTIVITY_END()
}

namespace NetMCprUpsNoBearerActivity
{
/**
Process TNoBearer messages.

Issue a UPS authorisation request towards the NetUps and, if successful, continue with the core NoBearer
activity (MCprNoBearer).

This activity (NetMCprNoBearer) implements UPS authorisation for the NoBearer activity.  NetMCprNoBearer
performs the UPS authorisation and, if authorisation is successful, invokes the existing MCprNoBearer
activity to continue with the NoBearer process.  In effect, NetMCprNoBearer is "layered over"
the MCprNoBearer activity:

Originator ("west") <---> NetMCprNoBearer <---> MCprNoBearer <---> Control Provider ("east")

- Originator:		Send TNoBearer
- NetMCprNoBearer:	Accept TNoBearer from originator (i.e. not sent from self)
- NetMCprNoBearer:	Issue a UPS authorisation request to NetUps.
- NetMCprNoBearer:	Accept a TPolicyCheckResponse message from self (generated when the NetUps request succeeds)

- NetMCprNoBearer:	Post a TNoBearer message to self (destined for MCprNoBearer)
- MCprNoBearer:		Accept TNoBearer message

- MCprNoBearer:		Send TBindTo back to NetMCprNoBearer
- NetMCprNoBearer:	Accept TBindTo
- NetMCprNoBearer:	Send TBindTo to originator

- Originator:		Send TBindToComplete
- NetMCprNoBearer:	Accept TBindToComplete from originator
- NetMCprNoBearer:	Send TBindToComplete to MCprNoBearer
- NetMCprNoBearer:	Ends

Cancellation policy: incoming TCancel messages cause cancellation of any outstanding UPS request.
Once we have passed the UPS request phase, then TCancel messages set the activity error and
are consumed.  Rationale is that UPS requests can take an indefinate time to process, so must
be cancellable.  Consuming TCancel after UPS authorisation is just retaining the existing behaviour of
original NoBearer activity.
*/

DEFINE_CUSTOM_NODEACTIVITY(ECFActivityUpsNoBearer, NetMCprUpsNoBearer, TCFControlProvider::TNoBearer, CUpsNoBearer::NewL)
	// TNoTagOrInvokeUps checks whether Networking use of UPS has been "short-circuited" off or not.
	FIRST_NODEACTIVITY_ENTRY(NetMCprStates::TAwaitingNoBearerNotFromSelf, CUpsNoBearer::TNoTagOrInvokeUps)
	NODEACTIVITY_ENTRY(CUpsNoBearer::KInvokeUps, CUpsAuthorisationActivityBase::TIssueRequestToNetUps, CUpsAuthorisationActivityBase::TAwaitingPolicyCheckResponseOrCancel, CUpsAuthorisationActivityBase::TNoTagOrError)
	NODEACTIVITY_ENTRY(KNoTag, NetMCprStates::TSendNoBearerToSelf, CoreNetStates::TAwaitingBindToOrCancel, TNoTag)
	// @TODO 1116 check this out 
	NODEACTIVITY_ENTRY(KNoTag, CUpsNoBearer::TForwardBindToToOriginators, NetMCprStates::TAwaitingBindToCompleteOrCancel, TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CUpsNoBearer::TForwardBindToCompleteToSelf, MeshMachine::TAwaitingMessageState<TCFControlProvider::TBearer>, TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CUpsNoBearer::TForwardBearerToOriginators) 
	LAST_NODEACTIVITY_ENTRY(KErrorTag, TDoNothing)	// the activity error member has already been set in CUpsAuthorisationActivityBase::TNoTagOrError
NODEACTIVITY_END()
}

// Connection Status Notification

namespace NetMCprMonitorProviderStatusActivity
{
DEFINE_NODEACTIVITY(ECFActivityUpsProviderStatus, NetMCprUpsProviderStatusChange, TCFControlProvider::TDataClientStatusChange)
	NODEACTIVITY_ENTRY(KNoTag, NetMCprUpsStates::TUpsProcessProviderStatusChange, CoreNetStates::TAwaitingDataClientStatusChange, TNoTag)
NODEACTIVITY_END()
}

// Handles a notification that an IPCPR control client which originated a UPS request has left the IPCPR 
namespace NetMCprUpsStatusChangeActivity
{
DEFINE_NODEACTIVITY(ECFActivityUpsStatusChange, NetMCprUpsStatusChange, UpsMessage::TUPSStatusChange)
	NODEACTIVITY_ENTRY(KNoTag, NetMCprUpsStates::TProcessUpsStatusChange, NetMCprStates::TAwaitingUPSStatusChange, TNoTag)
NODEACTIVITY_END()
}

//
// Support for UPS
//

//
// CUpsAuthorisationActivity
//

MeshMachine::CNodeActivityBase* CUpsAuthorisationActivity::NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
	{
	TUint c = GetNextActivityCountL(aActivitySig, aNode);

	return new (ELeave) CUpsAuthorisationActivity(aActivitySig, aNode, c);
	}

CUpsAuthorisationActivity::CUpsAuthorisationActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount)
  : CUpsAuthorisationActivityBase(aActivitySig, aNode, aNextActivityCount)
	{
	}

TInt CUpsAuthorisationActivity::GetUpsDestinationString(TUpsDestinationAddrType aDestinationType, const TUpsDestinationAddr& aDestination, TDes& aOutputString)
/**
Form and return the UPS destination address string to be sent to the NetUps on each request.
*/
	{
	const TInt KMinOutputAddressLen = 55; 	// enough for IPv6 address with 32-bit decimal scope: "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx%nnnnnnnnnn"

	// Validate that the TUpsDestinationAddr is a TInetAddr.

	if (aDestinationType == ETSockAddress && aDestination.MaxLength() >= KMaxSockAddrSize && aOutputString.MaxLength() >= KMinOutputAddressLen)
		{
		const TSockAddr* addr = static_cast<const TSockAddr*>(&aDestination);
		TInetAddr inAddr(*addr);
		inAddr.Output(aOutputString);
		}
	else
		{
		aOutputString = KNullDesC();
		}
	return KErrNone;
	}

const TDesC& CUpsAuthorisationActivity::GetUpsAccessPointNameL()
	{
	CUpsNetworkMetaConnectionProvider& node = static_cast<CUpsNetworkMetaConnectionProvider&>(iNode);
	return node.ApName();
	}

NetUps::CNetUps* CUpsAuthorisationActivity::GetNetUpsL(TInt32 aServiceId)
	{
	// WINSCW will not compile the variable definition "NetUps::CNetUps* netUps;" in this function,
	// despite accepting "NetUps::CNetUps*" above as the return value for the function.  ARMv5 has
	// no problems.  Workaround by importing NetUps namespace.
	using namespace NetUps;

	CUpsNetworkMetaConnectionProvider& node = static_cast<CUpsNetworkMetaConnectionProvider&>(iNode);
	CNetUps* netUps = node.NetUps();
	if (netUps == NULL)
		{
		netUps = NetUps::CNetUps::NewL(aServiceId);
		node.SetNetUps(netUps);
		}
	return netUps;
	}

void CUpsAuthorisationActivity::PerformPolicyCheckResponseActions(NetUps::TNetUpsDecision aNetUpsDecision, const Messages::TNodeId& aCommsId)
	{	
	using namespace NetUps;
	
	if ((aNetUpsDecision == EYes || aNetUpsDecision == ESessionYes))
		{
		CUpsNetworkMetaConnectionProvider& node = static_cast<CUpsNetworkMetaConnectionProvider&>(iNode);
		node.IncrementUpsClientHandle(aCommsId);
		node.SetProviderStatusDown(EFalse);
		}
	}


void CUpsAuthorisationActivity::PerformPolicyCheckRequestActionsL(const Messages::TNodeId& aCommsId)
	{
	/**
	Cache the node id which represents the UPS Client:
	  For implicit sockets the MCPRs Node is used to represent the client.
	  For explicit connections, the node id of the CConnection which corresponds
	   with the client RConnection is used to represent the client.
	   
	This method is called prior to the MCPR posting a PolicyCheckRequest to the NetUps.   
	*/
	using namespace NetUps;
	
	CUpsNetworkMetaConnectionProvider& node = static_cast<CUpsNetworkMetaConnectionProvider&>(iNode);

	node.AddUpsClientCommsIdL(aCommsId);
	
	if (aCommsId != node.Id())
		{
		node.SetUpsControlClientPresent();
		}
	}

//
// CUpsNoBearer
//

MeshMachine::CNodeActivityBase* CUpsNoBearer::NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
	{
	TUint c = GetNextActivityCountL(aActivitySig, aNode);

	return new (ELeave) CUpsNoBearer(aActivitySig, aNode, c);
	}

CUpsNoBearer::CUpsNoBearer(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount)
  : CUpsAuthorisationActivity(aActivitySig, aNode, aNextActivityCount)
	{
	}

//
// Transitions
//

DEFINE_NETMCPR_TRANSITION(CUpsNoBearer, TForwardBindToToOriginators)
void CUpsNoBearer::TForwardBindToToOriginators::DoL()
/**
Forward a TBindTo received from the core MCprNoBearer activity on to our originators.
*/
    {
	NetMCprUpsActivities::CUpsNoBearer* const activity = static_cast<NetMCprUpsActivities::CUpsNoBearer*>(iContext.Activity());

	// store the activity id to which we need to send the subsequent TBindToComplete
	const TNodeCtxId& sender = address_cast<const TNodeCtxId>(iContext.iSender);
  	activity->SetBindToOriginatorActivityId(sender.NodeCtx());

	// Send TBindTo to originators.  PostToOriginators() will use current activity id and node id
	// as sender address, so we can re-use the current message without re-constructing a clone.
	// TODO why did this not work? activity->PostToOriginators(iContext.iMessage);
	activity->PostRequestTo(
				address_cast<Messages::TNodeId>(activity->FirstOriginator().RecipientId()),
				iContext.iMessage);
    }

DEFINE_NETMCPR_TRANSITION(CUpsNoBearer, TForwardBearerToOriginators)
void CUpsNoBearer::TForwardBearerToOriginators::DoL()
/**
Forward a TBearer received from the core MCprNoBearer activity on to our originators.
*/
    {
  	NetMCprUpsActivities::CUpsNoBearer* const activity = static_cast<NetMCprUpsActivities::CUpsNoBearer*>(iContext.Activity());

	// Send TBearer to originators.  PostToOriginators() will use current activity id and node id
	// as sender address, so we can re-use the current message without re-constructing a clone.
	activity->PostToOriginators(iContext.iMessage);
    activity->SetPostedTo(TNodeId::NullId());
    	
    }

DEFINE_NETMCPR_TRANSITION(CUpsNoBearer, TForwardBindToCompleteToSelf)
void CUpsNoBearer::TForwardBindToCompleteToSelf::DoL()
/**
Forward a TBindToComplete from our originators on to the core MCprNoBearer activity.
*/
    {
	const TNodeId& self = iContext.Node().Id();
	iContext.Activity()->PostRequestTo(self, iContext.iMessage);
    }

//
// State Forks
//

DEFINE_NETMCPR_STATEFORK(CUpsNoBearer, TNoTagOrInvokeUps)
TInt CUpsNoBearer::TNoTagOrInvokeUps::TransitionTag()
/**
Determine whether Networking usage of UPS functionality is "short-circuited" off or not.

@TODO PREQ1116 - should this be in coreMCpr ?
*/
	{
	const CUpsNetworkMetaConnectionProvider& node = iContext.Node();

	TInt tag = KNoTag;	
	if (!node.UpsDisabled())		// Note: UpsDisabled() is an optimisation - result of FindExtension() could also be used.
		{
		const Meta::SMetaData* const extension = 
			iContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CUPSAccessPointConfigExt::EUPSAccessPointConfigUid, CUPSAccessPointConfigExt::ETypeId));

		if (extension)
			{
			const CUPSAccessPointConfigExt* const upsExt = static_cast<const CUPSAccessPointConfigExt*>(extension);
			// KErrCompletion is a special error code which indicates that we should not perform
			// any UPS checking, as this will be performed by a higher layer.
			if  (upsExt->GetPolicyCheckResult() != KErrCompletion)
				{
				tag = CUpsNoBearer::KInvokeUps;
				}
			}
		}
	return tag;
	}

#endif //SYMBIAN_NETWORKING_UPS
