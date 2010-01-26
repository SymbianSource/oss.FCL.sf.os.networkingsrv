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
// ws_eng.h - http server engine
//



/**
 @internalComponent
*/
#ifndef __WS_ENG_H
#define __WS_ENG_H

#include <e32base.h>
#include <e32std.h>
#include <badesca.h>	 // CDesCArrayFlat class.

#include <es_sock.h>
#include <in_sock.h> // Standard Sockets Class

#if EPOC_SDK < 0x07010000
#if EPOC_SDK >= 0x06010000
    #include <agentclient.h>
#else
    #include <netdial.h>
#endif
#endif
#include <nifmbuf.h>
#include "ws_con.h"
//#include "webserver.rsg"

/*
#ifdef IPV6
	#define TInetAddr TInetAddr
//	#define KAfInet KAfInet6
	#define KInetAddrAny KInet6AddrNone
#endif
*/

// Define from CWebServerEnv
#define DEFAULT_PORT 2080
#define DEFAULT_SERVER_PATH _L("\\ws\\root\\")
#define	DEFAULT_CGI_PATH _L("\\ws\\cgi-bin\\")
#define DEFAULT_ERROR_PATH _L("\\ws\\error\\")
#define DEFAULT_BACKUP_PATH _L("\\ws\\backups\\")
#define DEFAULT_DEFAULT_RESOURCE _L("default.htm")
#define DEFAULT_HISTORY_SIZE 255


#define EXTENSION_SIZE 8
#define CONTENT_SIZE 80

#define MAX_HTTP_TYPES 32
#define MAX_NUMBER_CONNECTIONS 100
#define RECOVERY_TIME 60000000

#define HTTP_TYPES_ARRAY_GRANURALITY 32

const TUint KWSSocketMaxConnections = 10;
const TUint KHttpPort = 2080;


class CHttpTypes :  public CBase
{
public:
	CHttpTypes();
	~CHttpTypes();
public:
	void ReadHttpTypesFileL(const TDesC& aFileName);
	void SaveHttpTypesFileL(const TDesC& aFileName) const;
	void SetHttpTypesArray(CDesCArrayFlat* aHttpTypesArray);
	void SetDefaultHttpTypesL();
	TInt FindContentType(const TDesC& aExtension, TDes& aContentType) const;
	inline CDesCArrayFlat* HttpTypesArray() { return iHttpTypesArray; }
private:
	void ParseTypeLineL(const TDesC& aLine);
private:
	CDesCArrayFlat* iHttpTypesArray;

};






// WebServer Enviroment class.
// This reads a config file and stores the environment values of the WebServer.

class CWebServerEnv : public CBase
{
public:
	CWebServerEnv();
	~CWebServerEnv();
	void ConstructL();
public:
	void ReadConfigFileL(const TDesC& aConfigFileName);
	void SaveConfigFileL(const TDesC& aConfigFileName);
	void CheckAndSetDefaultL();
public: //Set & Check methods
	void SetBackupPathL(const TDesC& aBackupPath);
	void SetCgiPathL(const TDesC& aCgiPath);
	void SetErrorPathL(const TDesC& aErrorPath);
	void SetServerPathL(const TDesC& aServerPath);
	void SetDefaultResourceL(const TDesC& aDefaultResource);
	inline TDesC& BackupPath() { return *iBackupPath;}
	inline TDesC& CgiPath() { return *iCgiPath;}
	inline TDesC& ErrorPath() { return *iErrorPath;}
	inline TDesC& ServerPath() { return *iServerPath;}
	inline TDesC& DefaultResource() { return *iDefaultResource;}
private:
	void ParseConfigLineL(const RFs& aFs, const TDesC& aLine);
private: // These are private because I need to define a set function (to free the previous data)
	HBufC* iBackupPath;
	HBufC* iCgiPath;
	HBufC* iServerPath;
	HBufC* iErrorPath;
	HBufC* iDefaultResource;
public:
	TInt   iPort;
	TInt   iHistorySize;
	TBool  iDeleteMethod;
	TBool  iPutMethod;
	TBool  iHttpReq;
	TBool  iHttpResp;
	TBool  iMiscData;

};



// WebServer Observer class.
// Allows us to trace the behaviour of the Server.

class MWebServerObserver
{
public:
		virtual void HandleProgressEvent() = 0;
		virtual void UpdateNumberConL() = 0;
		virtual void ShowWarningL(const TInt aWarning) = 0;
		virtual void Write(const TDesC &aMsg) = 0;
};


// WebServer Engine class.

class CWebServerEng:public CActive
{
public:
	// Note: Should we declare NewL() static !?
	static CWebServerEng* NewL();
	void StartWebServerL();
	void Reset();
	void DestroyConnectionL(CWebServerCon* aWebServerCon);
	void SetObserver(MWebServerObserver* aObserver);
	void EngTimeOut();
	inline TDes& StatusText() { return iStatusText; }
	~CWebServerEng();
private:
	CWebServerEng();
	void ConstructL();
	void NotifyStatus();
	void CreateSocketAndConnectionL();
private:
	// Definition of the pure virtual function from CActive;
	void RunL();
	void DoCancel();
public:
	enum TWebServerState
	{
		ENotStarted,EWaitingForConnections,EShuttingDownServer,EErrorRecovery
	};
public:
	TWebServerState iWebServerState;
public:
	TBuf<1024> iStatusText;
private:
	RSocketServ iSocketServer;
	RSocket iSocket;				// Listen socket.
	RSocket* iConnection;			// Connection socket.
#if EPOC_SDK >= 0x07010000 
    RConnection iRConnection;         // To establish the connection with the ISP.
#elif EPOC_SDK >= 0x06010000 
    RGenericAgent iGenericAgent;     // To establish the connection with the ISP.
#else
    RNetDial iNetdial;				// To establish the connection with the ISP.
#endif
	CWebServerCon* iCon;			// The next connection.
private:
	TSglQue<CWebServerCon> iConList;//List of ongoing connections.
private:
	MWebServerObserver* iObserver;	
public:
	CWebServerEnv* iServerEnv;		// The enviroment values.
	CHttpTypes iHttpTypes;
public: // Debugging purpouses only.
	TInt iCurrentNumberConnections;
	TInt iTotalNumberConnections;
//	RTimer iErrorRecoveryTimer;
/*	CTimeOut* to;*/
friend class CWebServerCon;
};

#endif
