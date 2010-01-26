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
 @file newkeyderivationstep.cpp
 @internalTechnology
*/
#include "newtlsstepbase.h"
#include "newkeyderivationstep.h"
#include "psuedorandom.h"

#include <tlsprovinterface.h>

CNewKeyDerivationStep::CNewKeyDerivationStep()
	{
	SetTestStepName(KNewKeyDerivationStep);
	}
	

// These methods are been used because due to private inheritance some objects
// of TLS provider cannot be added directly into cleanup stack 	
void CleanupCTlsProvider(TAny* aCTLSProvider)
	{
	CTLSProvider * objToDelete=(CTLSProvider*)aCTLSProvider;
	delete objToDelete;
	}

void CleanupCTlsSession(TAny* aCTLSSession)
	{
	CTLSSession * objToDelete=(CTLSSession*)aCTLSSession;
	delete objToDelete;
	}	

void CleanupSuiteArray(TAny* aArraySuites)
	{
	RArray<TTLSCipherSuite> * objToDelete=(RArray<TTLSCipherSuite>*)aArraySuites;
	objToDelete->Close();
	objToDelete = NULL;
	}	

void CleanupKeyMaterial(TAny* aTestStep)
	{
	CNewKeyDerivationStep *testStepPointer =(CNewKeyDerivationStep*) aTestStep;

	// Cleans and delete item by item.
	if( testStepPointer->iClientMacSecret)
		{
		delete testStepPointer->iClientMacSecret;
		testStepPointer->iClientMacSecret = NULL;	
		}

	if( testStepPointer->iServerMacSecret)
		{
		delete testStepPointer->iServerMacSecret;
		testStepPointer->iServerMacSecret = NULL;	
		}

	if( testStepPointer->iClientWriteSecret)
		{
		delete testStepPointer->iClientWriteSecret;
		testStepPointer->iClientWriteSecret = NULL;	
		}

	if( testStepPointer->iServerWriteSecret)
		{
		delete testStepPointer->iServerWriteSecret;
		testStepPointer->iServerWriteSecret = NULL;	
		}

	if( testStepPointer->iClientInitVector)
		{
		delete testStepPointer->iClientInitVector;
		testStepPointer->iClientInitVector = NULL;	
		}

	if( testStepPointer->iServerInitVector)
		{
		delete testStepPointer->iServerInitVector;
		testStepPointer->iServerInitVector = NULL;	
		}
	}		
	
/************************************************************************
*    Note: This test steps currently only supports DH and PSK
*          key exchanges, to test additional algorithms the test step
*          need to be updated.
**************************************************************************/	

void CNewKeyDerivationStep::doTestL()
	{
	TInt err;
	iProvider = CTLSProvider::ConnectL();
	TCleanupItem cleanupInsProvider(CleanupCTlsProvider, iProvider);
	CleanupStack::PushL(cleanupInsProvider);

	// cleanup of key material.
	TCleanupItem cleanupInsKeyMaterial(CleanupKeyMaterial, this);
	CleanupStack::PushL(cleanupInsKeyMaterial);

	CTlsCryptoAttributes* atts = iProvider->Attributes();

	// sets the "server" random
	atts->iMasterSecretInput.iServerRandom.Copy(*iServerRandom);

	// Sets the client random
	atts->iMasterSecretInput.iClientRandom.Copy(*iClientRandom);

	// we only support null compression...
	atts->iCompressionMethod = ENullCompression;

	// Sets cipher suite for the test. 
	atts->iCurrentCipherSuite = iCipherSuite;

	// Sets protocol version. 
	atts->iNegotiatedProtocol = iProtocolVersion;
	atts->iProposedProtocol = iProtocolVersion;

	// set the session ID and "server" name (localhost)
	atts->iSessionNameAndID.iSessionId = iSessionId;
	atts->iSessionNameAndID.iServerName.iAddress = KLocalHost; 
	atts->iSessionNameAndID.iServerName.iPort = 443;  
	atts->idomainName.Copy(iDomainName);  

	// If cipher suite under test uses PSK (Pre Shared Key)
	if(UsePsk())
		{
		// Populates values for PSK 
		atts->iPskConfigured = true;
		atts->iPublicKeyParams->iKeyType = EPsk;

		atts->iPublicKeyParams->iValue4 = PskIdentity()->Alloc(); // = PskIdentity();
		atts->iPublicKeyParams->iValue5 = PskKey()->Alloc();

		}   
	else 
		{
		if(iUseDHParams)
			{

			// If cipher suite under test is NOT PSK 
			atts->iPublicKeyParams->iKeyType = EDHE;

			// The params are:
			// 1 - Prime
			// 2 - Generator
			// 3 - generator ^ random mod prime

			atts->iPublicKeyParams->iValue1 = Prime().BufferLC();
			CleanupStack::Pop(atts->iPublicKeyParams->iValue1);

			atts->iPublicKeyParams->iValue2 = Generator().BufferLC();
			CleanupStack::Pop(atts->iPublicKeyParams->iValue2);

			atts->iPublicKeyParams->iValue3 = KeyPair()->PublicKey().X().BufferLC();
			CleanupStack::Pop(atts->iPublicKeyParams->iValue3);
			}
		} 

	// No client authentication or dialogs for this test, please
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;

	if(UseNullCipher())
		{
		// Enables null cipher by setting appropiate parameter  
		atts->iAllowNullCipherSuites = ETrue;
		}  

	err = LeanGetCipherSuitesL();

	if (err != KErrNone)
		{
		User::Leave(iActive->iStatus.Int());
		}


	TCleanupItem cleanupInsSuitesArray(CleanupSuiteArray,&iSuites);
	CleanupStack::PushL(cleanupInsSuitesArray);


	// verifies certificate if is not a PSK cipher suite
	if( !UsePsk() )
		{

		// we have to verify the server certificate, to supply the certificate
		// and its parameters to the TLS provider.
		CX509Certificate* cert = NULL;

		err = LeanVerifyServerCertificate(cert, iServerCertificate);
		delete cert;

		// make sure it completed sucessfully.
		if (err != KErrNone)
			{
			User::Leave(iActive->iStatus.Int());
			}
		}    	

	// now, create a session with the parameters set
	err = LeanCreateSession();

	// ensure we succeeded
	if (err != KErrNone)
		{
		User::Leave(iActive->iStatus.Int());
		}

	TCleanupItem cleanupInsTlsSession(CleanupCTlsSession, iSession);
	CleanupStack::PushL(cleanupInsTlsSession);

	HBufC8* keyExMessage = NULL;
	err = LeanClientKeyExchange(keyExMessage);

	if (err != KErrNone)
		{
		User::Leave(iActive->iStatus.Int());
		}

	CleanupStack::PushL(keyExMessage);   
	HBufC8* premaster = LeanDerivePreMasterSecretL(*keyExMessage, iServerPrivateKey);
	CleanupStack::PopAndDestroy(keyExMessage);  

	// compute the master secret from the premaster.
	CleanupStack::PushL(premaster);
	HBufC8* master = LeanComputeMasterSecretL(*premaster);
	CleanupStack::PopAndDestroy(premaster);
	CleanupStack::PushL(master);

	// now generate what we think the derived EAP key block should look like.
	TBuf8<192> ourEAP;
	TBuf8<64> random;
	random.Append(atts->iMasterSecretInput.iClientRandom);
	random.Append(atts->iMasterSecretInput.iServerRandom);

	// make sure we're using TLS. This step makes no sense for SSL 3.0
	if (atts->iNegotiatedProtocol.iMajor == 3 && atts->iNegotiatedProtocol.iMinor == 0)
		{
		INFO_PRINTF1(_L("Error! Cannot use this test step with SSLv3!"));
		User::Leave(KErrNotSupported);
		}

	// compute the 128 byte block that uses the master secret as key.
	_LIT8(KEAPEncryptionLabel, "client EAP encryption");
	HBufC8* block1 = CTls10PsuedoRandom::PseudoRandomL(*master, KEAPEncryptionLabel, random, 128);
	ourEAP.Append(*block1);
	delete block1;

	// compute the 64 byte IV block
	HBufC8* block2 = CTls10PsuedoRandom::PseudoRandomL(KNullDesC8, KEAPEncryptionLabel, random, 64);
	ourEAP.Append(*block2);
	delete block2;

	// get the TLS provider's idea of what the EAP keyblock should be, and check they match.
	TBuf8<192> theirEAP;
	User::LeaveIfError(iSession->KeyDerivation(KEAPEncryptionLabel, atts->iMasterSecretInput, theirEAP));

	if (ourEAP == theirEAP)
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF1(_L("Failed! EAP-TLS is corrupt!"));	
		SetTestStepResult(EFail);
		}  
	
	CleanupStack::PopAndDestroy(5);

	}
	
