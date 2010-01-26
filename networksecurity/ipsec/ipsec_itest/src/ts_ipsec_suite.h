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
//

/**
 @file ts_ipsec_suite.h header file for IPsec test suite
*/

#ifndef TS_IPSEC_SUITE_H__
#define TS_IPSEC_SUITE_H__


#include <networking/testsuite.h>

#include "mlogger.h"

typedef struct
	{
	TInt iPolicyHandle;
	TInt iErrCode;
	}TCallbackArgs;
	
const TCallbackArgs KArgs = {0,0};

/**	
 *	Need an active scheduler, with methods 
 *	The test schedule has following unique properties
 *		- Manages test active objects with following properties
 *			1 The AOs have a unique integer ID, returned by ID, call
 *			2 The AOs have a state, returned by State.
 *
 **/
	

class CTestScheduler : public CActiveScheduler
	{
public:
	CTestScheduler() 
		{
		if ( (CTestScheduler*)this != Current() )
			CActiveScheduler::Install(this); 
		}
	
	static void Start() 
		{
		CActiveScheduler::Start();
		}

	static void Add(CActive* aAO)
		{
		// Maintain our own Q as we dont get access to the real one
		CActiveScheduler::Add(aAO);
		}

	void OnStarting()
		{
		// Notify everyone that the test step is starting
		}

	void OnStopping()
		{
		// Test step has finished, find out if we had errors
		}

	void Error(TInt /*aError*/) const
		{
		// The activeobject's runL left and deposited us here.
		// Find out if any of the AOs are still running,
		TInt newError;
		// Nesting here
		TBool ran = RunIfReady(newError, 0);
		if (!ran) {Stop();}
		}

	void SetResult(TCallbackArgs* aArgs)
		{
		iArgs.iErrCode -= aArgs->iErrCode;
		iArgs.iPolicyHandle = aArgs->iPolicyHandle;
		}

	TInt GetResult()
		{
		return iArgs.iErrCode;
		}
private:
	TCallbackArgs iArgs;
	};

class CTestSuiteIpsec : public CTestSuite //, public MLogger
	{
public:
	// from CTestSuite
	virtual void InitialiseL();

	virtual ~CTestSuiteIpsec();
	//
	void AddThisTestStepL(CTestStep* ptrTestStep);
	
	TInt static HandleCompletion(TAny* aArgs);

	//void Log( TRefByValue<const TDesC16> format, ... );

private:
	CTestScheduler* iScheduler;
	};

typedef CTestSuiteIpsec MLogger;

#endif // TS_IPSEC_SUITE_H__
