// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains the declarations of the test suite classes used to test 
// the CSD Agent Connection/Connection Failure steps
// 
//


/**
 @file
 @internalTechnology
*/
#ifndef __CSDAGENTTESTSTEPS_H__
#define __CSDAGENTTESTSTEPS_H__

#include "CsdAgentTestSuite.h"
#include <networking/teststep.h>
#include "es_sock.h"


class CsdAgentTestStep : public CTestStep
	{
		public:
            CsdAgentTestStep();
            virtual ~CsdAgentTestStep();
		
		private:
            enum TVerdict doTestStepPreambleL();
		
		protected:
			TInt OpenConnection(RConnection& conn, RSocketServ& ss);
			void CloseConnection(RConnection& conn);
			TInt EnumerateConnections(RConnection& conn, TUint& num);
			TInt WaitForAllInterfacesToCloseL(RSocketServ& ss);
	};           


_LIT(KSrcPath, "z:\\testdata\\configs\\agentdialog.ini");
_LIT(KDestPath, "c:\\private\\101f7989\\esock\\agentdialog.ini");

class CTestStepCsdAgentConnection: public CsdAgentTestStep
	{
	public:
        CTestStepCsdAgentConnection(TPtrC aName);
        void ConstructL() {};
        virtual ~CTestStepCsdAgentConnection() {};
	
	// called by framework to do the test
       virtual enum TVerdict doTestStepL(void);
	
	private:
	};

class CTestStepCsdAgtConnectionFailure: public CsdAgentTestStep
    {
    public:
        CTestStepCsdAgtConnectionFailure(TPtrC aName);
        void ConstructL() {};
        virtual ~CTestStepCsdAgtConnectionFailure() {};
    
    // called by framework to do the test
       virtual enum TVerdict doTestStepL(void);
    
    private:
    };

#endif
