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
// Tries to receive the O flag from an ip6 network
// 
//

/**
 @file
 @internalComponent
*/
 

#include "IPEventNotifier4Step_ReceiveOFlag.h"
#include "Te_IPEventNotifierSuiteDefs.h"
#include "ipeventtypes.h"
#include <comms-infras/netsignalevent.h>


CIPEventNotifier4Step_ReceiveOFlag::CIPEventNotifier4Step_ReceiveOFlag()
	{
	SetTestStepName(KIPEventNotifier4Step_ReceiveOFlag);
	}

TVerdict CIPEventNotifier4Step_ReceiveOFlag::doTestStepL()
	{
    SetTestStepResult(EFail);	
              
    //read from the config file.          
    _LIT(KReadingConfig,"CIPEventNotifier4Step_ReceiveOFlag::doTestStepL() reading config..");
	INFO_PRINTF1(KReadingConfig);
	if(!GetBoolFromConfig(ConfigSection(),KTe_IPENOFlagExpectedResultBool,iExpectedOFlagValue) ||
	   !GetStringFromConfig(ConfigSection(),KTe_IPENNetworkInterfacePrefixToMonitor, iNetworkInterfacePrefixToMonitor))
	    {
		User::Leave(KErrNotFound);
		}
		
	_LIT(KShowSettings,"The expected O flag is %d, on 1st interface starting with %S");
	INFO_PRINTF3(KShowSettings,iExpectedOFlagValue,&iNetworkInterfacePrefixToMonitor);
						


	_LIT(KGetIfName,"Getting full interface name..");
	INFO_PRINTF1(KGetIfName);
	TName fullName = GetFullInterfaceNameL(iNetworkInterfacePrefixToMonitor);
	_LIT(KDeclEv,"OK. declaring event..");
	INFO_PRINTF1(KDeclEv);
	NetSubscribe::TEvent event( this, SignalHandler );
	_LIT(KSubscribing,"OK. Subscribing..");
	//subscribe the O flag
	INFO_PRINTF1(KSubscribing);
	CDhcpSignal::SubscribeL( fullName, IPEvent::EMFlagReceived, event );
	_LIT(KOkWaiting,"OK. Waiting for O flag..");
	INFO_PRINTF1(KOkWaiting);


	CTimeout *timeout = CTimeout::NewLC(this);

	// a Router Advertisement is set to go out every 3 seconds on the test network
	//
	timeout->After(KFiveSecondDelayToCatchRouterAdvertisementEveryThreeSeconds);

	TRAPD(res,CActiveScheduler::Start())
	if(res != KErrNone)
		{
		if(res == KErrTimedOut)
			{
			_LIT(KTimedOut,"Timed out! Subscriber isn't working or network isn't publishing Router Advertisements");
			INFO_PRINTF1(KTimedOut);
			}
		else
			{
			_LIT(KUnexpectedErrCode,"Received unexpected error code: %d");
			INFO_PRINTF2(KUnexpectedErrCode,res);
			}
		SetTestStepResult(EFail);
		}
	
	_LIT(KOkTidyingUp,"OK. Tidying up..");
	INFO_PRINTF1(KOkTidyingUp);

	timeout->Cancel();
	CleanupStack::PopAndDestroy(timeout);
	if (iQuery && iQuery->iHandle)
		{
		CDhcpSignal::UnsubscribeL( event );
		}

	_LIT(KOkDone,"OK. Done.");
	INFO_PRINTF1(KOkDone);
	return TestStepResult();
	}

	

void CIPEventNotifier4Step_ReceiveOFlag::SignalHandler( TAny* aThis, const Meta::SMetaData* aData )
	{
	CIPEventNotifier4Step_ReceiveOFlag* pThis = static_cast<CIPEventNotifier4Step_ReceiveOFlag*>(aThis);
	//get the O flag value.
	TInt mFlag = static_cast<const IPEvent::CMFlagReceived*>(aData)->GetOFlag();

	if(mFlag == pThis->iExpectedOFlagValue)
		{
		pThis->SetTestStepResult(EPass);
		_LIT(KExpectedRcvd,"Expected value received from network");
		pThis->INFO_PRINTF1(KExpectedRcvd);
		CActiveScheduler::Current()->Stop();
		}
	else
		{
		_LIT(KUnexpectedVal,"Unexpected value recieved!");
		pThis->INFO_PRINTF1(KUnexpectedVal);
		CActiveScheduler::Current()->Halt(KErrAbort);
		}
	}

