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
// MacTableManager.cpp
// Hardware address manager class implementation.
// 
//

/**
 @file
 @internalTechnology
*/
#include "dhcphwaddrmanager.h"

  /**
  * Constants used to compare with the received hardware address. Macro __GNUC__ is defined to 
  * gccxml build error "value too large for type 'long'" avoid the error, though it can expand 
  * the 'Uint64' to 'long long unsigned int'
  *
  *	@internalTechnology
  */
#ifdef __GNUC__
const Uint64 KAnyMac = 0xFF;
const Uint64 KResetMac = 0x00;
#else
const Uint64 KAnyMac = 0xFFFFFFFFFFFF;
const Uint64 KResetMac = 0x000000000000;
#endif //__GNUC__

/**
  * Constructor
  *
  *	@internalTechnology
  *
  */
CDhcpHwAddrManager::CDhcpHwAddrManager()
	{
		
	}

/**
  * Destructor frees the list
  *
  *	@internalTechnology
  *
  */
CDhcpHwAddrManager::~CDhcpHwAddrManager()
	{
	iHardwareAddressList.Close();
	}

/**
  * Static function to create the instance of CDhcpHwAddrManager.
  *
  *	@internalTechnology
  *
  */
CDhcpHwAddrManager* CDhcpHwAddrManager::NewL()
	{
	return new (ELeave) CDhcpHwAddrManager();
	}

/**
  * Function to check that atleast any one of the harware address is provisioned or not. 
  *
  *	@internalTechnology
  * @return - ETrue if the list has at least one entry, otherwise EFalse.
  */
TBool CDhcpHwAddrManager::IsHwAddressProvisioned()
	{
	return (iHardwareAddressList.Count() > 0) ? ETrue : EFalse;
	}

/**
  * Function inserts the provided Hardware Address into RArray. If the received address (KResetMac)
  * is to reset the iHardwareAddressList, then clears the list.
  *
  *	@internalTechnology
  * @param aInt - 48 bits of hardware address.
  */
void CDhcpHwAddrManager::Insert(const Uint64& aInt)
	{
	if(aInt == KResetMac)
		{
		iHardwareAddressList.Close();
		return;
		}
	TInt index = iHardwareAddressList.Find(aInt, TIdentityRelation<Uint64>());
	if(index == KErrNotFound)
		{
		iHardwareAddressList.Append(aInt);
		}
	}

/**
  * Function identfies whether the received Hardware Address is Provisioned by the control 
  * application or not.
  *
  *	@internalTechnology
  * @param aInt - 48 bits of hardware address.
  * @return - ETrue, if the received address exists in the list, otherwise EFalse.
  */
TBool CDhcpHwAddrManager::Provisioned(const Uint64& aInt)
	{	
	
	return  ( iHardwareAddressList.Find(aInt, TIdentityRelation<Uint64>()) == KErrNotFound )? EFalse : ETrue;
	}

/**
  * Function identfies whether the DHCP server shall provide IP to any client or not.
  * If the provided hardware address is KAnyMac, then DHCP assigns IP in FCFS basis.
  *
  *	@internalTechnology
  * @return - ETrue if the application provides KAnyMac to DHCP server, otherwise EFlase.
  */
TBool CDhcpHwAddrManager::IsAnyClientAllowed()
	{
	return  ( iHardwareAddressList.Find(KAnyMac, TIdentityRelation<Uint64>()) == KErrNotFound )? EFalse : ETrue;
	}
