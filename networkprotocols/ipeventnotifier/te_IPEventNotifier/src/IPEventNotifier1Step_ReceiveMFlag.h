// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if (!defined __IPEVENTNOTIFIER1STEP_H__)
#define __IPEVENTNOTIFIER1STEP_H__
#include <test/testexecutestepbase.h>
#include "Te_IPEventNotifierSuiteStepBase.h"

class CIPEventNotifier1Step_ReceiveMFlag : public CTE_IPEventNotifierSuiteStepBase, public CDhcpSignal
	{
public:
	CIPEventNotifier1Step_ReceiveMFlag();
	virtual TVerdict doTestStepL();

	static void SignalHandler( TAny* aThis, const Meta::SMetaData* aData );

private:

	TBool iExpectedMFlagValue;
	TPtrC iNetworkInterfacePrefixToMonitor;
	};

_LIT(KIPEventNotifier1Step_ReceiveMFlag,"IPEventNotifier1Step_ReceiveMFlag");







#endif
