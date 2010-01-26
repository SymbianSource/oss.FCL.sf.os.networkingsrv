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
// Outbound plugin example protocol initialization
// 
//

/**
 @file addressinfo_init.cpp
 @internalTechnology
 @prototype
*/

#include "addressinfo.h"
#include "protocol_module.h"
#include <comms-infras/nifif.h>
#include <in_iface.h>

const TInt CAddressInfoFlowInfo::iOffset = _FOFF(CAddressInfoFlowInfo, iDLink);

CProtocolAddressInfo::CProtocolAddressInfo() : iFlowList(CAddressInfoFlowInfo::iOffset)
	/**
	* Constructor.
	*/
	{
	iAddrInfo.iFlows = &iFlowList;
	}

CProtocolAddressInfo::~CProtocolAddressInfo()
	/**
	* Desctructor.
	*
	*/
	{
	TDblQueIter<CAddressInfoFlowInfo> iter(iFlowList);
	CAddressInfoFlowInfo* flow;
	iter.SetToFirst();
	while((flow = iter++) != NULL)
		{
		flow->iDLink.Deque();
		flow->iMgr = NULL;
		}
	}

void CProtocolAddressInfo::NetworkAttachedL()
	/**
	* The TCP/IP stack has been attached to this plugin.
	*
	* The CProtocolPosthook impelements the basic BindL/BindToL and Unbind
	* processing. The NetworkAttached is called when the TCP/IP
	* is connected with this protocol module.
	*
	* This function installs a hook to monitor opening of all
	* flows. The OpenL will be called for flow that is opened.
	*/
	{
	NetworkService()->BindL(this, BindFlowHook());
	}

void CProtocolAddressInfo::NetworkDetached()
	/**
	* The TCP/IP stack is being detached from this plugin.
	*
	*/
	{
	//
	// One option: just decouple all current flow infos from the
	// procotol instance and let them "float" on their own
	// until their attached flows get closed.
	
	TDblQueIter<CAddressInfoFlowInfo> iter(iFlowList);
	CAddressInfoFlowInfo* flow;
	iter.SetToFirst();
	while((flow = iter++) != NULL)
		{
		flow->iDLink.Deque();
		flow->iMgr = NULL;
		}
	}

void CProtocolAddressInfo::Identify(TServerProtocolDesc *aDesc) const
	/**
	* Returns description of this protocol.
	*
	* The description is required by this and also by CProtocolFamilyBase::ProtocolList,
	* which in the example enviroment gets translated into a call to the
	* Describe. Identify can use the same function.
	*/
	{
	Describe(*aDesc);
	}

void CProtocolAddressInfo::Describe(TServerProtocolDesc& anEntry)
	/**
	* Return the description of the protocol.
	*
	* Becauses this example does not provide the socket service (SAP),
	* most of the fields in the description can be set to zero. Only
	* the following need to be initialized to something specific:
	*
	* - iName		The name of the protocol ("exain").
	* - iAddrFamily	The address family of the protocol (KAfExain).
	* - iProtocol	The protocol number (KProtocolExain).
	*/
	{
	anEntry.iName=_S("addressinfo");
	anEntry.iAddrFamily=KAfAddressInfo;
	anEntry.iSockType=0;
	anEntry.iProtocol=KProtocolAddressInfo;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=0;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=0;
	anEntry.iMessageSize=0;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=0;
	}

void CProtocolAddressInfo::InterfaceAttached(const TDesC &aName, CNifIfBase *aIf)
// We want to know when an interface attaches to the stack
	{
	(void)aName; //keep the compiler happy
	
	TNifIfInfo info;
	aIf->Info(info);
	
	TVersion version(78,96,12453);
	if (info.iVersion.iMajor == version.iMajor &&
		info.iVersion.iMinor == version.iMinor &&
		info.iVersion.iBuild == version.iBuild)
		{
		//Its our interface so lets pass a pointer to our addrinfo member
		//so it can be updated with addresses
		
		TBuf8<1> temp;
		aIf->Control(KSOLInterface, 0x734, temp, (TAny*)&iAddrInfo);
		
		}
	iNif = aIf;
	}


TInt ProtocolModule::NumProtocols()
	{
	return 1;
	}


void ProtocolModule::Describe(TServerProtocolDesc& anEntry, const TInt /*aIndex*/)
	{
	CProtocolAddressInfo::Describe(anEntry);
	}

CProtocolBase* ProtocolModule::NewProtocolL(TUint /*aSockType*/, TUint aProtocol)
	{
	if (aProtocol != KProtocolAddressInfo)
		User::Leave(KErrNotSupported);
	return new (ELeave) CProtocolAddressInfo;
	}


