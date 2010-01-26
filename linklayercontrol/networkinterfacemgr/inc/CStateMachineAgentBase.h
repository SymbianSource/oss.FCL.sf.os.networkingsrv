/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file CStateMachineAgentBase.H
*/

#if !defined(__CSTATEMACHINEAGENT_H__)
#define __CSTATEMACHINEAGENT_H__

#include <comms-infras/cagentbase.h>
#include <comms-infras/cagentsmbase.h>
#include <comms-infras/ni_log.h>

class CStateMachineAgentBase : public CAgentBase, public MAgentNotify
/**
 * An agent that owns a state machine
 * @note This class is part of a compatibility layer for porting agent extensions (.agx) from v6.1
 *
 * @publishedPartner
 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
 */
	{
public:
	IMPORT_C virtual ~CStateMachineAgentBase();
protected:
	IMPORT_C void ConstructL();
	IMPORT_C CStateMachineAgentBase();
public:
	// implementation of the CNifAgentBase interface
	IMPORT_C virtual void Connect(TAgentConnectType aType);
	IMPORT_C virtual void Connect(TAgentConnectType aType, CStoreableOverrideSettings* aOverrideSettings);
	IMPORT_C virtual void CancelConnect();
	IMPORT_C virtual void Disconnect(TInt aReason);
	IMPORT_C virtual TInt GetExcessData(TDes8& aBuffer);
	IMPORT_C virtual TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo);
	IMPORT_C virtual void GetLastError(TInt& aError);

	// implementation of the MAgentNotify interface
	IMPORT_C virtual void PreventConnectionRetries();
	IMPORT_C virtual void ServiceStarted();
	IMPORT_C virtual void ConnectionComplete(TInt aProgress, TInt aError);
	IMPORT_C virtual void ConnectionComplete(TInt aError);
	IMPORT_C virtual void DisconnectComplete();
	IMPORT_C virtual void UpdateProgress(TInt aProgress, TInt aError);
	IMPORT_C virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo);
	IMPORT_C virtual TInt IncomingConnectionReceived();
	IMPORT_C virtual TBool IsReconnect() const;
	
protected:
	enum TNotifyOperation
		{
		EUndefined,
		EServiceStarted,
		EConnectComplete,
		EDisconnectComplete
		};

protected:
	virtual CAgentSMBase* CreateAgentSML(MAgentNotify& aObserver, CDialogProcessor* aDlgPrc, CCommsDbAccess& aDb, TCommDbConnectionDirection aDir) = 0;
	void CreateAndStartStateMachineL();

	void CallNotifyCb(TNotifyOperation aOperation, TInt aError);
	static TInt NotifyCbComplete(TAny* aThisPtr);
	void DoNotify();

protected:
	CAgentSMBase* iStateMachine;
	CAsyncCallBack* iNotifyCb;
	TNotifyOperation iNotifyCbOp;
	TInt iNotifyCbError;
	TInt iDisconnectReason;
	};

#endif

