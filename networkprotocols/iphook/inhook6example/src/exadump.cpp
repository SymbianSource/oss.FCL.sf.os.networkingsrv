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
// exadump.cpp - packet dump plugin example module
// The packet dump implementation.
//



/**
 @file exadump.cpp
*/

#include <f32file.h>

#include "protocol_module.h"
#include "exadump.h"


//
// The "glue" between the protocol implementation and the generic protocol family module.
// (This section should not have any doxygen comments, because the ProtocolModule can be
// implemented multiple times in different examples -- the comments would be lost).
//
TInt ProtocolModule::NumProtocols()
	{
	return 1;
	}

void ProtocolModule::Describe(TServerProtocolDesc &aDesc, const TInt /*aIndex*/)
	{
	CProtocolExadump::Describe(aDesc);
	}

CProtocolBase *ProtocolModule::NewProtocolL(TUint aSockType, TUint aProtocol)
	{
	(void)aSockType;
	(void)aProtocol;
	return new (ELeave) CProtocolExadump;
	}

//
//
// The real implementation
//
//

void CProtocolExadump::Describe(TServerProtocolDesc &aDesc)
	/**
	* Fills in the TServerProtocolDesc.
	*
	* @retval aDesc	The description of the protocol.
	*
	* A class specific static function can be used, as the information is
	* always the same. Some other impelmentations might have non-static
	* version, if some fields of the TServerProtocolDesc are dynamic and
	* depend on the object. The following are initialized:
	*
	* - iName		The name of the protocol ("exadump").
	* - iAddrFamily	The address family of the protocol.
	* - iProtocol	The protocol number.
	*				The combination family + protocol number should
	*				be unique.
	* - iVersion	Mostly unused?
	* - iByteOrder	Mostly unused?
	* - iServiceInfo	Connectionless datagram sockets (only if SAP's)
	* - iNamingServices	Support RHostResolver and like?
	* - iSecurity	Mostly unused?
	* - iMessageSize	??
	* - iServiceTypeInfo	??
	* - iNumSockets	Max number of SAP's.)
	*
	*/
	{
/** @code */
	aDesc.iName =			_S("exadump");
	aDesc.iAddrFamily =		0x101f6cfe; // Use UID as family value.
	aDesc.iSockType =		KSockDatagram;
	aDesc.iProtocol =		1000;
	aDesc.iVersion =		TVersion(1, 0, 0);
	aDesc.iByteOrder =		EBigEndian;
	aDesc.iServiceInfo =	0;
	aDesc.iNamingServices = 0;
	aDesc.iSecurity =		0;
	aDesc.iMessageSize =	0;
	aDesc.iServiceTypeInfo = 0;
	aDesc.iNumSockets =		0;
/** @endcode */
	}


void CProtocolExadump::Identify(TServerProtocolDesc *aDesc) const
	/**
	* Returns the protocol description.
	*
	* The socket server and other protocols use this function to
	* retrieve the basic information about the protocol.
	*
	* @retval	aDesc	The protocol description.
	*/
	{
/** @code */
	Describe(*aDesc);
/** @endcode */
	}


CProtocolExadump::CProtocolExadump()
	/** Constructor. */
	{
/** @code */
	// Base time as expected by Unix systems. Note that
	// day and month numbering starts from ZERO!
	iBase.Set(_L("19700000:000000.000000"));
/** @endcode */
	}

CProtocolExadump::~CProtocolExadump()
	/**
	* Destructor.
	*
	* Release allocated resources.
	*/
	{
/** @code */
	if (iOpen > 0)
		{
		iDumpFile.Close();
		iFS.Close();
		}
	delete iBuffer;
/** @endcode */
	}

//

// CProtocolExadump::NetworkAttachedL
// ********************************
void CProtocolExadump::NetworkAttachedL()
	/**
	* When network becomes available, do the hooking.
	*
	*/
	{
/** @code */
	// Install a hook to dump all accepted packets just
	// before they are passed to the upper layer
	// protocol. The calling order of this hook, relative
	// to other hooks can be controlled through the tcpip.ini
	// option in the [hook] section
	//
	// hook_inany= *,exadump
	//
	// (call after all others) or
	//
	// hook_inany= exadump
	//
	// (call before all others).
	//
	NetworkService()->BindL(this, MIp6Hook::BindHookAll());

	// Install a hook to dump outbound packets. The calling
	// order of this hook, relative to other hooks can be
	// controlled through the tcpip.ini option in the [hook] section:
	//
	// hook_flow= *,exadump
	//
	// or even
	//
	// hook_flow= exadump,*,exadump
	//
	// which would give two dumps out of each packet. One before any other
	// flow hooks, and another one after all of them.
	//
	NetworkService()->BindL(this, MIp6Hook::BindFlowHook());
/** @endcode */
	}


//
// MIp6Hook specifications

MFlowHook *CProtocolExadump::OpenL(TPacketHead & /*aHead*/, CFlowContext * /*aFlow*/)
	/**
	* Decides whether to attach hook on flow or not.
	*
	* This example attaches every flow, but it does not require
	* a separate context for each flow. Thus, the MFlowHook has been
	* mixed into the CProtocolExadump class and it uses itself by
	* simply returning this pointer.
	*
	* @return	'this' as MFlowHook (always non-NULL)
	*/
	{
/** @code */
	Open();			// Increment reference count!
	return this;
/** @endcode */
	}

TInt CProtocolExadump::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo & aInfo)
	/**
	* Dumps the packet.
	*
	* This is called for each incoming packet. The return is always
	* KIp6Hook_PASS, because this does not try to modify or otherwise
	* change the normal packet processing.
	*
	* @param aPacket	The packet data
	* @param aInfo		The packet information
	*
	* @return KIp6Hook_PASS
	*/
	{
/** @code */
	DoPacket(aPacket, aInfo);
	return KIp6Hook_PASS;
/** @endcode */
	}


//
// MFlowHook methods, because MFlowHook is mixed in

void CProtocolExadump::Open()
	/**
	* Routes MFlowHook::Open to CProtocolPosthook::Open
	*/
	{
/** @code */
	CProtocolPosthook::Open();
/** @endcode */
	}

void CProtocolExadump::Close()
	/**
	* Routes MFlowHook::Close to CProtocolPosthook::Close
	*/
	{
/** @code */
	CProtocolPosthook::Close();
/** @endcode */
	}

TInt CProtocolExadump::ReadyL(TPacketHead & /*aHead*/)
	/**
	* Tests if flow is ready.
	*
	* In this example there is nothing that would make the flow
	* blocked, and always return KErrNone.
	*
	* @return KErrNone.
	*/
	{
	return KErrNone;
	}

TInt CProtocolExadump::ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	/**
	* Dumps the packet.
	*
	* This is called for each outgoing packet. The return is always
	* KErrNone, because this does not try to modify or otherwise
	* change the normal packet processing.
	*
	* @param aPacket	The packet data
	* @param aInfo		The packet information
	*
	* @return KErrNone
	*/
	{
/** @code */
	DoPacket(aPacket, aInfo);
	return KErrNone;
/** @endcode */
	}

//
// Own private functions

/**
* @name Some minimal definitions lifted from the PCAP headers.
*
* {@
*/
#include <sys/time.h>
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4
#define TCPDUMP_MAGIC 0xa1b2c3d4
#define DLT_RAW		12	/* raw IP */

struct pcap_file_header
	/** PCAP dump file header */
	{
	TUint32 magic;
	TUint16 version_major;
	TUint16 version_minor;
	TUint32 thiszone;	/* gmt to local correction */
	TUint32 sigfigs;	/* accuracy of timestamps */
	TUint32 snaplen;	/* max length saved portion of each pkt */
	TUint32 linktype;	/* data link type (LINKTYPE_*) */
	};

struct pcap_pkthdr
	/** PCAP dump file packet header */
	{
	struct timeval ts;	/* time stamp */
	TUint32 caplen;		/* length of portion present */
	TUint32 len;		/* length this packet (off wire) */
	};
/** @} */

void CProtocolExadump::DoPacket(const RMBufPacketBase &aPacket, const RMBufPktInfo &aInfo)
	/**
	* Dump the packet.
	*
	* This is called for both incoming and outgoing packets, from the
	* respective ApplyL methods.
	*
	* @param aPacket	The packet data
	* @param aInfo		The packet inforrmation
	*/
	{
/** @code */
	// Open the dump file, if not already opened (or attempted).
	if (iOpen == 0)
		DoOpen(1500);
	if (iOpen < 0)
		return;	// cannot open output file.

	//
	// Build PCAP frame into iBuffer (pcap_pkthdr + snapped portion of the packet)
	//
	TPtr8 buf = iBuffer->Des();
	struct pcap_pkthdr *const hdr = (struct pcap_pkthdr *)buf.Ptr();
	TPtr8 ptr((TUint8 *)buf.Ptr() + sizeof(*hdr), buf.MaxLength() - sizeof(*hdr));

	const TInt snap = aInfo.iLength > ptr.MaxLength() ? ptr.MaxLength() : aInfo.iLength;
	ptr.SetLength(snap);
	aPacket.CopyOut(ptr);

	hdr->caplen = snap;
	hdr->len = aInfo.iLength;

	TTime stamp;
	stamp.UniversalTime();
	const TTimeIntervalMicroSeconds elapsed = stamp.MicroSecondsFrom(iBase);
#ifdef I64INT
	hdr->ts.tv_usec = I64INT(elapsed.Int64() % 1000000);
	hdr->ts.tv_sec = I64INT(elapsed.Int64() / 1000000);
#else
	hdr->ts.tv_usec = (elapsed.Int64() % 1000000).GetTInt();
	hdr->ts.tv_sec = (elapsed.Int64() / 1000000).GetTInt();
#endif
	//
	// Write frame out.
	//
	iDumpFile.Write(buf, snap+sizeof(*hdr));
/** @endcode */
	}


void CProtocolExadump::DoOpen(TInt aSnaplen)
	/**
	* Try opening the dump file.
	*
	* Opens the dump file and writes the file header to it.
	*
	* @param aSnaplen The snap length
	*/
	{
/** @code */
	// Just close the previous, if called with open file.
	if (iOpen > 0)
		{
		iDumpFile.Close();
		iFS.Close();
		}

	// Allocate a working buffer for the packet frames
	// (include packet header and snaplen).
	delete iBuffer;
	iBuffer = HBufC8::NewMax(aSnaplen + sizeof(struct pcap_file_header));
	if (iBuffer == NULL)
		{
		iOpen = KErrNoMemory;
		return;
		}

	_LIT(KDumpFile, "exedump.dat");	// the name of the dump file.

	iOpen = iFS.Connect();
	if (iOpen != KErrNone)
		return;
	iOpen = iDumpFile.Replace(iFS, KDumpFile, EFileWrite);
	if (iOpen != KErrNone)
		{
		iFS.Close();
		return;
		}

	// Write the header
	TPckgBuf<struct pcap_file_header> hdr;

	hdr().magic = TCPDUMP_MAGIC;
	hdr().version_major = PCAP_VERSION_MAJOR;
	hdr().version_minor = PCAP_VERSION_MINOR;

	hdr().thiszone = 0;
	hdr().snaplen = aSnaplen;
	hdr().sigfigs = 0;
	hdr().linktype = DLT_RAW;
	iDumpFile.Write(hdr, hdr.Length());
	iOpen = 1;
	return;
/** @endcode */
	}
