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
// in_net.h - network layer protocol
//



/**
 @internalComponent
*/
#ifndef __IN_NET_H__
#define __IN_NET_H__

#include "inet.h"

const TUint KNetDefaultRecvBuf =  8192;  // Default receive buffer limit for SAP's (ICMP & IP)

//
// This is not a good class. It is misnamed. -- msa
// It is used by IP and ICMP, mainly for
// its Async buffer queues, i really would need something different..
//
class CProtocolInet6Network : public CProtocolInet6Base
	{
	//
	// Generic
	//
public:
	virtual ~CProtocolInet6Network();
	virtual void StartL();

	inline TInt QueueLimit() { return iQueueLimit; }
protected:
	//
	// The initial buffer limit for SAP's receive queues
	// created under this protocol (see. iQueueLimit on
	// CProviderInet6Network for interpretation).
	TInt iQueueLimit;

	virtual void Deliver(/*const*/ RMBufPacketBase &aPacket);
	};


class CProviderInet6Network : public CProviderInet6Base
	{
public:
	CProviderInet6Network(CProtocolInet6Base *aProtocol) : CProviderInet6Base(aProtocol) {}
	virtual ~CProviderInet6Network();
	virtual void InitL();
	virtual void Shutdown(TCloseType option);
	virtual void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol = NULL);
	virtual void GetData(TDes8 &aDesc, TUint options, TSockAddr *anAddr=NULL);
	virtual TInt GetData(RMBufChain& aData, TUint aLength, TUint aOptions, TSockAddr* anAddr=NULL);
	virtual TInt SetLocalName(TSockAddr &anAddr);
	virtual void AutoBind();

	// IsReceiving is intended for the protocol side. It is can be used by
	// the protocol side "Deliver" method to ask if this SAP is willing to
	// receive packets associated with the specified id. What the "id" is,
	// depends on the protocol. For, example, for IP level SAP/PRT pair, the
	// id is the protocol of the received packet (e.g. protocol or next header
	// field from the IP header).
	virtual TBool IsReceiving(const RMBufPktInfo &aInfo);
protected:
	RMBufPktQ iRecvQ;
	//
	// iQueueLimit is used to control how much buffered data is allowed
	// to be in the iRecvQ, before "congestion" control hits. The value counts
	// bytes in iRecvQ in following way
	// - if iQueueLimit < 0, then incoming packet is dropped (= "congestion")
	// - if iQueueLimit >= 0, then incoming packet is added into iRecvQ, and
	//   the length of the packet is subtracted from the iQueueLimit. When
	//   GetData removes the packet from the queue, the length is added back
	//   to iQueueLimit.
	// Thus, if left as initial value (= 0), only one packet at time can be
	// queued. If initialized to 8000, then at most 8000 bytes and 1 packet
	// can be queued at any point.
	TInt iQueueLimit;
	TInt iPacketsDropped;	// Count packets dropped due "congestion"
	TUint iInputStopped:1;
	};

#endif
