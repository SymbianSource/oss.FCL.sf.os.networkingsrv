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
// listener.cpp - name resolver query dispatcher
//

#include "engine.h"
#include "listener.h"
#include "resolver.h"
#include "servers.h"

#include "dns.h"
#include "dnd.hrh"		// EDndDump
#include "inet6log.h"


class CDndListener;
class CDndReplySender : public CActive
	/**
	* A thin slave class to run the Active object for reply sending.
	*
	* This is only needed to run the Active object for the
	* write direction. All processing occurs in the main
	* CDndListener object.
	*/
	{
	friend class CDndListener;
public:
	CDndReplySender(CDndListener &aMain);
	~CDndReplySender();
private:
	void DoCancel();
	void RunL();

	CDndListener &iMain;
	};

// CDndListener
// ************
class CDndListener : public CActive, public MDndListener
	/**
	* Main "module" implementing the resolver gateway.
	*
	* Function
	*
	* The listener owns several resolver objects. It waits for
	* any request from the applications, and whenever it receives one,
	* it passes the request to the resolver object.
	*
	* Contains
	*
	* @li	A socket for listening to application requests.
	* @li	A pool of MDndResolver objects - to accept and process
	*		multiple requests in parallel.
	* @li	A CDndDnsclient object - which communicates directly with the DNS.
	* @li	A reference to the owner CDndEngine object.
	*/
	{
	friend class CDndReplySender;
public:
	CDndListener(CDndEngine &aControl);
	void ConstructL();
	~CDndListener();
	// for debugging only (not used currently)
	void HandleCommandL(TInt aCommand);
	void PostReply();
private:
	// active object stuff, completion and cancel callback functions
	void RunL();
	void DoCancel();
	void KeepRunning();
	void SenderReady();
	void CancelSend();
	//this will handle leaves that happens within RunL method.
	//and thus does necessary cleanup.
	TInt RunError(TInt aError);

	RSocket iSocket;
	CDndEngine &iControl;

	/**
	// The resolver pool.
	//
	// When preparing for the next Accept() this array
	// is searched for inactive instance (!IsBusy()) to
	// be associated with the Accept(). If none is found
	// and there are empty slots left, a new resolver
	// instance is created. If none found, the listener
	// goes inactive until one of the resolvers becomes
	// available.
	*/
	MDndResolver *iResolver[KDndNumResolver];

	/**
	// CDndDnsclient object is responsible for providing the actual
	// DNS protocol service. The CDndListener itself only owns and
	// creates this service and passes the reference to the
	// resolver instances.
	*/
	CDndDnsclient *iDnsclient;

	// The server and interface manager
	MDnsServerManager *iServerManager;

	CDndReplySender *iReplySender;

	TDnsMessageBuf iMsg;
	};


// CDndReplySender implementation
// ******************************
CDndReplySender::CDndReplySender(CDndListener &aMain) :  CActive(0), iMain(aMain)
	{
	CActiveScheduler::Add(this);
	}

CDndReplySender::~CDndReplySender()
	{
	Cancel();
	}

void CDndReplySender::DoCancel()
	{
	iMain.CancelSend();
	}
		
void CDndReplySender::RunL()
	{
	iMain.SenderReady();
	}


// MDndListener instantiation
// **************************
MDndListener *MDndListener::NewL(CDndEngine &aControl)
	/**
	* Create and return CDndListener interface.
	*/
	{
	CDndListener *p = new (ELeave) CDndListener(aControl);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop();
	return p;
	}

// CDndListener, real implementation
// *********************************
CDndListener::CDndListener(CDndEngine &aControl) : CActive(0), iControl(aControl)
	{
	LOG(Log::Printf(_L("CDndListener::CDndListener(%x)"), &aControl));
	CActiveScheduler::Add(this);
	}

// CDndListener::ConstructL
// ************************
void CDndListener::ConstructL()
	{
	LOG(Log::Write(_L("CDndListener::ConstructL()")));

	iControl.CheckResultL
		(_L("Opening socket to IPv6 'resolver' gate"),
		iSocket.Open(iControl.iSS, _L("resolver")));
	iControl.CheckResultL(_L("Setopt sessions"), iSocket.SetOpt(KSoDndSessions, KSolDnd, KDndNumResolver));
	iReplySender = new (ELeave) CDndReplySender(*this);
	iServerManager = DnsServerManager::NewL(iControl);

	// Create and initialize the Dnsclient object.
	iDnsclient = CDndDnsclient::NewL(iControl, *iServerManager);
	KeepRunning();
	}

// CDndListener::~CDndListener
// ***************************
CDndListener::~CDndListener()
	{
	LOG(Log::Write(_L("CDndListener::~CDndListener()")));

	// Stop all AO activities (if any)
	Cancel();

	// Destroy all resolvers
	for (TUint i = 0; i < sizeof(iResolver) / sizeof(iResolver[0]); ++i)
		delete iResolver[i];

	delete iReplySender;

	delete iDnsclient;

	// Server Manager can be deleted only after all other objects
	// referencing it have been deleted!
	delete iServerManager;


	// Close the listening socket
	iSocket.Close();

	// Deque from the active scheduler
	if (IsAdded())
		Deque();
	}

// CDndListener::HandleCommandL
// ****************************
// (this should probably be removed--not used now)
void CDndListener::HandleCommandL(TInt aCommand)
	{
	if (iDnsclient)
		iDnsclient->HandleCommandL(aCommand);
	}

void CDndListener::RunL()
	{
	LOG(Log::Printf(_L("--> CDndListener::RunL() -start- iStatus=%d len=%d"), iStatus.Int(), iMsg.Length()));
	if (iStatus.Int() == KErrNone)
		{
		// A message from the stack has been received, dispatch it to the appropriate
		// resolver instance.
		//
		TInt available = -1;
		const TUint16 session = iMsg().iSession;
		if (session == 0 || iMsg.Length() < iMsg().HeaderSize())
			{
			LOG(Log::Printf(_L("\tSESSION == 0, type=%d, length=%d"), iMsg().iType, iMsg.Length()));
			if (iMsg().iType == KDnsRequestType_Configure)
				{
				// Notify components that want to know about it...
				iServerManager->ConfigurationChanged();
				iDnsclient->ConfigurationChanged();
				}
			goto done;
			}
		for (TUint i = 0; i < KDndNumResolver; ++i)
			{
			if (iResolver[i] == NULL)
				{
				if (available < 0)
					available = i;
				}
			else if (iResolver[i]->Session() == session)
				{
				// *NOTE* To be exact, one should here check whether a
				// reply is currently being sent from the buffer of this
				// resolver, because the new request will overrite the
				// old content. HOWEVER, the resolver GW is not sending
				// a new request to a session which is waiting a reply.
				// Thus, the fact that a new request appears here, means
				// that the reply buffer has been copied, and the DND
				// ActiveScheduler just has not yet called the Sender
				// RunL [this seems to happen regularly]. Doing sender
				// Cancel() would be just extra overhead.
				iResolver[i]->Start(iMsg);
				goto done;
				}
			else if (iResolver[i]->Session() == 0)
				{
				available = i;	// Prefer existing session.
				}
			}
		if (iMsg.Length() <= iMsg().HeaderSize())
			{
			// This is a cancel message without corresponding
			// resolver session. Just ignore it.
			}
		else if (available < 0)
			{
			// No free slots ...what now?
			// Should not happen, because gateway is not supposed to
			// give more work than has been allowed.
			}
		else
			{
			if (iResolver[available] == NULL)
				{
				iResolver[available] = MDndResolver::New(available, iControl, *this, *iDnsclient);
				}
			iResolver[available]->Start(iMsg);
			}
		}
	else
		{
		iControl.ShowTextf(_L("\tresolver socket error %d"), iStatus.Int());
		//
		// Shutdown DND if the server has terminated as otherwise noone will
		//
		if (iStatus.Int() == KErrServerTerminated)
			{
			CActiveScheduler::Stop();
			return;
			}
		}
done:
	KeepRunning();			// Keep Listener Running
	LOG(Log::Printf(_L("<-- CDndListener::RunL() -exit- iStatus=%d"), iStatus.Int()));
	}

void CDndListener::DoCancel()
	{
	LOG(Log::Write(_L("CDndListener::DoCancel()")));
	iSocket.CancelRecv();
	}


//	CDndListener::KeepRunning()
//	***************************
void CDndListener::KeepRunning()
	/**
	* Keep the receiver active.
	*
	* Activates the receiver, if not already active.
	*/
	{
	if (IsActive())
		return;				// I'm already active, nothing to do
	iSocket.Recv(iMsg, 0, iStatus);
	SetActive();
	}


// CDndListener::CancelSend
// ************************
void CDndListener::CancelSend()
	{
	iSocket.CancelSend();
	}
	
	
// CDndListener::PostReply
// ***********************
void CDndListener::PostReply()
	/**
	* Called when a resolver has a reply ready to be returned.
	*
	* The writer will eventually ask the resolver for the buffer
	* using the MdndResolver::ReplyMessage. This should return
	* ZERO length descriptor, if there is no reply to send.
	*/
	{
	if (!iReplySender->IsActive())
		SenderReady();	
	}
	
// CDndListener::SenderReady
// *************************
void CDndListener::SenderReady()
	/**
	* Sender is ready for next reply.
	*
	* Keeps reply sender running while any of the resolvers have
	* replies to be delivered.
	*/
	{
	for (TUint i = 0; i < KDndNumResolver; ++i)
		{
		if (iResolver[i])
			{
			const TDesC8 &buf = iResolver[i]->ReplyMessage();
			if (buf.Length() > 0)
				{
				LOG(Log::Printf(_L("\tresolver[%d] SESSION %d Write(%d)"), i, iResolver[i]->Session(), buf.Length()));
				iSocket.Write(buf, iReplySender->iStatus);
				iReplySender->SetActive();
				break;
				}
			}
		}
	}
/*
*  This function shall be called if there is any kind leave happens within
*  RunL method. This function will be invoked by Active Shceduler if Active
*  Objects RunL() method leaves.Thus it will give opportunity to active objects 
*  RunL method  to perform any sort of cleanup.
*  @param aError-  will have panic number or error number
*  @return - Returns KErrNone 
*/
TInt CDndListener::RunError(TInt /*aError*/)
{
	return KErrNone;
}
