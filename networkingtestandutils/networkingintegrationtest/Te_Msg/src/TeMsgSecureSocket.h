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
 @file TeMsgSecureSocket.h
*/
#if (!defined __TEMSGSECURE_SOCKET_H__)
#define __TEMSGSECURE_SOCKET_H__
#include <test/testexecutestepbase.h>
#include "TeMsgServer.h"
#include "TeHttpPacket.h"
#include <securesocket.h>
#include <e32base.h>
#include <e32std.h>


// Accept untrusted certificates without showing a dialog
#define NODIALOGS

_LIT( KSemaphoreName, "T_AUTOSSL" );

class CTestSecureSocket : public CActive
{
public:
	virtual ~CTestSecureSocket();
	static CTestSecureSocket*	NewL(CTestStep& aTestStep, RSocket& aSocket, const TDesC8& aDnsName);

	void	RunL();
	void	StartHandshake();

protected:
	CTestSecureSocket(CTestStep& aTestStep, RSocket& aSocket);
	void	ConstructL(const TDesC8& aDnsName);
	void	DoCancel();

private:
	enum TSSLState  
		{
		EStartClientHandshake,
		ESentData,
		ERecvData
		};

	TSSLState				iState;
	CTestStep&				iTestStep;
	CSecureSocket*			iSecureSocket;
	RSocket&				iSocket;
	HBufC8*					iDnsName;
	TBuf8<512>				iBuffer;
	RSemaphore				iSemaphore;
	TInt					iTotalBytesRead;
};

#endif
