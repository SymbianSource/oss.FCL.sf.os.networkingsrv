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
// This is a pep header file
//

#ifndef _PACKETINTERFACE_
#define _PACKETINTERFACE_

#include <e32base.h>
#include <NIFMBUF.H>

class MNifContextNotify
{
public:
	virtual void UnblockIndication() = 0;
	virtual void BlockIndication() = 0;
	virtual void ProcessPacket(RMBufChain& aPacket) = 0;
};

class CPEP;
class CPEPManager;
class CPacketInterface
{
friend class CPEPManager;
friend class CPEP;
//friend class CBlockSim;	// TEMp
public:	

	IMPORT_C void Send(RMBufChain &aPacket);	
	IMPORT_C ~CPacketInterface();				

	IMPORT_C void Block();
	
	IMPORT_C TBool ChannelOpen() const;	

	//Temp
	TUint8 ProxyId() { return iProxyId; };
	//
private:
	void UnBlock();

	CPacketInterface();	
	void SetPeP(CPEP *aPEP);
	CPEP *iPEP;
	void InitL(CPEPManager* aManager,TUint8 aProxyId);	

	RMBufPktQ iSendQueue;			// Queue of packets for this context	
	TInt iFlowState;				// Flow blocked?
	CAsyncCallBack* iSendCallBack;	// Callback object sending the packets	
	
	TUint8 iProxyId;
	CPEPManager *iManager;
	MNifContextNotify *iContextNotifier;

	//Temp
	TBool iFirst;

};

#endif
