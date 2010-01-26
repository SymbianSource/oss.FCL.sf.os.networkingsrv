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

#include <bautils.h>
#include <barsc.h>


#include <hwtlstokentype.rsg>
_LIT(KResourceFile, "Z:\\RESOURCE\\HwTlsTokenTYPE.RSC");


CHwTlsToken::CHwTlsToken(const TDesC& aLabel, CCTTokenType& aTokenType)
		: iTokenType(aTokenType), iLabel(aLabel)
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::CHwTlsToken") )
	}
	
CHwTlsToken* CHwTlsToken::NewL(const TDesC& aLabel, CCTTokenType& aTokenType)
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::NewL") )
		
	CHwTlsToken* that = new (ELeave) CHwTlsToken(aLabel, aTokenType);
	CleanupStack::PushL(that);
				
	that->ReadResourceFileL();
	
	TRAP_IGNORE(that->InitThreadLocStorageL() ) // regardless of success or failure
							// of cache initialisation the token initialization 
							// is completed
			
	CleanupStack::Pop(that);
	return that;
	
	}
	
CHwTlsToken::~CHwTlsToken()
	{
	if ( NULL != iStrings )
		iStrings->Close();

	delete iStrings;
	}

const TDesC& CHwTlsToken::Label()
	{
	return *(*iStrings)[0]; 
	}
	
void CHwTlsToken::ReadResourceFileL()
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::ReadResourceFileL") )
	
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
		HWTLSTOKEN_LOG2(  _L("	string: %d"), i )
		HWTLSTOKEN_LOG_HEX( (const TUint8*)((*strings)[i].Ptr()), 
						(*strings)[i].Size() )
		User::LeaveIfError(iStrings->Append((*strings)[i].AllocLC()));		
		CleanupStack::Pop();
		}
	
	CleanupStack::PopAndDestroy( 4, &fs); // Close strings, reader, file
	
	
	}

const TDesC& CHwTlsToken::Information(TTokenInformation aRequiredInformation)
	{
	return *(*iStrings)[aRequiredInformation + 1];
	}


MCTTokenType& CHwTlsToken::TokenType()
	{
	return iTokenType;
	}



TCTTokenHandle CHwTlsToken::Handle()
	{
	return TCTTokenHandle(TokenType().Type(), 1); 
	}

void CHwTlsToken::DoGetInterface(TUid aRequiredInterface,
							  MCTTokenInterface*& aReturnedInterface, 
							  TRequestStatus& aStatus) 
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::DoGetInterface") )	
	TUid uidProv = { KInterfaceTLSTokenProvider };
	TUid uidSess = { KInterfaceTLSSession };

	TRequestStatus* status = &aStatus;

	if ( aRequiredInterface == uidProv ) 
		{
		HWTLSTOKEN_LOG(  _L("	creation of CHwTlsTokenProvider attempted") )
		aReturnedInterface = new CHwTlsTokenProvider(iLabel, *this);
		}

	if ( aRequiredInterface == uidSess )
		{
		HWTLSTOKEN_LOG(  _L("	creation of CHwTLSSession attempted") )
		aReturnedInterface = new CHwTLSSession(iLabel, *this);
		}
	
	if ( (aRequiredInterface != uidProv) && (aRequiredInterface != uidSess) )
		{
			HWTLSTOKEN_LOG(  _L("	unrecognised interface UID passed") )
			User::RequestComplete(status, KErrNotSupported); 
			return;
		}

	if (aReturnedInterface)
		{
		HWTLSTOKEN_LOG(  _L("	interface successfully created") )
		User::RequestComplete(status, KErrNone);
		}
	else
		{
   	--ReferenceCount();
	User::RequestComplete(status, KErrNoMemory);
		}
	}


TBool CHwTlsToken::DoCancelGetInterface()
	{
	return ETrue;
	}


TInt& CHwTlsToken::ReferenceCount()
	{
	return iCount;
	}



void CHwTlsToken:: InitThreadLocStorageL() 
{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken:: InitThreadLocStorage") )
	TInt err;
		
	iCache = NULL;
			
	CHwTLSCache* globals = (CHwTLSCache*)Dll::Tls();

	// If the global pointer is valid, then the cache was already created; if not:
	if ( !globals )
		{
		HWTLSTOKEN_LOG(  _L("	thread local storage not initialised yet") )

		globals = new(ELeave) CHwTLSCache;
		err = Dll::SetTls( globals );
		globals->iCounter = 1;
		

		// If there was any kind of error, delete globals and clear the TLS
		if ( err )
			{
			HWTLSTOKEN_LOG(  _L("	error while initialising cache") )
			delete globals;
			globals = NULL;
			// Ensure thread local storage is clear 
			Dll::SetTls( 0 ); 
			}
		else
			{
		   RCPointerArray<CHwTLSSessionCache>* cache = new(ELeave) RCPointerArray<CHwTLSSessionCache>;
		   globals->iCacheArray = cache;
			HWTLSTOKEN_LOG(  _L("	cache successfully initialised") )
			iCache = globals;
			HWTLSTOKEN_LOG2(  _L("	cache's counter set to %d"), globals->iCounter )
			}
		
		}
	else
		{
		HWTLSTOKEN_LOG(  _L("	thread local storage  initialised before") )
		globals->iCounter++;
		iCache = globals;
		HWTLSTOKEN_LOG2(  _L("	cache's counter set to %d"), globals->iCounter )
		}
		
	
	if( NULL == iCache )
		User::Leave( KErrNoMemory );
	
	return; 
} 


void CHwTlsToken::DoRelease()
{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::DoRelease") )
	
	CHwTLSCache* globals = (CHwTLSCache*)Dll::Tls();
	if( NULL != globals )
		{
		globals->iCounter--;
		HWTLSTOKEN_LOG2(  _L("	cache's counter decreased to %d"), globals->iCounter )
		if( 0 == globals->iCounter )
			{
	      if(iCache && iCache->iCacheArray)
		      {
	         TInt noOfEntries = iCache->iCacheArray->Count();
      		HWTLSTOKEN_LOG2(  _L("	noOfEntries = %d"), noOfEntries )
	         for( TInt i = noOfEntries - 1; i >= 0; i-- )
               {
               CHwTLSSessionCache* cacheEntry = (*iCache->iCacheArray)[i];
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
				HWTLSTOKEN_LOG(  _L("	thread local storage set to NULL") )
				}
			if( KErrNone != err )
				{
				HWTLSTOKEN_LOG(  _L("	WARNING: failed to null thread local storage") )
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
	

void CHwTlsToken::GetCacheData(
				const TTLSServerAddr& aServerName,				
				RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
				TTLSSessionData& aOutputSessionData)
{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::GetCacheData") )	
	
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
		// must remove first all too old entries
		TInt err = KErrNone;
		CHwTLSSessionCache* entry = (*iCache->iCacheArray)[i];
		err = time.SecondsFrom( TTime( entry->CreationTime() ), interval);
		if ( (KErrNone != err)  // means interval out of range of TInt
			|| (interval.Int() >= KTLSCachingTimeout) )
		{
			iCache->iCacheArray->Remove(i);
			delete entry; 
			HWTLSTOKEN_LOG2(  _L("	removed old entry: %d"), i )
			continue;
		}
		
		if( (0 != entry->ServerAddr().iAddress.Compare( aServerName.iAddress )) ||
			( entry->ServerAddr().iPort != aServerName.iPort ) ) 
			continue;
			
		aOutputSessionData = entry->ReadData(); 
		
		if( entry->IsResumable() == EFalse )
			{
			HWTLSTOKEN_LOG2(  _L("	session no %d could match but is non-resumable"), i )
			continue;
			}		
						
		if ( (aAcceptableProtVersions.Count() > 0) && 
			(KErrNotFound == aAcceptableProtVersions.Find(aOutputSessionData.iProtocolVersion, protocolComp) ) )//!check )
			{
			aOutputSessionData.iSessionId.SetLength(0);
			continue;		
			}
		HWTLSTOKEN_LOG(  _L("	found good session") )
		break; 
	}

	return;
}



TBool CHwTlsToken::AddToCacheL (
		TTLSSessionData& aData, 
		TTLSSessionNameAndID& aSessNam, 
		TDes8& aMasterSecret,
		const TDes8& aEncodedServerCert // can be empty
		) 
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::AddToCacheL") )
	
	if( (NULL == iCache) || (0 == aSessNam.iSessionId.Length() ) )
		{
		HWTLSTOKEN_LOG(  _L("	cache is NULL or session id empty") )
		return EFalse;
		}
		
	// first remove sessions with same id to the same server
	if ( EFalse == RemoveFromCache( aSessNam.iServerName, aSessNam.iSessionId) )
		{
		HWTLSTOKEN_LOG(  _L("	session not cached - session with same id exists and is in use") )
		return EFalse;
		}
	

	HBufC8* encodedCert = NULL;
   if ( aEncodedServerCert.Length() )
      {
	   encodedCert = aEncodedServerCert.AllocL();
   	HWTLSTOKEN_LOG(  _L("	encoded Cert field set") )			
      }

   CHwTLSSessionCache* cacheEntry = 
		CHwTLSSessionCache::NewL( aSessNam.iServerName, aData, 
			aMasterSecret, encodedCert );
	
	TInt noOfEntries = iCache->iCacheArray->Count();
	TInt err = KErrNone;
	HWTLSTOKEN_LOG2(  _L("	actual no of entries in a cache: %d"), noOfEntries )
	if( noOfEntries < KTLSCacheSize )
		{
		err = iCache->iCacheArray->Append( cacheEntry );
		if( KErrNone != err )
			{
			delete cacheEntry;
   		HWTLSTOKEN_LOG(  _L("	no entry added ") )
         return EFalse;
			}
		return ETrue;
		}
	

	// if all entries occupied find the oldest, remove and insert new
	TInt i;
	TInt oldest = 0;
	for( i=0; i< noOfEntries; i++)
		{
		if( TTime( (*iCache->iCacheArray)[i]->CreationTime() ) <  
				TTime( (*iCache->iCacheArray)[oldest]->CreationTime() ) )
			oldest = i;
		}
	
	CHwTLSSessionCache* entryToDestroy = (*iCache->iCacheArray)[oldest];
	iCache->iCacheArray->Remove(oldest);
	HWTLSTOKEN_LOG(  _L("	the oldest entry removed from cache") )
	delete entryToDestroy; 
	err = iCache->iCacheArray->Append( cacheEntry );
	if( KErrNone != err )
		{
		delete cacheEntry;
		User::Leave( err );
		}
		
	HWTLSTOKEN_LOG(  _L("	added new entry") )
	
	return ETrue;
			
	}


TBool CHwTlsToken::RemoveFromCache( const TTLSServerAddr& aServerName, 
									const TDes8& aSessionId )
	{
	HWTLSTOKEN_LOG(  _L("CHwTlsToken::RemoveFromCache") )
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
				HWTLSTOKEN_LOG2(  _L("	found matching entry (%d) but is in use - not removed"), i )
				return EFalse;
				}
			CHwTLSSessionCache* entryToDestroy = (*iCache->iCacheArray)[i];
			iCache->iCacheArray->Remove(i);
			delete entryToDestroy; 
			HWTLSTOKEN_LOG2(  _L("	removed entry no: %d"), i )
			
			}
	}

	return ETrue; // no entry with aSessionId found

}


CHwTLSSessionCache* CHwTlsToken::SessionCache( const TTLSServerAddr& aServerName,
												TDes8&	aSessionId)
{

	HWTLSTOKEN_LOG(  _L("CHwTlsToken::SessionCache") )
	
	if( NULL == iCache || (0 == aSessionId.Length() ))
		return NULL;

	CHwTLSSessionCache* cacheEntry;
	TInt i;
	TInt noOfEntries = iCache->iCacheArray->Count();
	for( i=0; i < noOfEntries; i++ )
	{
		if( ((*iCache->iCacheArray)[i]->ReadData().iSessionId == aSessionId) &&
			((*iCache->iCacheArray)[i]->ServerAddr().iAddress == aServerName.iAddress) &&
			((*iCache->iCacheArray)[i]->ServerAddr().iPort == aServerName.iPort)	  )
			{
			HWTLSTOKEN_LOG2(  _L("	returning entry no %d"), i )
			cacheEntry = (*iCache->iCacheArray)[i];
			return cacheEntry;
			}
	}

	return NULL; // no entry with aSessionId found

}


//
// CHwTlsTokenType

void CHwTlsTokenType::List(RCPointerArray<HBufC>& aTokens, 
							   TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	HWTLSTOKEN_LOG(  _L("CHwTlsTokenType::List") )
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

void CHwTlsTokenType::CancelList()
	{
	}

void CHwTlsTokenType::OpenToken(const TDesC&, MCTToken*& aToken,
						   TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	HWTLSTOKEN_LOG( _L("CHwTlsTokenType::OpenToken (by name)") ) 
	TInt err = KErrNone;
	
	TRAP( err, ( aToken = CHwTlsToken::NewL(*iTokenInfo, *this) ) )
	TRequestStatus* r = &aStatus;
	if( aToken )
		{
		User::RequestComplete(r, KErrNone);
		IncReferenceCount();
		}
	else
		User::RequestComplete(r, err);
		
	}

void CHwTlsTokenType::OpenToken(TCTTokenHandle,
										 MCTToken*& aToken,
										 TRequestStatus& aStatus)
	{
	HWTLSTOKEN_LOG( _L("CHwTlsTokenType::OpenToken (by Handle)") ) 
	TInt err = KErrNone;
	
	TRAP( err, ( aToken = CHwTlsToken::NewL(*iTokenInfo, *this) ) )
	TRequestStatus* r = &aStatus;
	if( aToken )
		{
		User::RequestComplete(r, KErrNone);
		IncReferenceCount();
		}
	else
		User::RequestComplete(r, err);
		
	}

void CHwTlsTokenType::CancelOpenToken()
	{
	}


CHwTlsTokenType::~CHwTlsTokenType()
	{
	HWTLSTOKEN_LOG( _L("CHwTlsTokenType::~CHwTlsTokenType") )
	delete iTokenInfo;
	}

	
CHwTlsTokenType* CHwTlsTokenType::InitL()
{
	CHwTlsTokenType* that = new (ELeave) CHwTlsTokenType;
	CleanupStack::PushL(that);
	HWTLSTOKEN_LOG( _L("CHwTlsTokenType::InitL") )
	that->iTokenInfo = KHwTlsTokenInfo().AllocL(); 
	CleanupStack::Pop(that);
	CREATE_HWTLSTOKEN_LOG( _L("CHwTlsTokenType initialised") ) 
	return that;
}


const TImplementationProxy ImplementationTable[] = 
{
	IMPLEMENTATION_PROXY_ENTRY(KHwTlsTokenTypeUid,	CHwTlsTokenType::InitL)
};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

	return ImplementationTable;
}

