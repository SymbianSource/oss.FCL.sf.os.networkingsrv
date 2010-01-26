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
// ip6_frag.h - hooks for IPv6 fragment header
// The handler for IPv6 Fragment Header
//



/**
 @internalComponent
*/
#ifndef __IP6_FRAG_H__
#define __IP6_FRAG_H__

#include <in_bind.h>

class CFragmentHeaderHook : public CIp6Hook
	{
public:
	CFragmentHeaderHook(MNetworkService *aNetwork) : iNetwork(aNetwork) {}
	~CFragmentHeaderHook() {}
	static CFragmentHeaderHook *NewL(MNetworkService *aNetwork);
	/**
	* Fragments complete IPv4 or IPv6 packet into fragments.
	*
	* When called, it has been determined that the packet needs
	* to be fragmented.
	*
	* @param aPacket	The packet
	* @param aMtu		The MTU (max frag size)
	* @retval aFragQ	The fragmented result.
	*/
	virtual void Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ) = 0;
	/**
	* Adds a new IPv4 fragment into reassembly.
	*
	* @param aPacket	The fragment packet
	* @param aInfo		The info associated with the fragment
	* @param aHdr		The IPv4 header of the fragment
	*/
	virtual TInt Ip4ApplyL(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, const TInet6HeaderIP4 &aHdr) = 0;
	virtual void ConstructL() = 0;

protected:
	MNetworkService *iNetwork;
	};

#endif
