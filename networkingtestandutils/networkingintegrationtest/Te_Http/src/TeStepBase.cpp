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

// includes
#include <test/testexecutelog.h>
#include <e32base.h>

#include "TeStepBase.h"
#include "TeListenerMgr.h"

_LIT(KListenerName,	"Listener");
_LIT(KListenerSection,"ListenerSection");
_LIT(KConnectorName, "Connector");
_LIT(KConnectorSection,"ConnectorSection");

CTestStepBase::CTestStepBase(CTestListenerMgr* aListenerMgr)
: iListenerMgr(aListenerMgr)
	{
	}

CTestStepBase::~CTestStepBase()
	{
	}

void CTestStepBase::CreateListenerL()
	{
	TPtrC	listenerName;

	if ( GetStringFromConfig(KListenerSection, KListenerName, listenerName) )
		{
		INFO_PRINTF2(_L("listener name = %S"), &listenerName);
		iListener = iListenerMgr->CreateListenerL(listenerName, this);
		}
	else
		{
		ERR_PRINTF1(_L("Missing listener name"));
		User::Leave(KErrArgument);
		}
	}

void CTestStepBase::CreateConnectorL()
	{
	TPtrC	connectorName;

	if ( GetStringFromConfig(KConnectorSection, KConnectorName, connectorName) )
		{
		INFO_PRINTF2(_L("connector name = %S"), &connectorName);
		iConnector = iListenerMgr->CreateConnectorL(connectorName, this);
		}
	else
		{
		ERR_PRINTF1(_L("Missing connector name"));
		User::Leave(KErrArgument);
		}
	}

