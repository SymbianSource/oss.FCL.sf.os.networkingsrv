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
* RStartServer Client side header
* Declares the R class to start server process,
* the class covers EPOC platform & emulator diferences
* 
*
*/



/**
 @file CS_DeamonControl.h
 @internalComponent
*/

#if !defined (__CS_DEAMONCONTROL_H__)
#define __CS_DEAMONCONTROL_H__

#include <e32base.h>
#include "CS_Daemon.h"
#include "CS_DaemonConst.h"
#include "NifConfigurationControl.h"
#include "es_enum.h"
//#include "ni_log.h"
#include <comms-infras\commsdebugutility.h>

__FLOG_STMT(_LIT8(KLogTagCsDaemon, "CSDAEMON");)
__FLOG_STMT(_LIT8(KLogSubSysNifman, "NIFMAN");)

class CStartServer;
class CAsynchDaemonCancel;
class CNifDaemonProgress;
/**
 Daemon configuration, uses RConfigDaemon generic config daemon client API 
 to configure network layer
 @internalTechnology
 @version 0.03
 @date	26/05/2004
**/
class CNifDaemonConfiguration : public CNifConfigurationIf
	{
   	friend class CNifConfigurationControl;

protected:
   	CNifDaemonConfiguration(MNifIfNotify& aNifIfNotify);

public:
  	static CNifDaemonConfiguration* NewL(TAny* aMNifIfNotify);
   	virtual ~CNifDaemonConfiguration();

   	virtual void ConfigureNetworkL();
   	virtual void LinkLayerDown();
   	virtual void LinkLayerUp();
   	virtual void Deregister(TInt aCause);
   	virtual void SendIoctlMessageL(const RMessage2& aMessage);
   	virtual void CancelControl();
   	virtual void AsyncDelete();
      virtual void EventNotification(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource);

   	void DoOnDaemonProgress(TInt aStage, TInt aError);
   	void DoOnGenericProgress(TInt aStage, TInt aError);

protected:
   	virtual void DoCancel();
   	virtual void RunL();
   	virtual TInt RunError(TInt aError);

  	void ConstructL();

protected:
   	HBufC8* iIoBuf;
   	TPtr8 iIoPtr;
   	RConfigDaemon iConfigDaemon;
   	TConnectionInfoBuf iConnectionInfoBuf;
   	/**
    ipStartServer - != NULL when the daemon is being started
    @internalComponent
    @version 0.01
   	**/
   	CStartServer* ipStartServer;
   	/**
    iMessage - to keep the original reguest when forwarding the request for further processing
    @internalComponent
    @version 0.01
   	**/
   	RMessage2 iMessage;
   	CAsynchDaemonCancel* iAsynchDaemonCancel;
   	TBool iDeleteOnCompletion;
   	/** Object used to keep continously get progress notifications from the daemon. */
   	CNifDaemonProgress* iDaemonProgress;
   	/** Stores deregistration status. Points to iDeregActionStatus. */
   	TPtr8 iDesDeregActionStatus;
   	/** Used to store deregistration status. */
   	TInt iDeregActionStatus;
   	/** Used to store last generic progress stage. */
   	TInt iLastGenericProgressStage;
   	/** If ETrue, there is a deregistration event queued. */
   	TBool iDeregisterOnCompletionOfRequest;
   	/** Used to cache the cause code associated with the deregistration request. */ 
   	TInt iDeregistrationCauseCode;   	
   	/** ETrue if the daemon was successfully created. */
   	TBool iSuccessfullyCreatedDaemon;
	};
	

inline CNifDaemonConfiguration::CNifDaemonConfiguration(MNifIfNotify& aNifIfNotify) :
   CNifConfigurationIf(aNifIfNotify),
   iIoPtr(0, 0),
   iDesDeregActionStatus((TUint8*)&iDeregActionStatus,sizeof(iDeregActionStatus))
/**
 CNifDaemonConfiguration - constructor
 @internalComponent
 @param aNifIfNotify - client of the control
 @version 0.02
**/
	{
  	CActiveScheduler::Add(this);
	}
#endif


