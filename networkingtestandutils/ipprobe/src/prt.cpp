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
// prt.cpp - Packet Probe Hook
//

#include <e32std.h>
#include <e32base.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <es_prot_internal.h>
#endif

#include "family.h"
#include "prt.h"
#include "sap.h"
#include "inet6log.h"


CProtocolProbe::CProtocolProbe(TUint aId) : iId(aId)
	{
	LOG(Log::Printf(_L("Probe::CProtocolProbe()\r\n"));)
	}

void CProtocolProbe::InitL(TDesC& aTag)
	{
	LOG(Log::Printf(_L("Probe::InitL()\r\n"));)
	CProtocolBase::InitL(aTag);
	}


void CProtocolProbe::StartL()
	{
	//	__ASSERT_DEBUG(iProtocol != NULL, User::Leave(KErrGeneral));
	}

CProtocolProbe::~CProtocolProbe()
	{
	LOG(Log::Printf(_L("Probe::~CProtocolProbe()\r\n"));)
	}


CProtocolProbe *CProtocolProbe::NewL(TUint aId)
	{
	return new (ELeave) CProtocolProbe(aId);
	}

void CProtocolProbe::FillIdentification(TServerProtocolDesc& anEntry, TUint aId)
	{
	anEntry.iName=_S("probe");
	if (aId > 1)
		anEntry.iName.AppendNum(aId-1);
	anEntry.iAddrFamily = KAfProbe;
	anEntry.iSockType = KSockDatagram;
	anEntry.iProtocol = aId;
	anEntry.iVersion = TVersion(1, 0, 0);
	anEntry.iByteOrder = EBigEndian;
	anEntry.iServiceInfo = KSIDatagram | KSIConnectionLess;
	anEntry.iNamingServices = 0;
	anEntry.iSecurity = KSocketNoSecurity;
	anEntry.iMessageSize = 0xffff;
	anEntry.iServiceTypeInfo = ESocketSupport;
	anEntry.iNumSockets = KUnlimitedSockets;
	}

void CProtocolProbe::Identify(TServerProtocolDesc *aDesc) const
	{
	FillIdentification(*aDesc, iId);
	}


CServProviderBase* CProtocolProbe::NewSAPL(TUint aProtocol)
	{
	LOG(Log::Printf(_L("Probe::NewSAPL(%d)\r\n"), aProtocol);)

	if (aProtocol != KSockDatagram)
		User::Leave(KErrNotSupported);
	CProviderProbe* sap = new (ELeave) CProviderProbe(this);
	sap->iNext = iList;
	iList = sap;
	return sap;
	}

//
// CProtocolProbe::CancelSAP
// *************************
// Disconnect SAP from the protocol
//
void CProtocolProbe::CancelSAP(const CServProviderBase* aSAP)
	{
	CProviderProbe **h, *sap;
	for (h = &iList; (sap = *h) != NULL; h = &sap->iNext)
		if (sap == aSAP)
			{
			*h = sap->iNext;
			break;
			}
	}


// CProtocolProbe::NetworkAttachedL
// ********************************
// When network becomes available, do the hooking!
//
void CProtocolProbe::NetworkAttachedL()
	{
	NetworkService()->BindL(this, MIp6Hook::BindPostHook());
	NetworkService()->BindL(this, MIp6Hook::BindPostHook()+1);
	}

//
//	CProtocolProbe::Deliver
//	***********************
//	Generate a copy of the packet to every bound provider
//
void CProtocolProbe::Deliver(RMBufChain &aPacket)
	{
	const RMBufPktInfo *const info = RMBufPacketBase::PeekInfoInChain(aPacket);

	for (CProviderProbe* sap = iList; sap != NULL; sap = sap->iNext)
		{
		if (sap->IsReceiving(*info))
			{
			RMBufPacketBase copy;
			TRAPD(err, copy.CopyPackedL(aPacket));
			if (err == KErrNone)
				sap->Process(copy, this);
			else
				copy.Free();
			}
		}
	}


TInt CProtocolProbe::Send(RMBufChain &aPacket, CProtocolBase* aSrc)
	{
	Deliver(aPacket);
	return CProtocolPosthook::Send(aPacket, aSrc);
	}

void CProtocolProbe::Process(RMBufChain &aPacket, CProtocolBase* aSrc)
	{
	Deliver(aPacket);
	CProtocolPosthook::Process(aPacket, aSrc);
	}
