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
// IP Connection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPCPR_H
#define SYMBIAN_IPCPR_H

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS
#include <comms-infras/ss_nodeinterfaces.h>
#endif


#include <comms-infras/corecpr.h>
#include <comms-infras/ss_coreprstates.h>
#include <comms-infras/mobilitycpr.h>
#include <comms-infras/ss_mobility_apiext.h>
#include <comms-infras/ss_datamonitoringprovider.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include "ipcpr_activities.h"
#include "ipcpr_states.h"

using namespace CorePanics;

_LIT(KIpCprPanic, "IpCprPanic");

class CIPConnectionProviderFactory;

#ifdef SYMBIAN_NETWORKING_UPS
NONSHARABLE_CLASS(RIPCprControlClientNodeInterface) : public ESock::RCFNodePriorityInterface
/** IP CPR override for RNodeInterface control client.
    Caches the control client's processId and threadId.
    Also caches whether this control client is being used to start a connection.
    When a IpCpr control clients start a connection this results in a UPS authorisation request
    being sent to the NetUps and iStartedConnection being set to True. This allows the netups
    to be updated when the client subsequently leaves.
@internalTechnology
@released Since 9.6 */
    {
public:
	explicit RIPCprControlClientNodeInterface(TUint aPriority)
    	:  ESock::RCFNodePriorityInterface(aPriority), iPolicyCheckRequestIssued(EFalse), iThreadId(0), iProcessId(0) 
    	 {    	
    	 }
public:
    TBool		iPolicyCheckRequestIssued;	
	TThreadId	iThreadId;
	TProcessId	iProcessId;
    };
#endif


NONSHARABLE_CLASS(CIPConnectionProvider) : public CMobilityConnectionProvider
/** IP connection provider

@internalTechnology
@released Since 9.4 */
    {
friend class CIPConnectionProviderFactory;
friend class IpCprStates::TProcessSubConnDataTransferred;

public:
    typedef CIPConnectionProviderFactory FactoryType;

	static CIPConnectionProvider* NewL(ESock::CConnectionProviderFactoryBase& aFactory);

protected:
    CIPConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory);
    virtual ~CIPConnectionProvider();
    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
#ifdef SYMBIAN_NETWORKING_UPS
    virtual Messages::RNodeInterface* NewClientInterfaceL(const Messages::TClientType& aClientType, TAny* aClientInfo = NULL);    
#endif
    };

#endif //SYMBIAN_IPCPR_H
