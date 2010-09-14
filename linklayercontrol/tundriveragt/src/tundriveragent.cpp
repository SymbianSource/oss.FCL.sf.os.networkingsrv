/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Source file for tundriver agent.
* 
*
*/

/**
 @file tundriveragent.cpp
 @internalTechnology
*/

#include "tundriveragent.h"
extern "C" EXPORT_C CNifAgentFactory* NewAgentFactoryL()
/**
Agent Factory
First ordinal export

@internalComponent
@return an object to class CTunAgentFactory,which Perform Agent initialisation.
*/
	{	
	return new(ELeave) CTunDriverAgentFactory;
	}

void CTunDriverAgentFactory::InstallL() 
/**
Performs a new Agent initialisation
*/
	{}
 
CNifAgentBase* CTunDriverAgentFactory::NewAgentL(const TDesC& /*aName*/)
/**
Creates a new TunDriverAgent to TunDriver

@param aName , name of the TunDriverAgent
@return a new instance of class TunDriverAgent
*/
	{
	return CTunDriverAgent::NewL();
	}

TInt CTunDriverAgentFactory::Info(TNifAgentInfo& aInfo, TInt /*aIndex*/) const
/**
Retrieves information about the Agent

@param aInfo,a reference of class TNifAgentInfo which contains information about TunDriverAgent
@return KErrNone if information retrieved successfully.
*/
	{
	aInfo.iName = KTunDriverAgentName;
	aInfo.iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	return KErrNone;
	}

CTunDriverAgent::CTunDriverAgent() :	iServiceStartedCallback(CActive::EPriorityStandard),
							iConnectCompleteCallback(CActive::EPriorityStandard), 
							iDisconnectCallback(CActive::EPriorityStandard)
/**
Default Constructor
*/
	{
	TCallBack serviceStartedCallback(ServiceStartedCb, this);
	iServiceStartedCallback.Set(serviceStartedCallback);
	
	TCallBack connectionCompleteCallback(ConnectCompleteCb, this);
	iConnectCompleteCallback.Set(connectionCompleteCallback);

	TCallBack disconnectionCompleteCallback(DisconnectCompleteCb, this);
	iDisconnectCallback.Set(disconnectionCompleteCallback);
	}
 
CTunDriverAgent::~CTunDriverAgent()
/**
Destructor
*/
	{
	iServiceStartedCallback.Cancel();
	iConnectCompleteCallback.Cancel();
	iDisconnectCallback.Cancel();
	}


CTunDriverAgent* CTunDriverAgent::NewL()
/**
Static NewL function constructing an object of class CTunDriverAgent

@return self,pointer to class TunDriverAgent,that owns a CAsyncCallback used to 
control the asynchronous ServiceStarted() and DisconnectComplete() call from the Agent to TunDriver. 
*/
	{
	CTunDriverAgent* self = new (ELeave) CTunDriverAgent();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

void CTunDriverAgent::ConstructL()
/**
2nd Phase Constructor
Calls CAgentBase::ConstructL()
construct the database and dialog processor
*/
	{
	CAgentBase::ConstructL();
	iCancelled = EFalse;
	iAgentProgress = ETunDriverAgtIdle;
	}

void CTunDriverAgent::Info(TNifAgentInfo& aInfo) const
/**
Information about this Agent

@param aInfo on return contains information about the agent
*/
	{
	aInfo.iName = KTunDriverAgentName;
	aInfo.iName.AppendFormat(_L("-AgentFactory[0x%08x]"), this);
	aInfo.iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

void CTunDriverAgent::Connect(TAgentConnectType aType)
/**
Connects this TunDriverAgent to TunDriver

@internalComponent  
@param aType, a variable of enum TAgentConnectType describing connection types.
*/
	{
	if (EAgentReconnect == aType)
		{
		iConnectCompleteCallback.CallBack();
		}
	else
		{
		iCancelled = EFalse;
		iServiceStartedCallback.CallBack();		
		}
	}

void CTunDriverAgent::Connect(TAgentConnectType aType, CStoreableOverrideSettings* /*aOverrideSettings*/)
/**
Connects this TunDriverAgent to TunDriver with store and retrieve commDB override sets to and from both
streams and buffers.

@param aType,a variable of enum TAgentConnectType describing connection types.
@param aOverrideSettings, a pointer to class CStoreableOverrideSettings which store CommDB overrides.
*/
	{
	Connect(aType);
	}

void CTunDriverAgent::CancelConnect()
/**
Cancels Connection of the TunDriverAgent to TunDriver 
*/
	{
	iServiceStartedCallback.Cancel();
	iConnectCompleteCallback.Cancel();
	iConnected = EFalse;
	iCancelled = ETrue;
	}

void CTunDriverAgent::Disconnect(TInt aReason)
/**
Disconnects from the TunDriverAgent to TunDriver with reason 

@param aReason, reason for disconnection to the agent.
*/
	{	   
	iLastErrorCode = aReason;
	iDisconnectCallback.CallBack();
	}

void CTunDriverAgent::ServiceStarted(TInt /*aError*/)
/**
ConnectionComplete from TunDriverAgent to TunDriver
This function is calling CallBack() function if connect is not cancelled

@param aError, error number for which service needs to be started.
*/
	{
	iAgentProgress = ETunDriverAgtConnecting;
	iNotify->AgentProgress(iAgentProgress, KErrNone);
	iNotify->ServiceStarted();
	
	/* if the connect has been cancelled during ServiceStarted() then we need to avoid 
	 * calling ConnectComplete() - done in this next callback - otherwise we get a panic */
	if (!iCancelled)
		iConnectCompleteCallback.CallBack();
	}

void CTunDriverAgent::ConnectionComplete(TInt aError)
/**
Second phase of completing connection of TunDriveragent to TunDriver

@param aError,error number for which connection is complete.
*/
    {
    if(aError==KErrNone)
	    {
        iAgentProgress = ETunDriverAgtConnected;
        iNotify->AgentProgress(ETunDriverAgtConnected, aError);
        iNotify->ConnectComplete(aError);
        iConnected = ETrue;
        return;
	    }
	iNotify->AgentProgress(iAgentProgress,aError);
	iNotify->ConnectComplete(aError);
	iConnected = EFalse;
    }

void CTunDriverAgent::DisconnectionComplete()
/**
Completes the disconnection of TunDriveragent from TunDriver
*/
	{
	
	if ( iDisconnecting )
	        {
            iNotify->AgentProgress(iAgentProgress, KErrNone);
	        }
	    else
	        {
            iAgentProgress = ETunDriverAgtDisconnected;
            iDisconnecting = ETrue;
	        iNotify->AgentProgress(iAgentProgress, KErrNone);
	        iNotify->DisconnectComplete();
	        iConnected = EFalse;
	        }
	}

TInt CTunDriverAgent::GetExcessData(TDes8& /*aBuffer*/)
/**
Gets excessData from TunDriveragent to TunDriver

@param aBuffer, variable containing the name of TunDriveragent
@return KErrNotSupported,error code if it does not get excess data from TunDriveragent.
*/
	{
	return KErrNotSupported;
	}


TInt CTunDriverAgent::Notification(TNifToAgentEventType /*aEvent*/, TAny* /*aInfo*/)
 /*
Establishes the Notification types from Nif to Agent for the TunDriveragent

@param aEvent,a variable of enum TNifToAgentEventType, which contains Notification types from TunDriver to TunDriverAgent
@param aInfo, information about the agent
@return KErrNotSupported,error code if the notification type does not exist.
*/
	{
	return KErrNone;		
	}

void CTunDriverAgent::GetLastError(TInt& aError)
/**
Gets the LastError
*/
	{
	aError = iLastErrorCode;
	}



TInt CTunDriverAgent::IncomingConnectionReceived()
/**
IncomingConnectionReceived

@return KErrNotSupported, error code if incoming connection is not received.
*/
	{
	return KErrNotSupported;
	}

TInt CTunDriverAgent::ServiceStartedCb(TAny* aThisPtr)
/**
 * Connection Callback static function 
 *
 */
	{
	CTunDriverAgent* self = (CTunDriverAgent*)aThisPtr;
	self->ServiceStarted(KErrNone);
	return KErrNone;
	}

TInt CTunDriverAgent::ConnectCompleteCb(TAny* aThisPtr)
/**
 * Second callback used during connection creation
 *
 */
    {
	CTunDriverAgent* self = (CTunDriverAgent*) aThisPtr;
	self->ConnectionComplete(KErrNone);
	return KErrNone;
    }

TInt CTunDriverAgent::DisconnectCompleteCb(TAny* aThisPtr)
/**
 * Disconnection callback static function
 *
 */
    {
	CTunDriverAgent* self = (CTunDriverAgent*) aThisPtr;
	self->DisconnectionComplete();
	return KErrNone;
    }
