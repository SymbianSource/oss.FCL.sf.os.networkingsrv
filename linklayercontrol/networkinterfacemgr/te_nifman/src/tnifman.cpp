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

/**
 @file tnifman.cpp
*/

#include "tnifman.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManTesttnfmn, "NifManTesttnfmn.");
#endif

RTestNif::RTestNif()
: iConnectionOpen(EFalse)
	{ }

RTestNif::~RTestNif()
	{
	__ASSERT_DEBUG(!iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 1));
	}

TInt RTestNif::Open(const TDesC& aName /* = TPtr(0,0) */)
	{
#ifdef _DEBUG

	__ASSERT_DEBUG(!iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 2));

	TInt err = iSocketServ.Connect();
	if(err!=KErrNone)
		{
		return err;
		}
	err = iConnection.Open(iSocketServ);
	if(err!=KErrNone)
		{
		iSocketServ.Close();
		return err;
		}

	(void)(aName);
	if(err!=KErrNone)
		{
		iConnection.Close();
		iSocketServ.Close();
		return err;
		}

	iConnectionOpen = ETrue;
	return KErrNone;
#else
	(void)aName;  // remove warning
	return KErrNotSupported;
#endif
	}

void RTestNif::Close()
	{
	if(iConnectionOpen)
		{
		iConnection.Close();
		iSocketServ.Close();
		}

	iConnectionOpen = EFalse;
	}

void RTestNif::Start(TInt aTestNo, TRequestStatus& aStatus)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 3));

	TInt err = SetInitialValue(aTestNo);

	if(err==KErrNone)
		{
		iConnection.Start(aStatus);
		}
	else
		{
		TRequestStatus* pS = (&aStatus);
		User::RequestComplete(pS, err);
		}
	}

TInt RTestNif::Start(TInt aTestNo)
	{

	TRequestStatus status;
	Start(aTestNo, status);
	User::WaitForRequest(status);
	return status.Int();
	}

TInt RTestNif::SetInitialValue(TInt aValue)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 4));

	TPckg<TInt> value(aValue);
	TInt err = iConnection.Control(KCOLAgent, KADummySetInitialValue, value);
	return err;
	}

TInt RTestNif::Stop()
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 5));

	iConnection.Stop();
	return KErrNone;
	}

TInt RTestNif::AgentInfo(TNifAgentInfo&)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 6));

	return KErrNotSupported; 
	}

TInt RTestNif::Progress(TNifProgress& aProg)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 7));

	return iConnection.Progress(aProg);
	}

void RTestNif::ProgressNotification(TNifProgressBuf& aProgBuf, TRequestStatus& aStatus, TUint aSelectedProgress /* = KConnProgressDefault */)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 8));

	iConnection.ProgressNotification(aProgBuf, aStatus, aSelectedProgress);
	}

void RTestNif::CancelProgressNotification()
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 9));

	iConnection.CancelProgressNotification();
	}

TInt RTestNif::LastProgressError(TNifProgress& aError)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 10));

	return iConnection.LastProgressError(aError);
	}

TInt RTestNif::NetworkActive(TBool&)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 11));

	return KErrNotSupported;
	}

TInt RTestNif::DisableTimers(TBool aDisable /* = ETrue */)
	{

	__ASSERT_DEBUG(iConnectionOpen, User::Panic(KSpecAssert_NifManTesttnfmn, 12));

	TPckg<TBool> disable(aDisable);
	return iConnection.Control(KCOLProvider, KConnDisableTimers, disable);
	}


