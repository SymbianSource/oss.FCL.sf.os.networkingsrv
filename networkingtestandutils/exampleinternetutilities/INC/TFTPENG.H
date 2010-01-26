// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Started by AY, May 1997
// 
//

#include <e32base.h>
#include <es_sock.h>
#include <in_sock.h>
#include <e32test.h>
#include <f32file.h>

enum TOpCode 
	{
	ERRQ=1,EWRQ,EDATA,EACK,EERR
	};

enum TOpComp 
	{
	ETimedOut=1,ESendFail,ERecvFail,EOpComplete,EFileOpenFailure,EError
	};

class TTftpPacket : public TPtr8
	{
public:
	TTftpPacket();
	TBool CheckBlock();
	TInt OpCode();
	void MakeRqst(const TOpCode aOpc, const TDesC& aName, const TDesC& aMode); 
	void MakeAckn();
	TInt MakeData(const TDesC8& aData);  
	void MakeError();//will need error code (will then lookup msg? 
private:
	TUint8 iBuffer[516];
	TUint16 iBlockNum;
	};

class CTftpEngine;

class CTftpTimer: public CTimer
	{
public:
	~CTftpTimer();
	static CTftpTimer* NewL(const TInt aPriority, CTftpEngine& aEngine);
protected:
    CTftpTimer(const TInt aPriority, CTftpEngine& aEngine);
	void ConstructL();
    virtual void RunL();
private:
	CTftpEngine& iEngine;
	};

class MTftpNotifier 
	{
public:
	virtual void OpComplete(TOpComp aResult, TInt aOp)=0;
	virtual void ProgressNotification()=0;
	};

class CTftpEngine: public CActive
	{
private:
	enum TState 
		{
		ESending, EReceiving, EComplete, ESendingLastData, 
		ESendingLastAckn, EReceivingLast
		};
public:
	IMPORT_C ~CTftpEngine();
	IMPORT_C void Get(const TDesC& aNetAddr, const TDesC& aName, const TDesC& aMode = _L("Octet"));
	IMPORT_C void Put(const TDesC& aNetAddr, const TDesC& aName, const TDesC& aMode = _L("Octet"));
	IMPORT_C void TimerExpired();
	IMPORT_C void SetTimeOut(const TInt aTimeOut);
	IMPORT_C static CTftpEngine *NewL(const TInt aPriority, MTftpNotifier& aNotifier);

protected:
    virtual void RunL();
    virtual void DoCancel();
private:
	CTftpEngine(const TInt aPriority, MTftpNotifier& aNotifier);
    void ConstructL();

private:
	RSocketServ iSockServ;
	RSocket iSocket;
	RFs iFs;
	RFile iFile;
	CTftpTimer* iTimer;
	TTftpPacket iPacket;
	TInt iTimeOut;
	TInetAddr iAddress;
	TState iState;
	MTftpNotifier& iNotifier;
	};
