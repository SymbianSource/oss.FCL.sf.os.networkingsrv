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
// THIS API IS INTERNAL TO NETWORKING AND IS SUBJECT TO CHANGE AND NOT FOR EXTERNAL USE
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include "netmcprups.h"
#include <comms-infras/netups.h>
using namespace ESock;

DEFINE_NETMCPR_TRANSITION(NetMCprUpsStates, TProcessUpsStatusChange)
void NetMCprUpsStates::TProcessUpsStatusChange::DoL()
/**
Processes a UPS Status Change notification: a connection which had 
previously sent a Policy Check Request to the NetUps is now in the 
process of leaving its Service Provider.
*/
    {
    UpsMessage::TUPSStatusChange* const msg = Messages::message_cast<UpsMessage::TUPSStatusChange>(&iContext.iMessage);    
	CUpsNetworkMetaConnectionProvider& node = iContext.Node();

	if (msg && node.UpsControlClientPresent()) // safe guard
		{
		TUPSStatusChangeParams* params = &msg->iParams;	

		if (!node.UpsDisabled())
			{
			NetUps::CNetUps* const netUps = node.NetUps();
			if (netUps)
				{
				//
				// netUps->DecrementConnectionCountL() panics if an attempt
				// is made to decrement the Connection Count past zero and this 
				// panic has proved useful in catching errors in logic.
				// However need to handle the scenario when authorisation
				// is denied and a result the conn leaves the ipcpr. This
				// will result a UpsStatusChange notification being generated.
				// This case is detected by determining whether the number of
				// client handles associated with a commsId is zero.
				//
				TInt32 index = 0;
				TInt32 count = 0;
				node.FindUpsClientHandle(params->iCommsId, index, count);
				if (count > 0)
					{
					// There should only be 1 UPS authorisation associated with
					// each CommsId, so in theory it should be possible just to
					// pass the CommsId into DecrementConnectionCount. However,
					// passing in the processId and threadId should speed up the
					// search time.
					netUps->DecrementConnectionCountL(params->iCommsId, params->iProcessId, params->iThreadId);
					}
				// It is possible for clients in different processes to issue an
				// RConnection::Start which results in a PolicyCheckRequest to the 
				// NetUps from this MCPR. This MCPR should not close its handle
				// to the NetUPS until all Connections, represented here by
				// params->iCommsId have either left, or are in the process
				// of leaving the IpCpr.

				TBool allHandlesDeleted = EFalse;
				node.DecrementUpsClientHandle(params->iCommsId, allHandlesDeleted);
				if (allHandlesDeleted)
					{
					node.CloseNetUps();
					}
				}
			}		
		}
    }

DEFINE_NETMCPR_TRANSITION(NetMCprUpsStates, TUpsProcessProviderStatusChange);

void NetMCprUpsStates::TUpsProcessProviderStatusChange::DoL()
/**
Send provider status change notifications to NetUps.  Post a TProviderStatusChange to self
in order to trigger the core ProviderStatusChange activity.

This transition is NetMCpr specific rather than core generic because of the UPS Service Id.

@TODO PREQ1116 - do we put this into CoreMCprStates as some form of template with ServiceId argument,
in a way in which NetMCpr doesn't need to link against netups.dll ?  Use of CUpsAuthorisationActivityBase
seems a little heavyweight in this respect.
*/
    {
    const TCFControlProvider::TDataClientStatusChange& msg = Messages::message_cast<TCFControlProvider::TDataClientStatusChange>(iContext.iMessage);

	CUpsNetworkMetaConnectionProvider& node = iContext.Node();

	if (!node.UpsDisabled() && msg.iValue == 0)
		{
		// filter out duplicate provider down events
		//
		// @TODO PREQ1116 - hack for ProviderStatusDown.  We are setting a single flag on the
		// NetMCpr node class to filter multiple provider down events that will adversely affect
		// NetUps operation.  Besides assuming that we only have one client, it is perhaps better
		// that the filtering be done in the NetUps.  However, filter here for now.
		if (!node.ProviderStatusDown())
			{
			node.SetProviderStatusDown(ETrue);
			if (node.UpsControlClientPresent() == EFalse)   // If the UPS request originated as a result of a RConnection::Start()
														    // then dont decrement the connection count on ProviderStatus Down -
															// this will be handled by TProcessUpsStatusChange::DoL().
														    // Only need to decrement the count if the original UPS request
														    // originated from an implicit socket. 
				{
				NetUps::CNetUps* const netUps = node.NetUps();
				if (netUps)
					{
					netUps->DecrementConnectionCountL(node.Id());
					TBool allHandlesDeleted = EFalse;
					node.DecrementUpsClientHandle(node.Id(), allHandlesDeleted);
					if (allHandlesDeleted)
						{
						node.CloseNetUps();
						}
					}
				}
			}
		}

	// Now execute the provider status change code in the core (i.e. MCprMonitorProviderStatusActivity).
	//
	// Explanation:
	// We are trying to do some UPS specific processing on receipt of TProviderStatusChange, and
	// then continue executing the core activity.  However, we cannot use the trick of accepting the
	// message, doing our processing, self posting the message, and ignoring self posted messages so
	// they drop through to the core.  This is because the core activity accesses iPeer, which is
	// only set if the message iSender field contains the real originator node and not that of the self
	// post.  Forwarding the message without change of course gives us no way of differentiating whether
	// it is self posted or not and ignoring it if so.  Hence we directly call the transition that the
	// core activity uses.
	
	PRStates::THandleDataClientStatusChangeAndDestroyOrphans coreStatusChange(iContext);
	coreStatusChange.DoL();
    }



#endif // SYMBIAN_NETWORKING_UPS
