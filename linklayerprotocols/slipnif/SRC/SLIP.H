/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file SLIP.H
 @internalComponent
*/

#if !defined(__SLIP_H__)
#define __SLIP_H__

#include <es_mbuf.h>
#include <comms-infras/nifif.h>
#include <nifutl.h>
#include <es_prot.h>

////////////////////////////////////////////////////////////////////////////////

const TInt KSlipDefaultMtu = 576; ///< default Maximum transmit unit
const TUint KSlipEndCh = 0300;    ///< End of frame
const TUint KSlipEscCh = 0333;
const TUint KSlipEscEndCh = 0334;
const TUint KSlipEscEscCh = 0335;
const TUint KSlipWriterPrority = 20;
const TUint KSlipReaderPrority = 10;

////////////////////////////////////////////////////////////////////////////////

enum TSlipPanic {	PanicSlipAssert1,
					PanicSlipAssert2,
					PanicSlipAssert3,
					PanicSlipAssert4,
					PanicSlipAssert5,
					PanicSlipAssert6,
					PanicSlipAssert7,
					PanicSlipAssert8

				};
void SlipPanic(TSlipPanic aPanic);

enum TSlipFault {};

void SlipFault(TSlipFault aFault);

////////////////////////////////////////////////////////////////////////////////
// SLIP Protocol Factory
////////////////////////////////////////////////////////////////////////////////

NONSHARABLE_CLASS(CSlipIfFactory) : public CNifIfFactory
/**
@internalComponent
*/
	{
protected:
	virtual void InstallL();
	virtual CNifIfBase* NewInterfaceL(const TDesC& aName);
	TInt Info(TNifIfInfo& aInfo, TInt aIndex) const;
	};

////////////////////////////////////////////////////////////////////////////////
// Slip Interface
////////////////////////////////////////////////////////////////////////////////

const TUint KSlipBufSize = 1024;

class TSlipBuf : public TBuf8<KSlipBufSize>
/**
@internalComponent
*/
	{
public:
#ifndef _DEBUG
	inline const TUint8 &operator[](TInt aIndex) const
		{ return iBuf[aIndex]; }
	inline TUint8 &operator[](TInt aIndex)
		{ return iBuf[aIndex]; }
#endif
	};

const TUint KSlipIsUp = 0x01;
const TUint KSlipSendBusy = 0x02;
const TUint KSlipSendInUse = 0x04;
const TUint KSlipRecvEscPend = 0x08;
const TUint KSlipCommConfigOk = 0x10;

NONSHARABLE_CLASS(CSlip) : public CNifIfLink, public MComm
/**
@internalComponent
*/
	{
public:
	CSlip(CNifIfFactory& aFactory);
	~CSlip();

	virtual void BindL(TAny *aId);
	virtual TInt State();
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource=0);
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual TInt Start();
	virtual void Stop(TInt aReason, MNifIfNotify::TAction aAction);
	virtual CNifIfBase* GetBinderL(const TDesC& aName);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	virtual void Restart(CNifIfBase* aIf);
	static void FillInInfo(TNifIfInfo& aInfo);
	//
	void CommReadComplete(TInt aStatus);
	void CommWriteComplete(TInt aStatus);
	
	// Slip Family 
	static void FillinInfo(TServerProtocolDesc &aEntry);
private:
	void DoRecv();
	void DoSend();
	void LinkDown(TInt aReason);
	TInt PacketModeOn();
	TInt PacketModeOff();
	TInt SpeedMetric();
private:
	TCommConfig iOrigConfig;
	TUint iFlags;
	CProtocolBase* iIpProtocol;
	
	/** Sender */
	CAsyncCallBack* iSendCallBack;

	RMBufPktQ iSendPktQ;
	RMBufChain iSendPkt;
	RMBuf* iSendMBuf;
	TUint8* iSendMPtr;
	TSlipBuf iSendBuf;

	TInt iSendMBufs;	///< Send current level 
	TInt iSendLoWat;	///< Send flow on level 
	TInt iSendHiWat;	///< Send flow off level 
	TSlipBuf iRecvBuf;	///< Receiver 

	TInt iRecvLen;
	RMBuf* iRecvMBuf;
	TUint8* iRecvMPtr;
	RMBufQ iRecvQ;
	};

#endif
