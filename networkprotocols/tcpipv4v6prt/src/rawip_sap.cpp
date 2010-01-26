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
// rawip_sap.cpp - Raw IP socket
//

#include "rawip.h"
#include <in_pkt.h>
#include <in_chk.h>
#include "in_net.h"
//

class CProtocolRawBinder;

class CProviderRawIp : public CProviderInet6Network
	{
public:
	CProviderRawIp(CProtocolInet6Base* aProtocol);
	~CProviderRawIp();
	virtual TInt SetLocalName(TSockAddr &aAddr);
	virtual TInt DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint aOptions, TUint aOffset);
	virtual TInt SecurityCheck(MProvdSecurityChecker *aChecker);
	virtual void InitL();
private:
	CProtocolRawBinder *iBinder;
	};


class CProtocolRawBinder : public CProtocolBase
	{
public:
	CProtocolRawBinder(CProviderRawIp &aProvider) : iProvider(aProvider) {}
	virtual void Identify(TServerProtocolDesc *) const;
	virtual void Process(RMBufChain&, CProtocolBase* aSourceProtocol=NULL);
private:
	CProviderRawIp &iProvider;
	};


CServProviderBase *RAWIP::NewSAPL(TUint aSockType, CProtocolInet6Base *aProtocol)
	{
	LOG(Log::Printf(_L("NewSAPL\t%S SockType=%d"), &aProtocol->ProtocolName(), aSockType));
	if (aSockType != KSockRaw)
		User::Leave(KErrNotSupported);
	CProviderRawIp *provider = new (ELeave) CProviderRawIp(aProtocol);
	CleanupStack::PushL(provider);
	provider->InitL();
	CleanupStack::Pop();
	LOG(Log::Printf(_L("NewSAPL\t%S SAP[%u] OK"), &aProtocol->ProtocolName(), (TInt)provider));
	return provider;
	}

//

CProviderRawIp::CProviderRawIp(CProtocolInet6Base* aProtocol) :
	CProviderInet6Network(aProtocol)
	{
	}

void CProviderRawIp::InitL()
	{
	iBinder = new (ELeave) CProtocolRawBinder(*this);
	CProviderInet6Network::InitL();	
	}

CProviderRawIp::~CProviderRawIp()
	{
	iProtocol->NetworkService()->Protocol()->Unbind(iBinder);
	delete iBinder;
	}

// SetLocalName (Bind() in application) with non-zero port activates
// the incoming path for packets of the specific protocol. 
TInt CProviderRawIp::SetLocalName(TSockAddr &aAddr)
	{
	const TInt id = aAddr.Port();
	// Only bind as real upper layer protocol, limit the value...
	if (id < 0 || id > 255)
		return KErrNotSupported;
	if (iProtocol == NULL || iProtocol->NetworkService() == NULL)
		return KErrNotReady;	// Should never really happen...
	iProtocol->NetworkService()->Protocol()->Unbind(iBinder);
	if (id > 0)
		{
		// Only non-zero protocol is bound.
		TRAPD(err, iProtocol->NetworkService()->BindL(iBinder, id));
		if (err != KErrNone)
			return err;
		}
	// iProtocolId is used to fill in the protocol number
	// for outgoing packets, if non-zero (if zero, then
	// the port of the destination address is used as protocol).
	iProtocolId = id;
	// Do the default SetLocalName without the port!
	aAddr.SetPort(0);
	return CProviderInet6Network::SetLocalName(aAddr);
	}


TInt CProviderRawIp::DoWrite(RMBufSendPacket & /*aPacket*/, RMBufSendInfo &aInfo, TUint /*aOptions*/, TUint /*aOffset*/)
	{
	iFlow.SetIcmpType(0, 0);
	iFlow.SetNotify(this);
	if (aInfo.iSrcAddr.Family())
		iFlow.SetLocalAddr(aInfo.iSrcAddr);
	if (aInfo.iDstAddr.Family())
		iFlow.SetRemoteAddr(aInfo.iDstAddr);
	return aInfo.iFlow.Open(iFlow, &aInfo);
	}

// Require network control for raw ip sockets
TInt CProviderRawIp::SecurityCheck(MProvdSecurityChecker *aChecker)
	{
	const TInt res = CProviderInet6Network::SecurityCheck(aChecker);
	if (res == KErrNone)
		return CheckPolicy(KPolicyNetworkControl, "TCPIP RAWIP SAP");
	return res;
	}

void CProtocolRawBinder::Process(RMBufChain &aPacket,CProtocolBase * /*aSourceProtocol*/)
	{
	for (;;)	// LOOP ONLY FOR EASY BREAK EXITS!
		{
		RMBufRecvInfo *const info = (RMBufRecvInfo *)RMBufPacketBase::PeekInfoInChain(aPacket);
		if (info == NULL)		
			break;
		if (info->iIcmp)
			{
			// For now, ignore ICMP error reports.
			// Could do the save icmp error to support the "Last ICMP" error feature,
			// in socket. That would require translating ICMP error codes into Symbian
			// error codes (which is done in udp.cpp for example -- should have some
			// shared code for this ICMP handling).
			break;
			}
		iProvider.Process(aPacket);
		// ALWAYS TERMINATE THE LOOP
		break;
		}
	aPacket.Free();
	}

void CProtocolRawBinder::Identify(TServerProtocolDesc *aInfo) const
	{
	// If asked, the binder returns the same info as the "rawip" protocol.
	RAWIP::Identify(*aInfo);
	}


