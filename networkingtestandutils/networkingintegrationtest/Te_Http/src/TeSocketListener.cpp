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

#include "TeSocketListener.h"

#include "TeSocketBase.h"

const TInt KListenQSize	= 5;

CTestSocketListener* CTestSocketListener::NewL(
									  CTestListenerMgr*			aListenerMgr,
									  CTestStepBase*			aTestStepBase
									  )
/**	
	The factory constructor.
*/
	{
	return new (ELeave) CTestSocketListener(aListenerMgr, aTestStepBase);
	}

CTestSocketListener::~CTestSocketListener()
/**	
	Destructor.
*/
	{
	// Cancel any outstanding requests
	Cancel();

	// Cleanup...
	delete iAcceptingSocket;
	delete iListeningSocket;
	}


CTestSocketListener::CTestSocketListener(
								CTestListenerMgr*			aListenerMgr,
								CTestStepBase*				aTestStepBase
								)
: CActive(CActive::EPriorityStandard), iListenerMgr(aListenerMgr),
iTestStepBase(aTestStepBase)
/**	
	Constructor.
*/
	{
	CActiveScheduler::Add(this);
	}

//	The socket listener starts listening on the specified port.
void CTestSocketListener::Listen(TUint16 aPort)
	{
	iPort = aPort;

	// Move to the StartListen state and self complete
	iState = EStartListen;
	CompleteSelf();
	}

void CTestSocketListener::CompleteSelf()
/**	
	Requests that the socket listener complete itself. This will cause the 
	RunL() to be called by the scheduler at the next opportunity.
*/
	{
	TRequestStatus* pStat = &iStatus;
	User::RequestComplete(pStat, KErrNone);
	SetActive();
	}

/*
 * Methods from CActive
 */

void CTestSocketListener::RunL()
/**	
	The request servicing function. Behaviour depends on the state of the socket
	listener.
*/
	{
	switch( iState )
		{
	case EStartListen:
		{
		// Open the listening socket on specified port
		iListeningSocket = CTestSocketBase::NewL(iListenerMgr, iTestStepBase, CTestSocketBase::EListenerSocket);
		
		// Start the listening service
		User::LeaveIfError(iListeningSocket->Listen(KListenQSize, iPort));
		iTestStepBase->INFO_PRINTF1(_L("Server Side: Listen() succeeds"));

		// Create a new accepting socket - marry it to the listening socket
		iAcceptingSocket = CTestSocketBase::NewL(iListenerMgr, iTestStepBase, CTestSocketBase::EBlankSocket);
		iListeningSocket->Accept(*iAcceptingSocket, iStatus);

		// Move to the EConnected state and set active
		iState = EConnected;
		SetActive();

		} break;

	case EConnected:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Server Side, in EConnected : Accept() fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}

		iTestStepBase->INFO_PRINTF1(_L("Server Side, in EConnected : Accept() succeeds"));

		// build some data to send
		iBuffer.SetMax();
		iBuffer.FillZ();

		iAcceptingSocket->RecvOneOrMore(iBuffer, iStatus);

		iState = EWaitForRequest;

		SetActive();
		} break;

	case EWaitForRequest:
		{
		// When RecvOneOrMore() fails, set the result to fail & stop the active scheduler
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Server Side, in EWaitForRequest : RecvOneOrMore() fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}

		// When RecvOneOrMore() returns time out, try to receive again 
		if (iStatus == KErrTimedOut)
			{
			iTestStepBase->INFO_PRINTF1(_L("Server Side, in EWaitForRequest : RecvOneOrMore() returns KErrTimedOut"));
			iBuffer.SetMax();
			iBuffer.FillZ();

			iAcceptingSocket->RecvOneOrMore(iBuffer, iStatus);
			SetActive();
			break;
			}

		// When receiving the client request, send a normal packet
		if (iBuffer[0] == (TUint8) 0xAA)
			{
			iTestStepBase->INFO_PRINTF1(_L("Server Side, in EWaitForRequest : receiving the client request"));
			
			iBuffer.SetMax();
			iBuffer.FillZ();

			// Form a normal ICMP packet
			iBuffer[0] = (TUint8) 0x08; // ICMP type = 8;
			iBuffer[1] = (TUint8) 0x00; // ICMP code = 0;
			iBuffer[2] = (TUint8) 0xF7; // ICMP checksum high byte
			iBuffer[3] = (TUint8) 0xFF; // ICMP checksum low byte
			
			iAcceptingSocket->Send(iBuffer, iStatus);
			iState = EWaitForSendToFinish;
			SetActive();
			break;
			}

		// When receiving client acknowledgement, send the last packet
		if ((iBuffer[0] == (TUint8) 0x08) && (iBuffer[1] == (TUint8) 0x00) 
			&& (iBuffer[2] == (TUint8) 0xF7) && (iBuffer[3] == (TUint8) 0xFF))
			{
			iTestStepBase->INFO_PRINTF1(_L("Server Side, in EWaitForRequest : receiving the normal packet succeeds"));

			iBuffer.SetMax();
			iBuffer.FillZ();

			// Form the last packet
			iBuffer[0] = (TUint8) 0xAB; 
			iBuffer[1] = (TUint8) 0xCD; 
			iBuffer[2] = (TUint8) 0xEF; 
			iBuffer[3] = (TUint8) 0xAB; 
			
			iAcceptingSocket->Send(iBuffer, iStatus);
			iState = EWaitForSendToFinish;
			SetActive();
			break;
			}

		// When receiving the last packet, set the result to PASS and stop the active scheduler
		if ((iBuffer[0] == (TUint8) 0xAB) && (iBuffer[1] == (TUint8) 0xCD) 
			&& (iBuffer[2] == (TUint8) 0xEF) && (iBuffer[3] == (TUint8) 0xAB))
			{
			iTestStepBase->INFO_PRINTF1(_L("Server Side, in EWaitForRequest : receiving the last packet succeeds"));

			iTestStepBase->SetTestStepResult(EPass);

			iAcceptingSocket->iSocket.Close();

			CActiveScheduler::Stop();

			break;
			}

		} break;

	case EWaitForSendToFinish:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Server Side, in EWaitForSendToFinish : Send() received packet fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}
		
		iTestStepBase->INFO_PRINTF1(_L("Server Side, in EWaitForSendToFinish : Send() packet succeeds"));

		// Send the received packet to the client
		iBuffer.SetMax();
		iBuffer.FillZ();
		iAcceptingSocket->RecvOneOrMore(iBuffer, iStatus);
	
		iState = EWaitForRequest;

		SetActive();
		} break;

		}
	}

void CTestSocketListener::DoCancel()
/**	
	The asynchronous request cancel.
*/
	{
	// Check state...
	if( iState == EConnected )
		{
		// Cancel Accept request
		iListeningSocket->CancelAccept();
		}
	if( iState == EWaitForRequest)
		{
		// Cancel RecvOneOrMore request
		iAcceptingSocket->CancelRecv();
		}
	if( iState == EWaitForSendToFinish)
		{
		// Cancel Send request
		iAcceptingSocket->CancelSend();
		}
	}

