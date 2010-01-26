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

#include <f32file.h>
#include <hal.h>
#include "TestMgr.h"
#include "DummyAgtRef.h"
#include "serial.h"

//
//indexes in TCommands array

//
CTestMgr::CTestMgr(CTestExecuteLogger& aLogger, CPPPANVL* aTestHarness)
	: CActive(CActive::EPriorityStandard), iLogger(aLogger), iTestHarness(aTestHarness), KCmdIDIndex(0),
	KIniIdIndex(1)
{
}
//
CTestMgr* 
CTestMgr::NewL(CTestExecuteLogger& aLogger, CPPPANVL* aTestHarness)
{
	CTestMgr* self = NewLC(aLogger, aTestHarness);
	CleanupStack::Pop();    // self
	return self;
}
//      
CTestMgr* 
CTestMgr::NewLC(CTestExecuteLogger& aLogger, CPPPANVL* aTestHarness)
{
	CTestMgr * self = new (ELeave) CTestMgr(aLogger, aTestHarness);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

void 
CTestMgr::ConstructL()
{
	ConfigureSerialL();
	//
	CActiveScheduler::Add(this);
	// Construct the send / recv AOs
	ipSerialRx = CSerialListener::NewL(&iCommPort, this);
	ipSerialTx = CSerialSender::NewL(&iCommPort, this);
	//
	ipDummyAgtRef=CDummyNifAgentRef::NewL(_L("aa"),*this, iLogger);
	//initial state
	iCurrentState=EIdle;
	
	// create a console to give the interactive user some progress feedback
	console = Console::NewL(_L("te_ppp progress"),TSize(KConsFullScreen,KConsFullScreen));
}

CTestMgr::~CTestMgr()
{
	delete console ;
	delete ipSerialRx;
	delete ipSerialTx;
	delete ipDummyAgtRef;
}

void 
CTestMgr::ConfigureSerialL()
{
	TInt err=iCommServer.Connect();
	MGR_TEST_CHECKL(err, KErrNone, _L("Failed comm server connect\n"));
	// ;
	// load CSY module - RS232
	TBuf<16> csyName;
	csyName.Copy(_L("ECUART"));     // RS232
	err=iCommServer.LoadCommModule(csyName);
	MGR_TEST_CHECKL(err,KErrNone,_L("Failed load comm module\n"));

	// Open comm port - available comm port depends on test platform
	TInt iMashineId=0;
	HAL::Get(HAL::EMachineUid,iMashineId);
	switch (iMashineId)
		{
		case HALData::EMachineUid_Assabet :
		case HALData::EMachineUid_Lubbock :
			err = iCommPort.Open(iCommServer, iData.KCtrlCommPortName4asbt, ECommShared) ;
			aPortNo = iData.KCtrlCommPortName4asbt [6] - '0' ;
			break ;

		// H2 target only possible for EKA2 kernel (see defs in HALData)
		case HALData::EMachineUid_OmapH2 :
			err = iCommPort.Open(iCommServer, iData.KCtrlCommPortName4H2, ECommShared) ;
			aPortNo = iData.KCtrlCommPortName4H2 [6] - '0' ;
			break ;
		
		default : // Emulator etc - generally port 1
			err = iCommPort.Open(iCommServer, iData.KCtrlCommPortName, ECommShared);
			if (err == KErrNone)
				{
				aPortNo = iData.KCtrlCommPortName [6] - '0' ;
				}
			else
				{					
				// though some machines use port 2 via USB
				err = iCommPort.Open(iCommServer, iData.KCtrlCommPortName4asbt, ECommShared);
				aPortNo = iData.KCtrlCommPortName4asbt [6] - '0' ;
				}
		}
		
	LOG_ERR_PRINTF2(_L("comm port open returned %d"),err);
	MGR_TEST_CHECKL(err,KErrNone,_L("Failed open commport \n"));
	// configure the physical port setttings
	TCommConfig portSettings;
	iCommPort.Config (portSettings);
	portSettings().iRate = EBps38400;
	portSettings().iParity = EParityNone;
	portSettings().iDataBits = EData8;
	portSettings().iStopBits = EStop1;
	// configure the logical port settings
	//      portSettings().iFifo = EFifoEnable;
	//      portSettings().iHandshake = (KConfigObeyXoff | KConfigSendXoff);        // for xon/xoff handshaking
	// terminate end-of-command with a $
	portSettings().iTerminator[0] = static_cast<TText8>(iData.KTerminator);
	portSettings().iTerminatorCount = iData.KTerminatorCount;
	
	err=iCommPort.SetConfig(portSettings);
	MGR_TEST_CHECKL(err,KErrNone,_L("Failed comm port SetConfig \n"));
}

void CTestMgr::DoCancel()
{
}

void CTestMgr::StartEngineL()
{
	// Start serial port listener
	LOG_INFO_PRINTF1(_L("Starting serial port listener\n"));
	console->Printf (_L("ready to receive on port %d\nPlase start ANVL on Linux Machine\n\n"), aPortNo) ;
	
	ipSerialRx->Recv();
}

void CTestMgr::Notify(TEvent aEvent)
{
	iEvent = aEvent;        
	SetActive();
	TRequestStatus* theStatus=&iStatus;//for the next line to compile
	User::RequestComplete(theStatus, KErrNone);
}
//
//State machine
//
void CTestMgr::RunL()
{
	switch(iCurrentState)
	{
	case EIdle:
		HandleEvents_Idle();
		break;
	case ETestStarted:
		HandleEvents_TestStarted();
		break;
	case ETestClosed:
		HandleEvents_TestClosed();
		break;
	default:
		__ASSERT_ALWAYS(ETrue,User::Panic(_L("Wrong state"),KErrGeneral));
		break;
	}
}

void 
CTestMgr::HandleEvents_Idle()
{
	switch (iEvent)
	{
	case EReadComplete:
		{
			//parse buffer and extract commands
			TBuffer buffer;
			buffer.Copy( *(ipSerialRx->GetData()) );
			//translate 8->16 if needed (for logging purposes)
			TBuf<KMaxMsgLen> bufferU ;
			bufferU.Copy(buffer);
			LOG_ERR_PRINTF2(_L("State: Idle -> Serial link read: (%S) \n") ,&bufferU);
			
			TCommands cmd;
			int result=ParseForCommands(buffer,cmd);
			if (result > 0)
			{
				//issue start event if needed
				if (0==iData.KStartCmd.CompareF(cmd[KCmdIDIndex]))
				{
					//echo the command (ack)
					//We need to do it as early as possible for
					//ANVL to have time to prepare for the negotiation
					ipSerialTx->Send(buffer);
					User::After(250000);
					
					LOG_INFO_PRINTF1(_L(""));
					LOG_INFO_PRINTF1(_L("##################################################"));
					LOG_INFO_PRINTF1(_L(""));
					//as the reader may buffer info -> kill it
					ipDummyAgtRef->DestroyIniReader();
					TRAPD(errCode,UpdateIniFileL(cmd));
					//even if we failed -> try to use ppp.ini
					if (!ipDummyAgtRef->CreateIniReader())
					{
						console->Printf (_L("*** ERROR failed to open ppp.ini file ***\n")) ;
					}
					else
					{
						console->Printf (_L("OPENed .ini file\n")) ;
					}
					

					//translate 8->16 if needed (for logging purposes)
					TBuf<KMaxMsgLen> indexU;
					indexU.Copy(cmd[KIniIdIndex]);
					if (errCode != KErrNone)
						LOG_ERR_PRINTF3(_L("State: Idle -> PPP configuration (id : %S) change failed (error : %d)"),&indexU,errCode);
					else
						LOG_ERR_PRINTF2(_L("State: Idle -> PPP configuration (id : %S) changed"),&indexU);
					LOG_INFO_PRINTF1(_L("State: Idle -> Serial link read acknowlegment sent"));
					//change the state
					Notify(EStartTest);
				}
			}
			else
				LOG_INFO_PRINTF1(_L("State: Idle -> Unrecognized command on serial link") );
		}
		break;
		
	case EStartTest:
		//issue request to the dummy agt ref
		LOG_INFO_PRINTF1(_L("State: Idle ->Starting PPP") );
		iCurrentState=ETestStarted;
		//imitate an agent's upcalls
		ipDummyAgtRef->ServiceStarted();
		if (EStopTest != iEvent)//ServiceStarted might have problems...
			ipDummyAgtRef->ConnectComplete(KErrNone);
		break;
		
	case EWriteComplete:
		LOG_INFO_PRINTF1(_L("State:Idle -> control link write finished"));
		break;
		
	default:
		LOG_ERR_PRINTF2(_L("State: Idle -> Unexpected event (%d) ") ,STATIC_CAST(TInt,iEvent));
	};
}

void
CTestMgr::HandleEvents_TestStarted()
{
	switch (iEvent)
	{
	case EReadComplete:
		//parse the buffer and extract instructions
		{
			TBuffer buffer;
			buffer.Copy( *(ipSerialRx->GetData()) );
			//translate 8->16 if needed (for logging purposes)
			TBuf<KMaxMsgLen> bufferU ;
			bufferU.Copy(buffer);
			LOG_ERR_PRINTF2(_L("State: TestStarted ->Serial link read: (%S) ") ,&bufferU);
			
			TCommands cmd;
			int result=ParseForCommands(buffer,cmd);
			if (result > 0)
			{
				//issue stop event if needed
				if (0==iData.KStopCmd.CompareF(cmd[KCmdIDIndex]))
					Notify(EStopTest);
				else if (0==iData.KStartTermCmd.CompareF(cmd[KCmdIDIndex]))
				{
					//initiate PPP termination from our side
					Notify(EStartTerminate);
				}
				else
					LOG_INFO_PRINTF1(_L("State: TestStarted -> Unexpected command "));
			}
			else
				LOG_INFO_PRINTF1(_L("State: TestStarted -> Unrecognized command on the serial link "));
		}
		break;
		
	case EStopTest:         // It's stopped by the ANVL
		//send stop to the dummy agt
		LOG_INFO_PRINTF1(_L("State: TestStarted ->Stop test "));
		iCurrentState=EIdle;
		ipDummyAgtRef->ServiceClosed();
		break;
		
	case EStartTerminate:           
		//
		LOG_INFO_PRINTF1(_L("State: TestStarted ->Send terminate Req"));
		//make PPP send Terminate Req
		ipDummyAgtRef->Stop();
		break;
		
	case ETestFinished://PPP considers negotiations finished 
		LOG_INFO_PRINTF1(_L("State: TestStarted -> Test finished"));
		iCurrentState=ETestClosed;
		break;
		
	case EWriteComplete:
		LOG_INFO_PRINTF1(_L("State:TestStarted -> control link write finished"));
		break;
	
	case EStartTest:
		//
		LOG_ERR_PRINTF2(_L("State: TestStarted -> Stop is missed and new start is received (%d) "),STATIC_CAST(TInt,iEvent));
		ipDummyAgtRef->ServiceClosed();

		//issue request to the dummy agt ref
		LOG_INFO_PRINTF1(_L("State: Idle ->Starting PPP") );
		//imitate an agent's upcalls
		ipDummyAgtRef->ServiceStarted();
		if (EStopTest != iEvent)//ServiceStarted might have problems...
			ipDummyAgtRef->ConnectComplete(KErrNone);
		break;

	default:
		LOG_ERR_PRINTF2(_L("State: TestStarted -> Unexpected event (%d) "),STATIC_CAST(TInt,iEvent));
	};
}

//ETestClosed state isn't used at the current state of the harness.
//So it's made transitional
void
CTestMgr::HandleEvents_TestClosed()
{
	switch (iEvent)
	{
	case EReadComplete:
		//parse the buffer and extract instructions
		{
			TBuffer buffer;
			buffer.Copy( *(ipSerialRx->GetData()) );
			//translate 8->16 if needed (for logging purposes)
			TBuf<KMaxMsgLen> bufferU ;
			bufferU.Copy(buffer);
			LOG_ERR_PRINTF2(_L("State: TestClosed ->Serial link read: (%S) ") ,&bufferU);
			
			TCommands cmd;
			int result=ParseForCommands(buffer,cmd);
			if (result > 0)
			{
				//issue stop event if needed
				if (0==iData.KStopCmd.CompareF(cmd[KCmdIDIndex]))
				{
					iCurrentState=EIdle;
					ipDummyAgtRef->ServiceClosed();
				}
				else 
					LOG_INFO_PRINTF1(_L("State: TestClosed -> Unexpected command "));
			}
			else
				LOG_INFO_PRINTF1(_L("State: TestClosed -> Unrecognized command on the serial link "));
		}
		break;
	default:
		LOG_ERR_PRINTF2(_L("State: TestClosed -> Unexpected event (%d) "),STATIC_CAST(TInt,iEvent));
	};
}

//The command from the ANVL are supposed to be of the next format
// startID            - where ID is one byte and designates the configuration tobe used
// stop 
//all others commands're considered to be wrong

TInt//return the number of elements put into aCommands  
CTestMgr::ParseForCommands(const TBuffer& aRawData,TCommands& aCommands )
{
	TInt index=aRawData.FindF(iData.KStartCmd);
	if (index >= 0)
	{
		TInt minimumLenght=iData.KMaxCfgIdLength+index+iData.KStartCmd.Length();
		if (aRawData.Length() >= minimumLenght)
		{
			//the cmd
			aCommands[KCmdIDIndex].Set(aRawData.Mid(index,iData.KStartCmd.Length()));
			//the cfg id
			TInt terminatorIndex=aRawData.FindF(iData.KTerminatorStr);
			if (terminatorIndex > 0)
			{
				aCommands[KIniIdIndex].Set(aRawData.Mid(index+iData.KStartCmd.Length(),terminatorIndex-index-iData.KStartCmd.Length()));
			}
			else
			{
				aCommands[KIniIdIndex].Set(_L8("no_index!!!"));
			}

			// display a progress message
			TInt val ;
			TLex8 lex (aCommands[KIniIdIndex]);
			lex.Val (val) ;

			console->Printf (_L("Start Command: %d\n"), val) ;
			return 2;
		}
		LOG_INFO_PRINTF1(_L("Wrong format of the start command"));
	}
	else
	{
		index=aRawData.FindF(iData.KStopCmd);
		if (index >=0)
		{
			console->Printf (_L("Stop Command\n")) ;
		
			aCommands[KCmdIDIndex].Set(aRawData.Mid(index,iData.KStopCmd.Length()));
			return 1;
		}
		else 
		{
			index=aRawData.FindF(iData.KStartTermCmd);
			if (index >= 0)
			{
				aCommands[KCmdIDIndex].Set(aRawData.Mid(index,iData.KStartTermCmd.Length()));
				return 1;
			}
		}
	}
	return 0;
}

//the  test ini file has format
//[[testid1]]
//........  <----content to be copied into ppp.ini
//[[testid2]]
//....
void CTestMgr::UpdateIniFileL(const TCommands& aCommands)
{
	TAutoClose<RFs> fs;
	RFs& fileServer=fs.iObj;
	User::LeaveIfError(fileServer.Connect());
	
	//open the test configuration file
	TFindFile fileLookup(fileServer);
	TInt error = fileLookup.FindByPath(iData.KTestConfigFileName,&iData.KTestConfigFilePaths) ;

	if (error != KErrNone)
	{
		LOG_ERR_PRINTF1(_L("Failed to find te_ppp.cfg configuration file"));
	}

	// show user some progress info
	TPtrC fn = fileLookup.File() ;
	console->Printf (_L("TE_PPP.CFG configuration file is %S\n"), &fn) ;
	
	TAutoClose<RFile> f;
	RFile& cfgFile=f.iObj;
	
	TInt fileSize=0;
	User::LeaveIfError(cfgFile.Open(fileServer,fileLookup.File(),EFileShareAny));
	User::LeaveIfError(cfgFile.Size(fileSize));
	//read its content & look for the config ID
	TUint8* pFileContent__=new TUint8[fileSize+1];
	if (!pFileContent__)
		User::Leave(KErrNoMemory);
	CleanupStack::PushL(pFileContent__);
	
	TPtr8 fileContent(pFileContent__,fileSize+1,fileSize+1);
	
	User::LeaveIfError(cfgFile.Read(fileContent));
	
	TBuf8<10> sectionHeader;
	sectionHeader.Append(_L8("[["));
	sectionHeader.Append(aCommands[KIniIdIndex]);
	sectionHeader.Append(_L8("]]"));
	//cut out the specific section: from "[[0xnn]]" to next "[[" or the end of file 
	TInt startPosition = fileContent.Find(sectionHeader);
	if (startPosition>= 0)
	{
		fileContent.Delete(0,startPosition+sectionHeader.Length());
		TInt endPosition=fileContent.Find(_L8("[["));
		if (endPosition >0 )
			fileContent.Delete(endPosition,fileContent.Length()-endPosition);
		
		TAutoClose<RFile> fileIni;
		RFile& iniFile=fileIni.iObj;
		//create new ppp.ini
		User::LeaveIfError(iniFile.Replace(fileServer,iData.KPppIniFullPath,EFileShareAny));
		User::LeaveIfError(iniFile.Write(fileContent));
		//              LOG_INFO_PRINTF1(_L("<<<<<<<<ppp.ini content>>>>>>>>>>>>"));
		//              LOG_ERR_PRINTF2(_L("%S"),&fileContent);
		//              LOG_INFO_PRINTF1(_L(">>>>>>>>-----------------<<<<<<<<<<<<"));
	}
	
	CleanupStack::Pop();//pFileContent__
}
