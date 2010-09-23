// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// SSL3.0 and TLS1.0 Application Data source file.
// Describes the implementation of the Application data (transmission
// and reception) state machines.
// 
//

/**
 @file
*/

#include "applicationdata.h"
#include "tlsconnection.h"
#include "recordprotocolevents.h"
#include "AlertProtocolEvents.h"
#include "hellorequest.h"


CSendAppData* CSendAppData::NewL( CRecordComposer& aRecordComposer )
{
	LOG(Log::Printf(_L("CSendAppData::NewL()"));)
	CSendAppData* self = new(ELeave) CSendAppData( aRecordComposer );
  	LOG(Log::Printf(_L("self %x - %x"), self, (TUint)self + sizeof( CSendAppData ));)
	CleanupStack::PushL( self );
	self->ConstructL( aRecordComposer );
	CleanupStack::Pop(self);
	return self;
}

inline CSendAppData::CSendAppData( CRecordComposer& aRecordComposer ) :
   iRecordComposer( aRecordComposer  )
{
	LOG(Log::Printf(_L("CSendAppData::CSendAppData()\n"));)
	iHistory = KTlsApplicationData; // This shows explicitly where we are
}

CSendAppData::~CSendAppData()
{
	LOG(Log::Printf(_L("CSendAppData::~CSendAppData()\n"));)
   SetSockXfrLength( NULL );
   Cancel( KErrNone );
	delete iSendAlert;
}

inline void CSendAppData::ConstructL( CRecordComposer& aRecordComposer )
{
	iSendAlert = new(ELeave)CSendAlert( *this, aRecordComposer );
	LOG(Log::Printf(_L("CSendAppData::ConstructL() %x\n"), SendAlert());)
	ResumeL();
}

void CSendAppData::Suspend()
{
	LOG(Log::Printf(_L("CSendAppData::Suspend()\n"));)
   
	iCurrentPos = iRecordComposer.CurrentPos();
	iAppData = iRecordComposer.UserData();
	iRecordComposer.SetUserData( NULL );
}

void CSendAppData::ResumeL()
{
	LOG(Log::Printf(_L("CSendAppData::ResumeL()\n"));)
   
	iRecordComposer.SetUserData( iAppData );
	iRecordComposer.ReConstructL( this, iCurrentPos ); //it sets record type to ETlsAppDataContentType
	iCurrentPos = 0;
	
	if ( !iActiveEvent )
	{	//active event is the record composer
		iActiveEvent = &iRecordComposer;
	}
}

void CSendAppData::SetSockXfrLength( TInt* aLen )
{
	LOG(Log::Printf(_L("CSendAppData::SetSockXfrLength()\n"));)

	iSockXfrLength = aLen;
	if ( iSockXfrLength )
	{
		*iSockXfrLength = 0;
	}
}

void CSendAppData::OnCompletion()
{
	LOG(Log::Printf(_L("CSendAppData::OnCompletion()\n"));)
   
	TDesC8* pAppData = iRecordComposer.UserData();
	if ( pAppData ) //could have finished via an error
	{
		if ( iSockXfrLength )
		{
			*iSockXfrLength = iRecordComposer.CurrentPos();
		}
		if ( iLastError == KErrNone && iStatus.Int() == KErrNone )
		{
			//hello request received?
			if ( iStateMachineNotify == iHelloReq && iHelloReq )
			{//no user request completion, signal to hello request
				iHelloReq->OnCompletion( this );
				return;
			}
			// Anything more to send?
			else if ( pAppData->Length() > iRecordComposer.CurrentPos() )
			{	// No error (cancel) && still some app data in the app buffer =>
				// => write another fragment
				iActiveEvent = &iRecordComposer;
				Start( iClientStatus, iStateMachineNotify );
				return;
			}
		}
	}
   
	iRecordComposer.SetUserData( NULL ); //we'r finished
	iRecordComposer.ResetCurrentPos();

	/* Fix for the TLS Client hang issue DEF130128.
	 * In the case of where one of the state machines receives the 
	 * asynchronous event alert due to TLS server reset, the state 
	 * machine triggers the cleanup activities. The TlsConnection 
	 * fails to clean up  the resources allocated for handling data 
	 * transfer and TLS handshake in such cases if one of the state 
	 * machines has issued the asynchronous server request and is 
	 * waiting for the request to complete. The change made below 
	 * will allow the code complete the pending status in such cases 
	 * and allow the cleanup to proceed.
	 */
	if ( iStatus.Int() == KRequestPending )
		{
			TRequestStatus* p=&iStatus;
			User::RequestComplete( p, iLastError );
		}
	/* End of fix for the TLS Client hang issue DEF130128. */
	
	CStateMachine::OnCompletion();
}

void CSendAppData::DoCancel()
{
	LOG(Log::Printf(_L("CSendAppData::DoCancel()\n"));)

	iLastError = KErrCancel;
	iRecordComposer.CancelAll();
	CStateMachine::DoCancel();
}

//
//
CRecvAppData* CRecvAppData::NewL( CTlsConnection& aTlsConnection )
{
	LOG(Log::Printf(_L("CRecvAppData::NewL()"));)
	CRecvAppData* self = new(ELeave) CRecvAppData( aTlsConnection );
  	LOG(Log::Printf(_L("self %x - %x"), self, (TUint)self + sizeof( CRecvAppData ));)
	CleanupStack::PushL( self );
	self->ConstructL( aTlsConnection );
	CleanupStack::Pop(self);
	return self;
}

inline CRecvAppData::CRecvAppData( CTlsConnection& aTlsConnection ) :
   iHeldData( 0, 0 ),
   iRecordParser( aTlsConnection.RecordParser() )
{
	LOG(Log::Printf(_L("CRecvAppData::CRecvAppData()\n"));)
	iHistory = KTlsApplicationData; // This shows explicitly where we are
}

CRecvAppData::~CRecvAppData()
{
	LOG(Log::Printf(_L("CRecvAppData::~CRecvAppData()\n"));)
   SetSockXfrLength( NULL );
   Cancel( KErrNone );
	delete iSendAlert;
}

inline void CRecvAppData::ConstructL( CTlsConnection& aTlsConnection )
{
	iSendAlert = new(ELeave)CSendAlert( *this, aTlsConnection.RecordComposer() );
	LOG(Log::Printf(_L("CRecvAppData::ConstructL() %x\n"), SendAlert());)
	ResumeL( aTlsConnection );
}

void CRecvAppData::Suspend()
{
	LOG(Log::Printf(_L("CRecvAppData::Suspend()\n"));)
   
	iHeldData = iRecordParser.HeldData();
	iAppData = iRecordParser.UserData();
	iRecordParser.SetUserData( NULL );
	iRecordParser.SetUserMaxLength( 0 );
}

void CRecvAppData::ResumeL( CTlsConnection& aTlsConnection )
{
	LOG(Log::Printf(_L("CRecvAppData::ResumeL()"));)
	
	__ASSERT_DEBUG( (iAppData && iClientStatus) || (!iAppData && !iClientStatus), TlsPanic(ETlsPanicAppDataResumeButNotStarted));
	__ASSERT_DEBUG( iSendAlert, TlsPanic(ETlsPanicAlertReceived));
	iRecordParser.SetUserData( iAppData );
	iRecordParser.SetUserMaxLength( iAppData ? iAppData->MaxLength() : 0 );
	iRecordParser.ReConstructL( this, iHeldData, *iSendAlert );

	// Destroy the Rx list & add the only handshake event accepted during application data mode.
	CHandshakeParser* pParser = iRecordParser.HandshakeParser();
	pParser->DestroyRxList();
	CHelloRequest* helloReq = new(ELeave)CHelloRequest( aTlsConnection, *this );
	//coverity[leave_without_push]
	LOG(Log::Printf(_L("helloReq %x - %x"), helloReq, (TUint)helloReq + sizeof( CHelloRequest ));)
	pParser->AddToList( *helloReq );

	aTlsConnection.SendAppData()->SetHelloRequest( helloReq ); //no ownership passed
	if ( !iActiveEvent )
	{	//active event is the record parser
		iActiveEvent = &iRecordParser;
	}
}

void CRecvAppData::OnCompletion()
{
	LOG(Log::Printf(_L("CRecvAppData::OnCompletion()\n"));)

   if ( iLastError == KErrNone && iStatus.Int() == KErrNone )
   {
      if ( iRecordParser.TlsRecordType() == ETlsHandshakeContentType )
      {//it could be just hello request received => suspend() could have already happened
         return;
      }
	   TDes8* pAppData = iRecordParser.UserData();
	   if ( pAppData ) //could have finished via an error
	   {
		   if ( iSockXfrLength && pAppData->Length() ) //are we to report RecvOneOrMore?
		   {	//yes and finish
			   *iSockXfrLength = pAppData->Length();
		   }
		   else if ( pAppData->Length() < pAppData->MaxLength() )
		   {	//no error (cancel) && still some free room in the app data buffer =>
			   //=> read another fragment
			   iActiveEvent = &iRecordParser;
			   Start( iClientStatus, iStateMachineNotify );
			   return;
		   }
	   }
   }
   
	iRecordParser.SetUserData( NULL ); //we'r finished
	iRecordParser.SetUserMaxLength( 0 );
	
	/* Fix for the TLS Client hang issue DEF130128.
	 * In the case of where one of the state machines receives the 
	 * asynchronous event alert due to TLS server reset, the state 
	 * machine triggers the cleanup activities. The TlsConnection 
	 * fails to clean up  the resources allocated for handling data 
	 * transfer and TLS handshake in such cases if one of the state 
	 * machines has issued the asynchronous server request and is 
	 * waiting for the request to complete. The change made below 
	 * will allow the code complete the pending status in such cases 
	 * and allow the cleanup to proceed.
	 */
	if ( iStatus.Int() == KRequestPending )
		{
			TRequestStatus* p=&iStatus;
			User::RequestComplete( p, iLastError );
		}
	/* End of fix for the TLS Client hang issue DEF130128. */
	
	CStateMachine::OnCompletion();
}

void CRecvAppData::DoCancel()
{
	LOG(Log::Printf(_L("CRecvAppData::DoCancel()\n"));)
   
    iLastError = KErrCancel; 
	iRecordParser.CancelAll();
	CStateMachine::DoCancel();
}
