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
 @file Connectiontester.CPP
*/

#include "connectiontester.h"

LOCAL_C void GetStatsL(RConnection& aConn, RSocketServ& aSS, CLogger& aLogger);

#pragma warning (disable:4238)
LOCAL_C void PrintRoutingTableL(RSocketServ& aSS, CLogger& aLogger);


void GetStatsL(RConnection& aConn, RSocketServ& aSS, CLogger& aLogger)
		{
		TUint count;
		User::LeaveIfError(aConn.EnumerateConnections(count));

		RConnection conn;
		TRequestStatus Status;
		TPckgBuf<TConnectionInfo> tempinfo;

		for (TUint i = 1; i <= count; i++)
			{
			// get details
			User::LeaveIfError(aConn.GetConnectionInfo(i, tempinfo));
			User::LeaveIfError(conn.Open(aSS));
			User::LeaveIfError(conn.Attach(tempinfo, RConnection::EAttachTypeMonitor));

			TInt UpLoad = 0, Dowload = 0;
			TPckg<TUint> UpLoadb(UpLoad), Dowloadb(Dowload);
			conn.DataTransferredRequest(UpLoadb, Dowloadb, Status);
			User::WaitForRequest(Status);
			if (Status.Int() == KErrNone)
				aLogger.Log(_L("Downloaded %d bytes, Uploaded %d bytes on Iap %d on Network %d"),
					UpLoad, Dowload, tempinfo().iIapId, tempinfo().iNetId);
			else aLogger.Log(_L("DataTransferredRequest returned error %d on Iap %d on Network %d"),
					Status.Int(), tempinfo().iIapId, tempinfo().iNetId);
			conn.Close();
			}
		}

#pragma warning (disable:4238)
void PrintRoutingTableL(RSocketServ& aSS, CLogger& aLogger)
		{
		RSocket socket;
		TInt err = socket.Open(aSS, KAfInet, KSockDatagram, KProtocolInetUdp);
		User::LeaveIfError(err);

		CleanupClosePushL(socket);

		TName address1, address2, address3, address4, address5;
		HBufC* hBuf = HBufC::NewLC(0x300);
		TPtr buf = hBuf->Des();

		User::LeaveIfError(socket.SetOpt(KSoInetEnumRoutes, KSolInetRtCtrl));
		TPckgBuf<TSoInetRouteInfo> next;
		aLogger.Log(_L("Routing table contents:"));
		_LIT(KFormat,"%39S | %39S | %17S | %39S | %6S");
		_LIT(KDestAddrLabel, "Destination Address");
		_LIT(KNetMaskLabel, "Netmask");
		_LIT(KGatewayAddrLabel, "Gateway Address");
		_LIT(KIfAddrLabel, "Interface Address");
		_LIT(KMetricLabel, "Metric");
		buf.AppendFormat(KFormat,
			&KDestAddrLabel, &KNetMaskLabel,
			&KGatewayAddrLabel, &KIfAddrLabel, &KMetricLabel);
		aLogger.Log(buf);
		_LIT16(KFormat1,"%39S | %39S | %17S | %39S | %6d");
		FOREVER
			{
			buf.Zero();
			err = socket.GetOpt(KSoInetNextRoute, KSolInetRtCtrl, next);
			if (err != KErrNone)
				break;
			
			next().iDstAddr.OutputWithScope(address1);
			next().iNetMask.OutputWithScope(address2);
			next().iGateway.OutputWithScope(address3);
			next().iIfAddr.OutputWithScope(address4);
			buf.AppendFormat(KFormat1, &address1, &address2, &address3, &address4, next().iMetric);
			
			aLogger.Log(_L("%S"), &buf);
			}
		aLogger.Log(KNullDesC);

		User::LeaveIfError(socket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl));
		TPckgBuf<TSoInetInterfaceInfo> interf;
		TPckgBuf<TSoInetIfQuery> query;
		aLogger.Log(_L("Interface table contents:"));

		TBuf<64> zone;
		_LIT(KFormatInt1,"%16S | %48S | %6S |%6S | %6S | %17S | %17S | %17S | %17S | %17S");
		_LIT(KIfNameLabel, "Interface name");
		_LIT(KZonesLabel, "Zones @ [Scopes]");
		_LIT(KStateLabel, "State");
		_LIT(KMTULabel, "MTU");
		_LIT(KGwayOrPeerAddrLabel, "GW/peer address");
		_LIT(KNameServer1Label, "Name server1");
		_LIT(KNameServer2Label, "Name server2");
		buf.Zero();
		buf.AppendFormat(KFormatInt1,
			&KIfNameLabel, &KZonesLabel,
			&KStateLabel, &KMTULabel, &KMetricLabel,
			&KIfAddrLabel,
			&KGwayOrPeerAddrLabel, &KDestAddrLabel, 
			&KNameServer1Label, &KNameServer2Label);
		aLogger.Log(buf);
		err = KErrNone;
		_LIT16(KFormatInt2,"%16S | %48S | %6d |%6d | %6d | %17S | %17S | %17S | %17S | %17S");
		FOREVER
			{
			err = socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, interf);
			if (err != KErrNone)
				break;
			query().iName = interf().iName;
			err =  socket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, query);
			if (err != KErrNone)
				break;
			
			query().iDstAddr.OutputWithScope(address1);
			interf().iAddress.OutputWithScope(address2);
			interf().iDefGate.OutputWithScope(address3);
			interf().iNameSer1.OutputWithScope(address4);
			interf().iNameSer2.OutputWithScope(address5);
			zone.Zero();
			for (TInt i = 0; i < 16; i++)
				zone.AppendFormat(_L("%2d:"), query().iZone[i]);
			
			buf.Zero();
			buf.AppendFormat(KFormatInt2, &interf().iName,  &zone,
				interf().iState, interf().iMtu, interf().iSpeedMetric,
				&address2, &address3, &address1, &address4, &address5);
			aLogger.Log(_L("%S"), &buf);
			}
		aLogger.Log(KNullDesC);

		CleanupStack::PopAndDestroy();
		CleanupStack::Pop();
		socket.Close();
		}

	 CConnTester* CConnTester::NewLC(CLogger* aLogger)
		{
#ifdef __WINS__
		#define CDRV1_PATH _L("ECDRV")
		#define COMM_PATH _L("ECOMM")
	    User::LoadPhysicalDevice(CDRV1_PATH);    
	    User::LoadLogicalDevice(COMM_PATH);
#endif

		CConnTester* self = new(ELeave) CConnTester(aLogger);
		CleanupStack::PushL(self);

		User::LeaveIfError(self->iSS.Connect());
		User::LeaveIfError(self->iConn.Open(self->iSS));
		aLogger->Log(_L("Connected to Socket server"));
		return self;
		}

	CConnTester::~CConnTester()
		{
		Cancel();
		iConn.Stop();
		iSS.Close();

		if(iProgress)
			delete iProgress;
		}

	void CConnTester::StartConenction(TInt aIapId, TInt aNetId, TCommDbDialogPref aPref)
		{
		if(IsActive())
			{
			iLogger->Log(_L("Conenction object Already active"));
			return;
			}

		if (!(iConnState == EIdle))
			User::Invariant();

		if ((aIapId == 0) && (aNetId == 0))
			{
			iLogger->Log(_L("Starting Iap"));
			iConn.Start(iStatus);
			}
		else
			{
			iPref.SetIapId(aIapId);
			iPref.SetNetId(aNetId);
			iPref.SetDialogPreference(aPref);

			iLogger->Log(_L("Starting Iap %d on Network %d"), aIapId, aNetId);
			iConn.Start(iPref, iStatus);
			}

		iConnState = EStarting;
		SetActive();
		}

	// Get and store connection info in iInfo structure
	void CConnTester::GetConnectionInfo(TUint aIapId)
		{
		// Enumerate conenctions
		TUint count;
		User::LeaveIfError(iConn.EnumerateConnections(count));
	
		for (TUint i = 1; i <= count; i++)
			{
			// get details
			User::LeaveIfError(iConn.GetConnectionInfo(i, iInfo));
			if ( (iInfo().iIapId == aIapId) )//&& (iInfo().iNetId == aNetId))
				break;
			}
		iPref.SetIapId(iInfo().iIapId);
		iPref.SetNetId(iInfo().iNetId);
		}

	TInt CConnTester::JoinConnection(TUint aIapId, TUint aNetId)
		{
		iLogger->Log(_L("Joining connection %d on Network %d"), aIapId, aNetId);
		if (iConnState != EIdle)
			User::Invariant();

		GetConnectionInfo(aIapId);
		User::LeaveIfError(iConn.Attach(iInfo, RConnection::EAttachTypeNormal));

		iConnState = EJoined;

		return KErrNone;
		}

	TInt CConnTester::StopConnection(TUint aIapId, TUint aNetId)
		{
		if ((iConnState == EStarted) || (iConnState == EJoined))
			{
			iErr = iConn.Stop();
			iConnState = EIdle;
			return iErr;
			}

		if (iConnState == EIdle)
			User::LeaveIfError(JoinConnection(aIapId, aNetId));

		iLogger->Log(_L("Stopping Iap %d on Network %d"), iInfo().iIapId, iInfo().iNetId);
		iConnState = EIdle;
		iErr = iConn.Stop();
		return iErr;
		}

	void CConnTester::GetProgress()
		{
		if (!(iConnState == EStarting))
			User::Invariant();

		iProgress = new(ELeave) CProgress(iConn, *iLogger, iPref);
		(CActiveScheduler::Current())->Add(iProgress);
		iProgress->Progress();
		iIsProgressActive = EProgressStart;
		
		iConnState = EProgress;
		}

#ifdef _USE_QOS
	void CConnTester::StartTransferL(const TInetAddr& anAddr, const TInt aPort, CQoSParameters* aQosParameters, TInt aTransferType)
#else
	void CConnTester::StartTransferL(const TInetAddr& anAddr, const TInt aPort, TInt aTransferType)
#endif
		{
		if (!((iConnState == EStarted) || (iConnState == EJoined)))
			User::Invariant();

		iSender = new(ELeave) CSender(*iLogger, iSS, iPref);
		(CActiveScheduler::Current())->Add(iSender);
#ifdef _USE_QOS
		iSender->DoInitL(anAddr, aPort, iConn, aQosParameters);
#else
		iSender->DoInitL(anAddr, aPort, iConn);
#endif
		// This request is completed only after the socket is connected
		CSender::TAction type = (CSender::TAction)aTransferType;
		if(type == CSender::ERead)
			iSender->StartPassive(0, &iStatus, iConn, type);
		else 
			iSender->StartActive(&iStatus, type);
		// Do this to get the wait after every send
		//iSender->StartSending(anAddr, aPort, ETrue);
		iConnState = ETransfer;
		SetActive();
		}

	void CConnTester::GetStatsL() { ::GetStatsL(iConn, iSS, *iLogger); } 

	void CConnTester::PrintRoutingTableL() { ::PrintRoutingTableL(iSS, *iLogger); }

	TInt CConnTester::State() { return (TInt) iConnState; }

	TInt CConnTester::GetResult() { return iErr; }

	RConnection& CConnTester::GetConnection() { return iConn; }

	CConnTester::CConnTester(CLogger* aLogger)
		: CActive(CActive::EPriorityStandard), iLogger(aLogger)
		{
		iConnState = EIdle;
		(CActiveScheduler::Current())->Add(this);
		}

	void CConnTester::RunL()
		{
		iErr = iStatus.Int();

		// Notify the active scheduler about the state completion
		switch(iConnState)
			{
			case(EStarting):
			case(EProgress):
				{
				if (iProgress) 
					{
					if (!(iIsProgressActive == EProgressStart))
						User::Invariant();
	
					iIsProgressActive = EProgressDone;
					delete iProgress;
					iProgress = NULL;
					}

				if (iErr == KErrNone)
					{
					iConnState = EStarted;
					GetConnectionInfo(iPref.IapId());
					}
				else 
					iConnState = EError;
				iLogger->Log(_L("Started Iap %d on Network %d with error %d"), iInfo().iIapId, iInfo().iNetId, iErr);
				break;
				}
			case(ETransfer):
				{
				if (!(iSender))
					User::Invariant();
				delete iSender;
				iSender = NULL;
				iConnState = EStarted;
				break;
				}
			default:
				break;
			}

		CActiveScheduler::Stop();
		}

	void CConnTester::DoCancel()
		{
		switch(iConnState)
			{
			case(EStarting):
				{
				iConn.Stop();
				break;
				}
			case(EProgress):
				{
				if (!(iProgress))
					User::Invariant();
				iProgress->Cancel();
				delete iProgress;
				iProgress = NULL;
				iIsProgressActive = EProgressDone;
				break;
				}
			case(ETransfer):
				{
				if (!(iSender))
					User::Invariant();
				iSender->Cancel();
				delete iSender;
				iSender = NULL;
				break;
				}
			default:
				break;
			}
		}
