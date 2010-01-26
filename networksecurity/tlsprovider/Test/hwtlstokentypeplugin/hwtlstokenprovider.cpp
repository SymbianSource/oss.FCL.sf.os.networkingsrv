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
//


#include "hwtlstokentypeplugin.h"


//
// CHwTlsTokenProvider


MCTToken& CHwTlsTokenProvider::Token()
	{
	return iToken;
	}
	
	
const TDesC& CHwTlsTokenProvider::Label()
	{
	return iLabel;
	}


void CHwTlsTokenProvider::GetSession(
		const TTLSServerAddr& aServerName,		
		RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
		TTLSSessionData& aOutputSessionData, 
		TRequestStatus& aStatus)
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenProvider::GetSession") )
	iToken.GetCacheData(aServerName, 						
						aAcceptableProtVersions,
					 aOutputSessionData);
	TRequestStatus* status = &aStatus;
	User::RequestComplete( status, KErrNone);
	return;
	}


void CHwTlsTokenProvider::ClearSessionCache(
		const TTLSServerAddr& aServerName, 
		TTLSSessionId& aSession, 
		TBool& aResult, 
		TRequestStatus& aStatus)
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenProvider::ClearSessionCache") )
	aResult = iToken.RemoveFromCache( aServerName, aSession );
	TRequestStatus* status = &aStatus;
	if( EFalse == aResult )
		User::RequestComplete( status, KTLSErrCacheEntryInUse);
	else
		User::RequestComplete( status, KErrNone);
	return;
	}



void CHwTlsTokenProvider::CryptoCapabilities( 
		RArray<TTLSProtocolVersion>& aProtocols,
		RArray<TTLSKeyExchangeAlgorithm>& aKeyExchAlgs,
		RArray<TTLSSignatureAlgorithm>& aSigAlgs,
		TRequestStatus& aStatus)
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenProvider::CryptoCapabilities") )
	TRequestStatus* status = &aStatus;
	
	aProtocols.Reset();
	aKeyExchAlgs.Reset();
	
	TInt i;
	TInt err = KErrNone;
	
	err = aProtocols.Append( KTLS1_0 );
	if ( err != KErrNone )
				User::RequestComplete( status, err );
	err = aProtocols.Append( KSSL3_0 );
	if ( err != KErrNone )
				User::RequestComplete( status, err );
	
	err = aKeyExchAlgs.Append( ERsa );
	if ( err != KErrNone )
				User::RequestComplete( status, err );

	err = aSigAlgs.Append( ERsaSigAlg );
	if ( err != KErrNone )
				User::RequestComplete( status, err );

	// debug
	TInt max;
	
	max = aProtocols.Count();
	for(i=0; i<max; i++)
		HWTLSTOKEN_LOG2(  _L("	protocol version: 3.%d inserted into list"), aProtocols[i].iMinor ); 
	
	max = aKeyExchAlgs.Count();
	for(i=0; i<max; i++)
		HWTLSTOKEN_LOG2(  _L("	key exch alg: %d inserted into list"), aKeyExchAlgs[i] ); 
		
	max = aSigAlgs.Count();
	for(i=0; i<max; i++)
		HWTLSTOKEN_LOG2(  _L("	sign alg: %d inserted into list"), aSigAlgs[i] );
			

   if ( status )
      {
	   User::RequestComplete( status, KErrNone);
      }
	return;
	}



void CHwTlsTokenProvider::CancelGetSession()
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenProvider::CancelGetSession") )
	return;
	}

void CHwTlsTokenProvider::CancelCryptoCapabilities() 
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenProvider::CancelCryptoCapabilities") )
	return;
	}

void CHwTlsTokenProvider::CancelClearSessionCache()
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenProvider::CancelClearSessionCache") )
	return;
	}
