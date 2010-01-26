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
// loop6.h - loopback interface for IPv6
//



/**
 @internalComponent
*/
#ifndef __LOOP6_H__
#define __LOOP6_H__

//
// In Epoc R6 nifman.h has been split, CNifIfBase definition has been moved
// into <comms-infras/nifif.h>. 
//
#include <nifman.h>
#	include <comms-infras/nifif.h>	// ..for CNifIfBase in Epoc R6 and later
#include <nifmbuf.h>

const TUint KProtocolInet6Loop = 0x1F00;	// Virtual

class CIfLoop6 : public CNifIfBase
{
    public:
	CIfLoop6();
	virtual void ConstructL(const TDesC& aTag);
	~CIfLoop6();

	static CIfLoop6* NewL(const TDesC& aTag);
	
	virtual void BindL(TAny *aId);
	virtual TInt State();
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource);
	virtual TInt Notification(TAgentToNifEventType /* aEvent */, void * /* aInfo */ )
	{
		return KErrNone;
	};  
	
    protected:
	virtual void DoSend();
	virtual void DoProcess();
	void Loop(RMBufPacket& aTxPkt, RMBufPacket& aRxPkt);

    private:
	static TInt RecvCallBack(TAny* aCProtocol);
	static TInt SendCallBack(TAny* aCProtocol);

    protected:
	CProtocolBase* iNetwork;
	RMBufPktQ iSendQ;
	RMBufPktQ iRecvQ;
	CAsyncCallBack* iSendCallBack;
	CAsyncCallBack* iRecvCallBack;

    private:
	HBufC* iIfaceName;
	TInt iIfaceMTU;
};

#endif
