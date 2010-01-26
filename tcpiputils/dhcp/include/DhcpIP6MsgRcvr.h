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
// DHCPv6 Message receiver header file. Receiver keeps reading from socket untill the whole 
// msg has been read
// 
//

/**
 @file
*/

#ifndef DHCPIP6MSGRCVR_H
#define DHCPIP6MSGRCVR_H

#include "DhcpIP6Msg.h"
#include "DHCPIP6StateMachine.h"
#include <comms-infras/asynchevent.h>

class CDHCPIP6StateMachine;
class CDhcpIP6MessageReader : public CAsynchEvent
/**
  * Implements DHCPv6 message reader. It reads a datagram untill it has been fully read
  * out of ESOCK meaning that the whole message has been read
  *
  * @internalTechnology
  */
{
public:
   CDhcpIP6MessageReader(CDHCPIP6StateMachine& aDHCPIP6StateMachine) :
      CAsynchEvent(&aDHCPIP6StateMachine),
      iIncomingMsgDataPtr(NULL, 0)
      {
      }

   virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}

protected:
	TPtr8 iIncomingMsgDataPtr;
	TSockXfrLength iUnreadLength;
	TInetAddr iRcvAddr;
};

#endif
