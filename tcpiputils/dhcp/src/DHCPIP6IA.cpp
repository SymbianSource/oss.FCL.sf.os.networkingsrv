// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements the DHCPIP6 Identity Association classes
// 
//

/**
 @file DHCPIP6IA.cpp
 @internalTechnology
*/

#include "DHCPIP6IA.h"
#include "DHCP_Std.h"
#include "DHCPServer.h"
#include <comms-infras/metatypearray.h>

#ifdef _DEBUG
#include <e32property.h>
#endif

using namespace DHCPv6;

COptionNode* CDHCPOptionIA_TA::NewL()
	{ 
	return new(ELeave)CDHCPOptionIA_TA();
	}

COptionNode* CDHCPOptionIA_NA::NewL()
	{
	return new(ELeave)CDHCPOptionIA_NA();
	}

CDHCPOptionIAAddress::~CDHCPOptionIAAddress()
    {
    iNext = NULL; //prevent the base from deleting its members
    }

TInt CDHCPOptionIAAddress::GetAddressOptionL( TPtr8& aPtr, SIPAddressInfo& aAddressInfo )
    {
    if ( aPtr.Length() <= KOptionHeaderLength || 
		 TBigEndian::GetValue( aPtr.Ptr() + KOptionCodeOffset, KOptionCodeLen ) != EIaAddr )
        {
        return KErrNotFound;
        }
    ParseL( aPtr );
    aPtr.Set( GetBodyDes() );
    iRecord.ParseL( aPtr );

	// Must ensure IP6 address is word aligned! So declare it locally
	TIp6Addr ip6addr;
	Mem::Copy(&ip6addr,iAddress.GetBodyPtr(),KIp6AddressLength);
	  
	aAddressInfo.iAddress.SetAddress( ip6addr );
	aAddressInfo.iPreferredLifeTime = iPreferredLifeTime.GetBigEndian();
	aAddressInfo.iValidLifeTime = iValidLifeTime.GetBigEndian();
	CDHCPOptionStatusCode* pStatus = static_cast<CDHCPOptionStatusCode*>(GetOptions().FindOption( EStatusCode ));
	aAddressInfo.iStatusCode = pStatus ? pStatus->GetStatusCode() : ESuccess;

    return KErrNone;
    }


TInt CDHCPOptionIAAddress::AddAddressOptionL( TPtr8& aPtr, SIPAddressInfo& aAddressInfo, TStatusCodes aStatusCodeToSend )
   {
	InitialiseL(aPtr);
	CDHCPOptionStatusCode* option = NULL;
	if(aStatusCodeToSend != EStatusUnknown)
		{
		option = static_cast<CDHCPOptionStatusCode*>(iOptions.AddNodeL(EStatusCode, KDHCPOptionStatusCodeLength));
		}
	iRecord.InitialiseL(aPtr);
	Header().SetInitialValue(KDHCPOptionIAAddressWithStatusCodeLength - KOptionHeaderLength);
	SetLength();

	SetOpCode(static_cast<TUint>(EIaAddr));
	TIp6Addr* ip6addr = reinterpret_cast<TIp6Addr*>(iAddress.GetBodyPtr() );

    //-- carefully copy one object to another. See function description
    ObjectByteCopy(ip6addr, &aAddressInfo.iAddress.Ip6Address());

	iPreferredLifeTime.SetBigEndian( aAddressInfo.iPreferredLifeTime );
	iValidLifeTime.SetBigEndian( aAddressInfo.iValidLifeTime );
	if(option)
		{
		option->SetOpCode(static_cast<TUint>(EStatusCode));
		option->SetStatusCode(aStatusCodeToSend);
		}

	return KErrNone;
	}

TPtr8 CDHCPOptionIA_TA::GetOptionsDes() const
    {
    TPtr8 ptr(GetBodyDes());
    TInt len = ptr.Length() - KDHCPOptionIA_NATInitLength;
    return TPtr8( const_cast<TUint8*>(ptr.Ptr()) + KDHCPOptionIA_IAIDLength, len, len );
    }

TPtr8 CDHCPOptionIA_NA::GetOptionsDes() const
    {
    TPtr8 ptr(GetBodyDes());
    TInt len = ptr.Length() - KDHCPOptionIA_NATInitLength;
    return TPtr8( const_cast<TUint8*>(ptr.Ptr()) + KDHCPOptionIA_NATInitLength, len, len );
    }

START_ATTRIBUTE_TABLE( SIdentityAssociationConfigInfo, KDHCPv6Persinstence, 0 )
	REGISTER_ATTRIBUTE( SIdentityAssociationConfigInfo, iIAID, TMetaNumber )
	REGISTER_ATTRIBUTE( SIdentityAssociationConfigInfo, iAddressInfo, TMetaArray<SIPAddressInfo> )
END_ATTRIBUTE_TABLE()

void SIdentityAssociationConfigInfo::Reset()
        {
        TDhcpRnd rnd;
        iIAID = rnd.Rnd( KDHCPv6IA_NANumberSpaceMin, KDHCPv6IA_NANumberSpaceMax );
        iAddressInfo.Reset();
        }

void SIdentityAssociationConfigInfo::SetAddressStatus( TInt aIndex, TStatusCodes aStatusCode )
   {
	if ( aIndex == KAddrIndexAll )
		{
		TInt count = iAddressInfo.Count();
		for(TInt addrInfoIdx=0;addrInfoIdx<count;addrInfoIdx++)
			{
			iAddressInfo[addrInfoIdx].iStatusCode = aStatusCode;
			}
		}
	else
		{
		iAddressInfo[aIndex].iStatusCode = aStatusCode;
		}
   }

TInt SIdentityAssociationConfigInfo::AddressInfo( TInt aIndex, TStatusCodes aStatusCode ) const
	{
	TInt count = iAddressInfo.Count();
	for(TInt addrInfoIdx=aIndex;addrInfoIdx<count;addrInfoIdx++)
		{
		if ( (iAddressInfo[addrInfoIdx].iStatusCode & EStatusUnknown) == aStatusCode )
			{
			return addrInfoIdx + 1;
			}
		}
	return KErrNotFound;
	}

TInt SIdentityAssociationConfigInfo::AddAddressesL( CDHCPOptionIA& aDHCPOptionIA, TMessageType aMessageType )
        {
        TInt addrInfoIdx = 0;
        TStatusCodes	statusCode = EStatusUnknown;
        TStatusCodes	statusCodeToSend = EStatusUnknown;
        TInt remove = 0;

        switch(aMessageType)
                {
                case ESolicit:
                        break;
                case ERequest:
                        statusCode = EMarkToRequest;
                        break;
                case EConfirm:
                case ERenew:
                case ERebind:
                        statusCode = ESuccess;
                        statusCodeToSend = ESuccess;
                        break;
                case ERelease:
                        statusCode = EMarkForRelease;
                        statusCodeToSend = ESuccess;
                        remove = 1;
                        break;
                case EDecline:
                        statusCode = EMarkForDecline;
                        statusCodeToSend = ENoBinding;
                        remove = 1;
                        break;
                default:
                        break;
                }
        //make a room for address options
        TInt len = aDHCPOptionIA.GetLength();
        TInt newlen = len + (KDHCPOptionIAAddressWithStatusCodeLength * iAddressInfo.Count());
    //see DhcpIP6Msg.h comment !!!!BEWARE as to how the msg buffer allocation is handled
    //so far
        CDHCPOptionIAAddress* optionIaAddress = CDHCPOptionIAAddress::NewL();
        CleanupStack::PushL( optionIaAddress );
        TPtr8 ptr( aDHCPOptionIA.GetBodyPtr(), len, newlen );
        ptr.SetLength( len );

        while ( (addrInfoIdx = AddressInfo( addrInfoIdx, statusCode )) != KErrNotFound )
                {
                SIPAddressInfo& addressInfo = iAddressInfo[addrInfoIdx - 1];
                optionIaAddress->AddAddressOptionL( ptr, addressInfo, statusCodeToSend );
                if ( remove )
                        {
                        iAddressInfo.Remove( --addrInfoIdx );
                        }
                }
        //write the IA option len into the msg buffer
    aDHCPOptionIA.Header().SetInitialValue( ptr.Length() );
    aDHCPOptionIA.SetLength();
        CleanupStack::PopAndDestroy();
        return ptr.Length() - len; //return te length of what we've just put in (address options)
        }

void SIdentityAssociationConfigInfo::ExtractAddressesL( CDHCPOptionIA& aDHCPOptionIA, TUint32 aRebindTimeSpan )
        {
        CDHCPOptionIAAddress* optionIaAddress = CDHCPOptionIAAddress::NewL();
        CleanupStack::PushL( optionIaAddress );

        TPtr8 ptr( aDHCPOptionIA.GetOptionsDes() );
        SIPAddressInfo addressInfoNew;

        while (optionIaAddress->GetAddressOptionL( ptr, addressInfoNew ) == KErrNone)
                {
                iAddressInfo.AppendL( addressInfoNew );
                }
        CleanupStack::PopAndDestroy();

        //check validity and remove what's not needed
        for(TInt addrInfoIdx=0;addrInfoIdx<iAddressInfo.Count();addrInfoIdx++)
                {
                SIPAddressInfo& addressInfo = iAddressInfo[addrInfoIdx];
/*the RFC3315 doesn't explicitly say this but since it says:
[At time T2(our aRebindTimeSpan) for an IA the client initiates a Rebind/Reply message exchange
with any available server. The message exchange is terminated when the valid lifetimes of all
the addresses assigned to the IA expire]
the following check seems like a good idea*/
                if ( addressInfo.iPreferredLifeTime > addressInfo.iValidLifeTime ||
                        aRebindTimeSpan > addressInfo.iValidLifeTime ||
                        addressInfo.iStatusCode != ESuccess )
                        {
                        addressInfo.iStatusCode = EMarkForDecline;
                        }
                }
        }

START_ATTRIBUTE_TABLE( SIdentityAssociationConfigInfoNA, KDHCPv6Persinstence, 0 )
	REGISTER_ATTRIBUTE( SIdentityAssociationConfigInfoNA, iT1, TMetaNumber )
	REGISTER_ATTRIBUTE( SIdentityAssociationConfigInfoNA, iT2, TMetaNumber )
END_ATTRIBUTE_TABLE_BASE(SIdentityAssociationConfigInfo,KDHCPv6PersinstenceId.iUid)

void SIdentityAssociationConfigInfoNA::ExtractIAOptionInfoL( CDHCPOptionIA_NA& aDHCPOptionIA )
	{
	iT1 = aDHCPOptionIA.GetT1();
	iT2 = aDHCPOptionIA.GetT2();
	ExtractAddressesL( aDHCPOptionIA, iT2 );
   }

void TInterfaceConfigInfo::AppendIAOptionsL(CDHCPMessageHeaderIP6& aMessage, TMessageType aMessageType)
     {
     CDHCPOptionIA_NA* pIA_NA = static_cast<CDHCPOptionIA_NA*>(aMessage.AddOptionL(EIaNa, KDHCPOptionIA_NATInitLength));
     pIA_NA->SetIAID(iSIdentityAssociationConfigInfoNA.IaId());
     // indicate no preference for T1 and T2
     pIA_NA->SetT1(KIA_NoTimePreference);
     pIA_NA->SetT2(KIA_NoTimePreference);
     switch(aMessageType)
             {
             case ESolicit:
                     break;
             case EConfirm:
             case ERenew:
             case ERebind:
                     pIA_NA->SetT1(iSIdentityAssociationConfigInfoNA.iT1);
                     pIA_NA->SetT2(iSIdentityAssociationConfigInfoNA.iT2);
                     //fall through
             case ERequest:
             case ERelease:
             case EDecline:
					 {
                     TInt addrOptLen = iSIdentityAssociationConfigInfoNA.AddAddressesL(*pIA_NA,aMessageType);
                     TPtr8 msg = aMessage.Message().Des();
                     msg.SetLength( msg.Length() + addrOptLen);
                     break;
					 }
             default:
                     User::Leave(KErrNotSupported);
             }

   }

void TInterfaceConfigInfo::ParseIAOptionsL(CDHCPMessageHeaderIP6& aMessage)
        {
        // Find an NA option
        CDHCPOptionIA_NA* optionIa = static_cast<CDHCPOptionIA_NA*>(aMessage.GetOptions().FindOption(EIaNa));
        if (!optionIa || optionIa->GetT1() > optionIa->GetT2())
                {//nothing to do the fact that we don't have any address is picked up when the address is being
                //congfigured
                return;
                }
        if ( aMessage.GetMessageType() == EReply )
            {
            iSIdentityAssociationConfigInfoNA.ResetAddressInfos();
            iSIdentityAssociationConfigInfoNA.ExtractIAOptionInfoL( *optionIa );
            }
        }

void TInterfaceConfigInfo::CheckForUnicast( CDHCPMessageHeaderIP6& aMessage )
        {
        COptionNode* pNode = aMessage.GetOptions().FindOption(EUnicast);
        if ( pNode )
                {
                iUseUnicast = ETrue;

				// Must ensure IP6 address is word aligned! So declare it locally
               	TIp6Addr ip6addr;
				Mem::Copy(&ip6addr,pNode->GetBodyPtr(),KIp6AddressLength);  

                iServerAddress.SetAddress( ip6addr );
                }
        }

void TInterfaceConfigInfo::GetServerAddress( TInetAddr& aAddress )	// to send/receive data
        {
        __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("TInterfaceConfigInfo::GetServerAddress unicast %d"), iUseUnicast));
        if ( iUseUnicast )
                {
                aAddress = iServerAddress;
                }
        else
                {
                aAddress.SetAddress( KDHCPv6RelayAgentsNServers );
                }
#ifdef _DEBUG
		// Simulate initialisation, renewal or rebind failure by using the wrong port.
		if( ( CDHCPServer::DebugFlags() & KDHCP_FailDiscover ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRenew ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRebind ) )
			{
			aAddress.SetPort(KDhcpv6WrongDestPort);
			}
		else
			{
			TInt destPort;
			RProperty::Get(KMyPropertyCat, KMyPropertyDestPortv6, destPort);
			aAddress.SetPort(destPort);
			}
#else
        aAddress.SetPort(KDhcpv6DestPort);
#endif
        }

/**
  * Creates one IAID for TA(SIdentityAssociationConfigInfoTA) and
  * one for NA(SIdentityAssociationConfigInfoNA) in case they don't exist yet
  * (restored after reboot, sleep,....)
  *
  * @see SIdentityAssociationConfigInfoTA, SIdentityAssociationConfigInfoNA
  * @internalTechnology
  */
void DHCPv6::TInterfaceConfigInfo::Reset()
        {
        ResetUseUnicast();
        //iSIdentityAssociationConfigInfoTA.iIAID = rnd.Rnd( KDHCPv6IA_TANumberSpaceMin, KDHCPv6IA_TANumberSpaceMax );
        iSIdentityAssociationConfigInfoNA.Reset();
        
		// Clear the DHCP server address now that we no longer have a lease.
        iServerAddress.SetAddress( KInet6AddrNone );
        }
