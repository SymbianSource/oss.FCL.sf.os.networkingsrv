// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_NETMCPR_H
#define SYMBIAN_NETMCPR_H

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/mobilitymcpr.h>
#include <comms-infras/ss_nodemessages.h>
#include "netmcprstates.h"
#include "IPMessages.h"

#include <metadatabase.h>
#include <commsdattypesv1_1.h>
#include <in_sock.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <tcprecvwin.h>
#include <comms-infras/ss_protopt.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

_LIT(KNetMCprPanic, "NetworkMCprPanic");

static const TInt KPanicNoActivity = 1;
static const TInt KPanicNoContext = 2;
static const TInt KPanicNoConnectionHandle = 3;
class RParameterFamily;

namespace NetMCprStates
	{
	class TProcessPolicyParams;
	}


//
//CNetworkMetaConnectionProvider
NONSHARABLE_CLASS(CNetworkMetaConnectionProvider) : public CMobilityMetaConnectionProvider
    {
friend class NetMCprStates::TProcessPolicyParams;

public:
	static CNetworkMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory,
	                                            const ESock::TProviderInfo& aProviderInfo);

protected:
    CNetworkMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory,
                                   const ESock::TProviderInfo& aProviderInfo,
                                   const MeshMachine::TNodeActivityMap& aActivityMap);
    void ConstructL();

	virtual ~CNetworkMetaConnectionProvider();
    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

private:
	TBool FindMatchingPolicyL(TCFIPMessage::TPolicyParams& aPolicyParam);
	//void ProcessPolicyParamsL(const ESock::TAddrUpdate& aAddrUpdate, const Messages::TNodeId& aFlowId, const Messages::TNodeId& aSenderSCprId, const Messages::TNodeId& aSenderIPCprId);
	void InitDbL();

	void ProcessPolicyParamsL(const Messages::TRuntimeCtxId& aSender, TCFIPMessage::TPolicyParams& aPolicyParam);
	ESock::RCFParameterFamilyBundleC CreateParameterBundleL(TUint aPolicyId);
	TInt FillInParamsL(TUint aParamId, RParameterFamily& aFamily);
	TInt FillInGenericQosParamsL(TUint aParamId, RParameterFamily& aFamily);
	TInt FillInUMTSParamsL(TUint aParamId, RParameterFamily& aFamily);

	// matching helper functions
	TBool CheckSrcAddressMatch(const TSockAddr& aFirst, CCDPolicySelectorRecord* aRecord);
	TBool CheckDstAddressMatch(const TSockAddr& aFirst, CCDPolicySelectorRecord* aRecord);
	TBool CheckProtocol(TUint aProtocolId, CCDPolicySelectorRecord* aRecord);
	TBool CheckSrcPort(TUint aPort, CCDPolicySelectorRecord* aRecord);
	TBool CheckDstPort(TUint aPort, CCDPolicySelectorRecord* aRecord);
	TBool CheckIap(TUint aIapId, CCDPolicySelectorRecord* aRecord);
	TBool CheckAppUid(TUid aAppUid, CCDPolicySelectorRecord* aRecord);

private:
	CommsDat::CMDBSession* iDbSession;
	CommsDat::CMDBRecordSet<CommsDat::CCDPolicySelectorRecord>* iPolicySelectorRecSet;
    };

#ifdef SYMBIAN_NETWORKING_UPS

#define DEFINE_NETMCPR_TRANSITION(class, method) DEFINE_SMELEMENT(class::method, NetStateMachine::MStateTransition, class::TContext)
#define DEFINE_NETMCPR_STATEFORK(class, method) 	DEFINE_SMELEMENT(class::method, NetStateMachine::MStateFork, class::TContext)
#define DEFINE_NETMCPR_STATE(class, method)		DEFINE_SMELEMENT(class::method, NetStateMachine::MState, class::TContext)

#endif


#endif //SYMBIAN_NETMCPR_H


