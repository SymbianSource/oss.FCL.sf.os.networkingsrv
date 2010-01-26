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
// rawip.cpp - Raw IP socket support
// This implements the raw IP socket protocol module
//

#include "inet6log.h"
#include "rawip.h"
#include "in_net.h"
#include <icmp6_hdr.h>
#include <in_pkt.h>

_LIT(KRawIpName, "rawip");

//
//	CProtocolRawIp
//	*************
//	The implementation and methods of the CProtocolRawIp4/6 is totally internal
//	to this module. No other module needs to be aware of this.
//	Thus the class definition is included here.
//
class CProtocolRawIp : public CProtocolInet6Network
	{
public:
	CProtocolRawIp();
	virtual ~CProtocolRawIp();
	virtual CServProviderBase *NewSAPL(TUint aSockType);
	virtual TInt Send(RMBufChain &aPacket,CProtocolBase* aSourceProtocol=NULL);
	virtual void Identify(TServerProtocolDesc *) const;
	virtual void BindToL(CProtocolBase *aProtocol);
protected:
	};

//	*****
//	RAWIP
//	*****
CProtocolBase *RAWIP::NewL()
	{
	return new (ELeave) CProtocolRawIp();
	}

void RAWIP::Identify(TServerProtocolDesc &aEntry)
	{
	aEntry.iName = KRawIpName;
	aEntry.iProtocol = KProtocolInetRawIp;
	aEntry.iAddrFamily = KAfInet;
	aEntry.iSockType = KSockRaw;
	aEntry.iVersion = TVersion(KInet6MajorVersionNumber, KInet6MinorVersionNumber, KInet6BuildVersionNumber);
	aEntry.iByteOrder = EBigEndian;
	aEntry.iServiceInfo = KRAWIPServiceInfo;
	aEntry.iNamingServices = KRAWIPNameServiceInfo;
	aEntry.iSecurity = KSocketNoSecurity;
	aEntry.iMessageSize = KRAWIPMaxDatagramSize;
	aEntry.iServiceTypeInfo = KRAWIPServiceTypeInfo;
	aEntry.iNumSockets = KRAWIPMaxSockets;
	}

//

//
//	CProtocolRawIp* constructors and destructors
//	*******************************************

CProtocolRawIp::CProtocolRawIp()
	{
	}

CProtocolRawIp::~CProtocolRawIp()
	{
	}

//
// CProtocolRawIp::NewSAPL
//	Create a new instance of a CServProviderBase (SAP) for the
//	socket manager. The caller is responsible for the bookkeeping
//	and destruction of this created object!
//
CServProviderBase* CProtocolRawIp::NewSAPL(TUint aSockType)
	{
	return RAWIP::NewSAPL(aSockType, this);
	}

void CProtocolRawIp::Identify(TServerProtocolDesc *aInfo) const
	{
	RAWIP::Identify(*aInfo);
	}


void CProtocolRawIp::BindToL(CProtocolBase *aProtocol)
	/**
	* Bind to another protocol.
	*
	* The rawIP must be configured to bind to IP6. However, when instantiated
	* it will not know what actual protocols it will be requesting from the
	* IP layer. It will know this only after the actual socket has been opened.
	*
	* Thus, this only records the presense of the network.
	*/
	{
	ASSERT(this != aProtocol);
	TServerProtocolDesc info;
	aProtocol->Identify(&info);
	if (iNetwork == NULL && info.iProtocol == KProtocolInet6Ip)
		{
		// The network bind detected (aProtocol is IP6 instance!)
		iNetwork = ((CProtocolInet6Binder *)aProtocol)->NetworkService();
		// Following Open will be cancelled by the destructor of CProtocolInet6Binder
		aProtocol->Open();
		return;
		}
	User::Leave(KErrNotSupported);
	}

//
// CProtocolRawIp::Send()
//
//	Pass the packet as is to the IP layer. This method is
//	supposed to be used by the Raw IP Service provider modules
//	to forward their packets down the stack.
TInt CProtocolRawIp::Send(RMBufChain &aPacket,CProtocolBase* /*aSourceProtocol*/)
	{
	return iNetwork->Send(aPacket);
	}


