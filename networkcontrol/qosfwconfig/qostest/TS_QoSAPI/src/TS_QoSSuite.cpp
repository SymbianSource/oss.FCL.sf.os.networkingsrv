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
// TestSuiteSelfTest.cpp
// This main DLL entry point for the TS_SelfTest.dll
// 
//

// Test system includes
#include "TS_QoSStep.h"
#include "TS_QoSSuite.h"
#include "TS_QoSSocketSection.h"
#include "TS_QoSSocketServer.h"
#include "TS_QoSOperationSection.h"
#include <qoslib.h>
#include <c32comm.h>

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif



/* NewTestEtelPacket is exported at ordinal 1
 * this provides the interface to allow schedule test
 * to create instances of this test suite
 */
EXPORT_C CTS_QoSSuite* NewTestSuiteSelfTestL() 
    { 
	return new (ELeave) CTS_QoSSuite;
    }

/*
 *	Destructor
 */
CTS_QoSSuite::~CTS_QoSSuite()
	{
	TInt i;

	// if any QoS Channels have been left open then close them
	if (iQoSChannel.Count() > 0)
		{
		// Close any Open QoS Channels
		Log(_L("Closing <%d> QoS Channel(s)"), iQoSChannel.Count());
		for (i = iQoSChannel.Count(); i > 0; i--)
			{
			Log(_L("Close QoS Channel <%d>"), i);
			CloseQoSChannelL(i);
			}
		}

	// Delete QoS Channel Refrence
	iQoSChannel.ResetAndDestroy();
	// Close RArray object
	iQoSSet.Close();
	iQoSChangeSet.Close();
	iJoin.Close();
	iLeave.Close();

	nExtraJoinImplicitTcpArray.Close();
	nExtraJoinImplicitUdpArray.Close();
	nExtraJoinExplicitTcpArray.Close();
	nExtraJoinExplicitUdpArray.Close();
	}

/* make a version string available for test system 
 */
_LIT(KTxtVersion,"1.0");
TPtrC CTS_QoSSuite::GetVersion( void )
	{
	return KTxtVersion();
	}

/* Add a test step into the suite
 */
void CTS_QoSSuite::AddTestStepL( CTS_QoSStep * aPtrTestStep )
	{
	// test steps contain a pointer back to the suite which owns them
	aPtrTestStep->iQoSSuite = this; 

	// add the step using tyhe base class method
	CTestSuite::AddTestStepL(aPtrTestStep);
	}

/*  Constructor
 */
void CTS_QoSSuite::InitialiseL( void )
	{
	// Load the serial drivers
	TInt ret = User::LoadPhysicalDevice(PDD_NAME);
	if ( KErrNone != ret && KErrAlreadyExists != ret )
		{
		User::Leave( ret );
		}
		
	ret = User::LoadLogicalDevice(LDD_NAME);
	if ( KErrNone != ret && KErrAlreadyExists != ret )
		{
		User::Leave( ret );
		}

 	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    ret = StartC32WithCMISuppressions(KPhbkSyncCMI);
	if ( KErrNone != ret && KErrAlreadyExists != ret )
		{
		User::Leave( ret );
		}

	// Socket Server Operations
	AddTestStepL( new(ELeave) CTS_QoSOpenServer );
	AddTestStepL( new(ELeave) CTS_QoSCloseServer );

    // Socket Operations
	AddTestStepL( new(ELeave) CTS_QoSSocketSection1_3 );
	AddTestStepL( new(ELeave) CTS_CEsockSendAndRecvData );

	// QoS Operations
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_0 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_1 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_2 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_3 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_4 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_5 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_6 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_7 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_8 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_9 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_10 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_11 );
	AddTestStepL( new(ELeave) CTS_QoSOperationSection2_12 );

	// Install Active Scheduler
	CActiveScheduler* self = new(ELeave) CActiveScheduler;
	// Installs the specified active scheduler as the current active scheduler. 
	// The installed active scheduler now handles events for this thread.
	CActiveScheduler::Install(self);
	}


/******************
 * QoS Operations *
 ******************/

void CTS_QoSSuite::CloseQoSChannelL(TInt aIndex)
	{
/* Close QoS Channel
 */
	TInt err = iQoSChannel[aIndex-1]->iQoSChannel->Close();
	
	if (err!=KErrNone)
		{
		Log(_L("Failed to close QoS channel: return value = <%d>"), err);
		User::Leave(err);
		}

	// Remove pointer
	iQoSChannel.Remove(aIndex - 1);
	}

void CTS_QoSSuite::RemoveRArraySocketCount()
{
	TInt i;

	for (i = 0; i < nExtraJoinImplicitTcpArray.Count(); i++)
		nExtraJoinImplicitTcpArray.Remove(i);

	for (i = 0; i < nExtraJoinExplicitTcpArray.Count(); i++)
		nExtraJoinExplicitTcpArray.Remove(i);

	for (i = 0; i < nExtraJoinImplicitUdpArray.Count(); i++)
		nExtraJoinImplicitUdpArray.Remove(i);

	for (i = 0; i < nExtraJoinExplicitUdpArray.Count(); i++)
		nExtraJoinExplicitUdpArray.Remove(i);
}
	

