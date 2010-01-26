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
// Implementation of CLoopbackTestStepBase class
// 
//

/**
 @file 
 @internalComponent
*/
#include "loopbackteststepbase.h"
#include "inifileconfigurator.h"
#include <test/testexecutelog.h>
#include <c32comm.h> // call to StartC32WithCMISuppressions'

using namespace te_ppploopback;

namespace 
	{		
	// test configuration ini file entries.	
	_LIT(KClientIniFilePath,        "c:\\private\\101F7989\\ESock\\ppp.ini");
	_LIT(KClientIapId,              "ClientIAPID");
	_LIT(KClientIpAddr ,            "ClientIPAddress");
	_LIT(KClientPppMaxFailureCount ,"ClientPPPMaxFailureCount");
	_LIT(KClientPppMaxRestartCount ,"ClientPPPMaxRestartCount");
	_LIT(KClientPppRestartPeriod ,  "ClientPPPRestartPeriod");
	_LIT(KClientPppLrdPeriod,       "ClientPPPLRDPeriod");
	
	_LIT(KServerIniFilePath,        "c:\\private\\101F7989\\ESock\\pppd.ini");

	_LIT(KServerIapId ,             "ServerIAPID");
	_LIT(KServerIpAddr ,            "ServerIPAddress");
	_LIT(KServerPppMaxFailureCount ,"ServerPPPMaxFailureCount");
	_LIT(KServerPppMaxRestartCount ,"ServerPPPMaxRestartCount");
	_LIT(KServerPppRestartPeriod ,  "ServerPPPRestartPeriod");
	_LIT(KServerPppLrdPeriod ,      "ServerPPPLRDPeriod");
	
	TPtrC clConfigParams[] =
		{
		TPtrC(KClientIniFilePath),
		TPtrC(KClientIapId),
		TPtrC(KClientIpAddr),
		TPtrC(KClientPppMaxFailureCount),
		TPtrC(KClientPppMaxRestartCount),
		TPtrC(KClientPppRestartPeriod),
		TPtrC(KClientPppLrdPeriod)		
		};
		
	TPtrC svrConfigParams[] =
		{
		TPtrC(KServerIniFilePath),
		TPtrC(KServerIapId),
		TPtrC(KServerIpAddr),
		TPtrC(KServerPppMaxFailureCount),
		TPtrC(KServerPppMaxRestartCount),
		TPtrC(KServerPppRestartPeriod),
		TPtrC(KServerPppLrdPeriod)		
		};
		
	enum ConfigParameter
		{
		EIniFileName,
		EIapId,
		EIpAddr,
		EPppMaxFailureCount,
		EPppMaxRestartCount,
		EPppRestartPeriod,
		EPppLrdPeriod		
		};		
		
	_LIT(KMicrosecsIdleTimeout, "MicrosecsIdleTimeout");	// Config file access	
	
	// Necessary for RConnection/RSocket based testing.
	// Not moved into CPppEndpointImpl in case they need to be configured at runtime
	// based on ini file.
	const TInt KClPort  = 2030;	
	const TInt KSvrPort = 2029;
		
	_LIT(KLcpSection, "[lcp]");	
	_LIT(KLrdSection, "[lrd]");
	}

/**
Requests to launch the TCP/IP message exchange sequence over the PPP link.
The sequence is actually launched only when PPP link is up.


@post Underlying resources are released.
*/
void CLoopbackTestStepBase::InitMessageExchangeL()
	{	
	if(iPeerIsUp)
		{
		iServer->OpenCommChannelL();
		iServer->RequestMessageL();
		iClient->OpenCommChannelL();
		iClient->SendMessageL(iMessage);
		}
		
	// If we are here, then our end of PPP link is Up. i.e, as far as the other 
	// endpoint is concerned, Peer is UP.
	iPeerIsUp = ETrue;
	}

/**
Setup the event handlers to NOT execute message exchange
*/
void CLoopbackTestStepBase::SetupForNoMessageExchange()
	{
	iHandlers[EClient][ELinkUp]   = &CLoopbackTestStepBase::OnClientLinkUpNoMessage;
	iHandlers[EServer][ELinkUp]   = &CLoopbackTestStepBase::OnServerLinkUpNoMessage;
	iHandlers[EServer][ELinkDown] = &CLoopbackTestStepBase::OnServerLinkDownNoMessageL;
	}

/**
Setup the event handlers to execute message exchange
*/	
void CLoopbackTestStepBase::SetupForMessageExchange()
	{
	iHandlers[EClient][ELinkUp]   = &CLoopbackTestStepBase::OnClientLinkUpL;
	iHandlers[EServer][ELinkUp]   = &CLoopbackTestStepBase::OnServerLinkUpL;
	iHandlers[EServer][ELinkDown] = &CLoopbackTestStepBase::OnServerLinkDown;
	}


/**
C++ constructor

*/
CLoopbackTestStepBase::CLoopbackTestStepBase():
	iServer(NULL),
	iClient(NULL),
	iOldSched(NULL),
	iStepSched(NULL),
	iMessageExchangeOk(EFalse),
	iAtFirstExchange(ETrue),
	iClLinkDownErr(KErrNone),
	iSvrLinkDownErr(KErrNone),
	iDnsAddrsAssignmentOk(EFalse),
	iPeerIsUp(EFalse)	
	{	
	iHandlers[EServer][ELinkUp]   = &CLoopbackTestStepBase::OnServerLinkUpL;	
	iHandlers[EServer][ELinkDown] = &CLoopbackTestStepBase::OnServerLinkDown;
	
	iHandlers[EServer][ESend]     = &CLoopbackTestStepBase::OnServerSend;
	iHandlers[EServer][ERecv]     = &CLoopbackTestStepBase::OnServerRecvL;	
		
	iHandlers[EClient][ELinkUp]   = &CLoopbackTestStepBase::OnClientLinkUpL;
	iHandlers[EClient][ELinkDown] = &CLoopbackTestStepBase::OnClientLinkDown;
	
	iHandlers[EClient][ESend]     = &CLoopbackTestStepBase::OnClientSend;
	iHandlers[EClient][ERecv]     = &CLoopbackTestStepBase::OnClientRecvL;	
	
	iHandlers[ETimeoutTimer][ETimeout] = &CLoopbackTestStepBase::OnTimerEvent;
	
	iMessage = KMessage;
	}



/**
C++ destructor


@post Underlying resources are released.
*/
CLoopbackTestStepBase::~CLoopbackTestStepBase()
	{
	if(iServer) delete iServer;
	if(iClient) delete iClient;
	iOldSched  = NULL;
	iStepSched = NULL;
	}
	
/**
 Removes the active scheduler associated with asynchronous PPP endpoints.
 
 @leave when AS causes a Leave
 @pre The scheduler is installed.
 @post The scheduler is uninstalled. Original scheduler is reinstalled.
 */
void CLoopbackTestStepBase::RemoveActiveSchedL()	
	{
	CActiveScheduler::Install(iOldSched);
	CleanupStack::PopAndDestroy(iStepSched);
	}

/**
 Installs the active scheduler associated with asynchronous PPP endpoints.
 
 @leave when AS causes a Leave
 
 @post The AS is installed.
 */
void CLoopbackTestStepBase::InstallActiveSchedLC()
	{
	iOldSched = CActiveScheduler::Current();
	iStepSched = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(iStepSched);
	CActiveScheduler::Install(iStepSched);	
	}	


/**
 Writes PPP ini files based on configuration provided in the test .ini files.
  

 @param configParams Configuration Parameters, either client or server.
 @leave There was a problem writing to the ini files.
 */
void CLoopbackTestStepBase::WriteIniFileL(TPtrC configParams[])
	{
	__UHEAP_MARK;

	TPtrC iniFileName(configParams[EIniFileName]);
	CIniFileConfigurator* iniFileConfig = CIniFileConfigurator::NewLC(iniFileName);
	
	// LCP ---------------------------------------------------------
	iniFileConfig->CreateFileSectionL(KLcpSection);
	
	iniFileConfig->EnableMaxFailureL();	
	TPtrC maxFailureCount; 
	GetStringFromConfig(ConfigSection(),configParams[EPppMaxFailureCount], maxFailureCount);
	iniFileConfig->SetMaxFailureCountL(maxFailureCount);
	
	iniFileConfig->EnableMaxRestartL();	
	TPtrC maxRestartCount;
	GetStringFromConfig(ConfigSection(),configParams[EPppMaxRestartCount], maxRestartCount);
	iniFileConfig->SetMaxRestarteCountL(maxRestartCount);
	
	iniFileConfig->EnableRestartTimerL();	
	TPtrC maxRestartPeriod;
	GetStringFromConfig(ConfigSection(),configParams[EPppRestartPeriod], maxRestartPeriod);
	iniFileConfig->SetMaxRestartPeriodL(maxRestartPeriod);
	
	// LCP Termination: 
	// Enabled by default.
	// Use default (RFC) values for counters and timers.
	
	// LRD ---------------------------------------------------------
	iniFileConfig->CreateFileSectionL(KLrdSection);
	iniFileConfig->EnableLrdL();	
	TPtrC maxLrdPeriod;
	GetStringFromConfig(ConfigSection(),configParams[EPppLrdPeriod], maxLrdPeriod);
	iniFileConfig->SetLrdPeriodL(maxLrdPeriod);	

	CleanupStack::PopAndDestroy(iniFileConfig);	
	
	__UHEAP_MARKEND;
	}



/**
 Loads high level endpoint configuration
 Does NOT attempt to create a connection.
  
 @internalComponent
 @leave There was a problem creating ppp ini files.
 */
void CLoopbackTestStepBase::LoadEndpointConfig(TPtrC configParams[], TInt& aIapId, TBufC<15>& aIpAddr)
	{
	GetIntFromConfig(ConfigSection(),configParams[EIapId], aIapId);
	
	TPtrC ipAddrPtr;
	GetStringFromConfig(ConfigSection(), configParams[EIpAddr], ipAddrPtr);
	aIpAddr = ipAddrPtr;		
	}


/**
 Configures the PPP Server with test step specific parameters.
 Does NOT attempt to create a connection.
  
 @leave There was a problem creating ppp ini files.
 
 @post Endpoints are configured and ready for connection.
 */
void CLoopbackTestStepBase::ConfigurePppServerL()
	{	
	WriteIniFileL(svrConfigParams);		// pppd.ini
	LoadEndpointConfig(svrConfigParams, iSvrIapId,iSvrIpAddr);
	}
/**
 Configures the PPP Client with test step specific parameters.
 Does NOT attempt to create a connection.
  

 @leave There was a problem creating ppp ini files.
 
 @post Endpoints are configured and ready for connection.
 */	
void CLoopbackTestStepBase::ConfigurePppClientL()
	{
	WriteIniFileL(clConfigParams); 
	LoadEndpointConfig(clConfigParams, iClIapId, iClIpAddr);
	}


/**
Verifies that the last message exchange is correct by
checking the message and its source IP address


@param aRcvEndpoint the receiving endpoint
@param aMsgSent the message that was sent
@param aSndIpAddr sender's IP address.
@return ETrue if OK, EFalse otherwise.
*/
TBool CLoopbackTestStepBase::MessageExchangeIsCorrect
	(CPppEndpointImpl* aRcvEndpoint,
	 const TDesC& aMsgSent,
	 const TDesC& aSndIpAddr	 
	)
	{
	
	HBufC16* receivedMsg = aRcvEndpoint->GetDataIn();
	
	INFO_PRINTF3(_L("Message: sent= \"%s\", received =\"%s\""), aMsgSent.Ptr(), receivedMsg->Des().Ptr());
	
	TInt msgDiff = aMsgSent.Compare(*receivedMsg);
	delete receivedMsg;
	receivedMsg = NULL;
	
	TInetAddr sndAddr;
	sndAddr.Input(aSndIpAddr);
	TInetAddr msgSourceAddr = aRcvEndpoint->GetSourceInetAddr();
	
			
	return 
		(
		msgDiff == 0 &&
		msgSourceAddr.Match(sndAddr) // source address is of the actual sender
		);			
	}

/**
 MPppEndpointListener implementation
 Dispatches event to concrete handlers.
  
 @post Event is dispatched and handled.
 */
void CLoopbackTestStepBase::OnEvent(
		MPppEndpointListener::EEndpointId aSourceId, 
		MPppEndpointListener::EEventId aEventId,
		TInt aError)
	{
	ASSERT(aSourceId >= EMinEndpointId && aSourceId <= EMaxEndpointId);
	ASSERT(aEventId  >= EMinEventId    && aEventId  <= EMaxEventId);
	
	(this->*iHandlers[aSourceId][aEventId])(aError);	
	}
	
/**
Processes a Server Send event


@param aErrorCode error code associated with the event

@post None
*/
void CLoopbackTestStepBase::OnServerSend(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("OnServerSend error= %d"), aErrorCode);
	if(aErrorCode != KErrNone)
		{
		SetTestStepResult(EFail);
		}	
	}


	
/**
Processes a Server Receive event


@param aErrorCode error code associated with the event

@post None
*/	
void CLoopbackTestStepBase::OnServerRecvL(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("OnServerRecv error= %d"), aErrorCode);
	if(aErrorCode == KErrNone)
		{
		if(MessageExchangeIsCorrect(iServer, iMessage, iClIpAddr)) 
			{
			NotifyMessageExchangeCorrectL();
			}
		}
	}
	


/**
Processes a Client Send event


@param aErrorCode error code associated with the event

@post None
*/		
void CLoopbackTestStepBase::OnClientSend(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("OnClientSend error= %d"), aErrorCode);
	if(aErrorCode != KErrNone)
		{
		SetTestStepResult(EFail);
		}	
	}
/**
Processes a Client Receive event


@param aErrorCode error code associated with the event

*/		
void CLoopbackTestStepBase::OnClientRecvL(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("OnClientRecv error= %d"), aErrorCode);
	if(aErrorCode == KErrNone)
		{
		if(MessageExchangeIsCorrect(iClient, iMessage, iSvrIpAddr)) 
			{
			NotifyMessageExchangeCorrectL();			
			}
		}
	}
	
/**
Processes a timeout
A placeholder, so that classes that don't one need won't have
to implement it.

*/	
void CLoopbackTestStepBase::OnTimerEvent(TInt /*aErrorCode*/ )
	{	
	}
	

/**
 Initializes PPP server side endpoint
 Does not create PPP link

 
 @post endpoint initialized
 */
void CLoopbackTestStepBase::InitPppServerL()
	{
	iServer = CPppEndpointImpl::NewL(iSvrIapId, iSvrIpAddr.Des(), KSvrPort, iClIpAddr.Des(), KClPort);	
	iServer->SetObserver(this, MPppEndpointListener::EServer);
	}



/**
 Makes sure PPP server is in idle mode
 

 @pre server endpoint exists and has issued a connection request.
 @post the server is in idle mode.
 */
void CLoopbackTestStepBase::PutPppServerInIdleMode()
	{
	TInt microsecsIdleTimeout = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsIdleTimeout, microsecsIdleTimeout);
	User::After(microsecsIdleTimeout);
	}

/**
 Initializes PPP client side endpoint
 Does not create PPP link

 
 @post endpoint initialized
 */	
void CLoopbackTestStepBase::InitPppClientL()
	{
	iClient = CPppEndpointImpl::NewL(iClIapId, iClIpAddr.Des(), KClPort, iSvrIpAddr.Des(), KSvrPort);
	iClient->SetObserver(this, MPppEndpointListener::EClient);		
	INFO_PRINTF2(_L("Client %x initialized"), iClient);
	}
	
/**
 Release client-side resources. Does not affect PPP 
 (PPP link must be closed by now) 

 @pre PPP link is down
 @post resources have been released. 
 */		
void CLoopbackTestStepBase::ShutdownAndDestroyPppClientL()
	{
	iClient->Cancel();	
	
	INFO_PRINTF2(_L("Destroying client %x "), iClient);
	
	delete iClient;	
	iClient = NULL;
	}


/**
 Release server-side resources. Does not affect PPP 
 (PPP link must be closed by now)
 
 @pre PPP link is down
 @post resources have been released. 
 */		
void CLoopbackTestStepBase::ShutdownAndDestroyPppServerL()
	{
	iServer->Cancel();
	
	INFO_PRINTF2(_L("Destroying server %x "), iServer);
	
	delete iServer;	
	iServer = NULL;
	}

	
/**
Processes a Server PPP link up event


@param aErrorCode error code associated with the event
*/	
void CLoopbackTestStepBase::OnServerLinkUpL(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("Server Link Up error=%d"), aErrorCode);
	InitMessageExchangeL();
	}
	
/**
Processes a Server PPP link down event


@param aErrorCode error code associated with the event
*/	
void CLoopbackTestStepBase::OnServerLinkDown(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("Server link down event error=%d"),aErrorCode);	
	if(aErrorCode == KErrNone)
		{
		TRAP(iSvrLinkDownErr, iServer->DisconnectFromPeerL()); // Must be closed, otherwise
															   // it is still connected to Peer.
		
		TBool clientDisconnectionOk = !(iClient->IsConnectedToPeer());		
		
		INFO_PRINTF3(_L("Client link down error=%d. Server link down error=%d (error is ok)"), iClLinkDownErr, iSvrLinkDownErr);	
		INFO_PRINTF4(_L("final (1 == ETrue, 0 == EFalse): client disconnected ok= %d message exchange + IP addr check ok= %d DNS addr check ok= %d:"), clientDisconnectionOk, iMessageExchangeOk, iDnsAddrsAssignmentOk );
		if(	
			iMessageExchangeOk          &&
			iDnsAddrsAssignmentOk		&&
			iClLinkDownErr  == KErrNone &&
			clientDisconnectionOk       
	 	)	   
			{		
			SetTestStepResult(EPass);
			}
		}	
	iPeerIsUp = EFalse;
	CActiveScheduler::Stop();	
	}


/**
Processes a Client PPP link up event

@param aErrorCode error code associated with the event
*/	
void CLoopbackTestStepBase::OnClientLinkUpL(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("Client Link Up error=%d"), aErrorCode);
	InitMessageExchangeL();
	}
	
/**
Processes a Client PPP link down event

@param aErrorCode error code associated with the event
*/	
void CLoopbackTestStepBase::OnClientLinkDown(TInt /*aErrorCode*/)
	{	
	}

/**
Processes a notification that a single message exchange is correct.

*/
void CLoopbackTestStepBase::NotifyMessageExchangeCorrectL()
	{
	// state machine to handle 2 exchanges. If the first one fails, 
	// the second one does not take place.
	
	if(iAtFirstExchange) // First exchange is correct
		{
		iClient->RequestMessageL();	
		iServer->SendMessageL(iMessage);
		iAtFirstExchange = EFalse;
		}
	else // Second exchange, and it's correct too.
		{
		iMessageExchangeOk = ETrue;
		iAtFirstExchange = ETrue;	
		
		
		// Check DNS IP address assignment:
		// The server got DNS from client, so we only check the server.
		iDnsAddrsAssignmentOk = CheckDnsAddrsAssignmentL();		
		
		// We must close the channels before PPP link is down,
		// otherwise we get an error.
		iClient->CloseCommChannel();
		iServer->CloseCommChannel();
		
		// Close PPP link. 
		// TRAP is necessary because we do not want the test to leave,
		// Leaving at this point is test failure and must be recorded as EFail
		
		TRAP(iClLinkDownErr, iClient->DisconnectFromPeerL());
		}		
	}

/**
 Called when Client Link is Up, is situtation where we do not need 
 to exchange messages. Therefore, it does not initialiaze anything.
 
 @param aErrorCode associated with Link UP event.
 @internalComponent
 */
void CLoopbackTestStepBase::OnClientLinkUpNoMessage(TInt aErrorCode)
	{
	// don't do anything. We are just making sure no messages
	// are exchanged.
	INFO_PRINTF2(_L("Client Link Up event error=%d "), aErrorCode);
	}
/**
 Called when Server Link is Up, is situtation where we do not need 
 to exchange messages. Therefore, it does not initialiaze anything.
 
 @param aErrorCode associated with Link UP event.
 */	
void CLoopbackTestStepBase::OnServerLinkUpNoMessage(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("Server Link Up event error=%d "), aErrorCode);
	}
/**
 Called when Server Link is Down, is situtation where we do not need 
 to exchange messages. Therefore, it does not check message exchange correctness.
 
 @param aErrorCode associated with Link UP event.
 */	
void CLoopbackTestStepBase::OnServerLinkDownNoMessageL(TInt aErrorCode)
	{
	INFO_PRINTF2(_L("Server Link Down event error=%d "), aErrorCode);
	TRAPD(err, iServer->DisconnectFromPeerL());
	INFO_PRINTF2(_L("Server disconnected with error= %d (error is OK)"), err);
	CActiveScheduler::Stop();
	}
	
/**
 Checks that Client DNS address is correct
 
 @param aClientDnsAddr DNS address reported by client
 @param aCorrectDnsAddr DNS address that should be assigned to the client
 @return ETrue if DNS address reported by client == DNS address that should have been assigned
 */	
TBool CLoopbackTestStepBase::CheckDnsAddr(TUint32 aClientDnsAddr, TPtrC aCorrectDnsAddr)
	{
	TInetAddr correctDnsAddr;
	correctDnsAddr.Input(aCorrectDnsAddr);
	
	TInetAddr clientDnsAddr;
	clientDnsAddr.SetAddress(aClientDnsAddr);
	
	const TInt KDnsAddrBufLen = 17;
	TBuf<KDnsAddrBufLen> clientDnsAddrBuf;
		
	clientDnsAddr.Output(clientDnsAddrBuf);
	clientDnsAddrBuf.ZeroTerminate();
	
	TBuf<KDnsAddrBufLen> correctDnsAddrBuf(aCorrectDnsAddr);
	correctDnsAddrBuf.ZeroTerminate();
	
	INFO_PRINTF3(_L("Verifying DNS: client ip=%s, correct ip=%s"), clientDnsAddrBuf.Ptr(), correctDnsAddrBuf.Ptr());	
	
	return correctDnsAddr.Match(clientDnsAddr);	
	}
	
/**
 Checks that Client DNS addresses have been assigned correctly.

 @return ETrue if DNS addresses have been assigned correctly
 */	
TBool CLoopbackTestStepBase::CheckDnsAddrsAssignmentL()
	{
	TBool dnsAddrsOk = EFalse;
		
	_LIT(KCorrectPrimaryDnsParam, "ClientPrimaryDNS");
	TPtrC correctPrimaryDns; 
	GetStringFromConfig(ConfigSection(),KCorrectPrimaryDnsParam, correctPrimaryDns);
	
	_LIT(KCorrectSecondaryDnsParam, "ClientSecondaryDNS");
	TPtrC correctSecondaryDns; 
	GetStringFromConfig(ConfigSection(),KCorrectSecondaryDnsParam, correctSecondaryDns);
	
	TUint32 clientPrimaryDns   = 0;
	TUint32 clientSecondaryDns = 0;	
	TRAPD(dnsErr, iClient->GetDnsAddrsL(clientPrimaryDns, clientSecondaryDns));
	if(dnsErr == KErrNone)
		{		
		dnsAddrsOk = ( 
			CheckDnsAddr(clientPrimaryDns,   correctPrimaryDns) 
			&&
			CheckDnsAddr(clientSecondaryDns, correctSecondaryDns)
			);
		}			
	else
		{
		INFO_PRINTF2(_L("DNS addr retrieval left with error=%d "), dnsErr);
		}
	
	return dnsAddrsOk;
	}

	
TVerdict CLoopbackTestStepBase::doTestStepPreambleL()
	{	
	_LIT(KPhbkSyncCMI, "phbsync.cmi");
	TInt err = StartC32WithCMISuppressions(KPhbkSyncCMI);
	TEST(err == KErrNone || err == KErrAlreadyExists);
	INFO_PRINTF1(_L("Test Step Preamble: Disabled phonebook synchronizer"));
	
	SetTestStepResult(EFail);	
	return TestStepResult();
	}
