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
// IP Connection Provider state declarations.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef IPCPR_STATES_H_INCLUDED
#define IPCPR_STATES_H_INCLUDED

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/ss_activities.h>
#include <comms-infras/corecpr.h>
#include <comms-infras/corecprstates.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace TcpAdaptiveReceiveWindow
{
	const TInt32 KIpBearerInfoUid = 0x2001F96E;
	const TInt KIpBearerInfoParameterType = 1;

	const TUint32 KBearerInfo = 1;
}

using namespace TcpAdaptiveReceiveWindow;

class XBearerInfo : public XParameterSet
{
	public:
	
	enum
	{
		EUid= KIpBearerInfoUid,
		EId = KIpBearerInfoParameterType,
	};
	IMPORT_C static XBearerInfo* NewL(RParameterFamily aFamily, RParameterFamily::TParameterSetType aType);
	IMPORT_C static XBearerInfo* NewL();
	inline TUint GetBearerType()
		{
		return iBearerType;
		}
	inline void SetBearerType(TUint aBearerType)
		{
		iBearerType = aBearerType;
		}
		
	private:	
	TUint iBearerType;
	
	public:
	DATA_VTABLE
};

#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

class CIPConnectionProvider;

namespace IpCprStates
{
typedef MeshMachine::TNodeContext<CIPConnectionProvider, CprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TAwaitingPolicyParams, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingPolicyParams )

DECLARE_SMELEMENT_HEADER( TSendPolicyParams, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendPolicyParams)
// -------- Capability Checks for Starting/Stopping a connection --------

#ifndef SYMBIAN_NETWORKING_UPS

DECLARE_SMELEMENT_HEADER( TCheckStartCapabilities, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TCheckStartCapabilities)

#endif //SYMBIAN_NETWORKING_UPS

DECLARE_SMELEMENT_HEADER( TCheckStopCapabilities, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)

	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TCheckStopCapabilities)

// -------- Provider status change handling for subconn events --------

DECLARE_SMELEMENT_HEADER( TAwaitingSubConnDataTransferred, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingSubConnDataTransferred)

DECLARE_SMELEMENT_HEADER( TProcessSubConnDataTransferred, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProcessSubConnDataTransferred)




DECLARE_SMELEMENT_HEADER( TSendInitialSubConnectionOpenedEvent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendInitialSubConnectionOpenedEvent)

DECLARE_SMELEMENT_HEADER( TSendSubsequentSubConnectionOpenedEvent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendSubsequentSubConnectionOpenedEvent)

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*State transition. Will be triggered when there TStarted message is received from the lower layers.
This transition will trigger the request/response mechanism to retrieve bearer type from lower layer.
New Parameter set is added to Parameter bundle and passed to service Provider from IPCPR.
Agent CPR/PDP CPR will update the parameter with the bearer type and send it back to IPCPR.
IPCPR will extract the TCP receive window from the lookup table at NetMCPR after it receives the 
bearer type and send it to IPSCPR(ie. the dataclients)

TInitialiseParamsAndSendToSelf will initialise the parameter bundle at IPCPR and send to self.
SendToSelf will start a mechanism ato send the bundle to the service providers below.

The message is recd. ate the Agent/PDP CPR and the bundle is updated with the bearer type.
Respons with retrieved params sends a response to the IPCPR.

						 ---------------------------------------
						|				IPCPR					|
						|										|
						 ---------------------------------------
						/\				    \/        		/\
						|					|				|
						|1.TStarted			|2.TInitialise	|3.TUpdateBundle
						|					|ParamsAnd		|AndRespondWith
						|					|SendToSelf		|RetrievedParams
						 ---------------------------------------
						|				Agent/PDP				|
						|		     		CPR					|
						 ---------------------------------------

*/

//State Transition to Update NetMCPR after bearer type is obtained
DECLARE_SMELEMENT_HEADER(TUpdateProvisionConfigAtStartup, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TUpdateProvisionConfigAtStartup)

//State Transition which will trigger message to data clients. 
DECLARE_SMELEMENT_HEADER(TSendTransportNotificationToDataClients, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendTransportNotificationToDataClients)

//State transition to update bearer info after it is recd in modulation change
DECLARE_SMELEMENT_HEADER(TUpdateProvisionConfigAtModulation, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TUpdateProvisionConfigAtModulation)

//State Transition to initialise parameter bundle and send to self
DECLARE_SMELEMENT_HEADER(TInitialiseParams, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TInitialiseParams)

DECLARE_SMELEMENT_HEADER(TSendParamsToSelf, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendParamsToSelf)

DECLARE_AGGREGATED_TRANSITION2(
   TInitialiseParamsAndSendToSelf,
   TInitialiseParams,
   TSendParamsToSelf
   )
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

} // IpCprStates

#endif
// IPCPR_STATES_H_INCLUDED
