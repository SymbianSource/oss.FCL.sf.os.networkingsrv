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
 @prototype
*/


#ifndef SYMBIAN_AGENTSCPRFACTORY_H
#define SYMBIAN_AGENTSCPRFACTORY_H


#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/ss_log.h>
#include <comms-infras/ss_nodemessages.h>


NONSHARABLE_CLASS(CAgentSubConnectionProviderFactory) : public ESock::CSubConnectionProviderFactoryBase
	{		
public:
    enum { iUid = 0x10281DEA };
	static CAgentSubConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
	CAgentSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer);
	
	// Implementation of CSubConnectionProviderFactoryBase
   virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
   };

#endif
// SYMBIAN_AGENTSCPRFACTORY_H

