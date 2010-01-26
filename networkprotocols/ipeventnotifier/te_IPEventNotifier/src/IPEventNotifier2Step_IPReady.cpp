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
// Tries to push addresses to induce Duplicate Address Detection, and catch the result
// 
//

/**
 @file
 @internalComponent
*/
 

#include "IPEventNotifier2Step_IPReady.h"
#include "Te_IPEventNotifierSuiteDefs.h"
#include "ipeventtypes.h"
#include <comms-infras/netsignalevent.h>
#include "in6_opt.h"

	
CIPEventNotifier2Step_IPReady::CIPEventNotifier2Step_IPReady()
	{
	SetTestStepName(KIPEventNotifier2Step_IPReady);
	}
	
	
TVerdict CIPEventNotifier2Step_IPReady::doTestStepL()
	{
    SetTestStepResult(EFail);	
    _LIT(KReadingConfig,"CIPEventNotifier2Step_IPReady::doTestStepL() reading config..");
	INFO_PRINTF1(KReadingConfig);
	TPtrC testAddr;
	if(!GetStringFromConfig(ConfigSection(),KTe_IPENAddressToPush, testAddr) ||
	   !GetBoolFromConfig(ConfigSection(),KTe_IPENExpectedReady,iExpectedReady) ||
	   !GetStringFromConfig(ConfigSection(),KTe_IPENNetworkInterfacePrefixToMonitor, iNetworkInterfacePrefixToMonitor))
	    {
		User::Leave(KErrNotFound);
		}
		
	_LIT(KAddrToPush,"The address to be pushed is %S");
	INFO_PRINTF2(KAddrToPush, &testAddr);
	
	
	if ( iTestAddr.Input(testAddr) != KErrNone )
		{
		_LIT(KUnparsable,"Unparsable address in config!");
		INFO_PRINTF1(KUnparsable);
		User::Leave(KErrArgument);
		}
		
	iTestAddr.ConvertToV4Mapped();
		

	_LIT(KSubscribing,"Subscribing..");
	INFO_PRINTF1(KSubscribing);
	TName fullName = GetFullInterfaceNameL(iNetworkInterfacePrefixToMonitor);
	NetSubscribe::TEvent event( this, SignalHandler );
	CDhcpSignal::SubscribeL( fullName, IPEvent::EIPReady,event );
	_LIT(KGettingDetails,"OK. Getting interface details..");
	INFO_PRINTF1(KGettingDetails);


	// Now push the address.. Once we start the scheduler, the subscriber
	//  should pick it up immediately!

	TPckgBuf<TSoInet6InterfaceInfo> originalCfg;

	const TSoInet6InterfaceInfo& originalCfgRef = originalCfg(); (void)originalCfgRef; // for debugging

	GetInterfaceInfoL(fullName,originalCfg);

	_LIT(KChangingAddress,"OK. Changing address..");
	INFO_PRINTF1(KChangingAddress);

	TInt err;

	TPckgBuf<TSoInet6InterfaceInfo> changeConfig(originalCfg());
	
    changeConfig().iState    = EIfUp;
    changeConfig().iDoState  = ETrue;
    changeConfig().iDoId     = ETrue;
    changeConfig().iDoPrefix = ETrue;
    changeConfig().iAlias    = EFalse; // replace existing address
    changeConfig().iDelete   = ETrue;
	changeConfig().iDoAnycast= EFalse;
	changeConfig().iDoProxy  = EFalse;
	

	TPckgBuf<TSoInet6InterfaceInfo> changeToNew(changeConfig());
    changeToNew().iAlias    = EFalse; // replace with new
	changeToNew().iDelete   = EFalse;
    changeToNew().iAddress  = iTestAddr;
	changeToNew().iAddress.SetScope(0xffffffff);
	err = iSocket->SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, changeToNew);
	User::LeaveIfError(err);

	_LIT(KWaitingForNotification,"OK. Waiting for notification..");
	INFO_PRINTF1(KWaitingForNotification);




	CTimeout* timeout = CTimeout::NewLC(this);

	timeout->After(KTenSecondDADCompletionDelay);

	TRAPD(res,CActiveScheduler::Start())
	if(res != KErrNone)
		{
		if(res == KErrTimedOut)
			{
			_LIT(KTimedOut,"Timed out! Subscriber isn't working");
			INFO_PRINTF1(KTimedOut);
			}
		else
			{
			_LIT(KUnexpectedErrCode,"Received unexpected error code: %d");
			INFO_PRINTF2(KUnexpectedErrCode,res);
			}
		return TestStepResult();
		}

	_LIT(KRestoringAddress,"OK. Restoring address to original..");
	INFO_PRINTF1(KRestoringAddress);

	TPckgBuf<TSoInet6InterfaceInfo> restoreOriginal(changeConfig());
    restoreOriginal().iAlias    = EFalse; // replace with orig
	restoreOriginal().iDelete   = EFalse;	
	err = iSocket->SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, restoreOriginal);
	User::LeaveIfError(err);


	_LIT(KRestoredOk,"Restored OK. Clearing up..");
	INFO_PRINTF1(KRestoredOk);

		
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


void CIPEventNotifier2Step_IPReady::SignalHandler( TAny* aThis, const Meta::SMetaData* aData )
	{
	CIPEventNotifier2Step_IPReady* pThis = static_cast<CIPEventNotifier2Step_IPReady*>(aThis);
	const IPEvent::CIPReady* data = static_cast<const IPEvent::CIPReady*>(aData);
	const TInetAddr& addr = data->GetIPAddress();

	if(addr == pThis->iTestAddr)
		{
		if(data->GetAddressValid() == pThis->iExpectedReady)
			{
			pThis->SetTestStepResult(EPass);
			if(pThis->iExpectedReady)
				{
				_LIT(KAccepted,"Address accepted as expected.");
				pThis->INFO_PRINTF1(KAccepted);
				}
			else
				{
				_LIT(KRejected,"Duplicate address rejected as expected.");
				pThis->INFO_PRINTF1(KRejected);
				}
			CActiveScheduler::Current()->Stop();
			}
		else
			{
			if(pThis->iExpectedReady)
				{
				_LIT(KFoundDup,"Address expected ready but was found to be duplicate!");
				pThis->INFO_PRINTF1(KFoundDup);
				}
			else
				{
				_LIT(KFoundFree,"Address expected duplicate but was actually free!");
				pThis->INFO_PRINTF1(KFoundFree);
				}
			CActiveScheduler::Current()->Halt(KErrAbort);
			}
		}
	}


