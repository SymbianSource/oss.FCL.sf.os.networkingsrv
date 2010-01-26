/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Short version of DHCP clinet side that is used by test software
* 
*
*/



/**
 @file te_TestDaemonClient.h
*/

#if (!defined __TESTDAEMONCLIENT_H__)
#define __TESTDAEMONCLIENT_H__

#include "DHCP_Std.h"

class RTestDaemonClient : public RSessionBase
	{
public:
	void CreateSession();
	void Ioctl(TUint aOptionLevel, TUint aOptionName, TRequestStatus& aStatus, TDes8* aDes=NULL);
	};

#endif

