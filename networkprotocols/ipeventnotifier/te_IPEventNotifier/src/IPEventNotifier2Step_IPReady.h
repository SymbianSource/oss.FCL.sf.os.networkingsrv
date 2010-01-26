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


#if (!defined __IPEVENTNOTIFIER2_STEP_H__)
#define __IPEVENTNOTIFIER2_STEP_H__
#include <test/testexecutestepbase.h>
#include "Te_IPEventNotifierSuiteStepBase.h"
#include <in_sock.h>

class CIPEventNotifier2Step_IPReady : public CTE_IPEventNotifierSuiteStepBase, public CDhcpSignal
	{
public:
	CIPEventNotifier2Step_IPReady();
	virtual TVerdict doTestStepL();

	static void SignalHandler( TAny* aThis, const Meta::SMetaData* aData );

private:

	TBool iExpectedReady;
	TInetAddr iTestAddr;
	TPtrC iNetworkInterfacePrefixToMonitor;
	};

_LIT(KIPEventNotifier2Step_IPReady,"IPEventNotifier2Step_IPReady");

#endif
