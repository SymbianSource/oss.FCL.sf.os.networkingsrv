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
#include <e32test.h>
//
//Constants & macro definitions for te_ppp test harness
//

#define NIF_IFSERVERMODE    _L("Service\\IfServerMode")
const TUint KMaxNumArgs = 5;					
const TUint KMaxMsgLen = 100; // in bytes, since we deal with 8bit (narrow) data

//common typedefs
typedef TPtrC8 TCommands[KMaxNumArgs]; // container type for parsed control commands
typedef TBuf8<KMaxMsgLen> TBuffer;       //control link raw data buffer type

class CCommonData {
public:
	CCommonData();

	//control link constants
	const TInt   KTerminator; //$ symbol
	const TInt   KTerminatorCount; 
	
	const TPtrC8 KTerminatorStr;
	const TPtrC  KCtrlCommPortName;
	const TPtrC  KCtrlCommPortName4asbt;
	const TPtrC  KCtrlCommPortName4H2;
	
	
	//control command names
	const TPtrC8   KStartCmd;
	const TPtrC8   KStopCmd;
	const TPtrC8   KStartTermCmd;
	
	const TUint   KMaxCfgIdLength;
	//
	const TUint  KMaxCmdLen;			

	//const TUint KSlashChar;
	const TPtrC  KModemPortString;
	const TPtrC  KModemCsyString;
	const TPtrC  KModemCommRoleString;
	const TPtrC  KIapIdString;

	//hardcoded ppp configuration (in normal situation it's in the CommDB)
	const TBool  KPppIsServerMode;
	const TPtrC  KPppCommdbName;
	const TPtrC  KPppPortString;
	const TPtrC  KPppCsyString;
	const TUint32 KPppCommRole;
	const TUint32 KPppIapId;
	const TBool  KPppIsLcpExtEnabled;
	const TBool  KPppIsSwCompEnabled;
	const TBool  KPppIsCallbackEnabled;
 	
	//file paths
	const TPtrC  KTestConfigFilePaths;
	const TPtrC  KTestConfigFileName;
	const TPtrC  KPppIniFullPath;
	
	const TPtrC  KPppNifFileName;
	
	//const TPtrC KPppNifFilePath;
	
	//special for the test. Under this section there're values which normally are at commdb
	const TPtrC  KCommDbSectionName;
};

#endif //__COMMON_H__
