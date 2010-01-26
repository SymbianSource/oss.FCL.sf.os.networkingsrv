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
//

/**
 @file addressinfo.cpp
 @internalTechnology
 @prototype
*/

#include "addressinfo.h"
#include <es_sock.h>

CAddressInfoFlowInfo::CAddressInfoFlowInfo( CFlowContext &aFlow) : iFlow(aFlow)
	/**
	* Constructor.
	*
	* @param aFlow	The flow context.
	*/
	{
	}

CAddressInfoFlowInfo::~CAddressInfoFlowInfo()
	/**
	* Destructor.
	*
	* The #iMgr value indicates whether object is still linked
	* or not (because, depending on how CProtocolAddressInfo::NetworkDetached()
	* or destructor has been implemented, this could be a <em>floating flow</em>,
	* which is not attached to the protocol any more).
	*
	* However, if this is still attached, then must detach the the flow
	* from the protocol instance.
	*/
	{
	if (iMgr)
		{
		iMgr = NULL;
		iDLink.Deque();
		}
	}


class CIPProtoBinder;
MFlowHook *CProtocolAddressInfo::OpenL(TPacketHead& /*aHead*/, CFlowContext* aFlow)
	/**
	* A new flow is opening.
	*
	* @param aFlow	The flow context.
	*/
	{
	CAddressInfoFlowInfo* info = new (ELeave) CAddressInfoFlowInfo(*aFlow);
	
	CIPProtoBinder *binder = 0;
	
	__CFLOG_VAR((KIPAddrInfoHookTag1, KIPAddrInfoHookTag1, _L8("NewFlow lport %d rport %d proto %d"),aFlow->LocalPort(),aFlow->RemotePort(),aFlow->Protocol()));
	if (iAddrInfo.Match(aFlow->LocalPort(),aFlow->RemotePort(),aFlow->Protocol(), binder))
		{
		__CFLOG_VAR((KIPAddrInfoHookTag1, KIPAddrInfoHookTag1, _L8("NewFlow matched id %x"), binder));
		info->iBinder = binder;
		}
	else
		{
		info->iBinder = 0;
		}
	
	iFlowList.AddLast(*info);	// Save the flow info for later use
	info->iMgr = this;			// info is now linked!
	return info;
	}

TInt CAddressInfoFlowInfo::ReadyL(TPacketHead& /*aHead*/)
	/**
	* A flow has been connect to interface.
	*
	* This is called when the flow has been connected to the interface,
	* source address has been selected.
	*
	* The hook does not have any reason to block the connection
	* process, and thus always returns READY.
	*
	* @return EFlow_READY always.
	*/
	{
	return EFlow_READY;
	}

//
// Applies all the remaining modifications needed for every outgoing packet
//
TInt CAddressInfoFlowInfo::ApplyL(RMBufSendPacket& aPacket, RMBufSendInfo& aInfo)
	/**
	* A outbound packet is being sent to the flow.
	*
	* @return KErrNone
	*/
	{
	aInfo.iDstAddr.SetPort(reinterpret_cast<TInt>(iBinder));
	(void)aPacket;
	(void)aInfo;

	return KErrNone;
	}

void CAddressInfoFlowInfo::Close()
	/**
	* The reference counting, one less.
	*
	* The MFlowHook objects <b>must implement</b> reference counting semantics
	* and they must automaticly self-destruct after the last reference
	* is removed.
	*
	* A object returned by OpenL counts as one reference. As each
	* flow gets own object, the initial value after CBase construction
	* is 0. This is not touched, and thus, the object is deleted when
	* the count goes negative.
	*/
	{
	if (--iRefs < 0)
		{
		delete this;
		}
	}

void CAddressInfoFlowInfo::Open()
	/**
	* The reference counting, one more.
	*
	* The MFlowHook objects <b>must implement</b> reference counting semantics
	* and they must automaticly self-destruct after the last reference
	* is removed.
	*/
	{
	iRefs++;
	}


