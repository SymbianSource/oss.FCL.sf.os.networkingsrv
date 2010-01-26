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
// ah_eng.h - IPv6/IPv4 IPSEC authentication header
// The IPSEC Authentication Header (AH) packet processing class
//



/**
 @internalComponent
*/
#ifndef __AH_ENG_H__
#define __AH_ENG_H__

#include "sa_spec.h"
//
//	TIpsecAH
//
class TIpsecAH
	/**
	* IPsec AH engine.
	*
	* This implements the packet processing for the
	* IPsec Authentication Header (AH).
	*/
	{
public:
	TIpsecAH(MAssociationManager *aProtocol) : iManager(aProtocol) {}
	TInt ApplyL(CSecurityAssoc &aSa, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	TInt ApplyL(CSecurityAssoc* &aSa, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo);
	TInt Overhead(const CSecurityAssoc &aSa) const;
private:
	MAssociationManager *iManager;
	};

#endif
