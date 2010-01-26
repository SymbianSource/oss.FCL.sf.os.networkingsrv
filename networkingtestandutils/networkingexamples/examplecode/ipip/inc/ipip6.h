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

#ifndef __IPIP6_H__
#define __IPIP6_H__

#include <in_sock.h>
#include <ext_hdr.h>

// just 2 random numbers
const TUint KAfInetIPIPHookEx = 0x0666;

const TUint KProtocolInetIPIPHookEx = 0x0333;

_LIT(KProtocolIPIPName, "t_Protocol");

enum TIPIPHookExPanic
	{
	EIpipPanic_BadBind,
	EIpipPanic_BadHeader,
	EIpipPanic_BadCall,
	EIpipPanic_BadIndex,
	EIpipPanic_IoctlFailed,
	EIpipPanic_CorruptPacketIn,
	EIpipPanic_Ip6NotSupported
	};

GLREF_C void Panic(TIPIPHookExPanic aPanic);

// Definitions for the IPIP policy
enum TIPIPPolicyOptions
	{
	EIPIPDeletePolicy,
	EIPIPAddPolicy,
	EIPIPQueryPolicies
	};

// Defines the tunnel endpoints
struct TIPIPActionSpec
	{
	TIPIPActionSpec(const TInetAddr& aSrc, const TInetAddr& aDest) 
		:iSrcAddr(aSrc), iDestAddr(aDest) 
	{}

	TInetAddr iSrcAddr;
	TInetAddr iDestAddr;
	};

// Identifies the flows the tunnelling has to be applied to
struct TIPIPPacketSpec
	{
	TIPIPPacketSpec(const TInetAddr& aSrc, const TInetAddr& aDest) 
		:iSrcAddr(aSrc), iDestAddr(aDest) 
	{}

	TInetAddr iSrcAddr;
	TInetAddr iDestAddr;
	};

struct TIPIPPolicyMsg
	{
	TIPIPPolicyMsg(const TIPIPPacketSpec& aPacket, const TIPIPActionSpec& aAction, TIPIPPolicyOptions aOptions)
		:iPacket(aPacket), iAction(aAction), iOption(aOptions)
	{}

	TIPIPPacketSpec iPacket;
	TIPIPActionSpec iAction;
	TIPIPPolicyOptions iOption;
	};
typedef TPckgBuf<TIPIPPolicyMsg> TIPIPMsg;

/*
// Defines the message
static const TInt KIPIPMsgMaxLen = 400;
class TIPIPMessage : public TBuf8<KIPIPMsgMaxLen>
	{
public:
	// Construct the message with relevant args
	TIPIPMessage(const TIPIPActionSpec& aAction, const TIPIPPacketSpec& aPacket, TIPIPPolicyOptions aOption);

	TIPIPActionSpec& Action();
	TIPIPPacketSpec& Packet();
	TIPIPPolicyOptions& Option();
private:
	};
*/

#endif //__IPIP6_H__
