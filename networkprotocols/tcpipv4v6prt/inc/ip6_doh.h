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
// ip6_doh.h - hooks for IPv6 protocol message options
// Default IPv6 Options Hooks (Hop-by-Hop and Destination Options)
// The default handler for IPv6 Destination options which is
// implicitly bound to the protocol processing loop by the ip6.cpp.
//



/**
 @internalComponent
*/
#ifndef __IP6_DOH_H__
#define __IP6_DOH_H__

#include <in_bind.h>

class CDefaultOptionsHook : public CIp6Hook
	{
public:
	CDefaultOptionsHook(MNetworkService *aProtocol) : iProtocol(aProtocol) {}
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);
protected:
	MNetworkService *iProtocol;
	};


//
//	*NOTE*
//		Because current default implementation only skips the options
//		the same code works for both Destination and Hop-by-hop options.
//		(This will not be true later, thus other than InitL() method
//		need to be overridden also!
//
class CDestinationOptionsHook : public CDefaultOptionsHook
	{
public:
	CDestinationOptionsHook(MNetworkService *aProtocol) : CDefaultOptionsHook(aProtocol) {}
	void ConstructL();
	};

class CHopOptionsHook: public CDefaultOptionsHook
	{
public:
	CHopOptionsHook(MNetworkService *aProtocol) : CDefaultOptionsHook(aProtocol) {}
	void ConstructL();
	};
#endif
