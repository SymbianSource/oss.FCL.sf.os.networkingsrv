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

#ifndef __MLOGGER_H__
#define __MLOGGER_H__

#include <e32std.h>

#include "ts_ipsec_suite.h"

_LIT(KNewData, "%S");

class CLogger : public CBase
	{
private:
	CTestSuite& iLogger;
	TBuf<200> iBuffer;

public:
	CLogger(CTestSuite& aLogger) :iLogger(aLogger) { }

	void Log( TRefByValue<const TDesC16> aFmt, ... )
		{
		VA_LIST list;
		VA_START(list,aFmt);

		iBuffer.FormatList(aFmt, list);
		iLogger.Log(KNewData, &iBuffer);
		}

	~CLogger() { }
	};

class TestLog16Overflow :public TDes16Overflow
/**	
@internalComponent
*/
	{
public:
	// TDes16Overflow pure virtual
	virtual void Overflow(TDes16& /*aDes*/) { User::Panic(_L("Log output buffer overflow"),1); }
	};

#endif
