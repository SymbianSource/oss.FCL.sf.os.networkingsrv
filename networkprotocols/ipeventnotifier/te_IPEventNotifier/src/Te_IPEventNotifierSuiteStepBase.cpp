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


#include "Te_IPEventNotifierSuiteStepBase.h"
#include "Te_IPEventNotifierSuiteDefs.h"

#include <networking/ipeventtypes.h>
#include <nifman.h>
#include <commdbconnpref.h>
#include <comms-infras/netsignalevent.h>





// Utility classes for communicating with the RootServer (taken from DHCP).
 
CFactoryChannel::~CFactoryChannel()
	{
	if ( iC32Root.Handle() )
		{
		iC32Root.Close();
		}
	}

void CFactoryChannel::SendMessageL( NetMessages::CMessage& aQuery )
	{
	//connect to root server and find an instance
	if ( !iC32Root.Handle() )
		{
		User::LeaveIfError(iC32Root.Connect());
		}
	//send a message to find a channel instance
	const TInt nLen = aQuery.Length();
	User::LeaveIfError(nLen);
	iBuf.Close();
	iBuf.CreateL( nLen );
	TPtr8 ptr( const_cast<TUint8*>(iBuf.Ptr()), iBuf.MaxLength() );
	User::LeaveIfError( aQuery.Store( ptr ) );
	iBuf.SetLength( ptr.Length() );
	//this should be very quick so we can afford to hold different interfaces up for a while
	//we should really have a solution for servers running each session in a different thread
	User::LeaveIfError( iC32Root.SendMessage( iModule, CommsFW::EFactoryMsg, ptr ) );
	TPtrC8 ptrC( ptr );
	User::LeaveIfError( aQuery.GetTypeId().Check( ptrC ) );
	User::LeaveIfError( aQuery.Load( ptrC ) );
	}

CDhcpSignal::~CDhcpSignal()
	{
	delete iQuery;
	delete iNetSubscribe;
	}

void CDhcpSignal::SubscribeL(const TName& aInterfaceName, TInt aEventId, NetSubscribe::TEvent& aEvent )
	{
	_LIT(KModName,"ESock_IP");
	iModule.Copy( KModName );
	if ( !iQuery )
		{
		iQuery = NetMessages::CTypeIdQuery::NewL();
		iQuery->iUid = IPEvent::KFactoryImplementationUid;
		iQuery->iTypeId = IPEvent::KProtocolId;
		iQuery->iHandle = NULL;
		iQuery->iOid.Copy( aInterfaceName.Left( iQuery->iOid.MaxLength() ) );
		}
	if ( !iQuery->iHandle )
		{
		CFactoryChannel::SendMessageL( *iQuery );

		STypeId typeId = STypeId::CreateSTypeId( NetSubscribe::KTransportUid, NetSubscribe::EPublishSubscribe );
		ASSERT( !iNetSubscribe );
		iNetSubscribe = NetSubscribe::CNetSubscribe::NewL( typeId );

		NetSubscribe::SSignalId signalId( IPEvent::KEventImplementationUid, aEventId, iQuery->iHandle );
		aEvent.SubscribeL(*iNetSubscribe, signalId);
		//and wait for the signal
		}
	}

void CDhcpSignal::UnsubscribeL( NetSubscribe::TEvent& aEvent )
	{
	//deregister & release the handle
	ASSERT( iQuery );
	ASSERT( iNetSubscribe );	
	aEvent.Cancel(*iNetSubscribe);
	CFactoryChannel::SendMessageL( *iQuery );
	}

	

CTimeout* CTimeout::NewLC(CTE_IPEventNotifierSuiteStepBase * aTestStep)
	{
	CTimeout* inst = new (ELeave) CTimeout(aTestStep);
	CleanupStack::PushL(inst);
	inst->ConstructL();
	return inst;
	}
	
CTimeout::CTimeout(CTE_IPEventNotifierSuiteStepBase * aTestStep):
	CTimer(10),
	iTestStep(aTestStep)
	{
	__DECLARE_NAME(_S("CTimeout"));
	}

void CTimeout::ConstructL()
	{
	// initialise timer
	CTimer::ConstructL();
	
	// add self to scheduler
	CActiveScheduler::Add(this);
	}

void CTimeout::RunL()
//	Timer has triggered
	{
	_LIT(KTimedOut,"Timed out");
	iTestStep->INFO_PRINTF1(KTimedOut);
	CActiveScheduler::Current()->Halt(KErrTimedOut);
	}




TVerdict CTE_IPEventNotifierSuiteStepBase::doTestStepPreambleL()
	{
	_LIT(KUsingDefault,"Using default Preamble");
    INFO_PRINTF1(KUsingDefault);

	TInt IAPToUse;
	_LIT(KReadingConfig,"Reading config to find IAP number..");
	INFO_PRINTF1(KReadingConfig);
	if(!GetIntFromConfig(ConfigSection(),KTe_IPENIAPToUse,IAPToUse))
		{
		User::Leave(KErrNotFound);
		}

	_LIT(KOkIAP,"OK. IAP %d.");
	INFO_PRINTF2(KOkIAP, IAPToUse);

	StartConnectionL(IAPToUse);

	iActSched = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(iActSched);
	CActiveScheduler::Install(iActSched);

	
	return TestStepResult();
	}

TVerdict CTE_IPEventNotifierSuiteStepBase::doTestStepPostambleL()
	{
	CActiveScheduler::Install(NULL);
	CleanupStack::PopAndDestroy(iActSched);

	_LIT(KClosing,"Using default Postamble. Closing connection..");
    INFO_PRINTF1(KClosing);
	CleanupStack::PopAndDestroy(iSocket); // closes
    CleanupStack::PopAndDestroy(iSocket); // deletes
	CleanupStack::PopAndDestroy(iRConnection); // closes
    CleanupStack::PopAndDestroy(iRConnection); // deletes
	CleanupStack::PopAndDestroy(iSocketServ); // closes
    CleanupStack::PopAndDestroy(iSocketServ); // deletes
    _LIT(KOk,"OK.");
    INFO_PRINTF1(KOk);
	return TestStepResult();
	}

CTE_IPEventNotifierSuiteStepBase::~CTE_IPEventNotifierSuiteStepBase()
	{
	}

CTE_IPEventNotifierSuiteStepBase::CTE_IPEventNotifierSuiteStepBase()
	{
	}


	
	
void CTE_IPEventNotifierSuiteStepBase::StartConnectionL(TInt aIAP)
	{
	_LIT(KSockServConn,"iSocketServ.Connect..");
    INFO_PRINTF1(KSockServConn);
    iSocketServ=new RSocketServ;
    CleanupStack::PushL(iSocketServ);
	TInt err = iSocketServ->Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(*iSocketServ);

	_LIT(KRconnOpen,"OK. iRConnection.Open..");
    INFO_PRINTF1(KRconnOpen);
    iRConnection=new RConnection;
    CleanupStack::PushL(iRConnection);
	err = iRConnection->Open(*iSocketServ, KConnectionTypeDefault);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(*iRConnection);

	TCommDbConnPref commDbPref;
	commDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	commDbPref.SetIapId(aIAP);

	_LIT(KRconnStart,"OK. iRConnection.Start..");
    INFO_PRINTF1(KRconnStart);
	err = iRConnection->Start(commDbPref);
	TESTEL(err == KErrNone, err);

	_LIT(KSockOpen,"OK. iSocket.Open..");
    INFO_PRINTF1(KSockOpen);
    iSocket=new RSocket;
    CleanupStack::PushL(iSocket);
	err = iSocket->Open(*iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, *iRConnection);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(*iSocket);
    _LIT(KOk,"OK.");
    INFO_PRINTF1(KOk);
	}


const TName CTE_IPEventNotifierSuiteStepBase::GetFullInterfaceNameL(const TDesC& aNamePrefix)
/**
    Returns full name of interface that begins with aNamePrefix.
    Does this by querying the socket for its interfaces.
    Leaves if not found.
  */
	{
	TName interfaceName;
	
	_LIT(KGettingIf,"Getting interface name starting with %S ..");
	INFO_PRINTF2(KGettingIf,&aNamePrefix);

	TPckgBuf<TSoInet6InterfaceInfo> info;
	TSoInet6InterfaceInfo& infoRef = info(); (void)infoRef;  // declared for debugging
	User::LeaveIfError(iSocket->SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl, 0));
	
	TName thisInterfaceName;
	do
		{
		// leave if interface not found
		User::LeaveIfError(iSocket->GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info));
		thisInterfaceName = info().iName;
		}
	while ( thisInterfaceName.Find(aNamePrefix) != 0 );
	
	// if we reach this point we have found a matching interface.

	_LIT(KFound,"Found: %S");
	INFO_PRINTF2(KFound,&thisInterfaceName);

	return thisInterfaceName;
	}

void CTE_IPEventNotifierSuiteStepBase::GetInterfaceInfoL(const TDesC& aName, TPckgBuf<TSoInet6InterfaceInfo>& aInfo)
/**
    Finds the interface information of the named interface.
    Does this by querying the socket for its interfaces.
    Leaves if not found.
  */
	{
	_LIT(KGettingInfo,"Getting interface info for %S ..");
	INFO_PRINTF2(KGettingInfo,&aName);

	User::LeaveIfError(iSocket->SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl, 0));
	
	do
		{
		// leave if interface not found
		User::LeaveIfError(iSocket->GetOpt(KSoInetNextInterface, KSolInetIfCtrl, aInfo));
		}
	while ( aInfo().iName != aName );
	
	// if we reach this point we found the right interface and have already put its info in aInfo.
	_LIT(KFound,"Found.");
	INFO_PRINTF1(KFound);
	}
	
