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

#include "TeSocketConnector.h"

#include "TeSocketBase.h"
#include <es_sock.h>
#include <in_sock.h>

const TInt KIpv6MaxAddrSize		= 39;

CTestSocketConnector* CTestSocketConnector::NewL(
										CTestListenerMgr*			aListenerMgr,
										CTestStepBase*				aTestStepBase
										)										
/**	
	The factory constructor.
*/
	{
	return new (ELeave) CTestSocketConnector(aListenerMgr, aTestStepBase);
	}

CTestSocketConnector::~CTestSocketConnector()
/**	
	Destructor.
*/
	{
	// Cancel any outstanding requests
	Cancel();

	// Cleanup...
	delete iHost;
	delete iConnectingSocket;
	
	// Close the handler
	iHostResolver.Close();
	}


CTestSocketConnector::CTestSocketConnector(
								  CTestListenerMgr*			aListenerMgr,
								  CTestStepBase*			aTestStepBase
								  )
: CActive(CActive::EPriorityStandard), iListenerMgr(aListenerMgr),
iTestStepBase(aTestStepBase)

/**	
	Constructor.
*/	{
	CActiveScheduler::Add(this);
	}

void CTestSocketConnector::ConnectL(const TDesC16& aLocalHost, TUint16 aLocalPort)
/**	
	Start connection to specified local host. The socket connector starts 
	connecting to the specified local host on the specified port.
*/
	{

	// Copy the local host IP address and port
	iHost = aLocalHost.AllocL();
	iPort = aLocalPort;

	// Move to the PendingDNSLookup state and self complete.
	iState = EPendingDNSLookup;
	CompleteSelf();
	}

void CTestSocketConnector::CompleteSelf()
/**	
	Requests that the socket connector complete itself. This will caused the 
	RunL() to be called by the scheduler at the next opportunity.
*/
	{
	TRequestStatus* pStat = &iStatus;
	User::RequestComplete(pStat, KErrNone);
	SetActive();
	}

/*
 *	Methods from CActive
 */

void CTestSocketConnector::RunL()
/**	
	The request servicing function. Behaviour depends on the state of the socket
	connector.
*/
	{
	switch( iState )
		{
	case EPendingDNSLookup:
		{		
		User::LeaveIfError(iHostResolver.Open(
									   iListenerMgr->iSocketServTwo,
									   KAfInet, 
									   KProtocolInetUdp
									   ));		

		// Start the DNS lookup for the local host name.
		iHostResolver.GetByName(*iHost, iHostDnsEntry, iStatus);

		// Move to the Connecting state and go active
		iState = EConnecting;
		SetActive();
		} break;
	case EConnecting:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Client side, in EConnecting : GetByName() fails with error %d"), result);

			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}

		iTestStepBase->INFO_PRINTF1(_L("Client side, in EConnecting : GetByName() succeeds"));

		// DNS lookup successful - form the internet address object
		iAddress = TInetAddr(iHostDnsEntry().iAddr);

		iAddress.SetPort(iPort);

		TBuf<KIpv6MaxAddrSize> ip16bit;
		iAddress.Output(ip16bit);
		
		iTestStepBase->INFO_PRINTF2(_L("Client Side : DNS lookup complete -> IP address %S"), &ip16bit);

		// Create the connecting socket
		iConnectingSocket = CTestSocketBase::NewL(iListenerMgr, iTestStepBase, CTestSocketBase::EConnectorSocket);

		// Start connecting to the local client
		iConnectingSocket->Connect(iAddress, iStatus);

		// Move to the Connected state and go active
		iState = EConnected;

		SetActive();
		} break;
	case EConnected:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Client side, in EConnected : Connect() fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}

		iTestStepBase->INFO_PRINTF1(_L("Client side, in EConnected : Connect() succeeds"));
		
		iBuffer.SetMax();
		iBuffer.FillZ();

		// Form a request packet
		iBuffer[0] = (TUint8) 0xAA;	  
		iConnectingSocket->Send(iBuffer, iStatus);

		iState = EWaitForSendRequestToFinish;
		SetActive();
		} break;

	case EWaitForSendRequestToFinish:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Client side, in EWaitForSendRequestToFinish : Send() fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}
		
		iTestStepBase->INFO_PRINTF1(_L("Client side, in EWaitForSendRequestToFinish : Send() request packet succeeds"));
		
		iBuffer.SetMax();
		iBuffer.FillZ();

		// Receive the normal packet from the server
		iConnectingSocket->RecvOneOrMore(iBuffer, iStatus);

		iState = EWaitForResponse;

		SetActive();
		}break;
	case EWaitForResponse:
		{

		// When RecvOneOrMore() fails, set the result to fail & stop the active scheduler
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Client Side, in EWaitForResponse : RecvOneOrMore() fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}
		
		// When RecvOneOrMore() returns time out, try to receive again 
		if (result == KErrTimedOut)
			{
			iTestStepBase->INFO_PRINTF1(_L("Client Side, in EWaitForResponse : RecvOneOrMore() returns KErrTimedOut"));
			iBuffer.SetMax();
			iBuffer.FillZ();

			iConnectingSocket->RecvOneOrMore(iBuffer, iStatus);
			SetActive();
			break;
			}
		
		// When client receives the normal packet, send the receiving normal packet responce back
		if ((iBuffer[0] == (TUint8) 0x08) && (iBuffer[1] == (TUint8) 0x00) 
			&& (iBuffer[2] == (TUint8) 0xF7) && (iBuffer[3] == (TUint8) 0xFF))
			{
			iTestStepBase->INFO_PRINTF1(_L("Client Side, in EWaitForResponse : RecvOneOrMore() receives the normal packet"));
			iConnectingSocket->Send(iBuffer, iStatus);
			iState = EWaitForSendNormalPacketToFinish;
			SetActive();
			break;
			}
		else
			{
			// When client receives the last packet
			if ((iBuffer[0] == (TUint8) 0xAB) && (iBuffer[1] == (TUint8) 0xCD) 
				&& (iBuffer[2] == (TUint8) 0xEF) && (iBuffer[3] == (TUint8) 0xAB))
				{
				iTestStepBase->INFO_PRINTF1(_L("Client Side, in EWaitForResponse : receiving the last packet succeeds"));
				
				// Send the last packet
				iConnectingSocket->Send(iBuffer, iStatus);
				iState = EWaitForSendLastPacketToFinish;
				SetActive();
				break;
				}
			// When client receives the junk packet, set the result to FAIL
			else
				{
				iTestStepBase->INFO_PRINTF1(_L("Client Side, in EWaitForResponse : RecvOneOrMore() receives the junk packet"));
				iTestStepBase->SetTestStepResult(EFail);
				CActiveScheduler::Stop();
				break;
				}
			}

		}
	
	case EWaitForSendNormalPacketToFinish:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Client Side, in EWaitForSendNormalPacketToFinish : Send() the normal packet fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}

		iTestStepBase->INFO_PRINTF1(_L("Client Side, in EWaitForSendNormalPacketToFinish : sending the normal packet succeeds"));

		iBuffer.SetMax();
		iBuffer.FillZ();

		iConnectingSocket->RecvOneOrMore(iBuffer, iStatus);
		iState = EWaitForResponse;
		SetActive();
		} break;

	case EWaitForSendLastPacketToFinish:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			iTestStepBase->INFO_PRINTF2(_L("Client Side, in EWaitForSendLastPacketToFinish : sending the last packet fails with error %d"), result);
			iTestStepBase->SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}
			
		iTestStepBase->INFO_PRINTF1(_L("Client Side, in EWaitForSendLastPacketToFinish : sending the last packet succeeds"));

		// Set the result to Pass
		iTestStepBase->SetTestStepResult(EPass);

		// Close the socket
		iConnectingSocket->iSocket.Close();

		// Stop the activer scheduler
		CActiveScheduler::Stop();

		} break;

		}
	}

void CTestSocketConnector::DoCancel()
/**	
	The asynchronous request cancel.
*/
	{
	// Check state
	switch( iState )
		{
	case EConnecting:
		{
		// DNS lookup is pending - cancel
		iHostResolver.Cancel();
		} break;
	case EConnected:
		{
		if( iConnectingSocket )
			{
			// Connection is pending - cancel and delete the socket
			iConnectingSocket->CancelConnect();
			delete iConnectingSocket;
			iConnectingSocket = NULL;
			}
		} break;
	case EWaitForSendRequestToFinish:
		{
		iConnectingSocket->CancelSend();
		} break;
	case EWaitForResponse:
		{
		iConnectingSocket->CancelRecv();
		} break;
	case EWaitForSendNormalPacketToFinish:
		{
		iConnectingSocket->CancelSend();
		} break;
	case EWaitForSendLastPacketToFinish:
		{
		iConnectingSocket->CancelSend();
		} break;
	default:
		// Do nothing...
		break;
		}
	}
