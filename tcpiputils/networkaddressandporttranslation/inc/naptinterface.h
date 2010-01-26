// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file 
 @publishedPartner
 @released
*/

#ifndef __NAPT_INTERFACE_H__
#define __NAPT_INTERFACE_H__


const TUint KSolNapt = 0x10283515;

const TUint KSoNaptSetup =1;
const TUint KSoNaptUplink =2;

#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
const TUint KSoNaptProvision =3;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION

class TInterfaceLockInfo
/**This is stack based class for information required by napt.This class will be scoped as internal
 * teconology.The information provided by napt will use it. 
 */
	
	{
	public:
	
	// IAP of the public interface.
	TUint32 iPublicIap;
	
	//IAP of the private interface.
	TUint32 iPrivateIap;
	
	//Private interface IP 
	TInetAddr iPrivateIp;
	
	//Public interface IP
	TInetAddr iPublicIp;
	
	//netmask length need to translated.Netmask will be 8 for class A. 16 for class B and 24 class C.
	//this is specificaly for napt. Others can use it accordingly
	TUint iNetmaskLength;
	//Provisioned IP 
	TUint32 iProvisionedIp;
	
	//Interface Index
	TUint32 iIfIndex;
		
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION	
	//To enable or disable uplink access.
	TBool iUplinkAccess;
#endif	

	};
	
class TUplinkInfo
	{
	public:
	//IAP of the private interface.
	TUint32 iPrivateIap;
	// IAP of the public interface.
	TUint32 iPublicIap;
	};	
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
class TIpForwardingInfo
/**This class is for provisioning NAPT. It is used only by control application.
 */
	{
	public:
	//To enable or disable uplink access.
	TBool iUplinkAccess;
	//private iap settings to be enabled
	TUint32 iPrivateIap;
	};
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#endif //__NAPT_INTERFACE_H__
