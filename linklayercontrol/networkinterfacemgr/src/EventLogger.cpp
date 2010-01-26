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
 @file
 @internalTechnology
*/

#include "EventLogger.h"
#include "SLogger.h"
#include "AgentPanic.h"
#include <logengdurations.h>

EXPORT_C CEventLogger* CEventLogger::NewL()
	{
	CEventLogger* eventLogger = new(ELeave) CEventLogger;
	CleanupStack::PushL(eventLogger);
	eventLogger->ConstructL();
	CleanupStack::Pop();
	return eventLogger;
	}

CEventLogger::CEventLogger(): CActive(EPriorityIdle)
	{
	CActiveScheduler::Add(this);
	}

void CEventLogger::ConstructL()
	{
	User::LeaveIfError(iFsEventLog.Connect());
	iLogWrap = CLogWrapper::NewL(iFsEventLog,CActive::EPriorityStandard);
	iCurrentLogEvent = CLogEvent::NewL();
	iLogEventQueue = new(ELeave) CArrayPtrFlat<CLogEvent>(KEventStateMaxCount);
	}

EXPORT_C CEventLogger::~CEventLogger()
	{
	Cancel();
	// the queue could be not empty if the active object is aborted and destructed
	while (iLogEventQueue && (iLogEventQueue->Count() > 0))
		{
		CLogEvent* eventPtr = iLogEventQueue->At(0);
		// delete the CLogEvent object
		delete eventPtr;
		//remove the pointer from the queue
		iLogEventQueue->Delete(0);
		}
	delete iLogWrap;
	iLogWrap=NULL;
	delete iCurrentLogEvent;
	iCurrentLogEvent=NULL;
	delete iLogEventQueue;
	iLogEventQueue = NULL;
	iFsEventLog.Close();
	}

EXPORT_C void CEventLogger::Cancel()
	{
	if(iLogWrap)
		{
		iLogWrap->Log().Cancel();
		}
	//this method written before CEventLogger moved to a CActive is hiding the access to Cactive version of Cancel()
	// The only time cancel should be called is when there is a Panic and we need to destroy the whole object)
	CActive::Cancel();
	}

EXPORT_C void CEventLogger::LogCallStart(const TDesC& aRemote,TInt aLogDir,const TDesC& aTelNum,TUid aDataEventType, TRequestStatus& aStatus)
/**
DEPRECATED, use LogDataAddEvent instead
*/
	{
	LOGSTRING2("GenConn:\tCEventLogger LogCallStart aRemote:%S", &aRemote);
	LogDataAddEvent(R_LOG_CON_CONNECTED, aRemote, aLogDir, aTelNum, aDataEventType);
	// we could complete the TRequestStatus right now. We go safer way and let istatus complete when logg evnt has been added
	LogDataNotifyLastEventUpdate(&aStatus);
	}

EXPORT_C void CEventLogger::LogCallEnd(TRequestStatus& aStatus)
/**
DEPRECATED, use  LogDataUpdateEvent instead
some code may rely on aStatus being triggered when the logger has completed doing 
the last update so that we destroy everything
*/
	{
	LOGSTRING("GenConn:\tCEventLogger LogCallEnd");
	LogDataUpdateEvent( R_LOG_CON_DISCONNECTED, TUid::Null());
	LogDataNotifyLastEventUpdate(&aStatus);	
	}

EXPORT_C void CEventLogger::LogDataTransferred(TInt64 aBytesSent, TInt64 aBytesReceived,TUid aDataEventType, TRequestStatus& aStatus)
/**
DEPRECATED, use  LogDataUpdateEvent instead
*/
	{
	LogDataUpdateEvent(KConnectionStatusIdNotAvailable , aDataEventType, aBytesSent, aBytesReceived);
	// we complete the aStatus right now even if the log might be updated later
	TRequestStatus* st = &aStatus;
	User::RequestComplete(st, KErrNone);
	}
EXPORT_C void CEventLogger::LogDataAddEvent(TInt aRConnectionStatusId, const TDesC& aRemote, TInt aLogDir, const TDesC& aTelNum, TUid aDataEventType)
	{
	LOGSTRING2("GenConn:\tCEventLogger LogDataAddEvent aRConnectionStatusId:%d", aRConnectionStatusId);
	//It is possible to add a new logevent with a new log id for the same connection (reconnect case)
	// assuming that all the next updates will be for the new event and not the old one.

	// [NeilMa 140403]: This method cannot leave and has no return value, but 
	// performs memory allocations if the event cannot be logged immediately. 
	// Therefore, if the memory alloc fails for any reason, the event is 
	// currently discarded with no record. This method should be replaced by 
	// one which can Leave or returns an error code. See Typhoon DEF022946.
	TTime time;
	time.UniversalTime();

	if (!IsActive() && (iLogEventQueue->Count() ==0))
		{
	    iCurrentLogEvent->SetId(KGenconnLogWaitingForLogId);
		iCurrentLogEvent->SetTime(time);
		TBuf<KLogMaxStatusLength > logStatusBuf;
		iLogWrap->Log().GetString(logStatusBuf, aRConnectionStatusId); // Ignore error - string blank on error which is ok
		iCurrentLogEvent->SetStatus(logStatusBuf);
		iCurrentLogEvent->SetRemoteParty(aRemote);
		TBuf<KLogMaxDirectionLength> logDirBuf;
		iLogWrap->Log().GetString(logDirBuf, aLogDir); // Ignore error - string blank on error which is ok
		iCurrentLogEvent->SetDirection(logDirBuf);
		iCurrentLogEvent->SetNumber(aTelNum);
		iCurrentLogEvent->SetEventType(aDataEventType);
		iCurrentLogEvent->SetDurationType(KLogDurationValid);
		iStatus=KRequestPending;
		iLogWrap->Log().AddEvent(*iCurrentLogEvent, iStatus);
		SetActive();
		}
	else
		{
		// add the request to the queue, it will be processed asap
		CLogEvent* eventUpdate = 0;
		TRAPD(error, eventUpdate = CLogEvent::NewL());
		if (KErrNone != error)
			{
			return; // event is discarded!
			}
	    eventUpdate->SetId(KGenconnLogWaitingForLogId);
		eventUpdate->SetTime(time);
		TBuf<KLogMaxStatusLength > logStatusBuf;
		iLogWrap->Log().GetString(logStatusBuf, aRConnectionStatusId); // Ignore error - string blank on error which is ok
		eventUpdate->SetStatus(logStatusBuf);
		eventUpdate->SetRemoteParty(aRemote);
		TBuf<KLogMaxDirectionLength> logDirBuf;
		iLogWrap->Log().GetString(logDirBuf, aLogDir); // Ignore error - string blank on error which is ok
		eventUpdate->SetDirection(logDirBuf);
		eventUpdate->SetNumber(aTelNum);
		eventUpdate->SetEventType(aDataEventType);
		eventUpdate->SetDurationType(KLogDurationValid);
		// add to the queue
		TRAP(error, iLogEventQueue->AppendL(eventUpdate));
		if (KErrNone != error)
			{
			delete eventUpdate; // event is discarded!
			return;
			}
		}
	}

TInt CEventLogger::UpdateLogEventParam(CLogEvent& aLogEvent, TInt aRConnectionStatusId, const TUid& aDataEventType, const TInt64& aBytesSent, const TInt64& aBytesReceived)
	{
	
	TTime time;
	TInt ret =KErrNone;
	time.UniversalTime();
	TTimeIntervalSeconds interval(0);
	if (time.SecondsFrom(iCurrentLogEvent->Time(),interval) != KErrNone)
		{
		interval = 0;	// no duration available ->error
		}
	if (KConnectionStatusIdNotAvailable != aRConnectionStatusId)
		{
		//status needs to be updated
		TBuf<KLogMaxStatusLength > logStatusBuf;
		iLogWrap->Log().GetString(logStatusBuf, aRConnectionStatusId); // Ignore error - string blank on error which is ok
		aLogEvent.SetStatus(logStatusBuf);
		}
	if ( aDataEventType != TUid::Null())
		{
		aLogEvent.SetEventType(aDataEventType);
		}
	aLogEvent.SetDuration(interval.Int());		//0 or not
	//check if data metrics need to be updated
	TInt64 byteInfoNotAvailable(KBytesInfoNotAvailable);
	if ((aBytesReceived != byteInfoNotAvailable) && (aBytesSent != byteInfoNotAvailable))
		{
		TBuf8<KDatabufferSize> dataBuffer;
		dataBuffer.Num(aBytesSent);
		dataBuffer.Append(TChar(','));
		dataBuffer.AppendNum(aBytesReceived);
		TRAP(ret, aLogEvent.SetDataL(dataBuffer));
		}
	return ret;
	}
EXPORT_C TInt CEventLogger::LogDataUpdateEvent(TInt aRConnectionStatusId, const TUid& aDataEventType)
	{
	return LogDataUpdateEvent(aRConnectionStatusId, aDataEventType, KBytesInfoNotAvailable, KBytesInfoNotAvailable);
	}

EXPORT_C TInt CEventLogger::LogDataUpdateEvent(TInt aRConnectionStatusId, const TUid& aDataEventType, const TInt64& aBytesSent, const TInt64& aBytesReceived)
	{
	LOGSTRING("GenConn:\tCEventLogger LogDataUpdateEvent");
	TInt ret = KErrNone;
	// check if there is a request ongoing on the current event
	// check if no request pending then start it otherwise wait until the previous request is finished and keep going on.
	//PROBLEM HERE: 
	// if LogDataAddEvent has not been called then iCurrentLogEvent->Id() == KLogNullId we do nothing, we should assert
	// if id is not ready yet, iCurrentLogEvent->Id() is equal to KGenconnLogWaitingForLogId and the active object is active.
	// So it is put as a request and it will be updated when the event is ready to be processed otherwise if we don't log at all, we should do nothing.
	if (iCurrentLogEvent->Id() != KLogNullId)
		{
		if (!IsActive() && (iLogEventQueue->Count() ==0))
			{
			// request update straight on
			UpdateLogEventParam(*iCurrentLogEvent, aRConnectionStatusId, aDataEventType, aBytesSent, aBytesReceived);
			iLogWrap->Log().ChangeEvent(*iCurrentLogEvent, iStatus);
			SetActive();
			}
		else
			{
			// add the request to the queue, it will be processed asap
			CLogEvent* eventUpdate = 0;
			TRAP(ret, eventUpdate = CLogEvent::NewL());
			if(KErrNone != ret)
				{
				return ret;
				}
			TRAP(ret, eventUpdate->CopyL(*iCurrentLogEvent));
			if(KErrNone != ret)
				{
				delete eventUpdate;
				return ret;
				}

			ret = UpdateLogEventParam(*eventUpdate, aRConnectionStatusId, aDataEventType, aBytesSent, aBytesReceived);
			if(KErrNone != ret)
				{
				delete eventUpdate;
				return ret;
				}

			// add to the queue
			TRAP(ret, iLogEventQueue->AppendL(eventUpdate));
			if(KErrNone != ret)
				{
				delete eventUpdate;
				return ret;
				}
			}
		}
	return ret;
	}

EXPORT_C void CEventLogger::LogDataNotifyLastEventUpdate(TRequestStatus* aStatus)
	{
	//only 1 listener for the notification supported
	__ASSERT_DEBUG((iNotificationRequestStatus == NULL), AgentPanic(Agent::EEventLoggerMoreThanOneListenerForNotifyLastUpdate));
	iNotificationRequestStatus = aStatus;
	if (!IsActive() && (iLogEventQueue->Count() ==0))
		{
		//already finished processing all the log event updates in the queue
		// we can complete straight on
		User::RequestComplete(iNotificationRequestStatus, KErrNone);
		iNotificationRequestStatus = NULL;	// did the job, so do need the pointer anymore.
		}
	}

void CEventLogger::RunL()
	{
	// request has completed
	// delete completed event and check if there is a next event pending
	// If LogEng is not supported, a dummy logeng just returns error straight on.
	// but we carry on doing all the requests
	if (iLogEventQueue->Count() >0)
		{
		CLogEvent* nextEventPtr = iLogEventQueue->At(0);
		__ASSERT_DEBUG((nextEventPtr != NULL), AgentPanic(Agent::ENullCLogEventPointerPresentInLogEventQueue));
		if (nextEventPtr->Id() == KGenconnLogWaitingForLogId)
			{
			//Id was not available when the update has been entered because addEvent did not return at that time
			nextEventPtr->SetId(iCurrentLogEvent->Id());
			}
		iCurrentLogEvent->CopyL(*nextEventPtr);
		iLogWrap->Log().ChangeEvent(*iCurrentLogEvent, iStatus);
		SetActive();
		// delete the ongoing CLogEvent we just copied to currentLogEvent
		delete nextEventPtr;
		//remove the pointer from the queue
		iLogEventQueue->Delete(0);
		}
	else if (iNotificationRequestStatus!=NULL)
		{
		// We have finished processing all the log event updates in the queue
		User::RequestComplete(iNotificationRequestStatus, KErrNone);
		iNotificationRequestStatus = NULL;	// did the job, so do need the pointer anymore.
		}
	}

void CEventLogger::DoCancel()
	{
	if(iLogWrap)
		{
		iLogWrap->Log().Cancel();
		}
	// usually you do not need to cancel an update on events, just let them go and be removed from the queue when update is done
	// if we cancel the logger, most likely the whole Logger Object will be destroyed
}

