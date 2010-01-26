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

#include "ipscpr.h"
#include "ipdeftbasescpr.h"
#include "ipscprlog.h"
#include "ipscprstates.h"
#include <comms-infras/ss_log.h>
#include "IPMessages.h"

#include <comms-infras/ss_msgintercept.h>
#include <elements/nm_signatures.h>
#include <comms-infras/ss_nodemessages_factory.h>

#if defined __FLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
	#define KIPSCprTag KESockSubConnectionTag
	//_LIT8(KIPSCprSubTag, "ipscpr");
#endif

using namespace MeshMachine;
using namespace Messages;
using namespace ESock;
using namespace NetStateMachine;

//We reserve space for two preallocated activities that may start concurrently on the SCPR
//node: destroy and data client stop.
static const TUint KDefaultMaxPreallocatedActivityCount = 2;
static const TUint KMaxPreallocatedActivitySize = sizeof(MeshMachine::CNodeRetryParallelActivity) + sizeof(MeshMachine::APreallocatedOriginators<4>);
static const TUint KIPSCPRPreallocatedActivityBufferSize = KDefaultMaxPreallocatedActivityCount * KMaxPreallocatedActivitySize;

namespace IPSCprAddressUpdate
{
DECLARE_DEFINE_NODEACTIVITY(IPDeftSCprBaseActivities::ECFActivityAddressUpdate, IPSCprAddressUpdate, TCFIPMessage::TDataClientRouted)
	NODEACTIVITY_ENTRY(KNoTag, IPBaseSCprStates::TStoreAddressUpdate, IPBaseSCprStates::TAwaitingAddressUpdate, IpSCprStates::TNoTagOrDoNothingTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpSCprStates::TCreateAddressInfoBundle, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, IpSCprStates::TSendSelfStart, CoreNetStates::TAwaitingStarted, IpSCprStates::TDoNothingTag)
	LAST_NODEACTIVITY_ENTRY(IpSCprStates::KDoNothingTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
namespace IPSCprParamsRequest
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityParamRequest, IPSCprSetParams, TCFScpr::TParamsRequest)
	FIRST_NODEACTIVITY_ENTRY(SCprStates::TAwaitingParamRequest, CoreNetStates::TNoTagOrBearerPresent)
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, SCprStates::TPassToServiceProvider, CoreNetStates::TAwaitingParamResponse, CoreNetStates::TBearerPresent)
	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, SCprStates::TStoreParamsAndPostToOriginators)
	LAST_NODEACTIVITY_ENTRY(KNoTag, SCprStates::TStoreAndRespondWithCurrentParams)
NODEACTIVITY_END()
}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace IPSCprApplyRequest
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityApplyChanges, IpSCprApplyReq, TCFScpr::TApplyRequest)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	FIRST_NODEACTIVITY_ENTRY(PRStates::TAwaitingApplyRequest, IpSCprStates::TNoTagOrSendApplyResponseOrErrorTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpSCprStates::TCreateAddressInfoBundleFromJoiningClient, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, IpSCprStates::TSendSelfStart, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendApplyResponse)
	LAST_NODEACTIVITY_ENTRY(SCprActivities::Apply::ESendApplyResponse, PRStates::TSendApplyResponse)
#else
	FIRST_NODEACTIVITY_ENTRY(SCprStates::TAwaitingApplyRequest, IpSCprStates::TNoTagOrSendApplyResponseOrErrorTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpSCprStates::TCreateAddressInfoBundleFromJoiningClient, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, IpSCprStates::TSendSelfStart, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, SCprStates::TSendApplyResponse)
	LAST_NODEACTIVITY_ENTRY(SCprActivities::Apply::ESendApplyResponse, SCprStates::TSendApplyResponse)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TRaiseActivityError)
NODEACTIVITY_END()
}

namespace IPSCprStartActivity
{
//TODO perhaps get this from a header file since its used in a number of places - see ss_subconn.cpp
typedef MeshMachine::TAcceptErrorState<CoreNetStates::TAwaitingApplyResponse> TAwaitingApplyResponseOrError;

DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStart, IPSCprStart, TCFServiceProvider::TStart, PRActivities::CStartActivity::NewL)
    FIRST_NODEACTIVITY_ENTRY(MeshMachine::TAwaitingMessageState<TCFServiceProvider::TStart>, IpSCprStates::TNoTagOrBearerPresentBlockedByStopOrBindTo)
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TBindSelfToPresentBearer, CoreNetStates::TAwaitingBindToComplete, MeshMachine::TTag<CoreNetStates::KBearerPresent>)

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendNoBearer, MeshMachine::TAwaitingMessageState<TCFControlProvider::TBearer>, MeshMachine::TTag<CoreNetStates::KBearerPresent>)

	//[401TODO] RZ: this tuple is the only thing that distinguishes this activity from the core although, the core should look
	//like this one - i.e.: sending params down on start is every scpr's bussiness.
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, PRStates::TSendParamsToServiceProvider, CoreNetStates::TAwaitingParamResponse, IpSCprStates::TNoTagOrAlreadyStarted)
#else
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, IpSCprStates::TSendParamsToServiceProvider, CoreNetStates::TAwaitingParamResponse, IpSCprStates::TNoTagOrAlreadyStarted)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStartServiceProviderRetry, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(CoreNetStates::KAlreadyStarted,CoreNetStates::TSendApplyRequest, TAwaitingApplyResponseOrError, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendStarted)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TRaiseActivityError)
NODEACTIVITY_END()
}


namespace IPSCprActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(activityMap)
    ACTIVITY_MAP_ENTRY(IPSCprAddressUpdate, IPSCprAddressUpdate)
    ACTIVITY_MAP_ENTRY(IPSCprApplyRequest, IpSCprApplyReq)
    ACTIVITY_MAP_ENTRY(IPSCprStartActivity, IPSCprStart)
#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(IPSCprParamsRequest, IPSCprSetParams)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
ACTIVITY_MAP_END_BASE(IPDeftBaseSCprActivities, ipdeftbasescprActivityMap)
}
//-=========================================================
//
// CIpSubConnectionProvider methods
//
//-=========================================================
CIpSubConnectionProvider* CIpSubConnectionProvider::NewL(CIpSubConnectionProviderFactory& aFactory)
/**
Construct a new IP SubConnection Provider Object

@params aFactory factory that create this object
@param aConnProvider Connection Provider associated with this object
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::NewL")));
	CIpSubConnectionProvider* self = new (ELeave) CIpSubConnectionProvider(aFactory);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


CIpSubConnectionProvider::~CIpSubConnectionProvider()
    {
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::~CIpSubConnectionProvider [%08x]"), this));
    LOG_NODE_DESTROY(KIPSCprTag, CIpSubConnectionProvider);
    }


CIpSubConnectionProvider::CIpSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory)
:CIpSubConnectionProviderBase(aFactory, IPSCprActivities::activityMap::Self())
    {
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::CIpSubConnectionProvider [%08x]"), this));
    LOG_NODE_CREATE(KIPSCprTag, CIpSubConnectionProvider);
    }

void CIpSubConnectionProvider::ConstructL()
/**
IP SubConnection Provider Second Phase Constructor
*/
	{
    CIpSubConnectionProviderBase::ConstructL(KIPSCPRPreallocatedActivityBufferSize);
	}

RNodeInterface* CIpSubConnectionProvider::NewClientInterfaceL(const TClientType& aClientType, TAny* /*aClientInfo*/)
    {
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::NewClientInterfaceL [%08x]"), this));

    if (aClientType.Type() & TCFClientType::EData)
        {
        return new (ELeave) RIPDataClientNodeInterface();
        }

    return CCoreSubConnectionProvider::NewClientInterfaceL(aClientType);
    }


void CIpSubConnectionProvider::Received(TNodeContextBase& aContext)
    {
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::Received [%08x]"), this));

    Messages::TNodeSignal::TMessageId noPeerIds[] = {
        TCFFactory::TPeerFoundOrCreated::Id(),
        TCFPeer::TJoinRequest::Id(),
        Messages::TNodeSignal::TMessageId()
        };

    MeshMachine::AMMNodeBase::Received(noPeerIds, aContext);
	MeshMachine::AMMNodeBase::PostReceived(aContext);
	}

void CIpSubConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::ReceivedL [%08x]"), this));
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
	IpSCprStates::TContext ctx(*this, aMessage, aSender, aRecipient);
    CIpSubConnectionProvider::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

TInt CIpSubConnectionProvider::AddressCompletionValidation(RIPDataClientNodeInterface& aDataClient)
/**
Helper function to check for presence of source  and destionation address in DataClient Node Interface

@param aDataClient Data Client
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CIpSubConnectionProvider::AddressCompletionValidation [%08x]"), this));

	TUint addrFamily = aDataClient.iCliSrcAddr.Family();

	if( addrFamily != KAfInet6 && addrFamily != KAfInet)
		{
		__IPCPRLOG(IpCprLog::Printf(_L("Source Address not defined")));
		return KErrNotReady;
		}

	addrFamily = aDataClient.iCliSrcAddr.Family();
	if( addrFamily != KAfInet6 && addrFamily != KAfInet)
		{
		__IPCPRLOG(IpCprLog::Printf(_L("Destination Address not defined")));
		return KErrNotReady;
		}

	return KErrNone;
	}

CSubConIPAddressInfoParamSet* CIpSubConnectionProvider::InitBundleL()
/**
Helper function to initialize param bundle
*/
	{
	CSubConIPAddressInfoParamSet* retValue(NULL);

	// create family
	RCFParameterFamilyBundleC& bundle = GetOrCreateParameterBundleL();
	RParameterFamily family=bundle.FindFamily(KSubConIPAddressInfoFamily);
	if ( family.IsNull() ) 
		{
		__IPCPRLOG(IpCprLog::Printf(_L("SubConIPAddressInfoFamily is created")));
		//family = bundle.CreateFamilyL(KSubConIPAddressInfoFamily); //PJLEFT
		RCFParameterFamilyBundle newBundle;
		newBundle.CreateL();
		newBundle.Open(iParameterBundle);
		family = newBundle.CreateFamilyL(KSubConIPAddressInfoFamily);
		retValue = CSubConIPAddressInfoParamSet::NewL(family, RParameterFamily::ERequested);
		newBundle.Close();
		}
	else
		{
		// get param set
		STypeId typeId = STypeId::CreateSTypeId( CSubConIPAddressInfoParamSet::EUid,
						CSubConIPAddressInfoParamSet::ETypeId );
		retValue = static_cast<CSubConIPAddressInfoParamSet*>
			(family.FindParameterSet(typeId,RParameterFamily::ERequested));
		}

	return retValue;
	}
