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
 @internal
 @released
*/

#if !defined(__POLICYREQUEST_H__)
#define __POLICYREQUEST_H__

#include <comms-infras/ss_nodemessages.h>
#include <comms-infras/ss_mmnode.h>
#include <comms-infras/ss_platsec_apiext.h>
#include "IPMessages.h"
#include <elements/nm_messages_internal.h>

class TCFConnPolicyRequest;

//
//CPolicyRequest
NONSHARABLE_CLASS(CPolicyRequest) : public CBase,
	                                public ESock::ACFMMNodeIdBase
	{
	friend class TCFConnPolicyRequest;

public:
    CPolicyRequest(const TCFConnPolicyRequest& aMsg);
    virtual ~CPolicyRequest();
    void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

protected:
	void Received(MeshMachine::TNodeContextBase& aContext);

public: //For simpler access from the states.
	ESock::RCFParameterFamilyBundleC iParamBundle;
	Messages::TNodeId iFlowNodeId;
	Messages::RNodeInterface* iDeftScprClient;
	Messages::TNodeId iSenderSCPrNodeId;
	Messages::TNodeId iIPCPrNodeId;
	Messages::TNodeId iNewSCprId;
	TInt iError;
	};

//
//TCFConnPolicyRequest
NONSHARABLE_CLASS(TCFConnPolicyRequest) : public Messages::TSelfDispatcherAndErrorHandler
	{
public:
	explicit TCFConnPolicyRequest(ESock::RCFParameterFamilyBundleC& aParamBundle, Messages::TNodeId aFlowNodeId, Messages::TNodeId aSenderSCPrNodeId, Messages::TNodeId aIPCPrNodeId)
	   :iParamBundle(aParamBundle),
		iFlowNodeId(aFlowNodeId),
		iSenderSCPrNodeId(aSenderSCPrNodeId),
		iIPCPrNodeId(aIPCPrNodeId)
		{
		}

protected:
	void StartL(const Messages::TRuntimeCtxId& aSender);
	void DispatchL(const Messages::TRuntimeCtxId& aSender, const Messages::TRuntimeCtxId& aRecipient);
	TCFConnPolicyRequest()
		{
		}

public:
	ESock::RCFParameterFamilyBundleC iParamBundle;
	Messages::TNodeId iFlowNodeId;
	Messages::TNodeId iSenderSCPrNodeId;
	Messages::TNodeId iIPCPrNodeId;
	Messages::TNodeId iNewSCPrNodeId;
public:
	DECLARE_MVIP_CTR(TCFConnPolicyRequest)
	DATA_VTABLE
	};


#endif
// __POLICYREQUEST_H__

