/**
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file declaring the Ethernet CFProtocol ECOM factories
* 
*
*/



/**
 @file
*/

#ifndef ETHPROVISION_H_INCLUDED_
#define ETHPROVISION_H_INCLUDED_

#include <in_sock.h>
#include <comms-infras/metadata.h>


/**
Provisioning information for CLANLinkCommon.
*/
class TLanLinkProvision : public Meta::SMetaData
	{
public:
	enum 
		{
		EUid = 0x10281DDB, ETypeId = 1
		};

public:
	inline const TDesC8& PacketDriverName() const;
	inline const TDesC8& ConfigDaemonName() const;
	inline const TDesC8& LddFilename() const;
	inline const TDesC8& LddName() const;
	inline const TDesC8& PddFilename() const;
	inline const TDesC8& PddName() const;

	inline void SetPacketDriverName(const TDesC&);
	inline void SetConfigDaemonName(const TDesC&);
	inline void SetLddFilename(const TDesC&);
	inline void SetLddName(const TDesC&);
	inline void SetPddFilename(const TDesC&);
	inline void SetPddName(const TDesC&);
	
	enum { KMaxNameSize = 32 };

private:
	TBuf8<KMaxNameSize> iPacketDriverName;
	TBuf8<KMaxNameSize> iConfigDaemonName;
	TBuf8<KMaxNameSize> iLddFilename;
	TBuf8<KMaxNameSize> iLddName;
	TBuf8<KMaxNameSize> iPddFilename;
	TBuf8<KMaxNameSize> iPddName;

public:
	DATA_VTABLE
	};

/**
Provisioning information for CLanIp4Bearer
*/
class TLanIp4Provision : public Meta::SMetaData
	{
public:
	enum 
		{
		EUid = 0x10281DDB, ETypeId = 2
		};

public:
	inline TUint32 LocalAddr() const;
  	inline TUint32 NetMask() const;
  	inline TUint32 BroadcastAddr() const;
  	inline TUint32 DefGateway() const;
  	inline TUint32 PrimaryDns() const;
  	inline TUint32 SecondaryDns() const;
  	inline TUint32 Order() const;

	inline void SetLocalAddr(TUint32);
  	inline void SetNetMask(TUint32);
  	inline void SetBroadcastAddr(TUint32);
  	inline void SetDefGateway(TUint32);
  	inline void SetPrimaryDns(TUint32);
  	inline void SetSecondaryDns(TUint32);
  	inline void SetOrder(TUint32);
  	
private:
  	TUint32	iLocalAddr;
  	TUint32	iNetMask;
  	TUint32	iBroadcastAddr;
  	TUint32	iDefGateway;
  	TUint32	iPrimaryDns;
  	TUint32	iSecondaryDns;
  	TUint32 iOrder;

public:
	DATA_VTABLE
	};

/**
Provisioning information for CLanIp6Bearer
*/
class TLanIp6Provision : public Meta::SMetaData
	{
public:
	enum 
		{
		EUid = 0x10281DDB, ETypeId = 3
		};

public:
	inline const TIp6Addr& PrimaryDns() const;
	inline const TIp6Addr& SecondaryDns() const;
	inline TUint32 Order() const;

	inline void SetPrimaryDns(const TIp6Addr&);
	inline void SetSecondaryDns(const TIp6Addr&);
  	inline void SetOrder(TUint32);
	
private:
	TIp6Addr iPrimaryDns;
	TIp6Addr iSecondaryDns;
	TUint32 iOrder;
	
public:
	DATA_VTABLE
	};

	
// TLanLinkProvision inline functions

inline const TDesC8& TLanLinkProvision::PacketDriverName() const { return iPacketDriverName; }	

inline const TDesC8& TLanLinkProvision::ConfigDaemonName() const { return iConfigDaemonName; }

inline const TDesC8& TLanLinkProvision::LddFilename() const	{ return iLddFilename; }

inline const TDesC8& TLanLinkProvision::LddName() const	{ return iLddName; }

inline const TDesC8& TLanLinkProvision::PddFilename() const { return iPddFilename; }

inline const TDesC8& TLanLinkProvision::PddName() const	{ return iPddName; }
	
inline void TLanLinkProvision::SetPacketDriverName(const TDesC& aName)
	{
	iPacketDriverName.Copy(aName);
	}
	
inline void TLanLinkProvision::SetConfigDaemonName(const TDesC& aName)
	{
	iConfigDaemonName.Copy(aName);
	}
	
inline void TLanLinkProvision::SetLddFilename(const TDesC& aName)
	{
	iLddFilename.Copy(aName);
	}
	
inline void TLanLinkProvision::SetLddName(const TDesC& aName)
	{
	iLddName.Copy(aName);
	}
	
inline void TLanLinkProvision::SetPddFilename(const TDesC& aName)
	{
	iPddFilename.Copy(aName);
	}
	
inline void TLanLinkProvision::SetPddName(const TDesC& aName)
	{
	iPddName.Copy(aName);
	}

// TLanIp4Provision inline functions

inline TUint32 TLanIp4Provision::LocalAddr() const { return iLocalAddr; }
	
inline TUint32 TLanIp4Provision::NetMask() const { return iNetMask;	}
	
inline TUint32 TLanIp4Provision::BroadcastAddr() const { return iBroadcastAddr;	}
	
inline TUint32 TLanIp4Provision::DefGateway() const	{ return iDefGateway; }
	
inline TUint32 TLanIp4Provision::PrimaryDns() const	{ return iPrimaryDns; }
	
inline TUint32 TLanIp4Provision::SecondaryDns() const { return iSecondaryDns; }

inline TUint32 TLanIp4Provision::Order() const { return iOrder; }

inline void TLanIp4Provision::SetLocalAddr(TUint32 aLocalAddr) { iLocalAddr = aLocalAddr; }
	
inline void TLanIp4Provision::SetNetMask(TUint32 aNetMask) { iNetMask = aNetMask;	}
	
inline void TLanIp4Provision::SetBroadcastAddr(TUint32 aBroadcastAddr) { iBroadcastAddr = aBroadcastAddr;	}
	
inline void TLanIp4Provision::SetDefGateway(TUint32 aDefGateway) { iDefGateway = aDefGateway; }
	
inline void TLanIp4Provision::SetPrimaryDns(TUint32 aPrimaryDns) { iPrimaryDns = aPrimaryDns; }
	
inline void TLanIp4Provision::SetSecondaryDns(TUint32 aSecondaryDns) { iSecondaryDns = aSecondaryDns; }

inline void TLanIp4Provision::SetOrder(const TUint32 aOrder) {  iOrder = aOrder; }


// TLanIp6Provision inline functions

inline const TIp6Addr& TLanIp6Provision::PrimaryDns() const	{ return iPrimaryDns; }
	
inline const TIp6Addr& TLanIp6Provision::SecondaryDns() const {	return iSecondaryDns; }

inline TUint32 TLanIp6Provision::Order() const { return iOrder; }

inline void TLanIp6Provision::SetPrimaryDns(const TIp6Addr& aPrimaryDns) { iPrimaryDns = aPrimaryDns; }
	
inline void TLanIp6Provision::SetSecondaryDns(const TIp6Addr& aSecondaryDns) { iSecondaryDns = aSecondaryDns; }

inline void TLanIp6Provision::SetOrder(const TUint32 aOrder) { iOrder = aOrder; }

#endif
// ETHPROVISION_H_INCLUDED_
