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
*   Provision implemtation for Ipv4 and Ipv6 binders for tundriver.
* 
*
*/

/**
 @file tundriverprovision.cpp
 @internalTechnology
*/

#include <comms-infras/ss_log.h>
#include <in_sock.h>
#include <comms-infras/metadata.h>
#include <comms-infras/coremcpractivities.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include "tundrivermcpr.h"
#include "tundriverprovision.h"

using namespace ESock;

void CTunDriverProtoProvision::InitialiseConfigL(CCommsDatIapView* aIapView)
/**
* CTunDriverProtoProvision::InitialiseConfigL is called from TunDriver MCPR to initialze
* tundriver with values from comms database.
* @param aIapView
* @returns 
*/ 
	{
	ProvisionNetworkConfigL(aIapView);
	}

void CTunDriverProtoProvision::ProvisionNetworkConfigL(CCommsDatIapView* aIapView)
/**
* CTunDriverProtoProvision::ProvisionNetworkConfigL is called from TunDriver MCPR to identify 
* tundriver is associated with protocol IPv4 or IPv6
* @param aIapView
* @returns 
*/
    {
    HBufC* buf = NULL;
    aIapView->GetTextL(KCDTIdLANIfNetworks, buf);
    CleanupStack::PushL(buf);
    if (buf->Length() == 0)
        {
        User::Leave(KErrCorrupt);
        }

    TPtrC16 networks;
    networks.Set(*buf);
    TPtrC16 current;
    TUint32 order = 0;
    TInt commaPos = 0;
    while (commaPos != KErrNotFound)
        {
        commaPos = networks.LocateF(',');
        if (commaPos == KErrNotFound)
            {
            // take all of string
            current.Set(networks);
            }
        else
            {
            current.Set(networks.Ptr(), commaPos);
            }

        if (current.CompareF(_L("ip")) == KErrNone)
            {
            ProvisionIp4ConfigL(aIapView, order);
            }
#ifdef IPV6SUPPORT
        else if (current.CompareF(_L("ip6")) == KErrNone)
            {
            ProvisionIp6ConfigL(aIapView, order);
            }
#endif
        else
            {
            User::Leave(KErrCorrupt);
            }

        order++;
        networks.Set(networks.Mid(commaPos+1));
        }

    CleanupStack::PopAndDestroy(buf);
    }


void CTunDriverProtoProvision::ProvisionIp4ConfigL(CCommsDatIapView* aIapView, TUint32 aOrder)
/**
* CTunDriverProtoProvision::ProvisionIp4ConfigL will initialize with IPv4 Header values
* for tundriver IPv4 configuration. like IPaddress, NetMask, Gateway and DNS Address. 
* @param aIapView
* @returns 
*/
    {
    iIp4Provision.SetOrder(aOrder);

    // Read IP address configuration parameters
    TBool serverRequired;
    aIapView->GetBoolL(KCDTIdLANIpAddrFromServer, serverRequired);

    TUint32 addr;
    TInt err; 

    iIp4Provision.SetLocalAddr(KInetAddrNone);
    iIp4Provision.SetNetMask(KInetAddrNone);
    iIp4Provision.SetDefGateway(KInetAddrNone);
    iIp4Provision.SetPrimaryDns(KInetAddrNone);
    iIp4Provision.SetSecondaryDns(KInetAddrNone);
    if (!serverRequired)
        {
        CAgentMetaConnectionProvider::GetIp4AddrL(aIapView, KCDTIdLANIpAddr, addr);
        TInetAddr v4MappedAddr;
        v4MappedAddr.SetV4MappedAddress(addr);
        iIp4Provision.SetLocalAddr(v4MappedAddr);
        err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdLANIpNetMask, addr);
        if (err == KErrNone)
            {
            v4MappedAddr.SetV4MappedAddress(addr);
            iIp4Provision.SetNetMask(v4MappedAddr);
            }

        err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdLANIpGateway, addr);
        if (err == KErrNone)
            {
            v4MappedAddr.SetV4MappedAddress(addr);
            iIp4Provision.SetDefGateway(v4MappedAddr);
            }
        else if (err == KErrNotFound || err == KErrArgument)
            {
            iIp4Provision.SetDefGateway(iIp4Provision.LocalAddr());
            }
        else
            {
            User::Leave(err);
            }

        // Because CommDB doesn't define a Broadcast Address field, we must
        // calculate the broadcast address. This is based on the localAddr
        // and the netMask.

        TInetAddr localAddr;
        localAddr.SetV4MappedAddress(iIp4Provision.LocalAddr().Address());
        localAddr.SetPort(0);
        TInetAddr netMask;
        netMask.SetV4MappedAddress(iIp4Provision.NetMask().Address());
        netMask.SetPort(0);
        TInetAddr broadcast;
        broadcast.SubNetBroadcast(localAddr, netMask);
        iIp4Provision.SetBroadcastAddr(broadcast.Address());
        }

    aIapView->GetBoolL(KCDTIdLANIpDNSAddrFromServer, serverRequired);
    if (!serverRequired)
        {
        err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdLANIpNameServer1, addr);
        if (err == KErrNone)
            {
            TInetAddr primaryDns;
            primaryDns.SetV4MappedAddress(addr);
            iIp4Provision.SetPrimaryDns(primaryDns);
            err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdLANIpNameServer2, addr);
            if (err == KErrNone)
                {
                TInetAddr secondaryDns;
                secondaryDns.SetV4MappedAddress(addr);
                iIp4Provision.SetSecondaryDns(secondaryDns);
                }
            }

        if (err != KErrNone && err != KErrNotFound && err != KErrArgument)
            {
            User::Leave(err);
            }
        }
    }

#ifdef IPV6SUPPORT
void CTunDriverProtoProvision::ProvisionIp6ConfigL(CCommsDatIapView* aIapView, TUint32 aOrder)
/**
* CTunDriverProtoProvision::ProvisionIp6ConfigL will initialize with IPv4 Header values
* for tundriver IPv6 configuration. like IPaddress, NetMask, Gateway and DNS Address. 
* @param aIapView
* @returns 
*/
    {
    iIp6Provision.SetOrder(aOrder);

    // Read IP address configuration parameters
    TBool serverRequired;
    aIapView->GetBoolL(KCDTIdLANIpAddrFromServer, serverRequired);

    TIp6Addr addr;
    TInetAddr inetAddress;
    TInt err; 

    iIp6Provision.SetLocalAddr(KInetAddrNone);
    iIp6Provision.SetNetMask(KInetAddrNone);
    iIp6Provision.SetDefGateway(KInetAddrNone);
    iIp6Provision.SetPrimaryDns(KInetAddrNone);
    iIp6Provision.SetSecondaryDns(KInetAddrNone);
    if (!serverRequired)
        {
        CAgentMetaConnectionProvider::GetIp6AddrL(aIapView, KCDTIdLANIpAddr, addr);
        inetAddress.SetAddress(addr);
        iIp6Provision.SetLocalAddr(inetAddress);
        err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdLANIpNetMask, addr);
        if (err == KErrNone)
            {
            inetAddress.SetAddress(addr);
            iIp6Provision.SetNetMask(inetAddress);
            }
        err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdLANIpGateway, addr);
        if (err == KErrNone)
            {
            inetAddress.SetAddress(addr);
            iIp6Provision.SetDefGateway(inetAddress);
            }
        else if (err == KErrNotFound || err == KErrArgument)
            {
            iIp6Provision.SetDefGateway(iIp6Provision.LocalAddr());
            }
        else
            {
            User::Leave(err);
            }
        err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdLANIpNameServer1, addr);
        if(err == KErrNone)
            {
            inetAddress.SetAddress(addr);
            iIp6Provision.SetPrimaryDns(inetAddress);
            }
        else
            {
            User::LeaveIfError(err);
            }
        err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdLANIpNameServer2, addr);
        if(err == KErrNone)
            {
            inetAddress.SetAddress(addr);
            iIp6Provision.SetSecondaryDns(inetAddress);
            }
        else
            {
            User::LeaveIfError(err);
            }
        // Because CommDB doesn't define a Broadcast Address field, we must
        // calculate the broadcast address. This is based on the localAddr
        // and the netMask.

        TInetAddr localAddr;
        localAddr = iIp6Provision.LocalAddr();
        localAddr.SetPort(0);
        TInetAddr netMask;
        netMask = iIp6Provision.NetMask();
        netMask.SetPort(0);
        TInetAddr broadcast;
        broadcast.SubNetBroadcast(localAddr, netMask);
        iIp6Provision.SetBroadcastAddr(broadcast);
        }

    iIp6Provision.SetPrimaryDns(KInetAddrNone);
    iIp6Provision.SetSecondaryDns(KInetAddrNone);

    aIapView->GetBoolL(KCDTIdLANIpDNSAddrFromServer, serverRequired);
    if (!serverRequired)
        {
        err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdLANIpNameServer1, addr);
        if (err == KErrNone)
            {
            inetAddress.SetAddress(addr);
            iIp6Provision.SetPrimaryDns(inetAddress);
            err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdLANIpNameServer2, addr);
            if (err == KErrNone)
                {
                inetAddress.SetAddress(addr);
                iIp6Provision.SetSecondaryDns(inetAddress);
                }
            }

        if (err != KErrNone && err != KErrNotFound && err != KErrArgument)
            {
            User::Leave(err);
            }
        }
    }
#endif

//
// Attribute table for provisioning structures passed to CFProtocol
//

START_ATTRIBUTE_TABLE(CTunDriverProtoProvision, CTunDriverProtoProvision::EUid, CTunDriverProtoProvision::ETypeId)
// no attributes defined as no serialisation takes place.
END_ATTRIBUTE_TABLE()
