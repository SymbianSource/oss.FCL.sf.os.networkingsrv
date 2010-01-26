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
// sc.h - IPv6/IPv4 security check protocol
//



/**
 @internalComponent
*/
#ifndef __SC_H__
#define __SC_H__

#include "ipsec.h"
//
//	This header contains "private" data between sc_prt.cpp and sc_sap.cpp.
//	It should not be included by anyone else.

class MSecurityPolicyManager
	{
public:
	/**
	* Set a new policy for the IPSEC.
	*
	* @param aPolicy	The policy definition string
	* @retval aOffset	Parsing offset into the policy string. In and out parameter.
	*
	* @return KErrNone, if policy installed, and error otherwise.	
	*/
	virtual TInt SetPolicy(const TDesC &aPolicy, TUint &aOffset) = 0;
	/**
	* Deliver a copy of a packet to all policy sockets.
	*
	* @param aPacket The packet
	*/
	virtual void Deliver(RMBufPacketBase& aPacket) = 0;
	};

//
// Secpol Socket Provider Base
//

class CProviderSecpol: public CProviderIpsecBase
	{
public:
	CProviderSecpol(MSecurityPolicyManager& aProtocol);

	void Start();
	TUint Write(const TDesC8& aDesc,TUint options, TSockAddr* anAddr=NULL);
public:
	// Used by CProtocolSecpol only.
	TBool IsReceiving();
private:
	MSecurityPolicyManager& iProtocol;
	TInt iPacketsDropped;	// Count packets dropped due "congestion"
	};

#endif
