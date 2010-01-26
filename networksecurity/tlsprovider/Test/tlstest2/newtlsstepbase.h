// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file newtlsstepbase.h
 @internalTechnology	
*/
#ifndef __CNEWTLSSTEPBASE_H__
#define __CNEWTLSSTEPBASE_H__

#include "tlsstepbase.h"

#include <asymmetric.h>
#include <asymmetrickeys.h>   
#include <symmetric.h>  
#include <asnpkcs.h>   
#include "tlsgenericactive.h"  


/**
* 
*  About this new test step base:
*  This new test step base is derived from existing test base "CTlsStepBase" but
*  has been optimised for out of memory test.
*  Main differences with previous test base:
*  -  All reading of values form INI file had been moved to test pre-amble.
*  -  No production code under test had been initialised in test pre-amble.
*  -  Includes OOM test facilities. 
*
*/


class CNewTlsStepBase : public CTlsStepBase
	{
	public:
	
	~CNewTlsStepBase();
	
	virtual TVerdict doTestStepPreambleL(); 
	virtual TVerdict doTestStepL();
 	virtual TVerdict doOOMTestL();
	virtual void doTestL();
	void ConstructL();

	// These "Lean" functions are base on fuctions with similar name in 
	// first base but they do not read any INI value when called. 

	// Generic methods.
	TInt LeanVerifyServerCertificate(CX509Certificate*& aCertOut, HBufC8* aCertIn);
	TInt LeanCreateSession();
	TInt LeanClientKeyExchange(HBufC8*& aMessageOut);
	HBufC8* LeanDerivePreMasterSecretL(const TDesC8& aClientKeyExMessage, CDecPKCS8Data* aServerKeyData);
	TInt LeanCipherSuiteIndex(const TTLSCipherSuite& aSuite);
	HBufC8* LeanComputeMasterSecretL(const TDesC8& aPremasterSecret);
	HBufC8* LeanComputeSslMasterSecretL(const TDesC8& aPremasterSecret);
	HBufC8* LeanComputeTlsMasterSecretL(const TDesC8& aPremasterSecret);
	void LeanComputeTlsCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom);
	void LeanComputeSslCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom);
	TInt LeanGetCipherSuitesL();
				
	public:

	TBool iOOMCondition;
	TBool iOOMAllowNonMemoryErrors;
	TBool iUseDHParams;
 	CGenericActive* iActive;
	
	// Data that should deleted by test step destructor
	HBufC8*				iServerRandom;
	HBufC8*				iClientRandom;
	TTLSCipherSuite		iCipherSuite;
	TTLSProtocolVersion iProtocolVersion;
	TTLSSessionId		iSessionId;
	TPtrC				iDomainName;
	
	HBufC8*				iServerCertificate;
	CTLSSession*		iSession;
	CDecPKCS8Data*      iServerPrivateKey;

	// Data that has to be reset for each OOM cycle..
    CActiveScheduler* iSched;
	CTLSProvider* iProvider;
	RArray<TTLSCipherSuite> iSuites;

	HBufC8* iClientMacSecret;
	HBufC8* iServerMacSecret;

	HBufC8* iClientWriteSecret;
	HBufC8* iServerWriteSecret;

	HBufC8* iClientInitVector;
	HBufC8* iServerInitVector;	
	
		
	};

#endif /* __CNEWTLSSTEPBASE_H__ */
