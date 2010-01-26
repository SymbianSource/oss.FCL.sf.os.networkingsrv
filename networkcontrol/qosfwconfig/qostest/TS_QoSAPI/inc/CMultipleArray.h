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
 
#if (!defined __CMULTIPLEARRAY_H_)
#define __CMULTIPLEARRAY_H_

#include "TestSuite.h"
#include "TestStep.h"
#include <Es_sock.h>
#include <in_sock.h>
#include "qoslib.h"
#include "cdbcols.h"
#include "commdbconnpref.h"
#include "TS_QoSStep.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

class CTS_QoSStep;
class CMultipleArray : public CBase
{
public:
	// Constructer
	CMultipleArray();
	// Destructor
	~CMultipleArray();


        static CMultipleArray *NewLC();
	void Initialize(CTS_QoSStep * aQoSStep);
	
	/*
	 * QoS Parameter Refrence Methods
	 */
	TInt AppendInteger(TInt aInteger);
	TInt Get(TInt aPosition);
	TInt Count();
	CQoSParameters* SetQoSParametersL();
	TBool CompareQoSParameters(CQoSParameters* iQoSNetworkParameters);

	/*
	 * Socket Operations for Implicit and Explicit sockets
	 */
	void AddSocketToListL(RSocket* aSocket);
	RSocket* GetSocketHandle(TInt anIndex);
	TInt GetSocketListCount();
	void CloseSocket(TInt aIndex);
	void OpenExplicitTcpSocketL(TInt aExtraSocksToJoin, RSocketServ iSocketServer);
	void OpenImplicitTcpSocketL(RSocketServ iSocketServer);
	void OpenExplicitUdpSocketL(TInt aExtraSocksToJoin, RSocketServ iSocketServer);
	void OpenImplicitUdpSocketL(RSocketServ iSocketServer);
	void BindL(TInt aExtraSocksToJoin);
	void ConnectL(TInt aExtraSocksToJoin);
	void SendAndRecvL(TInt aExtraSocksToJoin);

	/* 
	 * Connection Operations for Explicit sockets
	 */
	void AddConnectionToListL(RConnection* aConnection);
	RConnection* GetConnectionHandle(TInt anIndex);
	TInt GetConnectionListCount();
	void CloseConnection(TInt aIndex);

	/* QoS Parameter Refrence in config file i.e their may be many diffrent QoS Parameter Refrence Mainly used 
	 * for when the QoS gets rejected by the network another QoS Parameter Refrence may be applied 
	 */
	// Used to store QoS Parameter Refrences
	RArray<TInt> iIntegers;
	 
	// Used for Explicit TCP and UDP Socket Connection(s)
	CArrayFixSeg<RSocket *> * iSocketList;			// Array of Sockets
	CArrayFixSeg<RConnection *> * iConnectionList;	// Array of Connections
	RQoSChannel* iQoSChannel;						// QoS Channel
	
	RArray<TInt> nSocketsExtraJoinInt;

	/*
	 * Connection Parameters
	 */
	RArray<TUint> iProtocol;						// TCP or UDP Protocol
	RArray<TInetAddr> iScrAddr;						// Source IP Address for each Socket
	RArray<TInetAddr> iDestAddr;					// Destination IP Address for each Socket
	RArray<TInt> iPort;								// Port Number for each Socket
	RArray<TInt> iPacketSize;						// Packet Size for each Socket Connection
	RArray<TInt> iPackets;							// Number Of Packets To Send for each Socket Connection
	RArray<TInt> iIAP;								// IAP location of IAP within the Commdb
	RArray<TCommDbBearer> iSetBearerSet;			// SetBearerSet location of SetBearerSet within the Commdb
    RArray<TBool> iBind;							// True if Bind has been done else False
	RArray<TBool> iConnect;							// True if Connect has been done else False

	/*
	 * QoS Parameters
	 */
	TInt iTokenRateUplink, iTokenBucketSizeUplink, iMaxTransferRateUplink, iMaxPacketSizeUplink, 
		 iMinPolicedUnitUplink, iDelayUplink, iTokenRateDownlink, iTokenBucketSizeDownlink, 
		 iMaxTransferRateDownlink, iMaxPacketSizeDownlink, iMinPolicedUnitDownlink, iDelayDownlink;
	TInt iPriorityUplink, iPriorityDownlink;
	TInt iDropModeUplink, iDropModeDownlink;
	TInt iAdaptationPriority;
	// 1 = True / 0 = False
	TInt iAdaptMode;

	// pointer to step, used so that we can do the logging 
	CTS_QoSStep * iQoSStep;

private:
	void ConstructL();	

};

#endif /* CMULTIPLEARRAY_H */
