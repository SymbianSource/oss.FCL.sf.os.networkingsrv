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
//

/**
 @file Connectionutils.CPP
*/

#include "connectionutils.h"

	CProgress::CProgress(RConnection& aConnection, CLogger& aLogger, TCommDbConnPref& aPref)
		: CActive(CActive::EPriorityStandard), iConn(aConnection), iPref(aPref), iLogger(aLogger)
		{}

	CProgress::~CProgress()
		{
		Cancel();
		}

	void CProgress::Progress(TRequestStatus* aStatus)
		{
		iSavedStatus = aStatus;
		if (iSavedStatus)
			*iSavedStatus = KRequestPending;
			
		iConn.ProgressNotification(iProgress, iStatus);
		SetActive();
		}

	void CProgress::RunL()
		{
		TInt err = iStatus.Int();
		// When progres is complete, do
		if(err == KErrNone)
			{
			iLogger.Log(_L("Iap %d on network %d progress %d completed, error %d"),
				iPref.IapId(), iPref.NetId(), iProgress().iStage , iProgress().iError);
			if (iProgress().iStage != KLinkLayerOpen)
				Progress();
			else if(iSavedStatus)
				{
				iLogger.Log(_L("Completing Progress object"));
				User::RequestComplete(iSavedStatus, err);
				iSavedStatus = NULL;
				}
			}
		}

	void CProgress::DoCancel()
		{
		if (iStatus.Int() == KRequestPending)
			iConn.CancelProgressNotification();
		}


	CReaderWriter::CReaderWriter(RSocket& aSocket, TIsReader aIsReader) 
		:CActive(CActive::EPriorityStandard), iSocket(aSocket),
		iSavedStatus(NULL), iBuf(0,0), iIsReader(aIsReader)
		{}

	void CReaderWriter::DoTransfer(TUint aBufSize, TIsQuick aIsQuickie, TRequestStatus* aStatus)
		{
		iBufSize = aBufSize;
		
		if (IsActive())
			{
			User::Invariant();
			}

		if (!iHeap)
			{
			iHeap = HBufC8::NewMaxL(iBufSize);
			iHeap->Des().Repeat( _L8("TCP-buffer:x helloworld") );
			}

		iSavedStatus = aStatus;
		if (iSavedStatus)
			*iSavedStatus = KRequestPending;
			
		StartRequest(KErrNone, aIsQuickie);
		}

	CReaderWriter::~CReaderWriter()
		{
		Cancel();
		if (iHeap) delete iHeap;
		}

	TInt CReaderWriter::BytesTransfered(void) { return iBufCount*iBufSize; } 
		
	void CReaderWriter::StartRequest(TInt err, TIsQuick aIsQuickie)
		{
		if ( (err == KErrNone) )
			{
			if (iIsReader == EReader)
				{
				// The reader starts reading after activation, 
				// It finishes when someone cancels it
				iBuf.Set(iHeap->Des());
				iSocket.Read(iBuf, iStatus);
				iBufCount++;
				}
			else 
				{
				if (aIsQuickie == ENormal)
					{
					// initialise data
					iScratchData.Format(_L8("TCP-buffer:%d helloworld"), ++iBufCount);
					iHeap->Des().Repeat( iScratchData );
					}				
				iSocket.Write(*iHeap, iStatus);
				}

			if (iSavedStatus)
				SetActive();
			else 
				User::WaitForRequest(iStatus);
			}
		}
	
	void CReaderWriter::RunL()
		{
		TInt err = iStatus.Int();
		if (iSavedStatus)
			{
			User::RequestComplete(iSavedStatus, err);
			iSavedStatus = NULL;
			}
		}

	void CReaderWriter::DoCancel()
		{
		if (iIsReader == EReader)
			iSocket.CancelRead();
		else if (iIsReader == EWriter)
			iSocket.CancelWrite();
		}


	CSender::CSender(CLogger& aLogger, RSocketServ& aSS,
		TCommDbConnPref& aPref)
		: CActive(CActive::EPriorityStandard),
		iSS(aSS), iPref(aPref), iLogger(aLogger)
		{}

	CSender::~CSender()
		{
#ifdef _USE_QOS
		delete iQosParams;
		iQosChannel.Close();
#endif
		Cancel();
		if (iReader) delete iReader;
		if (iWriter) delete iWriter;
		}

#ifdef _USE_QOS
	void CSender::DoInitL(const TInetAddr& anAddr, const TInt aPort, RConnection& aConn, CQoSParameters* parameters = NULL)
#else
	void CSender::DoInitL(const TInetAddr& anAddr, const TInt aPort, RConnection& aConn)
#endif
		{
		iRemote.SetV4MappedAddress(anAddr.Address());
		iRemote.SetPort(aPort);
		
		TInt err = iSock.Open(iSS, KAfInet, KSockStream, KProtocolInetTcp, aConn);
		User::LeaveIfError(err);
#ifdef _USE_QOS
		if (parameters)
			{
			iQosParams = new(ELeave) CQoSParameters;
			iQosParams->CopyL(*parameters);
			}
#endif
		}

#ifdef _USE_QOS
	void CSender::DoInitL(const TInetAddr& anAddr, const TInt aPort, CQoSParameters* parameters = NULL)
#else
	void CSender::DoInitL(const TInetAddr& anAddr, const TInt aPort)
#endif
	{
		iRemote.SetV4MappedAddress(anAddr.Address());
		iRemote.SetPort(aPort);
		
		TInt err = iSock.Open(iSS, KAfInet, KSockStream, KProtocolInetTcp);
		User::LeaveIfError(err);
#ifdef _USE_QOS
		if (parameters)
			{
			iQosParams = new(ELeave) CQoSParameters;
			iQosParams->CopyL(*parameters);
			}
#endif
		}
	
	void CSender::StartActive(TRequestStatus* aStatus, TAction aAction)
		{
		iAction = aAction;
		iSavedStatus = aStatus;

		// Mark the saved state as pending
		if (iSavedStatus) 
			*iSavedStatus = KRequestPending;

		TBuf<39> AddrBuf;
		iRemote.OutputWithScope(AddrBuf);
		
		iLogger.Log(_L("Trying to connect socket to %S on Iap %d on Network %d"), 
			&AddrBuf, iPref.IapId(), iPref.NetId());
		
		iSock.Connect(iRemote, iStatus);
		iState = EConnecting;
		// this is commented so that the first socket gets connected first
		//SetActive();
		User::WaitForRequest(iStatus);
		RunL();
		}

	void CSender::StartPassive(TInt aPort, TRequestStatus* aStatus, RConnection& aConn, TAction aAction)
		{
		iAction = aAction;
		iSavedStatus = aStatus;
		if (!iSavedStatus) User::Invariant();
		
		// Open a sock for listening on a connection
		TInt nRet = iListener.Open(iSS, KAfInet, KSockStream, KProtocolInetTcp, aConn);
		if (nRet != KErrNone)
			{
			iLogger.Log(_L("Failed to open socket on Iap %d on Network %d, : return value = %d"),
				nRet, iPref.IapId(), iPref.NetId());
			User::Leave(nRet);
			}

		// bind the socket
		iLocal.SetPort(aPort);
		nRet = iListener.Bind(iLocal);
		if (nRet != KErrNone)
			{
			iLogger.Log(_L("Failed to bind socketon Iap %d on Network %d, : return value = %d"),
				nRet, iPref.IapId(), iPref.NetId());
			User::Leave(nRet);
			}

		// listen on the socket
		nRet = iListener.Listen(1);
		if (nRet != KErrNone)
			{
			iLogger.Log(_L("Failed to listen on socketon Iap %d on Network %d, : return value = %d"),
				nRet, iPref.IapId(), iPref.NetId());
			User::Leave(nRet);
			}

		// open a blank socket
		nRet = iSock.Open(iSS);
		if (nRet != KErrNone)
			{
			iLogger.Log(_L("Failed to open Receiver socketon Iap %d on Network %d, : return value = %d"),
				nRet, iPref.IapId(), iPref.NetId());
			User::Leave(nRet);
			}

		// Mark the saved state as pending
		if (iSavedStatus)
			*iSavedStatus = KRequestPending;
		
		iLogger.Log(_L("Trying to Accept on Iap %d on Network %d"), iPref.IapId(), iPref.NetId());
		iListener.Accept(iSock, iStatus);
		// Wait for async incoming connction
		iState = EConnecting;
		//SetActive();
		User::WaitForRequest(iStatus);
		RunL();
		}
	
	TInt CSender::GetResult() { return iLastError; }

	void CSender::RunL()
		{
		TInt err = iStatus.Int();
		if(err == KErrNone)
			{
			if (iState == EConnecting)
				{
				TBuf<39> AddrBuf;
				iSock.LocalName(iLocal);
				iLocal.SetPort(iSock.LocalPort());
				iLocal.OutputWithScope(AddrBuf);
				iLogger.Log(_L("Socket on Iap %d on Network %d Connected"), iPref.IapId(), iPref.NetId());
				iLogger.Log(_L("Local address is %S"), &AddrBuf);
				iSock.RemoteName(iRemote);
				iRemote.OutputWithScope(AddrBuf);
				iLogger.Log(_L("Remote address is %S"), &AddrBuf);
#ifdef _USE_QOS				
				if (iQosParams)
					{
					RequestQosL();
					
					SetActive();
					return;
					//CActiveScheduler::Start();
					}
#endif
				iReader = new(ELeave) CReaderWriter(iSock, CReaderWriter::EReader);
				iWriter = new(ELeave) CReaderWriter(iSock, CReaderWriter::EWriter);
				(CActiveScheduler::Current())->Add(iReader);
				//(CActiveScheduler::Current())->Add(iWriter);
				iState = EReading;
				}

			if (iState != EReading) User::Invariant();

			if (iAction == EReadWrite)
				{
				if (iSendCount < KBufCount)
					{
					iLogger.Log(_L("Writing a buffer on Iap %d on Network %d"), iPref.IapId(), iPref.NetId());
					iWriter->DoTransfer(KStdBufSize);
					iSendCount++;
					
					iLogger.Log(_L("Reading a buffer on Iap %d on Network %d"), iPref.IapId(), iPref.NetId());
					iReader->DoTransfer(KStdBufSize, CReaderWriter::EQuick, &iStatus);
					SetActive();
					iState = EReading;
					return;				
					}
				}
			if(iAction == EWrite)
				{
				iLogger.Log(_L("Writing %d buffers of size %d on Iap %d on Network %d"), 
						KBufCount, KTtcpBufSize, iPref.IapId(), iPref.NetId());
				
				TTime start, now;
				start.HomeTime();
				// sync
				while(iSendCount++ < KBufCount)
					iWriter->DoTransfer(KTtcpBufSize) ;

				now.HomeTime();
				TInt64 us = now.MicroSecondsFrom(start).Int64()/1000;
				iLogger.Log(_L("Time taken to transmit %d bytes - %d Milli sec"), iWriter->BytesTransfered(), us);
				}
			if(iAction == ERead)
				{
				iLogger.Log(_L("Reading %d buffers of size %d on Iap %d on Network %d"), 
						KBufCount, KTtcpBufSize, iPref.IapId(), iPref.NetId());
				
				TTime start, now;
				start.HomeTime();
			
				while(iSendCount++ < KBufCount)
					iReader->DoTransfer(KTtcpBufSize) ;

				now.HomeTime();
				TInt64 us = now.MicroSecondsFrom(start).Int64()/1000;
				iLogger.Log(_L("Time taken to read %d bytes - %d Milli sec"), iReader->BytesTransfered(), us);
				}
			}

		else if (iState == EConnecting)
			iLogger.Log( _L("Socket connect/accept on Iap %d on Network %d failed with error %d"), iPref.IapId(), iPref.NetId(), err);
		
		iLastError = err;
		iSock.Shutdown(RSocket::ENormal, iStatus);
		User::WaitForRequest(iStatus);
		if (iSavedStatus)
			User::RequestComplete(iSavedStatus, err);
		else 
			CActiveScheduler::Stop();
		}

	TInt CSender::RunError(TInt aError)
		{
		iLogger.Log(_L("CSender Active Object Left with error %d"), aError);
		iLastError = aError;
		iSock.Shutdown(RSocket::ENormal, iStatus);
		User::WaitForRequest(iStatus);
		if (iSavedStatus)
			User::RequestComplete(iSavedStatus, aError);
		else 
			CActiveScheduler::Stop();
		return KErrNone;
		}

	void CSender::DoCancel()
		{
		if (iStatus.Int() == KRequestPending)
			{
			if (iState == EConnecting) iSock.CancelConnect();

			if (iReader) iReader->Cancel();
			if (iWriter) iWriter->Cancel();
			}
		}

#ifdef _USE_QOS
	void CSender::RequestQosL()
		{
		if (iState != EConnecting)
			User::Invariant();
		if (iQosParams != NULL)
			{
			TInt err = iQosChannel.Open(iSock);
			iLogger.Log(_L("Opened QoS channel: Err = %d"), err);
			User::LeaveIfError(err);
			CleanupClosePushL(iQosChannel);

			err = iQosChannel.NotifyEvent(*this);
			iLogger.Log(_L("requested QoS notification: Err = %d"), err);
			User::LeaveIfError(err);

			err = iQosChannel.SetQoS(*iQosParams);
			iLogger.Log(_L("Set QoS : return value = <%d>"), err);
			User::LeaveIfError(err);
			iState = ESettingQos;
			iStatus = KRequestPending;
			CleanupStack::Pop();
			}
		}

	void CSender::Event(const CQoSEventBase& aQoSEvent)
		{
		if (iState != ESettingQos)
			User::Invariant();

		iLogger.Log(_L("QoS event type %d\n"), aQoSEvent.EventType());

		iQosEvent = (TQoSEvent)aQoSEvent.EventType();
		switch (iQosEvent)
			{
		case EQoSEventConfirm:
			iLogger.Log(_L("Got reqested QOS"));
			break;
		default:
			iLogger.Log(_L("Failed to get QOS"));
			break;
			};

		delete iQosParams;
		iQosParams = NULL;
		iState = EConnecting;
		//CActiveScheduler::Stop();
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrNone);
		}
#endif
