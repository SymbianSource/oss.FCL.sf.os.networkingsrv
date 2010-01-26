// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32base.h>
#include <e32std.h>
#include <c32comm.h>
#include <e32hal.h>
#include <comms-infras\nifif.h>
#include <comms-infras\nifagt.h>
//

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#define LDD_FNAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
//#define PDD2_NAME _L("EUART2")
#define LDD_NAME _L("ECOMM")
#define LDD_FNAME _L("FCOMM")
#endif

GLDEF_C void CommInitL(TBool aEnhanced)
	{
	TInt err;
	err=User::LoadPhysicalDevice(PDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		User::Leave(err);
#if defined PDD2_NAME
# if defined (__EPOC32__)
	TMachineInfoV1Buf info;
	User::LeaveIfError(UserHal::MachineInfo(info));
	if (info().iMachineName.Compare(_L("PROTEA_RACKC"))==0
		|| info().iMachineName.Compare(_L("PROTEA_RACKD"))==0)
		{
# endif
		err = User::LoadPhysicalDevice(PDD2_NAME);
		if (err!=KErrNone && err!=KErrAlreadyExists)
			User::Leave(err);
# if defined (__EPOC32__)
		}
# endif
#endif
	if (aEnhanced)
		err=User::LoadLogicalDevice(LDD_FNAME);
	else
		err=User::LoadLogicalDevice(LDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		User::Leave(err);
	}

//
