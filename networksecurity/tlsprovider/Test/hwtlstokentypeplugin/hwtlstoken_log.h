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


#ifndef __HWTLSTOKEN_LOG_H__
#define __HWTLSTOKEN_LOG_H__

#ifdef _DEBUG

#include <flogger.h>
_LIT( KHwTLSTokenComponent, "hwtlstoken");
_LIT( KHwTLSTokenLog, "hwtlstoken.log");


class HwTLSTokenLog
	{
public:
	static void CreateLog(const TDesC& aFile, const TDesC& aDes);
	static void Printf(const TDesC& aFile, TRefByValue<const TDesC> aFmt, ...);
	static void HexDump( const TDesC& aFile,
						const TUint8* aInput,
						TInt aLen);
	};

#define LOG(AAA) { AAA }

#define CREATE_HWTLSTOKEN_LOG(AAA)      HwTLSTokenLog::CreateLog(KHwTLSTokenLog(), AAA);
#define HWTLSTOKEN_LOG(AAA)             HwTLSTokenLog::Printf(KHwTLSTokenLog(), AAA);
#define HWTLSTOKEN_LOG2(AAA, BBB)       HwTLSTokenLog::Printf(KHwTLSTokenLog(), AAA, BBB);

#define HWTLSTOKEN_LOG_HEX(AAA, BBB)         HwTLSTokenLog::HexDump(KHwTLSTokenLog(), AAA, BBB);

#else
#define LOG(AAA)

#define CREATE_HWTLSTOKEN_LOG(AAA)	
#define HWTLSTOKEN_LOG(AAA)
#define HWTLSTOKEN_LOG2(AAA, BBB)

#define HWTLSTOKEN_LOG_HEX(AAA, BBB)

#endif

#endif
