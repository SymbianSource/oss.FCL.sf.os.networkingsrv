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


#if !defined(__SERVER_H__)
#define __SERVER_H__

#if !defined(__E32STD_H__)
#include <E32STD.H>
#endif

#include "engine.h"

class RBrdBroadcastServerSession : public RSessionBase
    {
public:
    RBrdBroadcastServerSession();
    void ConstructL(TInt aMaximumNumberOfSimultaneousServerRequests, TBool aSimulateMemoryAllocationFailure);
    TVersion Version() const;
    void PrintInt(TInt number);
    void PrintStr(char *str);
    void PrintfStr(char *format, char *str);
    void PrintfInt(char *format, TInt number);
    TInt NumberOfSessions() const;
private:
    TInt CreateSession(TInt aMaximumNumberOfSimultaneousServerRequests);
    static void StartServerL(TBool aSimulateMemoryAllocationFailure);
    };

_LIT(KLitServerName, "0x1ffb1467-AnvlServer");

enum {EBrdMajorVersionNumber=1};
enum {EBrdMinorVersionNumber=5};
enum {EBrdBuildNumber=4};

enum
    {
    EAnvlPrintInt,
    EAnvlPrintStr,
    EAnvlPrintfInt,
    EAnvlPrintfStr,
    EAnvlNumberOfSessions
    };

const TUint KBrdSimulateMemoryAllocationFailure=0x80000000;

class TBrdServerInitializationParameters
    {
public:
    inline TBrdServerInitializationParameters(TThreadId aThreadId, TRequestStatus& aRequestStatus, TBool aSimulateMemoryAllocationFailure) :iThreadId(aThreadId), iRequestStatus(&aRequestStatus), iSimulateMemoryAllocationFailure(aSimulateMemoryAllocationFailure) {}
    inline TThreadId ThreadId() const {return iThreadId;}
    inline TRequestStatus*& RequestStatus() {return iRequestStatus;}
    inline TBool SimulateMemoryAllocationFailure() {return iSimulateMemoryAllocationFailure;}
private:
    TThreadId iThreadId;
    TRequestStatus* iRequestStatus;
    TBool iSimulateMemoryAllocationFailure;
    };


// class CBrdServer : public CServer
class CBrdServer : public CServer2
    {
public:
    static CBrdServer* NewLC(CAnvltestEngine *aControl);
    virtual ~CBrdServer();
    TInt NumberOfSessions();
private:

private:
    CBrdServer(CAnvltestEngine *aControl);
    void ConstructL();
    
	// virtual CSharableSession* NewSessionL(const TVersion& aVersion) const;
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;

public:
    CAnvltestEngine *iControl;
    
private:

    };

// class CBrdSession : public CSession
class CBrdSession : public CSession2 
   {
public:
    // static CBrdSession* NewL(RThread aClient);
    static CBrdSession* NewL(/* TInt aClient */);
	virtual ~CBrdSession();
private:
    enum TPanic
        {
        EPanicUnknownFunction=1,
        };

private:
    // CBrdSession(RThread aClient);
	CBrdSession(/* TInt aClient */);
    
	void ConstructL();
    void PrintStr(TPtr8 &bufPtr);
    void PrintfStr(TPtr8 &bufPtr, TPtr8 &bufPtr2);
    void PrintfInt(TPtr8 &bufPtr, TInt number);
    
	// virtual void ServiceL(const RMessage& aMessage);
	virtual void ServiceL(const RMessage2& aMessage);

    void PanicClient(TPanic aPanic);
private:

    };

CBrdServer *StartServerL(CAnvltestEngine *aControl);


#endif

