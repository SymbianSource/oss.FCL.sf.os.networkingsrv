/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* (test steps which set the mode of subsequent test case calls.
* this gives more power to the test scripts and reduces need
* for adding 1 test step in C++ per test case)
* 
*
*/



/**
 @file te_dhcpTestCommandSteps.h
*/
#if (!defined __DHCPTESTCOMAMNDSTEPS_H__)
#define __DHCPTESTCOMMANDSTEPS_H__
#include <test/testexecutestepbase.h>
#include "te_dhcpTestServer.h"


class CDhcpTestCommandBase : public CDhcpTestStepBase
	{
public:
	TVerdict doTestStepPreambleL();
	};
	

#define DECLARE_DHCP_TEST_COMMAND(nr) \
	_LIT(KDhcpTestCommand##nr,#nr); \
class CDhcpTestCommand##nr : public CDhcpTestCommandBase \
   { \
public: \
	CDhcpTestCommand##nr() \
      { \
	  SetTestStepName(KDhcpTestCommand##nr); \
      }; \
	  virtual TVerdict doTestStepL(); \
	}
	

DECLARE_DHCP_TEST_COMMAND(SetAddressMode);
DECLARE_DHCP_TEST_COMMAND(SetIAPToUse);
DECLARE_DHCP_TEST_COMMAND(SetDebugFlags);
DECLARE_DHCP_TEST_COMMAND(SetIPv4LinkLocal);

#endif
