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
// Implementation CLanIp6Bearer class, a derived from CLanxBearer.
// History
// 15/11/01 Started by Julian Skidmore.
// 11/03		Refactored as part of the DHCP relocation work 
// 
//

/**
 @file
*/

#include <in_sock.h> // Header is retained, but in_sock.h is modified for ipv6
#include <in6_if.h>
#include <in_pkt.h>
#include <comms-infras/connectionsettings.h>
#include "CLanIp6Bearer.h"
#include "EthProto.h"
#include <comms-infras/es_protbinder.h>
#include "EthProvision.h"
#include <nifmbuf.h>

using namespace ESock;

/**
Required ConstructL() method.  Performs the following:
"	Sets a unique name for this binder instance in the iIfName field - "eth6[0x%x]" where "%x"
    is the hexadecimal address of the "this" object.
"	Update the MAC address from link using UpdateMACAddr()
"	Reads CommDb settings (e.g. IPv6 static DNS configuration) from the LAN Service Table via 
    ReadCommDbLanSettingsL().
*/
void CLanIp6Bearer::ConstructL()
{
	iIfName.Format(_L("eth6[0x%08x]"), this);
   	UpdateMACAddr();
	// ReadCommDbLanSettingsL() moved from here to point when provisioning information received
}

TInt CLanIp6Bearer::Control(TUint aLevel,TUint aName,TDes8& aOption)
{
	if ((aLevel==KCOLInterface) || (aLevel==KSOLInterface))
   		{
		switch (aName)
      		{
			case KSoIfHardwareAddr:
			 	{
				typedef TPckgBuf<TSoIfHardwareAddr> THwAddr;
				THwAddr& theHwAddrRef=((THwAddr&)aOption);
				THwAddr hwAddrCopy(theHwAddrRef);
				theHwAddrRef().iHardwareAddr.SetFamily(1); // Set the address family type
				theHwAddrRef().iHardwareAddr.SetLength(4); // The length of the family part.
				theHwAddrRef().iHardwareAddr.Append(hwAddrCopy.Mid(12,4));		// set the hardware type in the port field
				theHwAddrRef().iHardwareAddr.Append(Link()->MacAddress());
				return KErrNone;
				}
        	default:
         		return KErrNotSupported;
      		}
   		}
	return KErrNotSupported;
}

/**
ResolveMulticastIp6 is a helper method for Process. It takes a TSockAddr& destination address
and a reference to the packet and generates a destination Mac address automatically from the 
destination Ip6 address, by applying the standard KMulticastPrefix (which is the byte sequence
{0x33, 0x33}) and then appending bytes 12 to 15 of the Ip6 destination address.
@param aDstAddr The destination address from the info header on the packet
@param aPdu		The Ip6 packet (actually an RMBufPacket).
*/
void CLanIp6Bearer::ResolveMulticastIp6(TSockAddr& aDstAddr,RMBufChain& aPdu)
{ 
	// We need to obtain the IPv6 header - i.e. the destination address.
	// Octets 12 to 15 of the destination address, should be copied to
	// Octets 2 to 5 of the aDstAddr.
	// Octets 0 and 1 should be 0x3333.
	TIpv6Header* header = (TIpv6Header*)aPdu.First()->Next()->Ptr(); 
	//__ASSERT_DEBUG(header->GetVersion()==6, User::Panic(_L("Ether6:3"), 0));
	TUint ix;
	for(ix=0; ix<2; ix++)
		{
		aDstAddr[ix]=KMulticastPrefix[ix];
		}
	for(ix=2; ix<KMACByteLength; ix++)
		{
		aDstAddr[ix]=header->iDestAddrB[ix+10];
		}
}

/**
ShiftLinkLayerAddress is a helper method for Process. It takes a TSockAddr& destination address 
which already contains the correct information, but in the wrong place and copies bytes 8 to 1
3 of the aDstAddr to 0 to 5 of aDstAddr.
@param aDstAddr The destination address from the info header on the packet
*/
void CLanIp6Bearer::ShiftLinkLayerAddress(TSockAddr& aDstAddr)
{
	for(TUint ix=0; ix<KMACByteLength; ix++)
		{
		aDstAddr[ix]=aDstAddr[ix+8];
		}
}

/**
Call down from the Protocol
Real Protocol data is in the second MBuf of the RMBufChain
ARP will modify the first MBuf so it's ready for EtherXX Framing and
will have inserted the dest MAC address if it could resolve it
ARP send method returns KErrNotFound if it can't resolve and ARP
hangs onto the packet data till the address is resolved at which
point it calls us back to forward the pdu to the packet driver.
Caller Flow controls off if the return to this method is <= 0
@param aPdu		A reference to the packet to be sent (really an RMBufPkt)
@param aSource  A pointer to the source protocol.
@return The no. of bytes send.
*/
MLowerDataSender::TSendResult CLanIp6Bearer::Send(RMBufChain& aPdu)
{
	// Check that we are allowed to send data
	if (!Link()->BearerIsActive(this))
		{
			aPdu.Free();
			return ESendBlocked;
		}

	// just some checking.
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPdu);
	TUint32 family = info->iDstAddr.Family();
	__ASSERT_DEBUG(family!=KAfInet, User::Panic(_L("Ether6:1"), 0));
	if(family==KAfInet6)
		{											// Have to generate the MAC as described in 
		ResolveMulticastIp6(info->iDstAddr,aPdu); 	//the RFC2464, section 7.
		}
	else if (family==0x1)
		{
		ShiftLinkLayerAddress(info->iDstAddr);
		}
	return static_cast<MLowerDataSender::TSendResult>(Link()->FrameSend(aPdu,NULL,KIP6FrameType));
}

/**
WantsProtocol, takes an ethernet aProtocolCode code and the initial part of the packet's 
payload as input and returns ETrue if the bearer wants the packet of this type of protocol 
and payload, or EFalse otherwise. CLanIp6Bearer wants the protocol if it is an Arp message or
an Ip6 protocol (an Ip protocol, whose Version field == 6). WantsProtocol also sets 
iArpMsgNext to ETrue if the protocol was Arp or EFalse otherwise.
@param aProtocolCode The Ethernet protocol Type.
@param aPayload A pointer to the beginning of the packet payload.
@return sets iArpMsgNext to ETrue if the protocol was Arp or EFalse otherwise.
*/
TBool CLanIp6Bearer::WantsProtocol(TUint16 aProtocolCode,const TUint8* aPayload)
{
	return (aProtocolCode==KIP6FrameType && (aPayload[0]>>4)==KIP6Protocol) ? ETrue:EFalse;
}

/**
Process processes a packet received from the LinkLayer.
@param aPdu The Packet to be sent (actually an RMBufPkt).
@param aLLC Unclear.
*/
void CLanIp6Bearer::Process(RMBufChain& aPdu, TAny* /*aLLC*/)
{
	if(iUpperReceiver)
		{
		iUpperReceiver->Process(aPdu);
		}
	else
		{
	  	aPdu.Free();
	  	}
}

void CLanIp6Bearer::UpdateMACAddr()
{
	TE64Addr myEuMac;
	myEuMac.SetAddrFromEUI48(Link()->MacAddress().Ptr());
	iEuiMac.SetAddress(myEuMac);
}

/**
Read any static DNS configuration information from CommDb
ReadCommDbLanSettingsL() reads CommDb settings from the LAN Service Table.  In particular, it 
reads the fields related to IPv6 Static DNS configuration.  The LAN_IP6_DNS_ADDR_FROM_SERVER 
field is a boolean that indicates whether IPv6 Static DNS configuration is required or not.  
If FALSE, then static configuration is required.  In this case, the IPv6 primary and secondary 
DNS server addresses are read from the CommDb fields LAN_IP6_NAME_SERVER1 and LAN_IP6_NAME_SERVER2 
and stored in the iPrimaryDns and iSecondaryDns data members.  They are later presented to 
the TCP/IP stack when the Control() method is called with the KSoIfConfig option.
*/
void CLanIp6Bearer::ReadCommDbLanSettingsL()
{
	ASSERT(iProvision);
	iPrimaryDns = iProvision->PrimaryDns();
	iSecondaryDns = iProvision->SecondaryDns();
}

TInt CLanIp6Bearer::GetConfig(TBinderConfig& aConfig)
	{
    TBinderConfig6* config = TBinderConfig::Cast<TBinderConfig6>(aConfig);
    
	if(config == NULL)
		{
		return KErrNotSupported;
		}

	config->iFamily = KAfInet6;
		
	config->iInfo.iFeatures = KIfNeedsND | KIfCanMulticast;
	config->iInfo.iMtu = Link()->Mtu();
	config->iInfo.iRMtu = Link()->Mtu();
	config->iInfo.iSpeedMetric = Link()->SpeedMetric();
	
	TEui64Addr& localId = TEui64Addr::Cast(config->iLocalId);
	localId = iEuiMac;

	// If required, configure static DNS addresses

	if (!iPrimaryDns.IsUnspecified())
		{
		config->iNameSer1.SetAddress(iPrimaryDns);
		if (!iSecondaryDns.IsUnspecified())
			config->iNameSer2.SetAddress(iSecondaryDns);
		}

	return KErrNone;
	}

const TDesC8& CLanIp6Bearer::ProtocolName() const
/**
Return the protocol name of this bearer

Called from CLANLinkCommon
*/
	{
	_LIT8(KProtocolName, "ip6");
	return KProtocolName();
	}

void CLanIp6Bearer::SetProvisionL(const Meta::SMetaData* aProvision)
	{
	if (iProvision == NULL)
		{
		iProvision = static_cast<const TLanIp6Provision*>(aProvision);
   		ReadCommDbLanSettingsL();
		}
	}
	
