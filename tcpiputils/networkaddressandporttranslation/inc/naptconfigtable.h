// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// NAPT (Network Address & Port translation Code )
// 
//

/**
 @file 
 @internalComponent
 @prototype
*/

#ifndef __NAPTCONFIGTABLE_H
#define __NAPTCONFIGTABLE_H
#include<in_sock.h>
#include "naptinterface.h"

class CSapNapt;
class CProtocolNapt;

class TNaptConfigInfo 
	{
    public:
    	
	TInetAddr iPrivateIp;
	TInetAddr iPublicGatewayIP;
	TInt32    iPrivateIap;
	TInt32    iPublicIap;
	TUint32   iScopedest; //this is network ID of the interface.
	TUint32   iScopeSrc;
	TUint32   iIfIndex;
	TUint     iNetMaskLength;
	
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION	
    TBool iUplinkAccess;
	TUint32   iProvisionedIp;
#endif	
		
	};


class CNaptClientConfigMgr
	{
	public:
	
	~CNaptClientConfigMgr();
	static CNaptClientConfigMgr* NewL(CProtocolNapt& aProtocol);
	//This method finds config in the list based on IapId passed
	TNaptConfigInfo* FindConfig(TUint32 aIfIndex, TInt aIfFlag = 1);
	//This method removes config based on IapId and removes it from the list
	void DeleteConfig(TUint32 aIapId);
	//This method deletes config based on pointer and removes it from the list
	void DeleteConfig(TNaptConfigInfo* aConfigInfo);
	//This methods add info structure to array list
	void AddConfigL(const TNaptConfigInfo* aConfigInfo);
	//This method updates the config received in the list
	//Creates new if the config record is not already exising in the list
	TInt UpdateConfigL(const TInterfaceLockInfo* aInfo, CSapNapt* aSap);
		
	private:
	
	CNaptClientConfigMgr(CProtocolNapt& aProtocol);
	void ConstructL();
	RPointerArray<TNaptConfigInfo> iConfigList;
	CProtocolNapt& iProtocol;		
		
	};
	

	

#endif //naptconfigtable.h
