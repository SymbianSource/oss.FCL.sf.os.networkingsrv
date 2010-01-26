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
// Tries to receive the M flag from an ip6 network
// 
//

/**
 @file
 @internalComponent
*/
 

#include "IPEventNotifier1Step_ReceiveMFlag.h"
#include "Te_IPEventNotifierSuiteDefs.h"
#include "ipeventtypes.h"
#include <comms-infras/netsignalevent.h>


CIPEventNotifier1Step_ReceiveMFlag::CIPEventNotifier1Step_ReceiveMFlag()
	{
	SetTestStepName(KIPEventNotifier1Step_ReceiveMFlag);
	}

TVerdict CIPEventNotifier1Step_ReceiveMFlag::doTestStepL()
	{
    SetTestStepResult(EFail);	
              
    _LIT(KReadingConfig,"CIPEventNotifier1Step_ReceiveMFlag::doTestStepL() reading config..");
	INFO_PRINTF1(KReadingConfig);
	if(!GetBoolFromConfig(ConfigSection(),KTe_IPENMFlagExpectedResultBool,iExpectedMFlagValue) ||
	   !GetStringFromConfig(ConfigSection(),KTe_IPENNetworkInterfacePrefixToMonitor, iNetworkInterfacePrefixToMonitor))
	    {
		User::Leave(KErrNotFound);
		}
		
	_LIT(KShowSettings,"The expected M flag is %d, on 1st interface starting with %S");
	INFO_PRINTF3(KShowSettings,iExpectedMFlagValue,&iNetworkInterfacePrefixToMonitor);
						


	_LIT(KGetIfName,"Getting full interface name..");
	INFO_PRINTF1(KGetIfName);
	TName fullName = GetFullInterfaceNameL(iNetworkInterfacePrefixToMonitor);
	_LIT(KDeclEv,"OK. declaring event..");
	INFO_PRINTF1(KDeclEv);
	NetSubscribe::TEvent event( this, SignalHandler );
	_LIT(KSubscribing,"OK. Subscribing..");
	INFO_PRINTF1(KSubscribing);
	CDhcpSignal::SubscribeL( fullName, IPEvent::EMFlagReceived, event );
	_LIT(KOkWaiting,"OK. Waiting for M flag..");
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

	

void CIPEventNotifier1Step_ReceiveMFlag::SignalHandler( TAny* aThis, const Meta::SMetaData* aData )
	{
	CIPEventNotifier1Step_ReceiveMFlag* pThis = static_cast<CIPEventNotifier1Step_ReceiveMFlag*>(aThis);
	TInt mFlag = static_cast<const IPEvent::CMFlagReceived*>(aData)->GetMFlag();

	if(mFlag == pThis->iExpectedMFlagValue)
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

