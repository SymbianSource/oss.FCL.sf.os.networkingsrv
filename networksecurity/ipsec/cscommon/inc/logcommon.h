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
//



/**
 @internalComponent
*/

#if !defined(__LOGCOMMON_H__)
#define __LOGCOMMON_H__

#if !defined(__FLOG_ACTIVE)
#  define LOG(a)
#else 
#  define LOG(a) a 
#  define _LOG

__FLOG_STMT(_LIT8(KLogSubSys, "Ipsec");) // subsystem name

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
	__FLOG_DECLARATION_VARIABLE;
	__FLOG_OPEN(KLogSubSys,KLogComponent);
	__FLOG_VA((aDes));
	__FLOG_CLOSE;
}

inline void Log::Printf(TRefByValue<const TDesC> aFmt, ...)
{
__FLOG_DECLARATION_VARIABLE;
__FLOG_OPEN(KLogSubSys,KLogComponent);
__FLOG_VA((aFmt));
__FLOG_CLOSE;
} 

inline void Log::Printf(TRefByValue<const TDesC8> aFmt, ...)
{
__FLOG_DECLARATION_VARIABLE;
__FLOG_OPEN(KLogSubSys,KLogComponent);
__FLOG_VA((aFmt));
__FLOG_CLOSE;
}

inline void Log::HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen)
{
__FLOG_DECLARATION_VARIABLE;
__FLOG_OPEN(KLogSubSys,KLogComponent);
__FLOG_HEXDUMP((aHeader,aMargin,aPtr,aLen));
__FLOG_CLOSE;
}

#endif // !defined(_FLOG_ACTIVE) || EPOC_SDK < 0x06000000

#endif //__LOGCOMMON_H__
