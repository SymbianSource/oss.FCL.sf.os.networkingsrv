/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file TeSocketBase.h
*/
#ifndef __TESOCKETBASE_H__
#define __TESOCKETBASE_H__
#include <test/testexecutestepbase.h>
#include "TeHttpServer.h"
#include "TeStepBase.h"
#include "TeListenerMgr.h"

#include <e32base.h>
#include <in_sock.h>
#include <es_sock.h>


class CTestSocketBase : public CBase
	{
public:
/**	
	The TSocketType enumerates the type of socket, e.g if the socket should be
	initiased for a specified protocol. Can be expanded to specify a secure 
	socket.
	
*/
	enum TSocketType
		{
		/**	The listener socket (on the server side).
		*/
		EListenerSocket			= 0,

		/**	The connector socket (on the client side).
		*/
		EConnectorSocket,
		/**	The blank socket.
		*/
		EBlankSocket
		};

public:
	static CTestSocketBase* NewL(CTestListenerMgr* aListenerMgr, CTestStepBase* aTestStepBase, TSocketType aSocketType);

	virtual ~CTestSocketBase();

protected:
	CTestSocketBase(CTestListenerMgr* aListenerMgr, CTestStepBase* aTestStepBase);

	void ConstructL(TSocketType aSocketType);

public: //method

	TInt Listen(TUint aQSize, TUint16 aPort);

	void Accept(CTestSocketBase& aBlankSocket, TRequestStatus& aStatus);

	void CancelAccept();

	void Connect(TInetAddr& aAddr, TRequestStatus& aStatus);

	void CancelConnect();

	void RecvOneOrMore(TDes8& aBuffer, TRequestStatus& aStatus);

	void CancelRecv();

	void Send(const TDesC8& aBuffer, TRequestStatus& aStatus);

	void CancelSend();

	void Shutdown(TRequestStatus& aStatus);

	void RemoteName(TInetAddr& aAddr);

	void LocalName(TInetAddr& aAddr);


public:

	//	The socket
	RSocket					iSocket;

private:

	//  The number of bytes read
	TSockXfrLength			iBytesReceived;

	CTestListenerMgr*		iListenerMgr;

	CTestStepBase*			iTestStepBase;

	};
#endif // __TESOCKETBASE_H__
