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
 
#include "CMultipleArray.h"


// Constructor
CMultipleArray::CMultipleArray()
{
}



CMultipleArray* CMultipleArray::NewLC()
{
	CMultipleArray* self= new (ELeave) CMultipleArray;
	CleanupStack::PushL(self);
	self->ConstructL();
    	return self;
}

// Destructor
CMultipleArray::~CMultipleArray()
{

	TInt i;

	// Explicit opening of sockets
	// if test case 1_6 or 1_7 have been run then we need to close connections.
	if (GetConnectionListCount() > 0)
		{
		iQoSStep->iQoSSuite->Log(_L("Closing <%d> connection(s)"), GetConnectionListCount());
		for (TInt i = GetConnectionListCount(); i > 0; i--)
			{
			iQoSStep->iQoSSuite->Log(_L("Close connection <%d>"),i);
			CloseConnection(i);
			}
		}
	
	// if any sockets have been left open then close them
	if (GetSocketListCount() > 0)
		{
	
		// Close any Open Sockets
		iQoSStep->iQoSSuite->Log(_L("Closing<%d> socket(s)"), GetSocketListCount());
		for (i  = GetSocketListCount(); i > 0; i--)
			{
			iQoSStep->iQoSSuite->Log(_L("NClose socket <%d>"), i);
			CloseSocket(i);
			}
		}

	iIntegers.Close();
	
	// Close RArray Objects
	iProtocol.Close();		// TCP or UDP Protocol
	iScrAddr.Close();		// Source IP Address for each Socket
	iDestAddr.Close();		// Destination IP Address for each Socket
	iPort.Close();			// Port Number for each Socket
	iPacketSize.Close();	// Packet Size for each Socket Connection
	iPackets.Close();		// Number of Packets for each Socket Connection
	iBind.Close();
	iConnect.Close();
	nSocketsExtraJoinInt.Close();

	// Delete pointers
	delete iConnectionList;
	delete iSocketList;
	delete iQoSChannel;
}

void CMultipleArray::ConstructL()
{
	iSocketList = new(ELeave) CArrayFixSeg<RSocket*>(1);
	iConnectionList = new(ELeave) CArrayFixSeg<RConnection*>(1);
	iQoSChannel = new (ELeave) RQoSChannel;

}
void CMultipleArray::Initialize(CTS_QoSStep * aQoSStep)
{
	iQoSStep = aQoSStep;
}

/*
 * QoS Parameter Refrence Methods
 */
TInt CMultipleArray::AppendInteger(TInt aInteger)
{
	return iIntegers.Append(aInteger);
}

TInt CMultipleArray::Get(TInt aPosition)
{
	return iIntegers[aPosition];
}

TInt CMultipleArray::Count()
{
	return iIntegers.Count();
}


/*********************
 * Socket Operations *
 *********************/

/* Add socket to array of type RSocket CArrayFixSeg<RSocket *>
 */
void CMultipleArray::AddSocketToListL(RSocket* aSocket)
	{
/* Add a socket handle to the end of the array
 * Return the current array size, use this to retrieve handle
 */
	TRAPD(err, iSocketList->AppendL(aSocket));

	if(err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("The array to store the sockets has left to insufficient memory available with the error code <%d>"), err);
		User::Leave(err);
		}

//	return iSocketList->Count();
	}


/* Returns the socket handle
 */
RSocket* CMultipleArray::GetSocketHandle(TInt anIndex)
	{
	return iSocketList->At(anIndex);
	}

/*
 * Returns number of elements in the array of type RSocket CArrayFixSeg<RSocket *>
 */
TInt CMultipleArray::GetSocketListCount()
	{
	return iSocketList->Count();
	}

/*
 * Close a Socket from the array of type RSocket CArrayFixSeg<RSocket *>
 */
void CMultipleArray::CloseSocket(TInt aIndex)
	{
/* Close the socket
 * Remove entry from list
 */
	RSocket* sock = iSocketList->At(aIndex - 1);
	sock->Close();
	iSocketList->Delete(aIndex - 1);
	delete sock;
	}


/* 
 * Open a Implicit Tcp Socket
 */
void CMultipleArray::OpenImplicitTcpSocketL(RSocketServ iSocketServer)
	{
   /* Get number of sockets from script
	* Open a TCP socket
	* Store socket handle
	* Return the EFail or EPass
	*/
	RSocket* sock;

	sock = new (ELeave) RSocket;
	CleanupStack::PushL(sock);
	TInt err = sock->Open(iSocketServer, KAfInet, KSockStream, KProtocolInetTcp);

	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}
		
	AddSocketToListL(sock);
	CleanupStack::Pop();	//sock
	}

/* 
 * Open a Explicit Tcp Socket
 */
void CMultipleArray::OpenExplicitTcpSocketL(TInt aExtraSocksToJoin, RSocketServ iSocketServer)
	{
   /* Get number of sockets from script
	* Open a TCP socket
	* Store socket handle
	* Return the EFail or EPass
	*/
	RSocket* sock;
	RConnection* conn;
	TCommDbConnPref prefs;
	TInt err = 0;

	sock = new (ELeave) RSocket;
	conn = new (ELeave) RConnection;
	CleanupStack::PushL(sock);
	CleanupStack::PushL(conn);

	// Start an outgoing connection
	err = conn->Open(iSocketServer);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}

	// Setup SetBearerSet
	prefs.SetBearerSet(iSetBearerSet[aExtraSocksToJoin]);
	// Setup IAP
	prefs.SetIapId(iIAP[aExtraSocksToJoin]);

	err = conn->Start(prefs);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}

	err = sock->Open(iSocketServer, KAfInet, KSockStream, KProtocolInetTcp, *conn);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}
		
	AddSocketToListL(sock);
	AddConnectionToListL(conn);

	CleanupStack::Pop(2);	//sock and conn
	}

/* 
 * Open an Implicit UDP Socket
 */
void CMultipleArray::OpenImplicitUdpSocketL(RSocketServ iSocketServer)
	{
   /* Get number of sockets from script
	* Open a UDP socket
	* Store socket handle
	* Return the EFail or EPass
	*/
	RSocket* sock;

	sock = new (ELeave) RSocket;
	CleanupStack::PushL(sock);
	TInt err = sock->Open(iSocketServer, KAfInet, KSockDatagram, KProtocolInetUdp);


	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}

	AddSocketToListL(sock);
	CleanupStack::Pop();	//sock
	}

/* 
 * Open an Explicit UDP Socket
 */
void CMultipleArray::OpenExplicitUdpSocketL(TInt aExtraSocksToJoin, RSocketServ iSocketServer)
	{
   /* Get number of sockets from script
	* Open a UDP socket
	* Store socket handle
	* Return the EFail or EPass
	*/
	RSocket* sock;
	RConnection* conn;
	TCommDbConnPref prefs;
	TInt err = 0;
		
	sock = new (ELeave) RSocket;
	conn = new (ELeave) RConnection;
	CleanupStack::PushL(sock);
	CleanupStack::PushL(conn);

	// Start an outgoing connection
	err = conn->Open(iSocketServer);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open Connection: return value = <%d>"), err);
		User::Leave(err);
		}
		
	// Setup SetBearerSet
	prefs.SetBearerSet(iSetBearerSet[aExtraSocksToJoin]);	
	// Setup IAP
	prefs.SetIapId(iIAP[aExtraSocksToJoin]);

	err = conn->Start(prefs);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}

	err = sock->Open(iSocketServer, KAfInet, KSockDatagram, KProtocolInetUdp, *conn);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to open socket: return value = <%d>"), err);
		User::Leave(err);
		}
		
	AddSocketToListL(sock);
	AddConnectionToListL(conn);

	CleanupStack::Pop(2);	//sock and conn
	}

/*
 * Bind Socket to an Address (We then get a unique local port assigned to the Socket)
 * We need to do this for TCP or UDP before Opening a QoS Channel with the Socket
 * This is so QoS Channel Sockets can be identified from one another
 */
void CMultipleArray::BindL(TInt aExtraSocksToJoin)
	{
	TInt result, err;

	if (iProtocol[aExtraSocksToJoin] == KProtocolInetUdp)
		result = (GetSocketHandle(aExtraSocksToJoin))->SetOpt(KSoUdpSynchronousSend, KSolInetUdp, 1 );
	
	// Bind to Local address for both TCP and UDP
	result = GetSocketHandle(aExtraSocksToJoin)->Bind(iScrAddr[aExtraSocksToJoin]);
	if (result!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to bind socket: return value = <%d>"), result);
		User::Leave(result);
		}

	// Set Bind to True so we know we have already done a Bind
	err = iBind.Insert(ETrue, aExtraSocksToJoin);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to insert value into iBind RArray : return value = <%d>"), err);
		User::Leave(err);
		}
	}


/*
 *  Connect TCP / Bind UDP then Connect
 */
void CMultipleArray::ConnectL(TInt aExtraSocksToJoin)
{
	/* Set to an IPv4 or IPv6 address
	 * Set to a listening port
	 * Connect to the address on listening port n 
	 */
	TRequestStatus stat;
	TInt result, err;

	// Connect TCP
	if (iProtocol[aExtraSocksToJoin] == KProtocolInetTcp)
		{
		// Get Destination Address and Set the Destination Port
		iDestAddr[aExtraSocksToJoin].SetPort(iPort[aExtraSocksToJoin]);
		
		// Connect to the Destination Address
		GetSocketHandle(aExtraSocksToJoin)->Connect(iDestAddr[aExtraSocksToJoin], stat);
		User::WaitForRequest(stat);
		if (stat!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to connect socket: return value = <%d>"), stat);
		User::Leave(stat.Int());
		}

		// Set Bind to True so we know we have already done a Bind, Connect does a bind for us
		err = iBind.Insert(ETrue, aExtraSocksToJoin);
		if (err!=KErrNone)
			{
			iQoSStep->iQoSSuite->Log(_L("Failed to insert value into iBind RArray : return value = <%d>"), err);
			User::Leave(err);
			}
	
		// Set iConnect to True so we know we have made a connection for later
		err = iConnect.Insert(ETrue, aExtraSocksToJoin);
		if (err!=KErrNone)
			{
			iQoSStep->iQoSSuite->Log(_L("Failed to insert value into iConnect RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		}

	// Connect UDP
	else if (iProtocol[aExtraSocksToJoin] == KProtocolInetUdp)
		{
		// Get Destination Address and Set the Destination Port
		iDestAddr[aExtraSocksToJoin].SetPort(iPort[aExtraSocksToJoin]);
		// Check to see if we have done a Bind on the Local Address allready
		if (iBind[aExtraSocksToJoin] != 1)
			{
			result = (GetSocketHandle(aExtraSocksToJoin))->SetOpt(KSoUdpSynchronousSend, KSolInetUdp, 1 );
			// Bind to Local address
			result = GetSocketHandle(aExtraSocksToJoin)->Bind(iScrAddr[aExtraSocksToJoin]);
			if (result!=KErrNone)
				{
				iQoSStep->iQoSSuite->Log(_L("Failed to bind socket to local address: return value = <%d>"), result);
				User::Leave(result);
				}
				
			// Set Bind to True so we know we have already done a Bind
			err = iBind.Insert(ETrue, aExtraSocksToJoin);
			if (err!=KErrNone)
				{
				iQoSStep->iQoSSuite->Log(_L("Failed to insert value into iBind RArray : return value = <%d>"), err);
				User::Leave(err);
				}
			}

		// Connect to the Destination Address
		GetSocketHandle(aExtraSocksToJoin)->Connect(iDestAddr[aExtraSocksToJoin], stat);
		User::WaitForRequest(stat);
		if (stat!=KErrNone)
			{
			iQoSStep->iQoSSuite->Log(_L("Failed to connect socket to destination address: return value = <%d>"), stat);
			User::Leave(stat.Int());
			}

		// Set iConnect to True so we know we have made a connection for later
		err = iConnect.Insert(ETrue, aExtraSocksToJoin);
		if (err!=KErrNone)
			{
			iQoSStep->iQoSSuite->Log(_L("Failed to insert value into iConnect RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		}
	}

void CMultipleArray::SendAndRecvL(TInt aExtraSocksToJoin)
	{
	TRequestStatus stat;
	TBuf8<50> Data;

	// set up data buffers
	HBufC8 * writebuf = HBufC8::NewMaxLC( iPacketSize[aExtraSocksToJoin] );
	HBufC8 * readbuf  = HBufC8::NewMaxLC( iPacketSize[aExtraSocksToJoin] );
	TPtr8 ptrWritebuf = writebuf->Des();
	TPtr8 ptrReadbuf = readbuf->Des();

	TInt recvCount = 0;

	// Send / Recv TCP
	if (iProtocol[aExtraSocksToJoin] == KProtocolInetTcp)
		{
		for (TInt i = 0; i < iPackets[aExtraSocksToJoin]; i++)
			{
			iQoSStep->iQoSSuite->Log( _L("Sending TCP data, %d packets of %d bytes = %d"), 
			iPackets[aExtraSocksToJoin], iPacketSize[aExtraSocksToJoin], iPackets[aExtraSocksToJoin] * iPacketSize[aExtraSocksToJoin]);

			// initialise data
			Data.Format(_L8("TCP-packet:%d helloworld"),i);
			ptrWritebuf.Repeat( Data );

			// write data
			GetSocketHandle(aExtraSocksToJoin)->Write(ptrWritebuf, stat);
			User::WaitForRequest(stat);
			if (stat!=KErrNone)
				{
				iQoSStep->iQoSSuite->Log(_L("Failed to write tcp data to destination: return value = <%d>"), stat);
				User::Leave(stat.Int());
				}

			// read data
			GetSocketHandle(aExtraSocksToJoin)->Read(ptrReadbuf, stat);
			User::WaitForRequest(stat); //, TimerStatus);
			if (stat!=KErrNone)
				{
				iQoSStep->iQoSSuite->Log(_L("Failed to read tcp data from destination: return value = <%d>"), stat);
				User::Leave(stat.Int());
				}

			// compare the data
			if (ptrWritebuf.Compare( ptrReadbuf ) != 0)
				{
				iQoSStep->iQoSSuite->Log(_L("Data written to and read from destination address do not match in size"));
//				return Fail;
				}

			recvCount+=ptrReadbuf.Length();
			}

		CleanupStack::PopAndDestroy(2);	//	writebuf and readbuf
		} 

	// Send / Recv UDP 
	if (iProtocol[aExtraSocksToJoin] == KProtocolInetUdp)
		{
		iQoSStep->iQoSSuite->Log( _L("Send Udp Data, %d packets of %d bytes = %d"), 
		iPackets[aExtraSocksToJoin], iPacketSize[aExtraSocksToJoin], iPackets[aExtraSocksToJoin] * iPacketSize[aExtraSocksToJoin]);
	
		for (TInt i = 0; i < iPackets[aExtraSocksToJoin]; i++)
			{
			// initialise data
			Data.Format(_L8("UDP-packet:%d helloworld"),i);
			ptrWritebuf.Repeat( Data );	

			// write data
			GetSocketHandle(aExtraSocksToJoin)->Send(ptrWritebuf, 0, stat);
			User::WaitForRequest(stat);
			if (stat!=KErrNone)
				{
				iQoSStep->iQoSSuite->Log(_L("Failed to write udp data to destination: return value = <%d>"), stat);
				User::Leave(stat.Int());
				}

			GetSocketHandle(aExtraSocksToJoin)->Recv(ptrReadbuf, 0, stat);
			User::WaitForRequest(stat);
			if (stat!=KErrNone)
				{
				iQoSStep->iQoSSuite->Log(_L("Failed to read udp data from destination: return value = <%d>"), stat);
				User::Leave(stat.Int());
				}

			// compare the data
			if (ptrWritebuf.Compare( ptrReadbuf ) != 0 )
				{
				iQoSStep->iQoSSuite->Log(_L("Data written to and read from destination address do not match in sizevalue"));
//				return Fail;
				}
			recvCount += ptrReadbuf.Length();
			}

		// get rid of the old buffers 
		CleanupStack::PopAndDestroy(2);	//	writebuf and readbuf
		}


	// check the total received (95 per cent is allowable for us)
	if (recvCount*iPacketSize[aExtraSocksToJoin] < (0.95*(iPackets[aExtraSocksToJoin]*iPacketSize[aExtraSocksToJoin])))
		{
		iQoSStep->iQoSSuite->Log(_L("The total packets received is less than 95 per cent of the overall packets sent"));
//		return Fail;
		}
	}


/*************************
 * Connection Operations *
 *************************/

/* Add connection to array of type RConnection CArrayFixSeg<RConnection *>
 */
void CMultipleArray::AddConnectionToListL(RConnection* aConnection)
{
	TRAPD(err, iConnectionList->AppendL(aConnection));

	if(err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("The array to store the connection has left to insufficient memory available with the error code <%d>"), err);
		User::Leave(err);
		}

//	return iConnectionList->Count();
}


/* Returns the connection handle
 */
RConnection* CMultipleArray::GetConnectionHandle(TInt anIndex)
	{
//	return (RSocket *) (iSocketList->At(anIndex));
	return iConnectionList->At(anIndex);
	}

/*
 * Returns number of elements in the array of type RConnection CArrayFixSeg<RConnection *>
 */
TInt CMultipleArray::GetConnectionListCount()
	{
	return iConnectionList->Count();
	}

/*
 * Close a Connection from the array of type RConnection CArrayFixSeg<RConnection *>
 */
void CMultipleArray::CloseConnection(TInt aIndex)
	{
/* Close the Connection
 * Remove entry from list
 */
	RConnection* conn = iConnectionList->At(aIndex - 1);
	conn->Close();
	iConnectionList->Delete(aIndex - 1);
	delete conn;
	}

/*
 * Setup QoS Parameters
 */
CQoSParameters* CMultipleArray::SetQoSParametersL()
{
	CQoSParameters *parameters;
	TInt err = 0;

	parameters = new (ELeave) CQoSParameters;
	CleanupStack::PushL(parameters);

	// Uplink Parameters
    parameters->SetTokenRateUplink(iTokenRateUplink);
    parameters->SetTokenBucketSizeUplink(iTokenBucketSizeUplink);
    parameters->SetMaxTransferRateUplink(iMaxTransferRateUplink);
    parameters->SetMaxPacketSizeUplink(iMaxPacketSizeUplink);
    parameters->SetMinPolicedUnitUplink(iMinPolicedUnitUplink);
    parameters->SetDelayUplink(iDelayUplink);
    parameters->SetPriorityUplink((TUint16)iPriorityUplink);
    err = parameters->SetDropModeUplink((TUint8)iDropModeUplink);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to set DropModeUpLink : return value = <%d>"), err);
		User::Leave(err);
		}

	// Downlink Parameters
    parameters->SetTokenRateDownlink(iTokenRateDownlink);
    parameters->SetTokenBucketSizeDownlink(iTokenBucketSizeDownlink);
    parameters->SetMaxTransferRateDownlink(iMaxTransferRateDownlink);
    parameters->SetMaxPacketSizeDownlink(iMaxPacketSizeDownlink);
    parameters->SetMinPolicedUnitDownlink(iMinPolicedUnitDownlink);
    parameters->SetDelayDownlink(iDelayDownlink);
    parameters->SetPriorityDownlink((TUint16)iPriorityDownlink);
    err = parameters->SetDropModeDownlink((TUint8)iDropModeDownlink);
	if (err!=KErrNone)
		{
		iQoSStep->iQoSSuite->Log(_L("Failed to set DropModeDownLink : return value = <%d>"), err);
		User::Leave(err);
		}

	// Adaptor Parameters
    parameters->SetAdaptationPriority((TUint8)iAdaptationPriority);
    parameters->SetAdaptMode(iAdaptMode);

	CleanupStack::Pop();	// parameters

	return parameters;
}

TBool CMultipleArray::CompareQoSParameters(CQoSParameters* iQoSNetworkParameters) // Pass Network QoS Parameters
	{
	// Compare App QoS Parameters with Network QoS Parameters

	// Uplink Parameters
	if (!iQoSNetworkParameters->TokenRateUplink() == iTokenRateUplink)
		return FALSE;
    if (!iQoSNetworkParameters->MaxTransferRateUplink() == iMaxTransferRateUplink)
		return FALSE;
	if (!iQoSNetworkParameters->MaxPacketSizeUplink() == iMaxPacketSizeUplink)
		return FALSE;
    if (!iQoSNetworkParameters->MinPolicedUnitUplink() == iMinPolicedUnitUplink)
		return FALSE;
	if (!iQoSNetworkParameters->PriorityUplink() == iPriorityUplink)
		return FALSE;
    if (!iQoSNetworkParameters->DropModeUplink() == iDropModeUplink)
		return FALSE;
	if (!iQoSNetworkParameters->TokenRateDownlink() == iTokenRateDownlink)
		return FALSE;
    if (!iQoSNetworkParameters->TokenBucketSizeDownlink() == iTokenBucketSizeDownlink)
		return FALSE;
	if (!iQoSNetworkParameters->MaxTransferRateDownlink() == iMaxTransferRateDownlink)
		return FALSE;
    if (!iQoSNetworkParameters->MaxPacketSizeDownlink() == iMaxPacketSizeDownlink)
		return FALSE;
	if (!iQoSNetworkParameters->MinPolicedUnitDownlink() == iMinPolicedUnitDownlink)
		return FALSE;
    if (!iQoSNetworkParameters->DelayDownlink() == iDelayDownlink)
		return FALSE;
	if (!iQoSNetworkParameters->PriorityDownlink() == iPriorityDownlink)
		return FALSE;
    if (!iQoSNetworkParameters->DropModeDownlink() == iDropModeDownlink)
		return FALSE;
    if (!iQoSNetworkParameters->AdaptationPriority() == iAdaptationPriority)
		return FALSE;
	if (!iQoSNetworkParameters->AdaptMode() == iAdaptMode)
		return FALSE;
	
	return TRUE;
	}
