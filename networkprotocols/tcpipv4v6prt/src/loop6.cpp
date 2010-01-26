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
// loop6.cpp - loopback interface for IPv6
//

#include "in_fmly.h"
#include "loop6.h"
#include <in6_if.h>	// KSoIf*
#include "networkinfo.h"

CIfLoop6::CIfLoop6()
{
	__DECLARE_NAME(_S("CIfLoop6"));
}

void CIfLoop6::ConstructL(const TDesC& aTag)
{
	iIfaceName = aTag.AllocL();
	iIfaceMTU = 4096;

	TCallBack scb(SendCallBack, this);
	iSendCallBack = new(ELeave) CAsyncCallBack(scb, KInet6DefaultPriority);

	TCallBack rcb(RecvCallBack, this);
	iRecvCallBack = new(ELeave) CAsyncCallBack(rcb, KInet6DefaultPriority);
}

CIfLoop6::~CIfLoop6()
{
	iRecvQ.Free();
	iSendQ.Free();

	delete iRecvCallBack;
	delete iSendCallBack;
	delete iIfaceName;
}

CIfLoop6* CIfLoop6::NewL(const TDesC& aTag)
{
	CIfLoop6* p = new(ELeave) CIfLoop6;

	CleanupStack::PushL(p);
	p->ConstructL(aTag);
	CleanupStack::Pop();

	return p;
}

void CIfLoop6::BindL(TAny* aId)
{
	iNetwork = (CProtocolBase*)aId;
	iNetwork->StartSending((CProtocolBase *)this);
}

TInt CIfLoop6::State()
{
	return EIfUp;
}

TInt CIfLoop6::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny*)
	{
	if (aLevel == KSOLInterface)
		{
		switch (aName)
			{
			case KSoIfInfo6:
				{
				__ASSERT_DEBUG(STATIC_CAST(TUint, aOption.MaxLength()) >= sizeof (TSoIfInfo6), User::Panic(_L("LOOP6"), 0));

				if (STATIC_CAST(TUint, aOption.MaxLength()) < sizeof (TSoIfInfo6))
					return KErrArgument;

				TSoIfInfo6& opt = *(TSoIfInfo6*)aOption.Ptr();
				opt.iRMtu = iIfaceMTU;
				}
				// FALL THROUGH AS TSoIfInfo6 IS AN EXTENSION OF TSoIfInfo

			case KSoIfInfo:
				{
				__ASSERT_DEBUG(STATIC_CAST(TUint, aOption.MaxLength()) >= sizeof (TSoIfInfo), User::Panic(_L("LOOP6"), 0));

				if (STATIC_CAST(TUint, aOption.MaxLength()) < sizeof (TSoIfInfo))
					return KErrArgument;

				TSoIfInfo& opt = *(TSoIfInfo*)aOption.Ptr();
	
				opt.iFeatures = KIfIsLoopback | KIfCanMulticast;
				opt.iMtu = iIfaceMTU;
				opt.iSpeedMetric = 30000;
				opt.iName = *iIfaceName;

				return KErrNone;
				}


			case KSoIfConfig:
				// PS: If KSoIfConfig returns KErrNotSupported, Update6 does not proceed
				// with setting routes, etc.
				{
				  TSoInet6IfConfig& cfg = *(TSoInet6IfConfig*)aOption.Ptr();	
				  if (!iIfaceName->Compare(_L("loop6")) && cfg.iFamily == KAfInet6)
				    {
				    
				      TSockAddr unspec(KAFUnspec);
				      cfg.iLocalId = unspec;
				      cfg.iRemoteId = unspec;
				      return KErrNone;
				    }
				}
				break;
			case KSoIfGetConnectionInfo:
				{
				if (STATIC_CAST(TUint, aOption.MaxLength()) < sizeof(TSoIfConnectionInfo))
					return KErrArgument;
				TSoIfConnectionInfo &opt = *(TSoIfConnectionInfo *)aOption.Ptr();
				opt.iIAPId = 0;
				opt.iNetworkId = 0;
				return KErrNone;
				}

			default:
				break;
			}
		}
	return KErrNotSupported;
}

void CIfLoop6::Info(TNifIfInfo& aInfo) const
{
	aInfo.iVersion = TVersion(KInet6MajorVersionNumber, KInet6MinorVersionNumber, KInet6BuildVersionNumber);
	aInfo.iFlags = KNifIfIsBase | KNifIfCreatedAlone;
	aInfo.iName = *iIfaceName;
	aInfo.iProtocolSupported = KProtocolInet6Ip;
}

TInt CIfLoop6::Send(RMBufChain &aPacket, TAny*)
{
	iSendQ.Append(aPacket);
	iSendCallBack->CallBack();

	return 1;
}

TInt CIfLoop6::RecvCallBack(TAny* aCProtocol)
{
	((CIfLoop6*)aCProtocol)->DoProcess();

	return 0;
}

TInt CIfLoop6::SendCallBack(TAny* aCProtocol)
{
	((CIfLoop6*)aCProtocol)->DoSend();

	return 0;
}

void CIfLoop6::DoSend()
{
	RMBufPacket send;
	RMBufPacket recv;

	while (iSendQ.Remove(send))
	{
		send.Unpack();
		Loop(send, recv);
		iRecvQ.Append(recv);
		iRecvCallBack->CallBack();
		send.Free();
	}
}

void CIfLoop6::DoProcess()
{
	RMBufPacket packet;

	while (iRecvQ.Remove(packet))
	{
		iNetwork->Process(packet, (CProtocolBase*)this);
	}
}
			
void CIfLoop6::Loop(RMBufPacket& aSend, RMBufPacket& aRecv)
{
	RMBufPktInfo* info = aSend.Info();

	info->iProtocol = KProtocolInet6Loop;

	aSend.SetInfo(NULL);
	aRecv.Assign(aSend);
	aRecv.SetInfo(info);
	aRecv.Pack();
}
