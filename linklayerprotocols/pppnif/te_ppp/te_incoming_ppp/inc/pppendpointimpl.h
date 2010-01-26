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
// Interface for CPppEndpointImpl class 
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __PPPENDPOINTIMPL_H__
#define __PPPENDPOINTIMPL_H__

#include <e32std.h>
#include <e32base.h>
#include <es_sock.h>
#include <in_sock.h>
#include <commdbconnpref.h>

#include "pppendpoint.h"
#include "ppplinkimpl.h"

namespace te_ppploopback
{

#define MAX_MSG_LEN	(256)        //in bytes
#define MAX_MSG_LEN_UNICODE (128) //in words  
#define MAX_HOST_NAME (256)


/**
 * Notification interface for socket observers
 * Adopted from symbian sockets training excersise
 *
 * @internalComponent
 * @test
 */
class MSocketObserver
	{
public:
	
	/**
	Called on any socket event
	@param aSource Notification source - either transmitter or receiver.
	@param aReason Event being notified.
	@post Observer was notified.
	*/
	virtual void HandleSocketEvent (TInt aSource, TInt aReason)=0;
	};

/**
 * Receiver simplex channel on a socket.
 * 
 Adopted from symbian sockets training excersise
 
 @internalComponent
 @test
 */
class CRx : public CActive
	{	
public:

	/** Receiver channel events */
	enum TEvent
		{
		ERxReceiveOK, ERxReceiveFailed 
		};
		
	static CRx* NewL(RSocket* aSocket);
	~CRx();

	void RxL();	
	void SetObserver (MSocketObserver* aObserver);
		
	HBufC16* GetDataIn(void);
	TInetAddr GetSourceInetAddr(void);
	

private:
	CRx(RSocket* aSocket);
	void NotifyEvent (TInt aEvent);
	
	// pure virtuals from CActive implemented in this derived class
	void RunL(void);
	void DoCancel (void);

private:
	/** Observer for events on this channel */
	MSocketObserver* iObserver;
	
	/** Length of reveived data */
	TSockXfrLength iRecvLen;
	
	/** Socket for this channel */
	RSocket* iSocket;
	
	/** Raw byte buffer for received messages,  */	
	TBuf8<MAX_MSG_LEN>	      	iDataBuffer;
	
	/** Unicode buffer for received messages - allows to process Unicode strings */
	TBuf16<MAX_MSG_LEN_UNICODE>	iDataBuffer16;
	
	/* The source address of received datagram */
	TInetAddr iRecvAddr;
	};

/**
 Transmitter simplex channel on a socket.
 Adopted from symbian sockets training excersise
 
 @internalComponent
 @test
 */ 
class CTx : public CActive
	{
public:
	
	/** Transmitter channel events */
	enum TEvent
		{
		ETxTransmitOK, ETxTransmitFailed 
		};
		
	static CTx* NewL(RSocket* aSocket, const TPtrC& aPeerIpAddr, TInt aPeerPort);
	
	~CTx();
	
	void TxL(TDesC16& aData);
	
	void SetObserver (MSocketObserver* aObserver);

private:
	
	CTx(RSocket* aSocket, const TPtrC& aPeerIpAddr, TInt aPeerPort);
	
	void NotifyEvent (TInt aEvent);
	
	// pure virtuals from CActive implemented in this derived class
	void RunL(void);
	void DoCancel (void);

private:
	
	/** Observer of events on this channel */
	MSocketObserver* iObserver;
	
	/** This channel's socket */
	RSocket*         iSocket;
	
	/** Destination address */
	TInetAddr        iSendAddr;
	
	/** Raw bytes buffer for messages to be sent */
	TBuf8<MAX_MSG_LEN>	iDataBuffer;
	
	};

/**
 Represents PPP endpoint.
 Implements a Duplex channel on a socket.
 Encapsulates a receiever and a transmitter simplex channels.
 Manages transmission / receiving.
 
 @internalComponent
 @test
 */
class CPppEndpointImpl : 
	public CActive, 
	public MSocketObserver,
	public MPppLinkObserver
	{

public:
	// MSocketObserver implementation.
	virtual void HandleSocketEvent (TInt aSource, TInt aReason);
	
	// MPppLinkObserver implementation
	void NotifyLinkUp(TInt aError);
	void NotifyLinkDown(TInt aError);

	static CPppEndpointImpl* NewL(TInt aIapId, const TPtrC& aOwnIpAddr, TInt aOwnPort, const TPtrC& aPeerIpAddr, TInt aPeerPort);
	~CPppEndpointImpl();
	
	void ConnectToPeerL();
	void DisconnectFromPeerL();
	
	TBool IsConnectedToPeer();
	
	void OpenCommChannelL();
	void CloseCommChannel();
	
	void SetObserver (MPppEndpointListener* aListener, MPppEndpointListener::EEndpointId aId);
	void SendMessageL (TDesC& aMsg);
	void RequestMessageL(void);
	
	HBufC16*  GetDataIn(void);
	TInetAddr GetSourceInetAddr(void);
	void      GetDnsAddrsL(TUint32& aDns1, TUint32& aDns2);
public:
	
	/** 
	Source of PPP endpoint events:
	ESourceLink: the PPP link itself (UP / DOWN events)
	ESourceRx: receiver channel: something was received
	ESourceTx: transmitter channel: something was sent
	*/ 
	enum TSource
		{
		ESourceLink, ESourceRx, ESourceTx
		};
	
private:
	void ConstructL(void);
	
	// CActive implementation
	void RunL(void);
	void DoCancel (void);

private:
	CPppEndpointImpl(TInt aIapId, const TPtrC& aOwnIpAddr, TInt aOwnPort, const TPtrC& aPeerIpAddr, TInt aPeerPort);
		
	// Connection preferences, overriding CommDB
	TInt iIapId;       
	
	//
	// Endpoints: 
	//
	
	/** This endpoint's IP address */
	TPtrC iOwnIpAddr;
	
	/** This endpoint's socket port */
	TInt  iOwnPort;	
	
	/** Peer endpoint's IP address */
	TPtrC iPeerIpAddr;
	
	/** Peer endpoint's socket port */
	TInt  iPeerPort;	
	
	/** Used to identify the endpoint among peers */
	MPppEndpointListener::EEndpointId iOurId;
	
	/** This ednpoint's end of PPP link */
	CPppLinkImpl* iPppLink;
	
	
	//
	// High level duplex channel
	//
	
	/** Esock*/
	RSocketServ 	iSession;
	
	/** Used to create PPP link */
	RConnection 	iRConn;
	
	/** Allows specifying IAP to iRConn */ 
	TCommDbConnPref iConnPref;
	
	/** The TCP/IP socket, allowing to send/receive data */
	RSocket     	iSocket;	
		 
	/** The listener used to notify about events on channels. */
	MPppEndpointListener* iListener; 	
		
	//
	// The simplex channels 
	//
	
	/** Receiver channel */
	CRx* iRxAO;
	
	/** Transmitter channel */
	CTx* iTxAO;
	};

} // namespace te_ppploopback
#endif
