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
// Support for sending provisioning information, retrieved from Agent, to the CFProtocol
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef SYMBIAN_AGENTMESSAGES_H
#define SYMBIAN_AGENTMESSAGES_H

#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/linkmessages.h>
#include <comms-infras/nifprvar.h>
#include <comms-infras/agentprconst.h>
#include <comms-infras/metadata.h>
#include <cdblen.h>
#include <nifman.h>

#include <comms-infras/nifprvar_internal.h>

class CAgentSubConnectionProvider;
class CCredentialsConfig;
class CAgentAdapter;



/**
Base class for notification handlers for Agents
*/
class CAgentNotificationHandler : public CBase
   {
   friend class CAgentSubConnectionProvider;

public:
    void Initialise (CAgentSubConnectionProvider* aAgentSCPr);

    // Upcalls from the agent
    IMPORT_C virtual void ConnectCompleteL ();
    IMPORT_C virtual TInt NotificationFromAgent (TAgentToFlowEventType aEvent, TAny* aInfo);
    IMPORT_C virtual void ServiceStarted ();

    // Notification received from the flow, bound for the agent
    IMPORT_C virtual TInt NotificationFromFlow (TFlowToAgentEventType aEvent);

protected:
    IMPORT_C CAgentNotificationHandler ();
    IMPORT_C void AppendExtensionL(const Meta::SMetaData* aExtension);
    IMPORT_C const Meta::SMetaData* GetExtension(const Meta::STypeId& aType) const;


    // Utility calls to the agent
    IMPORT_C TInt ReadPortName (TDes& aPortName);
    IMPORT_C TInt ReadIfParams (TDes& aIfParams);
    IMPORT_C TInt ReadIfNetworks (TDes& aIfNetworks);
    IMPORT_C TInt ReadExcessData (TDes8& aBuffer);
    IMPORT_C TInt ReadNifName (TDes& aNifName);
    IMPORT_C TInt QueryIsDialIn();
    IMPORT_C TInt NotificationToAgent (TFlowToAgentEventType aEvent, TAny* aInfo);

    // Utility calls for posting messages
    IMPORT_C TInt PostMessageToFlow(const Messages::TRuntimeCtxId& aSender, Messages::TSignatureBase& aMessage);
    IMPORT_C TInt AnonPostMessageToFlow(const Messages::TSignatureBase& aMessage);

private:
    static CAgentNotificationHandler* NewL();   // Only for use by the AgentSCPr
    CAgentSubConnectionProvider* iAgentSCPr;
    };



/**
Information to be provisioned to Agent SCPr's by MCPr's
*/
class CAgentProvisionInfo : public CBase, public Meta::SMetaData
	{
public:
    /**
    EPromptForReconnect - When the link is lost unexpectedly with the possibility to
    reconnect prompt the user to make the decision

    EDoNotAttemptReconnect - When the link is lost unexpectedly with the possibility to
    reconnect never attempt reconnection and Error the Cpr

    EAttemptReconnect - When the link is lost unexpectedly with the possibility to
    reconnect always perform the reconnection.
    */
    enum TAgentReconnectOption
        {
        EPromptForReconnect = -1,
        EDoNotAttemptReconnect = 0,
        EAttemptReconnect = 1,
        };

public:
    enum
    {
    EUid = 0x10281E07,
    ETypeId = 1,
    };

	IMPORT_C ~CAgentProvisionInfo();

	inline CAgentProvisionInfo()
	    : iAgentNotificationHandler(NULL),
		iAgentAdapter(NULL),
		iCredentials(NULL),
		iIapId(0),
		iBearerSet(0),
		iReconnectOption(EPromptForReconnect), // this is the same as the legacy behaviour
		iReconnectAttempts(0)
		{
		}

	inline const TDesC& AgentName () const;
	inline TInt SetAgentName (const TDesC& aAgentName);

	inline TUint32 IapId() const;
	inline void SetIapId(TUint32 aIap);

	inline TUint32 NetworkId() const;
	inline void SetNetworkId(TUint32 aNetId);

	inline TInt BearerSet() const;
	inline void SetBearerSet(TUint32 aBearerSet);

	inline CAgentNotificationHandler* AgentNotificationHandler() const;
	inline void SetAgentNotificationHandler (CAgentNotificationHandler* aAgentNotificationHandler);

	inline CCredentialsConfig* Credentials() const;
	inline void SetCredentials (CCredentialsConfig* aCredentials);

	inline CAgentAdapter* AgentAdapter() const;
	inline void SetAgentAdapter (CAgentAdapter* aAgentAdapter);

	inline CAgentProvisionInfo::TAgentReconnectOption ReconnectOption() const;
	inline void SetReconnectOption(CAgentProvisionInfo::TAgentReconnectOption aOption);

	inline TUint32 ReconnectAttempts() const;
	inline void SetReconnectAttempts(TUint32 aAttempts);

private:
	RBuf iAgentName;
	CAgentNotificationHandler* iAgentNotificationHandler;
	CAgentAdapter* iAgentAdapter;

	// iCredentials pointer contains information associated with an
	// AuthenticationResponse message sent to the Flow.
	CCredentialsConfig* iCredentials;
	TUint32 iIapId;
	TUint32 iNetworkId;
	TUint32 iBearerSet;
	TAgentReconnectOption iReconnectOption;
	TUint32 iReconnectAttempts;

public:
	DATA_VTABLE
	};


class CCredentialsConfig : public CBase, public Meta::SMetaData
/**

@internalTechnology
@released Since 9.4
*/
	{
public:
    enum
    {
    EUid = 0x10281E07,
    ETypeId = 2,
    };

    inline TInt Initialise(const TDesC& aUserName, const TDesC& aPassword, TInt aResult);
    static IMPORT_C CCredentialsConfig* NewLC(ESock::CCommsDatIapView* aIapView);
    inline ~CCredentialsConfig();

    //getters
    inline const TDesC& GetUserName() const;
    inline const TDesC& GetPassword() const;
    inline TInt  GetResult() const;

    //setters
    inline TInt  SetUserName(const TDesC& aUserName);
    inline void  SetUserName(HBufC* aUserName);
    inline TInt  SetPassword(const TDesC& aPassword);
    inline void  SetPassword(HBufC* aUserName);
    inline void  SetResult(TInt aResult);

protected:
    inline CCredentialsConfig();

    RBuf iUserName;
    RBuf iPassword;
    TInt iResult;
public:
	DATA_VTABLE
};

#include <comms-infras/agentmessages.inl>

#endif
// SYMBIAN_AGENTMESSAGES_H



