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
// Started by MWT, February 1998
// 
//

#if !defined (__TraceRtENG_H__)
#define __TraceRtENG_H__

#if !defined(__IN_SOCK_H__)
#include <in_sock.h>
#endif


class MTraceRtNotificationHandler
	{
public:
	virtual void Starting(const TNameRecord& aTo, TInt aTtl, TInt aDataLen)=0;
	virtual void Probe(TInt aTtl) = 0;
	virtual void Reply(TInt aNo, TInt aDelta, TUint aId) = 0;
	virtual void FromHost(const TNameRecord& aHost) = 0;
	virtual void Finished(TInt aError) =0;
	};

class TTraceRtOptions
	{
public:
	IMPORT_C TTraceRtOptions();
	TUint iMaxTtl;
	TBool iResolveAddress;
	TTimeIntervalMicroSeconds32 iWait;
	TUint iTos;
	THostName iDestname;
	TInt iNrProbes;
	TBool iPrompt;
	};


class CTraceRtTimer;
class CTraceRtSender;
class CTraceRtReceiver;
class HTraceRtHeader;


class CTraceRtEng : public CBase
	{
friend class CTraceRtTimer;
friend class CTraceRtSender;
friend class CTraceRtReceiver;

public:
	IMPORT_C static CTraceRtEng* NewL(MTraceRtNotificationHandler& aUi);
	IMPORT_C ~CTraceRtEng();

	IMPORT_C void SetPriorities(TInt aTimerPriority, TInt aSenderPriority, TInt aReceiverPriority);
	IMPORT_C void Start(const TTraceRtOptions& aOptions);
	IMPORT_C void Cancel();
	IMPORT_C void CancelAndFinished();

protected:
	IMPORT_C void ConstructL(MTraceRtNotificationHandler& aUi);
	IMPORT_C CTraceRtEng();

private:
	void TimerComplete();
	void SendComplete();
	void SendDoCancel();
	void RecvComplete();
	void RecvDoCancel();
	void NextSend();
	void ResolveReply();
	void DoError(TInt aError);


	enum TState { EStopped, ELookingUpHost, ESending, ELookingUpReply };

	TState iState;
	MTraceRtNotificationHandler* iUi;
	CTraceRtTimer *iTimer;
	CTraceRtSender *iSender;
	CTraceRtReceiver *iReceiver;
	TTraceRtOptions iOptions;
	RSocketServ iSocketServ;
	RSocket iIcmpSocket;
	RHostResolver iResolver;
	TNameEntry iNameEntry;
	TInetAddr iSrcAddr;
	TInetAddr iDstAddr;

	HTraceRtHeader *iRecvData;
	HTraceRtHeader *iSendData;

	TInt iProbeNr;
	TUint iTtl;
	TBool iGotThere;
	TInt iUnreachCount;
	TTime iSendTime;
	TUint iLastCode;

	RConnection iConnect;
	TBool iResolv;

	TInt iSeq;
	TInt iIdent;
	};

enum TTraceRtDefaultPriorites 
	{
	ETraceRtTimerPriority=10,
	ETraceRtSenderPriority=20,
	ETraceRtReceiverPriority=30,
	};
	
// ICMP Type and Code values
const TUint KTraceRtTypeEchoReply				= 0;
const TUint KTraceRtCodeEcho					= 0;
const TUint KTraceRtTypeEchoRequest				= 8;
const TUint KTraceRtTypeUnreachable				= 3;
const TUint	KTraceRtCodeUnreachNet				= 0;
const TUint	KTraceRtCodeUnreachHost				= 1;
const TUint	KTraceRtCodeUnreachProtocol			= 2;
const TUint	KTraceRtCodeUnreachPort				= 3;
const TUint	KTraceRtCodeUnreachNeedFrag			= 4;
const TUint	KTraceRtCodeUnreachSrcRouteFail		= 5;
const TUint	KTraceRtCodeUnreachNetUnknown		= 6;
const TUint	KTraceRtCodeUnreachHostUnknown		= 7;
const TUint	KTraceRtCodeUnreachSrcHostIsolated	= 8;	
const TUint	KTraceRtCodeUnreachNetProhibited	= 9;
const TUint	KTraceRtCodeUnreachHostProhibited	= 10;
const TUint	KTraceRtCodeUnreachNetTOS			= 11;
const TUint	KTraceRtCodeUnreachHostTOS			= 12;
const TUint	KTraceRtCodeUnreachProhibited		= 13;
const TUint	KTraceRtCodeUnreachPrecVolation		= 14;
const TUint	KTraceRtCodeUnreachPrecCutoff		= 15;
const TUint KTraceRtTypeTimeExceeded			= 11;
const TUint	KTraceRtCodeExceedInTransit			= 0;
const TUint	KTraceRtCodeExceedInReasm			= 1;
const TUint KTraceRtCodeTimeout					= KMaxTUint;
const TUint KTraceRtCodeTimedOutInTransit		= KMaxTUint-1;
const TUint KTraceRtCodeEchoReply				= KMaxTUint-2;

#endif

