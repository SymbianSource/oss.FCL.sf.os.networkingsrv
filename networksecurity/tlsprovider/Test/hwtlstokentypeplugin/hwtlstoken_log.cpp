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


#include "hwtlstoken_log.h"

#ifdef _DEBUG

void HwTLSTokenLog::CreateLog(const TDesC& aFile, const TDesC& aDes)
	{
	RFileLogger::Write(KHwTLSTokenComponent(), aFile, EFileLoggingModeAppend, aDes);
	}

void HwTLSTokenLog::Printf(const TDesC& aFile, TRefByValue<const TDesC> aFmt,...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KHwTLSTokenComponent(), aFile, EFileLoggingModeAppend, aFmt, list);
	}


void HwTLSTokenLog::HexDump(
						 const TDesC& aFile,
						const TUint8* aInput,
						TInt aLen)
	{
	
	_LIT( KTlsHdr, "HexDump" );
	_LIT( KTlsContnd, "contnd:" );
	
	RFileLogger::HexDump(KHwTLSTokenComponent(), 
						aFile, 
						EFileLoggingModeAppend, 
						KTlsHdr().Ptr(), 
						KTlsContnd().Ptr(), 
						aInput,
						aLen);
	}

#endif
