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
// exasap_sap.cpp - Plugin with SAP interface
//

#include <in_sock.h>
#include <nifmbuf.h>
#include "provider_module.h"

class CProviderExasap : public CServProviderBase, public MProviderForPrt
	{
public:
	CProviderExasap(MProtocolForSap &aProtocol);
	virtual ~CProviderExasap();

	//
	// Pure virtual methods from CServProviderBase. These must be implemented
	//
	virtual void Start();
	virtual void LocalName(TSockAddr& aAddr) const;
	virtual TInt SetLocalName(TSockAddr& aAddr);
	virtual void RemName(TSockAddr& aAddr) const;
	virtual TInt SetRemName(TSockAddr& aAddr);
	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8& aOption) const;
	virtual void Ioctl(TUint aLevel, TUint aName, TDes8* aOption);
	virtual void CancelIoctl(TUint aLevel, TUint aName);
	virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption);
	virtual TUint Write(const TDesC8& aDesc, TUint aOptions, TSockAddr* aAddr=NULL);
	virtual void GetData(TDes8& aDesc, TUint aOptions, TSockAddr* aAddr=NULL);
	virtual void ActiveOpen();
	virtual void ActiveOpen(const TDesC8& aConnectionData);
	virtual TInt PassiveOpen(TUint aQueSize);
	virtual TInt PassiveOpen(TUint aQueSize, const TDesC8& aConnectionData);
	virtual void Shutdown(TCloseType option);
	virtual void Shutdown(TCloseType option,const TDesC8& aDisconnectionData);
	virtual void AutoBind();

	//
	// MProviderForPrt methods
	//
	void Deliver(RMBufChain& aPacket);
	TBool IsReceiving(const RMBufPktInfo &info);

protected:
	void Error(TInt aError, TUint aOperationMask);
	inline TBool FatalState()
		{ return (iErrorMask & (MSocketNotify::EErrorFatal|MSocketNotify::EErrorConnect)) != 0; }

	MProtocolForSap &iProtocol;

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
	TUint iErrorMask;
	};

CProviderExasap::CProviderExasap(MProtocolForSap &aProtocol) : iProtocol(aProtocol)
	{
	__DECLARE_NAME(_S("CProviderExasap"));
	iQueueLimit = 8000;
	}

CProviderExasap::~CProviderExasap()
	{
	iProtocol.Unregister(*this);
	}

void CProviderExasap::Start()
	{
	iProtocol.Register(*this);
	}


void CProviderExasap::LocalName(TSockAddr& /* aAddr*/) const
	{
	}

TInt CProviderExasap::SetLocalName(TSockAddr& /*aAddr*/)
	{
	return KErrNone;
	}

void CProviderExasap::RemName(TSockAddr& aAddr) const
	{
	aAddr.SetFamily(0);
	}

TInt CProviderExasap::SetRemName(TSockAddr& /* aAddr */)
	{
	return KErrNotSupported;
	}

TInt CProviderExasap::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	{
	// Because this is basicly a "raw mode" and "header included" SAP, recognize
	// these two options, return 1 for each, if asked
	if (aLevel == KSolInetIp && (aName == KSoHeaderIncluded || aName == KSoRawMode))
		{
		TPckgBuf<TInt> val;
		val() = 1;
		aOption = val;
		return KErrNone;
		}
	return KErrNotSupported;
	}

TInt CProviderExasap::SetOption(TUint aLevel, TUint aName, const TDesC8& /*aOption*/)
	{
	if (aLevel == KSolInetIp && (aName == KSoHeaderIncluded || aName == KSoRawMode))
		return KErrNone;	// Return OK for these two options (ignore value).
	return KErrNotSupported;
	}

void CProviderExasap::ActiveOpen()
	{
	}

void CProviderExasap::ActiveOpen(const TDesC8& /*aConnectionData*/)
	{
	}

TInt CProviderExasap::PassiveOpen(TUint /*aQueSize*/)
	{
	return KErrNotSupported;
	}

TInt CProviderExasap::PassiveOpen(TUint /*aQueSize*/, const TDesC8& /*aConnectionData*/)
	{
	return KErrNotSupported;
	}

void CProviderExasap::Ioctl(TUint /*level*/,TUint /*name*/,TDes8* /*anOption*/)
	{
	}

void CProviderExasap::CancelIoctl(TUint /*aLevel*/,TUint /*aName*/)
	{
	}


TUint CProviderExasap::Write(const TDesC8 &/*aDesc*/, TUint /*aOptions*/, TSockAddr* /*aAddr*/)
	{
	// For now, Write does nothing
	return 1;
	}

void CProviderExasap::Shutdown(TCloseType aOption, const TDesC8& /*aDisconnectionData*/)
	{
	Shutdown(aOption);
	}

void CProviderExasap::Shutdown(TCloseType aOption)
	{
	switch(aOption)
		{
		case EStopInput:
			iInputStopped = ETrue;
			iRecvQ.Free();
			iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
			break;

		case EStopOutput:
			iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
			break;

		default:
	        if (aOption != EImmediate)
				iSocket->CanClose();
		}
	}


void CProviderExasap::AutoBind()
	{
	// Ignore silently
	}

// CProviderExasap::Error
// **********************
// Soft errors are not immediately reported to the socket server.
// A soft error is indicated by a zero aOperationMask.
//
// The socket error can be cleared by calling this routing with
// aError == KErrNone.
//
void CProviderExasap::Error(TInt aError, TUint aOperationMask)
	{
	if (aError <= KErrNone && !FatalState())
		{
		if (aError == KErrNone)
			iErrorMask = aOperationMask;
		else
			iErrorMask |= aOperationMask;
	    if (iSocket && aOperationMask)
			iSocket->Error(aError, aOperationMask);
		}
	}

//
// CProviderExasap::Deliver()
// **************************
// Process incoming data from the protocol object.
//
void CProviderExasap::Deliver(RMBufChain& aPacket)
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
		iSocket->NewData(1);
		}
	else
		aPacket.Free();
	}

// CProviderExasap::IsReceiving
// ****************************
//
TBool CProviderExasap::IsReceiving(const RMBufPktInfo & /*aInfo*/)
	{
	if (iQueueLimit < 0)
		{
		// Receive Queue limit is full, cannot receive this packet
		iPacketsDropped++;
		return FALSE;
		}
	return TRUE;
	}


// CProviderExasap::GetData
// ************************
void CProviderExasap::GetData(TDes8 &aDesc, TUint aOptions, TSockAddr *anAddr)
	{
	RMBufPacketBase packet;
	if (!iRecvQ.Remove(packet))
		return;

	const RMBufPktInfo *const info = packet.Unpack();

	packet.CopyOut(aDesc, 0);

	if (anAddr!=NULL)
		*anAddr = info->iSrcAddr;

	if (aOptions & KSockReadPeek)
		{
		packet.Pack();
		iRecvQ.Prepend(packet);
		iSocket->NewData(1);
		}
	else
		{
		iQueueLimit += info->iLength; // Allow more packets in..
		packet.Free();
		}
	}


//
// ProviderModule glue
//

CServProviderBase* ProviderModule::NewSAPL(TUint /*aType*/, MProtocolForSap &aProtocol)
	{
	return new (ELeave) CProviderExasap(aProtocol);
	}
