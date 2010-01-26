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
// DHCPv6 Message receiver implementation file. Receiver keeps reading from socket untill the whole 
// message has been read
// 
//

/**
 @file
*/

#include "DhcpIP6MsgRcvr.h"
#include "DHCPServer.h"

#ifdef _DEBUG
#include <e32property.h>
#endif

CAsynchEvent* CDhcpIP6MessageReader::ProcessL(TRequestStatus& aStatus)
/**
  * Keeps reading from socket util the full UDP datagram has been received
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDhcpIP6MessageReader::ProcessL")));
	
	DHCPv6::CDHCPMessageHeaderIP6* v6Msg = DHCPIPv6().DhcpMessage();
	TPtr8 v6MsgDes = v6Msg->Message().Des();
	TInt length = v6MsgDes.Length();

   	if (iIncomingMsgDataPtr.Length() == 0)
		{
		v6MsgDes.Zero();
		// Create a pointer to the memory location represented by the HBufC 
		// containing the incoming message
		iIncomingMsgDataPtr.Set(const_cast<TUint8*>(v6MsgDes.Ptr()), 0, v6MsgDes.MaxLength());
		}
	else if (iUnreadLength() > 0) 
		{
		// Resize the message buffer for the remaining data
		iStateMachine->ReAllocL(iUnreadLength());
      
		// Move the access pointer to the end of the previously read data ready
		// for the start of the remainder
		iIncomingMsgDataPtr.Set(const_cast<TUint8*>(v6MsgDes.Ptr() + v6MsgDes.Length()), 0, iUnreadLength());		
		
		v6MsgDes.SetLength(length + iIncomingMsgDataPtr.Length());
		}
	else
		{	
		// We've finished reading the message, complete the request
		v6MsgDes.SetLength(length + iIncomingMsgDataPtr.Length());
		iIncomingMsgDataPtr.SetLength(0);

		TRequestStatus* p = &aStatus;
		User::RequestComplete(p, KErrNone);

		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDhcpIP6MessageReader::ProcessL - finished")));
		return iNext;
		}

	iRcvAddr.SetAddress(KAFUnspec);
#ifdef _DEBUG
	// Simulate initialisation, renewal or rebind failure by using the wrong port.
	if( ( CDHCPServer::DebugFlags() & KDHCP_FailDiscover ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRenew ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRebind ) )
		{
		iRcvAddr.SetPort(KDhcpv6WrongSrcPort);
		}
	else
		{
		TInt destPort;
		RProperty::Get(KMyPropertyCat, KMyPropertyDestPortv6, destPort);
		iRcvAddr.SetPort(destPort - 1);
		}
#else
	iRcvAddr.SetPort(KDhcpv6SrcPort);
#endif	

	DHCPIPv6().iSocket.RecvFrom(iIncomingMsgDataPtr, iRcvAddr, KSockSelectReadContinuation, aStatus, iUnreadLength);
	
	return this;
	}
