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
// SWTLSSESSIONCACHE.CPP
// 
//

#include "hwtlstokentypeplugin.h"



//
// CHwTLSSessionCache


CHwTLSSessionCache* CHwTLSSessionCache::NewL(
		TTLSServerAddr& aServerAddr, 
		TTLSSessionData& aData, 
		TDes8& aMasterSecret,
		HBufC8* aEncodedServerCert )
{
	CHwTLSSessionCache* self = new(ELeave) CHwTLSSessionCache;

	self->SetValues( aServerAddr, aData, aMasterSecret,
		aEncodedServerCert);
	self->SetResumable(EFalse, EFalse, EFalse);
	self->iClientCertInfo = NULL;
	
	return self;
 
}


void CHwTLSSessionCache::SetValues( TTLSServerAddr& aServerAddr, 
		TTLSSessionData& aData, 
		TDes8& aMasterSecret,
		HBufC8* aEncodedServerCert )
	{
	iServerAddr = aServerAddr;
	iSessionData = aData;
	
	iMasterSecret.Copy( aMasterSecret );

	TTime time;
	time.UniversalTime();
	iCreationTime = time.Int64() ;

	iEncodedServerCert = aEncodedServerCert; 
		
	return;
		
	}



void CHwTLSSessionCache::AddKeyInfo( const TCTTokenObjectHandle& aClientKeyObject,
									CCTCertInfo* aClientCertInfo )
{
	iClientKeyObject = aClientKeyObject;
	iClientCertInfo = aClientCertInfo;
}


void CHwTLSSessionCache::SetResumable(
									 TBool aResumable,
									 TBool aServerAuthenticated, 
									 TBool aClientAunthenticated )
	{
	iResumable = aResumable;
	iSessionData.iServerAuthenticated = aServerAuthenticated;
	iSessionData.iClientAuthenticated = aClientAunthenticated;
	}
	
TBool CHwTLSSessionCache::IsResumable()
	{
	return iResumable;
	}



CHwTLSSessionCache::~CHwTLSSessionCache()
{
	HWTLSTOKEN_LOG2(  _L("CHwTLSSessionCache::~CHwTLSSessionCache %x"), iEncodedServerCert )
	delete iEncodedServerCert;
	iMasterSecret.FillZ();
	if( NULL != iClientCertInfo )
		iClientCertInfo->Release();
}


const TTLSSessionData CHwTLSSessionCache::ReadData()
{
	return iSessionData;
}


HBufC8* CHwTLSSessionCache::ServerCertificate()
	{
	return iEncodedServerCert;
	}

CCTCertInfo* CHwTLSSessionCache::ClientCertificate()
	{
	return iClientCertInfo;
	}


const TTLSServerAddr CHwTLSSessionCache::ServerAddr()
{
	return iServerAddr;
}


const TCTTokenObjectHandle CHwTLSSessionCache::ClientKeyHandle()
{
	return iClientKeyObject; 
}


TInt64  CHwTLSSessionCache::CreationTime()
{
	return  iCreationTime;
}

const TPtrC8 CHwTLSSessionCache::MasterSecret()
{
	HWTLSTOKEN_LOG(  _L("CHwTLSSessionCache::MasterSecret") )
	return  TPtrC8( iMasterSecret ); 
}

