// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation of CPppEndpointImpl class
// 
//

/**
 @file 
 @internalComponent
*/



#include "pppendpointimpl.h"


#include <in_sock.h>
#include <ss_pman.h>

using namespace te_ppploopback; 

/**
 Constructs the receiver channel.
 
 @param aSocket The socket used by receiver.
 */
CRx::CRx(RSocket* aSocket) : CActive(CActive::EPriorityStandard),iRecvLen(MAX_MSG_LEN), iSocket(aSocket)
	{
	}

/**
 Static construction: Constructs the receiver channel.
 
 @param aSocket The socket used by receiver.
 */
CRx* CRx::NewL(RSocket* aSocket)
	{
	CRx* self = new (ELeave) CRx(aSocket);
	CleanupStack::PushL(self);
	CleanupStack::Pop();
	CActiveScheduler::Add(self);
	return self;
	}


/**
 Destroys the receiver channel.
 
 */
CRx::~CRx()
	{
	Cancel(); // cancel ANY outstanding request at time of destruction
	iObserver = NULL;
	iSocket = NULL;	
	}

/**
 Issues asynchronous read request.
 
 @post Request has been issued.
 */
void CRx::RxL()
	{

	// Issue read request
	iSocket->RecvFrom(iDataBuffer, iRecvAddr, 0, iStatus);
	SetActive();
	}

/**
 Sets the observer for the events on this channel.
 
 @param aObserver The observer.
 @post Observer is set.
 */
void CRx::SetObserver (MSocketObserver* aObserver)
	{
	iObserver = aObserver;
	}

/**
 Retrieves the data received on the channel.
 
 @return data buffer.
 @pre There was data reception notification
 */
HBufC16* CRx::GetDataIn(void)
	{
	return iDataBuffer16.Alloc();
	}

/**
 Retrieves the source address of the data received on the channel.
 
 @return source address
 @pre There was data reception notification
 */	
 TInetAddr CRx::GetSourceInetAddr(void)
 	{
 	return iRecvAddr;
 	}
 

/**
 Notify the channel observer that there was an event.
 
 @param aEvent the event
 @post Observer is Notified.
 */
void CRx::NotifyEvent (TInt aEvent)
	{
	iObserver->HandleSocketEvent(CPppEndpointImpl::ESourceRx, aEvent);
	}


/**
 Service an event on our channel
 Implementation of ActiveObject::RunL()
 
 @pre There was a request issued.
 @post event was serviced.
 */
void CRx::RunL(void)
	{
	if (iStatus==KErrNone) // received some data
		{
		// covert binary received to unicode
		TPtrC16 myPtr (reinterpret_cast<const TUint16*>(iDataBuffer.Ptr()), (iDataBuffer.Size()/2));
		iDataBuffer16 = myPtr;

		NotifyEvent(CRx::ERxReceiveOK);
		}
	else
		{
		// some error condition
		NotifyEvent(CRx::ERxReceiveFailed);
		}
	}
/**
 Cancels receive events.
 
 @post No more events come from this channel.
 */
void CRx::DoCancel (void)
	{
	iSocket->CancelRecv();
	}



/**
 Constructs the Transmitter channel.
 
 @param aSocket the channel's socket
 @param aPeerIpAddr the IP address of the communication peer
 @param aPeerPort UDP port of the peer.
 @post Channel is constructed.
 */
CTx::CTx(RSocket* aSocket, const TPtrC& aPeerIpAddr, TInt aPeerPort) 
	: CActive(CActive::EPriorityStandard),
	iSocket(aSocket)
	{
	iSendAddr.Input(aPeerIpAddr);
	iSendAddr.SetPort(aPeerPort);	
	}
/**
 Static Construction.

 @param aSocket     the channel's socket
 @param aPeerIpAddr the IP address of the communication peer
 @param aPeerPort    UDP port of the peer.
 @post Channel is constructed.
 */
CTx* CTx::NewL(RSocket* aSocket, const TPtrC& aPeerIpAddr, TInt aPeerPort)
	{
	CTx* self = new (ELeave) CTx(aSocket, aPeerIpAddr, aPeerPort);
	CleanupStack::PushL(self);
	CleanupStack::Pop();
	CActiveScheduler::Add(self);
	return self;
	}
	
/**
 Destruction

 @post Channel is destroyed
 */
CTx::~CTx()
	{
	Cancel(); // cancel ANY outstanding request at time of destruction
	iObserver = NULL;
	iSocket = NULL;
	}
/**
 Request to transmit a message, asynchronous.

 @param aData the data to send
 @post Request was issued.
 */
void CTx::TxL (TDesC16& aData)
	{
	if (!IsActive())
		{
		// Take a copy of the unicode data to be sent as binary.
		TPtrC8 myPtr (reinterpret_cast<const TUint8*>(aData.Ptr()), aData.Size());
		iDataBuffer = myPtr;

		iSocket->SendTo(iDataBuffer, iSendAddr, 0, iStatus);
		SetActive();
		}
	}
/**
 Sets the observer for this channel
 
 @param aObserver the channel's observer
 @post Observer is set.
 */
void CTx::SetObserver (MSocketObserver* aObserver)
	{
	iObserver = aObserver;
	}

/**
 Notify the observer about an event on the channel.

 @param aEvent the event
 @post Observer was notified.
 */
void CTx::NotifyEvent (TInt aEvent)
	{
	iObserver->HandleSocketEvent(CPppEndpointImpl::ESourceTx, aEvent);
	}

/**
 Service an event on the channel
 Implementation of ActiveObject::RunL()

 @post Event was serviced.
 */
void CTx::RunL(void)
	{
	if (iStatus==KErrNone) // transmit ok
		{
		NotifyEvent(CTx::ETxTransmitOK);
		}
	else
		{
		NotifyEvent(CTx::ETxTransmitFailed);
		}
	}

/**
 Cancels all outstanding send requests
 
 @post All requests were cancelled.
 */
void CTx::DoCancel (void)
	{
	iSocket->CancelSend();
	}
	
	
//******************************************************************************************
// PPP Endpoint implementation. 

/**
 Construction
  
 @param aIapId Iap Id for this endpoint
 @param aOwnIpAddr IP address to use with this endpoint
 @param aOwnPort  Port to used with this endpoint
 @param aPeerIpAddr the IP address of the communication peer 
 @param aPeerPort UDP port of the peer.
 @post Endpiont is constructed
 */
CPppEndpointImpl::CPppEndpointImpl(TInt aIapId, const TPtrC& aOwnIpAddr, TInt aOwnPort, const TPtrC& aPeerIpAddr, TInt aPeerPort):
	 CActive(CActive::EPriorityStandard),
	 iIapId(aIapId),
	 iOwnIpAddr(aOwnIpAddr),
	 iOwnPort(  aOwnPort),
	 iPeerIpAddr(aPeerIpAddr),	 
	 iPeerPort(  aPeerPort),
	 iPppLink(NULL)
	{
	}

/**
 Static Construction
 
 @param aIapId      Id for this endpoint
 @param aOwnIpAddr  IP address to use with this endpoint
 @param aOwnPort    Port to used with this endpoint
 @param aPeerIpAddr the IP address of the communication peer 
 @param aPeerPort   UDP port of the peer.
 @post Endpoint had been constructed
 */
CPppEndpointImpl* CPppEndpointImpl::NewL(TInt aIapId, const TPtrC& aOwnIpAddr, TInt aOwnPort, const TPtrC& aPeerIpAddr, TInt aPeerPort)
	{
	CPppEndpointImpl* self = new (ELeave) CPppEndpointImpl(aIapId, aOwnIpAddr, aOwnPort, aPeerIpAddr, aPeerPort);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	CActiveScheduler::Add(self);
	return self;
	}

/**
 Second phase of construction
 Creates the simplex channels.

 @post Channels are constructed.
 */
void CPppEndpointImpl::ConstructL (void)
	{	
	iPppLink = new (ELeave) CPppLinkImpl(this, &iRConn); // PPP link
	
	iRxAO = CRx::NewL(&iSocket); // Receiver channel
	iRxAO->SetObserver(this);
	
	iTxAO = CTx::NewL(&iSocket, iPeerIpAddr, iPeerPort); // Transmitter channel	
	iTxAO->SetObserver(this);	
	}


/**
 Destruction
 Cancels all requests.

 @post Endpoint had been destroyed.
 */
CPppEndpointImpl::~CPppEndpointImpl()
	{
	// Cancel ANY outstanding request - including these requests owned by the channels
	Cancel();
	
	iListener = NULL;
	delete iRxAO;
	delete iTxAO;
	delete iPppLink;
	}
	
	
void CPppEndpointImpl::NotifyLinkUp(TInt aError)
	{
	iListener->OnEvent(iOurId, MPppEndpointListener::ELinkUp, aError);
	}
	
void CPppEndpointImpl::NotifyLinkDown(TInt aError)
	{
	iListener->OnEvent(iOurId, MPppEndpointListener::ELinkDown, aError);
	}




/**
 Handles an event on a channel

 @param aSource event source
 @param aReason event cause
 @post Event was handled.
 */
void CPppEndpointImpl::HandleSocketEvent (TInt aSource, TInt aReason)
	{
	// Translate and dispatch to our own listener.
	switch(aSource) // Determine who caused the event
		{
		case ESourceRx:
			if(aReason == CRx::ERxReceiveOK)
				{
				iListener->OnEvent(iOurId, MPppEndpointListener::ERecv, KErrNone);
				}
			else if(aReason == CRx::ERxReceiveFailed)
				{
				iListener->OnEvent(iOurId, MPppEndpointListener::ERecv, KErrGeneral);
				}
			else
				{
				ASSERT(EFalse);
				}
			break; 
		
		case ESourceTx :
			if(aReason == CTx::ETxTransmitOK)
				{
				iListener->OnEvent(iOurId, MPppEndpointListener::ESend, KErrNone);
				}
			else if(aReason == CTx::ETxTransmitFailed)
				{
				iListener->OnEvent(iOurId, MPppEndpointListener::ESend, KErrGeneral);
				}
			else
				{
				ASSERT(EFalse);
				}
			break;		
			
		
		default:
			ASSERT(EFalse); 
		}
	}


/**
 Requests a Connection to the PPP Peer asynchronously

 @post Request was issued.
 */
void CPppEndpointImpl::ConnectToPeerL()
	{	
	TInt err = KErrNone;;
	
	err = iSession.Connect(); // Connect to the socket server
	User::LeaveIfError(err);		
	
	err = iRConn.Open(iSession); 
	User::LeaveIfError(err);
	
	// Set connection preferences to use PPP
	iConnPref.SetIapId(iIapId);
	
	// NOTE: There is an issue with PhoneBook synchronizer as of March 3, 2004.
	// It may cause the tests to fail, if "ECommDbDialogPrefDoNotPrompt" is specified.
	// If this is the case, use "ECommDbDialogPrefPromptIfWrongMode" instead.
	iConnPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	
	
	// Issue asynchronous request to initialize PPP. This returns immediately.
	iPppLink->OpenPppLinkL(&iConnPref);
	}
	

/**
 Disconnects from the peer.

 @post Disconnected from the peer.
 */
void CPppEndpointImpl::DisconnectFromPeerL()
	{
	iPppLink->ClosePppLinkL();
	iRConn.Close();
	iSession.Close();		
	}
	
TBool CPppEndpointImpl::IsConnectedToPeer()
	{
	return (iRConn.SubSessionHandle() != 0);
	}

/**
 Opens a communication channel for this endpoint.
 Does not affect PPP operation.
 Used for allow a complete separation of PPP connection / disconnection and data exchange.
 
 @pre Successfully connected to PPP Peer.
 @post Communication channel is open
 */
void CPppEndpointImpl::OpenCommChannelL()
	{	
	TInt err = iSocket.Open(iSession, KAfInet, KSockDatagram, KProtocolInetUdp, iRConn);
	User::LeaveIfError(err);

	// Bind the socket
	TInetAddr ownAddrOnPort;
	ownAddrOnPort.Input(iOwnIpAddr);
	ownAddrOnPort.SetPort(iOwnPort);	
	User::LeaveIfError(iSocket.Bind(ownAddrOnPort));	
	}
	
/**
 Closes the communication channel for this endpoint.
 Does not affect PPP operation.
 Used for allow a complete separation of PPP connection / disconnection and data exchange.
 
 @post Communication channel is closed
 */
void CPppEndpointImpl::CloseCommChannel()
	{
	iRxAO->Cancel();
	iTxAO->Cancel();	
	iSocket.CancelAll();
	iSocket.Close();
	} 	
	

/**
 Request to send a message on the channel, asynchronous

 @param aMsg The message
 @pre Communication channel opened successfully.
 @post The request was issued
 */
void CPppEndpointImpl::SendMessageL (TDesC& aMsg)
	{
	iTxAO->TxL(aMsg);
	}

/**
 Sets a listener for this PPP endpoint.
 
 @param aListener The listener
 @param aId the ID for this endpoint
 @post Listener was set.
 */
void CPppEndpointImpl::SetObserver (MPppEndpointListener* aListener, MPppEndpointListener::EEndpointId aId)
	{
	iListener = aListener;
	iOurId = aId;
	}

/**
 Request to receive a message on the channel, asynchronous
 
 @pre Communication channel opened successfully.
 @post The request was issued
 */
void CPppEndpointImpl::RequestMessageL (void)
	{
	iRxAO->RxL();
	}


/**
 Retrieves the data received on this channel.
 
 @return the data buffer
 @pre Communication channel opened successfully.
 */

HBufC16* CPppEndpointImpl::GetDataIn(void)
	{
	return iRxAO->GetDataIn();
	}
 
	
/**
 Retrieves the source address of the data received on this channel.
 
 @return the source address
 @pre Receive event notification was received. 
 */
TInetAddr CPppEndpointImpl::GetSourceInetAddr(void)	
	{
	return iRxAO->GetSourceInetAddr();
	}




/**
Retrieves the primary and secondary IPv4 DNS addresses of this endpoint

@param aDns1 Set to the primary DNS address on return, if any (should be cleared on entry)
@param aDns2 Set to the secondary DNS address on return, if any (should be cleared on entry)

@pre must be connected to peer (Because we may need to obtain our DNS address from peer.
*/
void CPppEndpointImpl::GetDnsAddrsL(TUint32& aDns1, TUint32& aDns2)
	{
	TPckgBuf<TSoInetInterfaceInfo> infoBuf;		// IPv4 stack only
	TSoInetInterfaceInfo info;					// IPv4 stack only
	
	// Open a socket to get the interfaces.
	RSocket sock;
	User::LeaveIfError(
		sock.Open(iSession, KAfInet, KSockStream, KProtocolInetTcp, iRConn));

	// Iterate through all interfaces, looking for DNS servers
	for (;;)
		{
		TInt retc = sock.GetOpt(KSoInetNextInterface,KSolInetIfCtrl,infoBuf);
		if (retc == KErrNotFound)
			break;
		else if (retc != KErrNone)
			{
			break;
			}
			
		info = infoBuf();
		TBool gotAddrs = EFalse;
		
		
		if (info.iState == EIfUp)
			{
			if (info.iNameSer1.Address())
				{
				aDns1 = info.iNameSer1.Address();
				gotAddrs = ETrue;
				}
			
			if (info.iNameSer2.Address() && (info.iNameSer2.Address() != info.iNameSer1.Address()))
				{
				aDns2 = info.iNameSer2.Address();
				gotAddrs = ETrue;
				}
			}

		if (gotAddrs)
			break;
		}
		
		sock.Close();
	}





// ActiveObject Implementation.

/**
 Called by ActiveObject::Cancel()
 
 @post communication channel was closed.
 */
void CPppEndpointImpl::DoCancel (void)
	{
	CloseCommChannel();	// cancels everything and closes the socket.
	iPppLink->Cancel();
	}
	

/**
 ActiveObject::RunL() 
 Never gets called in this implementation
 
*/	
void CPppEndpointImpl::RunL(void)
	{
	}
	


