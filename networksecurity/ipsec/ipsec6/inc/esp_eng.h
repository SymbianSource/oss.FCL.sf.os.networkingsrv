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
// esp_eng.h - IPv6/IPv4 IPSEC encapsulating security payload
// The IPSEC Encapsulating Security Payload (ESP) packet processing
// class
//



/**
 @internalComponent
*/
#ifndef __ESP_ENG_H__
#define	__ESP_ENG_H__

#include "sa_spec.h"
//
//	TIpsecESP
//
class TIpsecESP
	/**
	* IPsec ESP engine.
	*
	* This implements the packet processing for the
	* IPsec Encapsulation Security Payload (ESP).
	*/
	{
public:

	TIpsecESP(MAssociationManager *aProtocol) : iManager(aProtocol) {}
	TInt ApplyL(CSecurityAssoc &aSa, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, RMBufAllocator &aBufAllocator);
	TInt ApplyL(CSecurityAssoc* &aSa, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, RMBufAllocator &aBufAllocator);
	TInt Overhead(const CSecurityAssoc &aSa) const;
private:
	MAssociationManager *iManager;
	};

#endif
