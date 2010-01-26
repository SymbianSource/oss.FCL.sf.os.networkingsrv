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
// implementation of UDP and TCP tranlation
// 
//

/**
 @file
 @internalTechnology 
*/


#include "hookdefs.h"
#include "naptinterface.h"
#include "naptutil.h"
#include "naptconfigtable.h"

void CNaptClientConfigMgr::ConstructL()
/** 
 * This is a phase2 constructor for this object.
 * @param None
 * @return None
 * 
 **/
	{
	
	}	
		
CNaptClientConfigMgr :: CNaptClientConfigMgr(CProtocolNapt& aProtocol):iProtocol(aProtocol)
/** 
 * This is a constructor for config manager class.
 * @param None
 * @return None
 * 
 **/
	{
	
	}

CNaptClientConfigMgr :: ~CNaptClientConfigMgr()
/** 
 * This is a destructor for config manager class.
 * @param None
 * @return None
 * 
 **/
	{
	iConfigList.ResetAndDestroy();
	}

CNaptClientConfigMgr* CNaptClientConfigMgr::NewL(CProtocolNapt& aProtocol)
/** 
 * This is a static method to create an instance of CNaptClientConfigMgr class
 * @param CProtocolNapt& - Reference to protocol instance object
 * @return CNaptClientConfigMgr* - Pointer to created object intance
 * 
 **/
	{
	CNaptClientConfigMgr* self = new(ELeave) CNaptClientConfigMgr(aProtocol);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}
	

TNaptConfigInfo* CNaptClientConfigMgr :: FindConfig(TUint32 aIfIndex, TInt aIfFlag)
/** 
 * This method finds a configuration from config list based on interface index or private IAP.
 * @param aIfIndex - Interface index
 * @return None
 * 
 **/
	{
	TNaptConfigInfo* cinfo = NULL;
	TUint32 ifindex = 0;
	/*Loop through the list to find an entry corresponding to aIapId*/
	for(TUint index = 0; index < iConfigList.Count(); index++)
		{
		(aIfFlag)?(ifindex = iConfigList[index]->iIfIndex):(ifindex = iConfigList[index]->iPrivateIap);
		if(ifindex != aIfIndex)
			continue;
		else
			{
			/*Found a matching entry here so break out of the loop*/
			cinfo = iConfigList[index];
			break;	
			}
			
		}
		/*return configuration info*/
		return cinfo;
	}
	
void CNaptClientConfigMgr :: DeleteConfig(TUint32 aIapId)
/** 
 * This method deletes a configuration from config list based on IAP Id.
 * @param aIapId - IAP Identifier
 * @return None
 * 
 **/
	{
	/*Loop through the list to find an entry corresponding to aIapId*/
	for(TUint index = 0; index < iConfigList.Count(); index++)
		{
		if(iConfigList[index]->iPrivateIap != aIapId)
			continue;
		else
			{
			/*Found a matching entry here so break out of the loop*/
			delete iConfigList[index];
			iConfigList[index] = NULL;
			iConfigList.Remove(index);
			break;	
			}
        }
        iConfigList.Compress();
	}

void CNaptClientConfigMgr :: DeleteConfig(TNaptConfigInfo *aConfigInfo)
/** 
 * This method deletes a configuration from config list using a pointer
 * @param aIfIndex - Interface index
 * @return None
 * 
 **/
	{
	TInt index = iConfigList.Find(aConfigInfo);
	if(index != KErrNotFound)
		{
		delete iConfigList[index];
		iConfigList[index] = NULL;
		iConfigList.Remove(index);
		iConfigList.Compress();
		}
		
	}


TInt CNaptClientConfigMgr::UpdateConfigL(const TInterfaceLockInfo* aInfo, CSapNapt* aSap)
/** 
 * This method updates the configuration in the list. In case configuration is already present it updates
 * the existing configuration corresponsing to interface index else it creates a new config and adds it
 * the configuration list.
 * @param const TInterfaceLockInfo* aInfo - Interface structure used for provisioning info
 * @param CSapNapt* - Pointer to SAP thru which provisioning is being done.
 * @return TInt - KErrNone in case of success.
 * 
 **/
	{
	
	TInt err = KErrNone;
	TNaptConfigInfo* cinfo = NULL;
	
	if(aInfo != NULL)
		{
		cinfo = aSap->GetConfigInfo();
		if(cinfo == NULL)
			{
			cinfo = FindConfig(aInfo->iIfIndex);
			if(!cinfo)
				cinfo = new(ELeave)TNaptConfigInfo;
			else
				{
				//Interface Index is already in Use, aborting operation
				err = KErrAbort;
				return err;
				}
			}
			//cinfo is initialized above, hence Pointer NULL check is not required here.
			cinfo->iPublicIap = aInfo->iPublicIap;
			cinfo->iPrivateIap = aInfo->iPrivateIap;
			TInetAddr::Cast(cinfo->iPublicGatewayIP).SetV4MappedAddress((aInfo->iPublicIp).Address());
			TInetAddr::Cast(cinfo->iPrivateIp).SetV4MappedAddress((aInfo->iPrivateIp).Address());
			//converting masklength to v6 mapped.So it will be 128-8 for class A, 128-16 for class B
			//128-24 for class C
			cinfo->iNetMaskLength = 128-(aInfo->iNetmaskLength);			
	        cinfo->iIfIndex = aInfo->iIfIndex;	
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
        	cinfo->iUplinkAccess  = aInfo->iUplinkAccess;			
        	//Get the client IP which DHCP will offer from IAP table
			//This is a not proper and will be fixed later for full DHCP impl	
			err = TNaptUtil::GetClientIp(aInfo->iPrivateIap, cinfo->iProvisionedIp);
#endif
			if(!aSap->GetConfigInfo())
				{
				AddConfigL(cinfo);
				aSap->SetConfigInfo(cinfo);
				}
        }
		return err;
	}
	
	
void CNaptClientConfigMgr::AddConfigL(const TNaptConfigInfo* aConfigInfo)
/** 
 * This method adds the configuration to the config list. 
 *
 * @param const TNaptConfigInfo* aConfigInfo - Config Info to be added
 * @return None
 * 
 **/
	{
	iConfigList.AppendL(aConfigInfo);
	}
