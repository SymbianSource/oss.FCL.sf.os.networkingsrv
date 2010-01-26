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

#include <comms-infras/commsdebugutility.h>

#include <e32test.h>

__FLOG_STMT(_LIT8(KSubsys, "Test");)
__FLOG_STMT(_LIT8(KComponent, "test.txt");)
	
_LIT(KNewData, "%S\n");

class CLogger : public CBase
	{
private:
	__FLOG_DECLARATION_MEMBER;

	RTest& iLogger;
	TBuf<200> iBuffer;

public:
	CLogger(RTest& aLogger) :iLogger(aLogger)
		{ __FLOG_OPEN(KSubsys, KComponent); }

	void Log( TRefByValue<const TDesC16> aFmt, ... )
		{
		VA_LIST list;
		VA_START(list,aFmt);

		iBuffer.FormatList(aFmt, list);
		iLogger.Printf(KNewData, &iBuffer);

		__FLOG_1(aFmt, list);
		}

	~CLogger()
		{ __FLOG_CLOSE; }
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
