/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file EVENTLOGGER.H
*/

#ifndef __EVENTLOGGER_H__
#define __EVENTLOGGER_H__
#include <logcli.h>
#include <logwraplimits.h>

/**
Event Logging facility
@internalComponent
*/
const TLogId KGenconnLogWaitingForLogId = KLogNullId -1;

/**
Event Logging facility
@internalComponent
*/
const TInt KBytesInfoNotAvailable = -1;

class CEventLogger : public CActive
/**
@internalTechnology
*/
	{
public:
	enum { KConnectionStatusIdNotAvailable = -1,
		      KEventStateMaxCount = 10,
		      KDatabufferSize = 64};
	IMPORT_C static CEventLogger* NewL();
	IMPORT_C virtual ~CEventLogger();
	IMPORT_C void Cancel();
	IMPORT_C void LogCallStart(const TDesC& aRemote,TInt aLogDir, const TDesC& aTelNum, TUid aDataEventType, TRequestStatus& aStatus);
	IMPORT_C void LogCallEnd(TRequestStatus& aStatus);
	IMPORT_C void LogDataTransferred(TInt64 aBytesSent, TInt64 aBytesReceived, TUid aDataEventType, TRequestStatus& aStatus);
	IMPORT_C void LogDataAddEvent(TInt aRConnectionStatusId, const TDesC& aRemote, TInt aLogDir, const TDesC& aTelNum, TUid aDataEventType);
	TInt UpdateLogEventParam(CLogEvent & aLogEvent, TInt aRConnectionStatusId, const TUid& aDataEventType, const TInt64& aBytesSent, const TInt64& aBytesReceived);
	IMPORT_C TInt LogDataUpdateEvent(TInt aRConnectionStatusId, const TUid& aDataEventType, const TInt64& aBytesSent, const TInt64& aBytesReceived);
	IMPORT_C TInt LogDataUpdateEvent(TInt aRConnectionStatusId, const TUid& aDataEventType);
	IMPORT_C void LogDataNotifyLastEventUpdate(TRequestStatus* aStatus);

	/** from CActive */
	void DoCancel();
	void RunL();
private:
	CEventLogger();
	void ConstructL();
private:
	CLogWrapper* iLogWrap;
	CLogEvent* iCurrentLogEvent;
	CArrayPtrFlat<CLogEvent>* iLogEventQueue;
	RFs iFsEventLog;
	TRequestStatus* iNotificationRequestStatus;
	};

#endif


