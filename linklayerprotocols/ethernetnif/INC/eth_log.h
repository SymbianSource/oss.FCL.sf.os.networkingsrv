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

/**
 @file
 @internalComponent 
*/

// Ethernet logging 
#ifndef __ETH_LOG_H__
#define __ETH_LOG_H__

#include <comms-infras/commsdebugutility.h>
#include <es_mbuf.h>
#include <comms-infras/nifprvar.h>
#include "EthProto.h"

#ifdef __FLOG_ACTIVE
#define TCPDUMP_LOGGING // log all ethernet frame in pcap format
#endif // __FLOG_ACTIVE

#if defined __FLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
_LIT8(KEther802LogTag1,"ethernet");
_LIT8(KEthLogTag2,"ethint");
_LIT8(KEthLogTag3,"etherpkt");

_LIT8(KEthTcpDumpFirstTag,"TcpDump");
_LIT8(KEthTcpDump, "EthTcpDp.log");

_LIT(KUnixTimeBaseDes, "19700101:");
#endif

/**	
Non-static class for use with connected flogger sessions.
Needed for logging of binary data
@internalComponent
@note There *might* be problems with this logging if IEEE 802.3 frames are received, but it should be OK
*/
__FLOG_STMT(	// only define this logging class if floggerv2 is in use
NONSHARABLE_CLASS(CEthLog) : public CBase
{
public:
	static CEthLog* NewL();
	~CEthLog();
	void DumpTcpDumpFileHeader();
	void DumpFrame(TTime timeStep, RMBufChain& aBuffer);

private:
	CEthLog();
	void ConstructL();

private:
	TTime iUnixTimeBase; // no global static in dlls, so have to stick it here since TTime needs its constructor calling
	TBuf8<KMaxTagLength> iEthTcpDumpLogFileName;
	__FLOG_DECLARATION_MEMBER;
};
) // end of __FLOG_STMT

#endif // __ETH_LOG_H__
