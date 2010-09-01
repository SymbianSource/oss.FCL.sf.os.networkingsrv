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
// sap.cpp - Packet Probe Hook
//

#include "sap.h"

CProviderProbe::CProviderProbe(CProtocolProbe* aProtocol) : iProtocol(aProtocol)
	{
	__DECLARE_NAME(_S("CProviderProbe"));
	iQueueLimit = 8000;
	}

CProviderProbe::~CProviderProbe()
	{
	if (iProtocol)
		iProtocol->CancelSAP(this);
	}

void CProviderProbe::Start()
	{
	}

TInt CProviderProbe::GetOption(TUint /*aLevel*/, TUint /*aName*/, TDes8& /*aOption*/) const
	{
	return KErrNotSupported;
	}

TInt CProviderProbe::SetOption(TUint /*aLevel*/, TUint /*aName*/, const TDesC8& /*aOption*/)
	{
//	return KErrNotSupported;
	return KErrNone;
	}

void CProviderProbe::Ioctl(TUint /*level*/,TUint /*name*/,TDes8* /*anOption*/)
	{
	Panic(EProbePanic_NotSupported);
	}

void CProviderProbe::CancelIoctl(TUint /*aLevel*/,TUint /*aName*/)
	{
	Panic(EProbePanic_NotSupported);
	}


TUint CProviderProbe::Write(const TDesC8 &/*aDesc*/, TUint /*aOptions*/, TSockAddr* /*aAddr*/)
	{
	// For now, Write does nothing on probe socket
	return 1;
	}

void CProviderProbe::Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/)
	{
	Panic(EProbePanic_NotSupported);
	}

void CProviderProbe::Shutdown(TCloseType aOption)
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

TInt CProviderProbe::SetLocalName(TSockAddr &/*aAddr*/)
	{
	return 0;	// Ignore silently
	}

void CProviderProbe::AutoBind()
	{
	// Ignore silently
	}

// CProviderProbe::Error
// *********************
// Soft errors are not immediately reported to the socket server.
// A soft error is indicated by a zero aOperationMask.
//
// The socket error can be cleared by calling this routing with
// aError == KErrNone.
//
void CProviderProbe::Error(TInt aError, TUint aOperationMask)
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
// CProviderProbe::Process()
// *************************
// Process incoming data from the protocol object.
//
void CProviderProbe::Process(RMBufChain& aPacket, CProtocolBase * /*aSourceProtocol*/)
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

// CProviderProbe::IsReceiving
// ***************************
//
TBool CProviderProbe::IsReceiving(const RMBufPktInfo & /*aInfo*/)
	{
	if (iQueueLimit < 0)
		{
		// Receive Queue limit is full, cannot receive this packet
		iPacketsDropped++;
		return FALSE;
		}
	return TRUE;
	}


// CProviderProbe::GetData
// ***********************
void CProviderProbe::GetData(TDes8 &aDesc, TUint aOptions, TSockAddr *anAddr)
	{
	RMBufPacketBase packet;
	if (!iRecvQ.Remove(packet))
		Panic(EProbePanic_NoData);

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

#ifdef TCPIP6_CAPABILITY
TInt CProviderProbe::SecurityCheck(MProvdSecurityChecker *aChecker)
	{
	_LIT_SECURITY_POLICY_C1(KPolicyNetworkControl, ECapabilityNetworkControl);
	return aChecker->CheckPolicy(KPolicyNetworkControl, "PROBE HOOK SAP");
	}
#endif
