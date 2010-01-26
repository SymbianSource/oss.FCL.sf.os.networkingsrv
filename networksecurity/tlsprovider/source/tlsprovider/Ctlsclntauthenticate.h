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
// This file does the implementation for encryption , decryption, mac computation and PRFL
// Security component: TLS Provider 
// 
//

/**
 @file 
 @internalTechnology
*/

#ifndef __TLSCLNTAUTHENTICATE_H__
#define __TLSCLNTAUTHENTICATE_H__


#include <e32std.h>
#include <e32base.h>

#include "tlstypedef.h"
#include "tlsprovider_log.h"
#include "tlsproviderpolicy.h"

#include <unifiedcertstore.h>
#include <unifiedkeystore.h>
#include <secdlg.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif

class CTlsClntAuthenticate : public CActive
	{
public:	
	
	static CTlsClntAuthenticate* NewL(const CTlsCryptoAttributes& aTlsCryptoAttributes,
											 HBufC8* aEncodedServerCerts);

	void DoClientAuthenticate(CCTCertInfo*& aSelectedCertInfo,
							  CCTKeyInfo*& aSelectedKeyInfo,
							  RPointerArray<CCertificate>* aStoredIntermediatesCACertificates,
							  TRequestStatus& aStatus);
	void CancelRequest();

	~CTlsClntAuthenticate();
	
private:

	//Active
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);

	CTlsClntAuthenticate(const CTlsCryptoAttributes& aTlsCryptoAttributes,
						 HBufC8* aEncodedServerCerts);

	//Local functions	
	TBool LookupCert(CX509Certificate* aCert);
	void GetAvailableKeyListL();
	void OnEGetCertListL();
	void OnEGetKeyListL();
	void OnGetAvailableKeyListL();
	void OnEUserDialogL();
 	void OnEListIntermediatesCACertsL();
 	void OnEGetIntermediatesIssuerL();		

 	void RemoveExpiredCertificatesL();
 	CCTKeyInfo* GetKeyInfoL(const TKeyIdentifier& aKeyID);
	
private:
	enum TStateLists {	ENullState,
						EUserDialog,
						EGetCertList,
						EGetKeyList,
 						EGetAvailableKeyList,						
 						EListIntermediatesCACerts,
 						EGetIntermediatesIssuer
 						};   

	//Pointers owned by this class
	CUnifiedKeyStore*  iPtrUnifiedKeyStore;
	CUnifiedCertStore* iPtrUnifiedCertStore;
	CCertAttributeFilter* iPtrFilter;
	MSecurityDialog* iSecurityDialog;
	
	//The actual Pointer owned by CTlsSessionImpl
	CCTCertInfo** iSelectedCertInfo; 
	CCTKeyInfo** iSelectedKeyInfo;

	//Data owned by CTlsProviderImpl
	const CTlsCryptoAttributes& iTlsCryptoAttributes; 


	RPointerArray<MCTKeyStore> iSupportedKeyStores;
	RMPointerArray<CCTKeyInfo> iKeyInfos; 
	RMPointerArray<CCTCertInfo> iCertInfos;	

	TCTTokenObjectHandle iUserSelectedCert;	
	
	TRequestStatus* iOriginalRequestStatus;
	
	TStateLists iCurrentState;
	TInt iCertTypeCount;
	TInt iKeyCount;	
	RFs iFs;

	HBufC8* iEncodedServerCerts; 
	TBool iAuthenticateOK;

 	RArray<TCTTokenObjectHandle> 	iClientCertHandleList;
 	RPointerArray<const TDesC8> 	iFullIssuerDistinguishedCANames;
 	RMPointerArray<CCTCertInfo> 	iIntermediatesCACertInfos;
 	RPointerArray<CCertificate>*	iStoredIntermediatesCACertificates;
 	TInt							iIntermediatesCACertCount;
 	CCertAttributeFilter*			iPtrIntermediatesCACertFilter;
 	CCertificate*					iIntermediatesCertificate;
 	TBool							iToListIntermediatesCACert;
 	RPointerArray<const TDesC8> 	iCurrentIssuerDistinguishedCANames;	
 	TBool							iClientAuthenticationDlgEnabled;
	};

#endif
