// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// udp_sap.cpp - UDP service access point
// UDP service access point
//



/**
 @file udp_sap.cpp
*/

#include "udp.h"
#include "inet6log.h"
#include <in6_opt.h>
#include <nifman_internal.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <in_sock_internal.h>
#endif

CProviderUDP6::CProviderUDP6(CProtocolInet6Base* aProtocol)
	: CProviderInet6Transport(aProtocol)
	{
	__DECLARE_NAME(_S("CProviderUDP6"));
	iProtocolId = KProtocolInetUdp;
	}

CProviderUDP6::~CProviderUDP6()
	{
	iProtocol->UnbindProvider(this);
	iSockInQ.Free();
	iSockOutBuf.Free();
	}


void CProviderUDP6::InitL()
	{
	CProviderInet6Transport::InitL();
	iFlow.SetProtocol(KProtocolInetUdp);
	iFlow.SetNotify(this);
	iSockInQLen = 0;
	iSockInBufSize = Protocol()->RecvBuf();
	}

void CProviderUDP6::Ioctl(TUint aLevel, TUint aName, TDes8* aOption)
	{
	// LOG provided by the base class
	CProviderInet6Transport::Ioctl(aLevel, aName, aOption);
	}


void CProviderUDP6::CancelIoctl(TUint aLevel, TUint aName)
	{
	// LOG provided by the base class
	CProviderInet6Transport::CancelIoctl(aLevel, aName);
	}


TInt CProviderUDP6::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	{
	TInt ret = KErrNotSupported;
	TInt intValue = 0;

	switch (aLevel)
		{
	case KSolInetUdp:
		switch (aName)
			{
		case KSoUdpReceiveICMPError:
			ret = GetOptionInt(aOption, intValue);
			if (ret == KErrNone)
				iSockFlags.iReportIcmp = intValue ? TRUE : FALSE;
#ifdef _LOG
			Log::Printf(_L("SetOpt\tudp SAP[%u] KSoUdpReceiveICMPError=%d err=%d"),
				(TInt)this, (TInt)iSockFlags.iReportIcmp, ret);
#endif
			break;

		case KSoUdpSynchronousSend:
			ret = GetOptionInt(aOption, intValue);
			if (ret == KErrNone)
				iSynchSend = intValue ? TRUE : FALSE;
#ifdef _LOG
			Log::Printf(_L("SetOpt\tudp SAP[%u] KSoUdpSynchronousSend = %d err=%d"),
				(TInt)this, iSynchSend, ret);
#endif
			break;
			
		case KSoUdpRecvBuf:
			ret = GetOptionInt(aOption, intValue);
			if (ret == KErrNone)
				iSockInBufSize = intValue;
#ifdef _LOG
			Log::Printf(_L("SetOpt\tudp SAP[%u] KSoUdpRecvBuf = %d err=%d"),
				(TInt)this, iSockInBufSize, ret);
#endif
			break;

		case KSoUdpAddressSet:
			ret = GetOptionInt(aOption, intValue);
			if (ret == KErrNone)
				iSockFlags.iAddressSet = intValue ? TRUE : FALSE;
#ifdef _LOG
			Log::Printf(_L("SetOpt\tudp SAP[%u] KSoUdpAddressSet = %d err=%d"),
				(TInt)this, (TInt)iSockFlags.iAddressSet, ret);
#endif
			break;

		default:
			break;
			}
		break;


	case KSolInetIp: 
		{ 
        if (aName == KSoReuseAddr) 
        	{ 
            ret = CheckPolicy(KPolicyNetworkControl, 0); 
            if (ret == KErrNone) 
      	      { 
              TInt intValue; 
              ret = GetOptionInt(aOption, intValue); 
              if (ret == KErrNone) 
      	        iSockFlags.iReuse = intValue ? TRUE : FALSE; 
              } 
#ifdef _LOG 
            Log::Printf(_L("SetOpt\tudp SAP[%u] KSoReuseAddr = %d err=%d"), 
      		               (TInt)this, (TInt)iSockFlags.iReuse, ret); 
#endif 
            return ret; // NOTE! We must not let CProviderInet6Transport handle this (security issue) 
            } 
        } 
        break; 
        
	default:
		break;
		}

	if (ret == KErrNotSupported)
		ret = CProviderInet6Transport::SetOption(aLevel, aName, aOption);

	return ret;
	}


TInt CProviderUDP6::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	{
	TInt ret = KErrNotSupported;

	switch (aLevel)
		{
	case KSOLSocket:
		if (aName == KSOReadBytesPending)
			ret = SetOptionInt(aOption, iSockInQ.IsEmpty() ? 0 : iSockInQ.First().Length() - iSockInQ.First().First()->Length());
		break;
	case KSolInetUdp:
		switch (aName)
			{
		case KSoUdpReceiveICMPError:
			ret = SetOptionInt(aOption, iSockFlags.iReportIcmp);
			break;

		case KSoUdpSynchronousSend:
			ret = SetOptionInt(aOption, iSynchSend);
			break;

		case KSoUdpRecvBuf:
			ret = SetOptionInt(aOption, iSockInBufSize);
			break;

		default:
			break;
			}
		break;

	default:
		break;
		}

	if (ret == KErrNotSupported)
		ret = CProviderInet6Transport::GetOption(aLevel, aName, aOption);

	return ret;
	}

TInt CProviderUDP6::SetRemName(TSockAddr &aAddr)
	{
	TInt err;
	TInetAddr addr = aAddr;

	if (err = CProviderInet6Transport::SetRemName(addr), err == KErrNone)
		{
		if (iFlow.FlowContext()->LocalPort() == KInetPortNone)
			CProviderInet6Transport::AutoBind();
		if (iFlow.FlowContext()->LocalPort() != KInetPortNone)
			err = iFlow.Connect();
		else
			{
			iSockFlags.iConnected = EFalse;
			iSockFlags.iReportIcmp = ETrue;
			err = KErrInUse;
			}
		}

	return (err < KErrNone) ? err : KErrNone;
	}


void CProviderUDP6::Shutdown(TCloseType aOption)
	{
	LOG(Log::Printf(_L("Shutdown\tudp SAP[%u] TCloseType=%d"), (TInt)this, aOption));
	switch(aOption)
		{
	case EStopInput:
		iSockFlags.iRecvClose = ETrue;
		iSockInQ.Free();
		iSockInQLen = 0;
		iSocket->Error(KErrNone,MSocketNotify::EErrorClose);
		Nif::SetSocketState(ENifSocketConnected, this);
		break;

	case EStopOutput:
		iSockFlags.iSendClose = ETrue;
		iSocket->Error(KErrNone,MSocketNotify::EErrorClose);
		Nif::SetSocketState(ENifSocketConnected, this);
		break;

	case ENormal:
		iSocket->CanClose();
		break;

	case EImmediate:
		break;

	default:
		Panic(EInet6Panic_NotSupported);
		break;
		}
	}


//
// PRTv1.0 API
//
void CProviderUDP6::GetData(TDes8 &aDesc, TUint aOptions, TSockAddr *aAddr)
	{
	TDualBufPtr buf(aDesc);
	Recv(buf, aDesc.Length(), aOptions, aAddr);
	}

//
// PRTv1.5 API
//
TInt CProviderUDP6::GetData(RMBufChain& aData, TUint aLength, TUint aOptions, TSockAddr* aAddr)
	{
	TDualBufPtr buf(aData);
	return Recv(buf, aLength, aOptions, aAddr);
	}

static void FillUdpHeader(TInet6Checksum<TInet6HeaderUDP> &aPkt, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint aOffset)
	{
	//
	// Build UDP header
	aPkt.iHdr->SetSrcPort(aInfo.iSrcAddr.Port());
	aPkt.iHdr->SetDstPort(aInfo.iDstAddr.Port());
	aPkt.iHdr->SetLength(aInfo.iLength);
	// Compute checksum
	aPkt.ComputeChecksum(aPacket, &aInfo, aOffset);
	if (aPkt.iHdr->Checksum() == 0)  // Zero indicates "no checksum" in IPv4
		aPkt.iHdr->SetChecksum(0xffff);
	}

TInt CProviderUDP6::DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint /*aOptions*/, TUint aOffset)
	{
	// Whenever a new packet to send arrives, the buffered packet, if any, is dropped!
	// [Because the new packet may make the flow selectors different from the buffered packet]
	LOG(if (!iSockOutBuf.IsEmpty()) Log::Printf(_L("\tudp SAP[%u] No space. Unsent packet DROPPED"), (TInt)this));
	iSockOutBuf.Free();

	if (aOffset == 0)
		{
		// When offset is 0, the packet is not using the KIpHeaderIncluded.
		// The UDP header needs to be allocated in front of the payload.
		TInt err = aPacket.Prepend(TInet6HeaderUDP::MinHeaderLength());
		if (err != KErrNone)
			return KErrNoMBufs;
		aInfo.iLength += TInet6HeaderUDP::MinHeaderLength();
		}
	TInet6Checksum<TInet6HeaderUDP> pkt(aPacket, aOffset);
	if (pkt.iHdr == NULL)
		{
		// Can happen if user uses KIpHeaderIncluded without UDP header, but
		// may catch other unexpected problems with the RMBuf handling. In
		// any case, this packet cannot be sent (no retries allowed).
		return KErrInet6ShortPacket;
		}
	if (aOffset > 0)
		{
		// The packet is sent using KIpHeaderIncluded option, extract the UDP
		// ports from the included header.
		aInfo.iSrcAddr.SetPort(pkt.iHdr->SrcPort());
		if (aInfo.iDstAddr.Port() == 0)
			{
			// aToAddr was not specified, or it had ZERO port. copy the port
			// from the UDP header.
			aInfo.iDstAddr.SetPort(pkt.iHdr->DstPort());
			}
		// Change the flow to match the included header! The header gives
		// both source and destination, and will OVERRIDE any previous
		// Bind() or Connect() for the socket.
		//
		// [Alternate: if socket is explicitly bound or connected,
		// do not accept HeaderIncluded, and return error?]
		// Should the port ranges be checked here or not?
		//
		// The source address may still be unknown and will become
		// known only after flow Open succeeds (watch out for
		// iSockOutBuf buffering case--address becomes known in
		// CanSend!)
		iFlow.SetLocalAddr(aInfo.iSrcAddr);
		iFlow.SetRemoteAddr(aInfo.iDstAddr);
		}
	else
		{
		// Packet is not using KIpHeaderIncluded.
		if (aInfo.iDstAddr.Family())
			{
			// [Alternate: if socket is connected, do not accept explicit
			// destination setting, and return error?]
			// If aToAddr specified, then iDstAddr already includes a copy of it.
			// Check port range
			if (aInfo.iDstAddr.Port() < 1 || aInfo.iDstAddr.Port() > 65535)
				{
				return KErrGeneral;
				}
			iFlow.SetRemoteAddr(aInfo.iDstAddr);
			}
		}

	const TInt status = aInfo.iFlow.Open(iFlow, &aInfo);
	if (status == EFlow_READY)
		{
		// Flow is ready for send
		//
		// Build UDP header
		FillUdpHeader(pkt, aPacket, aInfo, aOffset);
		return KErrNone;
		}
	else if (status < 0)
		return status;

	// status > 0


	// Block if the flow is blocked and application has requested synchronous send
	// or if we're waiting for an interface to come up
	//
	if (iSynchSend || (status == EFlow_PENDING && Protocol()->WaitNif()))
		{
		if (aOffset == 0)
			{
			// Cancel the UDP header allocation.
			aPacket.TrimStart(TInet6HeaderUDP::MinHeaderLength());
			}
		LOG(Log::Printf(_L("\tudp SAP[%u] Flow not ready (%d), BLOCKING the socket"), (TInt)this, status));
		// The socket write will be blocked!
		return status;
		}

	// Flow is not ready for sending, buffer the packet for a case where the flow
	// becomes ready before the application sends another packet.
	LOG(Log::Printf(_L("\tudp SAP[%u] Flow not ready (%d), BUFFER one packet"), (TInt) this, status));
	aPacket.Pack();
	iSockOutBuf.Assign(aPacket);
	iSockOutOffset = aOffset;
	return KErrNone;
	}

TInt CProviderUDP6::Recv(TDualBufPtr& aBuf, TUint aLength, TUint aOptions, TSockAddr* aAddr)
	{
//	LOG(Log::Printf(_L("CProviderUDP6::Recv(%d, %d)\r\n"), aLength, aOptions));
	ASSERT(iSockFlags.iRecvClose == EFalse);

	RMBufRecvPacket packet;
	RMBufRecvInfo *info;
	TInt off;


	if (!iSockInQ.Remove(packet))
		Panic(EInet6Panic_NoData);

	info = packet.Unpack();
	off = (/*iSockFlags.*/iRawMode || (aOptions & KIpHeaderIncluded)) ? 0 : info->iOffset;
	aLength = Min(STATIC_CAST(TInt, aLength), info->iLength - off);

	ASSERT(info->iLength == packet.Length());

	// Get remote address
	LOG(TBuf<70> tmp(_L("NULL")));
	if (aAddr != NULL)
		{
		*aAddr = info->iSrcAddr;
		LOG(TInetAddr::Cast(*aAddr).OutputWithScope(tmp));
		if (iAppFamily == KAfInet)
			TInetAddr::Cast(*aAddr).ConvertToV4();
		}

	if (aOptions & KSockReadPeek)
		{
		TInt err = aBuf.CopyIn(packet, off, aLength);
		packet.Pack();
		iSockInQ.Prepend(packet);
		LOG(Log::Printf(_L("GetData\tudp SAP[%u] Peek len=%d err=%d from=%S, calling NewData(1)"), (TInt)this, aLength, err, &tmp));
		iSocket->NewData(1);
		if (err != KErrNone)
			return KErrNoMBufs;
		}
	else
		{
		iSockInQLen -= info->iLength;
		if (off > 0)
			packet.TrimStart(off);
		aBuf.Consume(packet, aLength, iBufAllocator);
		packet.Free();
		LOG(Log::Printf(_L("GetData\tudp SAP[%u] length=%d from=%S"), (TInt)this, aLength, &tmp));
		}

	return 1;
	}


void CProviderUDP6::Process(RMBufChain& aPacket, CProtocolBase* /*aSourceProtocol*/)
	{
	TUint length = 0;
	RMBufRecvInfo *const info = RMBufRecvPacket::PeekInfoInChain(aPacket);
	if (info)
		{
		length = info->iLength;

		//
		// ESock does not properly buffer multiple datagrams in the receive buffer,
		// so we need to do it here. The worst part is that we don't know the size
		// of the receive buffer requested by the application, because ESock does not
		// pass the socket option to us. We use a TCPIP.INI parameter/Udp socket option instead.
		//
		if (iSockFlags.iRecvClose || !iSockFlags.iNotify ||
			(!iSockInQ.IsEmpty() && iSockInQLen + length > iSockInBufSize))
			{
			LOG(Log::Printf(_L("\tudp SAP[%u] No space. Incoming packet DROPPED"), (TInt)this));
			}
		else
			{
			iSockInQLen += length;
			iSockInQ.Append(aPacket);
			LOG(Log::Printf(_L("\tudp SAP[%u] Packet queued, QLen=%d, calling NewData(1)"), (TInt)this, iSockInQLen));
			iSocket->NewData(1);
			return;
			}
		}
	aPacket.Free();
	}

void CProviderUDP6::Error(TInt aError, TUint aOperationMask)
	{
	// Report errors by interrupting send and receive operations
	aOperationMask &= MSocketNotify::EErrorSend|MSocketNotify::EErrorRecv;
	CProviderInet6Transport::Error(aError, aOperationMask);

	//
	// Force the flow to reconnect on the next send, clearing the error state.
	//
	iFlow.FlowContext()->SetChanged();
	}

void CProviderUDP6::CanSend()
	{

	//
	// Check flow status.
	//
	// Note: calling iFlow.Status() here also makes sure that a connect()
	//       operation on the UDP socket will bring the flow up completely
	//       without requiring the application to follow it up with a
	//       send(). This is required by, e,g. the QoS framework.
	//
	TInt status;
	if (iSockOutBuf.IsEmpty())
		{
		// No buffered packet.
		status = iFlow.Status();
		if (status == EFlow_READY)
			{
			LOG(Log::Printf(_L("\tudp SAP[%u] CanSend() Nothing buffered, flow ready, wakeup"), (TInt)this));
			// Wake application
			CProviderInet6Transport::CanSend();
			return;
			}
		LOG(Log::Printf(_L("\tudp SAP[%u] CanSend() Nothing buffered, flow status %d"), (TInt)this, status));
		}
	else
		{
		// A packet is waiting for this!
		RMBufSendInfo *info = iSockOutBuf.PeekInfo();
		status = info->iFlow.Open(iFlow, info);
		if (status == EFlow_READY)
			{
			RMBufSendPacket packet;
			packet.Assign(iSockOutBuf);
			packet.Unpack();
			TInet6Checksum<TInet6HeaderUDP> pkt(packet, iSockOutOffset);
			if (pkt.iHdr == NULL)
				{
				// Can happen if user uses KIpHeaderIncluded without UDP header, but
				// may catch other unexpected problems with the RMBuf handling. In
				// any case, this packet cannot be sent (no retries allowed).
				LOG(Log::Printf(_L("CProviderUDP6::CanSend() Bad buffered UDP packet dropped")));
				info->iFlow.Close();
				packet.Free();
				}
			else
				{
				LOG(Log::Printf(_L("\tudp SAP[%u] CanSend() Sending buffered UDP packet"), (TInt)this));
				FillUdpHeader(pkt, packet, *info, iSockOutOffset);
				packet.Pack();
				iProtocol->Send(packet);
				}
			// Note: When packet is buffered, then the SocketServer is not blocked
			// on this socket. Thus, there is no need to call iSocket->Notify()!
			return;
			}
		LOG(Log::Printf(_L("\tudp SAP[%u] CanSend() Buffered packet waiting, flow status %d"), (TInt)this, status));
		}

	if (status < 0)
		{
		Error(status, MSocketNotify::EErrorSend|MSocketNotify::EErrorRecv);
		}
	}
