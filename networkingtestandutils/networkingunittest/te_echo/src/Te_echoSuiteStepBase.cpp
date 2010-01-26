// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file Te_echoSuiteStepBase.cpp
 @internalTechnology
*/

#include "Te_echoSuiteStepBase.h"
#include "Te_echoSuiteDefs.h"
#include <in_sock.h>

TVerdict CTe_echoSuiteStepBase::doTestStepPreambleL()
/**
 * @return - TVerdict
 * Implementation of CTestStep base class virtual
 * It is used for doing all initialisation common to derived classes in here.
 * Make it being able to leave if there are any errors here as there's no point in
 * trying to run a test step if anything fails.
 * The leave will be picked up by the framework.
 */
	{
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CTe_echoSuiteStepBase::doTestStepPostambleL()
/**
 * @return - TVerdict
 * Implementation of CTestStep base class virtual
 * It is used for doing all after test treatment common to derived classes in here.
 * Make it being able to leave
 * The leave will be picked up by the framework.
 */
	{
	return TestStepResult();
	}

CTe_echoSuiteStepBase::~CTe_echoSuiteStepBase()
	{
	}

CTe_echoSuiteStepBase::CTe_echoSuiteStepBase()
	{
	}

_LIT(KEchoServerIap, "EchoDaemonIap");
_LIT(KEchoServerProtocol, "EchoDaemonProtocol");
_LIT(KEchoServerPort, "EchoDaemonPort");
_LIT(KEchoServerPacketSize, "EchoDaemonPacketSize");

_LIT(KTcp, "Tcp");
_LIT(KUdp, "Udp");
_LIT(KDummyProtocolName,"Dummy");

void CTe_echoSuiteStepBase::ReadConfigFromIniL()
	{
	if (!GetIntFromConfig(ConfigSection(), KEchoServerIap, iIap))
		{
		User::Leave(KErrArgument);
		}

	TPtrC temp(0,NULL);
	if (!GetStringFromConfig(ConfigSection(), KEchoServerProtocol, temp))
		{
		iProtocol = KProtocolInetTcp;
		return;
		}
	else
		{
		if (temp.Compare(KTcp) == 0)
			{
			iProtocol = KProtocolInetTcp;
			}
		else if (temp.Compare(KUdp) == 0) 
			{
			iProtocol = KProtocolInetUdp;
			}
		else if (temp.Compare(KDummyProtocolName)==0)
			{
			iProtocol = KProtocolInetDummy;
			}
		else
			{
			User::Leave(KErrArgument);
			}
		}
	
	if (!GetIntFromConfig(ConfigSection(), KEchoServerPort, iPort))
		{ //set default port
			iPort=7;  //default echo server port
		}
		
	}
