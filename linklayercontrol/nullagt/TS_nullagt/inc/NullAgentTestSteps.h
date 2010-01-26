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
// Contains declarations of the test step classes for the Null Agent
// test suite.
// 
//

#ifndef __NULLAGENTTESTSTEPS_H__
#define __NULLAGENTTESTSTEPS_H__

#include "TestSuiteNullAgent.h"
#include <networking/teststep.h>
#include "es_sock.h"


class NullAgentTestStep : public CTestStep
{
public:
	NullAgentTestStep();
	virtual ~NullAgentTestStep();

private:
	enum TVerdict doTestStepPreambleL();
	
protected:					
	
	// to all of these methods pass in a pointer to objects you have created on the stack...
	// lots of them will also create temporary automatics but don't worry about that


	// connection specific stuff
	TInt OpenConnection(RConnection& conn, RSocketServ& ss);
	void CloseConnection(RConnection& conn);
	TInt EnumerateConnections(RConnection& conn, TUint& num);
	TInt WaitForAllInterfacesToCloseL(RSocketServ& ss);

};

//
// the classes which follow are the steps themselves...
//



_LIT(KSrcPath, "z:\\testdata\\configs\\agentdialog.ini");
_LIT(KDestPath, "c:\\private\\101f7989\\esock\\agentdialog.ini");


//
// Test Copy Test
NONSHARABLE_CLASS (CNullAgentPreCopy) : public NullAgentTestStep
{
public:
	CNullAgentPreCopy();
	virtual ~CNullAgentPreCopy();

	virtual enum TVerdict doTestStepL( void );
	void copyFileL (const TDesC& anOld,const TDesC& aNew);
};

//
// Test Post Delete
NONSHARABLE_CLASS (CNullAgentPostDelete) : public NullAgentTestStep
{
public:
	CNullAgentPostDelete();
	virtual ~CNullAgentPostDelete();

	virtual enum TVerdict doTestStepL( void );
	void deleteFileL (const TDesC& aFileName);
};


class CTestStepNullAgtSimpleConnection : public NullAgentTestStep
{
public:
	CTestStepNullAgtSimpleConnection(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtSimpleConnection() {};
	
	// called by framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtConnectionCancel : public NullAgentTestStep
{
public:
	CTestStepNullAgtConnectionCancel(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtConnectionCancel() {};
	
	// called by framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtLoopbackTest : public NullAgentTestStep
{
public:
	CTestStepNullAgtLoopbackTest(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtLoopbackTest() {};

	// called by the framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtReconnect: public NullAgentTestStep
{
public:
	CTestStepNullAgtReconnect(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtReconnect() {};

	// called by the framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtNotifications: public NullAgentTestStep
{
public:
	CTestStepNullAgtNotifications(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtNotifications() {};

	// called by the framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtOverrides: public NullAgentTestStep
{
public:
	CTestStepNullAgtOverrides(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtOverrides() {};

	// called by the framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtMultipleConnections: public NullAgentTestStep
{
public:
	CTestStepNullAgtMultipleConnections(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtMultipleConnections() {};

	// called by the framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

class CTestStepNullAgtCommDbIf: public NullAgentTestStep
{
public:
	CTestStepNullAgtCommDbIf(TPtrC aName);
	void ConstructL() {};
	virtual ~CTestStepNullAgtCommDbIf() {};

	// called by the framework to do the test
	virtual enum TVerdict doTestStepL(void);
private:
};

//
// the next section of this file contains declarations and definitions 
// common to many of the steps
//

const TUint KPortNo = 7;
const TUint KBufferLength = 512;


#endif /* __TESTSTEPNULLAGT_H__ */
