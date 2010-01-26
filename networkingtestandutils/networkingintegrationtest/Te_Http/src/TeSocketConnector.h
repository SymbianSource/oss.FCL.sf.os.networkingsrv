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

#ifndef __TESOCKETCONNECTOR_H__
#define __TESOCKETCONNECTOR_H__

#include <e32base.h>
#include <in_sock.h>
#include <es_sock.h>

#include "TeListenerMgr.h"

class CTestSocketBase;
class CTestStepBase;
class CTestListenerMgr;

class CTestSocketConnector : public CActive
/**	
	The CTestSocketConnector class provides client socket connecting, sending,
	and receiving data behaviour. 

	The client socket connector initially does a DNS lookup for the host name provided
	in the ConnectL() API. Once the IP address has been found for the host name
	the socket connector will attempt to establish a TCP connection with the 
	Local host. In this case, the loopback address is used.

	Once it connects to the server socket, it sends the request to the server and 
	receives the response back.
*/
	{
public:	// methods

	static CTestSocketConnector* NewL(
								CTestListenerMgr*			aListenerMgr,
								CTestStepBase*				aTestStepBase
								);


	virtual ~CTestSocketConnector();

	void ConnectL(const TDesC16& aLocalHost, TUint16 aLocalPort);

private:	// methods from CActive

	virtual void RunL();
	
	virtual void DoCancel();
	
private:	// methods

	CTestSocketConnector(
				   CTestListenerMgr*			aListenerMgr,
				   CTestStepBase*				aTestStepBase
				   );
	
	void CompleteSelf();


private:	// enums

/**	
	The state machine for the socket connector.
*/
	enum TConnectState
		{
		/** A connection has been requested. The DNS lookup needs to be initiated
			to find the IP address of the local host.
		*/
		EPendingDNSLookup,
		/**	The IP address of the Local host has been found. Initiated a TCP
			connection to that Local host.
		*/
		EConnecting,
		/**	The connection has been established. Ownership of the connected socket
			must be passed to the observer.
		*/
		EConnected,
		/** The socket connector waits for send request to complete
		*/
		EWaitForSendRequestToFinish,
		/** The socket connector waits for server response
		*/
		EWaitForResponse,
		/** The socket connector waits for send normal packet to complete
		*/
		EWaitForSendNormalPacketToFinish,
		/** The socket connector waits for send last packet to complete
		*/
		EWaitForSendLastPacketToFinish

		};

private:	// attributes

/** The state of the socket connector.
*/
	TConnectState				iState;

/** The host resolver session.
*/
	RHostResolver				iHostResolver;

/**	The host name/IP address for the Local client.
*/
	HBufC*						iHost;

/**	The port number on Local host with which to connect to
*/
	TUint16						iPort;

/** The DNS entry object for the Local client
*/
	TNameEntry					iHostDnsEntry;
	
/** The address of the Local host
*/
	TInetAddr					iAddress;

/**	The socket object that is connecting to the Local client
*/
	CTestSocketBase*			iConnectingSocket;

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

#endif	// __TESOCKETCONNECTOR_H__
