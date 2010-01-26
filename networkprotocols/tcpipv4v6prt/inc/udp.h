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
// udp.h - UDP for IPv6/IPv4
// UDP Protocol and Provider declarations.
//



/**
 @file udp.h
 @internalComponent
*/

#ifndef __UDP_H__
#define __UDP_H__

#include "in_trans.h"
#include <udp_hdr.h>
#include <in_chk.h>


const TUint KUdpDefaultRecvBuf =  32768;  //< Default receive window size.


typedef TInet6Checksum<TInet6HeaderUDP> TInet6PacketUDP;


/**
UDP protocol implementation.

@internalComponent
*/
class CProtocolUDP6 : public CProtocolInet6Transport
	{
public:
	CProtocolUDP6();
	CProtocolUDP6& operator=(const CProtocolUDP6&);
	virtual ~CProtocolUDP6();
	virtual CServProviderBase *NewSAPL(TUint aProtocol);
	virtual void InitL(TDesC& aTag);
	virtual void StartL();
	virtual void Identify(TServerProtocolDesc *) const;
	//virtual TInt GetOption(TUint level,TUint name,TDes8 &option,CProtocolBase* aSourceProtocol=NULL);
	//virtual TInt SetOption(TUint level, TUint aName,const TDesC8 &option,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt Send(RMBufChain& aPDU,CProtocolBase* aSourceProtocol=NULL);
	virtual void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol = NULL);
	static void Describe(TServerProtocolDesc&);

	TUint RecvBuf() const           { return iRecvBuf; }
	TUint WaitNif() const           { return iWaitNif; }

private:
	// Global receive buffer setting read from udp_recv_buf. Can be overridden with socket option.
	TUint iRecvBuf;
	
	// Set if the sender should be blocked to wait for interface startup.
	TUint iWaitNif:1;
	};


/**
UDP SAP declaration.

@internalComponent
*/
class CProviderUDP6 : public CProviderInet6Transport
	{
	friend class CProtocolUDP6;

public:
	CProviderUDP6(CProtocolInet6Base* aProtocol);
	virtual ~CProviderUDP6();
	virtual void InitL();
	virtual TInt GetOption(TUint level,TUint name,TDes8 &anOption) const;
	virtual void Ioctl(TUint level,TUint name,TDes8* anOption);
	virtual void CancelIoctl(TUint aLevel, TUint aName);
	virtual TInt SetOption(TUint level,TUint name, const TDesC8 &anOption);
	//virtual void AutoBind() {}
	virtual TInt SetRemName(TSockAddr &aAddr);
	virtual void Shutdown(TCloseType option);
	virtual void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol = NULL);
	virtual void Error(TInt aError, TUint aOperationMask = MSocketNotify::EErrorAllOperations);
	virtual void CanSend();

	// PRTv1.0 send and receive methods
	virtual void GetData(TDes8 &aDesc,TUint options,TSockAddr *aAddr=NULL);

	// PRTv1.5 send and receive methods
	virtual TInt GetData(RMBufChain& aData, TUint aLength, TUint aOptions, TSockAddr* anAddr=NULL);

private:
	virtual TInt DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint aOptions, TUint aOffset); 
	virtual TInt Recv(TDualBufPtr& aBuf, TUint aLength, TUint aOptions, TSockAddr* anAddr=NULL);

	CProtocolUDP6* Protocol() const { return (CProtocolUDP6*)iProtocol; }
    
	RMBufAllocator iBufAllocator;
	RMBufPktQ iSockInQ;				//< Queue for incoming packets.
	RMBufSendPacket iSockOutBuf;	//< Stores the last outgoing packet.
	TUint iSockOutOffset;			//< Offset to the UDP header in iSockOutBuf.
	TUint iSockInQLen;				//< Current input queue length in bytes.
	TUint iSockInBufSize;			//< Buffer size for storing inbound data.
	};


#endif
