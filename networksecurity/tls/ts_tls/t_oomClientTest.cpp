// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 
#include <e32base.h>
#include <e32cons.h>
#include <c32comm.h>
#include <f32file.h>
#include <es_sock.h>
#include <securesocket.h>
#include "t_oomClientTest.h"

ClientOOMTest* ClientOOMTest::NewL(CActiveScheduler* anActiveScheduler)
	{
	ClientOOMTest* self = new(ELeave) ClientOOMTest(anActiveScheduler);
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();		
	return self;
	}

void ClientOOMTest::ConstructL()
	{
	// Add this object to the active scheduler
	CActiveScheduler::Add( this );
	}

ClientOOMTest::ClientOOMTest(CActiveScheduler* anActiveScheduler) : CActive(CActive::EPriorityStandard)
	{
	iRunState = ClientOOMTest::ECreated;
	iActiveScheduler = anActiveScheduler;
	}

//
// Destructor
//
ClientOOMTest::~ClientOOMTest()
	{
	Cancel();
	delete iTlsSocket;
	iSocketServ.Close();
	}

void ClientOOMTest::Start()
	{
	TRequestStatus* p=&iStatus;	
	// Complete immediately
	User::RequestComplete(p, KErrNone);
	SetActive();
	}

TInt ClientOOMTest::RunError(TInt aError)
{
	iError = aError;
	iActiveScheduler->Stop();
	return KErrNone;
}

void ClientOOMTest::RunL()
{
   if ( iRunState != ERecvPagePending || iStatus.Int() != KErrEof )
   {//KErrEof on data reception is not an error
	   User::LeaveIfError(iStatus.Int());
   }
	_LIT(KSSLProtocol,"tls1.0");
	switch(iRunState)
	{
	case ClientOOMTest::ECreated:
			// Connect the socket server
			User::LeaveIfError(iSocketServ.Connect());
			// Open the socket
			User::LeaveIfError( iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp ) );	
			//Connect the socket
			connectInetAddr.Input( iAddress );
			//connectInetAddr.SetAddress(KTestAddress);
			connectInetAddr.SetPort(iPortNumber);  //TLS port

			iSocket.Connect( connectInetAddr, iStatus );	
			iRunState = EConnectPending;
			SetActive();
			return;

	case ClientOOMTest::EConnectPending:
			
			// Set the heap to fail on the Nth (iOOMThreshold) request 
			__UHEAP_FAILNEXT(iOOMThreshold);
			// Construct the Tls socket
			User::LeaveIfNull(iTlsSocket = CSecureSocket::NewL( iSocket,KSSLProtocol()));
			//=========================================================
			// start the handshake 
 			iTlsSocket->StartClientHandshake( iStatus );
			iRunState = EHSPending;
			SetActive();
			return;
	case ClientOOMTest::EHSPending:

			iSndBuffer.Copy( _L("GET ") );
			iSndBuffer.Append( _L("\n") );
			//=========================================================
			// send the request
			iTlsSocket->Send( iSndBuffer, iStatus );			
			iRunState = ESendGetRequestPending;
			SetActive();
			return;

	case ClientOOMTest::ESendGetRequestPending:

			iTlsSocket->RecvOneOrMore( iRcvBuffer, iStatus, aLen );
			iRunState = ERecvPagePending;
			SetActive();
			return;

	case ClientOOMTest::ERecvPagePending:

			iTlsSocket->Close();
			iRunState = KErrNone;
			iActiveScheduler->Stop();
			return;

	default:
			User::Leave(KErrNotSupported);
		}

}

