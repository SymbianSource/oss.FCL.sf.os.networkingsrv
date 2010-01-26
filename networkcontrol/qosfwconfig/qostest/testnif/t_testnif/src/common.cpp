// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// common .CPP
// Common utilities & classes definitions
// 
//

#include "common.h"
#include <comms-infras/commsdebugutility.h>

//the only way to call the proper WriteFormat is through smth like this
void Log(TRefByValue<const TDesC> aFmt, ...)
{
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KPppLogFolder,KPppLogFile,EFileLoggingModeAppend,aFmt,list);
}


_LIT(KChpFrmt1,"Check failed: %d ->expected %d (Msg:%S File:%S Line:%d)");

TBool 
Checkpoint(const TInt& aVal,const TInt& aExpectedVal,const char* aFileName,TUint aLine,const TDesC& aMsg)
{
	TBool isOK=(aVal==aExpectedVal);
	if (!isOK)
	{
		//convert to unicode
		TBuf<256> filename;
		filename.Copy(TPtrC8((TText8*)aFileName));
		
		 Log(KChpFrmt1,
								aVal,
								aExpectedVal,
								&aMsg,
								&filename,
								aLine);
		
	}
	return isOK;
}

_LIT(KChpFrmt2,"Checkpoint failed (Msg:%S File: %S Line: %d)");

TBool 
Checkpoint(TBool aExpr,const char* aFileName,TUint aLine,const TDesC& aMsg)
{
	if (!aExpr)
	{
		//convert to unicode
		TBuf<256> filename;
		filename.Copy(TPtrC8((TText8*)aFileName));
		Log(KChpFrmt2,&aMsg,&filename,aLine);
	}
	return aExpr;
}



