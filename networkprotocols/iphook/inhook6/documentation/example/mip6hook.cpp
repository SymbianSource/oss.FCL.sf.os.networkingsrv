// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is for doxygen documentation only, used as
// @dontinclude miphook6.cpp
// @skip pattern
// @until //-
// Note: Despite of name, this has nothing to do with Mobile IPv6 (MIP6).
//

#include <posthook.h>
#include <in_chk.h>
#include <icmp6_hdr.h>


class TExtensionHeader
	// Example of (very strange) extension header layout
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |  Don't care   | Next Header   |             Port              |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 	{
public:
	inline TInt HeaderLength() const { return 4; }
	inline static TInt MinHeaderLength() {return 4; }	
	inline static TInt MaxHeaderLength() {return 4; }

	inline TInt NextHeader() const { return i[1]; }
	inline TInt Port() { return (i[2] << 8) + i[3]; }
	inline void SetNextHeader(TUint8 aProtocol) { i[1] = aProtocol; }
	inline void SetPort(TInt aPort) { i[3] = (TUint8)aPort; i[2] = (TUint8)(aPort >> 8); } 
	TUint8 i[4];
	};

const TUint KDefaultPort = 0xf00d;
const TUint KDefaultProtocol = 200;

const TUint KSolHookExample = 0x101F6CFF;	// Use UID value as option level.
const TUint KSoHookExample_PORT = 1;		// Get/Set
const TUint KSoHookExample_PROTOCOL = 2;	// Get/Set

//-


class CHookExample : public CProtocolPosthook, public MFlowHook
	{
public:
	CHookExample();
	// Minimal glue for building the real PRT from this
	void Identify(TServerProtocolDesc *aDesc) const;
	void NetworkAttachedL();

	// MIp6Hook
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);
	MFlowHook *OpenL(TPacketHead &aHead, CFlowContext *aFlow);
	TInt SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &aFlow);
	TInt GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, CFlowContext &aFlow);

	// MFlowHook (for outbound processing)
	void Open();
	TInt ReadyL(TPacketHead &aHead);
	TInt ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	void Close();

	TUint16 iPort;			// The port number
	TUint8 iProtocol;		// The protocol number
	};
//-

CHookExample::CHookExample() : iPort(KDefaultPort), iProtocol(KDefaultProtocol)
	{
	}


TInt CHookExample::ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	{
	if (aInfo.iProtocol != iProtocol)
		// Normally, it should never get here in this example. But,
		// if the prototocol was just changed via SetOption...
		return KIp6Hook_PASS;

	TInet6Packet<TExtensionHeader> ext(aPacket, aInfo.iOffset);
	if (ext.iHdr == NULL)
		return KIp6Hook_PASS;	// Bad packet? But let someone else worry about it.
	if (ext.iHdr->Port() != iPort)
		return KIp6Hook_PASS;	// Not for me, let someone else handle it.

	const TInt hdrlen = aInfo.CheckL(ext.iHdr->HeaderLength());
	if (aInfo.iIcmp)
		{
		// Header is in returned packet of an ICMP Error
		const TInt offset = aInfo.iOffset - aInfo.iOffsetIp;// Relative offset within problem packet
		if (aInfo.iIcmp == KProtocolInet6Icmp && // only ICMP6 can be tested like this
			aInfo.iType == KInet6ICMP_ParameterProblem &&// A parameter problem...
			offset <= (TInt)aInfo.iParameter &&	// after start of this header?
			offset + hdrlen > (TInt)aInfo.iParameter)// and before end of this header?
			{
			// Someone didn't accept my strange header.
			// Could set some flag and stop sending it.
			aPacket.Free();
			return -1;
			}
		// Error is not about my header, fall to DONE
		}
	else
		{
		// Implement header semantics
		// (none in this example).
		}
	// Header processed (Done), skip over
	// - need to indicate the location of "my next header" field,
	aInfo.iPrevNextHdr = (TUint16)(aInfo.iOffset+1);
	// - need to make the protocol of the following header available
	aInfo.iProtocol = ext.iHdr->NextHeader();
	// - and finally, update the offset over my header.
	aInfo.iOffset += hdrlen;
	return KIp6Hook_DONE;
	}
//-


void CHookExample::Open()
	{
	// "Reroutes" MFlowHook::Open to CProtocolBase::Open
	CProtocolPosthook::Open();
	}
//-

void CHookExample::Close()
	{
	// "Reroutes" MFlowHook::Close to CProtocolBase::Close
	CProtocolPosthook::Close();
	}
//-

MFlowHook *CHookExample::OpenL(TPacketHead &, CFlowContext *aFlow)
	{
	// Attaching to every flow and using 'this' as MFlowHook for all
	Open(); // increments reference count.
	aFlow->iHdrSize += sizeof(TExtensionHeader); // Fixed size.
	return this;
	}
//-

TInt CHookExample::ReadyL(TPacketHead &)
	{
	return KErrNone; // Nothing here, always ready
	}
//-

TInt CHookExample::ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	{
	// Splice in the special header after the IP header.
	TInet6Checksum<TInet6HeaderIP4> ip(aPacket); // ..ipv4 is long enough for IPv6 too.
	TInt offset = 0;
	TUint8 protocol;
	if (ip.iHdr == NULL)
		return KErrNone; // Bad packet, not our worry...
	else if (ip.iHdr->Version() == 4)
		{
		offset = ip.iHdr->HeaderLength();
		protocol = (TUint8)ip.iHdr->Protocol();
		}
	else if (ip.iHdr->Version() == 6)
		{
		offset = ((TInet6HeaderIP *)ip.iHdr)->HeaderLength();
		protocol = (TUint8)((TInet6HeaderIP *)ip.iHdr)->NextHeader();
		}
	else
		return KErrNone; // Something strange, skip.

	// Allocate space in RMBufChain for the new heaer
	RMBufChain tail;
	aPacket.SplitL(offset, tail);
	TRAPD(err, aPacket.AppendL(sizeof(TExtensionHeader)));
	if (!tail.IsEmpty()) // RMBufChain is panicky on empty chains!
		aPacket.Append(tail);

	if (err == KErrNone)
		{
		aInfo.iLength += sizeof(TExtensionHeader);
		TInet6Packet<TExtensionHeader> ext(aPacket, offset);
		ext.iHdr->SetNextHeader(protocol);
		ext.iHdr->SetPort(iPort);
		// IP header needs "fixing"...
		if (ip.iHdr->Version() == 4)
			{
			ip.iHdr->SetProtocol(iProtocol);
			ip.iHdr->SetTotalLength(aInfo.iLength);
			ip.ComputeChecksum();
			}
		else
			{
			((TInet6HeaderIP *)ip.iHdr)->SetNextHeader(iProtocol);
			((TInet6HeaderIP *)ip.iHdr)->SetPayloadLength(aInfo.iLength - sizeof(TInet6HeaderIP));
			}
		}
	else
		aPacket.Free();
	return err;
	}
//-

void CHookExample::NetworkAttachedL()
	{
	// to get inbound packets (-> ApplyL)
	NetworkService()->BindL(this, BindHookFor(iProtocol));
	// to get a peek of outbound flows (-> OpenL).
	NetworkService()->BindL(this, BindFlowHook());
	}
//-


TInt CHookExample::SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &)
	{
	// Only deal with options specific to this hook
	if (aLevel == KSolHookExample)
		{
		// For now, this uses only TInt option parameter
		if (aOption.Length() < (TInt)sizeof(TInt))
			return KErrArgument;
		const TUint val = *((TUint *)aOption.Ptr());

		TInt err = KErrNone;
		switch (aName)
			{
		case KSoHookExample_PORT:
			// The valid range for port is 0..0xFFFF
			if (val & ~0xFFFF)
				return KErrArgument;
			iPort = (TUint16)val;
			return KErrNone;
		case KSoHookExample_PROTOCOL:
			// The valid range for protocol is 0..0xFF
			if (val & ~0xFF)
				return KErrArgument;
			if (NetworkService() && iProtocol != val)
				{
				// Undo previously installed hook for a different protocol.
				NetworkService()->Protocol()->Unbind(this, BindHookFor(iProtocol));
				// Install hook for the new protocol
				iProtocol = (TUint8)val;
				TRAP(err, NetworkService()->BindL(this, BindHookFor(iProtocol)));
				}
			else
				iProtocol = (TUint8)val;
			return err;
		default:
			break;
			}
		}
	return KErrNotSupported;
	}
//-

TInt CHookExample::GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, CFlowContext &)
	{
	// Only deal with options specific to this hook
	if (aLevel == KSolHookExample)
		{
		if (aOption.MaxLength() < (TInt)sizeof(TInt))
			return KErrArgument;

		TUint val = 0;
		switch (aName)
			{
		case KSoHookExample_PORT:
			val = iPort;
			break;
		case KSoHookExample_PROTOCOL:
			val = iProtocol;
			break;
		default:
			return KErrNotSupported;
			}
		aOption = TPtrC8((TUint8 *)&val, sizeof(val));
		return KErrNone;
		}
	return KErrNotSupported;
	}
//-

#ifdef REALLY_BUILD_EXAMPLE
// This section provides the minimal glue to the example harness to
// really build a working protocol module using the above definition.

#include "protocol_module.h"


TInt ProtocolModule::NumProtocols()
	{
	return 1;	// One protocol implemented.
	}
//-

void ProtocolModule::Describe(TServerProtocolDesc &aDesc, const TInt aIndex)
	{
	(void)aIndex;	// == 0 anyway.

	_LIT(KMyName, "docex1");

	aDesc.iName = KMyName;
	aDesc.iAddrFamily =	KAfInet;
	aDesc.iSockType = 0;
	aDesc.iProtocol = 123456;
	aDesc.iVersion = TVersion(1, 1, 1);
	aDesc.iByteOrder = EBigEndian;
	aDesc.iServiceInfo = 0;
	aDesc.iNamingServices = 0;
	aDesc.iSecurity = 0;
	aDesc.iMessageSize = 0;
	aDesc.iServiceTypeInfo = 0;
	aDesc.iNumSockets = 0;
//-
	}

CProtocolBase *ProtocolModule::NewProtocolL(TUint aSockType, TUint aProtocol)
	{
	(void)aSockType;
	(void)aProtocol;
	return new (ELeave) CHookExample;
	}
//-

void CHookExample::Identify(TServerProtocolDesc *aDesc) const
	{
	ProtocolModule::Describe(*aDesc);
	}
//-
#endif
