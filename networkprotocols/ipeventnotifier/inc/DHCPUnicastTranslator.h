// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file DHCPUnicastTranslator.h
 @internalTechnology
*/


#ifndef __DHCPUNICASTTRANSLATOR_H__
#define __DHCPUNICASTTRANSLATOR_H__

#include <e32std.h>
#include <ip6_hook.h>


/**
 *  DHCP Unicast Translator hook class.
 *   Detects unicast BOOTP traffic and converts it to broadcast
 */
class CDHCPUnicastTranslator : public CIp6Hook
	{
public:

	CDHCPUnicastTranslator(CProtocolInet6Binder& aIP);
	~CDHCPUnicastTranslator();

	// Fills in structure with info about this protocol
	void Identify(TServerProtocolDesc* aProtocolDesc)const;
	static void FillIdentification(TServerProtocolDesc& anEntry);

	// Called by the stack for every incoming packet
	TInt ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);

private:

	CProtocolInet6Binder& iParent;
	};



#endif // __DHCPUNICASTTRANSLATOR_H__



