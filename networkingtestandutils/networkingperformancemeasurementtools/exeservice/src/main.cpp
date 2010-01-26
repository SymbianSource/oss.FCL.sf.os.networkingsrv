// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ExeService main entry point
// 
//

/**
 @file
 @internalTechnology
*/


#include <CService.h>
#include <stdio.h>
#include "ExeService.h"

#define SERVICE_IID		0x34631000
#define SERVICE_VERSION	1


int main()
{
	printf("ExeService debug console...\n");

	if (__argc <= 1)
	{
		printf("Usage: exeservice --outputdir <output directory> --proclist <binaries to allow>\n");
		printf("e.g.  exeservice --outputdir outdir --proclist iperf,ping,cmd\n");
		return -1;
	}

	// set up on startup to ensure config is ok ASAP
	if (!CExeService::Instance()->Setup())
		return -1;

	printf("ExeService ready for UCC calls...\n");

	return StartUCCService(SERVICE_IID, SERVICE_VERSION);
}
