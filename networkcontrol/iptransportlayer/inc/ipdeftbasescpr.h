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
// IP Default SubConnection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPDEFTBASESCPR_H
#define SYMBIAN_IPDEFTBASESCPR_H

#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/corescpr.h>
#include <comms-infras/corescprstates.h>
#include <comms-infras/ss_nodeinterfaces.h>
#include <comms-infras/ss_corepractivities.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <tcprecvwin.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace IPBaseSCprStates
    {
    class TStoreAddressUpdate;
    class TRejoinDataClient;
    }

class CIpDefaultSubConnectionProviderFactory;
class CIpSubConnectionProviderFactory;

class RIPDataClientNodeInterface : public Messages::RNodeInterface
/** IP SCPR override for RNodeInterface data client.
Caches IP address updates reported by the data clients.

@internalTechnology
@released Since 9.4 */
    {
    public:
	TSockAddr   iCliSrcAddr;
	TSockAddr   iCliDstAddr;
	TInt        iProtocolId;
	TInt        iActivityAwaitingResponse;
	TUid	    iAppSid;

	RIPDataClientNodeInterface()
    	:Messages::RNodeInterface(),
    	 iActivityAwaitingResponse(MeshMachine::KActivityNull)
    	 {}
    };

NONSHARABLE_CLASS(CIpSubConnectionProviderBase) : public CCoreSubConnectionProvider
/** IP subconnection provider base class

@internalTechnology
@released Since 9.4 */
    {
    friend class IPBaseSCprStates::TStoreAddressUpdate;
    friend class IPBaseSCprStates::TRejoinDataClient;
public:
    static const TUint32 KInvalidIapId = 0xFFFFFFFF;

protected:
    virtual Messages::RNodeInterface* NewClientInterfaceL(const Messages::TClientType& aClientType, TAny* aClientInfo = NULL);
    CIpSubConnectionProviderBase(ESock::CSubConnectionProviderFactoryBase& aFactory,
                                 const MeshMachine::TNodeActivityMap& aActivityMap);

protected:
	TUint32 iIapId;
    };

NONSHARABLE_CLASS(CIpDefaultBaseSubConnectionProvider) : public CIpSubConnectionProviderBase
/** Default IP subconnection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class CIpDefaultSubConnectionProviderFactory;

public:
    typedef CIpSubConnectionProviderFactory FactoryType;
	TBool ImsFlag();

protected:
    CIpDefaultBaseSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory,
                             const MeshMachine::TNodeActivityMap& aActivityMap);
    static CIpDefaultBaseSubConnectionProvider* NewL(ESock::CSubConnectionProviderFactoryBase& aFactory);

    virtual ~CIpDefaultBaseSubConnectionProvider();
    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
    };

//-=========================================================
//
// Activities
//
//-=========================================================
namespace IPDeftSCprBaseActivities
    {
    enum TIPDeftSCprBaseActivities
        {
        ECFActivityAddressUpdate 	= ESock::ECFActivityCustom,
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
    	ECFActivityCPFCreate		= ESock::ECFActivityCustom + 1,
    	ECFActivityReceiveWin       = ESock::ECFActivityCustom+2
#else
	    ECFActivityCPFCreate		= ESock::ECFActivityCustom + 1
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

		};

    DECLARE_ACTIVITY_MAP(ipscprbaseActivityMap)
    }

namespace IPDeftBaseSCprActivities
    {
    enum TIPDeftBaseSCprActivities
        {
        ECFActivityAddressUpdate 	= ESock::ECFActivityCustom,
        };

    DECLARE_ACTIVITY_MAP(ipdeftbasescprActivityMap)
    }


//-=========================================================
//
// States
//
//-=========================================================
namespace IPBaseSCprStates
{
typedef MeshMachine::TNodeContext<CIpSubConnectionProviderBase, SCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TAwaitingAddressUpdate, MeshMachine::TState<TContext>, NetStateMachine::MState, IPBaseSCprStates::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingAddressUpdate )

DECLARE_SMELEMENT_HEADER( TStoreAddressUpdate, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TStoreAddressUpdate )

DECLARE_SMELEMENT_HEADER( TRejoinDataClient, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TRejoinDataClient )

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
//State transition to store bearer type in provisionconfig and send window size to data clients in custom message
DECLARE_SMELEMENT_HEADER( TSendTransportNotificationToDataClients, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendTransportNotificationToDataClients )

//State to accept TTransportNotification from IPCPR.
DECLARE_SMELEMENT_HEADER( TAwaitingTransportNotification, MeshMachine::TState<TContext>, NetStateMachine::MState, IPBaseSCprStates::TContext )
 	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingTransportNotification )
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

}

namespace IPDeftBaseSCprBinderRequestActivity
{
const TInt KPermissionDenied = 1;

typedef MeshMachine::TNodeContext<CIpDefaultBaseSubConnectionProvider, SCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TNoTagOrUseExistingOrPermissionDenied, PRActivities::CCommsBinderActivity::TNoTagOrUseExisting, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrUseExistingOrPermissionDenied )
}

namespace IPDeftBaseSCprDataClientStartActivity
{
typedef MeshMachine::TNodeContext<CIpDefaultBaseSubConnectionProvider, SCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TGetParams, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TGetParams )
}

namespace IPDeftSCprStopActivity
{
typedef IPBaseSCprStates::TContext TContext;

DECLARE_SMELEMENT_HEADER( TStopYourFlows, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TStopYourFlows )

DECLARE_SMELEMENT_HEADER( TNoTagBackwardsOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagBackwardsOrProviderStopped )

DECLARE_SMELEMENT_HEADER( TNoTagOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrProviderStopped )



DECLARE_SERIALIZABLE_STATE(
	TNoTagOrProviderStoppedBlockedByStart,
	CoreNetStates::TActivityStartMutex,
	IPDeftSCprStopActivity::TNoTagOrProviderStopped
	)
}

#endif //SYMBIAN_IPDEFTBASESCPR_H
