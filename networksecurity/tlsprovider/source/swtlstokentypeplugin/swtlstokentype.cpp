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

#include <bautils.h>
#include <barsc.h>


#include <swtlstokentype.rsg>
_LIT(KResourceFile, "Z:\\RESOURCE\\SWTLSTOKENTYPE.RSC");


CSwTLSToken::CSwTLSToken(const TDesC& aLabel, CCTTokenType& aTokenType)
		: iTokenType(aTokenType), iLabel(aLabel)
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::CSwTLSToken") )
	}
	
CSwTLSToken* CSwTLSToken::NewL(const TDesC& aLabel, CCTTokenType& aTokenType)
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::NewL") )
		
	CSwTLSToken* that = new (ELeave) CSwTLSToken(aLabel, aTokenType);
	CleanupStack::PushL(that);
				
	that->ReadResourceFileL();
	
	TRAP_IGNORE(that->InitThreadLocStorageL() ) // regardless of success or failure
							// of cache initialisation the token initialisation 
							// is completed
			
	CleanupStack::Pop(that);
	return that;
	
	}
	
CSwTLSToken::~CSwTLSToken()
	{
	if ( NULL != iStrings )
		iStrings->Close();

	delete iStrings;
	}

const TDesC& CSwTLSToken::Label()
	{
	return *(*iStrings)[0]; 
	}
	
void CSwTLSToken::ReadResourceFileL()
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::ReadResourceFileL") )
	
	RFs fs;
	User::LeaveIfError( fs.Connect() );
	CleanupClosePushL(fs);
	
	TFileName fileName;
	fileName.Copy(KResourceFile);

	BaflUtils::NearestLanguageFile(fs,fileName);
	if (!BaflUtils::FileExists(fs,fileName))
		{
		User::Leave(KErrNotFound); 
		}
		
	RResourceFile resourceFile;
	resourceFile.OpenL(fs,fileName);
	CleanupClosePushL(resourceFile);
	resourceFile.ConfirmSignatureL(0);
	resourceFile.Offset();
	TResourceReader reader;
	HBufC8* resource = resourceFile.AllocReadLC(R_SWTLST_STRINGS );
	reader.SetBuffer(resource);

	CDesCArray* strings = reader.ReadDesCArrayL();
	CleanupStack::PushL(strings);
	
	iStrings = new(ELeave) RCPointerArray<HBufC>;
	
	TInt count = strings->Count();
	for (TInt i = 0; i < count; i++)
		{
		SWTLSTOKEN_LOG2(  _L("	string: %d"), i )
		SWTLSTOKEN_LOG_HEX( (const TUint8*)((*strings)[i].Ptr()), 
						(*strings)[i].Size() )
		User::LeaveIfError(iStrings->Append((*strings)[i].AllocLC()));		
		CleanupStack::Pop();
		}
	
	CleanupStack::PopAndDestroy( 4, &fs); // Close strings, reader, file
	
	
	}

const TDesC& CSwTLSToken::Information(TTokenInformation aRequiredInformation)
	{
	return *(*iStrings)[aRequiredInformation + 1];
	}


MCTTokenType& CSwTLSToken::TokenType()
	{
	return iTokenType;
	}



TCTTokenHandle CSwTLSToken::Handle()
	{
	return TCTTokenHandle(TokenType().Type(), 1); 
	}

void CSwTLSToken::DoGetInterface(TUid aRequiredInterface,
							  MCTTokenInterface*& aReturnedInterface, 
							  TRequestStatus& aStatus) 
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::DoGetInterface") )	
	TUid uidProv = { KInterfaceTLSTokenProvider };
	TUid uidSess = { KInterfaceTLSSession };

	TRequestStatus* status = &aStatus;

	if ( aRequiredInterface == uidProv ) 
		{
		SWTLSTOKEN_LOG(  _L("	creation of CSwTLSTokenProvider attempted") )
		aReturnedInterface = new CSwTLSTokenProvider(iLabel, *this);
		}

	if ( aRequiredInterface == uidSess )
		{
		SWTLSTOKEN_LOG(  _L("	creation of CSwTLSSession attempted") )
		aReturnedInterface = new CSwTLSSession(iLabel, *this);
		}
	
	if ( (aRequiredInterface != uidProv) && (aRequiredInterface != uidSess) )
		{
			SWTLSTOKEN_LOG(  _L("	unrecognised interface UID passed") )
			User::RequestComplete(status, KErrNotSupported); 
			return;
		}

	if (aReturnedInterface)
		{
		SWTLSTOKEN_LOG(  _L("	interface successfully created") )
		User::RequestComplete(status, KErrNone);
		}
	else
		{
   	--ReferenceCount();
	User::RequestComplete(status, KErrNoMemory);
		}
	}


TBool CSwTLSToken::DoCancelGetInterface()
	{
	return ETrue;
	}


TInt& CSwTLSToken::ReferenceCount()
	{
	return iCount;
	}



void CSwTLSToken:: InitThreadLocStorageL() 
{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken:: InitThreadLocStorage") )
	TInt err;
		
	iCache = NULL;
			
	CSwTLSCache* globals = (CSwTLSCache*)Dll::Tls();

	// If the global pointer is valid, then the cache was already created; if not:
	if ( !globals )
		{
		SWTLSTOKEN_LOG(  _L("	thread local storage not initialised yet") )

		globals = new(ELeave) CSwTLSCache;
		err = Dll::SetTls( globals );
		globals->iCounter = 1;
		

		// If there was any kind of error, delete globals and clear the TLS
		if ( err )
			{
			SWTLSTOKEN_LOG(  _L("	error while initialising cache") )
			delete globals;
			globals = NULL;
			// Ensure thread local storage is clear 
			Dll::SetTls( 0 ); 
			}
		else
			{
		   RCPointerArray<CSwTLSSessionCache>* cache = new(ELeave) RCPointerArray<CSwTLSSessionCache>;
		   globals->iCacheArray = cache;
			SWTLSTOKEN_LOG(  _L("	cache successfully initialised") )
			iCache = globals;
			SWTLSTOKEN_LOG2(  _L("	cache's counter set to %d"), globals->iCounter )
			}
		
		}
	else
		{
		SWTLSTOKEN_LOG(  _L("	thread local storage  initialised before") )
		globals->iCounter++;
		iCache = globals;
		SWTLSTOKEN_LOG2(  _L("	cache's counter set to %d"), globals->iCounter )
		}
		
	
	if( NULL == iCache )
		User::Leave( KErrNoMemory );
	
	return; 
} 


void CSwTLSToken::DoRelease()
{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::DoRelease") )
	
	CSwTLSCache* globals = (CSwTLSCache*)Dll::Tls();
	if( NULL != globals )
		{
		globals->iCounter--;
		SWTLSTOKEN_LOG2(  _L("	cache's counter decreased to %d"), globals->iCounter )
		if( 0 == globals->iCounter )
			{
	      if(iCache && iCache->iCacheArray)
		      {
	         TInt noOfEntries = iCache->iCacheArray->Count();
      		SWTLSTOKEN_LOG2(  _L("	noOfEntries = %d"), noOfEntries )
	         for( TInt i = noOfEntries - 1; i >= 0; i-- )
               {
               CSwTLSSessionCache* cacheEntry = (*iCache->iCacheArray)[i];
               delete cacheEntry;
               }
		      iCache->iCacheArray->Close();
		      delete iCache->iCacheArray;
		      }
			delete globals;
      	iCache = NULL;
			TInt err = Dll::SetTls( NULL );
			if( KErrNone == err )
				{
				SWTLSTOKEN_LOG(  _L("	thread local storage set to NULL") )
				}
			if( KErrNone != err )
				{
				SWTLSTOKEN_LOG(  _L("	WARNING: failed to null thread local storage") )
				}
			}
		}
	
	delete this;
}

	
TBool ProtocolVersionComparison(const TTLSProtocolVersion& aPV1, 
										const TTLSProtocolVersion& aPV2)
	{
	return (aPV1 == aPV2);
	}
	

void CSwTLSToken::GetCacheData(
				const TTLSServerAddr& aServerName,				
				RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
				TTLSSessionData& aOutputSessionData)
{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::GetCacheData") )	
	
	aOutputSessionData.iSessionId.SetLength(0);
			
	if( NULL == iCache )
		// no cache
		return;
	
	
	TIdentityRelation<TTLSProtocolVersion> protocolComp( ProtocolVersionComparison );
	
	TTime time;
	time.UniversalTime();
	TTimeIntervalSeconds interval;
	
	TInt i;
	TInt noOfEntries = iCache->iCacheArray->Count();
	
	for( i = noOfEntries-1; i>=0; i-- )
	{
		// first, remove all 'too old' entries
		TInt err = KErrNone;
		CSwTLSSessionCache* entry = (*iCache->iCacheArray)[i];
		err = time.SecondsFrom( TTime( entry->CreationTime() ), interval);
		if ( (KErrNone != err)  // means interval out of range of TInt
			|| (interval.Int() >= KTLSCachingTimeout) )
		{
			iCache->iCacheArray->Remove(i);
			delete entry; 
			SWTLSTOKEN_LOG2(  _L("	removed old entry: %d"), i )
			continue;
		}
		
		if( (0 != entry->ServerAddr().iAddress.Compare( aServerName.iAddress )) ||
			( entry->ServerAddr().iPort != aServerName.iPort ) ) 
			continue;
			
		aOutputSessionData = entry->ReadData(); 
		
		if( entry->IsResumable() == EFalse )
			{
			SWTLSTOKEN_LOG2(  _L("	session no %d could match but is non-resumable"), i )
			continue;
			}		
						
		if ( (aAcceptableProtVersions.Count() > 0) && 
			(KErrNotFound == aAcceptableProtVersions.Find(aOutputSessionData.iProtocolVersion, protocolComp) ) )//!check )
			{
			aOutputSessionData.iSessionId.SetLength(0);
			continue;		
			}
		SWTLSTOKEN_LOG(  _L("	found good session") )
		break; 
	}

	return;
}



TBool CSwTLSToken::AddToCacheL (
		TTLSSessionData& aData, 
		TTLSSessionNameAndID& aSessNam, 
		TDes8& aMasterSecret,
		const TDes8& aEncodedServerCert // can be empty
		) 
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::AddToCacheL") )
	
	if( (NULL == iCache) || (0 == aSessNam.iSessionId.Length() ) )
		{
		SWTLSTOKEN_LOG(  _L("	cache is NULL or session id empty") )
		return EFalse;
		}
		
	// first remove sessions with same id connected to the same server
	if ( EFalse == RemoveFromCache( aSessNam.iServerName, aSessNam.iSessionId) )
		{
		SWTLSTOKEN_LOG(  _L("	session not cached - session with same id exists and is in use") )
		return EFalse;
		}
	

	HBufC8* encodedCert = NULL;
   if ( aEncodedServerCert.Length() )
      {
	   encodedCert = aEncodedServerCert.AllocL();
   	SWTLSTOKEN_LOG(  _L("	encoded Cert field set") )			
      }

   CSwTLSSessionCache* cacheEntry = 
		CSwTLSSessionCache::NewL( aSessNam.iServerName, aData, 
			aMasterSecret, encodedCert );
	
	TInt noOfEntries = iCache->iCacheArray->Count();
	TInt err = KErrNone;
	SWTLSTOKEN_LOG2(  _L("	actual no of entries in a cache: %d"), noOfEntries )
	if( noOfEntries < KTLSCacheSize )
		{
		err = iCache->iCacheArray->Append( cacheEntry );
		if( KErrNone != err )
			{
			delete cacheEntry;
   		SWTLSTOKEN_LOG(  _L("	no entry added ") )
         return EFalse;
			}
		return ETrue;
		}
	

	// if all entries occupied, find the oldest then remove and insert new
	TInt i;
	TInt oldest = 0;
	for( i=0; i< noOfEntries; i++)
		{
		if( TTime( (*iCache->iCacheArray)[i]->CreationTime() ) <  
				TTime( (*iCache->iCacheArray)[oldest]->CreationTime() ) )
			oldest = i;
		}
	
	CSwTLSSessionCache* entryToDestroy = (*iCache->iCacheArray)[oldest];
	iCache->iCacheArray->Remove(oldest);
	SWTLSTOKEN_LOG(  _L("	the oldest entry removed from cache") )
	delete entryToDestroy; 
	err = iCache->iCacheArray->Append( cacheEntry );
	if( KErrNone != err )
		{
		delete cacheEntry;
		User::Leave( err );
		}
		
	SWTLSTOKEN_LOG(  _L("	added new entry") )
	
	return ETrue;
			
	}


TBool CSwTLSToken::RemoveFromCache( const TTLSServerAddr& aServerName, 
									const TDes8& aSessionId )
	{
	SWTLSTOKEN_LOG(  _L("CSwTLSToken::RemoveFromCache") )
	if( NULL == iCache )
		return ETrue;

	TInt i;
	TInt noOfEntries = iCache->iCacheArray->Count();
	for( i = noOfEntries - 1; i >= 0; i-- )
	{
		if( ((*iCache->iCacheArray)[i]->ReadData().iSessionId == aSessionId || !aSessionId.Length()) &&
			 (*iCache->iCacheArray)[i]->ServerAddr().iAddress == aServerName.iAddress &&
			 (*iCache->iCacheArray)[i]->ServerAddr().iPort == aServerName.iPort ) 
			{
			if( (*iCache->iCacheArray)[i]->IsResumable() == EFalse )
				{
				SWTLSTOKEN_LOG2(  _L("	found matching entry (%d) but is in use - not removed"), i )
				return EFalse;
				}
			CSwTLSSessionCache* entryToDestroy = (*iCache->iCacheArray)[i];
			iCache->iCacheArray->Remove(i);
			delete entryToDestroy; 
			SWTLSTOKEN_LOG2(  _L("	removed entry no: %d"), i )
			
			}
	}

	return ETrue; // no entry with aSessionId found

}


CSwTLSSessionCache* CSwTLSToken::SessionCache( const TTLSServerAddr& aServerName,
												TDes8&	aSessionId)
{

	SWTLSTOKEN_LOG(  _L("CSwTLSToken::SessionCache") )
	
	if( NULL == iCache || (0 == aSessionId.Length() ))
		return NULL;

	CSwTLSSessionCache* cacheEntry;
	TInt i;
	TInt noOfEntries = iCache->iCacheArray->Count();
	for( i=0; i < noOfEntries; i++ )
	{
		if( ((*iCache->iCacheArray)[i]->ReadData().iSessionId == aSessionId) &&
			((*iCache->iCacheArray)[i]->ServerAddr().iAddress == aServerName.iAddress) &&
			((*iCache->iCacheArray)[i]->ServerAddr().iPort == aServerName.iPort)	  )
			{
			SWTLSTOKEN_LOG2(  _L("	returning entry no %d"), i )
			cacheEntry = (*iCache->iCacheArray)[i];
			return cacheEntry;
			}
	}

	return NULL; // no entry with aSessionId found

}


//
// CSwTLSTokenType

void CSwTLSTokenType::List(RCPointerArray<HBufC>& aTokens, 
							   TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	SWTLSTOKEN_LOG(  _L("CSwTLSTokenType::List") )
	TRequestStatus* r = &aStatus;
	HBufC* name = iTokenInfo->Alloc();
	if (name)
      {
      TInt n = aTokens.Append(name);
      if ( n != KErrNone )
         {
         delete name;
         }
		User::RequestComplete(r, n);
      }
	else
      {
		User::RequestComplete(r, KErrNoMemory);
      }
	}

void CSwTLSTokenType::CancelList()
	{
	}

void CSwTLSTokenType::OpenToken(const TDesC&, MCTToken*& aToken,
						   TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	SWTLSTOKEN_LOG( _L("CSwTLSTokenType::OpenToken (by name)") ) 
	TInt err = KErrNone;
	
	TRAP( err, ( aToken = CSwTLSToken::NewL(*iTokenInfo, *this) ) )
	TRequestStatus* r = &aStatus;
	if( aToken )
		{
		User::RequestComplete(r, KErrNone);
		IncReferenceCount();
		}
	else
		User::RequestComplete(r, err);
		
	}

void CSwTLSTokenType::OpenToken(TCTTokenHandle,
										 MCTToken*& aToken,
										 TRequestStatus& aStatus)
	{
	SWTLSTOKEN_LOG( _L("CSwTLSTokenType::OpenToken (by Handle)") ) 
	TInt err = KErrNone;
	
	TRAP( err, ( aToken = CSwTLSToken::NewL(*iTokenInfo, *this) ) )
	TRequestStatus* r = &aStatus;
	if( aToken )
		{
		User::RequestComplete(r, KErrNone);
		IncReferenceCount();
		}
	else
		User::RequestComplete(r, err);
		
	}

void CSwTLSTokenType::CancelOpenToken()
	{
	}


CSwTLSTokenType::~CSwTLSTokenType()
	{
	SWTLSTOKEN_LOG( _L("CSwTLSTokenType::~CSwTLSTokenType") )
	delete iTokenInfo;
	}

	
CSwTLSTokenType* CSwTLSTokenType::InitL()
{
	CSwTLSTokenType* that = new (ELeave) CSwTLSTokenType;
	CleanupStack::PushL(that);
	that->iTokenInfo = KSwTLSTokenInfo().AllocL(); 
	CleanupStack::Pop(that);
	CREATE_SWTLSTOKEN_LOG( _L("CSwTLSTokenType initialised") ) 
	return that;
}


const TImplementationProxy ImplementationTable[] = 
{
	IMPLEMENTATION_PROXY_ENTRY(KSwTLSTokenTypeUid,	CSwTLSTokenType::InitL)
};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

	return ImplementationTable;
}

