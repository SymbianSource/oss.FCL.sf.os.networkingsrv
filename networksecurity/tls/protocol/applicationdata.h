/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* SSL3.0 and TLS1.0 Application data header file.
* This file describes the Application data (transmission and reception)
* state machines.
* 
*
*/



/**
 @file ApplicationData.h
*/

#ifndef _APPLICATIONDATA_H_
#define _APPLICATIONDATA_H_

#include <comms-infras/statemachine.h>
#include "LOGFILE.H"
#include "tlsconnection.h"

class CRecordComposer;
class CSendAlert;
class CHelloRequest;
class CSendAppData : public CStateMachine
/** 
 * Describes a state machine which sends an Application's data to a remote server.
 */
{
public:
	static CSendAppData* NewL( CRecordComposer& aRecordComposer ); 
	~CSendAppData();
	void Start( TRequestStatus* aClientStatus, MStateMachineNotify* aStateMachineNotify );

	// These are called by CTlsConnection when a re-negotiation request comes through.
	void Suspend();
	void ResumeL();

	void SetUserData( TDesC8* aAppData );
	void SetSockXfrLength( TInt* aLen );
	CSendAlert* SendAlert() const;

	void SetHelloRequest( CHelloRequest* aHelloReq );
   
protected:
	CSendAppData( CRecordComposer& aRecordComposer );
	void ConstructL( CRecordComposer& aRecordComposer );

	virtual void DoCancel();
	virtual void OnCompletion();

protected:
	TDesC8* iAppData;	// Keeps app buffer to send data from during re-negotiation
	TInt  iCurrentPos;	// Keeps CRecordComposer::iCurrentPos during re-negotiation
	TInt* iSockXfrLength;
	CRecordComposer& iRecordComposer;
	CSendAlert* iSendAlert;
	CHelloRequest* iHelloReq; //to check whether hello req's been received and is waiting for
	//a record to be sent to start renegotiation - reference only no ownership
};


/////////////////////////////////////////////////////////////////////////////////////////////


class CRecordParser;
class CTlsConnection;
class CRecvAppData : public CStateMachine
/** 
 * Describes a state machine which receives data from a 
 * remote server, intended for an Application using a Secure socket.
 */
{
public:
	static CRecvAppData* NewL( CTlsConnection& aTlsConnection ); 
	~CRecvAppData();
	void Start( TRequestStatus* aClientStatus, MStateMachineNotify* aStateMachineNotify );

	// These are called by CTlsConnection when a re-negotiation request comes through
	void Suspend();
	void ResumeL( CTlsConnection& aTlsConnection );

	CSendAlert* SendAlert() const;
	void SetSockXfrLength( TInt* aLen );

	CHelloRequest* HelloRequest() const;

protected:
	CRecvAppData( CTlsConnection& aTlsConnection );
	void ConstructL( CTlsConnection& aTlsConnection );

	virtual void DoCancel();
	virtual void OnCompletion();

protected:
	TPtr8 iHeldData;	// Keeps CRecordParser::iHeldData (ptr in CStateMachine::iFragment)
						// during re-negotiation (marks a point in CStateMachine::iFragment)
	TDes8* iAppData;	// Keeps app buffer to receive data into during re-negotiation
	TInt* iSockXfrLength;

	CRecordParser& iRecordParser;
	CSendAlert* iSendAlert;
	CHelloRequest* iHelloReq; //to check whether hello req's been received and is waiting for
	//a record to be sent to start renegotiation - reference only no ownership
};



// Inline methods - CSendAppData class
inline void CSendAppData::Start( TRequestStatus* aClientStatus, MStateMachineNotify* aStateMachineNotify )
/**
 * Starts the 'Send Application data' state machine by calling the 
 * Start() method of the base state machine class (CStateMachine).
 *
 * @param aClientStatus Pointer to a TRequestStatus object that completes when  
 * data transmission is complete.
 * @param aStateMachineNotify Pointer to a MStateMachineNotify interface object.
 */
{
	LOG(Log::Printf(_L("CSendAppData::Start()\n"));)
	CStateMachine::Start( aClientStatus, (CAsynchEvent*)(iSendAlert), aStateMachineNotify );
}

inline CSendAlert* CSendAppData::SendAlert() const
{
	LOG(Log::Printf(_L("CSendAppData::SendAlert()\n"));)
	return iSendAlert;
}

inline void CSendAppData::SetUserData( TDesC8* aAppData )
{
	LOG(Log::Printf(_L("CSendAppData::SetUserData()\n"));)
	
	__ASSERT_DEBUG( iCurrentPos == 0, TlsPanic( ETlsPanicUserDataAlreadySet) );
	iAppData = aAppData;
}

inline void CSendAppData::SetHelloRequest( CHelloRequest* aHelloReq )
{
	iHelloReq = aHelloReq;
}


// Inline methods - CRecvAppData class
inline void CRecvAppData::Start( TRequestStatus* aClientStatus, MStateMachineNotify* aStateMachineNotify )
/**
 * Starts the 'Receive Application data' state machine by calling the 
 * Start() method of the base state machine class (CStateMachine).
 *
 * @param aClientStatus Pointer to a TRequestStatus object that completes when  
 * data reception is complete.
 * @param aStateMachineNotify Pointer to a MStateMachineNotify interface object.
 */
{
	LOG(Log::Printf(_L("CRecvAppData::Start()\n"));)
	CStateMachine::Start( aClientStatus, (CAsynchEvent*)iSendAlert, aStateMachineNotify );
}

inline void CRecvAppData::SetSockXfrLength( TInt* aLen )
{
	LOG(Log::Printf(_L("CRecvAppData::SetSockXfrLength()\n"));)
	iSockXfrLength = aLen;
}

inline CHelloRequest* CRecvAppData::HelloRequest() const
{
	return iHelloReq;
}

inline CSendAlert* CRecvAppData::SendAlert() const
{
	LOG(Log::Printf(_L("CRecvAppData::SendAlert()\n"));)
	return iSendAlert;
}

#endif
