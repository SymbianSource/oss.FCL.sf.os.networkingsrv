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

#ifndef SYMBIAN_IPPROTOSCPR_H
#define SYMBIAN_IPPROTOSCPR_H

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

#define KIPProtoSCPRTag KESockSubConnectionTag

namespace IPProtoSCpr
{
    class TStoreProvision;
	class TAwaitingFlowUp;
	class TAwaitingFlowDown;
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
#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	class TSendParamsToServiceProvider;
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	class TSendDataClientRoutedToFlow;
	class TStopNetCfgExtDelete;
	class TNoTagOrProviderStoppedOrDaemonReleased;
}

class CIPProtoSubConnectionProvider : public CCoreSubConnectionProvider, public ESock::ADataMonitoringProvider, public ESock::ALegacySubConnectionActiveApiExt,
                                   	  public ITFHIERARCHY_LINK_2(CIPProtoSubConnectionProvider, CCoreSubConnectionProvider, ESock::ADataMonitoringProtocolReq, ESock::ALegacySubConnectionActiveApiExt)
/** IPProto subconnection provider base class

@internalTechnology
@released Since 9.4 */
	{
	friend class CIPProtoSubConnectionProviderFactory;
	friend class IPProtoSCpr::TStoreProvision;
    friend class IPProtoSCpr::TAwaitingFlowUp;
	friend class IPProtoSCpr::TAwaitingFlowDown;
	friend class IPProtoSCpr::TStartNetCfgExt;
	friend class IPProtoSCpr::TStopNetCfgExtOrNoTag;
	friend class IPProtoSCpr::TStopNetCfgExt;
	friend class IPProtoSCpr::TProcessIoctl;
	friend class IPProtoSCpr::TCancelOutstandingIoctls;
	friend class IPProtoSCpr::THandoffToNetCfgExt;
	friend class IPProtoSCpr::TNoTagOrTryNetCfgExtOrCancelIoctl;
	friend class IPProtoSCpr::TNoTagOrCancelIoctl;
	friend class IPProtoSCpr::TTryPdpOrTryNetCfgExtOrCancelIoctl;
	friend class IPProtoSCpr::TProcessAgentEvent;		// for iControl
#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	friend class IPProtoSCpr::TSendParamsToServiceProvider;
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	friend class IPProtoSCpr::TSendDataClientRoutedToFlow;
	friend class IPProtoSCpr::TStopNetCfgExtDelete;
	friend class IPProtoSCpr::TNoTagOrProviderStoppedOrDaemonReleased;
public:
	typedef ITFHIERARCHY_LINK_2(CIPProtoSubConnectionProvider, CCoreSubConnectionProvider, ADataMonitoringProtocolReq, ALegacySubConnectionActiveApiExt) TIfStaticFetcherNearestInHierarchy;

    typedef CIPProtoSubConnectionProviderFactory FactoryType;
	virtual ~CIPProtoSubConnectionProvider();

	void ReturnInterfacePtrL(ADataMonitoringProtocolReq*& aInterface);
	void ReturnInterfacePtrL(ALegacySubConnectionActiveApiExt*& aInterface);

protected:
    CIPProtoSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory);
    void ConstructL();
    static CIPProtoSubConnectionProvider* NewL(ESock::CSubConnectionProviderFactoryBase& aFactory);
    void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

protected:
	CNetCfgExtNotify* iNotify;
	CNifConfigurationControl* iControl;
    Messages::RNodeInterface iFlow;
	TBool iIoctlCancelled;
	};

#endif //SYMBIAN_IPPROTOSCPR_H
