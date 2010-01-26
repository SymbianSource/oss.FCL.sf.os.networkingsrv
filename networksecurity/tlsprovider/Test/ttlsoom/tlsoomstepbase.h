/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file
*/

#ifndef __TLSOOMSTEPBASE_H__
#define __TLSOOMSTEPBASE_H__

#include <e32base.h>
#include <TestExecuteStepBase.h>

#include "tlsprovinterface.h"

/*
 * This is an active object to assist with async
 * tlsprovider calls, these are handled syncronously
 * using CActiveScheduler::Start() and Stop() calls
 */

class CTlsOOMStepBase : public CActive
	{	

public:
	TVerdict Start();
	
protected:
	CTlsOOMStepBase(CTestExecuteLogger& aLogger, const TDesC& aConfigPath);
	void RunL();	
	void DoCancel();
	virtual void DoTestStepL() = 0;
	
	void InitTlsProviderLC(CTLSProvider*& aTlsProvider, CTlsCryptoAttributes*& aCryptoAttributes,
		HBufC8*& aServerCert);
	
private:
	inline CTestExecuteLogger& Logger();
	void ReadFileDataL(TDes8& aDest, const TDesC& aFileName, RFs& aFileServer);
	
private:

	enum  
		{
		KMaxOOMSteps = 500	
		};
	
	TInt iCurrentStep;
	CTestExecuteLogger& iLogger;
	const TDesC& iConfigPath;
	
	};
	
inline CTestExecuteLogger& CTlsOOMStepBase::Logger()
	{
	return iLogger;
	}

#endif /* __TLSOOMSTEPBASE_H__ */
