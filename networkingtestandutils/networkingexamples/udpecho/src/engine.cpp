// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// engine.cpp - udp echo engine
//

#include <es_sock.h>
#include <e32base.h>

#include <nifmbuf.h>
#include "udpecho.h"
#include "engine.h"

#ifdef UDP_PEEK
#	include <ip4_hdr.h>
#	include <ip6_hdr.h>

typedef union { TInet6HeaderIP ip6; TInet6HeaderIP4 ip4; } TIpHdr;
#endif

CUdpEchoEngine::CUdpEchoEngine():
	CActive(EPriorityStandard), iEchoData(0,0)
	{
	}

CUdpEchoEngine::~CUdpEchoEngine()
	{
	Cancel();
	iSocketServer.Close();
		delete iEchoBuffer;
	}


void CUdpEchoEngine::ConstructL(CUdpEchoView *aAppView)
	{
	iAppView = aAppView;
	iEchoBuffer = HBufC8::NewL(1600);
	TPtr8 auxPtr(iEchoBuffer->Des());
	iEchoData.Set(auxPtr);
	iState = EIdle;
	CActiveScheduler::Add(this);	//Adds itself to the scheduler

	//Default values
	iPort = 7;
	iProtocol = UDP;
	iAction	= ECHO;
	iPerPacketTrace = TRUE;

	// Socket server
	TInt err = iSocketServer.Connect();
	if (err != KErrNone)
		{
		User::Panic(_L("Socket Server Connect failed"),err);
		}

	}


TInt CUdpEchoEngine::Start()
	{	
	TInt			err;
	TBuf<5>		 protocol;

	switch (iProtocol)
		{
	case UDP:
		protocol.Format(_L("udp"));
		break;
	case TCP:
		protocol.Format(_L("tcp"));
		break;
	case RAW:
		protocol.Format(_L("rawip"));
		break;
	default:
		iAppView->Write(_L("Bad Protocol type. Can't open socket\n"));
		return KErrCorrupt;
		}

	err = iSocket.Open(iSocketServer, protocol);
	if (err != KErrNone)
		{
		iAppView->Write(_L("Socket Open failed\n"));
		return err;
		}

	// Reuse the port
	iSocket.SetOpt(KSoReuseAddr, KProtocolInetIp, 1);
	err = iSocket.SetLocalPort(iPort);
	if (err != KErrNone)
		{
		iAppView->Write(_L("Socket Bind failed\n"));
		return err;
		}
	
	// Join multicast group if user has given one
	if (iMcastGroup.IsMulticast())
		{
		TIp6Mreq mreq;
		mreq.iAddr = iMcastGroup.Ip6Address();
		mreq.iInterface = 0;
		TPckgBuf< TIp6Mreq > opt(mreq);
		err = iSocket.SetOpt(KSoIp6JoinGroup, KSolInetIp, opt);
		if (err != KErrNone)
			{
			iAppView->Write(_L("Joining multicast failed\n"));
			}
		}

	if (iProtocol == TCP)
		{
		err = iSocket.Listen(5);
		if (err != KErrNone)
			{
			iAppView->Write(_L("Socket Listen failed\n"));
			return err;
			}
		__ASSERT_ALWAYS(!IsActive(), User::Panic(_L("CUdpEchoEngine"),1));

		// Create blank socket to accept connection
		err = iAcceptSocket.Open(iSocketServer);
		if (err != KErrNone)
			{
			iAppView->Write(_L("Blank Socket Open failed\n"));
			return err;
			}
		iSocket.Accept(iAcceptSocket, iStatus);
		SetActive();
		iActiveSocket = &iAcceptSocket;
		TBuf<80> aux;
		aux.Format(_L("Waiting TCP connection on port %d\n"),iPort);
		iAppView->Write(aux);
		iState = EOpening;
		}
	else
		{	// UDP or RAW
		iAddr.SetPort(iPort);		
		TBuf<80> aux;
		if (iProtocol == UDP)
			aux.Format(_L("Waiting UDP packets on port %d\n"), iPort);
		else
			aux.Format(_L("Waiting Raw IP packets on protocol %d\n"), iPort);
		iAppView->Write(aux);
		iActiveSocket = &iSocket;
		iState = EReceiving;
		ReceivePacket();
		}
	iTotalBytesRcvd = 0;
	return KErrNone;
	}


void CUdpEchoEngine::StopL()
	{
	// Leave the multicast group, if one was given
	if (iMcastGroup.IsMulticast())
		{
		TIp6Mreq mreq;
		mreq.iAddr = iMcastGroup.Ip6Address();
		mreq.iInterface = 0;
		TPckgBuf< TIp6Mreq > opt(mreq);
		TInt err = iSocket.SetOpt(KSoIp6LeaveGroup, KSolInetIp, opt);
		if (err != KErrNone)
			{
			iAppView->Write(_L("Leaving multicast failed\n"));
			}
		}
	
	// Cancel();	// This can not be done if waiting TCP connection (bug)
	if (iState == EOpening)
		{
		DoCancel();	// Must simulate because cancel can't be called in this case
		iStatus = KErrNone;
		iState = EClosing;
		RunL();
		}
	else if (iProtocol == TCP && iState != EIdle)
		{
		Cancel();
		iAppView->Write(_L("Closing TCP connection\n"));
		CloseTcpConnection();
		iState = EClosing;
		}
	else
		{	// UDP connection or TCP disconnected, can close immediately
		// Simulate ack from disconnected reguest
		Cancel();
		iStatus = KErrNone;
		iState = EClosing;
		RunL();
		}
	}


void CUdpEchoEngine::RunL()
	{
	TBuf<80> aux;

	if (iStatus == KErrDisconnected || iStatus == KErrEof)
		{
		aux.Format(_L("Connection closed by remote host -- %d bytes received\n"), iTotalBytesRcvd);
		iAppView->Write(aux);
		CloseSockets();
		iState = EIdle;
		// Automatic restart
		Start();
		}
	else if (iStatus != KErrNone)
		{
		iAppView->Write(_L("Something happened\n"));
		aux.Format(_L("RunL received iStatus = %d\n"),iStatus.Int());
		iAppView->Write(aux);
		CloseSockets();
		iState = EIdle;
		iAppView->UpdateBusyMsgL();
		}
	else switch (iState)
		{
		case EIdle:
			break;

		case EOpening:
			iAppView->Write(_L("Connected\n"));
			ReceivePacket();
			iState = EReceiving;
			break;

		case EReceiving:
#ifdef UDP_PEEK
			if (iFlags)
				{
				// Peeking incoming header
				const TIpHdr &ip = *(TIpHdr *)iEchoData.Ptr();
				TInetAddr src;
				TInetAddr dst;
				TInt ttl = 0;
				if (ip.ip4.Version() == 4)
					{
					// IPv4 packet
					src.SetAddress(ip.ip4.SrcAddr());
					dst.SetAddress(ip.ip4.DstAddr());
					ttl = ip.ip4.Ttl();
					}
				else if (ip.ip6.Version() == 6)
					{
					// IPv6 packet
					src.SetAddress(ip.ip6.SrcAddr());
					dst.SetAddress(ip.ip6.DstAddr());
					ttl = ip.ip6.HopLimit();
					}
				else
					{
					// Garbage, uh.. just forget it...
					}
				TBuf<50> src_buf;
				TBuf<50> dst_buf;
				src.Output(src_buf);
				dst.Output(dst_buf);
				aux.Format(_L("- Peek: src=%S dst=%S ttl=%d\n"), &src_buf, &dst_buf, ttl);
				iAppView->Write(aux);
				iEchoData.Set(iEchoBuffer->Des());
				iFlags = 0;
				iActiveSocket->RecvFrom(iEchoData, iAddr, iFlags, iStatus);
				SetActive();
				break;
				}
#endif
			iTotalBytesRcvd += iEchoData.Length();
			iSeq++;
			if (iPerPacketTrace)
				{
				const TInt missed = iEchoData[0] - iSeq;
				iSeq = iEchoData[0];
				if (iEchoData.Length() > 0 && missed > 0)
					aux.Format(_L("Received %d bytes. total %d (missed %d)\n"),iEchoData.Length(), iTotalBytesRcvd, missed);
				else
					aux.Format(_L("Received %d bytes. total %d\n"),iEchoData.Length(), iTotalBytesRcvd);
				iAppView->Write(aux);
				}
			if (iAction == ECHO)
				{
				SendPacket();
				iState = ESending;
				}
			else
				{
				ReceivePacket();
				}
			break;

		case ESending:
			ReceivePacket();
			iState = EReceiving;
			break;

		case EClosing:
			CloseSockets();
			iState = EIdle;
			aux.Format(_L("Echo engine stopped -- %d bytes received\n"), iTotalBytesRcvd);
			iAppView->Write(aux);
			iAppView->UpdateBusyMsgL();
			break;

		default:
			iAppView->Write(_L("Something is wrong, State unknown\n"));
			iState = EIdle;
			break;
		}
	}


void CUdpEchoEngine::ReceivePacket()
	{
	if (iProtocol == TCP)
		iActiveSocket->RecvOneOrMore(iEchoData, 0, iStatus, iNumRcvd);
	else	// UDP
		{
#ifdef UDP_PEEK
		iFlags = KSockReadPeek | KIpHeaderIncluded;
		iEchoData.Set((TUint8 *)iEchoBuffer->Ptr(), 0, sizeof(TInet6HeaderIP));
		iActiveSocket->RecvFrom(iEchoData, iAddr, iFlags, iStatus);
#else
		iActiveSocket->RecvFrom(iEchoData, iAddr, 0, iStatus);
#endif
		}
	SetActive();
	}

void CUdpEchoEngine::SendPacket()
	{
	if (iProtocol == TCP)
		iActiveSocket->Send(iEchoData, 0, iStatus, iNumRcvd);
	else	// UDP
		iActiveSocket->SendTo(iEchoData, iAddr, 0, iStatus);
	SetActive();
	}

void CUdpEchoEngine::CloseSockets()
	{
	iAcceptSocket.Close();
	iSocket.Close();
	iActiveSocket = NULL;
	}

void CUdpEchoEngine::CloseTcpConnection()
	{
	iAcceptSocket.Shutdown(RSocket::ENormal, iStatus);
	SetActive();
	}

void CUdpEchoEngine::DoCancel()
	{
	switch (iState)
		{
	case EIdle:
	case EClosing:
		break;

	case EOpening:
		iAcceptSocket.CancelAccept();
		break;

	case EReceiving:
		if (iActiveSocket)
			iActiveSocket->CancelRecv();
		break;

	case ESending:
		if (iActiveSocket)
			iActiveSocket->CancelSend();
		break;

	default:
		iAppView->Write(_L("Something is really wrong, State unknown\n"));
		if (iActiveSocket)
			iActiveSocket->CancelAll();
		iState = EIdle;
		break;
		}
	}
