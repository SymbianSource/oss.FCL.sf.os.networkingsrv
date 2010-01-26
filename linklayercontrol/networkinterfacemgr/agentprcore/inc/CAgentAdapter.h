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
//

/**
 @file
 @internalComponent
*/


#ifndef CAGENTADAPTER_H
#define CAGENTADAPTER_H


#include <comms-infras/nifagt.h>
#include <comms-infras/ss_nodemessages.h> //TStateChange
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/responsemsg.h>
#include <comms-infras/nifprvar_internal.h>

class CAgentSubConnectionProvider;
class CNifAgentBase;
class CAgentCprDataAccess;
class CAgentAdapterSessionNotifier;

namespace ESock
	{
	class MPlatsecApiExt;
	}

/**
This is the Agent Adapter for new style NIFs/SCPRs/CPRs
It is created and owned by Link Layer MCPRs and passed to the left in
the BindTo messages. It is controlled ONLY by the SCPR.  

@internalTechnology
@prototype
*/
NONSHARABLE_CLASS(CAgentAdapter) : public CBase, private MNifAgentNotify, 
	public ESock::MLinkCprServiceChangeNotificationApiExt
    {
    friend class CAgentCprDataAccess;
    friend class CAgentConnectionProvider;
    friend class CAgentAdapterSessionNotifier;
    
public:
    enum TAgentState
        {
        EDisconnected,
        EConnecting,
        EConnected,
        EDisconnecting,
        EReconnecting
        };

    static CAgentAdapter* NewL(CAgentSubConnectionProvider& aAgentScpr, const TDesC& aAgentName);
    ~CAgentAdapter();

    void ConnectAgent(TAgentConnectType aConnectType);
    void DisconnectAgent(TInt aReason);
    void PromptForReconnect();
    void Authenticate(TDes& aUsername, TDes& aPassword);
    void CancelAuthenticate();

    // Database Access Methods
    TInt ReadNifName(TDes8& aNifName);
    TInt ReadPortName(TDes8& aPortName);
    TInt ReadIfParams(TDes8& aIfParams);
    TInt ReadIfNetworks(TDes8& aIfNetworks);

    TInt ReadNifName(TDes16& aNifName);
    TInt ReadPortName(TDes16& aPortName);
    TInt ReadIfParams(TDes16& aIfParams);
    TInt ReadIfNetworks(TDes16& aIfNetworks);

    TInt ReadExcessData(TDes8& aBuffer);
    TInt QueryIsDialIn();

    TInt NotificationToAgent(TFlowToAgentEventType aEvent, TAny* aInfo);

    inline const TAgentState AgentState() const;
    
    TInt Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, ESock::MPlatsecApiExt* aPlatsecItf);
	void ClientAttachControl();
	
	// From MLinkCprServiceChangeNotificationApiExt Interface
	void RequestServiceChangeNotificationL(const Messages::TNodeId& aSender, ESock::RLegacyResponseMsg& aResponse);
	void CancelServiceChangeNotification(const Messages::TNodeId& aSender);

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	inline CNifAgentBase * Agent() const; 
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

protected:
    CAgentAdapter(CAgentSubConnectionProvider& aAgentScpr);
    inline CAgentSubConnectionProvider& AgentScpr() const;

private:
    // MNifAgentNotify Interface
    virtual void ConnectComplete(TInt aStatus);
    virtual void ReconnectComplete(TInt aStatus);
    virtual void AuthenticateComplete(TInt aStatus);
    virtual void DisconnectComplete();
    virtual void AgentProgress(TInt aStage, TInt aError);
    virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo = NULL);
    virtual void AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);
    virtual void ServiceStarted();

    // MNifAgentNotify Interface (Empty implementations)
    virtual TInt IncomingConnectionReceived();
    virtual void ServiceClosed();
    virtual void AgentProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);

    CNifFactory* FindOrCreateAgentFactoryL(const TDesC& aFilename);
    void CreateAgentL(const TDesC& aAgentName);
    
private:
    CAgentSubConnectionProvider& iAgentScpr;
    CNifAgentBase* iAgent;
    CNifAgentFactory* iFactory;
    TAgentState iAgentState; 
    TAgentConnectType iAgentConnectType;
    Elements::TStateChange iLastProgress;
	RPointerArray<CAgentAdapterSessionNotifier> iAgentAdapterSessionNotifiers;
    };


CAgentSubConnectionProvider& CAgentAdapter::AgentScpr() const
    {
    return iAgentScpr;
    }


const CAgentAdapter::TAgentState CAgentAdapter::AgentState() const
    {
    return iAgentState;
    }
    
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/**
 * Returns the agent 
 * @param none
 * @return Interface of the Nifman/Agent
**/
CNifAgentBase *CAgentAdapter::Agent() const
	{
   	return iAgent;
 	}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

/**
 * This class is an Adapter to allow CAgentBase to Notify for
 * ServiceChangeNotification post-399.
 * 
 * @internalTechnology
 * @prototype */
NONSHARABLE_CLASS(CAgentAdapterSessionNotifier) : public CBase,
	public MAgentSessionNotify
    {
private:
	// Phase #1 Constructor
	CAgentAdapterSessionNotifier (CAgentAdapter* aCreator,
			const Messages::TNodeId& aSender);
	
	// Phase #2 Constructor
	void ConstructL (ESock::RLegacyResponseMsg& aResponseMsg);
public:
	// 2-Phase Static Constructor
	static CAgentAdapterSessionNotifier* NewL(CAgentAdapter* aCreator,
			const Messages::TNodeId& aSender,
			ESock::RLegacyResponseMsg& aResponseMsg);
	
	// 2-Phase Static Constructor. In this case it builds an "empty" Notifier.
	static CAgentAdapterSessionNotifier* NewL(CAgentAdapter* aCreator,
				const Messages::TNodeId& aSender);
	
	// Virtual Destructor
	virtual ~CAgentAdapterSessionNotifier();

	// Operator to allow the usage of Find(...) inside a R(Pointer)Array container
	virtual TBool operator== (const CAgentAdapterSessionNotifier& aOtherInstanceOfThisClass) const;
	
	// Return an Id that identify this Agent Session
	virtual const Messages::TNodeId& NodeId() const;

	// Implementation of MAgentSessionNotify interface
    virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType);
    
    virtual void CancelServiceChangeNotification(TInt aReason = KErrCancel);
    
private:
	/** This identify "who" asked for being notified. */
	Messages::TNodeId iSender;
	/** The RResponseMsg to use to answer(notify) the request. */
	ESock::RLegacyResponseMsg* iResponseMsg;
	/** The AgentAdapter which has created this object.
	 * This Pointer allows this CAgentAdapterSessionNotifier to remove
	 * itself from the Array in the Creator (Queue of S.C. Notifications) */
	CAgentAdapter* iCreator;
    };

#endif
// CAGENTADAPTER_H

