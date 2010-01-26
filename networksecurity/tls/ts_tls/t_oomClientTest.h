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
 
#ifndef __T_OOMTLS_TEST_H__
#define __T_OOMTLS_TEST_H__

#include <e32base.h>
#include <e32cons.h>
#include <c32comm.h>
#include <es_sock.h>
#include <in_sock.h>
#include <ssl.h>
#include <securesocket.h>


class ClientOOMTest : public CActive
{
public:
	// Construct/destruct
	static ClientOOMTest *NewL(CActiveScheduler* anActiveScheduler);
	~ClientOOMTest();
	void Start();
	TInt State(){return iRunState;};
	TInt Error(){return iError;};
	enum TSSLTestRunStates  
		{
		ECreated,
		EConnectPending,
		EHSPending,
		ESendGetRequestPending,
		ERecvPagePending,
		EShutDownPending,
		EFinished
		};
	void SetOOMThreshold(TInt aThreshold){iOOMThreshold=aThreshold;};
	void SetIpAddress(const TDesC &aAddress){iAddress = aAddress;};
	void SetIpPort(TInt aPort){iPortNumber = aPort;};

protected:
	TInt RunError(TInt aError);
private:
	// Construction
	ClientOOMTest(CActiveScheduler* anActiveScheduler);	 
	void ConstructL();
	// Methods from CActive
	void RunL();
	void DoCancel(){};
	TInt		iRunState;
	CActiveScheduler* iActiveScheduler;
	RSocketServ iSocketServ;
	RSocket		iSocket;
	CSecureSocket* iTlsSocket; 
	TBuf8<30>	iSndBuffer;
	TBuf8<10000> iRcvBuffer;
	TSockXfrLength aLen;
	TInetAddr	connectInetAddr;
	TInt		iOOMThreshold;
	TBuf<128>	iAddress;
	TInt		iPortNumber;
   TInt  iError;
};

#endif
