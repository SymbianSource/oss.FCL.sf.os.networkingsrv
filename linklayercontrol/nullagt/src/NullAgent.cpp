// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file NullAgent.CPP 
*/

#include "NullAgent.h"
#include <d32comm.h> // Conversion of link speed constants
#include <cdbcols.h> // CommDB access

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <networking/cfbearers.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

/**
KNullAgentPanic indicates NullAgent

@internalComponent
*/

extern "C" EXPORT_C CNifAgentFactory* NewAgentFactoryL()
/**
Agent Factory
First ordinal export

@internalComponent
@return an object to class CNullAgentFactory,which Perform Agent initialisation.
*/
	{	
	return new(ELeave) CNullAgentFactory;
	}

void CNullAgentFactory::InstallL() 
/**
Performs a new Agent initialisation
*/
	{}
 
CNifAgentBase* CNullAgentFactory::NewAgentL(const TDesC& /*aName*/)
/**
Creates a new NullAgent to Nifman

@param aName , name of the NullAgent
@return a new instance of class CNullAgent
*/
	{
	return CNullAgent::NewL();
	}

TInt CNullAgentFactory::Info(TNifAgentInfo& aInfo, TInt /*aIndex*/) const
/**
Retrieves information about the Agent

@param aInfo,a reference of class TNifAgentInfo which contains information about NullAgent
@return KErrNone if information retrieved successfully.
*/
	{
	aInfo.iName = KNullAgentName;
	aInfo.iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	return KErrNone;
	}

CNullAgent::CNullAgent() :	iServiceStartedCallback(CActive::EPriorityStandard),
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

	iCancelled = EFalse;
	}
 
CNullAgent::~CNullAgent()
/**
Destructor
*/
	{
	iServiceStartedCallback.Cancel();
	iConnectCompleteCallback.Cancel();
	iDisconnectCallback.Cancel();
	}


CNullAgent* CNullAgent::NewL()
/**
Static NewL function constructing an object of class CNullAgent

@return self,pointer to class CNullAgent,that owns a CAsyncCallback used to 
control the asynchronous ServiceStarted() and DisconnectComplete() call from the Agent to Nifman. 
*/
	{
	CNullAgent* self = new (ELeave) CNullAgent();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); // self
	return self;
	}

void CNullAgent::ConstructL()
/**
2nd Phase Constructor
Calls CAgentBase::ConstructL()
construct the database and dialog processor
*/
	{
	CAgentBase::ConstructL();
	iConnected = EFalse;
	}

void CNullAgent::Info(TNifAgentInfo& aInfo) const
/**
Information about this Agent

@param aInfo on return contains information about the agent
*/
	{
	aInfo.iName = KNullAgentName;
	aInfo.iName.AppendFormat(_L("-AgentFactory[0x%08x]"), this);
	aInfo.iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

void CNullAgent::Connect(TAgentConnectType aType)
/**
Connects this NullAgent to Nifman

@internalComponent  
@param aType, a variable of enum TAgentConnectType describing connection types.
*/
	{
	if (EAgentReconnect == aType)
		{
		// skip forward to connect complete (service started would panic as we already have an interface etc)
		iConnectCompleteCallback.CallBack();
		}
	else
		{
		iCancelled = EFalse;
		iServiceStartedCallback.CallBack();		
		}
	}

void CNullAgent::Connect(TAgentConnectType aType, CStoreableOverrideSettings* /*aOverrideSettings*/)
/**
Connects this NullAgent to Nifman with store and retrieve commDB override sets to and from both
streams and buffers.

@param aType,a variable of enum TAgentConnectType describing connection types.
@param aOverrideSettings, a pointer to class CStoreableOverrideSettings which store CommDB overrides.
*/
	{
	Connect(aType);
	}

void CNullAgent::CancelConnect()
/**
Cancels Connection of the NullAgent to Nifman 
*/
	{
	iServiceStartedCallback.Cancel();
	iConnectCompleteCallback.Cancel();
	iConnected = EFalse;
	iCancelled = ETrue;
	}

void CNullAgent::Disconnect(TInt /*aReason*/)
/**
Disconnects from the NullAgent to Nifman with reason 

@param aReason, reason for disconnection to the agent.
*/
	{
	iDisconnectCallback.CallBack();
	}

void CNullAgent::ServiceStarted(TInt /*aError*/)
/**
ConnectionComplete from NullAgent to Nifman
This function is calling CallBack() function if connect is not cancelled

@param aError, error number for which service needs to be started.
*/
	{
	__ASSERT_DEBUG(iNotify, NullAgentPanic(NullAgent::ENullNifmanNotifyPointer));

	iNotify->AgentProgress(ENullAgtConnecting, KErrNone);
	iNotify->ServiceStarted();
	
	/* if the connect has been cancelled during ServiceStarted() then we need to avoid 
	 * calling ConnectComplete() - done in this next callback - otherwise we get a panic */
	if (!iCancelled)
		iConnectCompleteCallback.CallBack();
	}

void CNullAgent::ConnectionComplete(TInt aError)
/**
Second phase of completing connection of NullAgent to Nifman

@param aError,error number for which connection is complete.
*/
{
	__ASSERT_DEBUG(iNotify, NullAgentPanic(NullAgent::ENullNifmanNotifyPointer));

	iNotify->AgentProgress(ENullAgtConnected, KErrNone);
	iNotify->ConnectComplete(aError);
	iConnected = ETrue;
}

void CNullAgent::DisconnectionComplete()
/**
Completes the disconnection of NullAgent from Nifman
*/
	{
	__ASSERT_DEBUG(iNotify, NullAgentPanic(NullAgent::ENullNifmanNotifyPointer));

	iNotify->AgentProgress(ENullAgtDisconnected, KErrNone);
	iConnected = EFalse;
	iNotify->DisconnectComplete();
}

TInt CNullAgent::GetExcessData(TDes8& /*aBuffer*/)
/**
Gets excessData from NullAgent to Nifman

@param aBuffer, variable containing the name of NullAgent
@return KErrNotSupported,error code if it does not get excess data from NullAgent.
*/
	{
	return KErrNotSupported;
	}

TInt CNullAgent::Notification(TNifToAgentEventType aEvent, TAny* /*aInfo*/)
/**
Establishes the Notification types from Nif to Agent for the agent

@param aEvent,a variable of enum TNifToAgentEventType, which contains Notification types from Nif to Agent
@param aInfo, information about the agent
@return KErrNotSupported,error code if the notification type does not exist.
*/
	{
	TUint speedFromCommDb(0);
	switch (aEvent)
		{
		// Respond to Raw IP NIF events
		// N.B. This targets RawIP NIF in a test setup exclusively.
		case (ENifToAgentEventTsyConfig) : 
			iNotify->Notification(EAgentToNifEventTsyConfig, reinterpret_cast<TAny*>(&iTsyConfig));
			break;
				
		case (ENifToAgentEventTsyConnectionSpeed) : 
			// Reply with a static value from CommDB.
			speedFromCommDb = CommDbModemBearerRate();
			iNotify->Notification(EAgentToNifEventTsyConnectionSpeed, reinterpret_cast<TAny*>(&speedFromCommDb));
			break;				
		default : 
			return KErrNotSupported;			
		}
	return KErrNone;		
	}

void CNullAgent::GetLastError(TInt& aError)
/**
Gets the LastError
*/
	{
	aError = KErrNone;
	}

TInt CNullAgent::IncomingConnectionReceived()
/**
IncomingConnectionReceived

@return KErrNotSupported, error code if incoming connection is not received.
*/
	{
	return KErrNotSupported;
	}

TInt CNullAgent::ServiceStartedCb(TAny* aThisPtr)
/**
 * Connection Callback static function 
 *
 */
	{
	__ASSERT_DEBUG( aThisPtr, NullAgentPanic(NullAgent::ENullTAnyPointer));

	CNullAgent* self = (CNullAgent*)aThisPtr;
	self->ServiceStarted(KErrNone);
	return KErrNone;
	}

TInt CNullAgent::ConnectCompleteCb(TAny* aThisPtr)
/**
 * Second callback used during connection creation
 *
 */
{
	__ASSERT_DEBUG(aThisPtr, NullAgentPanic(NullAgent::ENullTAnyPointer));

	CNullAgent* self = (CNullAgent*) aThisPtr;
	self->ConnectionComplete(KErrNone);
	return KErrNone;
}

TInt CNullAgent::DisconnectCompleteCb(TAny* aThisPtr)
/**
 * Disconnection callback static function
 *
 */
{
	__ASSERT_DEBUG(aThisPtr, NullAgentPanic(NullAgent::ENullTAnyPointer));

	CNullAgent* self = (CNullAgent*) aThisPtr;
	self->DisconnectionComplete();
	return KErrNone;
}

/**
Returns the static link rate from CommDB Modem Bearer table

@return link rate 
*/ 
TUint CNullAgent::CommDbModemBearerRate()
	{
	static const TUint KDefaultSpeed(115200);// Default value to use in case of a failure.
	TUint32 speedMetric(0);
	
	TBuf<KCommsDbSvrMaxColumnNameLength> rateColName = TPtrC(MODEM_BEARER);
	rateColName.Append(TChar(KSlashChar));
	rateColName.Append(TPtrC(MODEM_RATE));
	
	TInt err = ReadInt(rateColName, speedMetric);
	if(KErrNone != err)
		{
		return KDefaultSpeed; 
		}
	
	switch(speedMetric)
		{
		case EBps50:
			return 50;
		case EBps75:
			return 75;
		case EBps110:
			return 110;
		case EBps134:
			return 134;
		case EBps150:
			return 150;
		case EBps300:
			return 300;
		case EBps600:
			return 600;
		case EBps1200:
			return 1200;
		case EBps1800:
			return 1800;
		case EBps2000:
			return 2000;
		case EBps2400:
			return 2400;
		case EBps3600:
			return 3600;
		case EBps4800:
			return 4800;
		case EBps7200:
			return 7200;
		case EBps9600:
			return 9600;
		case EBps19200:
			return 19200;
		case EBps38400:
			return 38400;
		case EBps57600:
			return 57600;
		case EBps115200:
			return 115200;
		case EBps230400:
			return 230400;
		case EBps460800:
			return 460800;
		case EBps576000:
			return 576000;
		case EBps1152000:
			return 1152000;
		case EBps4000000:
			return 4000000;
		default:
			break;
		}
	return KDefaultSpeed;
	}
		
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
Returns the value of the ethernet as the bearer
@return Ethernet bearer
*/
TUint32 CNullAgent::GetBearerInfo() const
	{
	return KEthernetBearer;
	}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
				
	







//void CNullAgent::MDPOLoginComplete(TInt /*aError*/) 
//	{}

//void CNullAgent::MDPOReadPctComplete(TInt /*aError*/) 
//	{}

//void CNullAgent::MDPODestroyPctComplete(TInt /*aError*/) 
//	{}

//void CNullAgent::MDPOQoSWarningComplete(TInt /*aError*/, TBool /*aResponse*/) 
//	{}
