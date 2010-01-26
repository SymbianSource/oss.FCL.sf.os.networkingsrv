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
// Implements generic base for NIFMAN side of configuration daemon
// 
//

/**
 @file NIFConfigurationControl.cpp
 @internalTechnology
*/

#include "CS_DaemonControl.h"
#include "NIFIF.H"
#include "CA_StartServer.h"
#include "CDBCOLS.H"
#include "ImplementationProxy.h"
#include <comms-infras\commsdebugutility.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManCSDmnCntrl, "NifManCSDmnCntrl");
#endif

_LIT(KIAPId, "IAP\\Id");
_LIT(KIAPNetwork, "IAP\\IAPNetwork");

class CAsynchDaemonCancel : public CActive
{
public:
   CAsynchDaemonCancel() :
      CActive(EPriorityStandard)
      {
      }

   void AsyncDelete();

   void CancelControl(RConfigDaemon& aConfigDaemon, TBool aDeleteOnCompletion);
   void CancelControl(RConfigDaemon& aConfigDaemon, TInt aOpMask, TBool aDeleteOnCompletion);

protected:
	virtual void RunL();
	virtual void DoCancel();

protected:
	TBool iDeleteOnCompletion;
	};

class CNifDaemonProgress : public CActive
/**
 * Active object class used by CNifDaemonConfiguration to maintain a persistent 
 * ProgressNotification call against the daemon. Delete this object only using the 
 * AsyncDelete method.
 *
 * @internalComponent
 */
	{
public:
  	static CNifDaemonProgress* NewL(RConfigDaemon& aConfigDaemon, CNifDaemonConfiguration* aProgressDest);
	void ProgressNotification();
   	void AsyncDelete();
   	void CancelControl();

protected:
	void RunL();
	void DoCancel();
		
private:
	// This is private so that you MUST use AsyncDelete.
  	~CNifDaemonProgress();
  	void ConstructL();
   	CNifDaemonProgress(RConfigDaemon& aConfigDaemon, CNifDaemonConfiguration* aProgressDest);
	/** Reference to the configuration daemon interface. */
   	RConfigDaemon& iConfigDaemon;
	/** Used to asynchrounously cancel any outstanding request on iConfigDaemon so that esock is never blocked. */
   	CAsynchDaemonCancel* iAsynchDaemonCancel;
	/** The target for progress notifications. */
   	CNifDaemonConfiguration* iProgressDest;
	/** Stores the progress notification. */
   	TDaemonProgressBuf iProgressBuf;
	/** If ETrue, the object should delete itself when the outstanding request completes. */
	TBool iDeleteOnCompletion;
	};

void CAsynchDaemonCancel::AsyncDelete()
/**
 * Delete the instance of CAsynchDaemonCancel safely.
 * 
 * @internalComponent
 */
	{
	if (IsActive())
		iDeleteOnCompletion = ETrue;
	else
		delete this;
	}

void CAsynchDaemonCancel::DoCancel()
   {//nothing to do
   }

void CAsynchDaemonCancel::CancelControl(RConfigDaemon& aConfigDaemon, TBool aDeleteOnCompletion)
   {
      if (!IsActive())
         {
         __ASSERT_DEBUG(!IsAdded(), User::Panic(KSpecAssert_NifManCSDmnCntrl, 1));	
         CActiveScheduler::Add(this);
         aConfigDaemon.Cancel(iStatus);
         SetActive();
         }
      iDeleteOnCompletion = aDeleteOnCompletion;
   }

void CAsynchDaemonCancel::CancelControl(RConfigDaemon& aConfigDaemon, TInt aOpMask, TBool aDeleteOnCompletion)
/**
 * CancelControl - Asynchronously cancels the last server request according to aOpMask.
 *
 * @internalComponent
 */
   {
      if (!IsActive())
         {
         __ASSERT_DEBUG(!IsAdded(), User::Panic(KSpecAssert_NifManCSDmnCntrl, 2));
         CActiveScheduler::Add(this);
         aConfigDaemon.Cancel(aOpMask,iStatus);
         SetActive();
         }
      iDeleteOnCompletion = aDeleteOnCompletion;
   }
   
void CAsynchDaemonCancel::RunL()
  	{
	   if ( iDeleteOnCompletion )
		   {
		   delete this;
		   }
	   else
		   {
		   Deque();
		   }
  	}
  	
CNifDaemonProgress* CNifDaemonProgress::NewL(RConfigDaemon& aConfigDaemon, CNifDaemonConfiguration* aProgressDest)
/**
 * Standard NewL for CNifDaemonProgress.
 * 
 * @internalComponent
 *
 * @param aConfigDaemon	Daemon client to be used.
 * @param aProgressDest	Destination for progress notifications.
 * @leave KErrNoMemory if there is insufficient heap.
 */
   {
   CNifDaemonProgress* pDaemonProgress = new(ELeave)CNifDaemonProgress(aConfigDaemon, aProgressDest);
   CleanupStack::PushL(pDaemonProgress);
   pDaemonProgress->ConstructL();
   CleanupStack::Pop(pDaemonProgress);
   return pDaemonProgress;
   }

CNifDaemonProgress::~CNifDaemonProgress()
/**
 * Destructor.
 * 
 * @internalComponent
 */
	{
	if (iAsynchDaemonCancel)
		{
		iAsynchDaemonCancel->AsyncDelete();
		iAsynchDaemonCancel = NULL;
		}
	}

void CNifDaemonProgress::ConstructL()
/**
 * Standard ConstructL for CNifDaemonProgress.
 * 
 * @internalComponent
 *
 * @leave KErrNoMemory if there is insufficient heap.
 */
   {
   iAsynchDaemonCancel = new(ELeave)CAsynchDaemonCancel;
   }

CNifDaemonProgress::CNifDaemonProgress(
	RConfigDaemon& aConfigDaemon, 
	CNifDaemonConfiguration* aProgressDest) :
	CActive(EPriorityStandard),
   	iConfigDaemon(aConfigDaemon),
   	iAsynchDaemonCancel(NULL),
   	iProgressDest(aProgressDest),
   	iDeleteOnCompletion(EFalse)
/**
 * Construct the progress active object. Adds the active object to the scheduler and 
 * issues the asynchronous call.
 * 
 * @internalComponent
 *
 * @param aConfigDaemon	Daemon client to be used.
 * @param aProgressDest	Destination for progress notifications.
 */
    {
    CActiveScheduler::Add(this);
    ProgressNotification();       
    }  	
  	
void CNifDaemonProgress::ProgressNotification()
/**
 * Issues the asynchronous progress notification request.
 * 
 * @internalComponent
 */
	{
	iConfigDaemon.ProgressNotification(iProgressBuf, iStatus);
	SetActive();
	}
  	
void CNifDaemonProgress::RunL()
/**
 * Indicates that a progress notification was received. 
 * The progress info will be found in iProgressBuf.
 * 
 * @internalComponent
 */
	{
	if (iDeleteOnCompletion)
		{
		// if iDeleteOnCompletion is true, we are to delete ourselves
	  	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonProgress::RunL - Deleting ourselves."));	
		delete this;
		}
	else if (iStatus.Int() == KErrNone)
		{
		// We successfully got a progress notification. Pass it
		// on the the target and re-issue the request.
	  	__FLOG_STATIC1(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonProgress::RunL - Successfully received progress notification %d. Re-issuing request."),iProgressBuf().iStage);	
		iProgressDest->DoOnDaemonProgress(iProgressBuf().iStage, iProgressBuf().iError);
		ProgressNotification();	
		}
	else if (iStatus.Int() == KErrNotReady)
		{
		// The daemon is starting-up. Re-issue the request.
		__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonProgress::RunL - Progress notification. Waiting for server to start."));
		ProgressNotification();	
		}
	else
		{
		// We failed to get a progress notification.
		// The daemon either does not support this request,
		// has exited or has died. Do not issue another request. 
		// We'll get into a tight loop if we do.
	  	__FLOG_STATIC1(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonProgress::RunL - Failed to get progress notification from daemon. The error was %d"),iStatus.Int());	
	  	}
  	}

void CNifDaemonProgress::AsyncDelete()
/**
 * Cancels any outstanding progress notification and optionally 
 * the instance of CNifDaemonProgress asynchronously to avoid
 * potential deadlock in Esock.
 *
 * @internalComponent
 */	
	{
   	if (IsActive())
    	{
	  	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonProgress::AsyncDelete - Waiting for CancelControl to complete"));	
       	iDeleteOnCompletion = ETrue;
       	CancelControl();
       	}
 	else
    	{
       	delete this;
       	}
	}

void CNifDaemonProgress::CancelControl()
/**
 * Cancels any outstanding progress asynchronously to avoid
 * potential deadlock in Esock.
 *
 * @internalComponent
 */
	{
	if (IsActive())
		{
		iAsynchDaemonCancel->CancelControl(iConfigDaemon, KConfigDaemonOpMaskProgress, iDeleteOnCompletion);
		if (iDeleteOnCompletion)
			iAsynchDaemonCancel = NULL;
		}
	}

void CNifDaemonProgress::DoCancel()
/**
 * Standard active object DoCancel.
 * 
 * @internalComponent
 */
	{
	// DoCancel() is only used by Cancel(), which is can not be 
	// used as it will block the ESOCK thread.
	__ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManCSDmnCntrl, 3)); 
	}
	
// Define the interface UIDs
const TImplementationProxy ImplementationTable[] = 
    {
    IMPLEMENTATION_PROXY_ENTRY(0x101FEBE2, CNifDaemonConfiguration::NewL)
    };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

    return ImplementationTable;
    }

CNifDaemonConfiguration* CNifDaemonConfiguration::NewL( TAny* aMNifIfNotify )
   {
   MNifIfNotify* nifIfNotify = reinterpret_cast<MNifIfNotify*>(aMNifIfNotify);
   CNifDaemonConfiguration* pDaemon = new(ELeave)CNifDaemonConfiguration( *nifIfNotify );
	CleanupStack::PushL(pDaemon);
   pDaemon->ConstructL();
	CleanupStack::Pop(pDaemon);
   return pDaemon;
   }

void CNifDaemonConfiguration::ConstructL()
   {
   __FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonConfiguration::ConstructL"));
   iAsynchDaemonCancel = new(ELeave)CAsynchDaemonCancel;
   }

void CNifDaemonConfiguration::ConfigureNetworkL()
/**
ConfigureNetworkL - starts a daemon and issues configuration request
@internalTechnology
@version 0.02
**/
	{
  	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonConfiguration::ConfigureNetworkL"));	
	//it could access directly CNifAgentRef::ConnectionInfo but i don't want the class be 
	//CNifAgentRef dependent
   	__ASSERT_DEBUG(iMessage.IsNull(), User::Panic(KSpecAssert_NifManCSDmnCntrl, 4));
	
	User::LeaveIfError(iNifIfNotify.ReadInt(KIAPId(), iConnectionInfoBuf().iIapId));
	User::LeaveIfError(iNifIfNotify.ReadInt(KIAPNetwork(), iConnectionInfoBuf().iNetId));
   	//this is the same read as in CNifConfigurationControl::NewL we do the read rather than storing the
   	//server name since the db access should really provide an efficient access to the settings
   	//already selected by NETCON
	TBuf<KCommsDbSvrMaxFieldLength> serverName;		/*100 bytes if unicode*/
	User::LeaveIfError(iNifIfNotify.ReadDes(TPtrC(SERVICE_CONFIG_DAEMON_NAME), serverName));	// safe to pass stack var here, as its copied down the track...

 	// For security reasons, ensure that the server name has the "!" prefix - only protected
 	// servers should be involved with Network Configuration. 	
 	_LIT(KExclamationMark, "!");
 	if (serverName.Left(1).Compare(KExclamationMark()) != 0)
 		serverName.Insert(0,KExclamationMark());
	
	__ASSERT_DEBUG(!ipStartServer, User::Panic(KSpecAssert_NifManCSDmnCntrl, 5));
	DoOnGenericProgress(KConfigDaemonLoading, KErrNone);
	ipStartServer = new(ELeave)CStartServer(iConfigDaemon, iConfigDaemon.Version(), 10);
	ipStartServer->Connect(serverName, iStatus);
	SetActive();
	}

void CNifDaemonConfiguration::LinkLayerDown()	
/**
 * Generates an EConfigDaemonLinkLayerDown request. Used to inform the
 * daemon that link-layer renegotiation has started.
 * 
 * @internalComponent
 */
	{
	if (iSuccessfullyCreatedDaemon)
		iConfigDaemon.LinkLayerDown();
	}

void CNifDaemonConfiguration::LinkLayerUp()	
/**
 * Generates an EConfigDaemonLinkLayerUp request. Used to inform the
 * daemon that link-layer renegotiation has completed.
 * 
 * @internalComponent
 */
	{
	if (iSuccessfullyCreatedDaemon)
		iConfigDaemon.LinkLayerUp();
	}

void CNifDaemonConfiguration::Deregister(
	TInt aCause)	
/**
 * Generates a deregistration request.
 * 
 * @internalComponent
 *
 * @param aCause Specifies what caused the deregistration request (idle timer or Stop call)
 */
	{
	if (IsActive())
		{
		// if a deregistration request isn't queued up, queue one up
		if (!iDeregisterOnCompletionOfRequest)
			{
			iDeregisterOnCompletionOfRequest = ETrue;
			iDeregistrationCauseCode = aCause;
			}
		}
	else if (!iSuccessfullyCreatedDaemon)
		{
		// note that there is no longer a queued deregistration
		// request
		iDeregisterOnCompletionOfRequest = EFalse;
		// fake the behaviour of the daemon and report 
		// KErrNotFound to be compatible with the previous
		// behaviour expected by the DHCP tests - KErrNotFound
		// as in the daemon "was not found"
		DoOnGenericProgress(KConfigDaemonStartingDeregistration, KErrNone);
		DoOnGenericProgress(KConfigDaemonFinishedDeregistrationStop, KErrNotFound);		
		}
	else
		{
		// note that there is no longer a queued deregistration
		// request
		iDeregisterOnCompletionOfRequest = EFalse;
		// progress notification before the operation
		DoOnGenericProgress(KConfigDaemonStartingDeregistration, KErrNone);
		// ask the daemon to deregister
		iConfigDaemon.Deregister(aCause, &iDesDeregActionStatus, iStatus);
		SetActive();
		}    	
	}

void CNifDaemonConfiguration::SendIoctlMessageL(const RMessage2& aMessage)
/**
* SendIoctlMessageL forwards Ioctl request to the daemon and activates the AO to wait for response
* 
@internalTechnology
@version 0.02
@param aMessage[in] a message to be processed (it's the caller's resposibility to forward just Ioctl
*                   messages)
**/
	{
   	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonConfiguration::SendIoctlMessageL"));	
	if (static_cast<TUint>(aMessage.Int0()) != KCOLConfiguration)
		{
		User::Leave(KErrNotSupported);
		}
   	else
		{
		if (!IsActive())
			{
			TDes8* ptr = NULL;
			// may not always have a buffer to read...
			if (aMessage.Ptr2())
				{
            	delete iIoBuf;
            	iIoBuf = NULL;
            	TInt maxLength = aMessage.GetDesMaxLength(2);
				iIoBuf=HBufC8::NewMaxL(maxLength);

				iIoPtr.Set(iIoBuf->Des());
				aMessage.ReadL(2, iIoPtr);
				iIoPtr.SetLength(maxLength);
				ptr = &iIoPtr;
				}
			//store msg after ReadDescriptorL which could leave
			iMessage = aMessage;
			iConfigDaemon.Ioctl(aMessage.Int0(), aMessage.Int1(), iStatus, ptr);
			SetActive();      
			}
		else
			{
			User::Leave(KErrInUse);
			}
		}
	}
	
void CNifDaemonConfiguration::AsyncDelete()
/**
Provide a clever method for deletion and 
asynchronously cancelling any outstanding event
so as to avoid any problems of deadlock
**/	
	{
   	if (IsActive()) //are we waiting?
    	{
       	//yes
       	iDeleteOnCompletion = ETrue;
       	CancelControl();
       	}
 	else
    	{
       	delete this;
       	}
	}

void CNifDaemonConfiguration::EventNotification(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
/**
 Notification - does nothing.
 @internalAll
 @version 0.01
**/
	{
	}

void CNifDaemonConfiguration::CancelControl()
/**
 CancelControl - cancels request asynchronously to avoid deadlock
 @internalAll
 @version 0.01
**/
	{
	if (IsActive()) //are we waiting?
		{
		//yes
		if (ipStartServer)
			{
			ipStartServer->Cancel(); //it'll set an error code to KErrCancel meaning that 
         //the requests will complete with KErrCancel
			}
		else
         {
		   iAsynchDaemonCancel->CancelControl(iConfigDaemon, iDeleteOnCompletion);
         if ( iDeleteOnCompletion )
            {
			   iAsynchDaemonCancel = NULL;
            }
         }
		//the RunL method will be called on the original request cancellation
		}
	}

void CNifDaemonConfiguration::DoCancel()
/**
DoCancel - cancels current request
@internalTechnology
@version 0.01
@see CActive::DoCancel
**/
	{
  	__ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManCSDmnCntrl, 6)); //we shouldn't ever get here, would block NIFMAN/ESOCK thread
	}

void CNifDaemonConfiguration::RunL()
/**
RunL - called when request completes
@internalTechnology
@version 0.03
@see CActive::RunL
**/
	{
  	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNifDaemonConfiguration::RunL"));	
  	
	if (iDeleteOnCompletion)
		{
		// if iDeleteOnCompletion is true, we are to delete ourselves
		
		// if there was an outstanding IOCTL request, complete the message
		if (!iMessage.IsNull())
			{
			// client request completion
			const TAny* ptr = iMessage.Ptr2();
			if (ptr)
				{
				__ASSERT_DEBUG(iIoBuf, User::Panic(KSpecAssert_NifManCSDmnCntrl, 7));
				iMessage.WriteL(2, *iIoBuf);
				delete iIoBuf; //no longer needed
				iIoBuf = NULL;
				}
			iMessage.Complete(iStatus.Int());
			}

      	__ASSERT_DEBUG(!ipStartServer || !ipStartServer->IsActive(), User::Panic(KSpecAssert_NifManCSDmnCntrl, 8));
		delete ipStartServer; //no needed any more
		ipStartServer = NULL;
		delete this;
		// *********************************************
		// CAREFUL... don't do anything after this point
		// because this object has been deleted
		// *********************************************		
		}
	else if (ipStartServer)
		{
		// the daemon server was created (or the creation failed)
		
		// if ipStartServer is not null, then the server has just been
		// started
		delete ipStartServer; //no needed any more
		ipStartServer = NULL;
		if (iStatus.Int() == KErrNone)
			{
			// record the fact that the daemon was successfully 
			// launched
			iSuccessfullyCreatedDaemon = ETrue;
			// signal that the daemon was loaded successfully		
			DoOnGenericProgress(KConfigDaemonLoaded, KErrNone);
			// create progress instance - this registers for daemon progress notifications
 		 	iDaemonProgress = CNifDaemonProgress::NewL(iConfigDaemon, this);			
			// signal before the Configure method is called		
			DoOnGenericProgress(KConfigDaemonStartingRegistration, KErrNone);
			//complete connection
			iConfigDaemon.Configure(iConnectionInfoBuf, iStatus);
			SetActive();
			//wait for the Configure to complete
			}
		else
			{
			//we're done with en error
			 iNifIfNotify.IfProgress(KLinkLayerOpen, iStatus.Int());
			}
		}
	else if (!iMessage.IsNull())
		{
		// the ioctl request completed
		
		// complete the message
		const TAny* ptr = iMessage.Ptr2();
		if (ptr)
			{
			__ASSERT_DEBUG(iIoBuf, User::Panic(KSpecAssert_NifManCSDmnCntrl, 9));
			iMessage.WriteL(2, *iIoBuf);
			delete iIoBuf; //no longer needed
			iIoBuf = NULL;
			}
		iMessage.Complete(iStatus.Int());
		
 		// start to deregister if we happen to have one queued
 		if (iDeregisterOnCompletionOfRequest)
 			Deregister(iDeregistrationCauseCode);
		}
	else if (iLastGenericProgressStage == KConfigDaemonStartingRegistration)
		{
		// the configure call completed
		
		// signal the completed Configure call
		DoOnGenericProgress(KConfigDaemonFinishedRegistration, iStatus.Int());
		//no user request => must be configuration completion => signal it up
		 iNifIfNotify.IfProgress(KLinkLayerOpen, iStatus.Int());
 		// start to deregister if we happen to have one queued
 		if (iDeregisterOnCompletionOfRequest)
 			Deregister(iDeregistrationCauseCode);
		}
	else if (iLastGenericProgressStage == KConfigDaemonStartingDeregistration)
		{
		// the deregistration request completed
		
		// if any error occurred, we should assume stop
		// is the desired result
		if (iStatus.Int() != KErrNone)
			iDeregActionStatus = EConfigDaemonDeregisterActionStop;
		// if the daemon doesn't support deregistration,
		// we act as if everything succeeded
		if (iStatus.Int() == KErrNotSupported)
			iStatus = KErrNone;
		// handle the result
		switch (iDeregActionStatus)
			{
		case EConfigDaemonDeregisterActionStop:
			// progress notification - note that this specific
			// notification will delete this object
			DoOnGenericProgress(KConfigDaemonFinishedDeregistrationStop, iStatus.Int());		
			// *****************************************************
			// CAREFUL... don't do anything after this point because 
			// this object has been deleted as a result of this
			// progress notification
			// *****************************************************
			break;
		case EConfigDaemonDeregisterActionPreserve:
			// progress notification
			DoOnGenericProgress(KConfigDaemonFinishedDeregistrationPreserve, iStatus.Int());		
	 		// start to deregister if we happen to have one queued
	 		if (iDeregisterOnCompletionOfRequest)
	 			Deregister(iDeregistrationCauseCode);
			break;
		default:
			User::Leave(KErrNotSupported);
			break;
			}			
		}
	else
		{
		// should never get here.
		__ASSERT_DEBUG(EFalse, User::Panic(KSpecAssert_NifManCSDmnCntrl, 10));
		}
	// ********************************************
	// CAREFUL... consider the possible deletion of
	// this object in the code above before adding
	// anything down here
	// ********************************************			
	}
	
TInt CNifDaemonConfiguration::RunError(TInt aError)
	{//see CNifDaemonConfiguration::RunL the only case it can leave is when iMessage is valid
   	if (!iMessage.IsNull())
		{
		iMessage.Complete(aError);
		}
   	return KErrNone;
	}	
	
CNifDaemonConfiguration::~CNifDaemonConfiguration()
/**
~CNifDaemonConfiguration - destructor
@internalTechnology
@version 0.02
**/
	{
	// complete any outstanding messages
	if (!iMessage.IsNull())
		{
		//client request completion - no point to write any data back to client here
		iMessage.Complete(KErrCancel);
		}
   	__ASSERT_DEBUG(!ipStartServer, User::Panic(KSpecAssert_NifManCSDmnCntrl, 11));

	// delete safely the progress request    
	if (iDaemonProgress)
		{
		iDaemonProgress->AsyncDelete();
		iDaemonProgress = NULL;
		}

	// unload the daemon		
    DoOnGenericProgress(KConfigDaemonUnloading, KErrNone);
   	iConfigDaemon.Close();
    DoOnGenericProgress(KConfigDaemonUnloaded, KErrNone);
    
    // delete safely the cancel request
   	if (iAsynchDaemonCancel)
   		{
   		iAsynchDaemonCancel->AsyncDelete();
   		iAsynchDaemonCancel = NULL;
   		}
   		
   	delete iIoBuf;
	}	

void CNifDaemonConfiguration::DoOnDaemonProgress(TInt aStage, TInt aError)
/**
 * Called by CNifDaemonProgress when it receives a progress notification. 
 * Passes the notification to CNifAgentRef.
 * 
 * @internalComponent
 * 
 * @param aStage Progress stage reported by the daemon
 * @param aError Error code associated with the stage
 */
	{
	iNifIfNotify.IfProgress(aStage, aError);
	}

void CNifDaemonConfiguration::DoOnGenericProgress(TInt aStage, TInt aError)
/**
 * Called to generate the cs_daemon generic progress notifications.
 * 
 * @internalComponent
 * 
 * @param aStage Generic progress stage
 * @param aError Error code associated with the stage
 */
	{
	iLastGenericProgressStage = aStage;
	iNifIfNotify.IfProgress(aStage, aError);
	}

