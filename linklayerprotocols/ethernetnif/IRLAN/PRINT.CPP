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
// Started by Mal, September 1997
// Irlan printing routines 
// 
//

/**
 @file
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <ir_sock.h>
#include "PKTDRV.H"
#include "ETHINTER.H"
#include "IRLAN.H"
#include "IRLANUTL.H"

#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif

/**
Prints the states in the log file.
*/
void CIrlanControlEngine::PrintState()
{
#ifdef __TRACEWIN__
	switch(iState)
		{
	case E_Idle:LOG(Log::Printf(_L("IRLAN	****E_Idle****\r\n")));break;
	case E_PassiveIdle:LOG(Log::Printf(_L("IRLAN	****E_PassiveIdle****\r\n")));break;
	case E_Query:LOG(Log::Printf(_L("IRLAN	****E_Query****\r\n")));break;
	case E_Conn:LOG(Log::Printf(_L("IRLAN	****E_Conn****\r\n")));break;
	case E_Info:LOG(Log::Printf(_L("IRLAN	****E_Info****\r\n")));break;
	case E_Media:LOG(Log::Printf(_L("IRLAN	****E_Media****\r\n")));break;
	case E_FilterConfig:LOG(Log::Printf(_L("IRLAN	****E_FilterConfig****\r\n")));break;
	case E_FilterOperation:LOG(Log::Printf(_L("IRLAN	****E_FilterOperation****\r\n")));break;
	case E_FilterBroadcast:LOG(Log::Printf(_L("IRLAN	****E_FilterBroadcast****\r\n")));break;
	case E_Open:LOG(Log::Printf(_L("IRLAN	****E_Open****\r\n")));break;
	case E_Wait:LOG(Log::Printf(_L("IRLAN	****E_Wait****\r\n")));break;
	case E_Arb:LOG(Log::Printf(_L("IRLAN	****E_Arb****\r\n")));break;
	case E_Data:LOG(Log::Printf(_L("IRLAN	****E_Data****\r\n")));break;
	case E_Close:LOG(Log::Printf(_L("IRLAN	****E_Close****\r\n")));break;
	default:LOG(Log::Printf(_L("IRLAN	****Unknown****\r\n")));break;
		}
#endif
}

/**
Prints the IAS Results.
@param aError Contains the IAS Results code.
*/
void CIrlanControlEngine::PrintIASResults()
{
#ifdef __TRACEWIN__
	TBuf<64> res;
	LOG(Log::Printf(_L("IRLAN	!!!!!!!!!!!!! SUCCESSFUL IAS QUERY !!!!!!!!!!!!!!!\r\n")));
	switch(iResults.Type())
		{
	case EIASDataMissing:
		LOG(Log::Printf(_L("IRLAN	Missing\r\n")));
		break;
	case EIASDataInteger:
		TInt num;
		if (iResults.GetInteger(num)==KErrNone)
			LOG(Log::Printf(_L("IRLAN	Integer %d\r\n"),num));
		else
			LOG(Log::Printf(_L("IRLAN	Bad type\r\n")));
		break;
	case EIASDataOctetSequence:
		LOG(Log::Printf(_L("IRLAN	Byte sequence\r\n")));
		iResults.GetOctetSeq(res);
		LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),(TUint8 *)&res[0],res.Length()));
		break;
	case EIASDataUserString:
		{
		TPtrC8 str=iResults.GetCharString8();
		LOG(Log::Printf(_L("IRLAN	String \"%S\"\r\n"),&str));
		break;
		}
	default:
		LOG(Log::Printf(_L("IRLAN	Bad Type\r\n")));
		break;
		}
#endif
}

#pragma warning (disable:4100) 
/**
Prints the IAS Error codes.
@param aError Contains the IAS Error code.
*/
void CIrlanControlEngine::PrintIASError(TInt /*aError*/)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	!!!!!!!!!!!!! FAILED IAS QUERY !!!!!!!!!!!!!!!\r\n")));
	switch (aError)
		{
	case KErrBadHandle:
		LOG(Log::Printf(_L("IRLAN	No such attribute\n")));
		break;
	case KErrBadName:
		LOG(Log::Printf(_L("IRLAN	No such class\n")));
		break;
	case KErrTimedOut:
		LOG(Log::Printf(_L("IRLAN	Stack timed out on query\n")));
		break;
	default:
		LOG(Log::Printf(_L("IRLAN	Unknown error %d\n"),aError));
		break;
		}
#endif
}
#pragma warning (default:4100)

/**
Prints the result codes.
@param aResult Contains the Result code.
*/
void CIrlanControlEngine::PrintResultCode(TIrlanQueryResult aResult)
{
	switch (aResult)
		{
	case EIrlanSuccess:
		LOG(Log::Printf(_L("IRLAN	SUCCESS!! on response\r\n")));
		break;
	case EIrlanInvalidFormat:
		LOG(Log::Printf(_L("IRLAN	FAILED!! Invalid Command Format\r\n")));
		break;
	default:
		LOG(Log::Printf(_L("IRLAN	Unknown result code\r\n")));
		break;
		}
}
