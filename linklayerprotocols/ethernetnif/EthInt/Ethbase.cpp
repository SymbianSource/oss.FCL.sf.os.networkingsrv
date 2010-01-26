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
// Ethernet and 802.3 protocol suite common link layer (interface) code 
// 
//

/**
 @file
*/

#include <f32file.h>
#include <e32std.h>	// for TTime
#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <nifmbuf.h>
#include <comms-infras/nifprvar.h>
#include <comms-infras/connectionsettings.h>	// for KSlashChar
#include <commdb.h>
#include "EthProto.h"
#include "PKTDRV.H"
#include <in_sock.h> // Header is retained, but in_sock.h is modified for ipv6
#include <d32ethernet.h>
#include "CLanIp4Bearer.h"
#include "CLanIp6Bearer.h"
#include "eth_log.h"
#include <cdbcols.h>
#include <cdblen.h>
#include "CLanxBearer.h"
#include "eth_log.h"
#include "EthProvision.h"
#include <comms-infras/ss_metaconnprov.h>					// for SAccessPointConfig
#include <comms-infras/ss_log.h>
#include <comms-infras/linkmessages.h>
#include <elements/nm_messages_base.h>
#include <elements/nm_messages_child.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace Elements;

_LIT8(KDescIp, "ip");
_LIT8(KDescIcmp, "icmp");
_LIT8(KDescIp6, "ip6");

/**
Constant supports Broadcasting or Multicasting
@internalComponent
*/

/**
Packet driver Function Pointer
@internalComponent
*/
typedef CPktDrvFactory* (*TPktDrvFactoryNewL)();

EXPORT_C CLANLinkCommon::CLANLinkCommon(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
	: CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf), iMMState(EStopped)
{
   	LOG_NODE_CREATE(KEthLogTag2, CLANLinkCommon);
}

/**
Loads, creates and starts the packet driver.
The packet driver's name is found from the LAN service table in commdb.
The name then has '.drv' appended and its driver factory is found using DoFindDriverFactoryL.
If found, the packet driver is created using the factory's NewDriverL method and the driver state is set to EDrvStarted.
@note Should be called from the ConstructL phase of the CLANLinkCommon initialisation.
*/
void CLANLinkCommon::LoadPacketDriverL()
{
	TBuf<KCommsDbSvrDefaultTextFieldLength+4> drvName;	// plus 4 is for the filename extension added below

	ASSERT(iLanLinkProvision);

	if (LinkProvision().PacketDriverName().Length() == 0)
		{
		__FLOG_STATIC(KEther802LogTag1,KEthLogTag2, _L("Empty packet driver name - is .cfg file up-to-date?  See ether802.ini for information on required fields in commdb."));
		User::Leave(KErrArgument);
		}

	drvName.Copy(LinkProvision().PacketDriverName());

	if(!(drvName.Right(4)==KPacketDriverFileExtension())) // no != for comparing strings, so a bit of laboured logic
		{
		drvName.Append(KPacketDriverFileExtension);
		}

	// Ref counted library ownership to ensure the library is not unloaded prematurely
	CPktDrvFactory* pktFactory = (CPktDrvFactory *)DoCreateDriverFactoryL(TUid::Uid(KUidNifmanPktDrv), drvName);
	CleanupStack::PushL(pktFactory);
	iPktDrvOwner = new(ELeave)CPacketDriverOwner(pktFactory);
	CleanupStack::Pop();
	iPktDrvOwner->Open();

	iPktDrv=pktFactory->NewDriverL(this);
}

EXPORT_C CLANLinkCommon* CLANLinkCommon::NewL(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
	{
	CLANLinkCommon* s=new (ELeave) CLANLinkCommon(aFactory, aSubConnId, aProtocolIntf);
	CleanupStack::PushL(s);
	s->ConstructL();
	CleanupStack::Pop();
	return s;
	}

/** Ethernet Type - Encapsulation
@internalComponent
*/
_LIT(KEtherType,"encapsulation");

/**
Reads the Link layer Ethernet settings in Ether802.ini. Currently this is just the ethernet
encapsulation type, stored in parameter encapsulation in section [ethernet], sets iEtherType
on the basis of the encapsulation type: ELLCEthernet, if the encapsulation parameter = 1, or
EStandardEthernet otherwise.
*/
void CLANLinkCommon::ReadEthintSettingsL()
{
	CESockIniData* ini = CESockIniData::NewL(ETHER802_CONFIG_FILENAME);
	CleanupStack::PushL(ini);
	TInt encapType = 0;
	ini->FindVar(KEthernetSection,KEtherType,encapType);

	CleanupStack::PopAndDestroy();

	switch(encapType)
		{
		case 0 :
			__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Framing type set to Ethernet II"));
			iEtherType=EStandardEthernet;
			break;
		case 1 :
			__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Framing type set to IEEE802.3 LLC-SNAP"));
			iEtherType=ELLCEthernet;
			break;
		default :
			__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Unrecognised or no framing type in config file - framing type set to Ethernet II"));
			iEtherType=EStandardEthernet;  // not a valid type
		}
}

/**
Implementation of the pure virtual from CLANLinkBase, for use by the bearers. Returns the
hardware interface obtained from the packet layer in the aBuffer descriptor in a 6 byte binary
Big-endian format.
@return ETrue if the hardware address was non-zero, EFalse otherwise.
*/
TBool CLANLinkCommon::ReadMACSettings()
{
	TBuf8<KMACByteLength> drvMacAddr;
	TUint8* config=iPktDrv->GetInterfaceAddress();
	drvMacAddr.Append(&config[0],KMACByteLength); // First 3 bytes of config not part of address
	iMacAddr.Copy(drvMacAddr); // update Ethints copy of the MAC address
	TUint bearerLim = iBearers->Count();
	for(TUint index=0;index<bearerLim;index++)
		{
		CLanxBearer* bearer=(*iBearers)[index];
		bearer->UpdateMACAddr();
		}

	// check that the macaddress has been passed and is not 0;
	return CheckMac(iMacAddr); // wrong.
}


TUint CLANLinkCommon::Mtu() const
{
	if(iEtherType==ELLCEthernet)
		{
		__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("MTU - value for LLC frames"));
		return KEtherMaxFrame;
		}
	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("MTU - default setting"));
	return KDefaultMtuSetting;
}

/**
The CLANLinkCommon::ConstructL performs all the memory allocating initialisations. It's
iContainer is allocated, using NewL; its iPktDriverFactories is created using its
iContainer->CreateL. A dynamic array of 16 iBearers are created.
@param TDesC Unused
*/
EXPORT_C void CLANLinkCommon::ConstructL()
{
	__FLOG_STATIC(KEther802LogTag1,KEthLogTag2, _L("Ethint started"));

	// Initialise class members that do not need to be read from the database
	iBearers = new (ELeave) CLanxBearerPtrArray(16);
	iOnlyThisBearer = NULL;

	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Reading Ethint settings"));
	ReadEthintSettingsL();

#ifdef TCPDUMP_LOGGING
	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Libpcap format logging enabled"));
	iLogger = CEthLog::NewL();
	iLogger->DumpTcpDumpFileHeader();
#endif // TCPDUMP_LOGGING
}

/**
Destructor. If there are any iPktDrvFactories, these are deleted first, by closing each Factory's
RLibrary iLib handle and then deleting the object. Then all the bearers are deleted . Finally,
the packet driver, the logical device driver and the physical device driver are freed and then
the packet driver is deleted.
*/
EXPORT_C  CLANLinkCommon::~CLANLinkCommon()
{
#ifdef TCPDUMP_LOGGING
	delete iLogger;
#endif // TCPDUMP_LOGGING

	delete iPktDrv;
	if(iPktDrvOwner != NULL ) // iPktDrvOwner may not be created if leave occurs while loading the driver.
	{
		iPktDrvOwner->Close();
	}
	ASSERT(0 == iBearers->Count());
	delete iBearers;

   	LOG_NODE_DESTROY(KEthLogTag2, CLANLinkCommon);
}

/**
Checks the descriptor for a valid (non-zero) MAC address.
@param aMacAddr A reference to a the descriptor containing the new MAC
@return ETrue if the MAC address was non-zero, EFalse otherwise.
*/
TBool CLANLinkCommon::CheckMac(TDes8& aMacAddr)
{
	for (TUint i=0;i<=5;i++)
		{
		if (aMacAddr[i] != 0)
			return ETrue;
		}
	return EFalse;
}

/**
This method is intended for use when a device does not have the correct MAC address when ARP is started
(e.g. IRLAN). It must be called by the packet driver as soon as an interface address is available.
It should be called before the packet driver sends LinkLayerUp to nifman,otherwise packets could
be sent with an invalid hardware address.
*/
EXPORT_C void CLANLinkCommon::FoundMACAddrL()
{
	iValidMacAddr = ReadMACSettings();
}

/**
Link Layer Up
Called by the packet driver when it is happy
@internalTechnology
*/
EXPORT_C void CLANLinkCommon::LinkLayerUp()
{
	//some drivers have a valid MAC only after negotiation finishes
	iValidMacAddr = ReadMACSettings();
	PostDataClientStartedMessage();

	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L ("CLANLinkCommon::LinkLayerUp()"));

	/**
	   With the new comms framework, this KLinkLayerOpen always sent and then will be caught and swallowed
	   at the ipproto layer if a netcfgext is being used. */
#ifndef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	if (LinkProvision().ConfigDaemonName().Length() == 0)
  		{
#endif

#ifndef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		}
#endif

	TUint bearerLim = iBearers->Count();
	for(TUint index=0;index<bearerLim;index++)
		{
		CLanxBearer *bearer=(*iBearers)[index];
		bearer->StartSending((CProtocolBase*)this);
		}
}

EXPORT_C void CLANLinkCommon::LinkLayerDown(TInt aError, TAction aAction) // Notify the Protocol that the line is down
/**
Link Layer Down
Called by the packet driver when the link has gone down

@param aError error code
@param aAction whether a reconnect is required or not
@internalTechnology
*/
	{
	// If a stop was requested from the SCpr then this LinkLayerDown()
	// was expected. This also means the SCpr has already been sent a
	// TCFDataClient::TStopped and the packet driver has been stopped.
	// If on the other hand the stop was not requested a TDataClientGoneDown
	// message needs to be sent to the SCpr and the packet driver still
	// needs a StopInterface call - usually this would mean that aAction
	// is EReconnect
	if (iStopRequested == EFalse && iMMState == EStarted)
	    {
	   
	    PostFlowGoingDownMessage(aError, (aAction == EReconnect) ? MNifIfNotify::EReconnect : MNifIfNotify::EDisconnect);
	    iPktDrv->StopInterface();
		iMMState = EStopped;
	    }
	}

/**
Given the RMBufChain &aPdu, which contains a PktInfo at the start, Convert the PktInfo into
an ether frame header. The source and dest address is already assumed to be filled, i.e.
multi-cast or whatever. aPdu is returned modified.
@param aPdu  Contains a PktInfo.
@param aType The Ether type for the packet,
@return  An error code, KErrNone if the Ethernet header could be prepended, or KErrGeneral
		 if it could not.
*/
TInt CLANLinkCommon::EtherFrame(RMBufChain &aPdu, TUint16 aType)
{
	TInt ret;
	//To start with, we need a buffer for the ethernet header,
	//and the packet info.

	//Create an ether header
	RMBufChain ethHdr;
	TUint etherHeaderSize = (iEtherType==EStandardEthernet) ?
	KEtherHeaderSize : KEtherLLCHeaderSize;
	TRAP(ret, ethHdr.AllocL(etherHeaderSize));
	if (ret != KErrNone)
		{
		return ret;
		}
	TEtherLLCFrame *hdrBuf=(TEtherLLCFrame*)ethHdr.First()->Buffer();

	//Get the packet info:
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPdu);

	// Fill in the dest, src Mac addresses:
	hdrBuf->SetDestAddr(info->iDstAddr);
	hdrBuf->SetSrcAddr(iMacAddr);
	switch(iEtherType)
		{
		case EStandardEthernet:
			BigEndian::Put16((TUint8*)&hdrBuf->iTypeLen,aType);
			break;
		case ELLCEthernet:
			// Several fields to fill in. According to nifmbuf.h,
			// info->iLength gives the actual length of the packet.
			BigEndian::Put16((TUint8*)&hdrBuf->iTypeLen,static_cast<TInt16>(info->iLength));
			hdrBuf->iDSAP=KLLC_SNAP_DSAP;
			hdrBuf->iSSAP=KLLC_SNAP_SSAP;
			hdrBuf->iCtrl=KLLC_SNAP_CTRL;
			hdrBuf->SetOUI(0);
			hdrBuf->SetType(aType);
			break;
		default:
			// Can't handle any other kind of Ethernet header.
			return KErrGeneral;
		}
	//Remove the original first Packet - by trimming the info from the packet.
	TUint dummyLength=aPdu.Length(); // At this point, info becomes invalid.
	aPdu.TrimStart(dummyLength-info->iLength);
	//Finally, add the saved packet buffer to the mac header
	aPdu.Prepend(ethHdr);
	return KErrNone;
}

/**
FrameSend frames the aPdu packet, giving it a type field defined by aType and then sends it via
the packet driver's Send method, returning its return value. If there is an error framing the
packet, the packet is dropped and KErrGeneral is returned.
@param aPdu      The Packet to be sent (actually an RMBufPkt).
@param aProtocol A pointer to the protocol which sent the packet (ignored)
@param aType     The Type field in the Ethernet header when framing. For iEtherFrame== EStandardEthernet,
				 this is stored in the iType field, for ELLCEthernet, this is stored in the
				 iTypeHi:iTypeLo fields.
@return 0 Tells the higher layer to send no more data else
		1 Tells higher layer that it can send more data
*/
TInt CLANLinkCommon::FrameSend(RMBufChain& aPdu,TAny* /*aProtocol*/, TUint16 aType)
{
	// Call down from the Protocol
	// The data begins in the second MBuf - because the first one
	// is the info. The upper layer must have filled in the destination
	// and source Mac addresses before this method is called and must
	// supply the ether header type in aType.
	// EtherFrame strips off the info and replaces it by the proper
	// ether header.

	// Caller Flow controls off if the return to this method is <= 0
	if(EtherFrame(aPdu,aType)==KErrNone)
		{
		// Dump it to the log in pcap format
#ifdef TCPDUMP_LOGGING
		//Find absolute time now
		TTime timeNow;
		timeNow.HomeTime();

		iLogger->DumpFrame(timeNow,aPdu);
#endif

		return iPktDrv->Send(aPdu);
		}
	else
		{ // drop the packet and return an error - incorrectly formatted.
		// which error though?
		aPdu.Free();
		}
	return KErrGeneral; // Should be a better error code though.
}


/**
GetProtocolType extracts protocol header information from an ethernet packet and is called
when processing a packet.
@param  aPdu			 An Ethernet packet (actually, it is really an RMBufPkt with a packed
						 info, not an RMBufChain)
@param  aEtherType       The Ether type for the packet, which is the type field for the packet,
						 for example: KIPFrameType (=0x800) or KIP6FrameType (0x8dd). Note:
						 GetProtocolType makes no restrictions on valid EtherTypes, since
						 this is handled by the bearers.
@param  aPayloadPtr      A pointer to the beginning of the actual payload after the Ethernet
					     header (or the Ethernet LLC header).
@param  aEtherHeaderSize The size of the Ethernet header, either the standard ethernet header
						 size (KEtherHeaderSize==14), or the LLC header size (KEtherLLCHeaderSize= standard+8).
@return An error code, KErrNone, if a valid procol type could be extracted, or KErrNotSupported otherwise.
*/
TInt CLANLinkCommon::GetProtocolType(RMBufChain& aPdu,
									 TUint16& aEtherType, TUint8*& aPayloadPtr,
									 TUint& aEtherHeaderSize) const
{
	RMBuf* chnptr=aPdu.First(); // Start of received packet
	// Start of actual packet data  (Ethernet II)
	TEtherFrame *etherFrame=(TEtherFrame*)(chnptr->Next()->Ptr());

	if ((aEtherType=etherFrame->GetType())>=1536)
		{ // Standard Ethernet type
		__ASSERT_DEBUG(chnptr->Next()->Length() >= KEtherHeaderSize, Panic(EIeee802AddrBadMBuf));
		__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L ("CLANLinkCommon::GetProtocolType ()  Got Ethernet-II-type frame"));

		if (chnptr->Next()->Length() == KEtherHeaderSize)
			{
			aPayloadPtr = chnptr->Next()->Ptr();
			}
		else
			{
			aPayloadPtr = etherFrame->iData;
			}
		aEtherHeaderSize=KEtherHeaderSize;
		}
	else // It's an LLC Ethernet frame (802.3)
		{
		if(chnptr->Next()->Length() >= KEtherLLCHeaderSize)
		    {
		    TEtherLLCFrame *etherLLCFrame = (TEtherLLCFrame*)etherFrame;
		    if(etherLLCFrame->iDSAP==KLLC_SNAP_DSAP &&
			etherLLCFrame->iSSAP==KLLC_SNAP_SSAP &&
			etherLLCFrame->iCtrl==KLLC_SNAP_CTRL)
			{ // Valid LLC-SNAP Frame.
			__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L ("CLANLinkCommon::GetProtocolType ()  Got 802.3-type frame, with LLC and SNAP headers"));
			aEtherType=etherLLCFrame->GetType(); // Standard Ethernet type.
			aPayloadPtr=etherLLCFrame->iData;
			aEtherHeaderSize=KEtherLLCHeaderSize;
			}
		    else
			{
			__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L ("CLANLinkCommon::GetProtocolType ()  Got what initially looked like a 802.3-type frame, but no SNAP header"));
			aEtherType=0;
			aPayloadPtr=0;
			aEtherHeaderSize=0;
			return KErrNotSupported;
			}
		    }
		else
		    {
	            return KErrCorrupt;
		    }
		}
	return KErrNone;
}

/**
Process processes a packet received from the packet driver.
@param aPdu The Packet to be sent (actually an RMBufPkt).
@param aLLC Unclear.
*/
EXPORT_C void CLANLinkCommon::Process(RMBufChain& aPdu, TAny* /*aLLC*/)
{
	TUint16 etherCode;
	TUint8* etherPtr;
	TUint etherHeaderSize;
	TBool packetProcessed=EFalse;
	TInt ret=GetProtocolType(aPdu,etherCode,etherPtr,etherHeaderSize);
	if (ret==KErrNone)
		{ // It's an ethernet packet of some kind.

		// Dump it to the log in pcap format
#ifdef TCPDUMP_LOGGING
		//Find absolute time now
		TTime timeNow;
		timeNow.HomeTime();

		RMBufPacket& pkt = (RMBufPacket&)aPdu;
		pkt.Unpack();
		iLogger->DumpFrame(timeNow,aPdu);
		pkt.Pack();
#endif

		TUint bearerLim = iBearers->Count();
		for(TUint index=0;index<bearerLim && !packetProcessed;index++)
			{
			CLanxBearer* bearer=(*iBearers)[index];
			if (BearerIsActive(bearer))
				{
				if(bearer->WantsProtocol(etherCode,etherPtr))
					{
					RMBufPacket& pkt = (RMBufPacket&)aPdu;
					pkt.Unpack();
					pkt.TrimStart(etherHeaderSize);
					pkt.Pack();
					bearer->Process(aPdu, (TAny*)(TUint32)etherCode);	// process expects ether type, but fixed function definition...
					packetProcessed=ETrue;
					}
				}
			}
		}
	if (ret!=KErrNone || !packetProcessed)
		{
		__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L ("CLANLinkCommon::Process() - dropping packet - unrecognised type"));
		aPdu.Free(); // Something strange about the packet - bin it
		}
}

/**
Implementation of pure virtual from CLANLinkBase for use by the packet driver.
Resume Sending is a notification call into NIF from the lower layer telling the NIF that a
previous sending congestion situation has been cleared and it can accept more downstack data.
NIF subsequently calls all the bearers' StartSending() methods directly.
*/
EXPORT_C void CLANLinkCommon::ResumeSending()
{
	TUint bearerLim = iBearers->Count();
	for(TUint index=0;index<bearerLim;index++)
		{
		CLanxBearer* bearer=(*iBearers)[index];
		bearer->StartSending((CProtocolBase*)this);
		}
}

/**

*/
EXPORT_C TInt CLANLinkCommon::ReadInt(const TDesC& /*aField*/, TUint32& /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C TInt CLANLinkCommon::WriteInt(const TDesC& /*aField*/, TUint32 /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C TInt CLANLinkCommon::ReadDes(const TDesC& /*aField*/, TDes8& /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C TInt CLANLinkCommon::ReadDes(const TDesC& aField, TDes16& aValue)
{
	// Emulate reading of following parameters only:
	// LAN_BEARER\LAN_BEARER_LDD_FILENAME
	// LAN_BEARER\LAN_BEARER_LDD_NAME
	// LAN_BEARER\LAN_BEARER_PDD_FILENAME
	// LAN_BEARER\LAN_BEARER_PDD_NAME

	_LIT(KLanBearer, "LANBearer\\");
	const TInt KLanBearerTokenSize = 10;

	TPtrC field(aField.Left(KLanBearerTokenSize));	// "LANBearer"

	if (field.Compare(KLanBearer()) == 0)
		{
		_LIT(KLanBearerLddFilename, "LDDFilename");
		_LIT(KLanBearerLddName, "LDDName");
		_LIT(KLanBearerPddFilename, "PDDFilename");
		_LIT(KLanBearerPddName, "PDDName");

		field.Set(aField.Mid(KLanBearerTokenSize));	// skip "LANBearer\\"
		if (field.CompareF(KLanBearerLddFilename) == 0)
			{
			aValue.Copy(LinkProvision().LddFilename());
			}
		else
		if (field.CompareF(KLanBearerLddName) == 0)
			{
			aValue.Copy(LinkProvision().LddName());
			}
		else
		if (field.CompareF(KLanBearerPddFilename) == 0)
			{
			aValue.Copy(LinkProvision().PddFilename());
			}
		else
		if (field.CompareF(KLanBearerPddName) == 0)
			{
			aValue.Copy(LinkProvision().PddName());
			}
		else
			{
			return KErrNotSupported;
			}
		return KErrNone;
		}
	else
		{
		return KErrNotSupported;
		}
}

/**

*/
EXPORT_C TInt CLANLinkCommon::WriteDes(const TDesC& /*aField*/, const TDesC8& /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C TInt CLANLinkCommon::WriteDes(const TDesC& /*aField*/, const TDesC16& /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C TInt CLANLinkCommon::ReadBool(const TDesC& /*aField*/, TBool& /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C TInt CLANLinkCommon::WriteBool(const TDesC& /*aField*/, TBool /*aValue*/)
{
	return KErrNotSupported;
}

/**

*/
EXPORT_C void CLANLinkCommon::IfProgress(TInt aStage, TInt aError)
{
	PostProgressMessage(aStage, aError);
}

/**

*/
EXPORT_C void CLANLinkCommon::NifEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
{
}


/**
Load a factory and check the Uid etc
function always opens factory CObject if successful
*/
CPktDrvFactory* CLANLinkCommon::DoCreateDriverFactoryL(TUid aUid2,const TDesC& aFilename)
{
	TParse parse;
	User::LeaveIfError(parse.Set(aFilename,0,0));

	CPktDrvFactory* f=0;

	TAutoClose<RLibrary> lib;
	User::LeaveIfError(lib.iObj.Load(aFilename));
	lib.PushL();
	// The Uid check
	if(lib.iObj.Type()[1]!=aUid2)
		User::Leave(KErrBadLibraryEntryPoint);

	TPktDrvFactoryNewL libEntry=(TPktDrvFactoryNewL)lib.iObj.Lookup(1);
	if (libEntry==NULL)
		User::Leave(KErrNoMemory);

	f =(*libEntry)(); // Opens CObject (can leave)

	CleanupStack::PushL(f);
	f->Install(lib.iObj); // Transfers the library object if successful

	// Can pop the library now - auto close will have no effect because handle is null
	CleanupStack::Pop();
	lib.Pop();

	return f;
}


// ======================================================================================
//
// from MFlowBinderControl

EXPORT_C MLowerControl* CLANLinkCommon::GetControlL(const TDesC8& aName)
/**
Request from upper CFProtocol to retrieve our MLowerControl class.

@param aName Protocol Name.  Presently, only "ip" and "ip6" are supported.
@return pointer to our MLowerControl class instance.  We leave on an error, hence this routine should
never return NULL.
*/
	{
	CLanxBearer* bearer=NULL;
    if (aName.CompareF(KDescIp6) == 0)
    	{
        bearer = new(ELeave) CLanIp6Bearer(this);
        }
	else if (aName.CompareF(KDescIp) == 0 || aName.CompareF(KDescIcmp) == 0)
		{
        bearer = new(ELeave) CLanIp4Bearer(this);
        }
	else
		{
		User::Leave(KErrNotSupported);
		}

    CleanupStack::PushL(bearer);
    bearer->ConstructL();

	// Must have provisioning information to operate
	ASSERT(iLanLinkProvision);
	iBearers->AppendL(bearer);
	ProvisionBearerConfigL(aName);
    CleanupStack::Pop();
    return bearer;
	}

EXPORT_C MLowerDataSender* CLANLinkCommon::BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
Request from upper CFProtocol to bind to this module.

@param aProtocol protocol name (e.g. "ip", "ip6" etc).
@param aReceiver pointer to MUpperDataReceiver instance of upper CFProtocol
@param aControl pointer to MUpperControl instance of upper CFProtocol
@return pointer to our MLowerDataSender instance (eventually passed to the upper CFProtocol)
*/
	{
	TInt index = 0;
	CLanxBearer* bearer = FindBearerByProtocolName(aProtocol, index);
	if (bearer)
		{
		bearer->SetUpperPointers(aReceiver, aControl);
		if (iValidMacAddr)
			{
			aControl->StartSending();
			}
		}
	else
		{
		User::Leave(KErrNotSupported);
		}

    iSubConnectionProvider.RNodeInterface::PostMessage(Id(), TCFControlProvider::TActive().CRef());
	return bearer;
	}

EXPORT_C void CLANLinkCommon::Unbind(MUpperDataReceiver* /*aReceiver*/, MUpperControl* aControl)
/**
Request from upper CFProtocol to unbind from this module.

@param aReceiver pointer to data receiver class of upper CFProtocol (originally passed to it in downward BindL() request)
@param aControl pointer to control class of upper CFProtocol (originally passed to it in downward BindL() request)
*/

	{
	TInt index = 0;
	CLanxBearer* bearer = FindBearerByUpperControl(aControl, index);
	ASSERT(bearer);
	iBearers->Delete(index);
	delete bearer;

	// If no-one remains bound to us, signal data client idle to SCPR
	MaybePostDataClientIdle();
	}


//
// Utilities
//

CLanxBearer* CLANLinkCommon::FindBearerByProtocolName(const TDesC8& aProtocol, TInt& aIndex) const
/**
Search the iBearers array searching for a match by protocol name.

@param aProtocol name of protocol (in)
@param aIndex on success, index of found entry (>=0), else -1.
@return pointer to CLanxBearer entry on success else NULL.
*/
	{
	TInt count = iBearers->Count();
	for (TInt i = 0 ; i < count ; ++i)
		{
		CLanxBearer* bearer = iBearers->At(i);
		if (bearer->ProtocolName() == aProtocol)
			{
			aIndex = i;
			return bearer;
			}
		}
	aIndex = -1;
	return NULL;
	}

CLanxBearer* CLANLinkCommon::FindBearerByUpperControl(const MUpperControl* aUpperControl, TInt& aIndex) const
/**
Search the iBearers array searching for a match by protocol name.

@param aProtocol name of protocol (in)
@param aIndex on success, index of found entry (>=0), else -1.
@return pointer to CLanxBearer entry on success else NULL.
*/
	{
	TInt count = iBearers->Count();
	for (TInt i = 0 ; i < count ; ++i)
		{
		CLanxBearer* bearer = iBearers->At(i);
		if (bearer->MatchesUpperControl(aUpperControl))
			{
			aIndex = i;
			return bearer;
			}
		}
	aIndex = -1;
	return NULL;
	}

// =====================================================================================
//
// Messages::ANode

EXPORT_C void CLANLinkCommon::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
/**
Routine called on an incoming message from SCPR

@param aCFMessage incoming message
@return KErrNone, else a system wide error code.
*/
    {
    CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);
	if (TEBase::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TEBase::TError::EId :
			SubConnectionError(static_cast<TEBase::TError&>(aMessage).iValue);
			break;
		case TEBase::TCancel::EId :
			CancelStartFlow();
			break;
		default:
			ASSERT(EFalse);
			}
		}
	else if (TEChild::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TEChild::TDestroy::EId :
			Destroy();
			break;
		default:
			ASSERT(EFalse);
			}
		}
	else if (TCFDataClient::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TCFDataClient::TStart::EId :
			StartFlowL();
			break;
		case TCFDataClient::TProvisionConfig::EId :
			ProvisionConfig(static_cast<TCFDataClient::TProvisionConfig&>(aMessage).iConfig);
			break;
		case TCFDataClient::TStop::EId :
			StopFlow(static_cast<TCFDataClient::TStop&>(aMessage).iValue);
			break;
		case TCFDataClient::TBindTo::EId :
            { 
			TCFDataClient::TBindTo& bindToReq = message_cast<TCFDataClient::TBindTo>(aMessage);
			if (!bindToReq.iNodeId.IsNull())
				{
				User::Leave(KErrNotSupported);
				}
			RClientInterface::OpenPostMessageClose(Id(), aSender, TCFDataClient::TBindToComplete().CRef());
            }
			break;
		default:
			ASSERT(EFalse);
			}
		}
	else if (TLinkMessage::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TLinkMessage::TAgentToFlowNotification::EId:
			{
			TLinkMessage::TAgentToFlowNotification& msg = message_cast<TLinkMessage::TAgentToFlowNotification>(aMessage);
			if(msg.iValue < KVendorSpecificNotificationStart)
				{
				User::Leave(KErrNotSupported);
				}
			else
				{
				User::LeaveIfError((iPktDrv) ? iPktDrv->Notification(
					static_cast<TAgentToNifEventType>(msg.iValue), NULL) : KErrNotSupported);
				}
			}
			break;
		default:
			ASSERT(EFalse);
			}; //endswitch
		}
	else		// (message and realm not recognised within this domain)
		{
		ASSERT(EFalse);
		}
    }

//
// Methods for handling incoming SCPR messages
//

void CLANLinkCommon::StartFlowL()
	{
	iStopRequested = EFalse;

	ASSERT(iMMState == EStopped);

	if (iSavedError != KErrNone)
		{
		// If there were errors during prior processing of the TProvisionConfig message,
		// leave here and cause a TError response to TCFDataClient::TStart.
		User::Leave(iSavedError);
		}

	iValidMacAddr = ReadMACSettings();

	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Starting Interface"));
	iMMState = EStarting;
	User::LeaveIfError(iPktDrv->StartInterface());
	}


void CLANLinkCommon::CancelStartFlow()
    {
    if (iMMState == EStarting)
        {
    	iStopRequested = ETrue;

    	iPktDrv->StopInterface();
    	PostFlowDownMessage(KErrCancel);
		}
    }

void CLANLinkCommon::StopFlow(TInt aError)
	{
	iStopRequested = ETrue;
	iMMState = EStopping;

	iPktDrv->StopInterface();
	PostFlowDownMessage(aError);
	}

void CLANLinkCommon::SubConnectionGoingDown()
	{
	}

void CLANLinkCommon::SubConnectionError(TInt /*aError*/)
	{
	}


/*
Provisioning description for Ethernet CFProtocol Flow:

- on receipt of the TProvisionConfig message, the pointer contained within is stored
  in iAccessPointConfig and the provisioning information contained within the AccessPointConfig
  array is validated:
	- TLanLinkProvision must be present.  It is added by the EtherMCPr and populated from CommsDat.  A pointer to it
	  is stored in iLanLinkProvision.
	- at least one of TLanIp4Provision or TLanIp6Provision must be present.  They are added by the EtherMCPr and
	  populated from CommsDat.

- the packet driver is loaded.

- if any of the above steps fail, the resulting error is saved in iSaveError so that when TCFDataClient::TStart message is
  subsequently received, a TError message can be sent in response (there is no response to TProvisionConfig message).
*/

void CLANLinkCommon::ProvisionConfig(const RMetaExtensionContainerC& aConfigData)
	{
	__FLOG_STATIC(KEther802LogTag1,KEthLogTag2,_L("CLANLinkCommon:\tProvisionConfig message received"));

	AccessPointConfig().Close();
	AccessPointConfig().Open(aConfigData);
	
    ASSERT(iLanLinkProvision == NULL); // Should only happen once

    iLanLinkProvision = static_cast<const TLanLinkProvision*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TLanLinkProvision::EUid, TLanLinkProvision::ETypeId)));
    if (iLanLinkProvision == NULL)
        {
        __FLOG_STATIC(KEther802LogTag1,KEthLogTag2,_L("CLANLinkCommon:\tProvisionConfig() - no Ethernet Link configuration"));
		iSavedError = KErrCorrupt;
		return;
        }

    iLanIp4Provision = static_cast<const TLanIp4Provision*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TLanIp4Provision::EUid, TLanIp4Provision::ETypeId)));
    if (iLanIp4Provision == NULL)
        {
        __FLOG_STATIC(KEther802LogTag1,KEthLogTag2,_L("CLANLinkCommon:\tProvisionConfig() - no IP4 configuration"));
        }

    iLanIp6Provision = static_cast<const TLanIp6Provision*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TLanIp6Provision::EUid, TLanIp6Provision::ETypeId)));
    if (iLanIp6Provision == NULL)
        {
        __FLOG_STATIC(KEther802LogTag1,KEthLogTag2,_L("CLANLinkCommon:\tProvisionConfig() - no IP6 config provisioned"));
        }

	if (iLanIp4Provision == NULL && iLanIp6Provision == NULL)
		{
		// At least one set of IP4/6 provisioning information must be present
		iSavedError = KErrCorrupt;
		return;
		}

    TRAPD(err, ProvisionConfigL());
    if (err != KErrNone)
        {
        iSavedError = err;
        }
    }


void CLANLinkCommon::ProvisionConfigL()
	{
	// Provision Bearers
	if (iBearers->Count())
		{
	    ProvisionBearerConfigL(KDescIp());
		ProvisionBearerConfigL(KDescIp6());
		}

	__FLOG_STATIC(KEther802LogTag1, KEthLogTag2, _L("Loading packet driver"));
    LoadPacketDriverL();
	}

void CLANLinkCommon::Destroy()
	{
	ASSERT(iMMState==EStopped);
	DeleteThisFlow();
	}

// Utility functions

void CLANLinkCommon::ProvisionBearerConfigL(const TDesC8& aName)
	{
	TInt index = -1;
	CLanxBearer* bearer = FindBearerByProtocolName(aName, index);
	if (bearer)
		{
        if (aName.CompareF(KDescIp) == 0)
		    {
		    if (iLanIp4Provision)
		        {
		        bearer->SetProvisionL(iLanIp4Provision); // CLanIp4Bearer
		        }
		    }
		else if (aName.CompareF(KDescIp6) == 0)
		    {
		    if (iLanIp6Provision)
		        {
		        bearer->SetProvisionL(iLanIp6Provision); // CLanIp6Bearer
		        }
		    }
		}
	}

void CLANLinkCommon::PostProgressMessage(TInt aStage, TInt aError)
	{
	//Be careful when sending TStateChange message as RNodeChannelInterface will override the activity id
	iSubConnectionProvider.RNodeInterface::PostMessage(Id(), TCFMessage::TStateChange(TStateChange( aStage, aError)).CRef());
	}

void CLANLinkCommon::PostFlowDownMessage(TInt aError)
	{
	if (iMMState == EStopping)
    	{
    	iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStopped(aError).CRef());
    	}
    else if (iMMState == EStarting)
    	{
    	iLastRequestOriginator.ReplyTo(Id(), TEBase::TError(TCFDataClient::TStart::Id(), aError).CRef());
    	}
    else if (iMMState == EStarted)
    	{
    	iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(aError).CRef());
    	}
    iMMState = EStopped;
	}

void CLANLinkCommon::PostFlowGoingDownMessage(TInt aError, MNifIfNotify::TAction aAction)
	{
	// TDataClientGoneDown only makes sense if the flow was actually started
	ASSERT(iMMState == EStarted);
	iMMState = EStopped;
	iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(aError, aAction).CRef());
	}

void CLANLinkCommon::PostDataClientStartedMessage()
	{
	iMMState = EStarted;
	iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStarted().CRef());
	}

void CLANLinkCommon::MaybePostDataClientIdle()
	{
	if (iBearers->Count() == 0)
		{
		iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TIdle().CRef());
		}
	}


/**
Sets the BigEndian 32-bit value at iDstAddr to aDestAddr
Support methods for TEtherLLCFrame.
Note: These Methods should really go in a file for themselves.
@param aDest The destination IP address.
*/
void TEtherLLCFrame::SetDestAddr( TDesC8& aDest)
{
	// copied a word at a time, remember BigEndian portability.
	for(TUint ix=0; ix<3; ix++)
		{
		BigEndian::Put16((TUint8*)&iDestAddr[ix],(TUint16)(((aDest[ix*2]<<8)|aDest[ix*2+1])&0xffff));
		}
}

/**
Sets the BigEndian 32-bit value at iSrcAddr to aSrcAddr.
@param aSrc The source IP address.
*/
void TEtherLLCFrame::SetSrcAddr(TDesC8& aSrc)
{
	for(TUint ix=0; ix<3; ix++)
		{
		BigEndian::Put16((TUint8*)&iSrcAddr[ix],(TUint16)(((aSrc[ix*2]<<8)|aSrc[ix*2+1])&0xffff));
		}
}

/**
aOUI is in native order, but the result is always stored in network byte order.
Can't use the bigendian methods, because they only work for 16 and 32 -bit items.
@param aOUI Byte order.
*/
void TEtherLLCFrame::SetOUI(TUint32 aOUI)
{
	//aOUI is in native order, but the result is
	//always stored in network byte order.
	//Can't use the bigendian methods, because
	//they only work for 16 and 32 -bit items.
	OUI[0] = (TUint8)((aOUI>>16)&0xff);
	OUI[1] = (TUint8)((aOUI>>8)&0xff);
	OUI[2] = (TUint8)(aOUI&0xff);
}

const TLanLinkProvision& CLANLinkCommon::LinkProvision()
	{
	return *iLanLinkProvision;
	}

void CLANLinkCommon::SetAllowedBearer(CLanxBearer* aBearer)
	{
		iOnlyThisBearer = aBearer;
	}
 TBool CLANLinkCommon::BearerIsActive(CLanxBearer* aBearer)
	{
		if (!iOnlyThisBearer ||
				aBearer == iOnlyThisBearer)
			{
			return ETrue;
			}
		return EFalse;
	}

