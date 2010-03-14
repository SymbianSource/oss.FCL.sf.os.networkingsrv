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
// Implementation CLanIp4Bearer class, a derived from CLanxBearer.
// History
// 15/11/01 Started by Julian Skidmore.
// 11/03		Refactored as part of DHCP relocation work 
// 
//

/**
 @file
*/

#include <in_sock.h> // Header is retained, but in_sock.h is modified for ipv6
#include <in_pkt.h>
#include <in6_if.h>
#include <nifvar.h>
#include <comms-infras/connectionsettings.h>
#include "CLanIp4Bearer.h"
#include "arp_hdr.h"
#include <commdb.h>
#include <cdblen.h>
#include "EthProto.h"
#include "eth_log.h"
#include "ProtocolHeaders.h"
#include <comms-infras/es_protbinder.h>
#include "EthProvision.h"
#include <nifmbuf.h>

using namespace ESock;

/**
Required ConstructL() method.  Performs the following:
"	Sets a unique name for this binder instance in the iIfName field - "eth6[0x%x]" where "%x"
    is the hexadecimal address of the "this" object.
"	Reads the IP Interface Settings using ReadIPInterfaceSettingsL().
"	Reads CommDb settings (e.g. IPv6 static DNS configuration) from the LAN Service Table via 
    ReadCommDbLanSettingsL().
*/
void CLanIp4Bearer::ConstructL()
{ 
	iIfName.Format(_L("eth[0x%08x]"), this);
	// ReadCommDbLanSettingsL() moved from here to point when provisioning information received
}

TInt CLanIp4Bearer::Control(TUint aLevel, TUint aName, TDes8& aOption)
	{
	if ((aLevel==KCOLInterface) || (aLevel==KSOLInterface))
		{
		switch (aName)
			{
			case KSoIfHardwareAddr:
				{
				if (static_cast<TUint>(aOption.Length())<sizeof(TSoIfHardwareAddr))
					{	
					return KErrArgument;
					}
				TSoIfHardwareAddr& opt=*(TSoIfHardwareAddr*)aOption.Ptr();
				opt.iHardwareAddr.SetFamily(1); // Set up the address family type
				opt.iHardwareAddr.SetPort(1);	// Hardware Type, has to be returned in the port field...
				opt.iHardwareAddr.SetLength(8); // The length so far, 4 for family + 4 for port...
				opt.iHardwareAddr.Append(Link()->MacAddress());
	         	return KErrNone;
	         	}
	         default:
	         	return KErrNotSupported;
			}
		}
	return KErrNotSupported;		// keep compiler happy - all returns covered by switch above
	}


_LIT8(KEtherMACMultiCastPrefix,"\x01\x00\x5e");

/**
Call down from the Protocol
Real Protocol data is in the second MBuf of the RMBufChain
The info block contains important addressing information;
  If the destination address family is KAfInet,
    then we must generate the hardware destination address
    since it could not be resolved by ARP. It is probably
    a broadcast or multicast address.
  Otherwise,
    the destination address contains a resolved hardware
    address and we only need to shift the address bytes
    for the lower layer.
Caller Flow controls off if the return to this method is <= 0
@param aPdu		A reference to the packet to be sent (really an RMBufPkt)
@param aSource  A pointer to the source protocol.
@return The no. of bytes send.
*/
MLowerDataSender::TSendResult CLanIp4Bearer::Send(RMBufChain& aPdu)
{
	// Check that we are allowed to send data
	if (!Link()->BearerIsActive(this))
		{
			aPdu.Free();
			return ESendBlocked;
		}
	
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPdu);

	if (info->iDstAddr.Family() == KAfInet) 
		{
		// Create hardware addresses for multicast and broadcast destinations

		if (TInetAddr::Cast(info->iDstAddr).IsMulticast())
			{
			TInetAddr multicastDest(TInetAddr::Cast(info->iDstAddr).Address() & KArpMacMulticastMask, 0);
			info->iDstAddr.Copy(KEtherMACMultiCastPrefix);
			TUint32 addr = multicastDest.Address();
			info->iDstAddr.Append((addr&0xff0000)>>16);
			info->iDstAddr.Append((addr&0xff00)>>8);
			info->iDstAddr.Append((addr&0xff));
			}
		else
			// Catch all types of broadcast addresses (including subnet)
			{
			for(TUint ix = 0; ix < KMACByteLength; ix++)
				{
				info->iDstAddr[ix] = 0xff;
				}
			}
		}
	else
		// Shift the already resolved hardware address to the beginning
		{
		for(TUint ix = 0; ix < KMACByteLength; ix++)
			{
			info->iDstAddr[ix] = info->iDstAddr[ix + KDestAddrOffset];
			}
		}

	TInt ret = Link()->FrameSend(aPdu, NULL, (TUint)info->iProtocol == KProtocolArp ? KArpFrameType : KIPFrameType);
	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("CLanIp4Bearer::Send() sent packet"));
	return static_cast<MLowerDataSender::TSendResult>(ret);
}

/**
StartSending notifies the protocol that this object is ready to transmit and process data. 
@param aProtocol A pointer to the object which signalled it is ready to StartSending.
*/
void CLanIp4Bearer::StartSending(CProtocolBase* aProtocol)
{
	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("CLanIp4Bearer::StartSending()"));
	CLanxBearer::StartSending(aProtocol);
}

/**
WantsProtocol, takes an ethernet aProtocolCode code and the initial part of the packet's 
payload as input and returns ETrue if the bearer wants the packet of this type of protocol 
and payload, or EFalse otherwise. CLanIp4Bearer wants the protocol if it is an Arp message or
an Ip4 protocol (an Ip protocol, whose Version field == 4). WantsProtocol also sets 
iArpMsgNext to ETrue if the protocol was Arp or EFalse otherwise.
@param aProtocolCode The Ethernet protocol Type.
@param aPayload A pointer to the beginning of the packet payload.
@return sets iArpMsgNext to ETrue if the protocol was Arp or EFalse otherwise.
*/
TBool CLanIp4Bearer::WantsProtocol(TUint16 aProtocolCode,const TUint8* aPayload)
{
	return ((aProtocolCode==KIPProtocol && (aPayload[0]>>4)==KIP4Protocol) || (aProtocolCode==KArpFrameType))  ? ETrue:EFalse;
}

/**
Process processes a packet received from the LinkLayer.
@param aPdu The Packet to be sent (actually an RMBufPkt).
@param aLLC Unclear.
*/
void CLanIp4Bearer::Process(RMBufChain& aPdu, TAny* aLLC)
{
	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("CLanIp4Bearer::Process(RMBufChain& aPdu,TAny* aLLC)"));

	if (iUpperReceiver)
		{
		// Set the protocol type in the info header if ARP packet
		RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPdu);
		if (reinterpret_cast<TUint32>(aLLC) == KArpFrameType)
			{
			__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("CLanIp4Bearer::Process() - Received ARP packet"));
			info->iProtocol = KProtocolArp;
			}

		__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("CLanIp4Bearer::Process() - calling iProtocol->Process(aPdu,..)"));
		iUpperReceiver->Process(aPdu);
		}
	else
		{
	  	aPdu.Free();
	  	}
}

/**
ReadCommDbLanSettingsL() reads CommDb settings from the LAN Service Table.  In particular, it
reads the fields related to IPv4 Static DNS configuration.  The LAN_IP4_DNS_ADDR_FROM_SERVER 
field is a boolean that indicates whether IPv4 Static DNS configuration is required or not.  
If FALSE, then static configuration is required.  In this case, the IPv4 primary and secondary 
DNS server addresses are read from the CommDb fields LAN_IP4_NAME_SERVER1 and LAN_IP4_NAME_SERVER2 
and stored in the iPrimaryDns and iSecondaryDns data members.  They are later presented to 
the TCP/IP stack when the Control() method is called with the KSoIfConfig option.
*/
void CLanIp4Bearer::ReadCommDbLanSettingsL()
{
	ASSERT(iProvision);
	
	iLocalAddr = iProvision->LocalAddr();
	iNetMask = iProvision->NetMask();
	iDefGateway = iProvision->DefGateway();
	iPrimaryDns = iProvision->PrimaryDns();
	iSecondaryDns = iProvision->SecondaryDns();
}


TInt CLanIp4Bearer::GetConfig(TBinderConfig& aConfig)
	{
    TBinderConfig4* config = TBinderConfig::Cast<TBinderConfig4>(aConfig);
    
	if(config == NULL)
		{
		return KErrNotSupported;
		}

	config->iFamily = KAfInet;		/* KAfInet / KAfInet6 - selects TBinderConfig4/6 */

	config->iInfo.iFeatures = KIfCanBroadcast | KIfCanMulticast | KIfNeedsND;		/* Feature flags */
	config->iInfo.iMtu = Link()->Mtu();					/* Maximum transmission unit. */
	config->iInfo.iRMtu = Link()->Mtu();					/* Maximum transmission unit for receiving. */
	config->iInfo.iSpeedMetric = Link()->SpeedMetric();	/* approximation of the interface speed in Kbps. */
	
	config->iAddress.SetAddress(iLocalAddr);			/* Interface IP address. */
	config->iNetMask.SetAddress(iNetMask);			/* IP netmask. */
	config->iBrdAddr.SetAddress(iBroadcastAddr);		/* IP broadcast address. */
	config->iDefGate.SetAddress(iDefGateway);		/* IP default gateway or peer address (if known). */
	config->iNameSer1.SetAddress(iPrimaryDns);		/* IP primary name server (if any). */
	config->iNameSer2.SetAddress(iSecondaryDns);		/* IP secondary name server (if any). */
	
	return KErrNone;
	}

const TDesC8& CLanIp4Bearer::ProtocolName() const
/**
Return the protocol name of this bearer

Called from CLANLinkCommon
*/
	{
	_LIT8(KProtocolName, "ip");
	return KProtocolName();
	}

void CLanIp4Bearer::SetProvisionL(const Meta::SMetaData* aProvision)
	{
	if (iProvision == NULL)
		{
		iProvision = static_cast<const TLanIp4Provision*>(aProvision);
		ReadCommDbLanSettingsL();
		}
	}
	
