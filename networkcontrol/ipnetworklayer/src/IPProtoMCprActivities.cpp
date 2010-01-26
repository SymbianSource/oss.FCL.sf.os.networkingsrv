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
// IPProto MCPR Activities
// 
//

/**
 @file
 @internalComponent
*/


#include "ipprotomcpr.h"
#include <coremcpractivities.h>
#include <coremcprstates.h>
#include <comms-infras/ss_nodeactivities.h>
#include <comms-infras/ss_nodemessages.h>
#include <comms-infras/corecpractivities.h>
#include "IPProtoMCPRActivities.h"

using namespace ESock;
using namespace NetStateMachine;
using namespace MCprActivities;

namespace IPProtoMCprSelectActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivitySelectNextLayer, IPProtoMCprSelect, TCFMessage::TSelectNextLayer, CSelectNextLayerActivity::NewL)
	//Reply from TAwaitingSelectNextLayer if no choices, otherwise accept
	FIRST_NODEACTIVITY_ENTRY(MCprStates::TAwaitingSelectNextLayer, TNoTagOrPermissionDenied)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, TMaybeLockIap, CoreStates::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TProcessSimpleSelectionPolicy, MCprStates::TSelectedProvider)
	//Start the selection main loop
	NODEACTIVITY_ENTRY(MCprStates::KSelectedProvider, CSelectNextLayerActivity::TFindOrCreateTierManager, MCprStates::TAwaitingTierManagerCreated, CoreStates::TNoTag)
	//Select next provider and enter the selection internal loop if provider received. Break if SelectComplete(NULL).
	NODEACTIVITY_ENTRY(KNoTag, CSelectNextLayerActivity::TSelectNextLayer, MCprStates::TAwaitingSelectComplete, CSelectNextLayerActivity::TNoTagOrSelectedProviderIsNull)
	//Break the selection internal loop if SelectComplete(NULL), otherwise stay in this tripple
	NODEACTIVITY_ENTRY(KNoTag, MCprStates::TJoinServiceProviderAndSendSelectComplete, MCprStates::TAwaitingSelectComplete, CSelectNextLayerActivity::TNoTagBackwardsOrSelectedProviderIsNull)
	//Break the selection main loop if no more choices, otherwise go back again
	THROUGH_NODEACTIVITY_ENTRY(MCprStates::KSelectedProviderIsNull, CoreStates::TDoNothing, CSelectNextLayerActivity::TNoTagOrSelectedProviderBackward)
	//Finish the activity

	LAST_NODEACTIVITY_ENTRY(KNoTag, MCprStates::TSendFinalSelectComplete)
	LAST_NODEACTIVITY_ENTRY(KPermissionDenied, CoreStates::TRaiseActivityError)
NODEACTIVITY_END()
}

