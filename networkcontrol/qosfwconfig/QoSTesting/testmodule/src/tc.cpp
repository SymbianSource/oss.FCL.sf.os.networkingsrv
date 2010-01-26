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
//
 
#include <flow.h>
#include "testmodule.h"
#include "tc.h"

CFlowData* CFlowData::NewL(CFlowContext* aHandle, CNif* aNif)
    {
    __ASSERT_ALWAYS(aHandle , User::Panic(_L("CFlowData::NewL"), 0));
    __ASSERT_ALWAYS(aNif , User::Panic(_L("CFlowData::NewL"), 0));
    CFlowData* aFlowData = new (ELeave) CFlowData(aHandle, aNif);
    CleanupStack::PushL(aFlowData);
    aFlowData->ConstructL();
    CleanupStack::Pop();
    return aFlowData;
    }

CFlowData::CFlowData(CFlowContext* aHandle, CNif* aNif) : 
  iNif(aNif), iFlowContext(aHandle)
    {
    }

CFlowData::~CFlowData()
    {
    }

void CFlowData::ConstructL()
    {
    }

