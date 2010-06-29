// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is part of an ECOM plug-in
// 
//

#include <ss_std.h>
#include "shimnifmansconn.h"
#include "es_prot.h"


/**
@internalComponent
*/
const TUint KMicrosecondsInASecond = 1000000;
const TInt KMaxTimerPeriod = KMaxTInt32/KMicrosecondsInASecond; //< max period of a CTimer using After()


CSubConnectionLinkShimClient::CSubConnectionLinkShimClient(const CConnection& aConnection, CNifManSubConnectionShim& aSubConnectionShim) :
	iConnection(aConnection), 
	iSubConnectionShim(aSubConnectionShim),
	iOutstandingProgressNotification(EFalse), 
	iOutstandingDataSentNotification(EFalse), 
	iOutstandingDataReceivedNotification(EFalse), 
	iOutstandingSubConnectionActivity(EFalse) 
/**
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tCSubConnectionLinkShimClient() created for id %d, iConnection %08x"), 
							 this, aSubConnectionShim.Id(), &iConnection));
	}

CSubConnectionLinkShimClient::~CSubConnectionLinkShimClient()
/**
Complete all outstanding RMessages

*/
	{	
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\t~CSubConnectionLinkShimClient(), id %d, iSubConnectionShim %08x"), 
				 this, iSubConnectionShim.Id(), this, &iSubConnectionShim));

	if(iActivityTimer)
		{
		iActivityTimer->Cancel();
		delete iActivityTimer;
		iActivityTimer = NULL;
		}

	if(iOutstandingProgressNotification)
		iOutstandingProgressNotificationMessage.Complete(KErrCancel);
	if(iOutstandingDataSentNotification)
		iOutstandingDataSentNotificationMessage.Complete(KErrCancel);
	if(iOutstandingDataReceivedNotification)
		iOutstandingDataReceivedNotificationMessage.Complete(KErrCancel);
	if(iOutstandingSubConnectionActivity)
		iOutstandingSubConnectionActivityMessage.Complete(KErrCancel);
	if (iSubConnectionShim.DataTransferShim())
		{
		iSubConnectionShim.DataTransferShim()->DeRegisterClient(*this);
		}
	}

TBool CSubConnectionLinkShimClient::Match(const CConnection& aConnection) const
	{
	return &iConnection == &aConnection;
	}
	
TSubConnectionUniqueId CSubConnectionLinkShimClient::Id()
	{
	return iSubConnectionShim.Id();
	}

TInt CSubConnectionLinkShimClient::ReturnCode() const
	{
	return iReturnCode;
	}
	
TInt CSubConnectionLinkShimClient::GetCurrentProgress(TNifProgress& aProgress)
/**
Return the current progress state

@param aProgress On return, contains the current progress from the subconnection
@return KErrNone if successful; otherwise one of the system-wide error codes
*/
	{	
	aProgress = iCurrentProgress;
	
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tGetCurrentProgress() => (%d, %d)"), 
			    this, aProgress.iStage, aProgress.iError));
	return KErrNone;
	}

TBool CSubConnectionLinkShimClient::StopL(const RMessage2& aMessage)
	{	
	TInt stopCode = 0;
	RConnection::TConnStopType stopType = static_cast<RConnection::TConnStopType>(aMessage.Int1());
	switch (stopType)
		{
		case RConnection::EStopNormal:
			stopCode = KErrCancel;
			break;
		case RConnection::EStopAuthoritative:
			stopCode = KErrConnectionTerminated;
			break;
		default:
			stopCode = KErrCancel; // to remove compile warning
			User::Leave(KErrArgument);
		}

	TInt ret = iSubConnectionShim.Provider().Stop(iSubConnectionShim.Id(), stopCode, &aMessage);
	if (ret != KErrNone)
		{
		User::Leave(ret);
		}
	return ETrue;
	}

TBool CSubConnectionLinkShimClient::DataTransferredL(const RMessage2& aMessage)
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataTransferredL(), id %d"),
				this, iSubConnectionShim.Id()));

	TUint uplinkDataVolume;
	TUint downlinkDataVolume;

	TInt ret = iSubConnectionShim.DataTransferShim()->DataTransferred(uplinkDataVolume, downlinkDataVolume);

	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataTransferredL(), ret %d, uplink %d, downlink %d"),
			    this, ret, uplinkDataVolume, downlinkDataVolume));

	if (KErrNone == ret)
		{
		TPckg<TUint> uplinkDataVolumePckg(uplinkDataVolume);
		TPckg<TUint> downlinkDataVolumePckg(downlinkDataVolume);

		aMessage.WriteL(1, uplinkDataVolumePckg);
		aMessage.WriteL(2, downlinkDataVolumePckg);
		}
	SetReturnCode(ret);
	return ETrue;
	}

TBool CSubConnectionLinkShimClient::DataTransferredCancel(const RMessage2& /*aMessage*/)
	{
	return ETrue;		
	}

TBool CSubConnectionLinkShimClient::RequestSubConnectionProgressNotificationL(const RMessage2& aMessage)
/**
Request from client for notification of new progress

@pre No outstanding request for data sent notifications for this subconnection on this RConnection
@param aMessage The client message
@return ETrue if the client message is to be completed immediately
@leave leaves with KErrInUse if there is already an outstanding RMessage for progress notification
*/
	{
	if(iOutstandingProgressNotification)
		User::Leave(KErrInUse);

	TInt clientRequestedProgress = 0;
	clientRequestedProgress = static_cast<TUint>(aMessage.Int2());
	// if	- the last progress we sent to the client differs from the current one
	// and	- the current progress is the same as the client requested progress OR 
	//        the client has no requested progress...
	if(iLastProgressToClient!=iCurrentProgress.iStage &&
		(iCurrentProgress.iStage == clientRequestedProgress || clientRequestedProgress==0))
		{		
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tRequestSubConnectionProgressNotificationL() returning progress (%d, %d)"), 
							 this, iCurrentProgress.iStage, iCurrentProgress.iError));

		// ...send the current progress back
		TPckg<TNifProgress> prog(iCurrentProgress);
		aMessage.WriteL(1, prog);
		return ETrue;
		}
	else	// store the client message until the next progress value arrives
		{
		//__FLOG_STATIC1(_L("ESock: "), _L("CSubConnectionLinkShimClient"), 
		// _L("[id: %d]: client requested progress notification; storing client message for later completion"), 
		// iSubConnectionsUniqueId);
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tRequestSubConnectionProgressNotificationL() storing client message for later completion"), this));

		iClientRequestedProgress = clientRequestedProgress;	// may be 0
		iOutstandingProgressNotificationMessage = aMessage;
		iOutstandingProgressNotification = ETrue;
		return EFalse;
		}
	}

TBool CSubConnectionLinkShimClient::CancelSubConnectionProgressNotification(const RMessage2& /*aMessage*/)
/**
Complete outstanding progress notification RMessage

@param aMessage The client message
@return ETrue if the client message is to be completed immediately
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tCancelSubConnectionProgressNotification(), id %d, iSubConnectionShim %08x"), 
						 iSubConnectionShim.Id(), this));

	if(iOutstandingProgressNotification)
		{
		iOutstandingProgressNotificationMessage.Complete(KErrCancel);
		iOutstandingProgressNotification = EFalse;
		}
	return ETrue;
	}

TBool CSubConnectionLinkShimClient::DataSentNotificationRequestL(const RMessage2& aMessage)
/**
Request notification when the specified (absolute or relative) volume of data has been sent

@pre No outstanding request for data sent notifications for this subconnection on this RConnection
@param aMessage The client message
@return ETrue if the client message is to be completed immeadiately
@leave leaves with KErrInUse if there is already an outstanding RMessage for data sent notification
*/
	{
	if(iOutstandingDataSentNotification)
		User::Leave(KErrInUse);

	TUint requestedUplinkGranularity = static_cast<TUint>(aMessage.Int1());
	if(requestedUplinkGranularity)	// the client is working in relative mode
		{
		iRemainingUplinkGranularity = requestedUplinkGranularity;
		iDataSentNotificationsInAbsoluteMode = EFalse;

		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataSentNotificationRequestL(), id %d (relative mode: %d bytes)"), 
							 this, iSubConnectionShim.Id(), iRemainingUplinkGranularity));
		}
	else							// the client is working in absolute mode
		{
		TPckg<TUint> iUplinkVolumeBuf(iUplinkDataNotificationVolume);
		aMessage.ReadL(2, iUplinkVolumeBuf);
		iDataSentNotificationsInAbsoluteMode = ETrue;

		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataSentNotificationRequestL() id %d (absolute mode: %d bytes)"), 
							 this, iSubConnectionShim.Id(), iUplinkDataNotificationVolume));

		if(iUplinkDataNotificationVolume >= iUplinkDataVolume)	// we've already sent this amount of data
			{
			__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataSentNotificationRequestL() id %d (completed immediately"), 
								 this, iSubConnectionShim.Id()));
			return ETrue;
			}
		}
	
	iOutstandingDataSentNotificationMessage = aMessage;
	iOutstandingDataSentNotification = ETrue;
	
	iSubConnectionShim.DataTransferShim()->DataSentNotificationRequest(requestedUplinkGranularity, iUplinkDataNotificationVolume);

	return EFalse;
	}

TBool CSubConnectionLinkShimClient::DataSentNotificationCancel(const RMessage2& /*aMessage*/)
/**
Complete outstanding data sent notification RMessage

@param aMessage The client message
@return ETrue if the client message is to be completed immediately
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataSentNotificationCancel() id %d"), 
				 this, iSubConnectionShim.Id()));
	iSubConnectionShim.DataTransferShim()->DataSentNotificationCancel();

	if(iOutstandingDataSentNotification)
		{
		iOutstandingDataSentNotificationMessage.Complete(KErrCancel);
		iOutstandingDataSentNotification= EFalse;
		}
	return ETrue;
	}

TBool CSubConnectionLinkShimClient::DataReceivedNotificationRequestL(const RMessage2& aMessage)
/**
Request notification when the specified (absolute or relative) volume of data has been sent

@pre No outstanding request for data sent notifications for this subconnection on this RConnection
@param aMessage The client message
@return ETrue if the client message is to be completed immediately
@leave leaves with KErrInUse if there is already an outstanding RMessage for data received notification
*/
	{

	if(iOutstandingDataReceivedNotification)
		User::Leave(KErrInUse);
	
	TUint requestedDownlinkGranularity = static_cast<TUint>(aMessage.Int1());
	if(requestedDownlinkGranularity)	// the client is working in relative mode
		{
		iRemainingDownlinkGranularity = requestedDownlinkGranularity;
		iDataReceivedNotificationsInAbsoluteMode = EFalse;
		
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataReceivedNotificationRequestL() id %d (relative mode: %d bytes)"), 
					 this, iSubConnectionShim.Id(), iRemainingDownlinkGranularity));
		}
	else							// the client is working in absolute mode
		{
		TPckg<TUint> iDownlinkVolumeBuf(iDownlinkDataNotificationVolume);
		aMessage.ReadL(2, iDownlinkVolumeBuf);
		iDataReceivedNotificationsInAbsoluteMode = ETrue;

		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataReceivedNotificationRequestL() id %d (absolute mode: %d bytes)"), 
							 this, iSubConnectionShim.Id(), iDownlinkDataNotificationVolume));
		
		if(iDownlinkDataNotificationVolume >= iDownlinkDataVolume)	// we've already received this amount of data
			{
			__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataReceivedNotificationRequestL() id %d(completed immediately)"), 
						 this, iSubConnectionShim.Id()));
			return ETrue;
			}
		}

	iOutstandingDataReceivedNotificationMessage = aMessage;
	iOutstandingDataReceivedNotification = ETrue;

	iSubConnectionShim.DataTransferShim()->DataReceivedNotificationRequest(requestedDownlinkGranularity, iDownlinkDataNotificationVolume);

	return EFalse;
	}

TBool CSubConnectionLinkShimClient::DataReceivedNotificationCancel(const RMessage2& /*aMessage*/)
/**
Complete outstanding data received notification RMessage

@param aMessage The client message
@return ETrue if the client message is to be completed immediately
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tDataReceivedNotificationCancel() id %d"), 
						 this, iSubConnectionShim.Id()));

	iSubConnectionShim.DataTransferShim()->DataReceivedNotificationCancel();

	if(iOutstandingDataReceivedNotification)
		{
		iOutstandingDataReceivedNotificationMessage.Complete(KErrCancel);
		iOutstandingDataReceivedNotification = EFalse;
		}
	return ETrue;
	}

TBool CSubConnectionLinkShimClient::IsSubConnectionActiveRequestL(const RMessage2& aMessage)
/**
Indicate whether the subconnection is active or not

@note Checks at a period defined in the RMessage
@note Only returns when the state varies from that provided by the client
@param aMessage The client message
@return ETrue if the client message is to be completed immediately
*/
	{
	if(iOutstandingSubConnectionActivity)
		User::Leave(KErrInUse);

	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tIsSubConnectionActiveRequestL() id %d"), 
				this, iSubConnectionShim.Id()));

	// Create the activity timer if it doesn't already exist (from a previous request)
	if(!iActivityTimer)
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tIsSubConnectionActiveRequestL() id %d - creating timer"), 
					this, iSubConnectionShim.Id()));

		iActivityTimer = CActivityTimer::NewL(this, KActivityTimerPriority);
		}

	TPckg<TBool> subConnectionActiveBuf(iClientBelievesSubConnectionActive);
	aMessage.ReadL(2, subConnectionActiveBuf);

	iSubConnectionShim.DataTransferShim()->DataTransferred(iPreviousUplinkDataVolume, iPreviousDownlinkDataVolume);	

	// get clients request timer period and check validity
	TInt timeInSeconds = static_cast<TUint>(aMessage.Int1());
	if(timeInSeconds > KMaxTimerPeriod) // secs; underlying CTimer limitation
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tIsSubConnectionActiveRequestL() id %d - rejecting timer request (%d secs)"), 
							 this, iSubConnectionShim.Id(), timeInSeconds));
		
		SetReturnCode(KErrArgument);
		return ETrue;
		}

	// store in microsecs
	iRequestedClientTimerPeriod = timeInSeconds * KMicrosecondsInASecond;
	
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tIsSubConnectionActiveRequestL() id %d, iClientBelievesSubConnectionActive %d, iRequestedClientTimerPeriod %d - Starting timer."), 
						 this, iSubConnectionShim.Id(), iClientBelievesSubConnectionActive, iRequestedClientTimerPeriod));

	iOutstandingSubConnectionActivity = ETrue;
	iOutstandingSubConnectionActivityMessage = aMessage;

	iActivityTimer->After(iRequestedClientTimerPeriod);
	return EFalse;
	}

TBool CSubConnectionLinkShimClient::IsSubConnectionActiveCancel(const RMessage2& /*aMessage*/)
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tIsSubConnectionActiveCancel() id %d, connection %08x"), 
						 this, iSubConnectionShim.Id(), &iConnection));

	if(iOutstandingSubConnectionActivity)
		{
		iActivityTimer->Cancel();
		ASSERT(iOutstandingSubConnectionActivity); // assert that the timer cancelled rather than completing
		iOutstandingSubConnectionActivityMessage.Complete(KErrCancel);
		iOutstandingSubConnectionActivity = EFalse;
		}
	return ETrue;
	}
	
TInt CSubConnectionLinkShimClient::GetSubConnectionInfo(const RMessage2& aMessage)
	{
	TUint index = static_cast<TUint>(aMessage.Int0());
	
	TInt result = KErrNone;
	TRAP(result,

   	// Find the size of the clients descriptor
   	TInt sizeOfSubConnInfo = aMessage.GetDesLengthL(1);
   
   	// Create an appropriately sized descriptor server-side
   	HBufC8* subConnectionInfo;
   	subConnectionInfo = HBufC8::NewL(sizeOfSubConnInfo);
   	CleanupStack::PushL (subConnectionInfo);
   	
   	TPtr8 subConnInfoPtr(subConnectionInfo->Des());

   	// and read the client data across
      aMessage.ReadL(1, subConnInfoPtr);

   	// Pass it down to the connection provider using the appropriate call
     	if(index==KUseEmbeddedUniqueId)
   		{
   		result = iSubConnectionShim.Provider().GetSubConnectionInfo(subConnInfoPtr);
   		}
   	else
   		{
   		result = iSubConnectionShim.Provider().GetSubConnectionInfo(index, subConnInfoPtr);
   		}
   
   	if (KErrNone == result)
   		{
   		// Write result back into client's address space
   		aMessage.WriteL(1, subConnInfoPtr);
   		}
   		
   	CleanupStack::PopAndDestroy (subConnectionInfo);
      );  // END TRAP
      
	SetReturnCode(result);
	return ETrue;
	}
	
void CSubConnectionLinkShimClient::ProgressNotification(TInt aStage, TInt aError, const TDesC8& /*aInfo*/)
/**
Notification of new progress stage from nif/agent via Nifman and CInterface

@param aStage The progress stage that has been reached
@param aError Any errors that have occured
@param aInfo No idea what this is, it's inserted by CInterface and is currently null
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tProgressNotification(%d, %d) id %d"), 
						this, aStage, aError, iSubConnectionShim.Id()));

	iCurrentProgress.iStage = aStage;
	iCurrentProgress.iError = aError;

	if(iOutstandingProgressNotification)
		{
		if(iLastProgressToClient!=iCurrentProgress.iStage && /* we could assume this since we've probably just received a new progress value */
			(iCurrentProgress.iStage == iClientRequestedProgress || iClientRequestedProgress==0))
			{
			TPckg<TNifProgress> prog(iCurrentProgress);
			TInt err= iOutstandingProgressNotificationMessage.Write(1, prog);
			iOutstandingProgressNotificationMessage.Complete(err);
			iOutstandingProgressNotification= EFalse;
			}
		}
	}

TInt CSubConnectionLinkShimClient::NotifyDataSent(TUint aUplinkVolume, TUint aCurrentGranularity)
/**
Upcall from connection provider, via MConnDataTransferNotify. Update the sent bytes count, and if necessary
complete any outstanding RMessages

@param aUplinkVolume The total number of bytes sent on this subconnection
@note Upcall from CInterface via CConnection
*/
	{	
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tNotifyDataSent(aUplinkVolume %d, aCurrentGranularity %d) id %d"), 
						 this, aUplinkVolume, aCurrentGranularity, iSubConnectionShim.Id()));

	iUplinkDataVolume = aUplinkVolume;

	TBool completeMessage = EFalse;

	if(iOutstandingDataSentNotification)
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - outstanding client request"), 
							 this, iSubConnectionShim.Id()));
		switch(iDataSentNotificationsInAbsoluteMode)
			{
			case ETrue:
				__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - %d bytes remaining to be sent before client completion (absolute mode)"), 
									 this, iSubConnectionShim.Id(), (iUplinkDataNotificationVolume - iUplinkDataVolume)));
				
				if (iUplinkDataVolume >= iUplinkDataNotificationVolume)
					{
					completeMessage = ETrue;
					}
				break;
				
			case EFalse:	// in relative mode
				iRemainingUplinkGranularity -= aCurrentGranularity;
				
				__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - %d bytes remaining to be sent before client completion (relative mode)."), 
									 this, iSubConnectionShim.Id(), iRemainingUplinkGranularity));
				
				if(iRemainingUplinkGranularity <= 0)
					{
					completeMessage = ETrue;
					}
				break;

			default:
				break;
			}
		}

	if(completeMessage)
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - completing client request."), 
							 iSubConnectionShim.Id(), this));
		TPckg<TUint> iUplinkDataVolumePckg(iUplinkDataVolume);
		TInt ret= iOutstandingDataSentNotificationMessage.Write(2, iUplinkDataVolumePckg);
		iOutstandingDataSentNotificationMessage.Complete(ret);
		iOutstandingDataSentNotification= EFalse;
		}
	return KErrNone;
	}

TInt CSubConnectionLinkShimClient::NotifyDataReceived(TUint aDownlinkVolume, TUint aCurrentGranularity)
/**
Update the received bytes count, and if necessary complete any outstanding RMessages

@param aDownlinkVolume The total number of bytes sent on this subconnection
@param aCurrentGranularity The currently set granularity of notifications from the CInterface object
@note Upcall from CInterface via CConnection
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tNotifyDataReceived(aDownlinkVolume %d, aCurrentGranularity %d)"), 
						 this, iSubConnectionShim.Id(), aDownlinkVolume, aCurrentGranularity));

	iDownlinkDataVolume = aDownlinkVolume;

	TBool completeMessage = EFalse;

	if(iOutstandingDataReceivedNotification)
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - outstanding client request"), 
							 this, iSubConnectionShim.Id()));
		switch(iDataReceivedNotificationsInAbsoluteMode)
			{
			case ETrue:
				__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - %d bytes remaining to be sent before client completion (absolute mode)."), 
									 this, iSubConnectionShim.Id(), (iDownlinkDataNotificationVolume - iDownlinkDataVolume)));

				if (iDownlinkDataVolume >= iDownlinkDataNotificationVolume)
					{
					completeMessage = ETrue;
					}
				break;

			case EFalse:	// in relative mode
				iRemainingDownlinkGranularity -= aCurrentGranularity;
		
				__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - %d bytes remaining to be sent before client completion (relative mode)."), 
									 this, iSubConnectionShim.Id(), iRemainingDownlinkGranularity));

				if(iRemainingDownlinkGranularity <= 0)
					{
					completeMessage = ETrue;
					}
				break;

			default:
				break;
			}
		}

	if(completeMessage)
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - completing client request."), 
					 this, &iSubConnectionShim));
		TPckg<TUint> iDownlinkDataVolumePckg(iDownlinkDataVolume);
		TInt ret= iOutstandingDataReceivedNotificationMessage.Write(2, iDownlinkDataVolumePckg);
		iOutstandingDataReceivedNotificationMessage.Complete(ret);
		iOutstandingDataReceivedNotification= EFalse;
		}
	return KErrNone;
	}

TInt CSubConnectionLinkShimClient::NotifyDataTransferred(const TUint aUplinkVolume, const TUint aDownlinkVolume)
/**
Upcall from CConnection, indicating that it has performed a DataTransferred request and notifying us of the results

@param aUplinkVolume The total amount of data sent so far on this subconnection
@param aDownlinkVolume The total amount of data received so far on this subconnection
*/
	{
	// Update internal data counters, complete any outstanding RMessages if appropriate
	// No granularities because we don't know what they are, and because we're taking the 
	// opportunity of using the client's call to update our counters, ie. it's not an 
	// actual notification
	NotifyDataSent(aUplinkVolume, 0);
	NotifyDataReceived(aDownlinkVolume, 0);
	return KErrNone;
	}

void CSubConnectionLinkShimClient::CheckSubConnectionActivity()
/**
Check for activity on the subconnection since the last call (to IsSubConnectionActiveRequest() )

*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tCheckSubConnectionActivity() id %d"), 
						 this, iSubConnectionShim.Id()));

	ASSERT(iOutstandingSubConnectionActivity);

	TUint newUplinkDataVolume;
	TUint newDownlinkDataVolume;
	
	iSubConnectionShim.DataTransferShim()->DataTransferred(newUplinkDataVolume, newDownlinkDataVolume);

	TBool dataTransferred = (newUplinkDataVolume!=iPreviousUplinkDataVolume) || 
		                    (newDownlinkDataVolume!=iPreviousDownlinkDataVolume);

	// If the data transferred volumes haven't change but the client thinks the connection is active...
	if(iClientBelievesSubConnectionActive)
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - client believes subconnection active"), 
							 this, iSubConnectionShim.Id()));

		if(dataTransferred)	// ...and it is, so just start another timer cycle
			{
			__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - ...and it is.  Restart timer for another cycle."), 
								 this, iSubConnectionShim.Id()));

			iPreviousUplinkDataVolume = newUplinkDataVolume;
			iPreviousDownlinkDataVolume = newDownlinkDataVolume;
			iActivityTimer->After(iRequestedClientTimerPeriod);
			}
		else				// ...tell them it's not
			{
			__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - ...and it isn't.  Notify client."), 
								 this, iSubConnectionShim.Id()));

			TPckg<TBool> subConnectionActiveBuf(dataTransferred);
			TInt ret= iOutstandingSubConnectionActivityMessage.Write(2, subConnectionActiveBuf);
			iOutstandingSubConnectionActivityMessage.Complete(ret);
			iOutstandingSubConnectionActivity = EFalse;
			}
		}
	else					// client believes subconnection is idle...
		{
		__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - client believes subconnection idle..."), 
							 this, iSubConnectionShim.Id()));

		if(dataTransferred)
			{
			__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - ...and it isn't.  Notify client."), 
								 this, iSubConnectionShim.Id()));

			TPckg<TBool> subConnectionActiveBuf(dataTransferred);
			TInt ret= iOutstandingSubConnectionActivityMessage.Write(2, subConnectionActiveBuf);
			iOutstandingSubConnectionActivityMessage.Complete(ret);
			iOutstandingSubConnectionActivity = EFalse;
			}
		else				// ...and it is, so just start another timer cycle
			{
			__CFLOG_VAR((KShimScprTag, KShimScprClientTag, _L8("CSubConnectionLinkShimClient %08x:\tid %d - ...and it is.  Restart timer for another cycle."), 
								 this, iSubConnectionShim.Id()));

			iPreviousUplinkDataVolume = newUplinkDataVolume;
			iPreviousDownlinkDataVolume = newDownlinkDataVolume;
			iActivityTimer->After(iRequestedClientTimerPeriod);
			}
		}
	}

CSubConnectionLinkShimClient::CActivityTimer* CSubConnectionLinkShimClient::CActivityTimer::NewL(CSubConnectionLinkShimClient* aOwner, TInt aPriority)
/**
Construct a new CActivityTimer()

@param aOwner The owning CSubConnectionLinkShimClient (on which we call methods upon timer completion)
@param aPriority The priority of the active object underlying this timer object
@return A pointer to the newly constructed CActivityTimer object
*/
	{
	CSubConnectionLinkShimClient::CActivityTimer* newActivityTimer = 
		new(ELeave) CSubConnectionLinkShimClient::CActivityTimer(aOwner, aPriority);

	CleanupStack::PushL(newActivityTimer);
	newActivityTimer->ConstructL();
	CleanupStack::Pop(newActivityTimer);
	return newActivityTimer;
	}

void CSubConnectionLinkShimClient::CActivityTimer::RunL()
/**
Call the owning object's check activity method

*/
	{ 
	iOwner->CheckSubConnectionActivity(); 
	} 
