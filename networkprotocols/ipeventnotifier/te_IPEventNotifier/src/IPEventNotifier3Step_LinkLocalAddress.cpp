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
// Pushes a LinkLocal address into an interface and tries to catch the notification about it from IPEN
// 
//

/**
 @file
 @internalComponent
*/
 

#include "IPEventNotifier3Step_LinkLocalAddress.h"
#include "Te_IPEventNotifierSuiteDefs.h"
#include "ipeventtypes.h"
#include <comms-infras/netsignalevent.h>

	
CIPEventNotifier3Step_LinkLocalAddress::CIPEventNotifier3Step_LinkLocalAddress()
	{
	SetTestStepName(KIPEventNotifier3Step_LinkLocalAddress);
	}



TVerdict CIPEventNotifier3Step_LinkLocalAddress::doTestStepL()
	{
    SetTestStepResult(EFail);
              
    _LIT(KReadingConfig,"CIPEventNotifier3Step_LinkLocalAddress::doTestStepL() reading config..");
	INFO_PRINTF1(KReadingConfig);
	TPtrC llAddr;
	if(!GetStringFromConfig(ConfigSection(),KTe_IPENAddressToPush, llAddr) ||
	   !GetStringFromConfig(ConfigSection(),KTe_IPENNetworkInterfacePrefixToMonitor, iNetworkInterfacePrefixToMonitor))
	   	{
		User::Leave(KErrNotFound);
		}
		
	_LIT(KAddrToPush,"The address to be pushed is %S");
	INFO_PRINTF2(KAddrToPush, &llAddr);

		
	if ( iLLAddr.Input(llAddr) != KErrNone )
		{
		_LIT(KUnparsable,"Unparsable address in config!");
		INFO_PRINTF1(KUnparsable);
		User::Leave(KErrArgument);
		}

	if(iLLAddr.IsLinkLocal() == false)
		{
		_LIT(KSpecifyLLAddr,"Please specify a linklocal address in config file! (169.254.x.x or fe80:...)!");
		INFO_PRINTF1(KSpecifyLLAddr);
		User::Leave(KErrArgument);
		}


	_LIT(KSubscribing,"Subscribing..");
	INFO_PRINTF1(KSubscribing);
	TName fullName = GetFullInterfaceNameL(iNetworkInterfacePrefixToMonitor);
	NetSubscribe::TEvent event( this, SignalHandler );
	CDhcpSignal::SubscribeL( fullName, IPEvent::ELinklocalAddressKnown,event );
	_LIT(KGettingDetails,"OK. Getting interface details..");
	INFO_PRINTF1(KGettingDetails);


	// Now push the address.. Once we start the scheduler, the subscriber
	//  should pick it up immediately!

	TPckgBuf<TSoInet6InterfaceInfo> originalCfg;

	const TSoInet6InterfaceInfo& originalCfgRef = originalCfg(); (void)originalCfgRef; // for debugging

	GetInterfaceInfoL(fullName,originalCfg);
	
	_LIT(KAddingLL,"OK. Adding a linklocal address..");
	INFO_PRINTF1(KAddingLL);
	
	TInt err;

	TPckgBuf<TSoInet6InterfaceInfo> addConfig(originalCfg());
	
    addConfig().iState    = EIfUp;
    addConfig().iDoState  = ETrue;
    addConfig().iDoId     = ETrue;
    addConfig().iDoPrefix = ETrue;
    addConfig().iAlias    = ETrue; // add another address
    addConfig().iDelete   = EFalse;
	addConfig().iDoAnycast= EFalse;
	addConfig().iDoProxy  = EFalse;
    addConfig().iAddress  = iLLAddr;
	addConfig().iAddress.SetScope(2);

	err = iSocket->SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, addConfig);
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


	_LIT(KRestoringAddress,"OK. Removing the address..");
	INFO_PRINTF1(KRestoringAddress);
	
	addConfig().iDelete = ETrue;
	err = iSocket->SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, addConfig);
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


void CIPEventNotifier3Step_LinkLocalAddress::SignalHandler( TAny* aThis, const Meta::SMetaData* aData )
	{
	CIPEventNotifier3Step_LinkLocalAddress* pThis = static_cast<CIPEventNotifier3Step_LinkLocalAddress*>(aThis);
	const IPEvent::CLinklocalAddressKnown* data = static_cast<const IPEvent::CLinklocalAddressKnown*>(aData);
	const TInetAddr& addr = data->GetIPAddress();

	if(addr == pThis->iLLAddr)
		{
		pThis->SetTestStepResult(EPass);
		_LIT(KExpValRecvd,"Expected linklocal value received from loopback");
		pThis->INFO_PRINTF1(KExpValRecvd);
		CActiveScheduler::Current()->Stop();
		}
	else
		{
		_LIT(KUnexpValRecvd,"Unexpected value recieved.. May not be a problem.");
		pThis->INFO_PRINTF1(KUnexpValRecvd);
		}
	}


