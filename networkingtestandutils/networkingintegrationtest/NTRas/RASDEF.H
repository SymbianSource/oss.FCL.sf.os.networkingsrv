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

#if !defined(__RASDEF_H__)
#define __RASDEF_H__

#include <es_ini.h>
#include <ntras.h>
#include <nifutl.h>
#include <comms-infras/nifagt.h>
#include "chat.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif
/**
@internalComponent
*/
enum TNtRasPanic
	{

	ENtRasPanic_ObjectNotDeleted,
	ENtRasPanic_BadState,	
	};

/**
@internalComponent
*/
void Panic(TNtRasPanic aPanic);

/**
@internalComponent
*/
const TInt KNtRasAsyncCbPriority = 0;
const TInt KKeyReaderPriority = 0;
const TInt KChatterPriority = 0;
const TInt KChatBufferSize = 64;
const TInt KChatTimeout = 3000000;
const TInt KChatMaxTries = 10;
const TInt KCommRxBufSize = 64;
const TInt KCommTimeout = 10000000;
const TInt KCommReadPriority = 150;
const TInt KCommWritePriority = 100;
const TInt KCommTimerPrioiry = 5;
const TInt KCommLineDropTime = 250000;

class CNtRas;

class CRasConnect : public CBase, public MComm, public MTimer, public MCommChatNotify
/**
@internalComponent
*/
	{
	enum TRasConState
		{ ERasConClosed, ERasConDropLines, ERasConConnecting,
		  ERasConTokenSent, ERasConConnected, ERasConDisconnecting };
public:
	CRasConnect(CNtRas* aNtRas);
	~CRasConnect();
	static CRasConnect* NewL(CNtRas* aRas);
	void ConstructL();

	TPtrC8 ExcessData();
	
	// from MComm
	virtual void CommReadComplete(TInt aStatus);
	virtual void CommWriteComplete(TInt aStatus);
	// from MTimer
	virtual void TimerComplete(TInt aStatus);
	// from MCommChatNotify
	virtual void ChatStringMatch(CCommChatString* aString);
	virtual void ChatTimeout();
private:
	void StartL();
	TInt InitComm();
	void Disconnect();
	TInt SendClient();
	void Complete(TInt aStatus);
	void SendToken();
private:
	CNtRas* iNtRas;
	CCommChatter* iChat;
	TInt iRxBufOffset;
	TBuf8<KCommRxBufSize> iRxBuf;
	TRasConState iState;
	TInt iTries;
	TBool iCommOpen;
	TUint32 iServerMode;
    CCommChatString* iString;
	};

class CRasDatabase : public CBase
/**
@internalComponent
*/
	{
public:

	static CRasDatabase* NewL(CNtRas* aRas);
	CRasDatabase(CNtRas* aRas);
	~CRasDatabase();
	TInt ReadField(const TDesC& aField, TUint32& aValue);
	TInt ReadField(const TDesC& aField, TDes8& aValue);
private:
	void ConstructL();
	CESockIniData* iIniFile;
	CNtRas* iNtRas;
	};

class CConnectBox : public CActive
/**
@internalComponent
*/
	{
public:
	CConnectBox(CNtRas* aNtRas);
	~CConnectBox();
	static CConnectBox* NewL(CNtRas* aNtRas, TBool aReconnect);
protected:
	void ConstructL(TBool aReconnect);

	virtual void RunL();
	virtual void DoCancel();
private:
	CNtRas* iNtRas;
	RNotifier iNotifier;
	TBufC<5> iYes;
	TBufC<5> iNo;
	TBufC<20> iLine1;
	TBufC<20> iLine2;
	TInt iButtonVal;
	TBool iReconnect;
	};

class CLoginBox : public CActive
/**
@internalComponent
*/
	{
public:
	CLoginBox(CNtRas* aNtRas);
	~CLoginBox();
	static CLoginBox* NewL(CNtRas* aNtRas, TDes& aUsername, TDes& aPassword);
protected:
	void ConstructL(TDes& aUsername, TDes& aPassword);
	virtual void RunL();
	virtual void DoCancel();
private:
	CNtRas* iNtRas;
	};

class CNtRasFactory : public CNifAgentFactory
/**
@internalComponent
*/
	{
protected:
	virtual void InstallL();
	virtual CNifAgentBase *NewAgentL(const TDesC& aName);
	virtual TInt Info(TNifAgentInfo& aInfo, TInt aIndex) const;
	};

class CNtRas : public CNifAgentBase
/**
@internalComponent
*/
	{
public:
	enum TRasOp
		{
		ERasOpStart,
		ERasOpRestart,
		ERasOpDisconnect,
		ERasOpAuthenticate,
		ERasOpReconnect
		};

	enum TRasState
		{
		ERasClosed,
		ERasSelect,
		ERasReconnect,
		ERasConnect,
		ERasOpen,
		ERasAuthenticate
		};

public:
	CNtRas();
	virtual ~CNtRas();
	void ConstructL();
	
	// from CNifAgentBase
	virtual void Info(TNifAgentInfo& aInfo) const;
	virtual void Connect(TAgentConnectType aType);
	virtual void Connect(TAgentConnectType aType, CStoreableOverrideSettings* aOverrideSettings);
	virtual void CancelConnect();
	virtual void Reconnect();
	virtual void CancelReconnect();
	virtual void Authenticate(TDes& aUsername, TDes& aPassword);
	virtual void CancelAuthenticate();
	virtual void Disconnect(TInt aReason);
	virtual TInt GetExcessData(TDes8& aBuffer);
	virtual TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo);
	virtual TInt IncomingConnectionReceived();
	virtual void GetLastError(TInt& aError);
	virtual TBool IsActive() const;
	virtual TBool IsReconnect() const;
	virtual void SetConnectionSettingsL(const TConnectionSettings& aSettings);
	virtual TConnectionSettings& ConnectionSettingsL();
	virtual void SetOverridesL(CStoreableOverrideSettings* aOverrideSettings);
	virtual CStoreableOverrideSettings* OverridesL();
	virtual void RequestNotificationOfServiceChangeL(MAgentSessionNotify* aSession);
	virtual void CancelRequestNotificationOfServiceChange(MAgentSessionNotify* aSession);
	//
	static void FillInInfo(TNifAgentInfo& aInfo);
	void ConnectDialogComplete(TInt aStatus);
	void AuthenticateDialogComplete(TInt aStatus);
	void ConnectComplete(TInt aStatus);

protected:
	virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoWriteInt(const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage );
	virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoWriteBool(const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage );	
	virtual HBufC* DoReadLongDesLC(const TDesC& aField, const RMessagePtr2* aMessage );


private:
	static TInt AsyCbHandler(TAny* aPtr);
	void DoProcess();
	void Process(TRasOp aOp);
	TInt CommInitialise();

private:
	TRasOp iOp;
	TRasState iState;
	CAsyncCallBack* iAsyCb;
	CRasDatabase* iDatabase;
	CLoginBox* iLoginBox;
	CConnectBox* iConnectBox;
	CRasConnect* iConnector;

	TDes* iUserNamePtr;
	TDes* iPasswordPtr;

	HBufC8* iExcessData;
	TConnectionSettings iSettings;
	};

class Util
/**
@internalComponent
*/
	{
public:
	static TInt GetCommaSepToken(TLex& aLex, TPtrC& aPtr);
	static TInt GetCommHandshake(const TDesC& aDes, TUint& aHandshake);
	static TInt GetCommFormat(const TDesC& aDes, TDataBits& aDataBits, TStopBits& aStopBits, TParity& aParity);
	static TInt GetCommSpeed(TInt aSpeed, TBps& aBps);
	static void CommInit();
	};

/**
@internalComponent
*/
_LIT(RAS_NIF_NAME, "ntras");

#endif
