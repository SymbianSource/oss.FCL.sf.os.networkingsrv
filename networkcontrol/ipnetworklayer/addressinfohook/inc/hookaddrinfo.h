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
 @file hookaddrinfo.h
 @internalTechnology
 @prototype
*/

#if !defined (_HOOKADDRINFO_H_)
#define _HOOKADDRINFO_H_


#include <networking/ipaddrinfoparams.h>

#include <cflog.h>

#ifdef __CFLOG_ACTIVE
// CommsDebugUtility logging tags. Use them to enable tracing for ReferenceSCPR
_LIT8(KIPAddrInfoHookTag1,"IPAddrHook");
_LIT8(KIPAddrInfoHookTag2,"IPInfo");
#endif

class CAddressInfoFlowInfo;
class CIPProtoBinder;

struct TIpAddrBinder
	{
	TIpAddrBinder(CIPProtoBinder *iBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo &iAddrInfo);
	
	CSubConIPAddressInfoParamSet::TSubConIPAddressInfo iAddrInfo;
	CIPProtoBinder *iBinder;
	};

class CHookAddressInfo : public CBase
	{
public:
	CHookAddressInfo();
	~CHookAddressInfo();
	
	//This function will be used by the hook.
	//It adds aAddrInfo to the list and expects the ID to be set
	IMPORT_C void AddL(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& aAddrInfo);
	IMPORT_C void Remove(CIPProtoBinder* aBinder);
	IMPORT_C void Remove(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo &aAddrInfo);
	
	TBool Match(TInt srcPort, TInt dstPort, TInt /*protocol*/, CIPProtoBinder* aBinder);
	
	TDblQue<CAddressInfoFlowInfo> *iFlows;
private:

	RPointerArray<TIpAddrBinder> iAddrInfo;
	};

#endif
