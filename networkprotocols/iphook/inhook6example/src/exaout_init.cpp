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
// exaout_init.cpp - Outbound plugin example protocol initialization
//

#include "exaout.h"
#include "protocol_module.h"

const TInt CExaoutFlowInfo::iOffset = _FOFF(CExaoutFlowInfo, iDLink);

CProtocolExaout::CProtocolExaout() : iFlowList(CExaoutFlowInfo::iOffset)
	/**
	* Constructor.
	*
	* (nothing much to do in this example)
	*/
	{
	}

CProtocolExaout::~CProtocolExaout()
	/**
	* Desctructor.
	*
	* What should be done with the flows? At minimum they must
	* be detached from the protocol instance! What else needs to
	* be done, depends on the desired plugin semantics.
	*
	* There are free choices to handle the situation
	*
	* (1) CFlowContext::SetChanged();
	*	The associated flow will be set into "changed" state. The next
	*	time a packet is sent using this flow (or if flows status is
	*	queried), the flow will be closed and re-connected => the
	*	CExaoutFlowInfo will be deleted at that point.
	*
	* (2) CFlowContext::SetStatus(KErrCancel);
	*	The associated flow is set into error state. The flow is closed
	*	at once. This is an indirect way of getting the CExaoutFlowInfo
	*	instances deleted. The side effect is that the attached socket
	*	(if any) also goes into error state.
	*
	* (3) Nothing
	*	The flow is left as is, CExaoutFlowInfo::ApplyL continues to be
	*	called.
	*
	* This example chooses the last alternative (3). However, the flows
	* must now be detached from the protocol instance (which is going
	* to be deleted).
	*/
	{
/** @code */
	CExaoutFlowInfo* temp = iFlowList.First();
	while (temp != NULL)
		{
		temp->iDLink.Deque();
		temp->iMgr = NULL;
		temp = iFlowList.First();
		}
/** @endcode */
	}

void CProtocolExaout::NetworkAttachedL()
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
/** @code */
	NetworkService()->BindL(this, BindFlowHook());
/** @endcode */
	}

void CProtocolExaout::NetworkDetached()
	/**
	* The TCP/IP stack is being detached from this plugin.
	*
	* What should be done with the flows? Depends on the
	* function of the plugin:
	*
	* - first choice: DO NOTHING! The flow objects
	* get automaticly deleted when the flow they are attached
	* to, is closed or reconnected.
	*
	* - if the processing of the flow requires presense of the
	* stack, then the processing code needs to check whether
	* the NetwokService() is still present or not.
	*
	* What you absolutely <b>CANNOT DO</b> here is: directly
	* delete the MFlowHook instances!
	*
	* This is similar situation as in ~CProtocolExaut() desctructor,
	* but because the protocol instance remains available, the flows
	* could be left attached to it.
	*/
	{
/** @code */
	//
	// One option: just decouple all current flow infos from the
	// procotol instance and let them "float" on their own
	// until their attached flows get closed.
	CExaoutFlowInfo* temp = iFlowList.First();
	while (temp != NULL)
		{
		temp->iDLink.Deque();
		temp->iMgr = NULL;
		temp = iFlowList.First();
		}
/** @endcode */
	}

void CProtocolExaout::Identify(TServerProtocolDesc *aDesc) const
	/**
	* Returns description of this protocol.
	*
	* The description is required by this and also by CProtocolFamilyBase::ProtocolList,
	* which in the example enviroment gets translated into a call to the
	* Describe. Identify can use the same function.
	*/
	{
/** @code */
	Describe(*aDesc);
/** @endcode */
	}

void CProtocolExaout::Describe(TServerProtocolDesc& anEntry)
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
/** @code */
	anEntry.iName=_S("exaout");
	anEntry.iAddrFamily=KAfExaout;
	anEntry.iSockType=0;
	anEntry.iProtocol=KProtocolExaout;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=0;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=0;
	anEntry.iMessageSize=0;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=0;
/** @endcode */
	}

//
// Example test environment wrappings
// ----------------------------------

TInt ProtocolModule::NumProtocols()
	{
	return 1;
	}


void ProtocolModule::Describe(TServerProtocolDesc& anEntry, const TInt /*aIndex*/)
	{
	CProtocolExaout::Describe(anEntry);
	}

CProtocolBase* ProtocolModule::NewProtocolL(TUint /*aSockType*/, TUint aProtocol)
	{
	if (aProtocol != KProtocolExaout)
		User::Leave(KErrNotSupported);
	return new (ELeave) CProtocolExaout;
	}


