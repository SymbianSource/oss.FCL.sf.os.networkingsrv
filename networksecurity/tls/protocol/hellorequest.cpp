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
// Hello Request message implementation file.
// 
//

/**
 @file
*/
  
#include "hellorequest.h"


CHelloRequest::CHelloRequest( CTlsConnection& aTlsConnection, CStateMachine& aStateMachine ) :
   CHandshakeReceive( aTlsConnection.TlsProvider(), aStateMachine, aTlsConnection.RecordParser() ),
   iTlsConnection( aTlsConnection )
{
}

TBool CHelloRequest::AcceptMessage( const TUint8 aHandshakeType ) const
{
	LOG(Log::Printf(_L("CHelloRequest::AcceptMessage()\n"));)
	
    __ASSERT_DEBUG( (iStateMachine->History() == KTlsApplicationData), TlsPanic(ETlsPanicHelloRequestRecWhileInAppData ));
   	return aHandshakeType == ETlsHelloRequestMsg;
}

CAsynchEvent* CHelloRequest::ProcessL( TRequestStatus& aStatus )
{
	LOG(Log::Printf(_L("CHelloRequest::ProcessL()\n"));)

	CStateMachine* pSendAppData = (CStateMachine*)iTlsConnection.SendAppData();
	__ASSERT_DEBUG( pSendAppData, TlsPanic(ETlsPanicNullStateMachine));
   
	if ( pSendAppData->ClientStatus() )
	{	//ok lets wait till the ongoing data is send under current crypto & compression
		pSendAppData->RegisterNotify( this );
	}
	else
	{	//start renegotiation since app data send SM stoped
		iTlsConnection.StartRenegotiation( &aStatus ); //iStateMachine will wait for re-negotiation
		//to complete
		//!!!at this stage 'this' has been deleted
	}

	//iStateMachine will wait for re-negotiation => no 
	//User::RequestComplete( KErrNone ); is called
   
	return NULL; //once the renegotiation has completed the iStateMachine stops
}

TBool CHelloRequest::OnCompletion( CStateMachine* aStateMachine )
{
	LOG(Log::Printf(_L("CHelloRequest::OnCompletion()\n"));)
	
	//ok the app data has been sent
	//deregister this as a notifier
	aStateMachine->DeRegisterNotify( this );
	//and register iTlsConnection back again 
	//(we could as well remember what has been registered but since it's always 
	//iTlsConnection .....)
	aStateMachine->RegisterNotify( &iTlsConnection );
	iTlsConnection.StartRenegotiation( &iStateMachine->iStatus ); //iStateMachine will wait for re-negotiation
	//to complete
	//!!!at this stage this has been deleted
   
	return EFalse; //don't want delete Data Send State Machine
}

