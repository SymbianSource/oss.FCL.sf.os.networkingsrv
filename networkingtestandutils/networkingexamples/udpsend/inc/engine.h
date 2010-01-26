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
// engine.h - udp packet sender engine
//



/**
 @internalComponent
*/
#ifndef __ENGINE_H
#define __ENGINE_H

#include <es_sock.h>
#include <in_sock.h>

#define SECOND		1000000
#define MILISECOND	1000
#define UDP_HDR_LEN	8

class CUDPSendView;
class CPingSingleSender;

class CUDPSendEngine : public CTimer
{
public:
	//constructor
	CUDPSendEngine();

	//destructor
	~CUDPSendEngine();
	
	//second phase constructor
	void ConstructL(CUDPSendView *aAppView);
	void FirstRunL();
	void StartL();
	void Stop();
	inline CUDPSendView *AppView() const
		{return iAppView;}
	inline TBool Running() const
		{return iRunning;}
	void SetHostName(const TDesC &aHostname) { iHostname = aHostname; }
	const TDesC* GetHostName() const { return &iHostname; }

protected:
	//Issues next RunL execution
	void IssueRequest();

	// will send all the packets
	void RunL();

	//Cancel Packet Sending
	void DoCancel();
	
	TBool SendFirstPacketL();
	void SendPacket();
	TBool LaunchHostnameDialogL();
	
public:
	// Variables accessed from the options dialog
	TUint	iProtocol;
	TUint	iDestPort;
	TUint	iTotalPackets;
	TUint	iDelay;
	TUint	iPacketSize;
	TBool	iSendSynch;
	TUint	iPriority;
	TUint	iFlowLabel;
	TUint	iTcpNagling;
	TInt	iTcpLinger;

private:
	CPingSingleSender *iSender;
	CUDPSendView *iAppView;
	TBuf<80> iHostname;         // Contains hostname to ping
	TBool iRunning;
};

#endif
