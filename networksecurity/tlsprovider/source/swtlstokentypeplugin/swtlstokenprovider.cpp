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


#include "swtlstokentypeplugin.h"


//
// CSwTLSTokenProvider


MCTToken& CSwTLSTokenProvider::Token()
	{
	return iToken;
	}
	
	
const TDesC& CSwTLSTokenProvider::Label()
	{
	return iLabel;
	}


void CSwTLSTokenProvider::GetSession(
		const TTLSServerAddr& aServerName,		
		RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
		TTLSSessionData& aOutputSessionData, 
		TRequestStatus& aStatus)
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenProvider::GetSession") )
	iToken.GetCacheData(aServerName, 						
						aAcceptableProtVersions,
					 aOutputSessionData);
	TRequestStatus* status = &aStatus;
	User::RequestComplete( status, KErrNone);
	return;
	}


void CSwTLSTokenProvider::ClearSessionCache(
		const TTLSServerAddr& aServerName, 
		TTLSSessionId& aSession, 
		TBool& aResult, 
		TRequestStatus& aStatus)
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenProvider::ClearSessionCache") )
	aResult = iToken.RemoveFromCache( aServerName, aSession );
	TRequestStatus* status = &aStatus;
	if( EFalse == aResult )
		User::RequestComplete( status, KTLSErrCacheEntryInUse);
	else
		User::RequestComplete( status, KErrNone);
	return;
	}



void CSwTLSTokenProvider::CryptoCapabilities( 
		RArray<TTLSProtocolVersion>& aProtocols,
		RArray<TTLSKeyExchangeAlgorithm>& aKeyExchAlgs,
		RArray<TTLSSignatureAlgorithm>& aSigAlgs,
		TRequestStatus& aStatus)
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenProvider::CryptoCapabilities") )
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
	err = aKeyExchAlgs.Append( EDHE );
	if ( err != KErrNone )
				User::RequestComplete( status, err );
	err = aKeyExchAlgs.Append( EPsk );
	if ( err != KErrNone )
				User::RequestComplete( status, err );

	err = aSigAlgs.Append( ERsaSigAlg );
	if ( err != KErrNone )
				User::RequestComplete( status, err );
	err = aSigAlgs.Append( EDsa );
	if ( err != KErrNone )
				User::RequestComplete( status, err );
	err = aSigAlgs.Append( EPskSigAlg);
	if ( err != KErrNone )
				User::RequestComplete( status, err );

	TInt max;
	
	max = aProtocols.Count();
	for(i=0; i<max; i++)
		SWTLSTOKEN_LOG2(  _L("	protocol version: 3.%d inserted into list"), aProtocols[i].iMinor ); 
	
	max = aKeyExchAlgs.Count();
	for(i=0; i<max; i++)
		SWTLSTOKEN_LOG2(  _L("	key exch alg: %d inserted into list"), aKeyExchAlgs[i] ); 
		
	max = aSigAlgs.Count();
	for(i=0; i<max; i++)
		SWTLSTOKEN_LOG2(  _L("	sign alg: %d inserted into list"), aSigAlgs[i] );
			

   if ( status )
      {
	   User::RequestComplete( status, KErrNone);
      }
	return;
	}



void CSwTLSTokenProvider::CancelGetSession()
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenProvider::CancelGetSession") )
	return;
	}

void CSwTLSTokenProvider::CancelCryptoCapabilities() 
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenProvider::CancelCryptoCapabilities") )
	return;
	}

void CSwTLSTokenProvider::CancelClearSessionCache()
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenProvider::CancelClearSessionCache") )
	return;
	}
