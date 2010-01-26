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

// include
#include <es_sock.h>

#include "TeSocketBase.h"

const TInt KDefaultFlags		= 0;

CTestSocketBase* CTestSocketBase::NewL(CTestListenerMgr* aListenerMgr, CTestStepBase* aTestStepBase, 
TSocketType aSocketType)
	{
	CTestSocketBase* self = new (ELeave) CTestSocketBase(aListenerMgr, aTestStepBase);
	CleanupStack::PushL(self);
	self->ConstructL(aSocketType);
	CleanupStack::Pop(self);
	return self;
	}

CTestSocketBase::~CTestSocketBase()
/**
	Destructor.
*/
	{
	iSocket.Close();
	delete iTestStepBase;
	delete iListenerMgr;
	}

CTestSocketBase::CTestSocketBase(CTestListenerMgr* aListenerMgr, CTestStepBase* aTestStepBase)
: iListenerMgr(aListenerMgr), iTestStepBase(aTestStepBase)
/**	
	Constructor.
*/
	{
	}

void CTestSocketBase::ConstructL(TSocketType aSocketType)
/**	
	Second phase constructor.
*/
	{
	switch( aSocketType )
		{
	// Open a listener socket
	case EListenerSocket:
		{
		User::LeaveIfError(iSocket.Open(iListenerMgr->iSocketServOne, KAfInet, KSockStream, KProtocolInetTcp));		
		} break;

	// Open a connector socket
	case EConnectorSocket:
		{
		User::LeaveIfError(iSocket.Open(iListenerMgr->iSocketServTwo, KAfInet, KSockStream, KProtocolInetTcp));		
		} break;

	// Open a blank socket
	case EBlankSocket:
		{
		User::LeaveIfError(iSocket.Open(iListenerMgr->iSocketServOne));
		} break;

	default:
		User::Invariant();
		}
	}

TInt CTestSocketBase::Listen(TUint aQSize, TUint16 aPort)
/**	
	Start the listen service. The socket is bound to the local port specified by
	aPort. The listen service is then started. The aQSize argument specifies the
	number of connections that can be received simultaneously, awaiting to be 
	accepted.
*/
	{

	TInetAddr addr;

	// Bind the socket to the port
	addr.SetPort(aPort);

	TInt error = iSocket.Bind(addr);

	if( error == KErrNone )
		{
		// Start the listening service
		error = iSocket.Listen(aQSize);
		}
	return error;
	}

void CTestSocketBase::Accept(CTestSocketBase& aBlankSocket, TRequestStatus& aStatus)
/**	
	Start asynchronous accept service. The socket should have had the listening
	service started. When a connection has been received, the blank socket will
	be given the connection. 
*/
	{
	iSocket.Accept(aBlankSocket.iSocket, aStatus);
	}

void CTestSocketBase::CancelAccept()
/**	
	Cancel the accept service.
*/
	{
	iSocket.CancelAccept();
	}

void CTestSocketBase::Connect(TInetAddr& aAddr, TRequestStatus& aStatus)
/**	
	Start asynchronous connect service. The address contains the IP address and
	port with which a tcp connection should be established with. The request 
	status is completed either when a connection has been established or an error
	has occurred.
*/
	{
	iSocket.Connect(aAddr, aStatus);
	}

void CTestSocketBase::CancelConnect()
/**	
	Cancel the connect service.
*/
	{
	iSocket.CancelConnect();
	}

void CTestSocketBase::RecvOneOrMore(TDes8& aBuffer, TRequestStatus& aStatus)
/**	
	Receive data from socket asynchronously. Any data received by the socket is 
	placed in the buffer supplied by aBuffer. The request status is completed 
	either when data has been received or an error has occurred.
*/
	{
	iSocket.RecvOneOrMore(aBuffer, KDefaultFlags, aStatus, iBytesReceived);
	}

void CTestSocketBase::CancelRecv()
/**	
	Cancel the receive service.
*/
	{
	iSocket.CancelRecv();
	}

void CTestSocketBase::Send(const TDesC8& aBuffer, TRequestStatus& aStatus)
/**	
	Send data to the socket asynchronously. The data in the supplied buffer is 
	sent to the socket. The request status is completed either when data has 
	been sent or an error has occurred.
*/
	{
	iSocket.Write(aBuffer, aStatus);
	}

void CTestSocketBase::CancelSend()
/**	
	Cancel the send service.
*/
	{
	iSocket.CancelWrite();
	}

void CTestSocketBase::Shutdown(TRequestStatus& aStatus)
/**	
	Shutdown the connection asynchronously. Any pending receive or send requests
	are allowed to complete before the connection is shutdown.
*/
	{
	iSocket.Shutdown(RSocket::ENormal, aStatus);
	}

void CTestSocketBase::RemoteName(TInetAddr& aAddr)
/**	
	Get the remote host name. The IP address and port of the remote host is set
	in the output argument.
*/
	{
	iSocket.RemoteName(aAddr);
	}

void CTestSocketBase::LocalName(TInetAddr& aAddr)
/**	
	Get the local socket name. The IP address and port of the local socket is 
	set in the output argument.
*/
	{
	iSocket.LocalName(aAddr);
	}

