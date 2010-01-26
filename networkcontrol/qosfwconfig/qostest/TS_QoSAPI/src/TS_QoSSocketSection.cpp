// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// TS_QoSSocketSectionSection1.cpp
// This file contains an example Test step implementation 
// This demonstrates the various functions provided
// by the CTestStep base class which are available within
// a test step 
// 
//

// Test system includes
#include "TS_QoSSocketSection.h"


/* 
 * Close Socket(s)
 */
CTS_QoSSocketSection1_3::CTS_QoSSocketSection1_3()
{
	/* store the name of this test case
	* this is the name that is used by the script file
	*/
	iTestStepName = _L("Close_Sockets_Test1.3");
}

CTS_QoSSocketSection1_3::~CTS_QoSSocketSection1_3()
{
}

enum TVerdict CTS_QoSSocketSection1_3::doTestStepL( void )
	{
	/* get number of sockets currently open
	* close all open sockets
	* Return pass
	*/
	TInt i, i2;

	// Loop through number of QoS Channels
	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
	{
		Log(_L("QoS Channel <%d>"),i);
			
		// Close Connections used for Explicit Tcp/Udp connections
		Log(_L("Closing <%d> connections(s)"), iQoSSuite->iQoSChannel[i]->GetConnectionListCount());
		if (iQoSSuite->iQoSChannel[i]->GetConnectionListCount() > 0)
			{
			Log(_L("Closing <%d> connection(s)"), iQoSSuite->iQoSChannel[i]->GetConnectionListCount());

			for (i2 = iQoSSuite->iQoSChannel[i]->GetConnectionListCount(); i2 > 0; i2--)
				{
				Log(_L("Close connection <%d>"),i2);
				iQoSSuite->iQoSChannel[i]->CloseConnection(i2);
				}
			}

		// Close connection details
		Log(_L("Closing <%d> set of connection details(s)"), iQoSSuite->iQoSChannel[i]->GetSocketListCount());
		for (i2 = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); i2 > 0; i2--)
			{
			iQoSSuite->iQoSChannel[i]->iSetBearerSet.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iIAP.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iProtocol.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iScrAddr.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iDestAddr.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iPort.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iPacketSize.Remove(i2-1);
			iQoSSuite->iQoSChannel[i]->iPackets.Remove(i2-1);
			// Not done a bind to local address yet
			iQoSSuite->iQoSChannel[i]->iBind.Remove(i2-1);
			// Not done a connect to remote address yet
			iQoSSuite->iQoSChannel[i]->iConnect.Remove(i2-1);
			// Not yet Set the QoS Parameters
			iQoSSuite->iQoSSet.Remove(i2-1);
			iQoSSuite->iQoSChangeSet.Remove(i2-1);
			iQoSSuite->iJoin.Remove(i2-1);
			iQoSSuite->iLeave.Remove(i2-1);
			}

		// Close Sockets
		Log(_L("Closing <%d> socket(s)"), iQoSSuite->iQoSChannel[i]->GetSocketListCount());
		for (i2 = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); i2 > 0; i2--)
			{
			Log(_L("Close socket <%d>"),i2);
			iQoSSuite->iQoSChannel[i]->CloseSocket(i2);
			}
		}

	return EPass;
	}


//
// Send TCP / UDP Data //
//

CTS_CEsockSendAndRecvData::CTS_CEsockSendAndRecvData()
	{
	// store the name of this test case
	iTestStepName = _L("SendAndRecvDataTest");
	}

CTS_CEsockSendAndRecvData::~CTS_CEsockSendAndRecvData()
	{
	}

enum TVerdict CTS_CEsockSendAndRecvData::doTestStepL( void )
	{
	TRequestStatus stat, stat2;
	TBuf8<50> Data;

	// Qos Channel(s)
	for (TInt i = 0;i < iQoSSuite->iQoSChannel.Count();i++)
		{
		// Socket(s)
		for (TInt i2 = 0;i2 < iQoSSuite->iQoSChannel[i]->GetSocketListCount() ;i2++)
			{
			// set up data buffers
			HBufC8 * writebuf = HBufC8::NewMaxLC( iQoSSuite->iQoSChannel[i]->iPacketSize[i2] );
			HBufC8 * readbuf  = HBufC8::NewMaxLC( iQoSSuite->iQoSChannel[i]->iPacketSize[i2] );
			TPtr8 ptrWritebuf = writebuf->Des();
			TPtr8 ptrReadbuf = readbuf->Des();

			TInt recvCount = 0;

			Log( _L("QoS Channel <%d>, Socket <%d>"), i+1, i2+1);

			// Send / Recv TCP
			if (iQoSSuite->iQoSChannel[i]->iProtocol[i2] == KProtocolInetTcp)
				{
				Log( _L("Sending TCP data, %d packets of %d bytes = %d"), 
				iQoSSuite->iQoSChannel[i]->iPackets[i2], iQoSSuite->iQoSChannel[i]->iPacketSize[i2], iQoSSuite->iQoSChannel[i]->iPackets[i2] * iQoSSuite->iQoSChannel[i]->iPacketSize[i2]);
				
				// Number of Packets to Send / Recv
				for (TInt i4 = 0; i4 < iQoSSuite->iQoSChannel[i]->iPackets[i2]; i4++)
					{
					// initialise data
					Data.Format(_L8("TCP-packet:%d helloworld"),i4);
					ptrWritebuf.Repeat( Data );

					// write data
					iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2)->Write(ptrWritebuf, stat);
					User::WaitForRequest(stat);
					TESTL(stat==KErrNone);

					// read data
					iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2)->Read(ptrReadbuf, stat);
					User::WaitForRequest(stat);
					TESTL(stat==KErrNone);

					// compare the data
					TESTL( ptrWritebuf.Compare( ptrReadbuf ) == 0);
					recvCount+=ptrReadbuf.Length();
					}
				//	writebuf and readbuf
				CleanupStack::PopAndDestroy(2);	
				} 

			// Send / Recv UDP
			if (iQoSSuite->iQoSChannel[i]->iProtocol[i2] == KProtocolInetUdp)
				{
				Log( _L("Send Udp Data, %d packets of %d bytes = %d"), 
				iQoSSuite->iQoSChannel[i]->iPackets[i2], iQoSSuite->iQoSChannel[i]->iPacketSize[i2], iQoSSuite->iQoSChannel[i]->iPackets[i2] * iQoSSuite->iQoSChannel[i]->iPacketSize[i2]);

				// Number of Packets to Send / Recv
				for (TInt i4 = 0; i4 < iQoSSuite->iQoSChannel[i]->iPackets[i2]; i4++)
					{
					// initialise data
					Data.Format(_L8("UDP-packet:%d helloworld"),i4);
					ptrWritebuf.Repeat( Data );

					// write data 
					iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2)->SendTo(ptrWritebuf, iQoSSuite->iQoSChannel[i]->iDestAddr[i2], 0, stat);
					User::WaitForRequest(stat);
					TESTL(stat==KErrNone);

					iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2)->RecvFrom(ptrReadbuf, iQoSSuite->iQoSChannel[i]->iDestAddr[i2], 0, stat2);
					User::WaitForRequest(stat2);
					TESTL(stat==KErrNone);

					// compare the data
				  	TESTL( ptrWritebuf.Compare( ptrReadbuf ) == 0 );
					recvCount += ptrReadbuf.Length();
					}
				//	writebuf and readbuf
				CleanupStack::PopAndDestroy(2);
				}

			// check the total received (95 per cent is allowable for us)
			TESTL(recvCount*iQoSSuite->iQoSChannel[i]->iPacketSize[i2] > (0.95*(iQoSSuite->iQoSChannel[i]->iPackets[i2]*iQoSSuite->iQoSChannel[i]->iPacketSize[i2])) );	
			}		
		}

	return EPass;
	}
