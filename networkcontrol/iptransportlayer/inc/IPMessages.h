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


#ifndef SYMBIAN_IPMESSAGES_H
#define SYMBIAN_IPMESSAGES_H

#include <comms-infras/ss_nodemessages.h>
#include <networking/pdpdef.h>
#include <commsdattypesv1_1.h>

#include <comms-infras/ss_nodemessages_ipmessages.h>
#include <addressupdate.h>

enum IPCustomActivities
    {
    ECFConnPolicyRequestActivity = ESock::ECFActivityCustom,
	ECFPolicyConnectionActivity,
    };

NONSHARABLE_CLASS(CIPSubConnParameterFactory) : public CBase
	{
public:
	static CSubConExtensionParameterSet* NewL(TAny* aConstructionParameters);
    enum
        {
        EUid = 0x102822D4,
        };
	};

NONSHARABLE_STRUCT(TSigPolicyParams) : public Messages::TSignatureBase
    {
protected:
	inline TSigPolicyParams() {}
	explicit TSigPolicyParams( const Messages::TNodeSignal::TMessageId& aMessageId, ESock::TAddrUpdate aAddrUpdate, Messages::TNodeId aSrcNodeId, Messages::TNodeId aFlowId, TUid aAppSid) :
	Messages::TSignatureBase( aMessageId),
	iAddrUpdate(aAddrUpdate),
	iSrcNodeId(aSrcNodeId),
	iFlowId(aFlowId),
	iAppSid(aAppSid)
	    {}
public:
	   DECLARE_MVIP_CTR( TSigPolicyParams )
   	   DATA_VTABLE

	ESock::TAddrUpdate 	iAddrUpdate;
	Messages::TNodeId 	iSrcNodeId;
	Messages::TNodeId 	iFlowId;
	TUid			iAppSid;
    };

template<TInt id, TInt32 realm>
NONSHARABLE_STRUCT(TCFIPMessageSigPolicyParams) : public TSigPolicyParams, public Messages::TSignatureBase::TTypeToMessageId<id, realm>
    {
	explicit TCFIPMessageSigPolicyParams(ESock::TAddrUpdate aAddrUpdate, Messages::TNodeId aSrcNodeId, Messages::TNodeId aFlowId, TUid aAppSid) :
	    TSigPolicyParams(Messages::TNodeSignal::TMessageId(id, realm), aAddrUpdate, aSrcNodeId, aFlowId, aAppSid ){}
    };

NONSHARABLE_STRUCT(TSigQoSWorkingNodeParams) : public Messages::TSignatureBase
    {
protected:
	inline TSigQoSWorkingNodeParams() {}
	explicit TSigQoSWorkingNodeParams( const Messages::TNodeSignal::TMessageId& aMessageId,
								ESock::RCFParameterFamilyBundleC& aParamBundle, Messages::TNodeId aFlowNodeId, Messages::TNodeId aSenderSCPrNodeId, Messages::TNodeId aCPrNodeId) :
	Messages::TSignatureBase( aMessageId),
	iParamBundle(aParamBundle),
	iFlowNodeId(aFlowNodeId),
	iSenderSCPrNodeId(aSenderSCPrNodeId),
 	iCPrNodeId(aCPrNodeId)
    {}
public:
	   DECLARE_MVIP_CTR( TSigQoSWorkingNodeParams )
   	   DATA_VTABLE

	ESock::RCFParameterFamilyBundleC iParamBundle;
	Messages::TNodeId iFlowNodeId;
	Messages::TNodeId iSenderSCPrNodeId;
	Messages::TNodeId iCPrNodeId;
    };

template<TInt id, TInt32 realm>
NONSHARABLE_STRUCT(TCFIPMessageSigQoSWorkingNodeParams) : public TSigQoSWorkingNodeParams, public Messages::TSignatureBase::TTypeToMessageId<id, realm>
    {
	explicit TCFIPMessageSigQoSWorkingNodeParams(ESock::RCFParameterFamilyBundleC& aParamBundle, Messages::TNodeId aFlowNodeId, Messages::TNodeId aSenderSCPrNodeId, Messages::TNodeId aCPrNodeId ) :
	    TSigQoSWorkingNodeParams(Messages::TNodeSignal::TMessageId(id, realm), aParamBundle, aFlowNodeId, aSenderSCPrNodeId, aCPrNodeId){}
    };

//DECLARE_IP_MESSAGE_SIG_2(SigPolicyParams, ESock::TAddrUpdate, AddrUpdate, Messages::TNodeId, FlowId)

using namespace ESock;

NONSHARABLE_CLASS(TCFIPMessage) : public TCFIPMessages
/**
IP message realm (messages specific to IP layer)

@internalComponent
*/
	{
  public:
    enum { ERealmId = 0x102822DA };

//  protected:
  	friend class CIpTierManager;

  private:
  	enum
  	    {
		ECFConnPolicyRequest 	= 1,
		ECFPolicyRequest		= 2,
		ECFPolicyParams			= 3,
	    };

  public:

	typedef Messages::TMessageSigNumber<ECFPolicyRequest, TCFIPMessage::ERealmId> TPolicyRequest;
	typedef TCFIPMessageSigPolicyParams<ECFPolicyParams, TCFIPMessage::ERealmId> TPolicyParams;

    static void RegisterL();
    static void DeRegister();
	};
/*
//
//DEBUG

#if defined (ESOCK_EXTLOG_ACTIVE)

inline const TText8* Messages::TNodeSignal::Printable() const
	{
	static const TText8* const szMessageId2Name[] =
		{
		_S8("EConnPolicyRequest"),							//1
		_S8("ECFConnPolicyRequest"),	            		//2
		_S8("ECFPolicyRequest"),							//3
		_S8("ECFPolicyParams"),						//4
		};

	const TText8* msgName = _S8("UNKNOWN");
	TUint n = MessageId().iId;
	if (n > sizeof(szMessageId2Name)/sizeof(szMessageId2Name[0]) || MessageId().Realm() != TCFMessage::ERealmId)
	    {
   	    msgName = _S8("ECFCustom");
	    }
	else if (n>0)
	    {
	    msgName = szMessageId2Name[n-1];
	    }
	return msgName;
	}

inline const TText8* Messages::TNodeSignal::ActivityIdToName() const
    {
    switch (ActivityId()&0xFF)
    	{
    	case ECFConnPolicyRequestActivity:				return _S8("ECFConnPolicyRequestActivity");
    	case ECFPolicyConnectionActivity:					return _S8("ECFPolicyConnectionActivity");

		default:
			return _S8("Activity Unknown");
    	}
    }
#endif
*/


#endif
// SYMBIAN_IPPROTOMESSAGES_H
