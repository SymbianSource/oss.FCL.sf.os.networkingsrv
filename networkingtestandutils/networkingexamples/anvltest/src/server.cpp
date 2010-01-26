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


#include <e32std.h>
#include <e32base.h>
#include <f32file.h>

#include "server.h"

_LIT(KLitPanicText, "ANVLSERVER");

enum TPanic
    {
    EPanicNegativeMaximumNumberOfSimultaneousServerRequests=1,
    EPanicNegativeNumberOfSessions,
    EPanicNeitherRequestStatusCompleted,
    EPanicNotAdded,
    EPanicUnexpectedError1=1000,
    EPanicUnexpectedError2=2000,
    EPanicUnexpectedError3=3000
    };

LOCAL_C void Panic(TInt aPanic)
    {
    User::Panic(KLitPanicText, aPanic);
    }


RBrdBroadcastServerSession::RBrdBroadcastServerSession()
    {
    }

void RBrdBroadcastServerSession::ConstructL(TInt aMaximumNumberOfSimultaneousServerRequests, TBool /*aSimulateMemoryAllocationFailure*/)
    {

    TInt error;
    error=CreateSession(aMaximumNumberOfSimultaneousServerRequests);
    if (error!=KErrNone)
        {
        }
    User::LeaveIfError(error);
    }

TVersion RBrdBroadcastServerSession::Version() const
    {
    return TVersion(EBrdMajorVersionNumber, EBrdMinorVersionNumber, EBrdBuildNumber);
    }

void RBrdBroadcastServerSession::PrintInt(TInt number)
    {

    
	TIpcArgs arguments(number);
        
	const TInt error=SendReceive(EAnvlPrintInt, arguments);
    
	__ASSERT_ALWAYS(error==KErrNone, Panic(EPanicUnexpectedError1+error));
    }

void RBrdBroadcastServerSession::PrintStr(char *str)
    {

	TPtrC8  desStr((TUint8*)str);
    
	TIpcArgs arguments(&desStr);
    
	const TInt error=SendReceive(EAnvlPrintStr, arguments);
    
	__ASSERT_ALWAYS(error==KErrNone, Panic(EPanicUnexpectedError1+error));
    }

void RBrdBroadcastServerSession::PrintfStr(char *format, char *str)
    {
    
	TPtrC8  desFormat((TUint8*)format);
    TPtrC8  desStr((TUint8*)str);
	TIpcArgs arguments(&desFormat,&desStr);
   
	const TInt error=SendReceive(EAnvlPrintfStr, arguments);
    
	__ASSERT_ALWAYS(error==KErrNone, Panic(EPanicUnexpectedError1+error));
    }

void RBrdBroadcastServerSession::PrintfInt(char *format, TInt number)
    {
    
    TPtrC8  desFormat((TUint8*)format);

	TIpcArgs arguments(&desFormat,number);

	const TInt error=SendReceive(EAnvlPrintfInt, arguments);
    
	__ASSERT_ALWAYS(error==KErrNone, Panic(EPanicUnexpectedError1+error));
    }

TInt RBrdBroadcastServerSession::NumberOfSessions() const
    {
      
	TIpcArgs arguments(TIpcArgs::ENothing);
	const TInt numberOfSessions=SendReceive(EAnvlNumberOfSessions, arguments);
    
	__ASSERT_ALWAYS(numberOfSessions>=0, Panic(EPanicNegativeNumberOfSessions));
    return numberOfSessions;
    }

TInt RBrdBroadcastServerSession::CreateSession(TInt aMaximumNumberOfSimultaneousServerRequests)
    {
    return RSessionBase::CreateSession(KLitServerName, Version(), aMaximumNumberOfSimultaneousServerRequests);
    }

void RBrdBroadcastServerSession::StartServerL(TBool /*aSimulateMemoryAllocationFailure*/)
    {
	}

// _LIT(KLitPanicServerText, "ANVL - server");
// _LIT(KLitPanicClientText, "ANVL - client");

CBrdServer *StartServerL(CAnvltestEngine *aControl)
    {
    CBrdServer *server;
    //CBrdActiveScheduler::NewLC();
    server = CBrdServer::NewLC(aControl);
    //CActiveScheduler::Start();
    //CleanupStack::PopAndDestroy(2); // server and active-scheduler (the latter also gets de-installed)
    return server;
    }


CBrdServer* CBrdServer::NewLC(CAnvltestEngine *aControl)
    {
    CBrdServer* server=new(ELeave) CBrdServer(aControl);
    //CleanupStack::PushL(server);
    server->ConstructL();
    return server;
    }

CBrdServer::~CBrdServer()
    {
    }

TInt CBrdServer::NumberOfSessions()
    {
    iSessionIter.SetToFirst();
    TInt numberOfSessions=0;
    while (iSessionIter++)
        ++numberOfSessions;
    return numberOfSessions;
    }

// CBrdServer::CBrdServer(CAnvltestEngine *aControl)
//    :CServer(EPriorityStandard), iControl(aControl)
CBrdServer::CBrdServer(CAnvltestEngine *aControl)
    :CServer2(EPriorityStandard), iControl(aControl)

    {
    __DECLARE_NAME(_S("CBrdServer"));
    }

void CBrdServer::ConstructL()
    {
    StartL(KLitServerName);
    }

// CSharableSession* CBrdServer::NewSessionL(const TVersion& /* aVersion */) const
CSession2* CBrdServer::NewSessionL(const TVersion& /* aVersion */, const RMessage2& /* aMessage */) const
    {

	// CBrdSession* session=CBrdSession::NewL(Message().Client());
    CBrdSession* session=CBrdSession::NewL(/* aMessage.Client(clientThread) */);

	// clientThread.Close();

	return session;
    }

// CBrdSession* CBrdSession::NewL(RThread aClient)
CBrdSession* CBrdSession::NewL(/* TInt aClient */)
    {
	
    // CBrdSession* session=new(ELeave) CBrdSession(aClient);
    
    CBrdSession* session=new(ELeave) CBrdSession(/* aClient */);
    
	CleanupStack::PushL(session);
    session->ConstructL();
    CleanupStack::Pop(); // session
    return session;
    }

CBrdSession::~CBrdSession()
    {
    //((CBrdServer*)Server())->SessionIsBeingDeleted();
    }


// CBrdSession::CBrdSession(RThread aClient)
//    :CSession(aClient)

CBrdSession::CBrdSession(/* TInt aClient */)
    :CSession2()
    {
    __DECLARE_NAME(_S("CBrdSession"));
    }

void CBrdSession::ConstructL()
    {
    }

void CBrdSession::PrintStr(TPtr8 &bufPtr)
    {
    CBrdServer *server = (CBrdServer*)Server();
    TInt lth = bufPtr.Length();
    
	HBufC* uniBuf=HBufC::NewL(lth*2);
     
	TPtr uniBufPtr = uniBuf->Des();
    uniBufPtr.Copy(bufPtr);
    server->iControl->ShowText(uniBufPtr);
    server->iControl->ShowText(_L("\n"));
    delete uniBuf;
    uniBuf = NULL;
    
    }

void CBrdSession::PrintfStr(TPtr8 &bufPtr, TPtr8 &bufPtr2)
    {
    CBrdServer *server = (CBrdServer*)Server();
    
    TInt lth = bufPtr.Length();
    HBufC* uniBuf=HBufC::NewL(lth*2);

    CleanupStack::PushL(uniBuf);
    
    TPtr uniBufPtr = uniBuf->Des();
    uniBufPtr.Copy(bufPtr);
    
    lth = bufPtr2.Length();
    HBufC* uniBuf2=HBufC::NewL((lth*2)+2);
    
    CleanupStack::PushL(uniBuf2);
    
    TPtr uniBufPtr2 = uniBuf2->Des();
    uniBufPtr2.Copy(bufPtr2);
    
    server->iControl->WriteText(uniBufPtr, uniBufPtr2.PtrZ());
    server->iControl->ShowText(_L("\n"));
    
    CleanupStack::Pop();
    CleanupStack::Pop();
    
    delete uniBuf;
    uniBuf = NULL;        
    delete uniBuf2;
    uniBuf2 = NULL;
    
    }

void CBrdSession::PrintfInt(TPtr8 &bufPtr, TInt number)
    {
    CBrdServer *server = (CBrdServer*)Server();
    
    TInt lth = bufPtr.Length();
    HBufC* uniBuf=HBufC::NewL(lth*2);
    TPtr uniBufPtr = uniBuf->Des();
    uniBufPtr.Copy(bufPtr);
    
    server->iControl->WriteText(uniBufPtr, number);
    server->iControl->ShowText(_L("\n"));
    delete uniBuf;
	uniBuf = NULL;
            
    }

// void CBrdSession::ServiceL(const RMessage& aMessage)
void CBrdSession::ServiceL(const RMessage2& aMessage)
    {

    CBrdServer *server = (CBrdServer*)Server();
    
    switch (aMessage.Function())
        {
        case EAnvlPrintInt:
            server->iControl->WriteText(_L("int=%d\n"), aMessage.Int0());
            aMessage.Complete(KErrNone);
            break;
        case EAnvlPrintStr:
            {
			
			TInt desLen = aMessage.GetDesLength(0);

			HBufC8* buf=HBufC8::NewL(desLen);

            TPtr8 bufPtr = buf->Des();

            // Message().ReadL(desPtr,bufPtr);
            
			aMessage.ReadL(0,bufPtr);

			PrintStr(bufPtr);
            aMessage.Complete(KErrNone);
            
            delete buf;
            buf = NULL;
            
            }
            break;
        case EAnvlPrintfStr:
            {

			TInt desLen = aMessage.GetDesLength(0);

			HBufC8* buf=HBufC8::NewL(desLen);
    
    		CleanupStack::PushL(buf);
    		
            TPtr8 bufPtr = buf->Des();
            
			// Message().ReadL(desPtr,bufPtr);

			aMessage.ReadL(0,bufPtr);

			desLen = aMessage.GetDesLength(1);

			HBufC8* buf2=HBufC8::NewL(desLen);
            
            CleanupStack::PushL(buf2);
    		
            TPtr8 bufPtr2 = buf2->Des();
			
            // Message().ReadL(desPtr,bufPtr2);
            
			aMessage.ReadL(0,bufPtr2);

            PrintfStr(bufPtr, bufPtr2);
            aMessage.Complete(KErrNone);

			CleanupStack::Pop();
    		CleanupStack::Pop();
    
            delete buf;
			buf = NULL;
            delete buf2;
			buf2 = NULL;
            
            }
            break;
        case EAnvlPrintfInt:
            {
            
			TInt desLen = aMessage.GetDesLength(0);
			
			HBufC8* buf=HBufC8::NewL(desLen);
           
            TPtr8 bufPtr = buf->Des();
                       
			// Message().ReadL(desPtr,bufPtr);
            
			aMessage.ReadL(0,bufPtr);

            PrintfInt(bufPtr, aMessage.Int1());
            
            aMessage.Complete(KErrNone);
            delete buf;
            buf = NULL;
            
            }
            break;
        case EAnvlNumberOfSessions:
            aMessage.Complete(server->NumberOfSessions());
            break;
        default:
            PanicClient(EPanicUnknownFunction);
            break;
        }
    }

void CBrdSession::PanicClient(TPanic aPanic)
    {

    // Panic(KLitPanicClientText, aPanic);
    Panic(aPanic);
    
	}

