// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IPProtoCpr.cpp
// IPProto Connection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/corecprstates.h>
#include <comms-infras/corecpractivities.h>
#include "linkcprextensionapi.h"

#ifdef __CFLOG_ACTIVE
	#define KIPProtoCprTag KESockConnectionTag
	//_LIT8(KIPProtoCprSubTag, "ipprotocpr");
#endif // __CFLOG_ACTIVE

using namespace ESock;
using namespace NetStateMachine;


START_ATTRIBUTE_TABLE(CLinkCprExtensionApi, CLinkCprExtensionApi::EUid, CLinkCprExtensionApi::ETypeId)
// No attributes defined, as no serialisation takes place.
END_ATTRIBUTE_TABLE()


CLinkCprExtensionApi* CLinkCprExtensionApi::NewLC(CIPProtoConnectionProvider& aCpr)
	{
	CLinkCprExtensionApi* self = new (ELeave) CLinkCprExtensionApi(aCpr);
	CleanupStack::PushL(self);
	return self;
	}

CLinkCprExtensionApi::CLinkCprExtensionApi(CIPProtoConnectionProvider& aCpr)
    : iLastProgress(KConnectionUninitialised, KErrNone),
	  iLastProgressError(KConnectionUninitialised, KErrNone),
	  iCpr(aCpr)
	{
	}

void CLinkCprExtensionApi::ProgressL(TProgressBuf& aBuffer) const
	{
	aBuffer = iLastProgress;
 	}

void CLinkCprExtensionApi::LastProgressError(TProgressBuf& aBuffer)
	{
	aBuffer = iLastProgressError;
	iLastProgressError.iStage = KConnectionUninitialised;
	iLastProgressError.iError = KErrNone;
	}

TInt CLinkCprExtensionApi::EnumerateSubConnectionsL(TUint& /*aCount*/)
	{
	return KErrNone;
	}

TInt CLinkCprExtensionApi::AllSubConnectionNotificationEnable()
	{
	return KErrNone;
	}

void CLinkCprExtensionApi::SetLastProgress(const Elements::TStateChange& aStateChange)
    {
    iLastProgress = aStateChange;
    if (aStateChange.iError != KErrNone)
        {
        iLastProgressError = aStateChange;
        }
    }
