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
// Inline methods for Agent PR Core classes
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef SYMBIAN_AGENTMESSAGES_INL
#define SYMBIAN_AGENTMESSAGES_INL


// ----------------------- Inlines for CAgentProvisionInfo -----------------------
/**
Set the agent name.
@note Only to be used by the MCPr
*/
TInt CAgentProvisionInfo::SetAgentName(const TDesC& aAgentName)
	{
	iAgentName.Close();
	return iAgentName.Create(aAgentName);
	}	


/**
Retrieve the agent name
*/
const TDesC& CAgentProvisionInfo::AgentName() const
	{
	return iAgentName;
	}

	
/**
Set the IAP Id
@note Only to be used by the MCPr
*/
inline void CAgentProvisionInfo::SetIapId(TUint32 aIap)
    {
    iIapId = aIap;
    }


/**
Retrieve the IAP Id
*/
inline TUint32 CAgentProvisionInfo::IapId() const
    {
    return iIapId;
    }

/**
Set the Network Id 
@note Only to be used by the MCPr
*/
inline void CAgentProvisionInfo::SetNetworkId(TUint32 aNetId)
    {
    iNetworkId = aNetId;
    }

/**
Retrieve the Network Id
*/
inline TUint32 CAgentProvisionInfo::NetworkId() const
    {
    return iNetworkId;
    }

/**
Set the BearerSet
@note Only to be used by the MCPr
*/
inline void CAgentProvisionInfo::SetBearerSet(TUint32 aBearerSet)
    {
    iBearerSet = aBearerSet;
    }


/**
Retrieve the BearerSet
*/
inline TInt CAgentProvisionInfo::BearerSet() const
    {
    return iBearerSet;
    }


/**
Set the Agent Notification Handler
@note Only to be used by the MCPr
*/
inline void CAgentProvisionInfo::SetAgentNotificationHandler(CAgentNotificationHandler* aAgentNotificationHandler)
	{
	iAgentNotificationHandler = aAgentNotificationHandler;
	}	


/**
Retrieve the Agent Notification Handler
*/
inline CAgentNotificationHandler* CAgentProvisionInfo::AgentNotificationHandler() const
	{
	return iAgentNotificationHandler;
	}


/**
Set the Credentials Config object pointer
@note Only to be used by the MCPr
*/
inline void CAgentProvisionInfo::SetCredentials(CCredentialsConfig* aCredentials)
   {
   iCredentials = aCredentials;
   }


/**
Retrieve the Credentials Config object pointer
*/
inline CCredentialsConfig* CAgentProvisionInfo::Credentials() const
   {
   return iCredentials;
   }
   

/**
Set the Agent Adapter object pointer
*/
inline void CAgentProvisionInfo::SetAgentAdapter(CAgentAdapter* aAgentAdapter)
    {
    iAgentAdapter = aAgentAdapter;
    }


/**
Retrieve the CAgent Adapter object pointer
*/
inline CAgentAdapter* CAgentProvisionInfo::AgentAdapter() const
    {
    return iAgentAdapter;
    }

/**
Retrieve the Reconnect Option
*/
inline CAgentProvisionInfo::TAgentReconnectOption CAgentProvisionInfo::ReconnectOption() const
    {
    return iReconnectOption;
    }

/**
Set the ReconnectOption
*/
inline void CAgentProvisionInfo::SetReconnectOption(CAgentProvisionInfo::TAgentReconnectOption aOption)
    {
    iReconnectOption = aOption;
    }


/**
Retrieve the Reconnection Attempts
*/
inline TUint32 CAgentProvisionInfo::ReconnectAttempts() const
    {
    return iReconnectAttempts;
    }

/**
Set the Reconnection Attempts
*/
inline void CAgentProvisionInfo::SetReconnectAttempts(TUint32 aAttempts)
    {
    iReconnectAttempts = aAttempts;
    }


// ----------------------- Inlines for CCredentialsConfig -----------------------
CCredentialsConfig::CCredentialsConfig()
  : iResult(KErrNone)
    {
    }


CCredentialsConfig::~CCredentialsConfig()
	{
	iUserName.Close();
	iPassword.Close();
	}


TInt CCredentialsConfig::Initialise(const TDesC& aUserName, const TDesC& aPassword, TInt aResult)
	{
	SetResult(aResult);
	
	TInt err = KErrNone;
	if ((err = SetUserName(aUserName)) != KErrNone)
		{
		return err;
		}
	return SetPassword(aPassword);
	}


void CCredentialsConfig::SetUserName(HBufC* aUserName)
    {
    iUserName.Close();
   	iUserName.Assign(aUserName);         
    }


TInt CCredentialsConfig::SetUserName(const TDesC& aUserName)
    {
    iUserName.Close();
   	return iUserName.Create(aUserName);         
    }


const TDesC& CCredentialsConfig::GetUserName() const
    {
    return iUserName;     
    }


void CCredentialsConfig::SetPassword(HBufC* aPassword)
    {
    iPassword.Close();
   	iPassword.Assign(aPassword);
    }


TInt CCredentialsConfig::SetPassword(const TDesC& aPassword)
    {
    iPassword.Close();
   	return iPassword.Create(aPassword);
    }


const TDesC& CCredentialsConfig::GetPassword() const
    {
    return iPassword;    
    }


void CCredentialsConfig::SetResult(TInt aResult)
    {
    iResult = aResult;
    }


TInt CCredentialsConfig::GetResult() const
    {
    return iResult;
    }

#endif
// SYMBIAN_AGENTMESSAGES_INL



