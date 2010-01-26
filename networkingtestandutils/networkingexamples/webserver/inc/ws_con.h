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
// ws_con.h - http server connection manager
//



/**
 @internalComponent
*/
#ifndef __WS_CON_H
#define __WS_CON_H

#include <in_sock.h> // Standard Sockets Class

#include <e32std.h>  //
#include <f32file.h> // TFileText, RFile & RFs (file system Session).


#include "ws_mes.h" // CHttpMessage Class
#include "ws_tim.h" // CWebServerTimer Class
#include "ws_cgi.h" // CExecCGI definition inteface.


#define BUFFER_SIZE		2048 //CHECK IT
#define RESOURCE_SIZE	512
#define BACKUP_MESSAGE_SIZE 2048
#define FILENAME_SIZE   32
#define BOUNDARY_SIZE   32
#define REMOTE_ADDRESS_SIZE 64
#define THREADNAME_SIZE 11

#ifdef __SECURE_DATA__
#define LOG_FILENAME _L("webserver.log")
#else
#define LOG_FILENAME _L("\\data\\webserver.log")
#endif // __SECURE_DATA__

#define TMP_INPUT	_L("tmpWSIn.tmp")
#define TMP_OUTPUT _L("tmpWSOut.tmp")


#define RETRY_AFTER_TIME _L("60")

const TInt	 KErrTimeOut = -998;
const TInt   KTimeOutTime = 60000000;
const TInt   KHeapSize = 0x800;

class CWebServerTimer;
class CWebServerCon;
class CWebServerEng;
class CWebServerEnv;
class CTimeOutTimer;
class CExecCGI;
class MWebServerObserver;


#include "ws_eng.h"

class CWebServerCon : public CActive
{
public:
	static CWebServerCon* NewL(CWebServerEng* aWebServer);
	~CWebServerCon();
public: // For the Observer.
	TDes& StatusText() { return iStatusText; }
	void SetObserver(MWebServerObserver* aObserver);
	void StartConnection();
	void WebServerBusyL();
	void ConnectionTimeOutL();
	void SignalRunL(TInt aError);
public: // CGI
	static TInt ThreadFunction(TAny* aArg);
public: // For the connection list in the WebServerEng.
	static const TInt iOffset;
private:
	CWebServerCon();
	void ConstructL(CWebServerEng* aWebServer);
private: // From Active. (Definition of the virtual functions)
	void RunL();
	void DoCancel();
private:
	TInt BackupResource();
	void ExecCGIL();
	void DeleteMethodL();
	void GetHttpMessageL();
	void GetMethodL();
	void GetPostBodyL();
	void NotifyingDataSent();
	void OptionsMethodL();
	void ParseHeaderCGIL();
	void PostMethodL();
	void ProcessCGIHeaderL(HBufC* aBuffer);
	void ProcessingRequestL();
	void PutMethodL();
	void PutURIL();
	void RangeSentL();
	void ReSendData();
	void SendCGIOutputL();
	void SendErrorMessageL(TUint ErrorType);
	void SendGetResponseL();
	void SendRangeURI();
	void SendRangeHeadersL();
	void SendTraceBody();
	void SetResourceL();
	void SendURI();
	TInt SetCgiEnv();
	void TraceMethodL();
	void WaitingForRequest();
public:
	void ShuttingDownConnection(RSocket::TShutdown aHow);
private: //Logging facilities
	void ShowMiscData();
	void ShowHttpRequest();
	void ShowHttpResponse();
	void NotifyStatus();
private:
	enum TConState
	{
		EWaitingRequest,EReceivingRequest,EReceivedRequest,ESendingGet/*,ESendRangeHeaders*/,ESendingRange,ERangeSent,
		EPuttingURI,ESendingTrace,EShuttingDownCon,ECloseConnection,ESendingError,/*EShutDownCon,EFConnection,*/EGettingPostBody,EProcessingCGI,EFinnished/*,
		ETimeOut,ENotifyDataSent,EHttpError,ENotifyTraceDataSent*/
	};
private:
	TSglQueLink iConLink;	// Link to a single-link-list holded by CWebServerEng to clean up connections after a Cancel from the user.
	CWebServerEng* iWebServer;
private: // The environment variables. (Defautl paths, default resource, etc...)
	CWebServerEnv* iServerEnv;
private:
	RSocket* iSocket;
	TConState iConState;
private: // First, idea to handle the incremental sending of the document.
	RFs ifsSession;
	RFile iFile;
private: //Logging stuff
	MWebServerObserver* iObserver;  // Just for debugging.
	TInt iConId;
	TBool iMiscData;
	TBool iHttpResp;
	TBool iHttpReq;
	TBuf<1024> iStatusText;		// Just for debugging.
private: // For timers
	TTimeIntervalMicroSeconds32 TimeOutTime;
private:
	HBufC8* iRawMessage;
	HBufC8* iPreviousData;
	TBuf8<BUFFER_SIZE> iData;
	TBuf8<BUFFER_SIZE> iCurrentData;
	TSockXfrLength iLen;
private: // member variables used for Ranged requests.
	TInt iCurrentPosition;
	TInt iLastPosition;
	TInt iSize;
	TBuf<BOUNDARY_SIZE> iBoundary;
private:
	HBufC* iResource;
private:
	CHttpMessage iMessage;
private:
	TBool iExists; // To know which kind of response we should do to a PUT Request.
	TInt iURISize; // To know how much data have been written in the file (Put Method)
	TInt iURISizeMax;
	CTimeOutTimer* iTimer;
private:// For POST method.
	RThread thread;
	RLibrary library;
	CExecCGI* cgi;
	CCommandArg comarg;
	TCgiEnv iCgiEnv;
	TBuf<FILENAME_SIZE> iTmpInput;
	TBuf<FILENAME_SIZE> iTmpOutput;
private: //Debugging.
	TInt datalen;
};

#endif
