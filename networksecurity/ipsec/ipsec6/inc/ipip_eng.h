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
// ipip_eng.h - IPv6/IPv4 in IPv6/IPv4 for IPSEC
// The IPSEC IP-in-IP engine (not really bound to IPSEC very tightly)
//



/**
 @internalComponent
*/
#ifndef __IPIP_ENG_H__
#define __IPIP_ENG_H__

#include "sa_spec.h"
//
//  TIpsecIPIP
//
class TIpsecIPIP
	{
public:
	TInt Overhead(const TIpAddress &aTunnel) const;
	TInt ApplyL(const TIpAddress &aTunnel, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	TInt ApplyL(TIpAddress &aTunnel, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo);
private:
	inline TUint AllocId();

	TInt iNextId;
	};


// IPv4 Packed Id number generation
TUint TIpsecIPIP::AllocId()
	{
	if (iNextId >= 0xffff)
		iNextId = 1;
	else
		++iNextId;
	return iNextId;
	}
#endif
