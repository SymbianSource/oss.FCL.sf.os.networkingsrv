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
// CSwTLSSessionCache


CSwTLSSessionCache* CSwTLSSessionCache::NewL(
		TTLSServerAddr& aServerAddr, 
		TTLSSessionData& aData, 
		TDes8& aMasterSecret,
		HBufC8* aEncodedServerCert )
{
	CSwTLSSessionCache* self = new(ELeave) CSwTLSSessionCache;

	self->SetValues( aServerAddr, aData, aMasterSecret,
		aEncodedServerCert);
	self->SetResumable(EFalse, EFalse, EFalse);
	self->iClientCertInfo = NULL;
	
	return self;
 
}


void CSwTLSSessionCache::SetValues( TTLSServerAddr& aServerAddr, 
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



void CSwTLSSessionCache::AddKeyInfo( const TCTTokenObjectHandle& aClientKeyObject,
									CCTCertInfo* aClientCertInfo )
{
	iClientKeyObject = aClientKeyObject;
	iClientCertInfo = aClientCertInfo;
}


void CSwTLSSessionCache::SetResumable(
									 TBool aResumable,
									 TBool aServerAuthenticated, 
									 TBool aClientAunthenticated )
	{
	iResumable = aResumable;
	iSessionData.iServerAuthenticated = aServerAuthenticated;
	iSessionData.iClientAuthenticated = aClientAunthenticated;
	}
	
TBool CSwTLSSessionCache::IsResumable()
	{
	return iResumable;
	}



CSwTLSSessionCache::~CSwTLSSessionCache()
{
	SWTLSTOKEN_LOG2(  _L("CSwTLSSessionCache::~CSwTLSSessionCache %x"), iEncodedServerCert )
	delete iEncodedServerCert;
	iMasterSecret.FillZ();
	if( NULL != iClientCertInfo )
		iClientCertInfo->Release();
}


const TTLSSessionData CSwTLSSessionCache::ReadData()
{
	return iSessionData;
}


HBufC8* CSwTLSSessionCache::ServerCertificate()
	{
	return iEncodedServerCert;
	}

CCTCertInfo* CSwTLSSessionCache::ClientCertificate()
	{
	return iClientCertInfo;
	}


const TTLSServerAddr CSwTLSSessionCache::ServerAddr()
{
	return iServerAddr;
}


const TCTTokenObjectHandle CSwTLSSessionCache::ClientKeyHandle()
{
	return iClientKeyObject; 
}


TInt64  CSwTLSSessionCache::CreationTime()
{
	return  iCreationTime;
}

const TPtrC8 CSwTLSSessionCache::MasterSecret()
{
	SWTLSTOKEN_LOG(  _L("CSwTLSSessionCache::MasterSecret") )
	return  TPtrC8( iMasterSecret ); 
}

