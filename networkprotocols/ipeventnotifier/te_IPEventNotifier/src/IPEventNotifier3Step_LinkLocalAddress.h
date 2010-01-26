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


#if (!defined __IPEVENTNOTIFIER3_STEP_H__)
#define __IPEVENTNOTIFIER3_STEP_H__
#include <test/testexecutestepbase.h>
#include "Te_IPEventNotifierSuiteStepBase.h"
#include <in_sock.h>

class CIPEventNotifier3Step_LinkLocalAddress : public CTE_IPEventNotifierSuiteStepBase, public CDhcpSignal
	{
public:
	CIPEventNotifier3Step_LinkLocalAddress();
	virtual TVerdict doTestStepL();

	static void SignalHandler( TAny* aThis, const Meta::SMetaData* aData );

private:

	TInetAddr iLLAddr;
	TPtrC iNetworkInterfacePrefixToMonitor;
	};

_LIT(KIPEventNotifier3Step_LinkLocalAddress,"IPEventNotifier3Step_LinkLocalAddress");







#endif
