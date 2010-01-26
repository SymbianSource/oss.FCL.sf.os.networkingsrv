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

#include "common.h"

// constructor to initialise all constant data.
CCommonData::CCommonData() :
  KTerminator(36), 
  KTerminatorCount(1),
  KTerminatorStr(_L8("$")),
  KCtrlCommPortName(_L("COMM::1")),		
  KCtrlCommPortName4asbt(_L("COMM::2")),
  KCtrlCommPortName4H2(_L("COMM::3")),
  KStartCmd(_L8("start")),
  KStopCmd(_L8("stop")),			
  KStartTermCmd(_L8("terminate")),
  KMaxCfgIdLength ( 1),
  KMaxCmdLen(30),
  KModemPortString(_L("ModemBearer\\PortName")),
  KModemCsyString(_L("ModemBearer\\CSYName")),
  KModemCommRoleString(_L("ModemBearer\\CommRole")),
  KIapIdString(_L("IAP\\Id")),
  KPppIsServerMode(EFalse),
  KPppCommdbName(_L("Dummy")),
  KPppPortString(_L("COMM::0")),
  KPppCsyString(_L("ECUART")),
  KPppCommRole(0),
  KPppIapId(1),
  KPppIsLcpExtEnabled(EFalse),
  KPppIsSwCompEnabled(EFalse),
  KPppIsCallbackEnabled(EFalse),
  KTestConfigFilePaths(_L("z:\\private\\101f7989\\ESock\\;\\;c:\\system\\data\\;z:\\testdata\\configs\\")),
  KTestConfigFileName(_L("te_ppp.cfg")),
  KPppIniFullPath(_L("\\private\\101F7989\\ESock\\ppp.ini")),
  KPppNifFileName(_L("tppp.nif")),
  KCommDbSectionName(_L("commdb"))
  
	{
	}
