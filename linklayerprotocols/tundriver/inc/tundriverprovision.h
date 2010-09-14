/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Header file for tundriver binder provision.
* 
*
*/



/**
 @file
 @internalTechnology
*/

#ifndef TUNDRIVERPROVISION_H_
#define TUNDRIVERPROVISION_H_

#include <comms-infras/metadata.h>
#include <in_sock.h>			
#include <metadatabase.h>		

namespace ESock
	{
	class CCommsDatIapView;
	}

class TTunDriverIp4Provision
    {
public:
    inline TInetAddr LocalAddr() const;
    inline TInetAddr NetMask() const;
    inline TInetAddr BroadcastAddr() const;
    inline TInetAddr DefGateway() const;
    inline TInetAddr PrimaryDns() const;
    inline TInetAddr SecondaryDns() const;
    inline TUint32 Order() const;

    inline void SetLocalAddr(TInetAddr);
    inline void SetNetMask(TInetAddr);
    inline void SetBroadcastAddr(TInetAddr);
    inline void SetDefGateway(TInetAddr);
    inline void SetPrimaryDns(TInetAddr);
    inline void SetSecondaryDns(TInetAddr);
    inline void SetOrder(TUint32);
    
private:
    TInetAddr iLocalAddr;
    TInetAddr iNetMask;
    TInetAddr iBroadcastAddr;
    TInetAddr iDefGateway;
    TInetAddr iPrimaryDns;
    TInetAddr iSecondaryDns;
    TUint32 iOrder;
    };


inline TInetAddr TTunDriverIp4Provision::LocalAddr() const { return iLocalAddr; }
    
inline TInetAddr TTunDriverIp4Provision::NetMask() const { return iNetMask; }
    
inline TInetAddr TTunDriverIp4Provision::BroadcastAddr() const { return iBroadcastAddr; }
    
inline TInetAddr TTunDriverIp4Provision::DefGateway() const { return iDefGateway; }
    
inline TInetAddr TTunDriverIp4Provision::PrimaryDns() const { return iPrimaryDns; }
    
inline TInetAddr TTunDriverIp4Provision::SecondaryDns() const { return iSecondaryDns; }

inline TUint32 TTunDriverIp4Provision::Order() const { return iOrder; }

inline void TTunDriverIp4Provision::SetLocalAddr(TInetAddr aLocalAddr) { iLocalAddr = aLocalAddr;}
    
inline void TTunDriverIp4Provision::SetNetMask(TInetAddr aNetMask) { iNetMask = aNetMask;   }
    
inline void TTunDriverIp4Provision::SetBroadcastAddr(TInetAddr aBroadcastAddr) { iBroadcastAddr = aBroadcastAddr;   }
    
inline void TTunDriverIp4Provision::SetDefGateway(TInetAddr aDefGateway) { iDefGateway = aDefGateway; }
    
inline void TTunDriverIp4Provision::SetPrimaryDns(TInetAddr aPrimaryDns) { iPrimaryDns = aPrimaryDns; }
    
inline void TTunDriverIp4Provision::SetSecondaryDns(TInetAddr aSecondaryDns) { iSecondaryDns = aSecondaryDns; }

inline void TTunDriverIp4Provision::SetOrder(const TUint32 aOrder) {  iOrder = aOrder; }

#ifdef IPV6SUPPORT
class TTunDriverIp6Provision
    {
public:
    inline TInetAddr LocalAddr() const;
    inline TInetAddr NetMask() const;
    inline TInetAddr BroadcastAddr() const;
    inline TInetAddr DefGateway() const;
    inline TInetAddr PrimaryDns() const;
    inline TInetAddr SecondaryDns() const;
    inline TUint32 Order() const;

    inline void SetLocalAddr(TInetAddr);
    inline void SetNetMask(TInetAddr);
    inline void SetBroadcastAddr(TInetAddr);
    inline void SetDefGateway(TInetAddr);
    inline void SetPrimaryDns(TInetAddr);
    inline void SetSecondaryDns(TInetAddr);
    inline void SetOrder(TUint32);
    
private:
    TInetAddr iLocalAddr;
    TInetAddr iNetMask;
    TInetAddr iBroadcastAddr;
    TInetAddr iDefGateway;
    TInetAddr iPrimaryDns;
    TInetAddr iSecondaryDns;
    TUint32 iOrder;
    };


inline TInetAddr TTunDriverIp6Provision::LocalAddr() const { return iLocalAddr; }
    
inline TInetAddr TTunDriverIp6Provision::NetMask() const { return iNetMask; }
    
inline TInetAddr TTunDriverIp6Provision::BroadcastAddr() const { return iBroadcastAddr; }
    
inline TInetAddr TTunDriverIp6Provision::DefGateway() const { return iDefGateway; }
    
inline TInetAddr TTunDriverIp6Provision::PrimaryDns() const { return iPrimaryDns; }
    
inline TInetAddr TTunDriverIp6Provision::SecondaryDns() const { return iSecondaryDns; }

inline TUint32 TTunDriverIp6Provision::Order() const { return iOrder; }

inline void TTunDriverIp6Provision::SetLocalAddr(TInetAddr aLocalAddr) { iLocalAddr = aLocalAddr;}
    
inline void TTunDriverIp6Provision::SetNetMask(TInetAddr aNetMask) { iNetMask = aNetMask;   }
    
inline void TTunDriverIp6Provision::SetBroadcastAddr(TInetAddr aBroadcastAddr) { iBroadcastAddr = aBroadcastAddr;   }
    
inline void TTunDriverIp6Provision::SetDefGateway(TInetAddr aDefGateway) { iDefGateway = aDefGateway; }
    
inline void TTunDriverIp6Provision::SetPrimaryDns(TInetAddr aPrimaryDns) { iPrimaryDns = aPrimaryDns; }
    
inline void TTunDriverIp6Provision::SetSecondaryDns(TInetAddr aSecondaryDns) { iSecondaryDns = aSecondaryDns; }

inline void TTunDriverIp6Provision::SetOrder(const TUint32 aOrder) {  iOrder = aOrder; }
#endif

class CTunDriverProtoProvision : public CBase, public Meta::SMetaData
/**
Main provisioning info class that aggregates specific provisioning classes.
*/
    {
public:
    enum 
        {
        EUid = 0x10281C3C,          
        ETypeId = 1
        };

public:
    void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);
    void ProvisionNetworkConfigL(ESock::CCommsDatIapView* aIapView);
    void ProvisionIp4ConfigL(ESock::CCommsDatIapView* aIapView, TUint32 aOrder);
#ifdef IPV6SUPPORT
    void ProvisionIp6ConfigL(ESock::CCommsDatIapView* aIapView, TUint32 aOrder);
#endif

public:
    TTunDriverIp4Provision iIp4Provision;
#ifdef IPV6SUPPORT
    TTunDriverIp6Provision iIp6Provision;
#endif
    
    DATA_VTABLE
    };
#endif // TUNDRIVERPROVISION_H_
