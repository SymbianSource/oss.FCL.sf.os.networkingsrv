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

#ifndef __TESTMGR_H__
#define __TESTMGR_H__
/**
*	Main  controller class (the engine)
*/
#include <c32comm.h>
#include <testexecuteserverbase.h>
#include <testexecutelog.h>
#include "TestSteps.h"

#include "common.h"

class CDummyNifAgentRef;
class CSerialListener;
class CSerialSender;

class CTestMgr : public CActive
{
public:
	~CTestMgr();
	static CTestMgr* NewL(CTestExecuteLogger& aLogger, CPPPANVL* aTestHarness);
	static CTestMgr* NewLC(CTestExecuteLogger& aLogger, CPPPANVL* aTestHarness);
	
	void StartEngineL();//main entry point. starts the state machine
	
	enum TEvent
	{	
		EReadComplete,   //ctrl link read complete
		EWriteComplete,  //ctrl link write complete 
		EStartTest,          //( EIdle -> ETestStarted  ) "start" command from the ANVL
		EStopTest,			 //( ETestStarted OR ETestClosed -> EIdle ) - "stop" command from the ANVL
		ETestFinished,		//( ETestStarted -> ETestClosed ) - PPP closed the connection
		EStartTerminate	  //( ETestStarted -> ETestClosed ) - "initiate terminate" command from the ANVL
	};
	//event notification
	void Notify(TEvent aEvent);
private:
	TInt aPortNo ;

	CTestMgr(CTestExecuteLogger& aLogger, CPPPANVL* aTestHarness);
	void ConstructL();		
	
	void RunL();
	void DoCancel();
	// configure control link
	void ConfigureSerialL();
	//state machine
	enum TState
	{
		EIdle,
		ETestStarted,
		ETestClosed //transitional to performe cleanup & buffered ops
	};
	//
	void HandleEvents_Idle();
	void HandleEvents_TestStarted();
	void HandleEvents_TestClosed();
	//utilities
	TInt ParseForCommands(const TBuffer& aRawData,TCommands& aCommands); //raw data from ctrl link -> cmds
	void UpdateIniFileL(const TCommands& aCommands);//gets appropriate chunk of cfg file and puts it into ppp.ini
	//attributes		
	RComm iCommPort;			
	RCommServ iCommServer;		
	CSerialListener* ipSerialRx;		// listening on the serial port
	CSerialSender* ipSerialTx;		// for sending on the serial port
	TEvent iEvent;					   //last event
	TState  iCurrentState;
	CDummyNifAgentRef*   ipDummyAgtRef;  
	CTestExecuteLogger iLogger;
	CPPPANVL* iTestHarness;
	const TUint KCmdIDIndex;
	const TUint KIniIdIndex;
	CCommonData iData;
	CConsoleBase* console ;		// personal display
};

#define LOG_INFO_PRINTF1(p1)							iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1)) 
#define LOG_INFO_PRINTF2(p1, p2)						iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2)) 
#define LOG_INFO_PRINTF3(p1, p2, p3)					iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3)) 
#define LOG_INFO_PRINTF4(p1, p2, p3, p4)				iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4))

#define LOG_ERR_PRINTF1(p1)								iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1)) 
#define LOG_ERR_PRINTF2(p1, p2)							iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2)) 
#define LOG_ERR_PRINTF3(p1, p2, p3)						iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3))
#define LOG_ERR_PRINTF4(p1, p2, p3, p4)					iLogger.LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3), (p4))

// Check a boolean is true
#define MGR_TESTL(a) iTestHarness->testBooleanTrueL((a), (TText8*)__FILE__, __LINE__) 
#define MGR_TEST(a)  iTestHarness->testBooleanTrue((a), (TText8*)__FILE__, __LINE__) 

// Check a boolean is true if not return error code b
#define MGR_TESTE(a, b) iTestHarness->testBooleanTrueWithErrorCode((a), (b), (TText8*)__FILE__, __LINE__) 
#define MGR_TESTEL(a, b) iTestHarness->testBooleanTrueWithErrorCodeL((a), (b), (TText8*)__FILE__, __LINE__)  
#define MGR_TEST_CHECKL(p1, p2, p3) iTestHarness->TestCheckPointCompareL((p1), (p2), (p3), (TText8*)__FILE__, __LINE__)	

#endif //__TESTMGR_H__
