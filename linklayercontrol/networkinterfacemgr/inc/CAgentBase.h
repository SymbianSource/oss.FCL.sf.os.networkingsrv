// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Base classes for agents that require commdb access.
// This file contains the APIs required to implement an agent for Symbian OS that accesses commdb.
// 
//

/**
 @file
 @publishedPartner
 @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
*/

#if !defined(__CAGENTBASE_H__)
#define __CAGENTBASE_H__

#include <comms-infras/nifagt.h>
#include <comms-infras/dbaccess.h>
#include <comms-infras/dialogprocessor.h>
#include <comms-infras/cnetworkcontrollerbase.h>

class CAgentDialogProcessor;

/**
 * Base class for agents that access commdb and use overrides
 *
 * @publishedPartner
 * @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
 */
class CAgentBase : public CNifAgentBase, public MServiceChangeObserver, public MDialogProcessorObserver, public MNetworkControllerObserver
	{
	friend class CAgentDialogProcessor;

public:
	IMPORT_C virtual ~CAgentBase();
protected:
	IMPORT_C void ConstructL();
	IMPORT_C CAgentBase();
public:

	// implementation of the CNifAgentBase interface
	IMPORT_C virtual void Reconnect();
	IMPORT_C virtual void CancelReconnect();
	IMPORT_C virtual void Authenticate(TDes& aUsername, TDes& aPassword);
	IMPORT_C virtual void CancelAuthenticate();
	IMPORT_C virtual void RequestNotificationOfServiceChangeL(MAgentSessionNotify* aSession);
	IMPORT_C virtual void CancelRequestNotificationOfServiceChange(MAgentSessionNotify* aSession);
	IMPORT_C virtual void SetConnectionSettingsL(const TConnectionSettings& aSettings);
	IMPORT_C virtual TConnectionSettings& ConnectionSettingsL();
	IMPORT_C virtual void SetOverridesL(CStoreableOverrideSettings* aOverrideSettings);
	IMPORT_C virtual CStoreableOverrideSettings* OverridesL();
	IMPORT_C virtual TBool IsActive() const;

	// implementation of the MDialogProcessorObserver interface
	IMPORT_C virtual void MDPOAuthenticateComplete(TInt aError);

	// implementation of the MServiceChangeObserver interface
	IMPORT_C virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType);

	// implementation of the MNetworkControllerObserver interface
	IMPORT_C virtual void SelectComplete(const TDesC& aName);
	IMPORT_C virtual void SelectComplete(TInt aError);
	IMPORT_C virtual void ReconnectComplete(TInt aError);


protected:
	IMPORT_C virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual TInt DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage);
	IMPORT_C virtual HBufC* DoReadLongDesLC(const TDesC& aField,const RMessagePtr2* aMessage);

	IMPORT_C virtual TInt DoCheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	IMPORT_C virtual TInt DoCheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage );

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
public:
	IMPORT_C virtual TUint32 GetBearerInfo() const;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW



private:
	static TInt AuthenticateCb(TAny* aThisPtr);
	inline CDialogProcessor* DialogProcessor() { return iDlgPrc; };

protected:
	CCommsDbAccess* iDatabase;				//< for commdb access
	CDialogProcessor* iDlgPrc;				//< pointer to the dialog processor for prompting the user
	CStoreableOverrideSettings* iOverrides;	//< application specified overrides for the settings in commdb (either these or TConnectionPreferences should be set, I think depending on which API the client is using (RConnection/RGenericAgent)??)
	RPointerArray<MAgentSessionNotify> iServiceChangeNotification; //< pointers to CNifSessions for notification of service change
	CAsyncCallBack* iAuthenticateCallback;	//< [*** to do ***]
	TInt iAuthenticateError;				//< the error code, if any, generated during authentication
	TConnectionSettings iSettings;			//< the settings for this connection
	};

#endif

