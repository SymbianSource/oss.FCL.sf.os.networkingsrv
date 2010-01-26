// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Short version of DHCP clinet side that is used by test software
// 
//

/**
 @file te_testDaemonClient.cpp
*/
#include <e32std.h>
#include "Te_TestDaemonClient.h"

#ifdef SYMBIAN_NETWORKING_PLATSEC
    #include "comms-infras/rconfigdaemonmess.h"
#else
    #include "comms-infras/CS_daemonmess.h"
#endif




void RTestDaemonClient::Ioctl(TUint aOptionLevel, TUint aOptionName, TRequestStatus& aStatus, TDes8* aDes)
	{
  	SendReceive(EConfigDaemonIoctl, TIpcArgs(aOptionLevel, aOptionName, aDes, aDes ? aDes->MaxLength() : 0), aStatus);
	}

void RTestDaemonClient::CreateSession()
	{
	RSessionBase::CreateSession(KDHCPServerName, TVersion(1,0,0), 10);
	}
