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
// IPProto SubConnection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTODEFTSCPR_H
#define SYMBIAN_IPPROTODEFTSCPR_H

#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/corescpr.h>
#include <comms-infras/corescprstates.h>
#include <comms-infras/corecprstates.h>
#include <comms-infras/netcfgextnotify.h>
#include <comms-infras/nifconfigurationcontrol.h>
#include <networking/pdpdef.h>
#include "IPProtoSCPRFactory.h"
#include <comms-infras/ss_datamonitoringprovider.h>
#include <comms-infras/ss_log.h>
#include <comms-infras/ss_nodemessages_legacy.h>

#define KIPProtoDeftScprTag KESockSubConnectionTag

namespace IPProtoDeftSCpr
{
    class TStoreProvision;
	//class TAwaitingFlowUp;
	//class TAwaitingFlowDown;
	class TStartNetCfgExt;
	class TStopNetCfgExtOrNoTag;
	class TStopNetCfgExt;
	class TProcessIoctl;
	class TCancelOutstandingIoctls;
	class THandoffToNetCfgExt;
	class TNoTagOrTryNetCfgExtOrCancelIoctl;
	class TTryPdpOrTryNetCfgExtOrCancelIoctl;
	class TNoTagOrCancelIoctl;
	class TProcessAgentEvent;
    class TAwaitingIoctlProcessed;
	class TStopNetCfgExtDelete;
	class TNoTagOrProviderStoppedOrDaemonReleased;
}

class CIPProtoDeftSubConnectionProvider : public CCoreSubConnectionProvider, public ESock::ADataMonitoringProvider, public ESock::ALegacySubConnectionActiveApiExt,
                                   	  public ITFHIERARCHY_LINK_2(CIPProtoDeftSubConnectionProvider, CCoreSubConnectionProvider, ESock::ADataMonitoringProtocolReq, ESock::ALegacySubConnectionActiveApiExt)
/** IPProto subconnection provider base class

@internalTechnology
@released Since 9.4 */
	{
	friend class CIPProtoSubConnectionProviderFactory;
	friend class IPProtoDeftSCpr::TStoreProvision;
    //friend class IPProtoDeftSCpr::TAwaitingFlowUp;
	//friend class IPProtoDeftSCpr::TAwaitingFlowDown;
	friend class IPProtoDeftSCpr::TStartNetCfgExt;
	friend class IPProtoDeftSCpr::TStopNetCfgExtOrNoTag;
	friend class IPProtoDeftSCpr::TStopNetCfgExt;
	friend class IPProtoDeftSCpr::TProcessIoctl;
	friend class IPProtoDeftSCpr::TCancelOutstandingIoctls;
	friend class IPProtoDeftSCpr::THandoffToNetCfgExt;
	friend class IPProtoDeftSCpr::TNoTagOrTryNetCfgExtOrCancelIoctl;
	friend class IPProtoDeftSCpr::TNoTagOrCancelIoctl;
	friend class IPProtoDeftSCpr::TTryPdpOrTryNetCfgExtOrCancelIoctl;
	friend class IPProtoDeftSCpr::TProcessAgentEvent;		// for iControl
	friend class IPProtoDeftSCpr::TAwaitingIoctlProcessed;
  	friend class IPProtoDeftSCpr::TStopNetCfgExtDelete;
	friend class IPProtoDeftSCpr::TNoTagOrProviderStoppedOrDaemonReleased;

	
public:
	typedef ITFHIERARCHY_LINK_2(CIPProtoDeftSubConnectionProvider, CCoreSubConnectionProvider, ADataMonitoringProtocolReq, ALegacySubConnectionActiveApiExt) TIfStaticFetcherNearestInHierarchy;

    typedef CIPProtoSubConnectionProviderFactory FactoryType;
	virtual ~CIPProtoDeftSubConnectionProvider();

	void ReturnInterfacePtrL(ADataMonitoringProtocolReq*& aInterface);
	void ReturnInterfacePtrL(ALegacySubConnectionActiveApiExt*& aInterface);

protected:
    CIPProtoDeftSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory);
    void ConstructL();
    static CIPProtoDeftSubConnectionProvider* NewL(ESock::CSubConnectionProviderFactoryBase& aFactory);
    void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

protected:
	CNetCfgExtNotify* iNotify;
	CNifConfigurationControl* iControl;
    Messages::RNodeInterface iFlow;
	TBool iIoctlCancelled;
public:
	TBool iNetworkConfigurationState;
	};

#endif //SYMBIAN_IPPROTODEFTSCPR_H
