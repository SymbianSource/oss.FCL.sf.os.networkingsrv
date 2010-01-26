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
// Defines the interface for CPppLoopbackTestStepBase class 
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __LOOPBACKTESTSTEPBASE_H__
#define __LOOPBACKTESTSTEPBASE_H__

#include <test/testexecutestepbase.h>
#include "te_ppploopbacksvr.h"

#include "pppendpoint.h"
#include "pppendpointimpl.h"
#include "timeouttimer.h"



namespace te_ppploopback
{
/**
 Defines a abstract base for RConnection based ppp loopback testing.
 Includes basic functionality necessary for PPP loopback testing.
 Concrete classes should inherit this functioality, defining only a testing FSM.
 implements listener to process PPP endpoint events using a simple FSM mechanism
 
 @internalComponent
 @test
 */
class CLoopbackTestStepBase : public CTestStep, public MPppEndpointListener
	{
public:
	CLoopbackTestStepBase();
	~CLoopbackTestStepBase();
	
	// MPppEndpointListener implementation
	void OnEvent(
		MPppEndpointListener::EEndpointId aId, 
		MPppEndpointListener::EEventId aEvent,
		TInt aError);	
	
// methods
protected:	
	TBool MessageExchangeIsCorrect
		(
	 	CPppEndpointImpl* aRcvEndpoint,
	 	const TDesC& aMsgSent,
	 	const TDesC& aSndIpAddr
	 	);	
	
	void LoadEndpointConfig(TPtrC configParams[], TInt& aIapId, TBufC<15>& aIpAddr);
	void WriteIniFileL(TPtrC configParams[]);
	void ConfigurePppServerL();
	void ConfigurePppClientL();	
	
public:	
	virtual void OnServerLinkUpL( TInt aErrorCode);
	virtual void OnServerLinkDown(TInt aErrorCode);
	virtual void OnServerSend(    TInt aErrorCode);
	virtual void OnServerRecvL(   TInt aErrorCode);	
	
	virtual void OnClientLinkUpL( TInt aErrorCode);
	virtual void OnClientLinkDown(TInt aErrorCode);
	virtual void OnClientSend(    TInt aErrorCode);
	virtual void OnClientRecvL(   TInt aErrorCode);	
	
	virtual void OnClientLinkUpNoMessage(TInt aErrorCode);
	virtual void OnServerLinkUpNoMessage(TInt aErrorCode);
	virtual void OnServerLinkDownNoMessageL(TInt aErrorCode);
	
	virtual void OnTimerEvent(TInt aErrorCode);	

protected:		
	virtual void InitPppServerL();
	virtual void ShutdownAndDestroyPppServerL();
	virtual void PutPppServerInIdleMode();

	
	virtual void InitPppClientL();
	virtual void ShutdownAndDestroyPppClientL();
	
	virtual void NotifyMessageExchangeCorrectL();
	
	void RemoveActiveSchedL();
	void InstallActiveSchedLC();
	
	void InitMessageExchangeL();
	
	void SetupForMessageExchange();
	void SetupForNoMessageExchange();
	
	TBool CheckDnsAddr(TUint32 aClientDnsAddr, TPtrC aCorrectDnsAddr);
	TBool CheckDnsAddrsAssignmentL();
	
	TVerdict doTestStepPreambleL();
	
// data
protected:
	typedef void (CLoopbackTestStepBase::*PHandler)(TInt);
	enum HandlerConstants {EEventSources = 4, EEventTypes = 6};
	
	PHandler iHandlers[EEventSources][EEventTypes]; 
		
	enum BufferSizes
		{
		EMessageBufLen = 6, 
		EIpAddBufLen = 15
		};	
	
	CPppEndpointImpl* iServer;		
	TInt iSvrIapId;
	TBufC<EIpAddBufLen> iSvrIpAddr;
	
	
	CPppEndpointImpl* iClient;
	TInt iClIapId;
	TBufC<EIpAddBufLen> iClIpAddr;		
	
	CActiveScheduler* iOldSched;
	CActiveScheduler* iStepSched;	
		
	/** Message to be exchanged by the peers.
	*/
	//HBufC* iMessage;
	TBufC<EMessageBufLen> iMessage;	
	
	TBool iMessageExchangeOk;
	TBool iAtFirstExchange;
	
	/** Client PPP link shutdown error status */
	TInt iClLinkDownErr;
	
	/** Server PPP link shutdown error status */
	TInt iSvrLinkDownErr;	
	
	/** DNS address assigned correctly */
	TBool iDnsAddrsAssignmentOk;
	
	/** At least one PPP endpoint is up */
	TBool iPeerIsUp;
	};
	
_LIT(KMessage, "Hello\0"); // Message to be exchanged.	

	
} // namespace te_ppploopback

#endif
