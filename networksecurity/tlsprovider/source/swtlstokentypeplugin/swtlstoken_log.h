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
 @internalTechnology
*/
 
#ifndef __SWTLSTOKEN_LOG_H__
#define __SWTLSTOKEN_LOG_H__

#ifdef _DEBUG

#include <flogger.h>
_LIT( KSwTLSTokenComponent, "swtlstoken");
_LIT( KSwTLSTokenLog, "swtlstoken.log");


class SwTLSTokenLog
	{
public:
	static void CreateLog(const TDesC& aFile, const TDesC& aDes);
	static void Printf(const TDesC& aFile, TRefByValue<const TDesC> aFmt, ...);
	static void HexDump( const TDesC& aFile,
						const TUint8* aInput,
						TInt aLen);
	};

#define LOG(AAA) { AAA }

#define CREATE_SWTLSTOKEN_LOG(AAA)      SwTLSTokenLog::CreateLog(KSwTLSTokenLog(), AAA);
#define SWTLSTOKEN_LOG(AAA)             SwTLSTokenLog::Printf(KSwTLSTokenLog(), AAA);
#define SWTLSTOKEN_LOG2(AAA, BBB)       SwTLSTokenLog::Printf(KSwTLSTokenLog(), AAA, BBB);

#define SWTLSTOKEN_LOG_HEX(AAA, BBB)         SwTLSTokenLog::HexDump(KSwTLSTokenLog(), AAA, BBB);

#else
#define LOG(AAA)

#define CREATE_SWTLSTOKEN_LOG(AAA)	
#define SWTLSTOKEN_LOG(AAA)
#define SWTLSTOKEN_LOG2(AAA, BBB)

#define SWTLSTOKEN_LOG_HEX(AAA, BBB)

#endif

#endif
