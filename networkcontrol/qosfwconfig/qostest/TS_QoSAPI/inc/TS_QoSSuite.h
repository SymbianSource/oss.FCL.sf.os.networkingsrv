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
// TestSuiteSelfTest.h
// This contains CTestSuiteSelfTest 
// 
//

#if (!defined __TEST_SUITE_SELF_TEST_H_)
#define __TEST_SUITE_SELF_TEST_H_

#include "TestSuite.h"
#include "TestStep.h"
#include <Es_sock.h>
#include <in_sock.h>
#include "qoslib.h"

class CMultipleArray;
class CTS_QoSStep;

class  CTS_QoSSuite : public CTestSuite
{

public:

	/*
	 * Suite Operations
	 */
	void InitialiseL( void );
	~CTS_QoSSuite();
	void AddTestStepL( CTS_QoSStep * ptrTestStep );
	TPtrC GetVersion( void );

	/*
	 * QoS Operations
	 */
	void CloseQoSChannelL(TInt aIndex);

	void RemoveRArraySocketCount();

	// An array of QoSChannels that stores RSockets / RConnections / Tcp and Udp Connection Details / QoS Prefrence Setting(s)
	RPointerArray<CMultipleArray> iQoSChannel;

	// Outcome of QoS Event 
	TQoSEvent qoSEventOutcome;				
	// True if QoS has been set else False
	RArray<TBool> iQoSSet;
	// True if Change QoS has been done else False
	RArray<TBool> iQoSChangeSet;
	// True if Join has been done else False
	RArray<TBool> iJoin;
    // True if Leave has been done else False
	RArray<TBool> iLeave;

	TInt beginQoSChannelCount;

	TInt nExtraJoinImplicitTcp;
	TInt nExtraJoinImplicitUdp;
	TInt nExtraJoinExplicitTcp;
	TInt nExtraJoinExplicitUdp;

	RArray<TInt> nExtraJoinImplicitTcpArray;
	RArray<TInt> nExtraJoinImplicitUdpArray;
	RArray<TInt> nExtraJoinExplicitTcpArray;
	RArray<TInt> nExtraJoinExplicitUdpArray;

	RSocketServ iSocketServer;						

private:

};

#endif /* __TEST_SUITE_SELF_TEST_H_ */
