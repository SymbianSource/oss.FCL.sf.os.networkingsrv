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
// Started by MWT, June 1997
// 
//

#if !defined (__PINGENG_H__)
#define __PINGENG_H__

#if !defined(__IN_SOCK_H__)
#include <in_sock.h>
#endif

class MPingNotificationHandler
	{
public:
	virtual void Pinging(const TNameRecord& aRecord, TInt aBytes) const=0;
	virtual void Sent() const=0;
	virtual void Reply(const TInetAddr& aFrom, TInt aBytes, TInt aSeq, TTimeIntervalMicroSeconds32 aTime) const=0;
	virtual void Icmp4Message(const TInetAddr& aFrom, TInt aType, TInt aCode, const TDesC8& aRestOfIt) const=0;
	virtual void Icmp6Message(const TInetAddr& aFrom, TInt aType, TInt aCode) const=0;
	virtual void Finished(const TNameRecord& aRecord, TInt aNrTransmitted, TInt aNrReceived, TInt aNrDuplicates, TInt aMin, TInt aMax, TInt aSum, TInt aError) =0;
	};

class TPingOptions
	{
public:
	IMPORT_C TPingOptions();
public:
	TInt iNumberOfPings;
	TTimeIntervalMicroSeconds32 iInterval;
	TTimeIntervalMicroSeconds32 iWait;
	THostName iDestname;
	TInt iPingSize;
	TBool iResolveAddress;
	TInt iPreload;
	TInt iBacklog;
	TBool iPrompt;
	TInt iConnIap;
	TInt iConnSnap;
	};

class CPingEng;
class TPingRecord
	{
friend class CPingEng;

public:
	TPingRecord(TUint aSeqNr);
private:
	TTime iSendTime;
	TSglQueLink iLink;
	TInt iSeqNr;
	};

class CPingTimer;
class CPingSender;
class CPingReceiver;
class HPingHeader;


class CPingEng : public CBase
	{
public:
	IMPORT_C static CPingEng* NewL(MPingNotificationHandler& aUi);
	IMPORT_C ~CPingEng();

	IMPORT_C void SetPriorities(TInt aTimerPriority, TInt aSenderPriority, TInt aReceiverPriority);
	IMPORT_C void StartL(const TPingOptions& aOptions);
	IMPORT_C void Cancel();
	IMPORT_C void CancelAndFinished();

	void TimerComplete();
	void SendCompleteL();
	void SendDoCancel();
	void RecvComplete();
	void RecvDoCancel();

private:
	void ConstructL();
	CPingEng(MPingNotificationHandler& aUi);

	void NextSend();
	void DoError(TInt aError);
	void PurgeQue();
	void EmptyPingRecordQue();
	void DeletePingRecord(TPingRecord* aRecord);
	enum TState { EStopped, ELookingUp, ESending };

	TState iState;
	MPingNotificationHandler& iUi;
	CPingTimer* iTimer;
	CPingSender* iSender;
	CPingReceiver* iReceiver;
	TPingOptions iOptions;
	RSocketServ iSocketServ;
	RSocket iSocket;
	RHostResolver iResolver;
	TNameEntry iNameEntry;
	TInetAddr iSrcAddr;
	
	TInt iNrTransmitted;
	TInt iNrReceived;
	TInt iNrDuplicates;
	TInt iMinTime;
	TInt iMaxTime;
	TInt iSumTime;

	TInt iSendFlags;

	TSglQue<TPingRecord> iQue;
	TInt iNoInQue;
	
	RConnection iConnect;
	HPingHeader* iRecvData;
	HPingHeader* iSendData;

	TInt iId;

    TBool iIsLoopback;
	};

enum TPingDefaultPriorites 
	{
	EPingTimerPriority=10,
	EPingSenderPriority=20,
	EPingReceiverPriority=30,
	};
	
// ICMP Type and Code values
const TUint KIPv4PingTypeEchoReply				= 0;
const TUint KIPv4PingCodeEcho					= 0;
const TUint KIPv4PingTypeEchoRequest			= 8;
const TUint KIPv4PingTypeUnreachable			= 3;
const TUint	KIPv4PingCodeUnreachNet				= 0;
const TUint	KIPv4PingCodeUnreachHost			= 1;
const TUint	KIPv4PingCodeUnreachProtocol		= 2;
const TUint	KIPv4PingCodeUnreachPort			= 3;
const TUint	KIPv4PingCodeUnreachNeedFrag		= 4;
const TUint	KIPv4PingCodeUnreachSrcRouteFail	= 5;
const TUint	KIPv4PingCodeUnreachNetUnknown		= 6;
const TUint	KIPv4PingCodeUnreachHostUnknown		= 7;
const TUint	KIPv4PingCodeUnreachSrcHostIsolated	= 8;	
const TUint	KIPv4PingCodeUnreachNetProhibited	= 9;
const TUint	KIPv4PingCodeUnreachHostProhibited	= 10;
const TUint	KIPv4PingCodeUnreachNetTOS			= 11;
const TUint	KIPv4PingCodeUnreachHostTOS			= 12;
const TUint	KIPv4PingCodeUnreachProhibited		= 13;
const TUint	KIPv4PingCodeUnreachPrecVolation	= 14;
const TUint	KIPv4PingCodeUnreachPrecCutoff		= 15;
const TUint KIPv4PingTypeSourceQuench			= 4;
const TUint KIPv4PingTypeRedirect				= 5;
const TUint	KIPv4PingCodeRedirectNet			= 0;
const TUint	KIPv4PingCodeRedirectHost			= 1;
const TUint	KIPv4PingCodeRedirectNetTOS			= 2;
const TUint	KIPv4PingCodeRedirectHostTOS		= 3;
const TUint KIPv4PingTypeTimeExceeded			= 11;
const TUint	KIPv4PingCodeExceedInTransit		= 0;
const TUint	KIPv4PingCodeExceedInReasm			= 1;
const TUint KIPv4PingTypeBadParameter			= 12;

const TUint KIPv6PingTypeEchoReply				= 129;
const TUint KIPv6PingTypeEchoRequest			= 128;
const TUint KIPv6PingCodeEcho					= 0;
const TUint KIPv6PingTypeUnreachable			= 1;
const TUint KIPv6PingCodeNoRoute				= 0;
const TUint KIPv6PingCodeAdminProhibited		= 1;
const TUint KIPv6PingCodeAddressUnreachable		= 3;
const TUint KIPv6PingCodePortUnreachable		= 4;
const TUint KIPv6PingTypePacketTooBig			= 2;
const TUint KIPv6PingCodePacketTooBig			= 0;
const TUint KIPv6PingTypeTimeExeeded			= 3;
const TUint KIPv6PingCodeHLExeeded				= 0;
const TUint KIPv6PingCodeFragReassemblyExeeded	= 1;
const TUint KIPv6PingTypeParamProblem			= 4;
const TUint KIPv6PingCodeErroneousHeader		= 0;
const TUint KIPv6PingCodeNextHeaderUnrecognised	= 1;
const TUint KIPv6PingCodeIPv6OptionUnrecognised = 2;

#endif

