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

#if !defined(__PPPLOG_H__)
#define __PPPLOG_H__

#include <comms-infras/commsdebugutility.h>
#include <comms-infras/ss_protflow.h>
#include "PPPSOCK.H"
#include "PPPDEF.H"

// ss_log.h is included
#if !defined(LOG)

#if defined (__FLOG_ACTIVE)

#define ESOCK_LOGGING_ACTIVE
#define LOG(a)		a
#else
#define LOG(a)
#endif

#endif

const TInt KMaxTcpDumpLogFileName = 58;

/** Packet frame file format */
enum {EpppDumpLogFormat,ElastLogFormat};

/** Format of packets to log in the packet frame file */
enum {EpppLogLinkFormat,ErawIpLogLinkFormat,ElastLogLinkFormat};

/** Data transfer direction (defined by pppdump log spec) */
enum {EpppDirectionSend=1,EpppDirectionReceive=2};

/**
PPP debug logging class.
Manages textual output and binary packet dump files.
@internalTechnology
*/
NONSHARABLE_CLASS(CPppLog) : public CBase
	{
public:
	static const TInt KHexDumpWidth = 16;

	static CPppLog* NewL();
	virtual ~CPppLog();
	void SetLogFileNameL(const TDesC& aName);
	
private:
	CPppLog();
	void ConstructL();
	
private:
	HBufC* iLogFileName;
	TBuf8<KMaxTcpDumpLogFileName> iTcpDumpLogFileName;
	__FLOG_DECLARATION_MEMBER;
	
        /** ETrue once the file header has been written to the packet dump file */
	TBool iLogHeaderDone;
	
	/** Packet dump file format to use */
	TInt iLogFormat;
	
	/** What should be logged in the packet dump file */
	TInt iLogType;
	
	/** Time of last packet logged */
	TTime iTimeOrigin;

public:
	virtual void Write(const TDesC& aDes);
	virtual void Printf(const TRefByValue<const TDesC> aFmt, ...);
	void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth = KHexDumpWidth);
//
	void Dump(RMBufChain& aPacket, TInt aChannel);
	void DumpPhase(TPppPhase aOldPhase, TPppPhase aNewPhase);
	void DumpState(const TText* aFsm, TPppFsmState aOldState, TPppFsmState aNewState);
    void DumpFrame(TInt direction, const TDesC8& aBuffer);
    void DumpFrameFileHeader(TInt aLogFormat, TInt aLogType);
    
private:
	TInt DumpLcp(TPtrC8& aDes);
	TInt DumpPap(TPtrC8& aDes);
	TInt DumpChap(TPtrC8& aDes);
	TInt DumpCbcp(TPtrC8& aDes);
	TInt DumpLcpOption(TPtrC8& aDes);
	TInt DumpIpcp(TPtrC8& aDes);
	TInt DumpIp6cp(TPtrC8& aDes);
	TInt DumpIpcpOption(TPtrC8& aDes);
	TInt DumpIp6cpOption(TPtrC8& aDes);
	void DumpChapType(const TUint8* aPtr);
	TInt DumpCcp(TPtrC8& aDes);
	TInt DumpCcpOption(TPtrC8& aDes);
	void DumpBytes(const TText* aMargin, const TUint8* aPtr, TInt aLen);
	TInt DumpIp(TPtrC8& aDes);
	TInt DumpIp6(TPtrC8& aDes);
	TInt DumpIp6MainHeader(TPtrC8& aDes);
	TInt DumpVjUncompTcp(TPtrC8& aDes);
	TInt DumpVjCompTcp(TPtrC8& aDes);
	TUint16 DecodeDelta(TUint8 * & aPtr);
	TInt16 DecodeSignedDelta(TUint8 * & aPtr);
	TInt DumpTcp(TPtrC8& aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength);
	TInt DumpIcmp(TPtrC8& aDes, TInt aLength);
	TInt DumpUdp(TPtrC8& aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength);
	TInt DumpDns(TPtrC8& aDes);
//
	TInt DumpDnsNameAndType(TPtrC8& aDes);
	TInt DumpDnsQuery(TPtrC8& aDes);
	TInt DumpDnsAnswer(TPtrC8& aDes);
	TInt UpdatePtr(TPtrC8& aDes, TUint8* ptr);

	const TText* ProtocolToText(TUint aValue);
	const TText* PhaseToText(TPppPhase aPhase);
	const TText* StateToText(TPppFsmState aState);
	const TText* LcpCodeToText(TUint aValue);
	const TText* LcpOptToText(TUint aValue);
	const TText* FsmCodeToText(TUint aValue);
	const TText* CallbackOpToText(TUint aValue);
	const TText* PapCodeToText(TUint aValue);
	const TText* ChapCodeToText(TUint aValue);
	const TText* CbcpCodeToText(TUint aValue);
	const TText* IpcpCodeToText(TUint aValue);
	const TText* Ip6cpCodeToText(TUint aValue);
	const TText* CcpCodeToText(TUint aValue);
	const TText* IpProtocolToText(TUint aValue);

	void PppDumpFileHeader();
	void PppDumpFrame(TInt direction, TUint32 timeStep, const TDesC8& aBuffer);
	};

#endif
