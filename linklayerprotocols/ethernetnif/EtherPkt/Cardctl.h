// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @internalComponent 
*/

#if !defined(__CARDCTL_H__)
#define __CARDCTL_H__

#include <d32ethernet.h>

#include "carddrv.h"
#include <comms-infras/connectionsettings.h>	// for KSlashChar

const TInt KConfigBufferSize = 12;


NONSHARABLE_CLASS(CIOBuffer) : public CBase
/**
Generic buffer class
Enables HBufC8 pointers to be queued
@internalComponent
*/
{
public:
	~CIOBuffer();
	inline HBufC8*	Data() const {return iBuf;};
	void FreeData();
	inline TPtr8& Ptr() {return iBufPtr;};
	void Assign(HBufC8* aBuffer = NULL);
    static CIOBuffer* NewL(HBufC8* aBuf = NULL);
    static CIOBuffer* NewL(const TInt aSize);
	static TInt LinkOffset();

private:
	CIOBuffer();
	void ConstructL(const TInt aSize);
	//	void Construct(HBufC8* aBuffer);
	
	TSglQueLink iLink;
	HBufC8* iBuf;
	TPtr8 iBufPtr;
};


// Main Card Control class. Controls open, close, read, write etc 
class CPcCardSender;
class CPcCardReceiver;
class CPcCardIOCTL;
class CPcCardEventHandler;

NONSHARABLE_CLASS(CPcCardControlEngine) : public CBase
/**
@internalComponent
*/
{
public:
	friend class CPcCardSender;
	friend class CPcCardReceiver;
	friend class CPcCardIOCTL;
	friend class CPcCardEventHandler;

	static CPcCardControlEngine *NewL(CPcCardPktDrv* aPktDrv);
	~CPcCardControlEngine();
	void StartL();
	void Stop();
	TUint8* GetInterfaceAddress();
	TInt Send(HBufC8* aBuffer);
	TBool CardOpen(){return iCardOpen;};

#if (!defined __WINS__)
	void ParseMACFromFileL();
#endif


private:
	CPcCardControlEngine(CPcCardPktDrv* aPktDrv);
	void ConstructL();
	void ProcessReceivedPacket(TDesC8 &aBuffer);
	void ResumeSending();
	void LinkLayerUp();
	void LoadDeviceDriversL();

private:
	TBool iCardOpen;
	CPcCardPktDrv* iNotify;

	TBuf8<KConfigBufferSize> iConfig;
	
	CPcCardSender* iSender;
	CPcCardReceiver* iReceiver;
	CPcCardEventHandler* iEventHandler;
	RBusDevEthernet iCard;
	TInt iPcmciaSocket;
	
	TBuf<KCommsDbSvrDefaultTextFieldLength>	iPDDName;
	TBuf<KCommsDbSvrDefaultTextFieldLength>	iLDDName;
};

NONSHARABLE_CLASS(CPcCardSender) : public CActive
/**
Writer Active object class
Queues buffers for transmit
@internalComponent
*/
{
public:
	static CPcCardSender* NewL(CPcCardControlEngine* aParent);
	~CPcCardSender();
	TInt Send(CIOBuffer* aBuffer);
	void EmptyQueue();

private:
	virtual void RunL();
	virtual void DoCancel();
	CPcCardSender();
	void InitL(CPcCardControlEngine* aParent);

	TInt iQueueLength;
	CPcCardControlEngine* iParent;
	TSglQue<CIOBuffer> iTxQueue;
	TBool iStopSending;
};

NONSHARABLE_CLASS(CPcCardReceiver) : public CActive
/**
Reader active object
One receive buffer only, read queue handled by LDD
@internalComponent
*/
{
public:
	static CPcCardReceiver* NewL(CPcCardControlEngine* aParent);
	~CPcCardReceiver();
	void QueueRead();
private:
	virtual void RunL();
	virtual void DoCancel();
	CPcCardReceiver();
	void InitL(CPcCardControlEngine* aParent);

	CPcCardControlEngine* iParent;
	HBufC8* iRecvBuffer;
	TUint iRecvBufLength;
	TPtr8 iRecvBufPtr;
};

NONSHARABLE_CLASS(CPcCardEventHandler) : public CActive
/**
@internalComponent
*/
{
public:
	~CPcCardEventHandler();
	static CPcCardEventHandler* NewL(CPcCardControlEngine* aParent);
	void GetEvent();
private:
	virtual void RunL();
	virtual void DoCancel();
	CPcCardEventHandler();
	void InitL(CPcCardControlEngine* aParent);

	TBuf8<32>	iEventBuffer;
	CPcCardControlEngine* iParent;
};

NONSHARABLE_CLASS(CPcCardIOCTL) : public CActive
/**
@internalComponent
*/
{
public:
	~CPcCardIOCTL();
	static CPcCardIOCTL* NewL(CPcCardControlEngine* aParent);
	TInt Ioctl(const TUint8 aIOCTLCode);
private:
	virtual void RunL();
	virtual void DoCancel();
	CPcCardIOCTL();
	void InitL(CPcCardControlEngine* aParent);
	TBuf8<32>	iIOCTLBuffer;
	TUint8 iCurrentIOCTL;

	CPcCardControlEngine* iParent;
};

#endif
