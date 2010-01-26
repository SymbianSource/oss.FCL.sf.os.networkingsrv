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
// in_net.cpp - network layer protocol
//

#include "in6_if.h"
#include "inet.h"
#include "in_net.h"
#include "tcpip_ini.h"
#include "inet6log.h"

void CProtocolInet6Network::StartL()
	{
	CProtocolInet6Base::StartL();
	//
	// Get the default value for the provider receive queue limiter
	// (use the protocol name as section name)
	TServerProtocolDesc info;
	Identify(&info);
	TInt value;
	if (iNetwork->Interfacer()->FindVar(info.iName, TCPIP_INI_RECV_BUF, value))
		iQueueLimit = value;
	else
		iQueueLimit = KNetDefaultRecvBuf;
	}


CProtocolInet6Network::~CProtocolInet6Network()
	{
	}

//
//	CProtocolInet6Network::Deliver
//	******************************
//	Generate a copy of the packet to every bound provider
//
void CProtocolInet6Network::Deliver(/*const*/ RMBufPacketBase &aPacket)
	// .. cannot use "const", because CopyInfoL is not const! -- msa */
	{
	CProviderInet6Network* provider;
	TInet6SAPIter iter(this);

	const RMBufPktInfo *const info = aPacket.Info();
	while (provider = (CProviderInet6Network *)iter++, provider != NULL)
		{
		if (provider->IsReceiving(*info))
			{
			RMBufRecvPacket copy;
			TRAPD(err, aPacket.CopyL(copy);aPacket.CopyInfoL(copy));
			if (err == KErrNone)
				{
				copy.Pack();
				provider->Process(copy);
				}
			else
				copy.Free();
			}
		}
	}

//
// Shared provider methods (defaults)
//
CProviderInet6Network::~CProviderInet6Network()
	{
	iProtocol->UnbindProvider(this);
	iRecvQ.Free();
	}


void CProviderInet6Network::InitL()
	{
	//
	// Set initial value for the iQueueLimit
	//
	iQueueLimit = ((CProtocolInet6Network*)iProtocol)->QueueLimit();
	CProviderInet6Base::InitL();
	iProtocol->UnbindProvider(this);
	iProtocol->BindProvider(this);
	}

TInt CProviderInet6Network::SetLocalName(TSockAddr &aAddr)
	{
	iProtocol->UnbindProvider(this);
	// Note: a non-zero port in aAddr is significant!
	iFlow.SetLocalAddr(aAddr);
	iProtocol->BindProvider(this);
	return 0;
	}

void CProviderInet6Network::AutoBind()
	{
	// TInetAddr() is assumed to be KAFUnspec & port=0
	iFlow.SetLocalAddr(TInetAddr());
	}


void CProviderInet6Network::Shutdown(TCloseType aOption)
	{
	
	switch(aOption)
		{
		case EStopInput:
			iInputStopped = 1;
			iProtocol->UnbindProvider(this);	// We don't need any incoming data
			iRecvQ.Free();
			iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
//			Nif::SetSocketState(ENifSocketConnected, this); // Reset CSocket state to ESStateConnected
			break;

		case EStopOutput:
			iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
//			Nif::SetSocketState(ENifSocketConnected, this); // Reset CSocket state to ESStateConnected
			break;

		default:
	        iProtocol->UnbindProvider(this);
	        if (aOption!=EImmediate)
				iSocket->CanClose();
		}
	}

//
// CProviderInet6Network::Process()
// ********************************
//	Process incoming data from the protocol object.
//
void CProviderInet6Network::Process(RMBufChain& aPacket, CProtocolBase * /*aSourceProtocol*/)
	{
	// iInputStopped is set at ShutDown() and no packets should be coming
	// from the protocol after that. However, without knowing the exact
	// details of the process model/threads, it could be possible that
	// a Process() call has been initiated by the protocol and interrupted
	// before the shutdown, thus there may be a need for this iInputStopped
	// flag, although I would prefer to do without... NEED TO VERIFY IF
	// iInputStopped is really needed!!! -- msa
	//
	if(!(iInputStopped ||
		(iErrorMask & (MSocketNotify::EErrorFatal|MSocketNotify::EErrorConnect|MSocketNotify::EErrorRecv))))
		{
		iQueueLimit -= RMBufPacketBase::PeekInfoInChain(aPacket)->iLength;
		iRecvQ.Append(aPacket);
		LOG(Log::Printf(_L("\t%S SAP[%u] Packet Queued, calling NewData(1), limit=%d"), &ProtocolName(), (TInt)this, iQueueLimit));
		iSocket->NewData(1);
		}
	else
		{
		LOG(Log::Printf(_L("\t%S SAP[%u] Packet Dropped"), &ProtocolName(), (TInt)this));
		aPacket.Free();
		}
	}

// CProviderInet6Network::IsReceiving
// **********************************
//
TBool CProviderInet6Network::IsReceiving(const RMBufPktInfo &aInfo)
	{
	if (!HasNetworkServices() && (aInfo.iFlags & KIpLoopbackPacket) == 0)
		return FALSE;	// No network services and packet is external.
	const TInt port = iFlow.FlowContext()->LocalPort();

	if (iQueueLimit < 0)
		{
		// Receive Queue limit is full, cannot receive this packet
		
		LOG(Log::Printf(_L("\t%S SAP[%u] Not receiving packets, limit=%d"), &ProtocolName(), (TInt)this, iQueueLimit));
		iPacketsDropped++;
		return FALSE;
		}
	return port == 0 || port == aInfo.iProtocol;
	}


// CProviderInet6Network::GetData
// ******************************

TInt CProviderInet6Network::GetData(RMBufChain& aData, TUint aLength, TUint aOptions, TSockAddr* aAddr)
	{
	RMBufRecvPacket packet;
	const RMBufRecvInfo *info;
	if (!iRecvQ.Remove(packet))
		{
		LOG(Log::Printf(_L("GetData\t%S SAP[%u] No Data Available (0)"), &ProtocolName(), (TInt)this));
		return 0;	// No Data Available
		}

	info = packet.Unpack();

	if (aAddr != NULL)
		*aAddr = info->iSrcAddr;

#ifdef _LOG
	TBuf<70> tmp(_L("NULL"));
	if (aAddr)
		TInetAddr::Cast(*aAddr).OutputWithScope(tmp);
#endif
	const TInt offset = ((aOptions & KIpHeaderIncluded) != 0 || iRawMode) ? 0 : info->iOffset;

	if (aOptions & KSockReadPeek)
		{
		//
		// Oops, peek needs to keep the original packet intact in the queue
		//
		TInt len = info->iLength - offset;
		if (len > (TInt)aLength)
			len = aLength;
		TInt err = packet.Copy(aData, offset, len);
		packet.Pack();
		iRecvQ.Prepend(packet);
		LOG(Log::Printf(_L("GetData\t%S SAP[%u] Peek len=%d err=%d from=%S, calling NewData(1)"), &ProtocolName(), (TInt)this, len, err, &tmp));
		iSocket->NewData(1);
		// Any failure to create the RMBufChain is signalled as KErrNoMBufs!
		return err != KErrNone ? KErrNoMBufs : len;
		}
	iQueueLimit += info->iLength; // Allow more packets in..
	packet.TrimStart(offset);
	aData.Assign(packet);
	if ((TUint)info->iLength > aLength)
		aData.TrimEnd(aLength);
	packet.Free();	// (Releases info block)
	LOG(Log::Printf(_L("GetData\t%S SAP[%u] length=%d from=%S"), &ProtocolName(), (TInt)this, aLength, &tmp));
	return aLength;
	}

void CProviderInet6Network::GetData(TDes8 &aDesc, TUint aOptions, TSockAddr *anAddr)
	{
	RMBufRecvPacket packet;
	const RMBufRecvInfo *info;
	if (!iRecvQ.Remove(packet))
		Panic(EInet6Panic_NoData);

	info = packet.Unpack();

	const TInt offset = ((aOptions & KIpHeaderIncluded) != 0 || iRawMode) ? 0 : info->iOffset;
	packet.CopyOut(aDesc, offset);

	if (anAddr!=NULL)
		*anAddr = info->iSrcAddr;

#ifdef _LOG
	TBuf<70> tmp(_L("NULL"));
	if (anAddr)
		TInetAddr::Cast(*anAddr).OutputWithScope(tmp);
#endif

	if (aOptions & KSockReadPeek)
		{
		packet.Pack();
		iRecvQ.Prepend(packet);
		LOG(Log::Printf(_L("GetData\t%S SAP[%u] peek aDesc = %d from %S, calling NewData(1)"), &ProtocolName(), (TInt)this, aDesc.Length(), &tmp));
		iSocket->NewData(1);
		}
	else
		{
		iQueueLimit += info->iLength; // Allow more packets in..
		packet.Free();
		LOG(Log::Printf(_L("GetData\t%S SAP[%u] aDesc = %d from=%S"), &ProtocolName(), (TInt)this, aDesc.Length(), &tmp));
		}
	}
