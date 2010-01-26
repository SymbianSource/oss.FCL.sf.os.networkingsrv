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
//

/**
 @file
 @internalTechnology
*/

#include "DummyProvision.h"
#include "DummyAgentHandler.h"

/**
Upcall from the Agent

Retrieve the dynamic provisioning information from the Agent and add a provisioning
structure to the CAccessPointConfig.
*/
void CDummyProtoAgentHandler::ConnectCompleteL()
	{
	TBool pop = EFalse;
	TDummyAgentProvision* provision = const_cast<TDummyAgentProvision*>(static_cast<const TDummyAgentProvision*>(GetExtension(STypeId::CreateSTypeId(TDummyAgentProvision::EUid, TDummyAgentProvision::ETypeId))));
	if (!provision)
		{
		provision = new (ELeave) TDummyAgentProvision;
		CleanupStack::PushL(provision);		
		pop = ETrue;
		}
				
	provision->SetIsDialIn(QueryIsDialIn());
	
	if (pop)	
		{
		AppendExtensionL(provision);
		CleanupStack::Pop(provision); // ownership transferred to AccessPointConfig
		}		
	}

