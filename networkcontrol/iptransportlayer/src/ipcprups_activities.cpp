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
// IP Connection Provider activity definitions specific to User Prompt Service (UPS).
// 
//

/**
 @file
 @internalComponent
*/

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include "ipcprups_activities.h"
#include <comms-infras/upsprstates.h>
#include <comms-infras/upsmessages.h>
#include <comms-infras/corecpractivities.h>
#include "ipcprups_states.h"

using namespace MeshMachine;
using namespace Messages;
using namespace UpsCprActivities;
using namespace ESock;
using namespace NetStateMachine;
using namespace IpCprActivities;
using namespace CprClientLeaveActivity;

namespace IpCprActivities
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityClientJoin, IpCprControlClientJoin, TNodeSignal::TNullMessageId, CIpCprCtrlClientJoinActivity::NewL) //May be waiting for both messages
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingControlClientJoin, IpCprStates::TNoTagOrUpsQueryOrAttachCapabilityQuery)

	// Non-UPS path
	THROUGH_NODEACTIVITY_ENTRY(IpCprStates::KAttachCapabilityQuery, IpCprStates::TCheckAttachCapabilities, TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TAddControlClientAndSendJoinCompleteIfRequest, TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendDataClientActive)

	// UPS path.  Here if message was TCtrlClientJoinRequest and UPS Access Point Config extension was found.
	THROUGH_NODEACTIVITY_ENTRY(IpCprStates::KUpsQuery, CDeferredCtrlClientJoinActivity::TStoreControlClient, TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CIpCprCtrlClientJoinActivity::TSendPolicyCheckRequest,
							UpsStates::TAwaitingPolicyCheckResponse, CIpCprCtrlClientJoinActivity::TNoTagOrUpsErrorTag)
	// UPS success.  A TPolicyCheckResponse was received with authorisation granted.
	LAST_NODEACTIVITY_ENTRY(KNoTag, CDeferredCtrlClientJoinActivity::TAddControlClientAndSendJoinComplete)

	// UPS error.  A TPolicyCheckResponse was received with authorisation denied.
	LAST_NODEACTIVITY_ENTRY(CIpCprCtrlClientJoinActivity::KUpsErrorTag, TDoNothing)
NODEACTIVITY_END()

DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityPolicyCheckRequest, IpCprPolicyCheckRequest, UpsMessage::TPolicyCheckRequest, CNodeParallelActivityBase::NewL)
	// TODO: decide whether we should move some of these transitions/forks to the UpsCoreProviders
	FIRST_NODEACTIVITY_ENTRY(UpsStates::TAwaitingPolicyCheckRequest, IpCprStates::TNoTagOrUpsDisabledOrUpsEnabled)
	LAST_NODEACTIVITY_ENTRY(IpCprStates::KUpsDisabled, IpCprStates::TPostDisabledPolicyCheckResponseToOriginators)

	THROUGH_NODEACTIVITY_ENTRY(IpCprStates::KUpsEnabled, IpCprStates::TPopulatePolicyCheckRequest, TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, UpsStates::TPostToControlProvider, UpsStates::TAwaitingPolicyCheckResponse, TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreStates::TPostToOriginators)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, TDoNothing)
NODEACTIVITY_END()

DECLARE_DEFINE_RESERVED_CUSTOM_NODEACTIVITY(ECFActivityClientLeave, IpCprClientLeave, Messages::TEPeer::TLeaveRequest, CClientLeaveActivity::New)
	FIRST_NODEACTIVITY_ENTRY(MeshMachine::TAwaitingMessageState<TEPeer::TLeaveRequest>, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendUpsStatusChange, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CClientLeaveActivity::TRemoveClientAndDestroyOrphanedDataClients, CClientLeaveActivity::TNoTagOrSendPriorityToCtrlProvider)
	NODEACTIVITY_ENTRY(CprStates::KSendPriorityToCtrlProvider, CClientLeaveActivity::TUpdatePriorityForControlProvider, CoreStates::TAwaitingJoinComplete, CClientLeaveActivity::TNoTagOrSendPriorityToServProvider)
	NODEACTIVITY_ENTRY(CprStates::KSendPriorityToServProvider, CClientLeaveActivity::TUpdatePriorityForServiceProviders, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CClientLeaveActivity::TSendLeaveCompleteAndSendDataClientIdleIfNeeded, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CprStates::TSendDataClientStatusStoppedIfNoControlClient)
NODEACTIVITY_END()
}

//
// CIpCprCtrlClientJoinActivity
//

MeshMachine::CNodeActivityBase* CIpCprCtrlClientJoinActivity::NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
	{
	TUint c = GetNextActivityCountL(aActivitySig, aNode);

	return new (ELeave) CIpCprCtrlClientJoinActivity(aActivitySig, aNode, c);
	}

CIpCprCtrlClientJoinActivity::CIpCprCtrlClientJoinActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount)
  : CIpCprUpsCtrlClientJoinActivity(aActivitySig, aNode, aNextActivityCount)
	{
	}

void CIpCprCtrlClientJoinActivity::GetPlatSecResultAndDestinationL(const ESock::MPlatsecApiExt& aPlatSecApi, TInt& aPlatSecResult, TUpsDestinationAddrType& aDestinationType, TDes8& aDestination) const
	{
   	aPlatSecResult = IpCprStates::CheckAttachPolicy(aPlatSecApi);	
	aDestination = KNullDesC8();
	aDestinationType = ETNone;
	}

//
// CIpCprUpsCtrlClientJoinActivity
//

CIpCprUpsCtrlClientJoinActivity::CIpCprUpsCtrlClientJoinActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount)
  : CDeferredCtrlClientJoinActivity(aActivitySig, aNode, aNextActivityCount)
	{
	}

DEFINE_SMELEMENT(CIpCprUpsCtrlClientJoinActivity::TSendPolicyCheckRequest, NetStateMachine::MStateTransition, CIpCprUpsCtrlClientJoinActivity::TContext)

void CIpCprUpsCtrlClientJoinActivity::TSendPolicyCheckRequest::DoL()
/**
This transition is IPCPR specific because it applies the platsec check against NetworkServices,
and the TUpsDestinationAddr happens to be null for IP, but may not be for other technologies.

@TODO PREQ1116 - could we provision the technology specific information in a new APC structure,
as Viki suggests, and make this generic code ?  How would this be done ?  Can we template the code
against capability ?
*/
	{
	// Send TPolicyCheckRequest
	
    const MPlatsecApiExt* const platSecApi = reinterpret_cast<const MPlatsecApiExt*>(address_cast<TNodeId>(iContext.iSender).Node().FetchNodeInterfaceL(MPlatsecApiExt::KInterfaceId));

	// Retrieve process id and thread id
	TProcessId processId;
   	TThreadId threadId;
	User::LeaveIfError(platSecApi->GetProcessAndThreadId(processId, threadId));

	TInt policyCheckResult = KErrCorrupt;		// sanity - ensure it is overwritten
	TUpsDestinationAddr destination;
	TUpsDestinationAddrType destinationType;
	
	CIpCprUpsCtrlClientJoinActivity* act = iContext.Activity();

	act->GetPlatSecResultAndDestinationL(*platSecApi, policyCheckResult, destinationType, destination);

	// Retrieve policy check result
 	const CMMCommsProviderBase& node = iContext.Node();
	// Create a TPolicyCheckRequest message (with an empty destination name) and send to Control Provider.
	const TPolicyCheckRequestParams params(processId, threadId, policyCheckResult, destination, destinationType);
	const UpsMessage::TPolicyCheckRequest checkMsg(params);

	RNodeInterface* const ctrlProvider = node.ControlProvider();
	ASSERT(ctrlProvider);
	act->PostRequestTo(*ctrlProvider, checkMsg);
	}

DEFINE_SMELEMENT(CIpCprUpsCtrlClientJoinActivity::TNoTagOrUpsErrorTag, NetStateMachine::MStateFork, CIpCprCtrlClientJoinActivity::TContext)

TInt CIpCprUpsCtrlClientJoinActivity::TNoTagOrUpsErrorTag::TransitionTag()
/**
Return KNoTag if UPS was successful, else KUpsErrorTag.

Also sets the activity error which triggers cleanup in ~CReversibleCtrlClientJoinActivity.

@return KNoTag if a TPolicyCheckResponse was received with KErrNone, KErrorTag if a
TPolicyCheckResponse was received without KErrNone, KErrorTag if TError was received.

@TODO PREQ1116 - this is generic enough to be moved into CReversibleCtrlClientJoinActivity at least.
*/
	{		
	const TEBase::TError* const errMsg = message_cast<TEBase::TError>(&iContext.iMessage);
	CNodeActivityBase* act = iContext.Activity();
	if (errMsg)
		{
		act->SetError(errMsg->iValue);
		return KUpsErrorTag;
		}

	const UpsMessage::TPolicyCheckResponse& msg = message_cast<UpsMessage::TPolicyCheckResponse>(iContext.iMessage);

	if (msg.iValue != KErrNone)
		{
		act->SetError(msg.iValue);
		return KUpsErrorTag;
		}
	return KNoTag;
	}

#endif //SYMBIAN_NETWORKING_UPS
