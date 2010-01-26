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
// Agent Handling routines for Tunnel
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/nifprvar.h>			// for TAgentToFlowEventType
#include "TunnelAgentHandler.h"
#include "tunnelProvision.h"
#include "tunnelnifvar.h"

using namespace MeshMachine;
using namespace Messages;

CTunnelAgentHandler::CTunnelAgentHandler(const TTunnelInfo& aTunnelInfo)
  : iTunnelInfo(aTunnelInfo)
	{
	}

void CTunnelAgentHandler::ServiceStarted()
	{
	// Notify Agent of the CFProtocol name.  iTunnelInfo points to provisioning information
	// in AccessPointConfig.  This is valid while we are valid, but if there's ever a need
	// to decouple this in future, then GetExtension[L]() can be called to retrieve the
	// provisioning information on-the-fly.
	NotificationToAgent(static_cast<TFlowToAgentEventType>(ENifToAgentEventTypeSetIfName), (TAny*)&iTunnelInfo.iIfName);
	}

TInt CTunnelAgentHandler::NotificationFromAgent(TAgentToFlowEventType aEvent, TAny* aInfo)
/**
Deal with notification calls from the Agent.

Mainly the update/set address notifications that indicate a new Tunnel address.  A suitable
TTunnelAddressMsg message is formed and sent to the CFProtocol.
*/
	{
	TBool updateFlag = EFalse;
	switch (aEvent)
		{
	case EAgentToNifEventTypeUpdateAddress:
		updateFlag = ETrue;
		/* FALLTHROUGH */
	case EAgentToNifEventTypeSetAddress:
		{
        TInetIfConfig* ifConfig = reinterpret_cast<TInetIfConfig*>(aInfo);

		// The TInetIfConfig object pointed to by aInfo is on the stack (not the heap) of the
		// VPNConnAgt.  Consequently, the information is copied into a TunnelAddressMsg message
		// and sent over to the Tunnel CFProtocol.  There can be several of these messages internally
		// generated one after another in the Agent when the Tunnel address changes.
		//
		// Note that the message only contains those fields of TInetIfConfig that are
		// actually used by the Tunnel CFProtocol.  This keeps the message size small and
		// avoids having to use a "blob" message.
#ifndef __GCCXML__
        //Using Null TCommsId, as PostMessageToFlow will substitue it properly.
		return AnonPostMessageToFlow(
        		TTunnelAgentMessage::TTunnelSetAddress(
        				updateFlag,
        				ifConfig->iAddress,
        				ifConfig->iNameSer1,
        				ifConfig->iNameSer2
        				).CRef()
        		); // This will set the NodeChannelId properly before routing the message to the Flow.
#else
		return 0;
#endif
		}

	default:
		// All other messages handled in default base class manner.
		return CAgentNotificationHandler::NotificationFromAgent(aEvent, aInfo);
		}
	}

