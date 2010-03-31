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

#include "networkconfigextensionbase.h"
#include <comms-infras/nifif.h>
#include <comms-infras/ca_startserver.h>
#include <cdbcols.h>
#include <comms-infras/commsdebugutility.h>
#include <cdblen.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManNetCfgExtn, "NifManNetCfgExtn");
#endif

_LIT(KIAPId, "IAP\\Id");
_LIT(KIAPNetwork, "IAP\\IAPNetwork");


/**
   ADeletionNotifier
*/
ADeletionNotifier::~ADeletionNotifier()
	{
	iDeletionListener->NotifyDeletionL(this);
	}

/**
   ADeferredDeletion
*/	
void ADeferredDeletion::ListenForObjectDeletionL(ADeletionNotifier */*aDelete*/)
	{
	iDeletionObjectCount++;
	}

EXPORT_C void ADeferredDeletion::DeleteThis()
	{
	iDeleteWhenAllObjectsDeleted = ETrue;
	if (iDeletionObjectCount == 0)
		{
		delete this;
		}
	}

EXPORT_C void ADeferredDeletion::NotifyDeletionL(ADeletionNotifier */*aDelete*/)
	{
	iDeletionObjectCount--;
	if (iDeleteWhenAllObjectsDeleted && iDeletionObjectCount == 0)
		{
		DeleteThis();
		}
	}

NONSHARABLE_CLASS(CAsynchDaemonCancel) : public CActive, public ADeletionNotifier
{
public:
   CAsynchDaemonCancel(ADeferredDeletion* aDeletionListener) :
	   CActive(EPriorityStandard), ADeletionNotifier(aDeletionListener)
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

NONSHARABLE_CLASS(CNifDaemonProgress) : public CActive, public ADeletionNotifier, public ADeferredDeletion
/**
 * Active object class used by CNetworkConfigExtensionBase to maintain a persistent 
 * ProgressNotification call against the daemon. Delete this object only using the 
 * AsyncDelete method.
 *
 * @internalComponent
 */
	{
public:
	static CNifDaemonProgress* NewL(RConfigDaemon& aConfigDaemon, CNetworkConfigExtensionBase* aProgressDest, ADeferredDeletion* aDeletionListener);
	void ProgressNotification();
   	void AsyncDelete();
	void DeleteThis();
   	void CancelControl();

protected:
	void RunL();
	void DoCancel();

	// This is protectde so that you MUST use AsyncDelete.
	virtual ~CNifDaemonProgress() {};
	
private:

  	void ConstructL();
   	CNifDaemonProgress(RConfigDaemon& aConfigDaemon, CNetworkConfigExtensionBase* aProgressDest, ADeferredDeletion* aDeletionListener);
	/** Reference to the configuration daemon interface. */
   	RConfigDaemon& iConfigDaemon;
	/** Used to asynchrounously cancel any outstanding request on iConfigDaemon so that esock is never blocked. */
   	CAsynchDaemonCancel* iAsynchDaemonCancel;
	/** The target for progress notifications. */
   	CNetworkConfigExtensionBase* iProgressDest;
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
         __ASSERT_DEBUG(!IsAdded(), User::Panic(KSpecAssert_NifManNetCfgExtn, 1));	
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
         __ASSERT_DEBUG(!IsAdded(), User::Panic(KSpecAssert_NifManNetCfgExtn, 2));
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
  	
CNifDaemonProgress* CNifDaemonProgress::NewL(RConfigDaemon& aConfigDaemon, CNetworkConfigExtensionBase* aProgressDest, ADeferredDeletion* aDeletionListener)
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
	   CNifDaemonProgress* pDaemonProgress = new(ELeave)CNifDaemonProgress(aConfigDaemon, aProgressDest, aDeletionListener);
   CleanupStack::PushL(pDaemonProgress);
   pDaemonProgress->ConstructL();
   CleanupStack::Pop(pDaemonProgress);
   return pDaemonProgress;
   }

void CNifDaemonProgress::DeleteThis()
	{
	if (iAsynchDaemonCancel)
		{
		iAsynchDaemonCancel->AsyncDelete();
		iAsynchDaemonCancel = NULL;
		}
	ADeferredDeletion::DeleteThis();
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
   iAsynchDaemonCancel = new(ELeave)CAsynchDaemonCancel(this);
   }

CNifDaemonProgress::CNifDaemonProgress(
	RConfigDaemon& aConfigDaemon, 
	CNetworkConfigExtensionBase* aProgressDest,
	ADeferredDeletion* aDeletionListener) :
	CActive(EPriorityStandard),
	ADeletionNotifier(aDeletionListener),
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
		DeleteThis();
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
		DeleteThis();
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
	__ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManNetCfgExtn, 3)); 
	}

CNetworkConfigExtensionBase* CNetworkConfigExtensionBase::NewL( TAny* aMNifIfNotify )
   {
   MNifIfNotify* nifIfNotify = reinterpret_cast<MNifIfNotify*>(aMNifIfNotify);
   CNetworkConfigExtensionBase* pDaemon = new(ELeave)CNetworkConfigExtensionBase( *nifIfNotify );
	CleanupStack::PushL(pDaemon);
   pDaemon->ConstructL();
	CleanupStack::Pop(pDaemon);
   return pDaemon;
   }

EXPORT_C void CNetworkConfigExtensionBase::ConstructL()
   {
   __FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNetworkConfigExtensionBase::ConstructL"));
   iAsynchDaemonCancel = new(ELeave)CAsynchDaemonCancel(this);
   }

EXPORT_C void CNetworkConfigExtensionBase::ConfigureNetworkL()
/**
ConfigureNetworkL - starts a daemon and issues configuration request
@internalTechnology
@version 0.02
**/
	{
  	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNetworkConfigExtensionBase::ConfigureNetworkL"));	
	//it could access directly CNifAgentRef::ConnectionInfo but i don't want the class be 
	//CNifAgentRef dependent
   	__ASSERT_DEBUG(iMessage.IsNull(), User::Panic(KSpecAssert_NifManNetCfgExtn, 4));
	
	User::LeaveIfError(iNifIfNotify->ReadInt(KIAPId(), iConnectionInfoBuf().iIapId));
	User::LeaveIfError(iNifIfNotify->ReadInt(KIAPNetwork(), iConnectionInfoBuf().iNetId));
   	//this is the same read as in CNifConfigurationControl::NewL we do the read rather than storing the
   	//server name since the db access should really provide an efficient access to the settings
   	//already selected by NETCON
	TBuf<KCommsDbSvrMaxFieldLength> serverName;		/*100 bytes if unicode*/
	User::LeaveIfError(iNifIfNotify->ReadDes(TPtrC(SERVICE_CONFIG_DAEMON_NAME), serverName));
	// safe to pass stack var here, as its copied down the track...

	// For security reasons, ensure that the server name has the "!" prefix - only protected
	// servers should be involved with Network Configuration.
	
	_LIT(KExclamationMark, "!");
	if (serverName.Left(1).Compare(KExclamationMark()) != 0)
		serverName.Insert(0,KExclamationMark());
	
	__ASSERT_DEBUG(!ipStartServer, User::Panic(KSpecAssert_NifManNetCfgExtn, 5));
	DoOnGenericProgress(KConfigDaemonLoading, KErrNone);
	ipStartServer = new(ELeave)CStartServer(iConfigDaemon, iConfigDaemon.Version(), 10);
	ipStartServer->Connect(serverName, iStatus);
	SetActive();
	}

EXPORT_C void CNetworkConfigExtensionBase::LinkLayerDown()	
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

EXPORT_C void CNetworkConfigExtensionBase::LinkLayerUp()	
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

EXPORT_C void CNetworkConfigExtensionBase::Deregister(
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

EXPORT_C void CNetworkConfigExtensionBase::SendIoctlMessageL(const ESock::RLegacyResponseMsg& aMessage)
/**
* SendIoctlMessageL forwards Ioctl request to the daemon and activates the AO to wait for response
* 
@internalTechnology
@version 0.02
@param aMessage[in] a message to be processed (it's the caller's resposibility to forward just Ioctl
*                   messages)
**/
	{
   	__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNetworkConfigExtensionBase::SendIoctlMessageL"));	
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
            	TInt maxLength = aMessage.GetDesMaxLengthL(2);
				iIoBuf=HBufC8::NewMaxL(maxLength);
				iIoPtr.Set(iIoBuf->Des());
				aMessage.ReadL(2, iIoPtr);
				
				TInt length = aMessage.GetDesLength(2);
				if (length<1) //length could be -ve due to error return from GetDesLength
					{
					iIoPtr.SetLength(maxLength);
					}
				else
					{
					iIoPtr.SetLength(length);
					}
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
	
EXPORT_C void CNetworkConfigExtensionBase::AsyncDelete()
/**
Provide a clever method for deletion and 
asynchronously cancelling any outstanding event
so as to avoid any problems of deadlock
**/	
	{
	if (IsActive()) //are we waiting?
    	{
       	//yes
		// complete any blocked client message immediately - no sense in keeping them hanging on and in a session close 
		// case the dealer may race us to this
		if (!iMessage.IsNull())
			{
			iMessage.Complete(KErrCancel);
			}
       	iDeleteOnCompletion = ETrue;
       	CancelControl();
       	// Zero our reference to the NIFMAN CNifAgentRef as it can be destroyed
       	// ahead of us (this problem manifests itself during cycles of connection
       	// start immediately followed by connection stop).
       	iNifIfNotify = NULL;
       	}
 	else
    	{
		DeleteThis();
       	}
	}

EXPORT_C void CNetworkConfigExtensionBase::CancelControl()
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
			if(iLastGenericProgressStage != KConfigDaemonStartingDeregistration)
				{
				iAsynchDaemonCancel->CancelControl(iConfigDaemon, iDeleteOnCompletion);
				if ( iDeleteOnCompletion )
					{
					iAsynchDaemonCancel = NULL;
					}
				}
			
			// Clear out outstanding RMessage2 as it is no longer outstanding (it will be completed elsewhere)
			iMessage = ESock::RLegacyResponseMsg();
			}
		//the RunL method will be called on the original request cancellation
		}
	}

EXPORT_C void CNetworkConfigExtensionBase::DoCancel()
/**
DoCancel - cancels current request
@internalTechnology
@version 0.01
@see CActive::DoCancel
**/
	{
  	__ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManNetCfgExtn, 6)); //we shouldn't ever get here, would block NIFMAN/ESOCK thread
	}

EXPORT_C void CNetworkConfigExtensionBase::RunL()
/**
RunL - called when request completes
@internalTechnology
@version 0.03
@see CActive::RunL
**/
	{
  	//__FLOG_STATIC0(KLogSubSysNifman, KLogTagCsDaemon, _L("CNetworkConfigExtensionBase::RunL"));	
  	
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
				__ASSERT_DEBUG(iIoBuf, User::Panic(KSpecAssert_NifManNetCfgExtn, 7));
				iMessage.WriteL(2, *iIoBuf);
				delete iIoBuf; //no longer needed
				iIoBuf = NULL;
				}
			iMessage.Complete(iStatus.Int());
			}

      	__ASSERT_DEBUG(!ipStartServer || !ipStartServer->IsActive(), User::Panic(KSpecAssert_NifManNetCfgExtn, 8));
		delete ipStartServer; //no needed any more
		ipStartServer = NULL;
		DeleteThis();
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
 		 	iDaemonProgress = CNifDaemonProgress::NewL(iConfigDaemon, this, this);
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
			iNifIfNotify->IfProgress(KLinkLayerOpen, iStatus.Int());
			}
		}
	else if (!iMessage.IsNull())
		{
		// the ioctl request completed
		
		// complete the message
		const TAny* ptr = iMessage.Ptr2();
		if (ptr)
			{
			__ASSERT_DEBUG(iIoBuf, User::Panic(KSpecAssert_NifManNetCfgExtn, 9));
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
		 iNifIfNotify->IfProgress(KLinkLayerOpen, iStatus.Int());
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
		
		// Async cancel must have completed
		// Start deregistration if it was queued up
		if (iDeregisterOnCompletionOfRequest)
			{
			Deregister(iDeregistrationCauseCode);
			}
		}
	// ********************************************
	// CAREFUL... consider the possible deletion of
	// this object in the code above before adding
	// anything down here
	// ********************************************			
	}
	
EXPORT_C TInt CNetworkConfigExtensionBase::RunError(TInt aError)
	{//see CNetworkConfigExtensionBase::RunL the only case it can leave is when iMessage is valid
   	if (!iMessage.IsNull())
		{
		iMessage.Complete(aError);
		}
   	return KErrNone;
	}	

EXPORT_C void CNetworkConfigExtensionBase::DeleteThis()
	{
	// delete safely the progress request    
	if (iDaemonProgress)
		{
		iDaemonProgress->AsyncDelete();
		iDaemonProgress = NULL;
		}

	// delete safely the cancel request
   	if (iAsynchDaemonCancel)
   		{
   		iAsynchDaemonCancel->AsyncDelete();
   		iAsynchDaemonCancel = NULL;
   		}

	ADeferredDeletion::DeleteThis();
/*	iDeleteWhenAllObjectsDeleted = ETrue;
	if (iDeletionObjectCount == 0)
		{
		delete this;
		}*/
	}

EXPORT_C CNetworkConfigExtensionBase::~CNetworkConfigExtensionBase()
/**
~CNetworkConfigExtensionBase - destructor
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
   	__ASSERT_DEBUG(!ipStartServer, User::Panic(KSpecAssert_NifManNetCfgExtn, 11));

	// unload the daemon		
    DoOnGenericProgress(KConfigDaemonUnloading, KErrNone);
   	iConfigDaemon.Close();
    DoOnGenericProgress(KConfigDaemonUnloaded, KErrNone);
       		
   	delete iIoBuf;
	}	

EXPORT_C void CNetworkConfigExtensionBase::EventNotification(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
/**
 Notification - does nothing. Needs to be implemented by deriving class to achieve functionality.
 @internalTechnology
 @version 0.01
**/
	{}
	
void CNetworkConfigExtensionBase::DoOnDaemonProgress(TInt aStage, TInt aError)
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
	if (iNifIfNotify)				// see comment in AsyncDelete()
		{
		iNifIfNotify->IfProgress(aStage, aError);
		}
	}

void CNetworkConfigExtensionBase::DoOnGenericProgress(TInt aStage, TInt aError)
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
	if (iNifIfNotify)				// see comment in AsyncDelete()
		{
		iNifIfNotify->IfProgress(aStage, aError);
		}
	}

