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
// ip6_rth.h - default hook for IPv6 routing header
// The default handler for IPv6 Routing Header which is implicitly
// bound to the protocol processing loop by the ip6.cpp.  Only only
// Routing Header with Type=0 is processed, others are left
// untouched.
//



/**
 @internalComponent
*/
#ifndef __IP6_RTH_H__
#define __IP6_RTH_H__

#include <in_bind.h>

class CRoutingHeaderHook : public CIp6Hook
	{
public:
	CRoutingHeaderHook(MNetworkService *aProtocol) : iProtocol(aProtocol) {}
	void ConstructL();
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);
private:
	MNetworkService *iProtocol;
	};

#endif
