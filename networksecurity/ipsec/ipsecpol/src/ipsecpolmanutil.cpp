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
// IPSecPolManUtil.cpp - IPSec Policy Manager Utilities
//

#include <in_sock.h>

#include "ipsecpolmanhandler.h"
#include "ipsecpolparser.h"
#include "ipsecpolapi.h"

//
// Build and write the strings to a policy file that tell that the clean
// tunnel mode IKE negotiation packets are accepted. For each gateway
// two strings are written: port 500 and port 4500.
// The format of strings are shown below. 10.10.10.10%2 is in this example
// an IPv4 gateway address, 255.255.255.255 is IPv4 full mask.
//
// remote 10.10.10.10%2 255.255.255.255 protocol 17 local_port  500
// remote 10.10.10.10%2 255.255.255.255 protocol 17 local_port 4500
//
//
TInt 
CIPSecPolicyManagerHandler::WriteTunnelModeIkeNegotiationStringsL(
    HBufC8*& aPolBfr)
    {
    TBuf8<1024> stringBuf;
    TInt err(KErrNone);

    // Create pointer array for gateway list
    RPointerArray<TInetAddr> gatewayList;
    CleanupClosePushL(gatewayList);

    // Set base for the selector list
    CSecurityPolicy* sp = iPieceData->Policies();
    CSelectorList* selectorList = sp->SelectorList();
    TInt selectorCount = selectorList->Count();

    // Loop through the selector list
    for (TInt i = 0; (i < selectorCount) && (err == KErrNone) ; i++)
        {
        CPolicySelector* policySelector = selectorList->At(i);
        TSecpolBundleIter iterl(policySelector->iBundle);
        CSecpolBundleItem* itemL = NULL;
        CSecpolBundleItem* itemWork;
       
        // Find the last item in bundle
        while ((itemWork = iterl++) != NULL)
            {
            if (!itemWork->iTunnel.IsUnspecified())
                itemL = itemWork;
            }
        // Add the address of the last gateway in the bundle
        // to the gateway list
        if (itemL)
            {
            // Search a matching element from the gateway list
            TInt count = gatewayList.Count();
            TInt j;
            for (j = 0; j < count && !itemL->iTunnel.Match(*gatewayList[j]); j++){}
            // Add a new element to the gateway list
            if (j==count)
                {
                TInt position = 0;
                gatewayList.Insert( &itemL->iTunnel, position);

                // Build two strings for policy file
                stringBuf.Zero();
                BuildTunnelModeIkeString(stringBuf,
                                         EInbound,
                                         500,
                                         itemL->iTunnel);
                BuildTunnelModeIkeString(stringBuf,
                                         EInbound,
                                         4500,
                                         itemL->iTunnel);

                // Write the string to file
                err = TPolicyParser::BufferAppend(aPolBfr, stringBuf);
                }
            }
        }

    CleanupStack::PopAndDestroy();
    return err;
    }
//
// Build a string that tells that the clean tunnel mode IKE
// negotiation packets are accepted:
// remote 10.10.10.10%2 255.255.255.255 protocol 17 local_port 500
//
//
void 
CIPSecPolicyManagerHandler::BuildTunnelModeIkeString(
    TDes8& aStringBuf,
    TInt aDirection,
    TInt aPort,
    TInetAddr& aGwAddr)
    {
    TBuf<60> addr;

    // Gateway IP address
    aStringBuf.Append(_L8(" remote "));
    aGwAddr.OutputWithScope(addr);
    aStringBuf.Append(addr);

    // Full mask, either IPv4 or IPv6
    if (aGwAddr.Family() == KAfInet || aGwAddr.IsV4Mapped())
        {
        if (!aGwAddr.Scope())
            {
            aStringBuf.Append(_L8(" 255.255.255.255 "));
            }
        else
            {
            aStringBuf.Append(_L8(" 255.255.255.255%-1 "));
            }
        }
    else
        {
        aStringBuf.Append(_L8(" FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF "));
        }

    // Protocol UDP
    aStringBuf.Append(_L8(" protocol 17 "));

    // Local or remote port
    if (aDirection == EInbound)
        {
        aStringBuf.Append(_L8(" local_port "));
        }
    else
        {
        aStringBuf.Append(_L8(" remote_port "));
        }

    // Port number
    aStringBuf.AppendFormat(_L8(" %d = { }\n"), aPort);
    }

//
// Build and write the strings to a policy file that tell that the clean
// transport mode IKE negotiation packets are accepted. For each
// transport mode selector two strings are written: port 500 and port 4500.
// The format of strings are shown below. 10.10.10.10%2 is in this example
// an IPv4 transport mode net, 255.255.255.0 is an IPv4 net mask.
//
// remote 10.10.10.10%2 255.255.255.0%-1 protocol UDP local_port  500
// remote 10.10.10.10%2 255.255.255.0%-1 protocol UDP local_port 4500
//
//
TInt 
CIPSecPolicyManagerHandler::WriteTransportModeIkeNegotiationStrings(
    HBufC8*& aPolBfr)
    {
    TInt err = KErrNone;

    //  Set base for the selector list
    CSecurityPolicy* sp = iPieceData->Policies();
    CSelectorList* selectorList = sp->SelectorList();
    TInt selectorCount = selectorList->Count();

    // Loop through the selector list
    for (TInt i = 0; i < selectorCount; i++)
        {
        CPolicySelector* policySelector = selectorList->At(i);
        if (policySelector->iBundle.IsEmpty())
            {
            continue;
            }
        TSecpolBundleIter iterl(policySelector->iBundle);
        CSecpolBundleItem* itemFirst = iterl;

        // Calculate the bundle count
        TInt bundleCount = 0;
        while ((iterl++) != NULL)
            {
            bundleCount++;
            }

        // Check that no gateway is specified, it means transport mode
        if (bundleCount == 1
            && itemFirst != NULL
            && itemFirst->iTunnel.IsUnspecified())
            {
            // Build two strings for policy file
            TBuf8<1024> stringBuf;
            stringBuf.Zero();
            BuildTransportModeIkeString(stringBuf,
                                        500,
                                        policySelector->iRemote,
                                        policySelector->iRemoteMask);
            BuildTransportModeIkeString(stringBuf,
                                        4500,
                                        policySelector->iRemote,
                                        policySelector->iRemoteMask);
            // Write the string to file
            err = TPolicyParser::BufferAppend(aPolBfr, stringBuf);
            }
        if (err != KErrNone)
            {
            break;
            }
        }
    return err;
    }

//
// Build a string that tells that the clean Transport mode IKE
// negotiation packets are accepted:
// remote 10.10.10.10%2 255.255.255.255%-1 protocol UDP local_port 500
//
//
void 
CIPSecPolicyManagerHandler::BuildTransportModeIkeString(
    TDes8& aStringBuf,
    TInt aPort,
    TInetAddr& aRemote,
    TInetAddr& aRemoteMask)
    {
    TBuf<60> addr;

    // Gateway IP address
    aStringBuf.Append(_L8(" remote "));
    aRemote.OutputWithScope(addr);
    aStringBuf.Append(addr);
    aStringBuf.Append(_L8(" "));

    // Mask
    aRemoteMask.OutputWithScope(addr);
    aStringBuf.Append(addr);
    aStringBuf.Append(_L8(" "));

    // Protocol UDP, local_port #, bypass action 
    aStringBuf.AppendFormat(_L8("protocol 17 local_port %d = { }\n"), aPort);
    }

//
// Build a string that tells that the clean DHCP protocol
// packets are accepted:
// outbound protocol 17 remote_port 67
// inbound  protocol 17 local_port 68
//
//
TInt 
CIPSecPolicyManagerHandler::BuildDhcpProtocolString( HBufC8*& aPolBfr)
    {
    TInt err = KErrNone;
    TBuf8<1024> stringBuf;

    // Outbound, protocol UDP, remote_port 67
    stringBuf.Append(_L8(" outbound protocol 17 remote_port 67 = { }\n"));

    // Inbound, protocol UDP, local_port 68
    stringBuf.Append(_L8(" inbound protocol 17 local_port 68 = { }\n"));

    // Write the string to file
    err = TPolicyParser::BufferAppend(aPolBfr, stringBuf);
    return err;
    }

//
// Build the strings that tell that clean MIPv4 protocol packets
// are accepted:
//
// *** outbound solicitation messages ***
// outbound remote 255.255.255.255 255.255 255.255 icmp_type 10
// outbound remote 224.0.0.2       255.255.255.255 icmp_type 10
//
// *** outbound Registration Request message ***
// outbound protocol 17 remote_port 434
//
// *** inbound Advertisement messages ***
// inbound local 255.255.255.255 255.255.255.255 icmp_type 9
// inbound local 224.0.0.1       255.255.255.255 icmp_type 9
//
// *** inbound Registration Reply message ***
// inbound protocol 17 local_port 434
//
//
TInt 
CIPSecPolicyManagerHandler::BuildMip4BypassSelectors( HBufC8*& aPolBfr)
    {
    TInt err = KErrNone;
    TBuf8<1024> stringBuf;

    // Outbound Solicitation message
    stringBuf.Append(_L8(" outbound "));
    stringBuf.Append(_L8(" remote "));
    stringBuf.Append(_L8(" 255.255.255.255 "));  // IP address
    stringBuf.Append(_L8(" 255.255.255.255 "));  // Mask
    stringBuf.Append(_L8(" icmp_type 10 = { }\n"));

    // Outbound Solicitation message
    stringBuf.Append(_L8(" outbound "));
    stringBuf.Append(_L8(" remote "));
    stringBuf.Append(_L8(" 224.0.0.2 "));
    stringBuf.Append(_L8(" 255.255.255.255 "));
    stringBuf.Append(_L8(" icmp_type 10 = { }\n"));

    // Outbound Registration Request message
    stringBuf.Append(_L8(" outbound "));
    stringBuf.Append(_L8(" protocol 17 "));
    stringBuf.Append(_L8(" remote_port 434 = { }\n"));

    // Inbound Advertisement message
    stringBuf.Append(_L8(" inbound "));
    stringBuf.Append(_L8(" local "));
    stringBuf.Append(_L8(" 255.255.255.255 "));
    stringBuf.Append(_L8(" 255.255.255.255 "));
    stringBuf.Append(_L8(" icmp_type 9 = { }\n"));

    // Inbound Advertisement message
    stringBuf.Append(_L8(" inbound "));
    stringBuf.Append(_L8(" local "));
    stringBuf.Append(_L8(" 224.0.0.1 "));
    stringBuf.Append(_L8(" 255.255.255.255 "));
    stringBuf.Append(_L8(" icmp_type 9 = { }\n"));

    // Inbound Registration Reply message
    stringBuf.Append(_L8(" inbound "));
    stringBuf.Append(_L8(" protocol 17 "));
    stringBuf.Append(_L8(" local_port 434 = { }\n"));

    // Write the string to file
    err = TPolicyParser::BufferAppend(aPolBfr, stringBuf);
    return err;
    }

//
//  Modify the SA names (SA = Security association) by appending
//  the SA name with suffix ZZ_n, where the n is the next policy
//  handle and zz_ is a separator
//
void 
CIPSecPolicyManagerHandler::MakeUniqueSANamesL()
    {
    CSecurityPolicy* policy = iPieceData->Policies();
    CArrayFixFlat<CPolicySpec *>* saList = policy->SAList();

    // Loop here until all SPDB SA names changed
    TInt i;
    TBuf8<100> buf;
    for (i = 0; i < saList->Count(); i++)
        {
        if (saList->At(i)->iSpectype == EPolSpecEP)
            {
            // TODO: Verify also that EndPoint names are unique
            continue;
            }

        // Copy an SA name to buf
        buf.Copy(saList->At(i)->iName->Des());
        buf.AppendFormat(_L8("zz_%d"), iCurrentPolicyHandle.iHandle + 1);
        // Delete the name bfr
        delete saList->At(i)->iName;
        saList->At(i)->iName = NULL;
        // Allocate a new bfr
        saList->At(i)->iName = HBufC8::NewL(buf.Length());
        // Set the new name buffer
        saList->At(i)->iName->Des().Copy(buf);
        }
    }

//
//  This function examines all selectors in the selector list and
//  creates for each selector a comparison word that is needed
//  to define the sequence of selectors in the file that is
//  sent to the IPSec protocol component. The comparison word
//  consists of the following parts:
//  -- byte 1 = 0
//  -- byte 2, bit 0 = 1 = port 500 or 4500 is defined in selector
//  -- byte 2, bit 0 = 1 = port 67 and 68 is defined in selector
//  -- byte 2, bit 0 = 1 = MIPv4 is defined by the selector
//  -- byte 2, bit 1 = 0
//  -- byte 4, bit 8 = 1 = all selectors have this bit on
//
//  The selector that has the greatest comparison word, is the first
//  in the sequence.
//
TInt 
CIPSecPolicyManagerHandler::BuildComparisonWord(CSelectorList* aSelList)
    {
    TInt count = aSelList->Count();
    TInt compWord(0);

    // Loop through the selector list
    for (TInt i = 0; i < count; i++)
        {
        CPolicySelector* policySelector = aSelList->At(i);
        compWord = 0;

        // Check if the local port 500 or 4500 is defined in selector, or
        // remote port 67 and local port 68 is defined, set a bit
        // in comparison word (IKE negotiation ports or DHCP ports)
        if ((policySelector->iLocal.Port() == 500
             || policySelector->iLocal.Port() == 4500)
            || (policySelector->iRemote.Port() == 67
                && policySelector->iLocal.Port() == 68))
            {
            compWord |= 0x00400000;
            }

        // Check if MIP4 protocol selectors:
        // -- IcmpType is 9 or 10
        //    or
        // -- iType is 9 or 10
        //    or
        //    protocol is 17 (UDP) and local port or remote_port is 434
        // and set a bit in comparison word
        if ((policySelector->iIcmpType == 9 || policySelector->iIcmpType == 10)
            || (policySelector->iType == 9 || policySelector->iType == 10)
            || (policySelector->iProtocol == 17
                && (policySelector->iRemote.Port() == 434
                    || policySelector->iLocal.Port() == 434)))
            {
            compWord |= 0x00400000;
            }

        // Each selector has at least this bit set on
        compWord |= 0x00000001;

        // Store comparison word to the selector
        policySelector->iCompWord = compWord;
        }

    return (KErrNone);
    }

//
//  This function examines all selectors in the selector list and
//  sets a unique sequence number to each selector. The sequence
//  numbers are used to define the sequence of selectors in a
//  file that is sent to the IPSec protocol component.
//  Each selector has a comparison word created previously.
//
//
TInt 
CIPSecPolicyManagerHandler::SetSequenceNumbers(CSelectorList* aSelList)
    {
    TInt count = aSelList->Count();
    TBool complete = EFalse;
    TInt currentSequenceNumber = 1;
    TUint32 currentHighValue;
    TInt j = 0;

    // Loop until each selector has a sequence number (exception:
    // ffffffff means deleted
    while (!complete)
        {
        currentHighValue = 0;

        // Loop through the selector list and search the selector that has
        // the highest value in comparison field
        for (TInt i = 0; i < count; i++)
            {
            CPolicySelector *policySelector = aSelList->At(i);

            if (policySelector->iSequenceNumber == -1)
                {
                continue;
                }

            if (policySelector->iSequenceNumber == 0 &&
                policySelector->iCompWord > currentHighValue )
                {
                j = i;
                currentHighValue = policySelector->iCompWord;
                }
            }

        // A round is complete
        if (currentHighValue == 0)
            {
            complete = ETrue;  // Each selector has a sequence number
            }
        else
            {
            CPolicySelector* tempSelector = aSelList->At(j);
            tempSelector->iSequenceNumber = currentSequenceNumber;
            currentSequenceNumber++;
            }
        }
    return 0;
    }

//
//  Compute either IPv4 or IPv6 mask length from a mask in TInetAddr
//
//
//
TInt 
CIPSecPolicyManagerHandler::CalculateMaskLength(TInetAddr& aMask)
    {
    TInt maskLength(0);
    if (aMask.Family() == KAfInet)
        {
        // 96 = corresponds now IPv6 size
        maskLength = 96 + MaskLength(aMask.Address());
        }
    else if (aMask.Family() == KAfInet6)
        {
        maskLength = MaskLength(aMask.Ip6Address());
        }
    return (maskLength);
    }

//
// MaskLength
// **********
// Borrowed from Tcpip6\src\iface.cpp
// Local utility, compute consecutive leftmost 1-bits from 32 bit integer
//
//
TInt 
CIPSecPolicyManagerHandler::MaskLength(TUint32 aAddr)
    {
    TInt count = 0;
    while (aAddr & 0x80000000)
        {
        count++;
        aAddr <<= 1;
        }
    return count;
    }

//
// MaskLength
// **********
// Borrowed from Tcpip6\src\iface.cpp
// Computes mask length from IPv6 address
//
//
TInt 
CIPSecPolicyManagerHandler::MaskLength(const TIp6Addr &aAddr)
    {
    TInt count = 0;
    TInt len = (sizeof(aAddr.u.iAddr8) / sizeof(aAddr.u.iAddr8[0]));
    for (TUint i = 0; i < len; ++i)
        {
        if (aAddr.u.iAddr8[i] == 0xFF)
            {
            count += 8;
            }
        else
            {
            // Calls 32-bit routine
            count += MaskLength(aAddr.u.iAddr8[i] << 24);
            break;
            }
        }
    return count;
    }

//
// This function deletes from the policy the Inbound/Outbound
// selector pairs by marking the selectors.
//
//
void 
CIPSecPolicyManagerHandler::DeleteExtraInboundOutboundSelectors()
    {
    // Set the base for the selector list
    CSecurityPolicy* sp = iPieceData->Policies();
    CSelectorList* list = sp->SelectorList();
    TInt count = list->Count();

    // Search a 'bypass/drop_everything_else' selectors and mark them.
    // These selectors will be deleted later when the objects are
    // converted to string format
    for (TInt i = 0; i < count; i++)
        {
        CPolicySelector* ps = list->At(i);

        if (IsBypassEverythingElse(*ps) || IsDropEverythingElse(*ps))
            {
            ps->iSequenceNumber = 0xffffffff;
            }
        }
    }

//
// This function adds the following selectors to the end of the
// string format policy file:
//  inbound = { }
//  outbound = { }
//
// This occurs only if the current policies are in bypass mode.
// Bypass mode means that the packets that do not match with any other
// selector, are transferred without IPSec encapsulation.
//
//
TInt 
CIPSecPolicyManagerHandler::AddInboundOutboundSelectorPair()
    {
    TBuf8<128> stringBuf;
    TInt err(KErrNone);

    // If drop mode, return immediately
    if (iBypassOrDropMode == KDropMode)
        {
        return err;
        }

    // Add strings to work buffer
    if (iBypassOrDropMode & KInboundBypass)
        {
        stringBuf.Append(_L8(" inbound = { }\n"));
        }
    if (iBypassOrDropMode & KOutboundBypass)
        {
        stringBuf.Append(_L8(" outbound = { }\n"));
        }

    // Write the string to file
    err = TPolicyParser::BufferAppend(iPolBfr, stringBuf);
    return err;
    }

//
//  Find selector that matches with parameters given by a Key
//  Management application. This function is a part of
//  GetIPSecSAInfo API.
//
//
CPolicySelector*
CIPSecPolicyManagerHandler::FindMatchingSelector()
    {
    TInetAddr localAddr;
    TInetAddr remoteAddr;

    // Set the base for the selector list
    CSecurityPolicy* sp = iPieceData->Policies();
    CSelectorList* selectorList = sp->SelectorList();
    TInt selectorCount = selectorList->Count();

    // Build local subnetwork address if the mask defined
    if (!iSelectorInfo->iLocalMask.IsUnspecified())
        {
        if ((iSelectorInfo->iLocal.IsV4Mapped()) &&
            (!iSelectorInfo->iLocalMask.IsV4Mapped()))
            {
            iSelectorInfo->iLocalMask.ConvertToV4Mapped();
            }
        localAddr.SubNet(iSelectorInfo->iLocal, iSelectorInfo->iLocalMask);
        }
    else
        {
        // No mask, use host address
        localAddr = iSelectorInfo->iLocal;   
        }
        
    // Set scope too    
    localAddr.SetScope(iSelectorInfo->iLocal.Scope());  

    // Build remote subnetwork address if the mask defined
    if (!iSelectorInfo->iRemoteMask.IsUnspecified())
        {
        if ((iSelectorInfo->iRemote.IsV4Mapped()) &&
            (!iSelectorInfo->iRemoteMask.IsV4Mapped()))
            {
            iSelectorInfo->iRemoteMask.ConvertToV4Mapped();
            }
        remoteAddr.SubNet(iSelectorInfo->iRemote, iSelectorInfo->iRemoteMask);
        }
    else
        {
        // No mask, use host address
        remoteAddr = iSelectorInfo->iRemote;
        }

    // Set scope too
    remoteAddr.SetScope(iSelectorInfo->iRemote.Scope());

    // Find the matching selector
    for (TInt i = 0; i < selectorCount; i++)
        {
        CPolicySelector* policySelector = selectorList->At(i);
        TInetAddr polLocalMask = policySelector->iLocalMask;
        TInetAddr polRemoteMask = policySelector->iRemoteMask;

        // Skip 'bypass/drop_everything_else' or interface selectors
        if (IsBypassEverythingElse(*policySelector)
            || IsDropEverythingElse(*policySelector)
            || policySelector->iDirection == KPolicySelector_INTERFACE)
            {
            continue;
            }

        // Convert local mask of policy to V4Mapped
        if (policySelector->iLocal.IsV4Mapped() && !polLocalMask.IsV4Mapped())
            {
            polLocalMask.ConvertToV4Mapped();
            }

        // Convert remote mask of policy to V4Mapped
        if (policySelector->iRemote.IsV4Mapped() && !polRemoteMask.IsV4Mapped())
            {
            polRemoteMask.ConvertToV4Mapped();
            }

        // Check that protocol match if it is set
        if ((policySelector->iProtocol) && (iSelectorInfo->iProtocol))
            {
            if (policySelector->iProtocol != iSelectorInfo->iProtocol)
                {
                continue;
                }
            }
            
        // Check that scope match if selector is 'scoped' ie. not global 
        if (!policySelector->iGlobalSelector)
            {
            if ((policySelector->iLocal.Scope() != localAddr.Scope())
                || (policySelector->iRemote.Scope() != remoteAddr.Scope()))
                {
                continue;
                }
            }

        // Check that local address/port match if address/port is set
        if (!policySelector->iLocal.IsUnspecified())
            {
            if (!policySelector->iLocal.Match(localAddr, polLocalMask)
                || !policySelector->iLocal.CmpPort(iSelectorInfo->iLocal))
                {
                continue;
                }
            }

        // Check that remote address/port match if address/port is set
        if (!policySelector->iRemote.IsUnspecified())
            {
            if (!policySelector->iRemote.Match(remoteAddr, polRemoteMask)
                || !policySelector->iRemote.CmpPort(iSelectorInfo->iRemote))
                {
                continue;
                }
            }

        // Matching selector found so return it            
        return (policySelector);
        }

    return (NULL);
    }

//
// CIPSecPolicyManagerHandler::FillSAInfoObject()
//
// Store SA parameters from TSecurityAssocSpec object to TIPSecSAInfo object. 
// The Info object will be delivered to the Key Management application. 
// The function is a part of the GetIPSecSAInfo API.
//
//
void 
CIPSecPolicyManagerHandler::FillSAInfoObject(
    CPolicySelector* aPolicySelector,
    TInt aIndex)
    {
    // Clear SA Transfrom template strings
    iSAInfo->iName.FillZ(KMaxName);
    iSAInfo->iName.SetLength(0);
    iSAInfo->iRemoteIdentity.FillZ(TIpsecSaSpec::KIpsecMaxIdentityLength);
    iSAInfo->iRemoteIdentity.SetLength(0);
    iSAInfo->iLocalIdentity.FillZ(TIpsecSaSpec::KIpsecMaxIdentityLength);
    iSAInfo->iLocalIdentity.SetLength(0);

    // Reset 'more SA exists' to default (none)
    iSAInfo->iMoreSasExist = EFalse;
 
    // Iterate through the action bundles to find the SA Transform template
    // that corresponds with the given index
    TSecpolBundleIter iter(aPolicySelector->iBundle);
    TInt count(0);
    while (iter)
        {
        CSecpolBundleItem* item = iter++;

        if (aIndex != count)
            {
            count++;
            continue;
            }

        // Corresponding bundle item found so use it to fill 
        // SA transform template
        
        TBool isLast = (!iter ? ETrue : EFalse);
        TBool isTunnelMode = (!item->iTunnel.IsUnspecified() || item->iTunnelEpName);

        iSAInfo->iTransportMode = !isTunnelMode;
        iSAInfo->iMoreSasExist = !isLast;

        // Check that SA specification exists
        
        //
        // NOTE:
        //  There is a kludge for testing the plain tunnel ie. there exists 
        //  no SA specification at all. The selector definition in the policy 
        //  is like:
        //
        //      remote 192.168.1.11 255.255.255.0 = { tunnel(10.66.77.1) }
        //
        //  and no SA definition exist that is named as 'tunnel'
        //
        if (item->iSpec)
            {
            TSecurityAssocSpec& spec = item->iSpec->iSpec;
            
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
       	    iSAInfo->iType = item->iSpec->iPropList->At(0)->iType;
            iSAInfo->iAalg = item->iSpec->iPropList->At(0)->iAalg;
            iSAInfo->iAalgLen = item->iSpec->iPropList->At(0)->iAalgLen;
            iSAInfo->iEalg = item->iSpec->iPropList->At(0)->iEalg;
            iSAInfo->iEalgLen = item->iSpec->iPropList->At(0)->iEalgLen;
            iSAInfo->iReplayWindowLength = spec.iReplayWindowLength;
            iSAInfo->iPfs = spec.iPfs;

	
			iSAInfo->iHard.iBytes = item->iSpec->iPropList->At(0)->iHard.sadb_lifetime_bytes;
            iSAInfo->iHard.iAddTime = item->iSpec->iPropList->At(0)->iHard.sadb_lifetime_addtime;
            iSAInfo->iHard.iUseTime = item->iSpec->iPropList->At(0)->iHard.sadb_lifetime_usetime;

            iSAInfo->iSoft.iAllocations = item->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_allocations;
            iSAInfo->iSoft.iBytes = item->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_bytes;
            iSAInfo->iSoft.iAddTime = item->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_addtime;
            iSAInfo->iSoft.iUseTime = item->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_usetime;
#else
            iSAInfo->iType = spec.iType;
            iSAInfo->iAalg = spec.iAalg;
            iSAInfo->iAalgLen = spec.iAalgLen;
            iSAInfo->iEalg = spec.iEalg;
            iSAInfo->iEalgLen = spec.iEalgLen;
            iSAInfo->iReplayWindowLength = spec.iReplayWindowLength;
            iSAInfo->iPfs = spec.iPfs;

            iSAInfo->iHard.iAllocations = spec.iHard.sadb_lifetime_allocations;
            iSAInfo->iHard.iBytes = spec.iHard.sadb_lifetime_bytes;
            iSAInfo->iHard.iAddTime = spec.iHard.sadb_lifetime_addtime;
            iSAInfo->iHard.iUseTime = spec.iHard.sadb_lifetime_usetime;

            iSAInfo->iSoft.iAllocations = spec.iSoft.sadb_lifetime_allocations;
            iSAInfo->iSoft.iBytes = spec.iSoft.sadb_lifetime_bytes;
            iSAInfo->iSoft.iAddTime = spec.iSoft.sadb_lifetime_addtime;
            iSAInfo->iSoft.iUseTime = spec.iSoft.sadb_lifetime_usetime;

#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

			iSAInfo->iSrcSpecific = spec.iMatchSrc;

            if (item->iSpec->iName)
                {
                TInt len(item->iSpec->iName->Length());
                if (len < KMaxName)
                    {
                    iSAInfo->iName.Copy(*item->iSpec->iName);
                    }
                }

            if (item->iSpec->iRemoteIdentity)
                {
                TInt len(item->iSpec->iRemoteIdentity->Length());
                if (len < TIpsecSaSpec::KIpsecMaxIdentityLength)
                    {
                    iSAInfo->iRemoteIdentity.Copy(*item->iSpec->iRemoteIdentity);
                    }
                }

            if (item->iSpec->iLocalIdentity)
                {
                TInt len(item->iSpec->iLocalIdentity->Length());
                if (len < TIpsecSaSpec::KIpsecMaxIdentityLength)
                    {
                    iSAInfo->iLocalIdentity.Copy(*item->iSpec->iLocalIdentity);
                    }
                }
            }

        break;
        }
    }

//
// Returns TRUE if selector is of type 'bypass_everything_else'
// TODO:
//  This method should be included within CPolicySelector class
//
//
TBool
CIPSecPolicyManagerHandler::IsBypassEverythingElse(
    const CPolicySelector& aPolicySelector) const
    {
    return (aPolicySelector.iDirection != KPolicySelector_INTERFACE 
            && aPolicySelector.iRemote.Family() == KAFUnspec
            && aPolicySelector.iLocal.Family() == KAFUnspec
            && aPolicySelector.iRemote.Port() == 0
            && aPolicySelector.iLocal.Port() == 0
            && aPolicySelector.iProtocol == 0
            && aPolicySelector.iIcmpType < 0
            && aPolicySelector.iIcmpCode < 0
            && aPolicySelector.iType < 0
            && aPolicySelector.iBundle.IsEmpty()
            && !aPolicySelector.iDropAction);
    }

//
// Returns TRUE if selector is of type 'drop_everything_else'
// TODO:
//  This method should be included within CPolicySelector class
//
//
TBool
CIPSecPolicyManagerHandler::IsDropEverythingElse(
    const CPolicySelector& aPolicySelector) const
    {
    return (aPolicySelector.iDirection != KPolicySelector_INTERFACE
            && aPolicySelector.iRemote.Family() == KAFUnspec
            && aPolicySelector.iLocal.Family() == KAFUnspec
            && aPolicySelector.iRemote.Port() == 0
            && aPolicySelector.iLocal.Port() == 0
            && aPolicySelector.iProtocol == 0
            && aPolicySelector.iIcmpType < 0
            && aPolicySelector.iIcmpCode < 0
            && aPolicySelector.iType < 0
            && aPolicySelector.iBundle.IsEmpty()
            && aPolicySelector.iDropAction);
    }

//
// Returns TRUE if given selectors are of 'interface' type and the
// name of interfaces are equal.
// TODO:
//  This method should be included within CPolicySelector class
//
//
TBool
CIPSecPolicyManagerHandler::IsEqualInterface(
    const CPolicySelector& aP1, 
    const CPolicySelector& aP2) const
    {
    return ((aP1.iDirection == KPolicySelector_INTERFACE)
            && (aP2.iDirection == KPolicySelector_INTERFACE)
            && (aP1.iInterface == aP2.iInterface));
    }

//
// Returns TRUE if the remote address of given selectors are equal.
// TODO:
//  This method should be included within CPolicySelector class
//
//
TBool
CIPSecPolicyManagerHandler::IsEqualRemoteAddress(
    const CPolicySelector& aP1, 
    const CPolicySelector& aP2) const
    {
    // Check if both addresses are set
    if ((aP1.iRemote.Family() == KAFUnspec) 
        || (aP2.iRemote.Family() == KAFUnspec))
        {
        return (EFalse);
        }

    // Check that both selectors are either global or scoped
    if (aP1.iGlobalSelector != aP2.iGlobalSelector)
        {
        return (EFalse);
        }
    
    // Check for address any and equal scope
    if (aP1.iRemote.IsUnspecified() && aP2.iRemote.IsUnspecified())
        {
        if (aP1.iGlobalSelector && aP2.iGlobalSelector)
            {
            return (ETrue);
            }
        if (aP1.iRemote.Scope() == aP2.iRemote.Scope())
            {
            return (ETrue);
            }
        }
        
    // Check for specific address and equal scope
    if ((!aP1.iRemote.IsUnspecified() && !aP2.iRemote.IsUnspecified())
        && aP1.iRemote.Match(aP2.iRemote)
        && aP1.iRemoteMask.Match(aP2.iRemoteMask))
        {
        if (aP1.iGlobalSelector && aP2.iGlobalSelector)
            {
            return (ETrue);
            }
        if (aP1.iRemote.Scope() == aP2.iRemote.Scope())
            {
            return (ETrue);
            }
        }
        
    return (EFalse);
    }

//
// Returns TRUE if local address of given selectors are equal.
// TODO:
//  This method should be included within CPolicySelector class
//
//
TBool
CIPSecPolicyManagerHandler::IsEqualLocalAddress(
    const CPolicySelector& aP1, 
    const CPolicySelector& aP2) const
    {
    // Check that address is set
    if ((aP1.iLocal.Family() == KAFUnspec) 
        || (aP2.iLocal.Family() == KAFUnspec))
        {
        return (EFalse);
        }

    // Check that both selectors are either global or scoped
    if (aP1.iGlobalSelector != aP2.iGlobalSelector)
        {
        return (EFalse);
        }
    
    // Check for address any and equal scope
    if (aP1.iLocal.IsUnspecified() && aP2.iLocal.IsUnspecified())
        {
        if (aP1.iGlobalSelector && aP2.iGlobalSelector)
            {
            return (ETrue);
            }
        if (aP1.iLocal.Scope() == aP2.iLocal.Scope())
            {
            return (ETrue);
            }
        }
        
    // Check for specific address and equal scope
    if ((!aP1.iLocal.IsUnspecified() && !aP2.iLocal.IsUnspecified())
        && aP1.iLocal.Match(aP2.iLocal)
        && aP1.iLocalMask.Match(aP2.iLocalMask))
        {
        if (aP1.iGlobalSelector && aP2.iGlobalSelector)
            {
            return (ETrue);
            }
        if (aP1.iLocal.Scope() == aP2.iLocal.Scope())
            {
            return (ETrue);
            }
        }
        
    return (EFalse);
    }

//
// Returns TRUE if given SA Transform templates are equal
// TODO:
//  This method should be included within TSecurityAssocSpec class.
//
//
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
TBool
CIPSecPolicyManagerHandler::IsEqualSaSpec(CSecpolBundleItem* aS1, 
                                          CSecpolBundleItem* aS2) const
    {
    if ((!aS1 && aS2) || (!aS2 && aS1))
        return (EFalse);
            
    if (aS1 && aS2)
        {
        if((aS1->iSpec) && (aS1->iSpec->iPropList) && (aS2->iSpec) && (aS2->iSpec->iPropList))
            {
            if (aS1->iSpec->iPropList->At(0)->iType != aS2->iSpec->iPropList->At(0)->iType)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iAalg != aS2->iSpec->iPropList->At(0)->iAalg)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iAalgLen != aS2->iSpec->iPropList->At(0)->iAalgLen)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iEalg != aS2->iSpec->iPropList->At(0)->iEalg)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iEalgLen != aS2->iSpec->iPropList->At(0)->iEalgLen)
                return (EFalse);
            if (aS1->iSpec->iSpec.iPfs != aS2->iSpec->iSpec.iPfs)
                return (EFalse);
            if (aS1->iSpec->iSpec.iReplayWindowLength != aS2->iSpec->iSpec.iReplayWindowLength)
                return (EFalse);
            
            if (aS1->iSpec->iPropList->At(0)->iHard.sadb_lifetime_allocations != aS2->iSpec->iPropList->At(0)->iHard.sadb_lifetime_allocations)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iHard.sadb_lifetime_bytes != aS2->iSpec->iPropList->At(0)->iHard.sadb_lifetime_bytes)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iHard.sadb_lifetime_addtime != aS2->iSpec->iPropList->At(0)->iHard.sadb_lifetime_addtime)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iHard.sadb_lifetime_usetime != aS2->iSpec->iPropList->At(0)->iHard.sadb_lifetime_usetime)
                return (EFalse);
            
            if (aS1->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_allocations != aS2->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_allocations)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_bytes != aS2->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_bytes)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_addtime != aS2->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_addtime)
                return (EFalse);
            if (aS1->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_usetime != aS2->iSpec->iPropList->At(0)->iSoft.sadb_lifetime_usetime)
                return (EFalse);
            }
        }
        
    return (ETrue);
    }
#else
TBool
CIPSecPolicyManagerHandler::IsEqualSaSpec(TSecurityAssocSpec* aS1, 
                                          TSecurityAssocSpec* aS2) const
    {
    if ((!aS1 && aS2) || (!aS2 && aS1))
        return (EFalse);
            
    if (aS1 && aS2)
        {
        if (aS1->iType != aS2->iType)
            return (EFalse);
        if (aS1->iAalg != aS2->iAalg)
            return (EFalse);
        if (aS1->iAalgLen != aS2->iAalgLen)
            return (EFalse);
        if (aS1->iEalg != aS2->iEalg)
            return (EFalse);
        if (aS1->iEalgLen != aS2->iEalgLen)
            return (EFalse);
        if (aS1->iPfs != aS2->iPfs)
            return (EFalse);
        if (aS1->iReplayWindowLength != aS2->iReplayWindowLength)
            return (EFalse);
        
        if (aS1->iHard.sadb_lifetime_allocations != aS2->iHard.sadb_lifetime_allocations)
            return (EFalse);
        if (aS1->iHard.sadb_lifetime_bytes != aS2->iHard.sadb_lifetime_bytes)
            return (EFalse);
        if (aS1->iHard.sadb_lifetime_addtime != aS2->iHard.sadb_lifetime_addtime)
            return (EFalse);
        if (aS1->iHard.sadb_lifetime_usetime != aS2->iHard.sadb_lifetime_usetime)
            return (EFalse);
        
        if (aS1->iSoft.sadb_lifetime_allocations != aS2->iSoft.sadb_lifetime_allocations)
            return (EFalse);
        if (aS1->iSoft.sadb_lifetime_bytes != aS2->iSoft.sadb_lifetime_bytes)
            return (EFalse);
        if (aS1->iSoft.sadb_lifetime_addtime != aS2->iSoft.sadb_lifetime_addtime)
            return (EFalse);
        if (aS1->iSoft.sadb_lifetime_usetime != aS2->iSoft.sadb_lifetime_usetime)
            return (EFalse);
        }
        
    return (ETrue);
    }
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
//
//  Find gateway that matches with gateway parameter given by the client. 
//  This function is a part of GetAvailableSelectors function.
//
//
void CIPSecPolicyManagerHandler::FillSelectorInfoObject()
	{
	// Set the base for the selector list
    CSecurityPolicy* sp = iPieceData->Policies();
    CSelectorList* selectorList = sp->SelectorList();
    TInt selectorCount = selectorList->Count();
    TIpsecSelectorInfo selectorInfo;
    
    TInt count = 0;
    
    if(iTunnel.IsUnspecified())
    	{
    	return;
    	}
    
    for (TInt i = 0; i < selectorCount; i++)
        {
        CPolicySelector* policySelector = selectorList->At(i);
        
        TSecpolBundleIter iter(policySelector->iBundle);
        CSecpolBundleItem* item = NULL;
        
        while (iter)
        	{
        	item = (CSecpolBundleItem *)iter++;
        	count++;
        	if(item)
        		{
        		if(item->iTunnel.Family() == iTunnel.Family())
        			{
        			if(item->iTunnel.Address() == iTunnel.Address())
        				{
      					// set the gateway address, sa index,remote address, remote port, remote mask, 
      					// direction, local address, local port, local mask and protocol to the 
      					//client's address space in selectorinfo object
      					selectorInfo.iTunnel.SetAddress(item->iTunnel.Address());
        				selectorInfo.iSaIndex = count;
        				selectorInfo.iDirection = policySelector->iDirection;
						
						selectorInfo.iRemote.SetAddress(policySelector->iRemote.Address());
						selectorInfo.iRemote.SetPort(policySelector->iRemote.Port());
						selectorInfo.iRemoteMask.SetAddress(policySelector->iRemoteMask.Address());
						
						selectorInfo.iLocal.SetAddress(policySelector->iLocal.Address());
						selectorInfo.iLocal.SetPort(policySelector->iLocal.Port());
						selectorInfo.iLocalMask.SetAddress(policySelector->iLocalMask.Address());
						
						selectorInfo.iProtocol = policySelector->iProtocol;
						iSelectorInfoArray->AppendL(selectorInfo);
						
					   }
        			}
        		}
        	}
        }

	}




