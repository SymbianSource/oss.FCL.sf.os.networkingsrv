// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Hardware address manager class declaration.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __DHCPHWADDRMANAGER_H__
#define __DHCPHWADDRMANAGER_H__

#include <e32base.h>
#include <e32cmn.h>
/**
  * This class receives the hardware address from the application which 
  * is loading the DHCP server, and preserves it for provisioning.
  *
  * @internalTechnology
  */
class CDhcpHwAddrManager : public CBase
{
public:
	static CDhcpHwAddrManager* NewL();
	TBool IsHwAddressProvisioned();
	void Insert(const Uint64& aInt);
	TBool Provisioned( const Uint64& aInt);
	TBool IsAnyClientAllowed();
	~CDhcpHwAddrManager();
protected:
	CDhcpHwAddrManager();
	void ConstructL();
private:
	RArray<Uint64> iHardwareAddressList;
};

#endif //__DHCPHWADDRMANAGER_H__
