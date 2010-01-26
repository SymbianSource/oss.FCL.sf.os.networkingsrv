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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <e32base.h>
#include <comms-infras/commsdebugutility.h>
#include "Tlog.h"

//
//Constants & macro definitions for t_ppp test harness
//

//Logging & testing
const TPtrC KPppLogFolder(_L("ppp"));
const TPtrC KPppLogFile(_L("t_ppp.txt"));


extern TBool Checkpoint(const TInt& aVal,const TInt& aExpectedVal,const char* aFileName,TUint aLine,const TDesC& aMsg);
extern TBool Checkpoint(TBool aExpr,const char* aFileName,TUint aLine,const TDesC& aMsg);
extern void Log(TRefByValue<const TDesC> aFmt, ...);
extern RTest test;
extern RLog  TestLog;

#define CHECKPOINT(value, expected,msg) Checkpoint((value),(expected),__FILE__,__LINE__,msg)
#define CHECKPOINT_BOOL(expr,msg) Checkpoint(expr,__FILE__,__LINE__,msg)
#define CHECKPOINT_L(value, expected,msg) if (! Checkpoint((value),(expected),__FILE__,__LINE__,msg)) User::Leave(value) 
#define CHECKPOINT_BOOL_L(expr,msg,err_code) if (! Checkpoint((expr),__FILE__,__LINE__,msg)) User::Leave(err_code)

 
#define LOG(msg) 	Log(msg)
#define LOGF1(fmt,arg1) 	Log((fmt),(arg1))
#define LOGF2(fmt,arg1,arg2)   Log((fmt),(arg1),(arg2))


//control link constants
const TInt KTerminator =36; //$ symbol
const  TPtrC8 KTerminatorStr(_L8("$"));
const TInt KTerminatorCount = 1; 
const TUint KMaxMsgLen = 100;// in bytes, since we deal with 8bit (narrow) data

const TPtrC KCtrlCommPortName(_L("COMM::1"));		
const TPtrC KCtrlCommPortName4asbt(_L("COMM::2"));	

const TUint KMaxNumArgs = 5;					
const TUint KMaxCmdLen = 30;					

//common typedefs
typedef TPtrC8 TCommands[KMaxNumArgs]; // container type for parsed control commands
typedef TBuf8<KMaxMsgLen> TBuffer;       //control link raw data buffer type

//control command names
const TPtrC8 KStartCmd			(_L8("start"));			// 
const TPtrC8 KStopCmd			(_L8("stop"));			// 
const TPtrC8 KStartTermCmd   (_L8("terminate"));//

const TUint   KMaxCfgIdLength = 1;
//
const TPtrC KModemPortString(_L("Modem\\PortName"));
const TPtrC KModemCsyString(_L("Modem\\CSYName"));
#define NIF_IFSERVERMODE    _L("Service\\IfServerMode")

//hardcoded ppp configuration (in normal situation it's in the CommDB)
const TBool KPppIsServerMode=EFalse;
const TPtrC KPppCommdbName(_L("Dummy"));
const TPtrC KPppPortString(_L("COMM::0"));
const TPtrC KPppCsyString(_L("ECUART"));
const TBool KPppIsLcpExtEnabled=EFalse;
const TBool KPppIsSwCompEnabled=EFalse;
const TBool KPppIsCallbackEnabled=EFalse;

//file paths
const TPtrC KTestConfigFilePaths =_L("\\;\\system\\data\\");
const TPtrC KTestConfigFileName =_L("t_ppp.xml");
const TPtrC KPppIniFullPath =_L("\\system\\data\\ppp.ini");

const TPtrC KPppNifFileName=_L("Tqosppp.nif");

const TPtrC KPppNtRasAgentFileName=_L("ntras.agt");
//const TPtrC KPppNifFilePath=_L("");

//special for the test. Under this section there're values which normally are at commdb
const TPtrC KCommDbSectionName=_L("commdb");





#endif //__COMMON_H__
