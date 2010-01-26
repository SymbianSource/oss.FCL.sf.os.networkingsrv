/**
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header for Base classes in States and State Machines
* Base classes for agents that are implemented using states and state machines.
* This file contains the APIs required to implement a more advanced agent for Symbian OS.
* 
*
*/



/**
 @file CAgentSMBase.h
 @publishedPartner
 @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
*/

#ifndef __CAGENTSMBASE_H__
#define __CAGENTSMBASE_H__

#include <comms-infras/nifprvar.h>
#include <e32base.h>
#include <comms-infras/dialogprocessor.h>


class MAgentNotify
/**
 * Notification of events from the CAgentSMBase to the CStateMachineAgent
 * @note This class is part of a compatibility layer for porting agent extensions (.agx) from v6.1
 * @note This class was previously called MAgentObserver in v6.1
 *
 * @publishedPartner
 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
 */
	{
public:
	/**
	 *
	 */
	virtual void PreventConnectionRetries() = 0;

	/**
	 *
	 */
	virtual void ServiceStarted() = 0;

	/**
	 *
	 */
	virtual void ConnectionComplete(TInt aProgress, TInt aError) = 0;

	/**
	 *
	 */
	virtual void ConnectionComplete(TInt aError) = 0;

	/**
	 *
	 */
	virtual void DisconnectComplete() = 0;

	/**
	 *
	 */
	virtual void UpdateProgress(TInt aProgress, TInt aError) = 0;

	/**
	 *
	 */
	virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo) = 0;

	/**
	 *
	 */
	virtual TInt IncomingConnectionReceived() = 0;
	};


class CCommsDbAccess;
class CDialogProcessor;

class MAgentStateMachineEnv : public MAgentNotify
/**
 * Interface from individual agent states to agent state machine
 * @note This class is part of a compatibility layer for porting agent extensions (.agx) from v6.1
 * @ingroup Agent
 *
 * @publishedPartner
 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
 */
	{
public:

	/**
	 *
	 */
	virtual CDialogProcessor* DlgPrc() = 0;

	/**
	 *
	 */
	virtual CCommsDbAccess* Db() =0;

	/**
	 *
	 */
	virtual void CompleteState(TInt aError) = 0;

	/**
	 *
	 */
	virtual TBool IsReconnect() const =0;

	/**
	 *
	 */
	virtual TBool CallBack() const =0;
	};

class CAgentStateBase : public CActive
/**
 * Base class for individual states within an agent
 * @note This class is part of a compatibility layer for porting agent extensions (.agx) from v6.1
 *
 * @publishedPartner
 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
 */
	{
public:
	IMPORT_C CAgentStateBase(MAgentStateMachineEnv& aSMObserver);
	IMPORT_C virtual ~CAgentStateBase();

	/**
	 * Start the processing for this state
	 */
	virtual void StartState() = 0;

	/**
	 * Create and return an instance of the next state object
	 */
	virtual CAgentStateBase* NextStateL(TBool aContinue) = 0;
protected:
	IMPORT_C void JumpToRunl(TInt aError);
protected:
	MAgentStateMachineEnv* iSMObserver;
	};

class CAgentSMBase : public CActive, public MAgentStateMachineEnv
/**
 * Base class for agent state machine
 * @note Typically this class is owned by CStateMachineAgentBase
 * @note This class is part of a compatibility layer for porting agent extensions (.agx) from v6.1
 *
 * @publishedPartner
 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
 */
	{
public:
	enum TSMContinueConnectType
	/**
	 * The action to be taken by the connection code
	 * @publishedPartner
	 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
	 */
		{
		ECallBack,
		EReconnect,
		EDisconnect
		};
	enum TSMPhase
	/**
	 * The current state of the agent state machine
	 * @publishedPartner
	 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
	 */
		{
		EConnecting,
		EConnected,
		EDisconnecting,
		EDisconnected
		};
public:
	IMPORT_C CAgentSMBase(MAgentNotify& aControllerObserver, CDialogProcessor* aDlgPrc, CCommsDbAccess& aDbAccess);
	IMPORT_C virtual ~CAgentSMBase();

	// Downward calls from NifMan to state machine/states
	void StartConnect();
	void CancelConnect();
	IMPORT_C virtual void ConnectionContinuation(TSMContinueConnectType aConnectionAction);

	/**
	 * Return any excess data was received during connection setup
	 * @note For example, after a script has run, there may be additional data received that is intended for the nif, which will retrieve it via this method
	 * @param aBuffer On return, the buffer contains the excess data from the agent
	 * @returns KErrNone, if successful; otherwise, one of the standard Symbian OS error codes
	 */
	virtual TInt GetExcessData(TDes8& aBuffer) = 0;

	/**
	 * Notification of an event from the nif
	 * @param aEvent The type of event that occured
	 * @param aInfo Any data associated with the event
 	 * @returns KErrNone if successful, otherwise one of the system-wide error codes
	 */
	virtual TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo) = 0;

	IMPORT_C virtual void GetLastError(TInt& aError);

	// Upward calls from states/state machine to NifMan. Inherited from MAgentNotify.
	IMPORT_C virtual void PreventConnectionRetries();
	IMPORT_C virtual void ServiceStarted();
	IMPORT_C virtual void ConnectionComplete(TInt aProgress,TInt aError);
	IMPORT_C virtual void ConnectionComplete(TInt aError);
	IMPORT_C virtual void DisconnectComplete();
	IMPORT_C virtual void UpdateProgress(TInt aProgress,TInt aError);
	IMPORT_C virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo);
	IMPORT_C virtual TInt IncomingConnectionReceived();

	// MAgentStateMachineEnv derivation
	IMPORT_C virtual CDialogProcessor* DlgPrc();
	IMPORT_C virtual CCommsDbAccess* Db();
	IMPORT_C virtual void CompleteState(TInt aError);
	virtual inline TBool IsReconnect() const;
	virtual inline TBool CallBack() const;

private:
	IMPORT_C virtual void RunL();
	IMPORT_C virtual void DoCancel();
	void ProcessState();
	void ConnectCompleteReset();
protected:
	CAgentStateBase* iState;
	MAgentNotify* iControllerObserver;
	CDialogProcessor* iDlgPrc;
	CCommsDbAccess* iDb;
	TBool iContinueConnection;
	TBool iIsReconnect;
	TBool iCallBack;
	TSMPhase iSMPhase;
	};

#include <comms-infras/cagentsmbase.inl>

#endif


