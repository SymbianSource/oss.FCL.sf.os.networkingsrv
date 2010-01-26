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
// Name        : 6to4.cpp
// Part of     : 6to4 plugin / 6to4.prt
// Implements 6to4 automatic and configured tunnels, see
// RFC 3056 & RFC 2893
// Version     : 0.2
//




// INCLUDE FILES
#include <inet6log.h>
#include <f32file.h>
#include <icmp6_hdr.h>
#include <ext_hdr.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <es_prot_internal.h>
#endif

#include "6to4.h"
#include "6to4_flow.h"
#include "6to4_listener.h"
#include "6to4_tunnel.h"

// EXTERNAL DATA STRUCTURES
// EXTERNAL FUNCTION PROTOTYPES  
// CONSTANTS
// MACROS
// LOCAL CONSTANTS AND MACROS


// This is a route that is needed for system to pass automatic tunneled 
// packets (2002::/16) to the 6to4 module.
const TIp6Addr K6to4Prefix =
	{ {{0x20, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}} };

static const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };


// MODULE DATA STRUCTURES
// LOCAL FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

// ============================= LOCAL FUNCTIONS ==============================

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CProtocol6to4::CProtocol6to4
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
CProtocol6to4::CProtocol6to4 ()
	{
	}

// Destructor
CProtocol6to4::~CProtocol6to4 ()
	{
	delete iConfig;
	delete iEventListener; 
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::InitL
// Initializes the 6to4.
// ----------------------------------------------------------------------------
//
void CProtocol6to4::InitL (TDesC & aTag)
	{
	CProtocolBase::InitL (aTag);

	// There are two configuration files. The first one, 6to4.ini
	// just defines the file name where the tunnels are located.
	// Because the tunnel file is of variable length, it can't
	// be parsed in ordinary Symbian way, but it needs it's own parser.
	LoadConfigurationFile ();
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::LoadConfigurationFile
// Reads first file K6to4IniData, e.g., 6to4.ini, and finds out the file
// where the configured tunnels are defined. Then parses and creates the 
// configured tunnels.
// ----------------------------------------------------------------------------
//
TBool CProtocol6to4::LoadConfigurationFile ()
	{
	if (iConfig)
		return EFalse;            // Already loaded!

	// if iConfigErr != 0 (KErrNone), then an attempt for
	// loading configuration has been made and failed,
	// assume it never will succeed and avoid further
	// attemps on each FindVar...
	if (iConfigErr)
		return EFalse;
	
	LOG (Log::Printf (_L ("CProtocol6to4::LoadConfigurationFile(): %S\r\n"), &K6To4IniData));
	TRAP(iConfigErr, iConfig = CESockIniData::NewL (K6To4IniData));

	if (iConfigErr)
		return EFalse;
		
	if (iConfig)
		{
		TPtrC tunnelIniFile;
		// Read initialization file name, e.g., 6to4_tunnels.ini.
		if (iConfig->FindVar (K6To4IniSectionTunnels, K6To4IniTunnelFile, tunnelIniFile))
			{
			// Parse the 6to4_tunnels.ini (or what was defined in 6to4.ini).
			// NOTE: iConfigErr use must be removed when other definitions
			// than configuded tunnels are possible in K6To4IniData file -tom
			TRAP(iConfigErr, LoadFileL (tunnelIniFile));
			if (iConfigErr)
				return EFalse;
			}
		// Missing K6To4IniSectionTunnels is fine
		}
		return ETrue;
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::NetworkAttachedL
// Binds the hooks (inbound & outbound) to the IP6. Creates virtual interfaces
// for tunneled packets. The automatically tunneled packets are handled by
// virtual interface 6to4, and the configured tunneled packets by virtual
// interface named according to the tunnel name. The compatibility to the 
// TCP/IP stack version is also checked (if not compatible, a log message is 
// printed). Registers a listener for interface IP addresses. That is used for
// filtering purposes. 
// ----------------------------------------------------------------------------
//
void CProtocol6to4::NetworkAttachedL ()
	{
	// Outbound hook
	NetworkService()->BindL (this, BindFlowHook ());

	// Inbound hook, 41 == IPv6 inner
	NetworkService()->BindL (this, BindHookFor (KProtocolInet6Ipip));
	// Inbound hook, 4, == IPv4 inner
	NetworkService()->BindL(this, BindHookFor (KProtocolInetIpip));

	MInterfaceManager *const ifmgr = NetworkService ()->Interfacer();

	ifmgr->AddRouteL (K6to4Prefix, 16, K6to4);
	// Create a new virtual interface "6to4" for the 6to4. This is used for 
	// automatic tunneling. The interface index of this virtual interface  
	// is saved to this object. 
	const MInterface *const m = ifmgr->Interface (K6to4);
	iInterfaceIndex = m->Index ();

	// For each configured tunnel, the same thing is needed. The route is
	// established using configured ip address and prefix information. The
	// virtual interface gets the name according to the configured tunnel name.
	// The virtual interface index is stored to the CTunnel object. 
	TTunnelQueueIter iter (iTunnelParser.TunnelQueue());
	CTunnel *tunnel = NULL;
	while ((tunnel = iter++) != NULL)
		{
		ifmgr-> AddRouteL (tunnel->RouteAddr(),	tunnel->RoutePrefixLength(), tunnel->Name());
		ifmgr-> AddRouteL (tunnel->VirtualIfAddr(), 128, tunnel->Name(), KRouteAdd_MYPREFIX);
		const MInterface *const vif = ifmgr->Interface (tunnel->Name());
		tunnel->SetInterfaceIndex(vif->Index ());
		}

	// The event service was not available, so to make sure it's there, this
	// Check is made. Prints a log message if any problems. 
	CheckCompatibility ();

	if (iEventService)
		{
		// A listener is needed for getting information on ip addresses
		// that are added/deleted to/from certain interfaces. This
		// information is used for filtering incoming packets from
		// malicious senders.
		delete iEventListener;
		iEventListener = NULL;
		iEventListener = new (ELeave) C6to4Listener (NetworkService(), *iEventService);
		}
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::NetworkDetached
// Unbind the hooks.
// ----------------------------------------------------------------------------
//
void CProtocol6to4::NetworkDetached ()
	{
	delete iEventListener;
	iEventListener = NULL;
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::Identify
// Provide identification information to the caller.
// ----------------------------------------------------------------------------
//
void CProtocol6to4::Identify (TServerProtocolDesc & aEntry)
	{
	aEntry.iName = K6to4;
	aEntry.iAddrFamily = KAfInet6;
	aEntry.iSockType = KSockDatagram;
	aEntry.iProtocol = KProtocolInet6Ipip;
	aEntry.iVersion = TVersion (1, 0, 0);
	aEntry.iByteOrder = EBigEndian;
	// aEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
	aEntry.iServiceInfo =
	  KSIConnectionLess | KSIMessageBased | KSIBroadcast | KSIPeekData |
	  KSIGracefulClose;
	aEntry.iNamingServices = 0;
	aEntry.iSecurity = KSocketNoSecurity;
	aEntry.iMessageSize = 0xffff;
	aEntry.iServiceTypeInfo = EPreferMBufChains | ENeedMBufs;
	aEntry.iNumSockets = KUnlimitedSockets;
	}

void CProtocol6to4::Identify (TServerProtocolDesc * aDesc) const
	{
	Identify (*aDesc);
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::ApplyL
// This is the handler for incoming packets. Detunnel legal packets.
// ----------------------------------------------------------------------------
//
TInt CProtocol6to4::ApplyL (RMBufHookPacket & aPacket, RMBufRecvInfo & aInfo)
	{
	if (aInfo.iIcmp)
		{
		// This is a returned packet inside an ICMP. Could check whether the
		// IP header is the tunneling generated by this packet and do something
		// about it, but for now, just ignore and let someone else deal with it.
		return KIp6Hook_PASS;
		}
	TInet6Packet < TIpHeader > ip (aPacket, aInfo.iOffset);
	if (ip.iHdr == NULL)
		return KIp6Hook_PASS;
	
	// Get inner destination address in "normalized" IPv6 format
	TIp6Addr inner_dst;
	if (aInfo.iProtocol == (TInt)KProtocolInet6Ipip)
		{
		// Inner header IPv6

		// Need to check the mapped length explicitly, because
		// IPv4 min size is less than IPv6 header...
		if (ip.iLength < (TInt)sizeof(TInet6HeaderIP))
			return KIp6Hook_PASS;
		inner_dst = ip.iHdr->ip6.DstAddr();
		}
	else if (aInfo.iProtocol == (TInt)KProtocolInetIpip)
		{
		// Inner header IPv4 (construct IPv4 mapped address)
		inner_dst.u.iAddr32[0] = 0;
		inner_dst.u.iAddr32[1] = 0;
		inner_dst.u.iAddr32[2] = v4Prefix.b;
		inner_dst.u.iAddr32[3] = ip.iHdr->ip4.DstAddrRef();
		}
	else
		return KIp6Hook_PASS;	// Ignore all others.

	// Let those packets through that belong to any configured tunnel 
	TTunnelQueueIter iter (iTunnelParser.TunnelQueue());
	for (;;)
		{
		CTunnel *const tunnel = iter++;
		if (tunnel == NULL)
			{
			// In case not handled by any of the configured tunnels, check if 
			// the address makes it possible to tunnel it automatically.
			if (inner_dst.u.iAddr16[0] == K6to4Prefix.u.iAddr16[0])
				{
				// Set interface index of the packet
				// Should check that inner src is one of my own 6to4 addresses?
				aInfo.iInterfaceIndex = iInterfaceIndex;
				}
			break;
			}
		if (inner_dst.IsEqual (tunnel->VirtualIfAddr()) &&
			TInetAddr::Cast(aInfo.iSrcAddr).Ip6Address().IsEqual(tunnel->EndpointAddr()))
			{
			// A tunnel willing to accept this incoming packet was found.
			// Set also virtual interface index for the packet
			aInfo.iInterfaceIndex = tunnel->InterfaceIndex();
			break;
			}
		}

	// Let the IP layer do the actual detunneling process
	// (peeling off one IP header layer). [This way tunneled
	// fragments get correctly processed, even if inner IP was
	// IPv4 -- not currently the case here, inner is always IPv6.
	// IPv6 fragments would work even if the outer IP layer
	// was peeled off here].
	return KIp6Hook_PASS;
	};

// ----------------------------------------------------------------------------
// CProtocol6to4::CheckCompatibility
// Checks if the MEventService API is implemented. If not, 6to4 is not 
// compatible with the TCP/IP stack version and log message is printed (and
// 6to4 shouldn't be used).
// ----------------------------------------------------------------------------
//
void CProtocol6to4::CheckCompatibility ()
	{
	MInterfaceManager *ifacer = NetworkService ()->Interfacer ();
	TInt err;

	TRAP (err, iEventService = IMPORT_API_L (ifacer, MEventService));
#ifdef _LOG
	if (err != KErrNone)
		LOG (Log::Printf (_L
			 ("MEventService not available. Some features may be missing\n")));
#endif
	}

// ----------------------------------------------------------------------------
// CProtocol6to4::LoadFileL
// Loads and parses a tunnel configuration file.
// ----------------------------------------------------------------------------
//
TInt CProtocol6to4::LoadFileL (const TDesC & aFile)
	{
	// Connect to the file server and open the tunnel configuration
	// file (e.g., 6to4_tunnels.ini or what configured in 6to4.ini).
	TAutoClose < RFs > fs;
	User::LeaveIfError (fs.iObj.Connect ());
	fs.PushL ();

	TAutoClose < RFile > file;
	User::LeaveIfError (file.iObj.Open (fs.iObj, aFile, EFileRead));
	file.PushL ();

	// Read file to a buffer
	TInt size;
	User::LeaveIfError (file.iObj.Size (size));
	HBufC8 *tmp_buf = HBufC8::NewL (size);

	CleanupStack::PushL (tmp_buf);

	TPtr8 tmp_ptr (tmp_buf->Des ());
	User::LeaveIfError (file.iObj.Read (tmp_ptr));
	HBufC16 *file_data = HBufC16::NewL (size);
	file_data->Des ().Copy (tmp_buf->Des ());
	TPtr16 ptr (file_data->Des ());

	CleanupStack::PushL (file_data);

	// Parse tunnels from the buffer.
	TInt ret = iTunnelParser.ParseL (ptr);

	if (ret != KErrNone)
		{
		LOG (Log::
			 Printf (_L
					 ("CProtocol6to4::LoadFileL [%S]: Error [%d] in tunnel conf file\r\n"),
					 &aFile, ret));
		}

	CleanupStack::Pop ();
	CleanupStack::Pop ();
	file.Pop ();
	fs.Pop ();

	delete file_data;
	delete tmp_buf;

	return KErrNone;
	}

// ----------------------------------------------------------------------------
// CProtocol6to4:OpenL
// Outbound Open handler for the protocol. Checks first if the packet belongs 
// to one of the tunnels. If so, creates and configures a 6to4 local flow 
// object. Sets the
// destination IP address according to the IPv4 address of the outer (IPv4)
// header. Clears the source IP address of the packet and sets the IP version
// to 4. Marks the next header to be IPv6. Returns the newly created flow.
// ----------------------------------------------------------------------------
//
MFlowHook *CProtocol6to4::OpenL (TPacketHead & aHead, CFlowContext * aFlow)
	{
	TIp6Addr & dst = aHead.ip6.DstAddr ();
	TIp6Addr & src = aHead.ip6.SrcAddr ();
	TIpAddress outerDst;

	// Check first if the flow belongs to one of the configured tunnels
	TTunnelQueueIter iter (iTunnelParser.TunnelQueue());
	for (;;)
		{
		CTunnel *const tunnel = iter++;
		if (tunnel == NULL)
			{
			// Packet didn't belong to configured tunnels, try automatic
			// tunneling
			if (dst.u.iAddr16[0] != K6to4Prefix.u.iAddr16[0])
				return NULL;    // No configured tunnel or a 6to4 address

			// Apply the 6to4 conversion for the address 
			// IPv4 format

			outerDst.u.iAddr32[0] = 0;
			outerDst.u.iAddr32[1] = 0;
			outerDst.u.iAddr32[2] = v4Prefix.b;
			outerDst.u.iAddr8[12] = dst.u.iAddr8[2];
			outerDst.u.iAddr8[13] = dst.u.iAddr8[3];
			outerDst.u.iAddr8[14] = dst.u.iAddr8[4];
			outerDst.u.iAddr8[15] = dst.u.iAddr8[5];

			outerDst.iScope = 0;
			break;
			}
		if (aHead.iInterfaceIndex == tunnel->InterfaceIndex())
			{
			// Found a configured tunnel
			// Use configured endpoint address as the outer destination ipv4 address
			outerDst = tunnel->EndpointAddr();
			break;
			}
		}
	// In current system, the (inner) source is always defined at this point
	// (cancel flow if it is not!)
	if (!aHead.iSourceSet)
		User::Leave(KErrNotReady);
	
	// Must generate a value for inner hoplimit. For this, use the socket
	// option to retrieve the currently defined value. The default is
	// different for multicast and unicast, so need to test the inner address
	// [Note: Really, the stack should be doing this for me, but ... -- msa]
	const TBool is_multicast =
		dst.IsMulticast() ||
		 (dst.u.iAddr32[0] == 0 &&
		  dst.u.iAddr32[1] == 0 &&
		  dst.u.iAddr32[2] == v4Prefix.b &&
		  (dst.u.iAddr8[12] & 0xF0) == 0xE0);
	TPckgBuf<TInt> opt;
	(void)aFlow->GetOption(KSolInetIp, is_multicast ? KSoIp6MulticastHops : KSoIp6UnicastHops, opt);
	aHead.ip6.SetHopLimit(opt());

	// We are interested in this flow, let's create a new local flow 
	// instance
	C6to4FlowInfo *info = new (ELeave) C6to4FlowInfo(aHead);


	// Next header is going to be 4 or 41
	aHead.iProtocol = (TUint8)(dst.IsV4Mapped() ? KProtocolInetIpip : KProtocolInet6Ipip);
	aHead.ip6.SetNextHeader (aHead.iProtocol);
	aHead.ip6.SetHopLimit(0);

	dst = outerDst.Address();
	aHead.iDstId = outerDst.iScope;

	// Clean the source IP address from the packet. Flow sets it 
	// automatically before the ReadyL() is called.
	aHead.iSourceSet = 0;
	src = KInet6AddrNone;
	aHead.iSrcId = 0;

	if (dst.IsV4Mapped())
		{
		aHead.ip6.SetVersion (4);
		// Update header size overhead
		aFlow->iHdrSize += 20;
		}
	else
		{
		aHead.ip6.SetVersion (6);
		// Update header size overhead
		aFlow->iHdrSize += sizeof(TInet6HeaderIP);
		}
	aHead.iIcmpType = 0;
	aHead.iIcmpCode = 0;
	aHead.iSrcPort = 0;
	aHead.iDstPort = 0;

#if TPACKETHEAD_FRAGMENT
	// Tunneled packets should be fragmented before tunneling
	aHead.iFragment = 1;
#endif
	return info;
	}

// ========================== OTHER EXPORTED FUNCTIONS ========================

//  End of File  


