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
// IP Connection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/corecprstates.h>
#include <comms-infras/corecpractivities.h>
#include "IPCpr.h"
#include "IPMessages.h"
#include <comms-infras/ss_log.h>
#include <comms-infras/api_ext_list.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef SYMBIAN_TRACE_ENABLE
	#define KIPCprTag KESockConnectionTag
	// _LIT8(KIPCprSubTag, "ipcpr");
#endif // SYMBIAN_TRACE_ENABLE

using namespace Messages;
using namespace ESock;
using namespace NetStateMachine;
using namespace MeshMachine;

//We reserve space for two preallocated activities that may start concurrently on the CPR
//node: destroy and data client stop.
static const TUint KDefaultMaxPreallocatedActivityCount = 2;
static const TUint KMaxPreallocatedActivitySize = sizeof(MeshMachine::CNodeRetryParallelActivity) + sizeof(MeshMachine::APreallocatedOriginators<4>);
static const TUint KIPCPRPreallocatedActivityBufferSize = KDefaultMaxPreallocatedActivityCount * KMaxPreallocatedActivitySize;

//-=========================================================
//
// CIPConnectionProvider methods
//
//-=========================================================
CIPConnectionProvider* CIPConnectionProvider::NewL(ESock::CConnectionProviderFactoryBase& aFactory)
    {
    CIPConnectionProvider* provider = new (ELeave) CIPConnectionProvider(aFactory);
    CleanupStack::PushL(provider);
    provider->ConstructL(KIPCPRPreallocatedActivityBufferSize);
    CleanupStack::Pop(provider);
	return provider;
    }

CIPConnectionProvider::CIPConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory)
	: CMobilityConnectionProvider(aFactory, IpCprActivities::ipCprActivities::Self())
    {
    LOG_NODE_CREATE(KIPCprTag, CIPConnectionProvider);
    }

CIPConnectionProvider::~CIPConnectionProvider()
    {
    LOG_NODE_DESTROY(KIPCprTag, CIPConnectionProvider);
    }

void CIPConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
   	TNodeContext<CIPConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
	CMobilityConnectionProvider::ReceivedL(aSender, aRecipient, aMessage);
	User::LeaveIfError(ctx.iReturn);
	}


#ifdef SYMBIAN_NETWORKING_UPS
RNodeInterface* CIPConnectionProvider::NewClientInterfaceL(const TClientType& aClientType, TAny* aClientInfo)
    {
    
    if (aClientType.Type() & TCFClientType::ECtrl)
    		{
    		if (aClientInfo == NULL)
    			{ // This is not CPR, probably CConnection, use min priority.
    			return new (ELeave) RIPCprControlClientNodeInterface(KMaxTUint);
    			}
    		else
    			{
    			const TUint* priority = static_cast<const TUint*>(aClientInfo);
    			return new (ELeave) RIPCprControlClientNodeInterface(*priority);
    			}
    		}
    
    return CConnectionProviderBase::NewClientInterfaceL(aClientType, aClientInfo);
    }
#endif

