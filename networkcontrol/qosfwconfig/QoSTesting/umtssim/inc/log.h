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

#if !defined(__LOG_H__)
#define __LOG_H__

#include <e32std.h>
#include <e32base.h>

#if defined(_MKLOG)
# define _LOG
#endif

#if defined(_LOG)

# if defined(__WINS__) || defined(CRYSTAL)
#  define LOG(a) a 
# elif defined (__EPOC32__)
#  define LOG(a) a 
# endif
#else
# define LOG(a)
# undef _LOG
#endif  // _LOG

#ifdef _LOG

#include <flogger.h>

_LIT(KUMTSSimLogFolder,"UMTSSim");
_LIT(KUMTSSimLogFile,"umtssim.txt");

class Log
{
public:
	static inline void Write(const TDesC& aDes);
	static inline void Printf(TRefByValue<const TDesC> aFmt, ...);
	static inline void Printf(TRefByValue<const TDesC8> aFmt, ...);
	static inline void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen);
};

inline void Log::Write(const TDesC& aDes)
{
	RFileLogger::Write(KUMTSSimLogFolder(), KUMTSSimLogFile(), EFileLoggingModeAppend, aDes);
}

inline void Log::Printf(TRefByValue<const TDesC> aFmt, ...)
{
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KUMTSSimLogFolder(), KUMTSSimLogFile(), EFileLoggingModeAppend, aFmt, list);
}

inline void Log::Printf(TRefByValue<const TDesC8> aFmt, ...)
{
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KUMTSSimLogFolder(), KUMTSSimLogFile(), EFileLoggingModeAppend, aFmt, list);
}

inline void Log::HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen)
{
	RFileLogger::HexDump(KUMTSSimLogFolder(), KUMTSSimLogFile(), EFileLoggingModeAppend, aHeader, aMargin, aPtr, aLen);
}

#endif

#endif
