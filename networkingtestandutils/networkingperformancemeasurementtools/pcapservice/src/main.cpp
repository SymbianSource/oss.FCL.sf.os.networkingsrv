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
// Pcap service main entry point
// 
//

/**
 @file
 @internalTechnology
*/

#include <stdio.h>
#include "GenericStub.h"	
#include "PcapService.h"

#define SERVICE_IID		0x34631002
#define SERVICE_VERSION	1


int main()
{
	printf("PcapService debug console...\n");

	// set up on startup to ensure config is ok ASAP
	if (!CPcapService::Instance()->Setup())
		return -1;

	printf("PcapService ready for UCC calls...\n");

	return StartUCCService(SERVICE_IID, SERVICE_VERSION);
}


