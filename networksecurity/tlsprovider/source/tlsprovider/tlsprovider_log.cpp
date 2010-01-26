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
// tlsprovider.cpp
// 
//


#include "tlsprovider_log.h"

#ifdef _DEBUG

void TLSProvLog::CreateLog(const TDesC& aFile, const TDesC& aDes)
	{
	RFileLogger::Write(KTlsProvider(), aFile, EFileLoggingModeAppend, aDes);
	}

void TLSProvLog::Printf(const TDesC& aFile, TRefByValue<const TDesC> aFmt,...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KTlsProvider(), aFile, EFileLoggingModeAppend, aFmt, list);
	}


void TLSProvLog::HexDump(
						 const TDesC& aFile,
						const TUint8* aInput,
						TInt aLen)
	{
	
	_LIT( KTlsHdr, "HexDump" );
	_LIT( KTlsContnd, "contnd:" );
	
	RFileLogger::HexDump(KTlsProvider(), 
						aFile, 
						EFileLoggingModeAppend, 
						KTlsHdr().Ptr(), 
						KTlsContnd().Ptr(), 
						aInput,
						aLen);
	
	}

#endif
