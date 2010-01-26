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
 @file decryptstep.cpp
 @internalTechnology
*/
#include "decryptstep.h"

#include <tlsprovinterface.h>
#include <x509cert.h>
#include <asymmetric.h>
#include <asymmetrickeys.h>
#include <asnpkcs.h>

CDecryptStep::CDecryptStep()
	{
	SetTestStepName(KDecryptStep);
	}
	
TVerdict CDecryptStep::doTestStepPreambleL()
	{
	ConstructL();
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	// Reads PSK values if included in INI file.
	ReadPskToBeUsedL();
	
	// Reads if NULL ciphers suites are to be allowed from INI file.
	ReadUseNullCipher();
	
	// read the "server" random
	HBufC8* random = ServerRandomL();
	atts->iMasterSecretInput.iServerRandom.Copy(*random);
	delete random;
	
	// and the client random
	random = ClientRandomL();
	atts->iMasterSecretInput.iClientRandom.Copy(*random);
	delete random;
	
	// we only support null compression...
	atts->iCompressionMethod = ENullCompression;
	
	// read the cipher suite for the test
	atts->iCurrentCipherSuite = CipherSuiteL();
	
	// read the protocol version
	TTLSProtocolVersion version = ProtocolVersionL();
	atts->iNegotiatedProtocol = version;
	atts->iProposedProtocol = version;
	
	// set the session ID and "server" name (localhost)
	atts->iSessionNameAndID.iSessionId = SessionId();
	atts->iSessionNameAndID.iServerName.iAddress = KLocalHost; 
	atts->iSessionNameAndID.iServerName.iPort = 443;
	atts->idomainName.Copy(DomainNameL());
	
	// If cipher suite under test is uses PSK (Pre Shared Key)
	if(UsePsk())
		{
		// Populates values for PSK 
		atts->iPskConfigured = true;
		atts->iPublicKeyParams->iKeyType = EPsk;
		atts->iPublicKeyParams->iValue4 = PskIdentity();
		atts->iPublicKeyParams->iValue5 = PskKey();
		}
	else 
		{
		// If cipher suite under test is NOT PSK 
		TRAPD(err, ReadDHParamsL());
		if (err == KErrNone)
			{
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
	
	return EPass;
	}
	
TVerdict CDecryptStep::doTestStepL()
	{
	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Cannot retrieve supported cipher suites! (Error %d)"),
			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// verifies certificate if is not a PSK cipher suite
  	if( !UsePsk() )
		{
			// we have to verify the server certificate, to supply the certificate
		// and its parameters to the TLS provider.

		INFO_PRINTF1(_L("Calling TLS Provider to verify server certificate."));

		CX509Certificate* cert = NULL;

		err = VerifyServerCertificateL(cert);
		delete cert;
		
			// make sure it completed sucessfully.
		if (err != KErrNone)
			{
			INFO_PRINTF2(_L("Failed! Server Certificate did not verify correctly! (Error %d)"),
				err);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		
		}   		
		
	
	INFO_PRINTF1(_L("Creating TLS Session."));	
	
	// now, create a session with the parameters set in the preamble
	err = CreateSessionL();
	
	// ensure we succeeded
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	INFO_PRINTF1(_L("Calling TLS session key exchange."));
	
	HBufC8* keyExMessage = NULL;
	err = ClientKeyExchange(keyExMessage);
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Key exchange failed! (Error %d)"), err);
		delete keyExMessage;
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	INFO_PRINTF1(_L("Deriving premaster secret."));
	
	// derive the premaster secret from the key exchange method	
	CleanupStack::PushL(keyExMessage);
	HBufC8* premaster = DerivePreMasterSecretL(*keyExMessage);
	CleanupStack::PopAndDestroy(keyExMessage);
	
	INFO_PRINTF1(_L("Deriving master secret."));
	
	// compute the master secret from the premaster.
	CleanupStack::PushL(premaster);
	HBufC8* master = ComputeMasterSecretL(*premaster);
	CleanupStack::PopAndDestroy(premaster);
	delete master;
	
	// Do the main meat of the test
	VerifyDecryptionL();
	
	return TestStepResult();
	}
	
void CDecryptStep::VerifyDecryptionL()
	{
	TInt expectedError(KErrNone);
	
	// Read in the parameters we'll use to configure this step...
	// Record Size - The size of the record to encrypt
	TInt recordSize(0);
	if (!GetIntFromConfig(ConfigSection(), KDRecordSize, recordSize))
		{
		INFO_PRINTF1(_L("Failed! Could not read test record size from config!"));
		SetTestStepResult(EFail);
		return;
		}
		
	// Record type - The type to record in the MAC
	TInt recordTypeInt(0);
	if (!GetIntFromConfig(ConfigSection(), KDRecordType, recordTypeInt))
		{
		INFO_PRINTF1(_L("Failed! Could not read test type size from config!"));
		SetTestStepResult(EFail);
		return;
		}
	TRecordProtocol recordType = (TRecordProtocol)recordTypeInt; // cast it to the enum.
	
	// Sequence number - The (fake) sequence number this packet is supposed to have arrived in
	TInt sequenceNumber(0);
	if (!GetIntFromConfig(ConfigSection(), KDSequenceNumber, sequenceNumber))
		{
		INFO_PRINTF1(_L("Failed! Could not read test record sequence number from config!"));
		SetTestStepResult(EFail);
		return;		
		}
		
	// Security checking, we may tamper with some parameters
	
	TInt serverRecordTypeInt(0);
	if (!GetIntFromConfig(ConfigSection(), KTamperedRecordType, serverRecordTypeInt))
		{
		// if this parameter isn't present, use the same value (no tampering)
		serverRecordTypeInt = recordTypeInt;
		}
	else
		{
		INFO_PRINTF3(_L("Using tampered record type of %d (original %d)."), serverRecordTypeInt, recordTypeInt);
		expectedError = KErrSSLAlertBadRecordMac;
		}
	TRecordProtocol serverRecordType = (TRecordProtocol)serverRecordTypeInt;
	
	TInt serverSequenceNumber(0);
	if (!GetIntFromConfig(ConfigSection(), KTamperedSequenceNumber, serverSequenceNumber))
		{
		serverSequenceNumber = sequenceNumber;
		}
	else
		{
		INFO_PRINTF3(_L("Using tampered sequence number of %d (original %d)."), serverSequenceNumber, sequenceNumber);
		expectedError = KErrSSLAlertBadRecordMac;
		}
	
	// Generate a block of random data of the size specified in the test ini
	HBufC8* record = HBufC8::NewLC(recordSize);
	TPtr8 ptr = record->Des();
	ptr.SetLength(recordSize);
	TRandom::RandomL(ptr);
	
	INFO_PRINTF1(_L("Creating server encrypted record."));
	
	TInt64 serverSeq = serverSequenceNumber;
	// encrypt the block with the server parameters
	HBufC8* ourRecord = EncryptRecordL(*record, serverSeq, serverRecordType, ETrue);
	CleanupStack::PushL(ourRecord);

	INFO_PRINTF1(_L("Calling TLS Session to decrypt record."));
	
	TInt64 seq = sequenceNumber;
	// now, call TLS Provider's DecryptAndVerifyL on the record
	HBufC8* decryptedRecord = NULL;

	TInt err = Session()->DecryptAndVerifyL(*ourRecord, decryptedRecord, seq, recordType);
	CleanupStack::PopAndDestroy(ourRecord);	
	
	// check the error is the expected error...
	if (err != expectedError)
		{
		INFO_PRINTF3(_L("Failed! Error returned (%d) did not match expected error (%d)!"),
			err, expectedError);
		SetTestStepResult(EFail);
		}
	// if it is, check the decrypted block was the same
	else if (*decryptedRecord != *record)
		{
		INFO_PRINTF1(_L("Failed! Decrypted block is corrupt!"));
		SetTestStepResult(EFail);
		return;
		}
	else
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}

	delete decryptedRecord;
	CleanupStack::PopAndDestroy(record);
	

	}
