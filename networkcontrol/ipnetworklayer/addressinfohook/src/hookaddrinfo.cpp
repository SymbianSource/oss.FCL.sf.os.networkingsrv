// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file hookaddrinfo.cpp
 @internalTechnology
 @prototype
*/


#include "hookaddrinfo.h"
#include "addressinfo.h"


CHookAddressInfo::CHookAddressInfo()
	: iFlows(0)
	{
	}

CHookAddressInfo::~CHookAddressInfo()
	{
	iAddrInfo.ResetAndDestroy();
	}


EXPORT_C void CHookAddressInfo::AddL(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& aAddrInfo)
	{
	TIpAddrBinder *addrBinder =  new(ELeave) TIpAddrBinder(aBinder, aAddrInfo);
	
	iAddrInfo.Append(addrBinder);

	//Try match the address info to existing flows	
	TDblQueIter<CAddressInfoFlowInfo> iter(*iFlows);
	CAddressInfoFlowInfo* flow;
	iter.SetToFirst();
	CIPProtoBinder *temp = 0;
	while((flow = iter++) != NULL)
		{
		if (Match(flow->iFlow.LocalPort(),flow->iFlow.RemotePort(),flow->iFlow.Protocol(), temp))
			{
			__CFLOG_VAR((KIPAddrInfoHookTag1, KIPAddrInfoHookTag1, _L8("Existing Flow matched id %08x, lport %d rport %d"), aBinder, flow->iFlow.LocalPort(), flow->iFlow.RemotePort()));
			flow->iBinder = aBinder;
			}
		}
	}
	
	
TBool CHookAddressInfo::Match(TInt srcPort, TInt dstPort, TInt /*protocol*/, CIPProtoBinder* aBinder)
	{
	TInt count = iAddrInfo.Count();
	for (TInt i=0; i < count; i++)
		{
		if (iAddrInfo[i]->iAddrInfo.iCliSrcAddr.Port() == srcPort &&
			iAddrInfo[i]->iAddrInfo.iCliDstAddr.Port() == dstPort)
			{
			aBinder = iAddrInfo[i]->iBinder;
			return ETrue;
			}
		}

	return EFalse;
	}


EXPORT_C void CHookAddressInfo::Remove(CIPProtoBinder* aBinder)
	{
	TInt count = iAddrInfo.Count();
	for (TInt i=count-1; i >=0; i--)
		{
		if (iAddrInfo[i]->iBinder == aBinder)
			{
			delete iAddrInfo[i];
			iAddrInfo.Remove(i);
			
			}
		}
	}

EXPORT_C void CHookAddressInfo::Remove(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo &aAddrInfo)
	{
	TInt count = iAddrInfo.Count();
	for (TInt i=count-1; i >=0; i--)
		{
		if (iAddrInfo[i]->iAddrInfo.Compare(aAddrInfo) && iAddrInfo[i]->iBinder == aBinder)
			{
			delete iAddrInfo[i];
			iAddrInfo.Remove(i);
			}
		}
	}

TIpAddrBinder::TIpAddrBinder(CIPProtoBinder *aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo &aAddrInfo)
	: iAddrInfo(aAddrInfo), iBinder(aBinder)
	{
	}
