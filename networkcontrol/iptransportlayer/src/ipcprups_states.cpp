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
// IP Connection Provider UPS state definitions.
// 
//

/**
 @file
 @internalComponent
*/

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include "IPCpr.h"
#include "ipcprups_states.h"
#include <comms-infras/ss_internal_activities.h>
#include <comms-infras/ss_nodemessages_factory.h>

#include <comms-infras/upsmessages.h>

using namespace Messages;
using namespace ESock;
using namespace IpCprActivities;

_LIT_SECURITY_POLICY_C1(KIpCprStartSecurityPolicy, ECapabilityNetworkServices);

//
// UPS Support
//

void IpCprStates::TCheckCapabilitiesForUps::DoL()
/**
Transition base class used to implement UPS checking for scenarios where the UPS parameters need to be
populated in the UPS Access Point Config structure (e.g. TNoBearer).  This Base class is intended to provide
generic functionality, with the pure virtual GetPlatSecResultAndDestinationL() performing the Platform Security
check against the specific capability and filling in any specific destination address.

This transition is intended to be derived from and not used in its own right, hence there is no
need for a corresponding DEFINE_SMELEMENT_NAMESPACE() statement (in fact, putting one in causes
a compiler error due to the pure virtual).
*/
    {
	const Meta::SMetaData* const extension = 
		iContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CUPSAccessPointConfigExt::EUPSAccessPointConfigUid, CUPSAccessPointConfigExt::ETypeId));

    // Check if UPS is enabled or not by checking for the presence or absence of the
    // UPS Access Point Config (APC) structure.
	if (extension)
		{
		// UPS is enabled - fill in the APC structure
		CUPSAccessPointConfigExt* const upsExt = const_cast<CUPSAccessPointConfigExt*> (static_cast<const CUPSAccessPointConfigExt*>(extension));
		upsExt->SetPolicyCheckResult(KErrCompletion);
		}
    }

DEFINE_SMELEMENT(IpCprStates::TCheckStartCapabilities, NetStateMachine::MStateTransition, IpCprStates::TContext)

void IpCprStates::TCheckStartCapabilities::GetPlatSecResultAndDestinationL(MPlatsecApiExt& aPlatSecApi, TInt& aPlatSecResult, TUpsDestinationAddrType& aDestinationType, TDes8& aDestination)
/**
Transition (derived from IpCprStates::TCheckCapabilitiesForUps) to implement UPS functionality for an RConnection::Start().
*/
	{
	aPlatSecResult = aPlatSecApi.CheckPolicy(KIpCprStartSecurityPolicy);
	aDestination = KNullDesC8();
	aDestinationType = ETNone;
	}

TInt IpCprStates::CheckAttachPolicy(const MPlatsecApiExt& aPlatSecApi)
/**
Common function for checking attach capability.
*/
	{
    return aPlatSecApi.CheckPolicy(KIpCprStartSecurityPolicy);	// Same as Start policy
	}

/**
IpCpr Attach capability check transition
*/
DEFINE_SMELEMENT(IpCprStates::TCheckAttachCapabilities, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TCheckAttachCapabilities::DoL()
    {
    const MPlatsecApiExt* const platsec = reinterpret_cast<const MPlatsecApiExt*>(address_cast<TNodeId>(iContext.iSender).Node().FetchNodeInterfaceL(MPlatsecApiExt::KInterfaceId));
	User::LeaveIfError(CheckAttachPolicy(*platsec));
    }

DEFINE_SMELEMENT(IpCprStates::TNoTagOrUpsQueryOrAttachCapabilityQuery, NetStateMachine::MStateFork, IpCprStates::TContext)
TInt IpCprStates::TNoTagOrUpsQueryOrAttachCapabilityQuery::TransitionTag()
/**
This state fork is IPCPR specific because it enables UPS checking on an RConnection::Attach() by virtue of examining the activity id
carried in the TCtrlClientJoinRequest.
*/
	{	
	ASSERT(iContext.iMessage.IsMessage<TCFControlClient::TJoinRequest>() || iContext.iMessage.IsMessage<TCFFactory::TPeerFoundOrCreated>());
	
	// If TCtrlClientJoinRequest has been received, check if it was from an RConnection::Attach().
	//
	// NOTE: This check against activity id is not ideal, as it places a dependency on any upper layers
	// to have a particular activity id originating the TCtrlClientJoinRequest.
	//
	TCFClientType::TFlags clientFlags = (TCFClientType::TFlags) 0;

	if(iContext.iMessage.IsMessage<TCFControlClient::TJoinRequest>())
		{
		TCFControlClient::TJoinRequest& message =
			message_cast<TCFControlClient::TJoinRequest>(iContext.iMessage);
		clientFlags = static_cast<TCFClientType::TFlags>(message.iClientType.Flags());
		}

	if (clientFlags & TCFClientType::EAttach)
		{
		// An RConnection::Attach() is taking place, so check if UPS has been enabled (by checking for the
		// presence of the UPS Access Point Config extension) and return the result.
		return UpsDisabled(iContext) ? IpCprStates::KAttachCapabilityQuery : IpCprStates::KUpsQuery;
		}
	return MeshMachine::KNoTag;
		
	}

DEFINE_SMELEMENT(IpCprStates::TNoTagOrUpsDisabledOrUpsEnabled, NetStateMachine::MStateFork, IpCprStates::TContext)
TInt IpCprStates::TNoTagOrUpsDisabledOrUpsEnabled::TransitionTag()
	{
	if (UpsDisabled(iContext))
		{
		return IpCprStates::KUpsDisabled;
		}
	else
		{
	    RNodeInterface* client = iContext.Node().FindClient(iContext.iSender);
		if ((client) // need to handle scenario when client is zero.
			&& (client->Type() & TCFClientType::EData))
				{
				return MeshMachine::KNoTag; // come from data client, so no plat sec checking required
				}		
		return IpCprStates::KUpsEnabled; // come from control client, so plat sec checking required
		} 
	}

DEFINE_SMELEMENT(IpCprStates::TPostDisabledPolicyCheckResponseToOriginators, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TPostDisabledPolicyCheckResponseToOriginators::DoL()
	{
	iContext.iNodeActivity->PostToOriginators(UpsMessage::TPolicyCheckResponse(GetPlatsecResultL(iContext)));
	}

DEFINE_SMELEMENT(IpCprStates::TPopulatePolicyCheckRequest, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TPopulatePolicyCheckRequest::DoL()
	{
	UpsMessage::TPolicyCheckRequest& msg = message_cast<UpsMessage::TPolicyCheckRequest>(iContext.iMessage);
	msg.iParams.iPlatSecResult = GetPlatsecResultL(iContext);

    RNodeInterface* client = iContext.Node().FindClient(iContext.iSender);
	if ((client)
		&& (client->Type() & TCFClientType::ECtrl))
		{
		RIPCprControlClientNodeInterface* ctrlClient = static_cast<RIPCprControlClientNodeInterface*>(client);
		ctrlClient->iPolicyCheckRequestIssued=ETrue;
		ctrlClient->iProcessId = msg.iParams.iProcessId;
		ctrlClient->iThreadId  = msg.iParams.iThreadId;
		}
	}

DEFINE_SMELEMENT(IpCprStates::TSendUpsStatusChange, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TSendUpsStatusChange::DoL()
	{
	RNodeInterface* client = iContext.Node().FindClient(iContext.iSender);
	if ((client)
		&& (client->Type() & TCFClientType::ECtrl))
		{
		RIPCprControlClientNodeInterface* ctrlClient = static_cast<RIPCprControlClientNodeInterface*>(client);
		
		if (ctrlClient->iPolicyCheckRequestIssued)
			{
			TUPSStatusChangeParams params(ctrlClient->iProcessId, ctrlClient->iThreadId, client->RecipientId());
			UpsMessage::TUPSStatusChange statusChangeMsg(params);
	
			RNodeInterface* controlProvider = iContext.Node().ControlProvider();
			ASSERT(controlProvider);
			controlProvider->PostMessage(iContext.Node().Id(), statusChangeMsg);
			}
		}
	}

TBool IpCprStates::UpsDisabled(TContext& aContext)
	{
	const Meta::SMetaData* const extension = 
		aContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CUPSAccessPointConfigExt::EUPSAccessPointConfigUid, CUPSAccessPointConfigExt::ETypeId));
	return extension == NULL;
	}

MPlatsecApiExt* IpCprStates::GetPlatsecApiExt(TContext& aContext)
	{
	MPlatsecApiExt* const platsec = reinterpret_cast<MPlatsecApiExt*>(address_cast<TNodeId>(aContext.iSender).Node().FetchNodeInterfaceL(MPlatsecApiExt::KInterfaceId));
	ASSERT(platsec);
	return platsec;
	}


TInt IpCprStates::GetPlatsecResult(TContext& aContext)
	{
	return GetPlatsecApiExt(aContext)->CheckPolicy(KIpCprStartSecurityPolicy);
	}

TInt IpCprStates::GetPlatsecResultL(TContext& aContext)
	{
	TInt result = GetPlatsecResult(aContext);
	if (result != KErrNone && result != KErrPermissionDenied && result != KErrCompletion)
		{
		User::Leave(result);
		}
	return result;
	}

#endif  //SYMBIAN_NETWORKING_UPS
