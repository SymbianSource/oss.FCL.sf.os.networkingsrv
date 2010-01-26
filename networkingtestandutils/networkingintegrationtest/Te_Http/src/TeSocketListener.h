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

#ifndef __TESOCKETLISTENER_H__
#define __TESOCKETLISTENER_H__

#include <e32base.h>
#include <es_sock.h>
#include "TeHttpServer.h"
#include "TeStepBase.h"
#include "TeListenerMgr.h"

class CTestSocketBase;
class CTestStepBase;
class CTestListenerMgr;

class CTestSocketListener : public CActive
/**	
	The CTestSocketListener class provides socket listening behaviour. It listens on 
	the specific port for the client to connect to. Once the connection is established, 
	it continues to listen to the  client request. 
*/
	{
public:	// methods

	static CTestSocketListener* NewL(
								CTestListenerMgr*			aListenerMgr,
								CTestStepBase*				aTestStepBase
								);

	virtual ~CTestSocketListener();

	void Listen(TUint16 aPort);

private:	// methods from CActive

	virtual void RunL();
	
	virtual void DoCancel();
	
//	virtual TInt RunError(TInt aError);

private:	// methods

	CTestSocketListener(
				   CTestListenerMgr*			aListenerMgr,
				   CTestStepBase*				aTestStepBase
				   );

	void CompleteSelf();

private:	// enums

/**	
	The state machine for the socket listener.
*/
	enum TListenState
		{

		/** The socket listener start listening on the specified port.
		*/
		EStartListen,
		/** The socket listener has been notified that a connection has been 
			established with the listening socket.
		*/
		EConnected,
		/** The socket listener waits for client request
		*/
		EWaitForRequest,
		/** The socket listener waits for send to complete
		*/
		EWaitForSendToFinish
		};

private:	// attributes

/** The state of the socket listener.
*/
	TListenState				iState;

/**	The port number on which the socket listener is listenining on.
*/
	TUint16						iPort;

/**	The socket object that is listening on the specified port.
*/
	CTestSocketBase*			iListeningSocket;

/**	The empty socket object that will accept the connection. It is married to
	the connection by the listening socket.
*/
	CTestSocketBase*			iAcceptingSocket;

/**	The buffer for sending and receiving data.
*/
	TBuf8<512>					iBuffer;

/**	The listener manager object for passing the RSocketServ object and 
	listener, connector sockets.
*/
	CTestListenerMgr*			iListenerMgr;

/**	The CTestStepBase object inheriting all the logging, recording results functions.
*/
	CTestStepBase*				iTestStepBase;

	};

#endif	// __TESOCKETLISTENER_H__
