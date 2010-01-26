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
 @internalComponent
*/

#if (!defined __IPEVENTNOTIFIER4STEP_H__)
#define __IPEVENTNOTIFIER4STEP_H__
#include <test/testexecutestepbase.h>
#include "Te_IPEventNotifierSuiteStepBase.h"

class CIPEventNotifier4Step_ReceiveOFlag : public CTE_IPEventNotifierSuiteStepBase, public CDhcpSignal
	{
public:
	CIPEventNotifier4Step_ReceiveOFlag();
	virtual TVerdict doTestStepL();

	static void SignalHandler( TAny* aThis, const Meta::SMetaData* aData );

private:

	TBool iExpectedOFlagValue;
	TPtrC iNetworkInterfacePrefixToMonitor;
	};

_LIT(KIPEventNotifier4Step_ReceiveOFlag,"IPEventNotifier4Step_ReceiveOFlag");







#endif
