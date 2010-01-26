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
// Dial Up Networking Dialog Server Header - Server Side
// 
//

#ifndef __ND_DLGSV_H__
#define __ND_DLGSV_H__

#include <e32base.h>
#include <commdb.h>
#include <dummyagentdialog.h>

/**
@internalComponent
*/
const TInt KMaxReadBufferSize=100;
const TInt KMaxWriteBufferSize=64;

/**
reasons for panic
@internalTechnology
*/
enum TAgentDialogPanic
	{
	EClientBadRequest,
	EClientBadDescriptor,
	EClientBadRecordId,
	EClientPctNotOpen,
	EClientPctAlreadyOpen,
	EClientReadPctOutstandingOnClose,
	EClientDestroyPctNotfcnOutstandingOnClose,
	//
	ESvrMainSchedulerError,
	ESvrCreateServer,
	ESvrStartServer,
	ESvrDllFileNameNotFound,
	ESvrPctNotOpen,
	ESvrClosePctReturnedError,
	ESvrNoReadPctOutstanding,
	ESvrNoDestroyPctNotfcnOutstanding,
	ESvrNotNullOnClose,
	ESvrNoCommsServerProcess,
	ESvrCannotOpenServerProcess,
	ESvrCancelReturnedError,
	ESvrGeneralCallBackNoSession,
	ESvrGeneralCallBackError
	};


/**
server name
@internalTechnology
*/
#define DIALOG_SERVER_NAME		_L("NetDialDialogServer")

/**
@internalTechnology
*/

// A version must be specifyed when creating a session with the server
const TUint KDialogServMajorVersionNumber=1;
const TUint KDialogServMinorVersionNumber=0;
const TUint KDialogServBuildVersionNumber=102;

// needed for creating server thread.
const TUint KDialogStackSize=0x3000;
const TUint KDialogMinHeapSize=0x1000;
const TUint KDialogMaxHeapSize=0x40000;

/**
@internalTechnology
*/

// opcodes used in message passing between client and server
enum TDialogServRqst
	{
	EGetIAP,
	EGetModemAndLocation,
	EWarnNewIAP,
	EGetAuthentication,
	EGetLogin,
	EGetReconnectReq,
	EOpenPct,
	EWritePct,
	EReadPct,
	EDestroyPctNotification,
	EClosePct,
	ECancelGetIAP,
	ECancelGetModemAndLocation,
	ECancelWarnIAP,
	ECancelLogin,
	ECancelAuthenticate,
	ECancelReconnect,
	ECancelReadPct,
	ECancelDestroyPctNotification,
	// JGG added new functions
	EWarnQoS,
	ECancelWarnQoS,
	// JGG test functions
	EObserveDialogs,
	ESetReconnectResponse,
	ESetQoSWarnResponse,
	ECancelObserveDialogs,
	ESetTestNumber
	};

/**
@internalTechnology
*/
enum TDialogServLeave
	{
	ENonNumericString
	};

class CDialogObserverReq;
class CNetDialDialogServer : public CServer2
/**
CNetDialDialogServer

The server class; an active object.
Contains an instance of RServer; a handle to the kernel
server representation which is used to receive messages. 
@internalTechnology
*/
	{
public:
	enum {EPriority=950}; // mpt - need to explain the magic here!
public:
	static CNetDialDialogServer* NewL();
// CServer
	virtual CSession2* NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const;

//
	static TInt ThreadFunction(TAny* aStarted);
protected:
	CNetDialDialogServer(TInt aPriority);
public:
	// JGG added for tests
	TBool iReconnectResponse;
	TBool iQoSWarnResponse;
	CDialogObserverReq* iDialogObserver;
	TInt iTestNumber;
	TInt iSessionCount;
	};


//
// CNetDialDialogSession
//
/*
class CIAPSelection;
class CModemAndLocationSelection;
class CIAPWarning;
class CLogin;
class CAuthenticate;
class CReconnect;
class CPct;
class CCommsDatabase;
class CQoSWarning;
*/
class CNetDialDialogSession : public CSession2
/**
@internalTechnology
*/
	{
public:
	static CNetDialDialogSession* NewL(RThread aClient, CNetDialDialogServer* aServer);
	CNetDialDialogSession(RThread aClient, CNetDialDialogServer* aServer);
	~CNetDialDialogSession();
// CSession
	virtual void ServiceL(const RMessage2& aMessage);
//
	void DispatchMessageL(const RMessage2& aMessage);

//
	void GetModemAndLocationL();
	void GetModemAndLocationCompleteL(TInt aStatus);

//
	void GetIAPL();
	void GetIAPCompleteL(TInt aStatus);
//
	void WarnIAPL();
	void WarnIAPCompleteL(TInt aStatus);
//
	void GetAuthenticationL();
	void CompleteAuthentication(TInt aStatus);
//	
	void GetLoginL();
	void CompleteLogin(TInt aStatus);
//	
	void ReconnectReqL();
	void CompleteReconnectReq(TBool aBool);
//	
	void CheckPctL(const RMessage2& aMessage);
	void OpenPctL();
	void WritePct();
	void ReadPctL();
	static TInt ReadPctCallBack(TAny* aContext);
	void ReadPctComplete(TInt aStatus);
	void DestroyPctNotificationL();
	static TInt DestroyPctCallBack(TAny* aContext);
	void DestroyPctComplete(TInt aStatus);
	void ClosePct();
//
	// JGG added for new dialog
	void WarnQoSL();
	void WarnQoSCompleteL(TInt aStatus);

//
	static TInt GeneralCallBack(TAny* aSession);
	void Cancel(TDialogServRqst aRequest);
protected:
	void PanicClient(TInt aPanic) const;
private:
	void ConstructL();
	void CheckBusyL();
	void CancelEverything();
private:
	TBool iBusy;
	TBool iPctOpen;
	TBool iReadPctOutstanding;
	TBool iDestroyPctNotfcnOutstanding;

	enum TSessionStates
		{
		EGetIAP,
		EGetModemAndLocation,
		EWarnIAP,
		ELogin,
		EAuthentication,
		EReconnectReq,
		EPct,
		// JGG added for new dialog
		EWarnQoSState
		} iState;

	RMessage2 iMessage;
	RMessage2 iDestroyNotificationMessage;
	RMessage2 iReadPctMessage;

	CNetDialDialogServer* iNetDialDialogSvr;

	//CIAPSelection* iIAPSelection;
	//CModemAndLocationSelection* iModemAndLocationSelection;
	//CIAPWarning* iIAPWarning;
	//CLogin* iLogin;
	//CAuthenticate* iAuthenticate;
	//CReconnect* iReconnect;
	//CPct* iPct;
	// JGG added for new dialog
	//CQoSWarning* iQoSWarning;

	TPckgBuf<TUint32> iIAP;
	TPckgBuf<TUint32> iModemId;
	TPckgBuf<TUint32> iLocationId;
	TPckgBuf<TIspConnectionNames> iConNames;
	TPckgBuf<TConnectionPrefs> iPrefs;
	TPckgBuf<TBool> iBool;
	TBuf<KCommsDbSvrMaxFieldLength> iNewIAPName;
	TBuf<KCommsDbSvrMaxFieldLength> iUsername;
	TBuf<KCommsDbSvrMaxFieldLength> iPassword;
	HBufC* iBuffer;
	TPtr iBufPtr;
	TPtr iPctWriteBuffer;
	TBuf<KMaxReadBufferSize> iPctReadBuffer;
	};

class CDialogObserverReq : public CBase
/**
@internalComponent
*/
	{
public:
	CDialogObserverReq(const RMessage2& aMessage);
	~CDialogObserverReq();
	void Complete();
	void Cancel();
private:
	RMessage2 iOutstandingReq;
	};

/**
Global functions
@internalTechnology
*/
GLREF_C void PanicServer(TAgentDialogPanic aPanic);

#endif
