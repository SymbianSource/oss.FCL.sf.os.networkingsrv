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
 
#ifndef __TLSPROV_LOG_H__
#define __TLSPROV_LOG_H__

#ifdef _DEBUG

#include <flogger.h>
_LIT( KTlsProvider, "tlsprovider");
_LIT( KTlsProviderLog, "tlsprovider.log");


class TLSProvLog
	{
public:
	static void CreateLog(const TDesC& aFile, const TDesC& aDes);
	static void Printf(const TDesC& aFile, TRefByValue<const TDesC> aFmt, ...);
	static void HexDump( const TDesC& aFile,
						const TUint8* aInput,
						TInt aLen);
	};

#define LOG(AAA) { AAA }

#define CREATE_TLSPROV_LOG(AAA)      TLSProvLog::CreateLog(KTlsProviderLog(), AAA);
#define TLSPROV_LOG(AAA)             TLSProvLog::Printf(KTlsProviderLog(), AAA);
#define TLSPROV_LOG2(AAA, BBB)       TLSProvLog::Printf(KTlsProviderLog(), AAA, BBB);
#define TLSPROV_LOG3(AAA, BBB, CCC)  TLSProvLog::Printf(KTlsProviderLog(), AAA, BBB, CCC);

#define TLSPROV_LOG_HEX(AAA, BBB)         TLSProvLog::HexDump(KTlsProviderLog(), AAA, BBB);

#else
#define LOG(AAA)

#define CREATE_TLSPROV_LOG(AAA)	
#define TLSPROV_LOG(AAA)
#define TLSPROV_LOG2(AAA, BBB)
#define TLSPROV_LOG3(AAA, BBB, CCC)

#define TLSPROV_LOG_HEX(AAA, BBB)

#endif

#endif
