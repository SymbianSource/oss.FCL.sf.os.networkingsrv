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

#include <e32svr.h>
#include "Et_sstd.h"

//
// CDestroyDummySubSession class definitions
//

CDestroyDummySubSession* CDestroyDummySubSession::NewL(CTelServer* aTelServer, CTelObject* aTelObject)
//
//	Create an async one shot
//
	{
	return new(ELeave)CDestroyDummySubSession(aTelServer,aTelObject);
	}

CDestroyDummySubSession::CDestroyDummySubSession(CTelServer* aTelServer, CTelObject* aTelObject)
//
// C'tor
//
	:CAsyncOneShot(CActive::EPriorityLow), iTelObject(aTelObject), iTelServer(aTelServer)
	{
	__DECLARE_NAME(_S("CDestroyDummySubSession"));
	}

CDestroyDummySubSession::~CDestroyDummySubSession()
	{
	Cancel();
	}

void CDestroyDummySubSession::RunL()
//
// Destroy the tel object and the the server
//
	{
	LOGTEXT(_L8("In RunL and about to Destroy the Tel Objects"));
	iTelObject->RemoveDummySubSessionDestroyer();
	iTelObject->TelObjectClose();
	iTelServer->Dec();
	delete this;
	}

//
// CTelSchedulerStop class definitions
//

CTelSchedulerStop* CTelSchedulerStop::NewL()
//
//	Create the TelScheduler Stop async one shot
//
	{
	return new(ELeave)CTelSchedulerStop;
	}

CTelSchedulerStop::CTelSchedulerStop()
//
// C'tor
//
	:CAsyncOneShot(CActive::EPriorityLow)
	{
	__DECLARE_NAME(_S("CTelSchedulerStop"));
	}

CTelSchedulerStop::~CTelSchedulerStop()
	{
	Cancel();
	}

void CTelSchedulerStop::RunL()
//
// Stop the Active Scheduler (after any libraries have been unloaded)
//
	{
	LOGTEXT(_L8("In RunL and about to stop CActiveScheduler"));
	CActiveScheduler::Stop();
	}

//
// CTelServer class definitions
//

CTelServer* CTelServer::New()
	{
	CTelServer* pS=new CTelServer(EPriority);

	__ASSERT_ALWAYS(pS!=NULL,Fault(EEtelFaultSvrCreateServer));
	TRAPD(r,pS->ConstructL());
	__ASSERT_ALWAYS(r==KErrNone,Fault(EEtelFaultSvrStartServer));
	return pS;
	}

CTelServer::CTelServer(TInt aPriority)
	: CServer2(aPriority,ESharableSessions), iSessionCount(0)
	{
	__DECLARE_NAME(_S("CTelServer"));
	}

void CTelServer::ConstructL()
	{
	//iPhoneManager=CPhoneManager::NewL();
	iSch=CTelSchedulerStop::NewL();
	iPriorityClientSession=NULL;
	StartL(ETEL_SERVER_NAME);
	}

CTelServer::~CTelServer()
	{
	LOGTEXT(_L8("CTelServer::~CTelServer()"));
	//delete iPhoneManager;
	delete iSch;
	delete iContextConfigChangeReq;
	delete iContextStatusChangeReq;
	delete iQoSConfigChangeReq;
	}

CSession2* CTelServer::NewSessionL(const TVersion &aVersion, const RMessage2& /*aMessage*/) const
//
// Create a new client for this server.
//
	{
	TVersion v(KEtelMajorVersionNumber,KEtelMinorVersionNumber,KEtelBuildVersionNumber);
	if (User::QueryVersionSupported(v,aVersion)==FALSE)
		User::Leave(KErrNotSupported);
	
	return (new(ELeave) CTelSession());
	}

void CTelServer::Inc()
//
// Increase session count
//
	{
	iSessionCount++;
	}

TInt CTelServer::Count() const
//
// Session count
//
	{
	return iSessionCount;
	}

void CTelServer::Dec()
//
// Decrement a session stop scheduler if no session
//
	{
	LOGTEXT(_L8("Entered CTelServer::Dec"));
	__ASSERT_ALWAYS((iSessionCount>0),Fault(EEtelFaultNegativeSessionCount));
	iSessionCount--;
	LOGTEXT2(_L8("Session Count is %d"),iSessionCount);
	if(iSessionCount==0)
		{
		LOGTEXT(_L8("Calling AsynOneShot::Call() "));
		iSch->Call();
		}
	LOGTEXT(_L8("Exited from CTelServer::Dec"));
	}

TBool CTelServer::IsPriorityClient(const CTelSession* aSession) const
//
// Check if aSession is the priority client session
//
	{
	return (aSession==iPriorityClientSession);
	}

TInt CTelServer::SetPriorityClient(CTelSession* aSession)
//
// Set aSession as the priority client session if there is not one already.
// Return KErrAlreadyExists if there is one already, KErrNoMemory if the
// heap cannot be allocated.
//
	{
	if (iPriorityClientSession==NULL)
		{
		iPriorityClientSession=aSession;
		return KErrNone;
		}
	return KErrAlreadyExists;
	}

TInt CTelServer::RemovePriorityClient(CTelSession* aSession)
//
// Remove aSession as the priority client if it is the priority client
// Returns KErrAccessDenied if aSession is not the priority client
//
	{
	if (IsPriorityClient(aSession))
		{
		iPriorityClientSession=NULL;
		return KErrNone;
		}
	return KErrAccessDenied;
	}

//
// CTelScheduler class definitions
//

CTelScheduler* CTelScheduler::New()
//
// Create and install the active scheduler.
//
	{
	CTelScheduler* pA=new CTelScheduler;
	__ASSERT_ALWAYS(pA!=NULL,Fault(EEtelFaultMainSchedulerError));
	CTelScheduler::Install(pA);
	return pA;
	}

void CTelScheduler::Error(TInt) const
//
// Called if any Run() method leaves.
//
	{
	Fault(EEtelFaultMainSchedulerError);
	}

GLDEF_C TInt EtelServerThread(TAny* /*anArg*/)
//
// The ETel Server Thread.
//
	{
	__UHEAP_MARK;
	LOGTEXT(_L8("----------New Log----------\015\012"));
	LOGTEXT(_L8("Entered ETel Server thread"));
	LOGTEXTREL(_L8("Entered ETel Server thread"));

	CTrapCleanup* pT;
	if ((pT=CTrapCleanup::New())==NULL)
		Fault(EEtelFaultCreateTrapCleanup);

	//
	// Start the scheduler and then the server
	//
	CTelScheduler* pScheduler = CTelScheduler::New();
	CTelServer* pServer=CTelServer::New();

	User::SetCritical(User::ENotCritical);
	RSemaphore s;
	TInt semRet=s.OpenGlobal(KETelSemaphoreName);

	if (semRet==KErrNone)
		{
		s.Signal();
		s.Close();
		}
	RLibrary lib;
	lib.Load(KEtelDLLName); // to ensure the access count on the library is always >0
										// while the ETel thread is running
	CTelScheduler::Start();
	LOGTEXT(_L8("ETel:\tScheduler has been stopped\n"));

	delete pT;
	delete pServer;
	delete pScheduler;

	LOGTEXT(_L8("ETel:\tAbout to exit ETel thread function\n"));
	LOGTEXTREL(_L8("ETel:\tAbout to exit ETel thread function\n"));
	__UHEAP_MARKEND;

	return(KErrNone);
	}
