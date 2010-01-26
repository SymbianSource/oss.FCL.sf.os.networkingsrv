// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __CONNECTIONUTILS_H__
#define __CONNECTIONUTILS_H__

#include <in_sock.h>
#include <es_enum.h>
#include <commdbconnpref.h>

#ifdef _USE_QOS
#include <networking/qoslib.h>
#include <umtsapi.h>
#endif

#include "mlogger.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qos_extension.h>
#include <networking/qoslib_internal.h>
#endif

#define KStdBufSize 1000
#define KTtcpBufSize 1892

#define KBufCount 2
#define KWaitMicroSecs 1000000

class CProgress : public CActive
	{
public:
	CProgress(RConnection& aConnection, CLogger& aLogger, TCommDbConnPref& aPref);
	~CProgress();
	void Progress(TRequestStatus* aStatus = NULL);

private:
	void RunL();
	void DoCancel();

	RConnection& iConn;
	TNifProgressBuf iProgress;
	TCommDbConnPref iPref;

	TRequestStatus* iSavedStatus;
	CLogger& iLogger;
	};

class CReaderWriter : public CActive
	{
public:
	enum TIsReader {EReader, EWriter};
	enum TIsQuick {EQuick, ENormal};

	CReaderWriter(RSocket& aSocket, TIsReader aIsReader);
	void DoTransfer(TUint aBufSize, TIsQuick aIsQuickie = ENormal, TRequestStatus* aStatus = NULL);

	~CReaderWriter();
	TInt BytesTransfered(void);

private:		
	void StartRequest(TInt err, TIsQuick aIsQuickie);	
	void RunL();
	void DoCancel();
private:
	RSocket& iSocket;

	TRequestStatus* iSavedStatus;
	HBufC8* iHeap;
	TPtr8 iBuf;
	TBuf8<50> iScratchData;
	const TIsReader iIsReader;
	TInt iBufCount, iBufSize;
	};

#ifdef _USE_QOS
class CSender : public CActive, private MQoSObserver
#else
class CSender : public CActive
#endif
	{
public:
	enum TAction {EReadWrite, ERead, EWrite};

	CSender(CLogger& aLogger, RSocketServ& aSS,	TCommDbConnPref& aPref);
	~CSender();

#ifdef _USE_QOS
	void DoInitL(const TInetAddr& anAddr, const TInt aPort, RConnection& aConn, CQoSParameters* parameters = NULL);
	void DoInitL(const TInetAddr& anAddr, const TInt aPort, CQoSParameters* parameters = NULL);
#else
	void DoInitL(const TInetAddr& anAddr, const TInt aPort, RConnection& aConn);
	void DoInitL(const TInetAddr& anAddr, const TInt aPort);
#endif

	void StartActive(TRequestStatus* aStatus = NULL, TAction aAction = EReadWrite);
	void StartPassive(TInt aPort, TRequestStatus* aStatus, RConnection& aConn, TAction aAction = ERead);	
	TInt GetResult();

private:
	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();

#ifdef _USE_QOS
	void RequestQosL();
	void Event(const CQoSEventBase& aQoSEvent);

	RQoSChannel iQosChannel;
	TQoSEvent iQosEvent;
	CQoSParameters* iQosParams;
#endif

	RSocket iSock, iListener;
	TInetAddr iRemote, iLocal;
	TRequestStatus* iSavedStatus;
	RSocketServ& iSS;
	const TCommDbConnPref iPref;

	CReaderWriter* iReader;
	CReaderWriter* iWriter;
	enum TState {EConnecting, EReading, ESettingQos};
	TState iState;
	TAction iAction;
	TInt iSendCount;
	CLogger& iLogger;
	TInt iLastError;
	};

#endif //__CONNECTIONUTILS_H__
