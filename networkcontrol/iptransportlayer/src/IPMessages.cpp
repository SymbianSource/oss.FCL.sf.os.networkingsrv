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
// IP-proprietary messages
// 
//

/**
 @file
 @internalTechnology
*/

#include <comms-infras/ss_thread.h>
#include "IPMessages.h"
#include "policyrequest.h"
#include <elements/metadata.h>

using namespace Messages;
using namespace Elements;
using namespace ESock;
using namespace CommsDat;

enum EMessageTypeId
//message signatures only (NOT messages) messages are declared under class TCFMessage
	{
	EConnPolicyRequestSig			= 2,
	ESigPolicyParams				= 3,

	ENodeSignalBase					= 1000,
	EPolicyRequestBase				= 1001,
	};

START_ATTRIBUTE_TABLE( TCFConnPolicyRequest, TCFIPMessage::ERealmId, EConnPolicyRequestSig )
	REGISTER_ATTRIBUTE( TCFConnPolicyRequest, iParamBundle, TMeta<RCFParameterFamilyBundleC> )
	REGISTER_ATTRIBUTE( TCFConnPolicyRequest, iFlowNodeId, TMeta<TNodeId> )
	REGISTER_ATTRIBUTE( TCFConnPolicyRequest, iSenderSCPrNodeId, TMeta<TNodeId> )
	REGISTER_ATTRIBUTE( TCFConnPolicyRequest, iIPCPrNodeId, TMeta<TNodeId> )
	REGISTER_ATTRIBUTE( TCFConnPolicyRequest, iNewSCPrNodeId, TMeta<TNodeId> )
END_ATTRIBUTE_TABLE_BASE( TSignatureBase, ESignatureBase )

START_ATTRIBUTE_TABLE( TSigPolicyParams, TCFIPMessage::ERealmId, ESigPolicyParams )
	REGISTER_ATTRIBUTE( TSigPolicyParams, iAddrUpdate, TMeta<TAddrUpdate> )
	REGISTER_ATTRIBUTE( TSigPolicyParams, iSrcNodeId, TMeta<TNodeId> )
	REGISTER_ATTRIBUTE( TSigPolicyParams, iFlowId, TMeta<TNodeId> )
	REGISTER_ATTRIBUTE( TSigPolicyParams, iAppSid, TMeta<TUid> )
END_ATTRIBUTE_TABLE_BASE( TSignatureBase, ESignatureBase )

DEFINE_MVIP_CTR(TSigPolicyParams);
DEFINE_MVIP_CTR(TCFConnPolicyRequest)

const TImplementationProxy SignatureImplementationTable[] =
	{
	MVIP_CTR_ENTRY(EConnPolicyRequestSig, TCFConnPolicyRequest),
	MVIP_CTR_ENTRY(ESigPolicyParams, TSigPolicyParams),	
	};

void TCFIPMessage::RegisterL()
	{
	//Temporary pain relieve?
	if (!TlsGlobals::Get().IsInterfaceRegistered(TUid::Uid(TCFIPMessage::ERealmId)))
		{
		TlsGlobals::Get().RegisterInterfaceL(TUid::Uid(TCFIPMessage::ERealmId), sizeof(SignatureImplementationTable) / sizeof(SignatureImplementationTable[0]), SignatureImplementationTable);
		}
	}

void TCFIPMessage::DeRegister()
	{
	TlsGlobals::Get().DeregisterInterface(TUid::Uid(TCFIPMessage::ERealmId));
	}

