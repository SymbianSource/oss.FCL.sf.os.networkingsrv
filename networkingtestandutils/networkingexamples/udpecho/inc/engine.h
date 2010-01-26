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
// engine.h - udp echo engine
//



/**
 @internalComponent
*/
#ifndef __ENGINE_H
#define __ENGINE_H

#include <es_sock.h>
#include <in_sock.h>


class CUdpEchoView;

class CUdpEchoEngine : public CActive
{
public:
	//constructor
	CUdpEchoEngine();

	//destructor
	~CUdpEchoEngine();
	
	//second phase constructor
	void ConstructL(CUdpEchoView *aAppView);
	TInt Start();
	void StopL();
	inline CUdpEchoView *AppView()
		{return iAppView;}
	inline TBool Running()
		{return (iState != EIdle);}

protected:
    // Active Object functions
    void RunL();
	void DoCancel();

private:
	void ReceivePacket();
    void SendPacket();
    void CloseTcpConnection();
    void CloseSockets();
public:
	// Variables accessed from the options dialog
	TUint	iProtocol;
	TUint	iAction;
	TUint	iPort;
	TInetAddr iMcastGroup;
	TBool		iPerPacketTrace;	// If true, each received buffer/packet causes trace output.

private:
	CUdpEchoView *iAppView;
    RSocketServ iSocketServer;
	RSocket		iSocket;
	RSocket		iAcceptSocket;
	RSocket		*iActiveSocket;  // Points to active socket, TCP->AcceptSoc, UDP->Socket
	TInetAddr	iAddr;
	enum TState {EIdle,EOpening,EReceiving,ESending,EClosing};
    TState      iState;
    TInt        iTotalBytesRcvd;
    HBufC8      *iEchoBuffer;
    TPtr8       iEchoData;
	TUint8		iSeq;
#ifdef UDP_PEEK
    TUint       iFlags;
#endif
    TSockXfrLength iNumRcvd;
};

#endif
