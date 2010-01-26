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
// BCAProvision.inl
// BCA Provisioning Message inline methods.
// CBCAProvision methods
// 
//

/**
 @file
 @internalTechnology
*/

#ifdef SYMBIAN_ZERO_COPY_NETWORKING
RCommsBufPond SCommsPondContext::GetCommsPond() 
	{
	return TCommsBufPondTLSOp::Get();
	}
#endif

const TDesC& CBCAProvision::GetBCAStack() const
    {
    return iBCAStack;
    }

const TDesC& CBCAProvision::GetBCAName() const
    {
    return iBCAName;
    }    

const TDesC& CBCAProvision::GetPortName() const
    {
	return iPortName;
    }

TUint32 CBCAProvision::GetIAPid() const
    {
    return iapId;
    }

TCommRole CBCAProvision::GetCommRole() const
    {
	return iCommRole;
    }

TUint32 CBCAProvision::GetHandShaking() const
    {
	return iHandShaking;
    }
	
#ifdef SYMBIAN_ZERO_COPY_NETWORKING
RCommsBufPond  CBCAProvision::GetCommsPond()
	{
		return iCommsPondContext.GetCommsPond();
	}
#endif

void CBCAProvision::SetBCAStack(const TDesC& aBCAStack)
    {
    iBCAStack.Close();
    iBCAStack.CreateL(aBCAStack);    
    }

void CBCAProvision::SetBCAName(const TDesC& aBCAName)
    {
    iBCAName.Close();
   	iBCAName.CreateL(aBCAName);    
    }

void CBCAProvision::SetIAPid(TUint32 aIAPid)
    {
    iapId = aIAPid;
    }

void CBCAProvision::SetPortName(const TDesC& aPortName)
    {
    iPortName.Close();
    iPortName.CreateL(aPortName);         
    }

void CBCAProvision::SetCommRole(TCommRole aCommRole)
    {
    iCommRole = aCommRole;
    }


void CBCAProvision::SetHandShaking(TUint32 aHandShaking)
    {
    iHandShaking = aHandShaking;
    }

//-=========================================================
// CIPConfig methods
//-=========================================================

TUint32 CIPConfig::GetIpAddress() const
    {
    return iIpAddress;
    }

TUint32 CIPConfig::GetIp4NameServer1() const
    {
    return iIp4NameServer1;
    }

TUint32 CIPConfig::GetIp4NameServer2() const
    {
    return iIp4NameServer2;
    }

const TIp6Addr& CIPConfig::GetIp6NameServer1() const
    {
    return iIp6NameServer1;
    }

const TIp6Addr& CIPConfig::GetIp6NameServer2() const
    {
    return iIp6NameServer2;
    }

TUint32 CIPConfig::GetIpNetMask() const
    {
    return iIpNetMask;
    }

TUint32 CIPConfig::GetIpGateway() const
    {
    return iIpGateway;
    }

TBool CIPConfig::GetIpAddrFromServer() const
    {
    return iIpAddrFromServer;
    }

TBool CIPConfig::GetIp4DNSAddrFromServer() const
    {
    return iIp4DNSAddrFromServer;
    }

TBool CIPConfig::GetIp6DNSAddrFromServer() const
    {
    return iIp6DNSAddrFromServer;
    }

TBool CIPConfig::GetEnableIpHeaderComp() const
    {
    return iEnableIpHeaderComp;    
    }
    
const TDesC& CIPConfig::GetPortName() const
    {
	return iPortName;
    }    

TUint32 CIPConfig::GetBroadCastAddr() const
    {
    return iBroadcastAddr;
    }
    
void CIPConfig::SetIpAddress(TUint32 aIpAddress)
    {
    iIpAddress = aIpAddress;         
    }

void CIPConfig::SetIp4NameServer1(TUint32 aIp4NameServer1)
    {
    iIp4NameServer1 = aIp4NameServer1;
    }

void CIPConfig::SetIp4NameServer2(TUint32 aIp4NameServer2)
    {
    iIp4NameServer2 = aIp4NameServer2;
    }

void CIPConfig::SetIp6NameServer1(const TIp6Addr& aIp6NameServer1)
    {
    iIp6NameServer1 = aIp6NameServer1;
    }

void CIPConfig::SetIp6NameServer2(const TIp6Addr& aIp6NameServer2)
    {
    iIp6NameServer2 = aIp6NameServer2;
    }

void CIPConfig::SetIpNetMask(TUint32 aIpNetMask)
    {
    iIpNetMask = aIpNetMask;
    }

void CIPConfig::SetIpGateway(TUint32 aIpGateway)
    {
    iIpGateway = aIpGateway;
    }

void CIPConfig::SetIpAddrFromServer(TBool aIpAddrFromServer)
    {
    iIpAddrFromServer = aIpAddrFromServer;
    }

void CIPConfig::SetIp4DNSAddrFromServer(TBool aIp4DNSAddrFromServer)
    {
    iIp4DNSAddrFromServer = aIp4DNSAddrFromServer;
    }

void CIPConfig::SetIp6DNSAddrFromServer(TBool aIp6DNSAddrFromServer)
    {
    iIp6DNSAddrFromServer = aIp6DNSAddrFromServer;
    }

void CIPConfig::SetEnableIpHeaderComp(TBool aEnableIpHeaderComp)
    {
    iEnableIpHeaderComp = aEnableIpHeaderComp;
    }

void CIPConfig::SetPortName(const TDesC& aPortName)
    {
    iPortName.Close();
    iPortName.CreateL(aPortName);         
    }
    
void CIPConfig::SetBroadCastAddr(TUint32 aBroadCast)
    {
    iBroadcastAddr = aBroadCast;
    }
