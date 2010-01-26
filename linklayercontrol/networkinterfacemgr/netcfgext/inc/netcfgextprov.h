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
 @file
 @brief Header file for CNetCfgExtProvision
 @internalTechnology
*/

#ifndef NETWORKCONFIGEXTENSIONPROVISION_H
#define NETWORKCONFIGEXTENSIONPROVISION_H

#include <cdblen.h>
#include <metadatabase.h>
#include <comms-infras/ss_tiermanagerutils.h>

class CNetCfgExtProvision: public Meta::SMetaData
	{
public:
	enum
		{
		EUid = 0x10282FFD,
		ETypeId = 0x0
		};
	
public:
	IMPORT_C static CNetCfgExtProvision* NewL();

	IMPORT_C void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);
	IMPORT_C ~CNetCfgExtProvision();

	inline const TUint Iap() const;
	inline const TUint NetworkId() const;
	inline const TDesC& ConfigDaemonManagerName() const;
	inline const TDesC& ConfigDaemonName() const;

	inline void SetIap(const TUint aIap);
	inline void SetNetworkId(const TUint aNetworkId);
	inline void SetConfigDaemonManagerName(const TDesC& aName);
	inline void SetConfigDaemonName(const TDesC& aName);
	
  protected:
	CNetCfgExtProvision() {};
	
	TUint iIap;
	TUint32 iNetworkId;
	TBuf<KCommsDbSvrMaxFieldLength> iConfigDaemonManagerName;
	TBuf<KCommsDbSvrMaxFieldLength> iConfigDaemonName;
	
	DATA_VTABLE
	};

inline const TUint CNetCfgExtProvision::Iap() const
	{
	return iIap;
	}

inline const TUint CNetCfgExtProvision::NetworkId() const
	{
	return iNetworkId;
	}

inline const TDesC& CNetCfgExtProvision::ConfigDaemonManagerName() const
	{
	return iConfigDaemonManagerName;
	}

inline const TDesC& CNetCfgExtProvision::ConfigDaemonName() const
	{
	return iConfigDaemonName;
	}

inline void CNetCfgExtProvision::SetIap(const TUint aIap)
	{
	iIap = aIap;
	}

inline void CNetCfgExtProvision::SetNetworkId(const TUint aNetworkId)
	{
	iNetworkId = aNetworkId;
	}

inline void CNetCfgExtProvision::SetConfigDaemonManagerName(const TDesC& aName)
	{
	iConfigDaemonManagerName.Copy(aName);
	}

inline void CNetCfgExtProvision::SetConfigDaemonName(const TDesC& aName)
	{
	iConfigDaemonName.Copy(aName);
	}


#endif // CHAPCFG_H




