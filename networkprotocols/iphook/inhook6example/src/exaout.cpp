// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// exaout.cpp - Outbound plugin example module
//

#include "exaout.h"

CExaoutFlowInfo::CExaoutFlowInfo( CFlowContext &aFlow) : iFlow(aFlow)
	/**
	* Constructor.
	*
	* @param aFlow	The flow context.
	*/
	{
	}

CExaoutFlowInfo::~CExaoutFlowInfo()
	/**
	* Destructor.
	*
	* The #iMgr value indicates whether object is still linked
	* or not (because, depending on how CProtocolExaout::NetworkDetached()
	* or destructor has been implemented, this could be a <em>floating flow</em>,
	* which is not attached to the protocol any more).
	*
	* However, if this is still attached, then must detach the the flow
	* from the protocol instance.
	*/
	{
/** @code */
	if (iMgr)
		{
		iMgr = NULL;
		iDLink.Deque();
		}
/** @endcode */
	}


MFlowHook *CProtocolExaout::OpenL(TPacketHead& /*aHead*/, CFlowContext* aFlow)
	/**
	* A new flow is opening.
	*
	* This example attaches to every flow.
	*
	* @param aFlow	The flow context.
	*/
	{
	CExaoutFlowInfo* info = new (ELeave) CExaoutFlowInfo(*aFlow);
	iFlowList.AddLast(*info);	// Save the flow info for later use
	info->iMgr = this;			// info is now linked!
	return info;
	}

TInt CExaoutFlowInfo::ReadyL(TPacketHead& /*aHead*/)
	/**
	* A flow has been connect to interface.
	*
	* This is called when the flow has been connected to the interface,
	* source address has been selected.
	*
	* The example does not have any reason to block the connection
	* process, and thus always returns READY.
	*
	* @return EFlow_READY always.
	*/
	{
/** code */
	return EFlow_READY;
/** @endcode */
	}

//
// Applies all the remaining modifications needed for every outgoing packet
//
TInt CExaoutFlowInfo::ApplyL(RMBufSendPacket& /*aPacket*/, RMBufSendInfo& /*aInfo*/)
	/**
	* A outbound packet is being sent to the flow.
	*
	* The example is dummy, and does do anything with the packet,
	* return always KErrNone (= packet processed). For some example,
	* see MFlowHook::ApplyL example.
	*
	* @return KErrNone
	*/
	{
/** @code */
	//
	// This would contain any packet processing code.
	// 
	return KErrNone;
/** @endcode */
	}

void CExaoutFlowInfo::Close()
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
/** @code */
	if (--iRefs < 0)
		{
		delete this;
		}
/** @endcode */
	}

void CExaoutFlowInfo::Open()
	/**
	* The reference counting, one more.
	*
	* The MFlowHook objects <b>must implement</b> reference counting semantics
	* and they must automaticly self-destruct after the last reference
	* is removed.
	*/
	{
/** @code */
	iRefs++;
/** @endcode */
	}
