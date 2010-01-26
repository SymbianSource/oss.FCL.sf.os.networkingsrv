/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
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
 * @file ts_ipsec_polapi.cpp Implements main test code for IPsec
 */

#include "t_ipsecikev2.h"
#include "t_ipsecconst.h"

#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif


/**
Purpose: Constructor of CT_IPSecIKEV2TestWrapper class

@internalComponent
*/
CT_IPSecIKEv2TestWrapper::CT_IPSecIKEv2TestWrapper()
:	iObject(NULL)
	{
	}
/**
Purpose: Destructor of CT_IPSecIKEV2TestWrapper class

@internalComponent
*/
CT_IPSecIKEv2TestWrapper::~CT_IPSecIKEv2TestWrapper()
	{
	//This is just an example about how to use create a new object
	delete iObject;
	iObject = NULL;
	}
	
/**
Purpose: Command fuction of CT_IPSecIKEV2TestWrapper class

@internalComponent
*/
CT_IPSecIKEv2TestWrapper* CT_IPSecIKEv2TestWrapper::NewL()
	{
	CT_IPSecIKEv2TestWrapper*	ret = new (ELeave) CT_IPSecIKEv2TestWrapper();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}
	
	
/**
Purpose: Command fuction for a wrapper class

@internalComponent
*/
void CT_IPSecIKEv2TestWrapper::ConstructL()
	{
	iObject	= new (ELeave) TInt;
	}


/**
Purpose: Command fuction for a wrapper class

@internalComponent
*/
TBool CT_IPSecIKEv2TestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret = ETrue;

	// Print out the parameters for debugging
	INFO_PRINTF2( _L("<font size=2 color=990000><b>aCommand = %S</b></font>"), &aCommand );
	INFO_PRINTF2( _L("aSection = %S"), &aSection );
	INFO_PRINTF2( _L("aAsyncErrorIndex = %D"), aAsyncErrorIndex );

	if(KNew() == aCommand)
		{
		DoCmdNewL(aSection);
		}
	else if(KTestIKEv2() == aCommand)
		{
		DoCmdTestIKEv2(aSection);
		}
	else if(KNegativeTestIKEv2() == aCommand)
		{
		DoCmdNegativeTestIKEv2(aSection);
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	}


/**
Purpose: To create a new object of the CTEFTest type through the API.

Ini file options:
	iniData - The data from the ini file at the section provided.

@internalComponent
@param  aSection Current ini file command section
*/
void CT_IPSecIKEv2TestWrapper::DoCmdNewL(const TDesC& aSection)
	{
	TInt objectValue = 0;
	if (!GetIntFromConfig(aSection, KObjectValue(), objectValue))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KObjectValue());
		SetBlockResult(EFail);
		}
	else
		{
		delete iObject;
		iObject = new (ELeave) TInt(objectValue);
		}
	}

void CT_IPSecIKEv2TestWrapper::DoCmdNegativeTestIKEv2(const TDesC& aSection)
	{
	TInt port = 0;
	if (!GetIntFromConfig(aSection, KPort(), port))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KPort());
		SetBlockResult(EFail);
		}
	
	TBuf<15> ipDAddr;
	TPtrC16 pIpDAddr = ipDAddr;
	if (!GetStringFromConfig(aSection, KDestAddr(), pIpDAddr))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KDestAddr());
		SetBlockResult(EFail);
		}
	
	TBuf<15> ipLAddr;
	TPtrC16 pIpLAddr = ipLAddr;
	if (!GetStringFromConfig(aSection, KLocaladdr(), pIpLAddr))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KLocaladdr());
		SetBlockResult(EFail);
		}
	
	TInt err = KErrNone;
	TRAP(err, TestIKEv2WithEchoL(port, pIpDAddr));
	if (err != KErrNone)
        {
        ERR_PRINTF2(_L("TestIKEv2WithEchoL() failed with error: %d"), err);
        SetError(err);
        }
	}

void CT_IPSecIKEv2TestWrapper::DoCmdTestIKEv2(const TDesC& aSection)
	{
	TInt port = 0;
	if (!GetIntFromConfig(aSection, KPort(), port))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KPort());
		SetBlockResult(EFail);
		}
	
	TBuf<15> ipDAddr;
	TPtrC16 pIpDAddr = ipDAddr;
	if (!GetStringFromConfig(aSection, KDestAddr(), pIpDAddr))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KDestAddr());
		SetBlockResult(EFail);
		}
	
	TBuf<15> ipLAddr;
	TPtrC16 pIpLAddr = ipLAddr;
	if (!GetStringFromConfig(aSection, KLocaladdr(), pIpLAddr))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KLocaladdr());
		SetBlockResult(EFail);
		}
	
	TInt err = KErrNone;
	//TestIKEv2WithEcho(port, pIpDAddr, pIpLAddr);
	TRAP(err, TestIKEv2WithEchoL(port, pIpDAddr));
	if (err == KErrNone )
		SetBlockResult( EPass );
	else
		{
		ERR_PRINTF2( _L("<font color=FF0000>Failed with Error value = %D</font>"), err );
		SetBlockResult( EFail );
		}
	}

void CT_IPSecIKEv2TestWrapper::TestIKEv2WithEchoL(TInt aPort, TPtrC16 aIpDAddr)
	{
	//Connect to socket server 
	RSocketServ socketServ;
	User::LeaveIfError(socketServ.Connect());
	CleanupClosePushL(socketServ);
	INFO_PRINTF1(_L("RSocketServ::Connect Completed Successfully"));
	
	// Open RConnection
	RConnection connect;
	User::LeaveIfError(connect.Open(socketServ, KConnectionTypeDefault));
	CleanupClosePushL(connect);
	INFO_PRINTF1(_L("RConnection::Open Completed Successfully"));

	// Start RConnection
	TCommDbConnPref prefs;
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	//prefs.SetIapId(12);
	TInt err = connect.Start(prefs);
	User::LeaveIfError(err);	// Handle options here like (ie: No Prompt)
	INFO_PRINTF1(_L("RConnection::Start Completed Successfully"));
		
	// Open RSocket
	RSocket sock;	
	User::LeaveIfError(sock.Open(socketServ, KAfInet, KSockStream, KProtocolInetTcp, connect));
	CleanupClosePushL(sock);
	INFO_PRINTF1(_L("RSocket::Open Completed Successfully"));
	
	// Connect
	TRequestStatus status;
	//TInetAddr localAddr(KInetAddrAny, aPort);
	//User::LeaveIfError(sock.Bind(localAddr));
	
	TInetAddr echoServAddr; //(KInetAddrAny, 6401);	// 7: TCP port
	echoServAddr.Input(aIpDAddr);
	echoServAddr.SetPort(aPort);
	sock.Connect(echoServAddr, status);
	User::WaitForRequest(status);
		
	TInt sockErr = status.Int(); 
	if( sockErr != KErrNone)
		{
		INFO_PRINTF2(_L("RSocket::Connect Failed with error code %D"), sockErr);
		/*sock.CancelAll();
		TRequestStatus newStatus;
		sock.Shutdown(RSocket::EImmediate, newStatus);
		User::WaitForRequest(newStatus);
		CleanupStack::PopAndDestroy(&sock);
		//stop connection
		User::LeaveIfError(connect.Stop(RConnection::EStopAuthoritative));
		*/
		User::Leave(sockErr);
		}
	INFO_PRINTF1(_L("RSocket::Connect Completed Successfully"));
	
	// Send a packet
	HBufC8* packet = HBufC8::NewL(KSockBufferLength);
	CleanupStack::PushL(packet);

	packet->Des().SetMax();
	packet->Des().Fill('D');	
	TInt LenHBuf = packet->Length();
	TInt sizeHBuf = packet->Size();
	
	sock.Send(packet->Des(), 0, status);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());
	INFO_PRINTF1(_L("RSocket::Send Completed Successfully"));
	
	// Receive it back
	packet->Des().FillZ();
	TPtr8 buff = packet->Des();
	TSockXfrLength len;
	sock.Recv(buff, 0, status, len);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());
	INFO_PRINTF1(_L("RSocket::Recv Completed Successfully"));
	
	TInt length = buff.Length();
	TInt size = buff.Size();
	/*if ( length != KSockBufferLength)
		User::LeaveIfError(KErrCorrupt);
	*/
	
	// Terminates 
	CleanupStack::PopAndDestroy(packet);
	CleanupStack::PopAndDestroy(&sock);
	INFO_PRINTF1(_L("RSocket::Close Completed Successfully"));
	
	//Close Connection
	connect.Close();
	INFO_PRINTF1(_L("RConnection::Close Completed Successfully"));
	/*
	//stop connection
	User::LeaveIfError(connect.Stop());
	INFO_PRINTF1(_L("RConnection::Stop Completed Successfully"));
	*/
	CleanupStack::PopAndDestroy(&connect);
	CleanupStack::PopAndDestroy(&socketServ);

	}



