/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header for TS_DummyPPPSuite class. This is the container
* class for all the RConnection multihoming test steps
* 
*
*/



/**
 @file TS_DummyPPPSuite.h
*/

#if (!defined __TS_DUMMYPPPSUITE_H__)
#define __TS_DUMMYPPPSUITE_H__

#include <e32std.h>
#include <c32comm.h>

#include <es_sock.h>
#include <in_sock.h>
#include <networking/testsuite.h>


#define MAX_NAME_LENGTH		10			///< Maximum length of connection name

_LIT (KTxtVersion,"1.01");				///< Version string for ScheduleTest

_LIT (KConnLogString, "Name [%S], RConnection[%x], RSocketServ[%x], Clients: %i");


class TS_DummyPPPStep;

class TS_DummyPPPSuite : public CTestSuite
{
public:
	virtual		~TS_DummyPPPSuite();
	void		InitialiseL(void);
	void		AddTestStepL(TS_DummyPPPStep* ptrTestStep );
	TPtrC		GetVersion(void);	
	
private:
	// nuffin
};

void CommInitL(TBool);

#endif /* __TS_DUMMYPPPSUITE_H__ */
