// portions Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IPSecPolManConflict - IPSec Policy Manager policy conflict checking routines
//
// Save as expressly licensed to you by Symbian Software Ltd, all rights reserved.
//

#include <in_sock.h>

#include "ipsecpolmanhandler.h"
#include "ipsecpolparser.h"
#include "ipsecpolapi.h"

//
// Calculates and returns the BypassDrop mode of given security policy.
// This method gets called whenever a new policy is parsed.
//
//
TInt
CIPSecPolicyManagerHandler::CalculatePolicyBypassDropMode(
    CSecurityPolicy& aSp) const
    {
    CSelectorList* list = aSp.SelectorList();
    TInt count(list->Count());
    TBool isInboundDropMode(EFalse);
    TBool isOutboundDropMode(EFalse);

    // Default mode is 'drop_everything_else' if nothing is specified in the
    // supplied policy
    TInt mode(KDropMode);

    // Iterate through the selector list
    for (TInt i = 0; i < count; i++)
        {
        CPolicySelector* ps = list->At(i);

        // Check if pure INBOUND with 'bypass_everything_else' action
        if (ps->iDirection == KPolicySelector_INBOUND
            && IsBypassEverythingElse(*ps)
            && !isInboundDropMode)
            {
            // Set Inbound bypass mode
            mode |= KInboundBypass;
            }

        // Check if pure INBOUND with 'drop_everything_else' action
        if (ps->iDirection == KPolicySelector_INBOUND
            && IsDropEverythingElse(*ps)
            && !isInboundDropMode)
            {
            // Clear Inbound bypass mode if drop requested and set mode to drop
            // so that if there exist conflicting bypass rule in the selector
            // list it does not override the drop mode ie. drop mode allways
            // overrides the bypass mode
            mode &= ~KInboundBypass;
            isInboundDropMode = ETrue;
            }

        // Check if pure OUTBOUND with 'bypass_everything_else' action
        if (ps->iDirection == KPolicySelector_OUTBOUND
            && IsBypassEverythingElse(*ps)
            && !isOutboundDropMode)
            {
            // Set Outbound bypass mode
            mode |= KOutboundBypass;
            }

        // Check if pure OUTBOUND with 'drop_everything_else' action
        if (ps->iDirection == KPolicySelector_OUTBOUND
            && IsDropEverythingElse(*ps)
            && !isOutboundDropMode)
            {
            // Clear Outbound bypass mode if drop requested and set mode to drop
            // so that if there exist conflicting bypass rule in the selector
            // list it does not override the drop mode ie. drop mode allways
            // overrides the bypass mode
            mode &= ~KOutboundBypass;
            isOutboundDropMode = ETrue;
            }
        }

    return (mode);
    }

//
// Determines the BypassDrop mode of combined policy that will be loaded into 
// the IPsec protocol component. This function gets called when Activate 
// or Unload request is entered.
//
// The combined policy loaded into the protocol component contains the
// 'drop_everything_else' rule if one or more active policies contains
// this rule. Otherwise 'bypass_every_else' rule is loaded.
//
//
TBool
CIPSecPolicyManagerHandler::CalculateCombinedPolicyBypassDropMode()
    {
    LOG(Log::Printf(_L("CalculateCombinedPolicyBypassDropMode\n")));

    // Combined mode is 'bypass_everything_else' by default
    TInt combinedMode(KInboundBypass | KOutboundBypass);

    // Iterate through the policy list to determine the combined mode
    TInt count(iActivePolicyList->Count());
    for (TInt i = 0; i < count; i++)
        {
        TActivePolicyListEntry* entry = iActivePolicyList->At(i);

        // Check if the policy is activated
        if (!entry->iActiveState)
            continue;

        // Check if active policy contains 'drop_everything_else' rule and
        // then set combined mode to 'drop' and stop iteration
        if (entry->iBypassOrDropMode == KDropMode)
            {
            combinedMode = KDropMode;
            break;
            }

        // Check if active policy contains 'drop_inbound_everything_else' and
        // then clear the 'inbound_bypass' flag of the combined mode
        if (!(entry->iBypassOrDropMode & KInboundBypass))
            {
            combinedMode &= ~KInboundBypass;
            }

        // Check if active policy contains 'drop_outbound_everything_else' and
        // then clear the 'outbound_bypass' flag of the combined mode
        if (!(entry->iBypassOrDropMode & KOutboundBypass))
            {
            combinedMode &= ~KOutboundBypass;
            }
        }

    // Save calculated Bypass/Drop mode for later use and return 
    // TRUE if mode changed. The saved mode is used when loading
    // the combined policy into IPsec protocol component
    TBool changed = (iBypassOrDropMode != combinedMode);
    iBypassOrDropMode = combinedMode;
    LOG(Log::Printf(_L("combined policy mode %d\n"), iBypassOrDropMode));	
    return(changed);
    }

//
// This function controls the checking of conflicts relating to the selectors
// in the new policy and the policies existing in the Active Policy list.
//
// There are two main conflict reasons:
//  -- Port and protocol parameters are not matching,
//  -- Incorrectly overlapping addresses.
//
// If a conflict is found, the iLastError member is filled with Info
// parameter of the old policy.
//
//
void
CIPSecPolicyManagerHandler::CheckSelectorConflictsL()
    {
    TInt j(0);
    TInt count(iActivePolicyList->Count());

    //
    // If the Active Policy list is empty no checking is done
    //
    if (!count)
        return;

    //
    // Take the next policy from the active policy list and parse the
    // policy. Do the port/protocol conflict tests against the new policy
    //
    for (j = 0; j < count; j++)
        {
        TakeNextActivePolicyL(j);
        ConflictTestForPortsAndProtocolL();
        }

    //
    // Take the next policy from the active policy list and parse the
    // policy. Do the overlapping address conflict tests.
    //
    for (j = 0; j < count; j++)
        {
        TakeNextActivePolicyL(j);

        // Set base for the first selector of new policy
        CSecurityPolicy* sp = iPieceData->Policies();
        CSelectorList* list = sp->SelectorList();
        TInt count = list->Count();

        // Iterate selectors and make comparison if necessary
        for (TInt i = 0; i < count; i++)
            {
            CPolicySelector* ps = list->At(i);

            // If 'bypass/drop_everything_else' action then skip comparison
            if (IsBypassEverythingElse(*ps) || IsDropEverythingElse(*ps))
                {
                continue;
                }
            
            // Compare selectors. If error found, the called function
            // calls ErrorHandling and stores the content of Info parameter of
            // old policy to the iLastError buffer
            CompareSelectorsL(ps);
            }
        }
    }

//
// This function takes the next policy from the active policy list,
// parses it to object format, and returns the parsed policy data
// in iPieceData2 member
//
//
void
CIPSecPolicyManagerHandler::TakeNextActivePolicyL(TInt aIndex)
    {
    // Release the previous parsed PieceData object
    delete iPieceData2;
    iPieceData2 = 0;

    // Allocate a new PieceData object
    iPieceData2 = new (ELeave) CIpSecurityPiece;
    if (!iPieceData2)
        {
        ErrorHandlingL(ENoMemory, 0);
        }
    iPieceData2->ConstructL();

    // Take the policy buffer of the next active policy
    HBufC8* policyHBufC8 = iActivePolicyList->At(aIndex)->iPolicyBuf;

    // Copy policy to 16bit buffer
    TPtr8 ptr8 = policyHBufC8->Des();
    TInt length = ptr8.Length();
    HBufC *policyDataHBufC16 = HBufC::NewL(length + 1);
    CleanupStack::PushL(policyDataHBufC16);
    TPtr ptr(policyDataHBufC16->Des());
    ptr.FillZ();
    ptr.Copy(ptr8);
    ptr.ZeroTerminate();

    // Parse the policy
    TIpSecParser parser(ptr);
    TInt err = parser.ParseAndIgnoreIKEL(iPieceData2);
    CleanupStack::PopAndDestroy();

    if (err != KErrNone)
        {
        ErrorHandlingL(EParsingError, err);
        }
    return;
    }

//
// This function checks whether there are conflicts between the new policy
// and old policies relating to the ports and protocol parameters
// in selectors.
//
//
void
CIPSecPolicyManagerHandler::ConflictTestForPortsAndProtocolL()
    {
    const TInt KAddMip4BypassSelectors = (1 << 3);

    // Set base for the selectors of old policy
    CSecurityPolicy *oldSp = iPieceData2->Policies();
    CSelectorList* oldSelectorList = oldSp->SelectorList();
    TInt oldSelectorCount = oldSelectorList->Count();

    // Set base for the selectors of new policy
    CSecurityPolicy *newSp = iPieceData->Policies();
    CSelectorList* newSelectorList = newSp->SelectorList();
    TInt newSelectorCount = newSelectorList->Count();

    // Take the selectors of new policy one by one and
    // search a selector in the old policy that has the
    // same address/mask
    for (TInt i = 0; i < newSelectorCount; i++)
        {
        CPolicySelector *newPolicySelector = newSelectorList->At(i);

        // Iterate into next selector if 'bypass/drop_everything_else'
        if (IsBypassEverythingElse(*newPolicySelector) 
            || IsDropEverythingElse(*newPolicySelector))
            {
            continue;
            }
        
        // Search a selector from the old policy that address or interface equals
        for (TInt j = 0; j < oldSelectorCount; j++)
            {
            CPolicySelector *oldPolicySelector = oldSelectorList->At(j);

            // Iterate into next selector if 'bypass/drop_everything_else'
            if (IsBypassEverythingElse(*oldPolicySelector) 
                || IsDropEverythingElse(*oldPolicySelector))
                {
                continue;
                }

            // Check if different 'interface' and addresses
            if (!IsEqualInterface(*newPolicySelector, *oldPolicySelector)
                && !IsEqualRemoteAddress(*newPolicySelector, *oldPolicySelector)
                && !IsEqualLocalAddress(*newPolicySelector, *oldPolicySelector))
                {
                continue;
                }

            TInt remotePort(oldPolicySelector->iRemote.Port());
            TInt localPort(oldPolicySelector->iLocal.Port());

            // Iterate to next selector if DHCP bypass is requested and selector
            // contains ports utilized with DHCP
            if ((iFunction & KAddDhcpBypassSelectors) 
                && (remotePort == 67 || localPort == 68))
                {
                continue;
                }
            //UMA support REQ 417-40027  
            //loading bypass policy with activated drop_mode policy. Above mentioned is only one way traffic.
            if ((iIPSecGANSupported) && (iFunction & KAddDhcpBypassSelectors) 
                && (remotePort == 68 || localPort == 67))
                {
                continue;
                }	

            // Iterate to next selector if IKE bypass is requested and selector 
            // contains ports utilized with IKE
            if ((iFunction & KAddIkeBypassSelectors)
                && (localPort == 500 || localPort == 4500))
                {
                continue;
                }

            // Iterate to next selector if MIPv4 bypass is requested and selector 
            // contains ports utilized with MIPv4
            if ((iFunction & KAddMip4BypassSelectors)
                && (localPort == 434 || remotePort == 434))
                {
                continue;
                }
  
            // Matching address found. Check the ports and protocol,
            // call ComparePortProtocol(). Return codes are:
            //     0 = Both new and old are without port/protocol
            //     1 = Only new or old contains port/protocol
            //     2 = Matching port/protocol and SAs
            //     3 = Matching port/protocol, but not matching SAs
            //     4 = Different port/protocols
            TInt status = ComparePortProtocol(oldPolicySelector, newPolicySelector);
            
            if (status == 0)             
                {
                // Ok: Both new and old are without port/protocol
                break;                   
                }

            if (status == 1)
                {
                // Error: Only new or old contains port/protocol
                BuildConflictInfoL();
                }

            if (status == 2)
                {
                // Ok: Matching port/protocol and SAs
                break;
                }

            if (status == 3)
                {
                // Ok: Matching port/protocol, but not matching SAs
                BuildConflictInfoL();
                }

            if (status == 4)
                {
                // Ok: Different port/protocols
                continue;
                }
            }
        }
    }

//
// This function checks whether there are the same port and protocol
// codes in two selectors (in old and new policy).
// The return codes are:
//     0 = Both new and old are without port/protocol
//     1 = Only new or old contains port/protocol
//     2 = Matching port/protocol and SAs
//     3 = Matching port/protocol, but not matching SAs
//     4 = Different port/protocol
//
//
TInt
CIPSecPolicyManagerHandler::ComparePortProtocol(
    CPolicySelector *aPolicySelector1,
    CPolicySelector *aPolicySelector2)
    {
    // Check whether the selector 1 and 2 have port and/or protocol
    TInt portProtocolExists1 = 0;
    TInt portProtocolExists2 = 0;
    if (aPolicySelector1->iRemote.Port() != 0 ||
        aPolicySelector1->iLocal.Port() != 0 ||
        aPolicySelector1->iProtocol != 0)
        {
        portProtocolExists1 = 1;
        }
    if (aPolicySelector2->iRemote.Port() != 0 ||
        aPolicySelector2->iLocal.Port() != 0 ||
        aPolicySelector2->iProtocol != 0)
        {
        portProtocolExists2 = 1;
        }

    // Check if both selectors are without port/protocol
    TInt paramCount = portProtocolExists1 + portProtocolExists2;
    if (paramCount == 0)
        {
        return 0;                    // 0 = Both new and old are without port/protocol
        }

    // If the selector 1 contains port or protocol and the selector 2 not,
    // or vice versa, set return code 1

    if (paramCount == 1)
        {
        return 1;                    // 1 = Only new or old contains port/protocol
        }

    // Both the selector 1 and selector 2 have port and/or protocol, check if
    // they have the same values

    if (aPolicySelector1->iRemote.Port() == aPolicySelector2->iRemote.Port() 
        && aPolicySelector1->iLocal.Port() == aPolicySelector2->iLocal.Port() 
        && aPolicySelector1->iProtocol == aPolicySelector2->iProtocol )
        {
        // The selectors have the same values for port and protocol, compare
        // the SA block parameters
        TInt err = CompareSAParameters(aPolicySelector2, aPolicySelector1);
        if (err == KErrNone)
            {
            return 2;                // 2 = Matching port/protocol and SAs
            }
        else
            {
            return 3;                // 3 = Matching port/protocol, but not matching SAs
            }
        }
        
    return 4;                        // 4 = Different port/protocol
    }

//
// Conflict found:
// Allocate buffer and copy to it the info section of old
// policy file. Call ErrorHandling that leaves
//
//
void
CIPSecPolicyManagerHandler::BuildConflictInfoL()
    {
    delete iLastConflictInfo;
    iLastConflictInfo = 0;
    HBufC* infoBfr = iPieceData2->Info();
    TPtr ptr = infoBfr->Des();
    TInt length = ptr.Length();
    iLastConflictInfo = HBufC8::NewL(length + 1);
    TPtr8 ptr8(iLastConflictInfo->Des());
    ptr8.FillZ();
    ptr8.Copy(ptr);
    ptr8.ZeroTerminate();
    
    ErrorHandlingL(ESelectorConflict , 0);
    }

//
// This function compares a selector of new policy given as input
// parameter to the selectors of the policy taken from
// the Active policy list and checks if the selectors have overlapping
// addresses. If a conflict is found, this function calls   .
// BuildConflictInfoL() that fills the iLastConflictInfo buffer
// and leaves.
//
//
void
CIPSecPolicyManagerHandler::CompareSelectorsL(CPolicySelector* aPolicySelector)
    {
    TBool overlappingOccurs = EFalse;
    TInt err = KErrNone;

    // Set base for the selectors of old policy
    CSecurityPolicy *sp = iPieceData2->Policies();
    CSelectorList* selectorList = sp->SelectorList();
    TInt selectorCount = selectorList->Count();

    // Take the selectors of old policy one by one and
    // compare them against the selector of the new policy
    for (TInt i = 0; i < selectorCount; i++)
        {
        CPolicySelector *ps = selectorList->At(i);

        // Skip comparison if 'bypass/drop_everything_else' action 
        if (IsBypassEverythingElse(*ps) || IsDropEverythingElse(*ps))
            {
            continue;
            }
            
        //  Compare SA parameters if equal 'interface' specific selectors
        if (IsEqualInterface(*ps, *aPolicySelector))
            {
            if (!CompareSAParameters(ps, aPolicySelector))
                {
                continue;
                }
            else    
                {
                BuildConflictInfoL();
                return;
                }
            }
            
        // If no remote address, no conflict
        if (ps->iRemote.IsUnspecified() 
            && aPolicySelector->iRemote.IsUnspecified())
            {
            continue;
            }

        // If different address families, no conflict
        if (ps->iRemote.Family() != aPolicySelector->iRemote.Family())
            {
            continue;
            }
        
        // If different scope IDs, no conflict
        if (ps->iRemote.Scope() != aPolicySelector->iRemote.Scope())
            {
            continue;
            }

        // If IPV6 address and not IPv4 Mappped, no conflict (should do something)
        if (aPolicySelector->iRemote.Family() == KAfInet6 
            && !aPolicySelector->iRemote.IsV4Mapped())
            {
            continue;
            }

        // Check address overlapping
        overlappingOccurs = 
            CheckAddressOverlapping(aPolicySelector->iRemote.Address(),
                                    aPolicySelector->iRemoteMask.Address(),
                                    ps->iRemote.Address(),
                                    ps->iRemoteMask.Address());
        //UMA support
       TBool flag_exception = EFalse;
       if( iIPSecGANSupported )
           {
           flag_exception = CheckException();
           LOG(Log::Printf(_L("::CompareSelectorsL, exception policy is = %d\n"), flag_exception));
           //Not performing overlapping because UMA loads with any to any selector. Now if any to any will result in 
           //overlapping as 0.0.0.0 0.0.0.0 will encrypt every packet, which dont leads exceptions and overlapping as concern
           if(flag_exception ||iCurrentException )
               {
               continue;
               }
           }
									
        if (overlappingOccurs)
            {
            err = ESelectorConflict;

            // Overlapping addresses, check if all parameters match. If
            // match, no conflict
            if (aPolicySelector->iRemote.Address() == ps->iRemote.Address() &&
                aPolicySelector->iRemoteMask.Address() == ps->iRemoteMask.Address())
                {
                // Compare SAs.
                // If port or protocol exists, the SA tests are already done
                err = KErrNone;
                if (aPolicySelector->iRemote.Port() == 0 &&
                    aPolicySelector->iLocal.Port() == 0 &&
                    aPolicySelector->iProtocol == 0)
                    {
                    err = CompareSAParameters(aPolicySelector, ps);
                    }
                }
            if (err != KErrNone)
                {
                // Allocate buffer and copy to it the info section of old
                // policy file and call ErrorHandling that leaves
                BuildConflictInfoL();
                }
            }
        }
    }

//
// CIPSecPolicyManagerHandler::CompareSAParameters()
//
// This function compares the SA parameters of new policy to the parameters 
// of the policy taken from the Active policy list. If any parameter differs, 
// conflict found.
//
//
TInt
CIPSecPolicyManagerHandler::CompareSAParameters(
    CPolicySelector *aPolicySelectorNew,
    CPolicySelector *aPolicySelectorOld)
    {
    // Check that there is no conflicts in drop action    
    if (aPolicySelectorNew->iDropAction != aPolicySelectorOld->iDropAction)
        return (ESelectorConflict);

    // Check that there is no conflicts in bypass/transform action
    if (aPolicySelectorNew->iBundle.IsEmpty() != aPolicySelectorOld->iBundle.IsEmpty())
        return (ESelectorConflict);
    
    // Check that there is no conflicts in policy actions        
    TInt err(KErrNone);
    TSecpolBundleIter iterBundleNew(aPolicySelectorNew->iBundle);
    TSecpolBundleIter iterBundleOld(aPolicySelectorOld->iBundle);
    while (1)
        {
        CSecpolBundleItem* itemBundleNew(NULL);
        CSecpolBundleItem* itemBundleOld(NULL);
        TSecurityAssocSpec* specNew(NULL);
        TSecurityAssocSpec* specOld(NULL);

        // Retrieve next bundle items from the list
        itemBundleNew = iterBundleNew++;
        itemBundleOld = iterBundleOld++;

        if (!itemBundleNew && !itemBundleOld)
            {
            // Equal count of bundle items so stop iteration, no conflicts
            break;
            }

        if (!itemBundleNew || !itemBundleOld)
            {
            // Different count of bundle items so stop iteration, conflict
            err = ESelectorConflict;
            break;
            }

        // Verify bundle items to find out if conflicts exist 

        if (itemBundleNew->iSpec)
            {
            specNew = &itemBundleNew->iSpec->iSpec;
            }
            
        if (itemBundleOld->iSpec)
            {
            specOld = &itemBundleOld->iSpec->iSpec;
            }

        // Verify SA Transform Template attributes
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
        if (!IsEqualSaSpec(itemBundleNew, itemBundleOld))
#else
        if (!IsEqualSaSpec(specNew, specOld))
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
            {
            err = ESelectorConflict;
            break;
            }

        // Verify Remote/Local Identity values
        if (itemBundleNew->iSpec && itemBundleOld->iSpec)
            {
            TDesC8* id1 = itemBundleNew->iSpec->iRemoteIdentity;
            TDesC8* id2 = itemBundleOld->iSpec->iRemoteIdentity;

            if ((!id1 && id2) || (id1 && !id2))
                {
                err = ESelectorConflict;
                break;
                }
                
            if (id1 && id2 && (id1->Compare(*id2)))
                {
                err = ESelectorConflict;
                break;
                }

            id1 = itemBundleNew->iSpec->iLocalIdentity;
            id2 = itemBundleOld->iSpec->iLocalIdentity;

            if ((!id1 && id2) || (id1 && !id2))
                {
                err = ESelectorConflict;
                break;
                }

            if (id1 && id2 && (id1->Compare(*id2)))
                {
                err = ESelectorConflict;
                break;
                }
            }
        }

    return (err);
    }

//
// This function examines whether the addresses in the Net1 and Net2 are
// overlapping
//
// return = ETrue, overlapping addresses or illegal mask
//          EFalse, not overlapping
//
//
TBool 
CIPSecPolicyManagerHandler::CheckAddressOverlapping(
    TUint32 aNet1IpAddress,     // Net1 low address
    TUint32 aNet1Mask,          // Net1 mask
    TUint32 aNet2IpAddress,     // Net2 low address
    TUint32 aNet2Mask)          // Net2 mask
    {
    TInt err;
    TUint32 net1IpAddressLow;
    TUint32 net1IpAddressHigh;
    TUint32 net2IpAddressLow;
    TUint32 net2IpAddressHigh;

    // Calculate Net1 low and high address values (range)
    net1IpAddressLow = aNet1IpAddress & aNet1Mask;
    err = GetRangeHighAddress(net1IpAddressHigh, net1IpAddressLow, aNet1Mask);
    if (err)
        {
        return ETrue; /* Illegal mask value */
        }
    if ( net1IpAddressHigh == 0 )
        {
        net1IpAddressHigh = net1IpAddressLow;
        }

    // Calculate Net2 low and high address values (range)
    net2IpAddressLow = aNet2IpAddress & aNet2Mask;
    err = GetRangeHighAddress(net2IpAddressHigh, net2IpAddressLow, aNet2Mask);
    if (err)
        {
        return ETrue; /* Illegal mask value */
        }
    if (net2IpAddressHigh == 0)
        {
        net2IpAddressHigh = net2IpAddressLow;
        }

    // Check if Net1 and Net 2 are overlapping
    if (net1IpAddressHigh >= net2IpAddressLow 
        && net1IpAddressLow <= net2IpAddressHigh)
        {
        return ETrue;   /* overlapping occurs */
        }

    if (net2IpAddressHigh >= net1IpAddressLow 
        && net2IpAddressLow <= net1IpAddressHigh)
        {
        return ETrue;   /* overlapping occurs */
        }

    return EFalse;
    }

//
// This is a subfunction for the overlapping test. The function
// builds the high range address from the low range and mask.
//
// return = 0 = OK high range returned in aNetIpAddressHigh parameter.
//          1 = illegal mask format, 0-bit between 1-bits
//
//
TInt 
CIPSecPolicyManagerHandler::GetRangeHighAddress(
    TUint32& aNetIpAddressHigh,
    TUint32 aNetIpAddressLow,
    TUint32 aNetMask)
    {
    TUint32 refBit = 1;
    TUint32 refMask = 0xffffffff;

    // Adjust range high address from IP address/mask pair
    // Find the first (= lowest) 1-bit in mask and assure
    // that there is no high order 0-bits in mask
    while (refBit && ((aNetMask & refBit) == 0))
        {
        refMask &= (~refBit);
        refBit = refBit << 1;
        }

    if ((aNetMask & refMask) == refMask)
        {
        aNetIpAddressHigh = aNetIpAddressLow | ~aNetMask;
        refBit = 0;
        }
    else
        {
        refBit = 1;  /* error */
        }

    return refBit;
    }
