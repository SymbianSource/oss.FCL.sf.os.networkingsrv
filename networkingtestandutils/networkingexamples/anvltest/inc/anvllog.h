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


#if !defined(__ANVLLOG_H__)
#define __ANVLLOG_H__

#include <flogger.h>

#ifdef _DEBUG
#define ANVLLOG(a)      a
#else
#define ANVLLOG(a)
#endif

const TInt KAnvlLogHexDumpWidth = 16;

class AnvlLog
    {
public:
    static void Write(const TDesC& aDes);
    static void Printf(TRefByValue<const TDesC> aFmt, ...);
    static void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth = KAnvlLogHexDumpWidth);

private:

    };

#endif
