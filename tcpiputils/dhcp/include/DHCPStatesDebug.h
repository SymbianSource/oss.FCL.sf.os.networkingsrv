// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains functionality to publish states as they are processed, this is so test harnesses can keep an eye on DHCP. 
// 
//

/**
 @file DHCPStatesDebug.h
*/

#ifndef __DHCPSTATESDEBUG_H__
#define __DHCPSTATESDEBUG_H__

#include <e32property.h>
#include "DHCP_Std.h"

//-- interface debug control functions numbers
const TUint KDHCP_GetPubSubMonitorHandle = KDhcpInterfaceDbgIoctl | KConnWriteUserDataBit | 1;	//-- obtain handle for client to monitor DHCP session
																								//--   (see DHCPStatesDebug.h)

namespace DHCPDebug
{

#ifdef _DEBUG_DHCP_STATE_NAMES
const TText* State_Name[] =
	{
	(const TText*)L"EDHCPSelect",
	(const TText*)L"EDHCPInformationConfig",
	(const TText*)L"EDHCPRebootConfirm",
	(const TText*)L"EDHCPRequest",
	(const TText*)L"EDHCPRebind",
	(const TText*)L"EDHCPWaitForDADBind",
	(const TText*)L"EDHCPRenew",
	(const TText*)L"EDHCPRelease",
	(const TText*)L"EDHCPRemoveConfiguredAddress",
	(const TText*)L"EDHCPDecline",
	(const TText*)L"EDHCPIP4Select",
	(const TText*)L"EDHCPIP6ListenToNeighbor",
	(const TText*)L"EDHCPIP6Solicit",
	(const TText*)L"EDHCPIP6Select",
	(const TText*)L"EDHCPIP6InformRequest",
	(const TText*)L"EDHCPIP6Release",
	(const TText*)L"EDHCPIP6Decline",
	(const TText*)L"EDHCPIP6Confirm",
	(const TText*)L"EDHCPIP6WaitForDAD",
	(const TText*)L"EDHCPIP6Renew",
	(const TText*)L"EDHCPIP6Rebind",
	(const TText*)L"EDHCPIP6Reconfigure",
	(const TText*)L"EDHCPIPAddressAcquisition",
	(const TText*)L"EStateUnknown"
	};
#else
extern const TText* State_Name[];
#endif


typedef enum
	{
	EDHCPSelect = 0,
	EDHCPInformationConfig = 1,
	EDHCPRebootConfirm = 2,
	EDHCPRequest = 3,
	EDHCPRebind = 4,
	EDHCPWaitForDADBind = 5,
	EDHCPRenew = 6,
	EDHCPRelease = 7,
	EDHCPRemoveConfiguredAddress = 8,
	EDHCPDecline = 9,
	EDHCPIP4Select = 10,
	EDHCPIP6ListenToNeighbor = 11,
	EDHCPIP6Solicit = 12,
	EDHCPIP6Select = 13,
	EDHCPIP6InformRequest = 14,
	EDHCPIP6Release = 15,
	EDHCPIP6Decline = 16,
	EDHCPIP6Confirm = 17,
	EDHCPIP6WaitForDAD = 18,
	EDHCPIP6Renew = 19,
	EDHCPIP6Rebind = 20,
	EDHCPIP6Reconfigure = 21,
	EDHCPIPAddressAcquisition = 22,
	EStateUnknown = 23
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	,
	EDHCPProvideOffer = 24,
	EDHCPSendAckNak = 25,
	EDHCPWaitForClientMsgs = 26,
	EDHCPIP4HandleClientMsgs = 27,
	EDHCPIP4SendRequestResponse = 28,
	EDHCPIP4SendInformResponse = 29,
	EDHCPIP4BindServer = 30
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	} State;

typedef enum
	{
	EUnknown = 0,
	ENotReady = 1,
	EReady = 2
	} Readiness;


// offset from the handle value.. as handle is a pointer to the state machine, we'd better not have too many of these :-)
//
typedef enum
	{
	EState      = 0,
	EReadiness  = 1
	} Attribute;



} // end of namespace


#define DHCP_DEBUG_PUBLISH(w,x)		{RProperty a; TUid u = TUid::Uid(0x101fd9c5); a.Define(u, (TUint)(w), RProperty::EInt); a.Set(u, (TUint)(w), (TInt)(x)); a.Close();}
#define DHCP_DEBUG_QUERYL(w,x)		{TAutoClose<RProperty> a; TUid u = TUid::Uid(0x101fd9c5); User::LeaveIfError( a.iObj.Attach(u, (TUint)(w)) ); a.PushL(); a.iObj.Get((TInt&)(x)); a.Pop();}
#define DHCP_DEBUG_SUBSCRIBEL(w)	{TAutoClose<RProperty> a; TUid u = TUid::Uid(0x101fd9c5); User::LeaveIfError( a.iObj.Attach(u, (TUint)(w)) ); a.PushL(); TRequestStatus pubStat; a.iObj.Subscribe(pubStat); User::WaitForRequest( pubStat ); User::LeaveIfError( pubStat.Int() ); a.Pop();}



#ifdef _DEBUG

#define DHCP_DEBUG_PUBLISH_STATE(x)  DHCP_DEBUG_PUBLISH( (DHCPDebug::EState+(TInt)iStateMachine) , (x) )
#define DHCP_DEBUG_PUBLISH_READY(x)  DHCP_DEBUG_PUBLISH( (DHCPDebug::EReadiness+(TInt)iDhcpStateMachine) , (x) )

#else // _DEBUG

#define DHCP_DEBUG_PUBLISH_STATE(x)
#define DHCP_DEBUG_PUBLISH_READY(x)

#endif // _DEBUG

#endif // __DHCPSTATESDEBUG_H__

