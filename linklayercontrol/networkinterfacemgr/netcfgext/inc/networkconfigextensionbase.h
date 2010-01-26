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
 @file networkconfigextensionbase.h
 @publishedPartner
 @deprecated
 
*/

#if !defined (__NETWORK_CONFIG_EXTENSION_BASE_H__)
#define __NETWORK_CONFIG_EXTENSION_BASE_H__

#include <e32base.h>
#include <comms-infras/rconfigdaemon.h>
#include <comms-infras/nifconfigurationcontrol.h>
#include <es_enum.h>
#include <comms-infras/commsdebugutility.h>

__FLOG_STMT(_LIT8(KLogTagCsDaemon, "CSDAEMON");)
__FLOG_STMT(_LIT8(KLogSubSysNifman, "NIFMAN");)

class CStartServer;
class CAsynchDaemonCancel;
class CNifDaemonProgress;

/**
   These two classes are to solve a class dependancy problem between
   CNetworkConfigExtensionBase and CAsynchDaemonCancel and CNifDaemonProgress
   and also between CNifDaemonProgress and CAsynchDaemonCancel.

   CNetworkConfigExtensionBase contains a RConfigDaemon object which is used by
   CAsynchDaemonCancel. CAsynchDaemonCancel is a self deleting object which
   CNetworkConfigExtensionBase can forgot about once it kicks it off. However
   CAsynchDaemonCancel uses the RConfigDaemon(which is an RSessionBase) to send
   a message to the configuration daemon. If CNetworkConfigExtensionBase is
   deleted before this message is completed on the server side, the RConfigDaemon
   sub session will be deleted and CAsynchDaemonCancel's RunL will never run.

   The active object will hang aronud forever until the CActiveScheduler tries
   to clean up at the end of the thread. ~CActiveScheduler tries to cancel all
   outstanding active objects, which will hang at this point because the session
   on the server side no longer exists.

   These classes stop this situation from happening. An class which wants to be
   notified of another classes deletion should aggregation ADeferredDeletion and
   a class that wants to inform others of its deletion should aggregate in
   ADeletionNotifier.

   -IK
*/
class ADeletionNotifier;

class ADeferredDeletion
/**
   Listener interface to be called when a watched object is deleted
   @publishedPartner
   @deprecated
   
*/
	{
public:
	ADeferredDeletion() :
		iDeletionObjectCount(0),
		iDeleteWhenAllObjectsDeleted(EFalse)
		{
		}

	void ListenForObjectDeletionL(ADeletionNotifier *aDelete);
	IMPORT_C virtual void DeleteThis();
	IMPORT_C void NotifyDeletionL(ADeletionNotifier *aDelete);

	virtual ~ADeferredDeletion() {}

private:
	TUint iDeletionObjectCount;
	TBool iDeleteWhenAllObjectsDeleted;
	};

class ADeletionNotifier
/**
   Aggregate for objects who wish to notify others when they are deleting
   
   @publishedPartner
   @deprecated
   
*/
	{
public:
	ADeletionNotifier(ADeferredDeletion* aDeletionListener)
		: iDeletionListener(aDeletionListener)
		{
		iDeletionListener->ListenForObjectDeletionL(this);
		}
	
	virtual ~ADeletionNotifier();
			
protected:
	ADeferredDeletion* iDeletionListener;
	};

/**
 Daemon configuration, uses RConfigDaemon generic config daemon client API 
 to configure network layer
 @publishedPartner
 @deprecated
 
 @version 0.03
 @date	26/05/2004
**/
class CNetworkConfigExtensionBase : public CNifConfigurationIf, public ADeferredDeletion
	{
   	friend class CNifConfigurationControl;

protected:
   	CNetworkConfigExtensionBase(MNifIfNotify& aNifIfNotify);

public:
  	static CNetworkConfigExtensionBase* NewL(TAny* aMNifIfNotify);

   	IMPORT_C virtual void ConfigureNetworkL();
   	IMPORT_C virtual void LinkLayerDown();
   	IMPORT_C virtual void LinkLayerUp();
   	IMPORT_C virtual void Deregister(TInt aCause);
   	IMPORT_C virtual void SendIoctlMessageL(const ESock::RLegacyResponseMsg& aMessage);
   	IMPORT_C virtual void CancelControl();
   	IMPORT_C virtual void AsyncDelete();
	/**
	Notification - used to inform the daemon of notification events.
	**/
	IMPORT_C virtual void EventNotification(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource);


   	void DoOnDaemonProgress(TInt aStage, TInt aError);
   	void DoOnGenericProgress(TInt aStage, TInt aError);

	IMPORT_C virtual ~CNetworkConfigExtensionBase();
	IMPORT_C virtual void DeleteThis();
	
protected:
   	IMPORT_C virtual void DoCancel();
   	IMPORT_C virtual void RunL();
   	IMPORT_C virtual TInt RunError(TInt aError);

	IMPORT_C void ConstructL();


	
protected:
   	HBufC8* iIoBuf;
   	TPtr8 iIoPtr;
   	RConfigDaemon iConfigDaemon;
   	TConnectionInfoBuf iConnectionInfoBuf;
   	/**
    ipStartServer - != NULL when the daemon is being started
    @publishedPartner
    @deprecated
    
    @version 0.01
   	**/
   	CStartServer* ipStartServer;
   	/**
    iMessage - to keep the original reguest when forwarding the request for further processing
    @publishedPartner
    @deprecated
    
    @version 0.01
   	**/
	ESock::RLegacyResponseMsg iMessage;
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
	

inline CNetworkConfigExtensionBase::CNetworkConfigExtensionBase(MNifIfNotify& aNifIfNotify) :
   CNifConfigurationIf(aNifIfNotify),
   iIoPtr(0, 0),
   iDesDeregActionStatus((TUint8*)&iDeregActionStatus,sizeof(iDeregActionStatus)),
   iDaemonProgress(NULL),
   iMessage()
/**
 CNetworkConfigExtensionBase - constructor
 @publishedPartner
 @deprecated
 @param aNifIfNotify - client of the control
 @version 0.02
**/
	{
  	CActiveScheduler::Add(this);
	}
#endif


