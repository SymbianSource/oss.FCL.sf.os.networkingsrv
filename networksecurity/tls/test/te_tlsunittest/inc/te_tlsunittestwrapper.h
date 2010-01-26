// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @file
 @internalTechnology
*/

#ifndef TE_TLSUNITTESTWRAPPER_H
#define TE_TLSUNITTESTWRAPPER_H

#include <test/datawrapper.h>
#include <securesocket.h>
#include "ssl_internal.h"

/** Number of chars allowed for address & page settings */
const TInt KSettingFieldWidth = 128;

/**
Forward declaration
*/ 
class RSocketServ;
class CTlsUnitTestWrapper;


class CTlsUnitTest : public CActive
	{
	
public:
	CTlsUnitTest();
	~CTlsUnitTest();
	static CTlsUnitTest *NewL();
	
	void 			ConstructL();
	void 			DoCancel()	{return; };
	void			RunL();
	virtual TInt	RunError(TInt aError);
	TInt 			HandshakeL(CTlsUnitTestWrapper* aObject); 
	void			MakePageRequestL();
	void			GetServerResponseL();
	void			ReadServerResponseL();
	void			ConnectionClosed();
	void			RenegotiateHandshake();
	
	// Generic

	public:
	TInt						iRunError;
	CTlsUnitTestWrapper*    iwrapperobject;
	/** For retries, after a delay */
	RTimer iTimer;
	
	private:
	/** Data sent buffer */
	TPtr8 iSndBuffer;
	/** #bytes sent */
	TSockXfrLength iBytesSent;
	/** Data received buffer */
	TPtr8 iRcvBuffer;
	/** #bytes received */
	TInt iTotalBytesRead;
	
		
	enum EState
		{
		EIdle,
		// Main test
		ESecureConnected,
		EGetRequestSent,
		EGetPageReceived,
		EConnectionClosed,
		ERenegotiateHandshake
		};  
	EState	iState;
		
	};


/**
Class implements the CDataWrapper base class and provides the commands used by the scripts file
*/
class CTlsUnitTestWrapper : public CDataWrapper, public MSoPskKeyHandler
	{
public:
	CTlsUnitTestWrapper();
	~CTlsUnitTestWrapper();
	
	static	CTlsUnitTestWrapper*	NewL();
	//This function is not used currently
	virtual TAny*	GetObject() { return this; }
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	
protected:
	void ConstructL();
	
private:

	void DoServerCertGetOpt();
	void DoConnect(const TDesC& aSection);
	void DoTLSHandshake();
	void DoEnableNullCipher();
	void DoSSLv2handshake();
	void DoPskConfig();
	void DoInitialize();
	void DoCheckGetOptDefault();
	void DoCheckSetOptDefault();
	void DoSetDialogMode();
	void DoPskSetConfig();
	void DoPskGetConfig();
	void DoMakePageRequest();
	void DoGetServerResponse();
	void DoReadServerResponse();
	void DoConnectionClosed();
	void DoCurrentCipherSuite();
	void DoSetProtocol();
	void DoRenegotiateHandshake();
	void DoServerCert();
	void DoSetUnsupportedProtocol();
		
public:
	virtual void GetPskL(const HBufC8 *aPskIdentityHint, HBufC8 *&aPskIdentity, HBufC8 *&aPskKey);
public:
	CTlsUnitTest* 		iActiveObjTest;
	TRequestStatus istatus;
	TBuf<128> iNextTestCaseInput;
	RSocketServ* isocketserv;
	RSocket isocket;
	CSecureSocket* isecuresocket;
	TBuf8<KSettingFieldWidth> iAddress;
	TInt iport;
	
		
	};
	
#endif //TE_TLSUNITTESTWRAPPER_H
