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
 @file encryptstep.cpp
 @internalTechnology
*/
#include "encryptstep.h"

#include <tlsprovinterface.h>
#include <x509cert.h>
#include <asymmetric.h>
#include <asymmetrickeys.h>
#include <asnpkcs.h>

CEncryptStep::CEncryptStep()
	{
	SetTestStepName(KEncryptStep);
	}
	
TVerdict CEncryptStep::doTestStepPreambleL()
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

TVerdict CEncryptStep::doTestStepL()
	{
	TInt result = CEncryptStep::doTestStepImplL();
	
	Logger().WriteFormat(_L("Test Result Was: %d."), result);
	
	TInt expectedResult(KErrNone);
	GetIntFromConfig(ConfigSection(), KExpectedResult, expectedResult);
	if (expectedResult != KErrNone)
		{
		Logger().WriteFormat(_L("Expected Validation Result Was: %d."), expectedResult);
			
		if (result == expectedResult)
			{
			Logger().Write(_L("Test step passed."));
			SetTestStepResult(EPass);
			}
		else
			{
			Logger().Write(_L("Test step failed."));
			SetTestStepResult(EFail);
			}
		}
	else
		{
			SetTestStepResult((result!=KErrNone) ? EFail : EPass);
		}
	
	return TestStepResult();
	}
	
TInt CEncryptStep::doTestStepImplL()	
	{
	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Cannot retrieve supported cipher suites! (Error %d)"), err);
		return err;
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
		return err;
		}
	
	INFO_PRINTF1(_L("Calling TLS session key exchange."));
	
	HBufC8* keyExMessage = NULL;
	err = ClientKeyExchange(keyExMessage);
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Key exchange failed! (Error %d)"), err);
		delete keyExMessage;
		return err;
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
	VerifyEncryptionL();
	
	return err;
	}
	

void CEncryptStep::VerifyEncryptionL()
	{
	// Read in the parameters we'll use to configure this step...
	// Record Size - The size of the record to encrypt
	TInt recordSize(0);
	if (!GetIntFromConfig(ConfigSection(), KRecordSize, recordSize))
		{
		INFO_PRINTF1(_L("Failed! Could not read test record size from config!"));
		SetTestStepResult(EFail);
		return;
		}
		
	// Record type - The type to record in the MAC
	TInt recordTypeInt(0);
	if (!GetIntFromConfig(ConfigSection(), KRecordType, recordTypeInt))
		{
		INFO_PRINTF1(_L("Failed! Could not read test record type from config!"));
		SetTestStepResult(EFail);
		return;
		}
	TRecordProtocol recordType = (TRecordProtocol)recordTypeInt; // cast it to the enum.
	
	
	// Sequence number - The (fake) sequence number this packet is supposed to have arrived in
	TInt sequenceNumber(0);
	if (!GetIntFromConfig(ConfigSection(), KSequenceNumber, sequenceNumber))
		{
		INFO_PRINTF1(_L("Failed! Could not read test record sequence number from config!"));
		SetTestStepResult(EFail);
		return;		
		}
	
	// construct a block of (random) data of the appropriate size
	HBufC8* record = HBufC8::NewLC(recordSize);
	TPtr8 ptr = record->Des();
	ptr.SetLength(recordSize);
	TRandom::RandomL(ptr);
	
	// Now, ask TLS provider to encrypt the block and put it through our own, and
	// ensure they agree.
	
	INFO_PRINTF1(_L("Calling TLS Session to encrypt test record."));
	
	TInt64 seq = sequenceNumber;
	HBufC8* tlsProvRecord = NULL;
	TInt err = Session()->EncryptL(*record, tlsProvRecord, seq, recordType);
	if (err != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}
	
	CleanupStack::PushL(tlsProvRecord);
	
	INFO_PRINTF1(_L("Creating our own version of the test record."));
	
	
	HBufC8* ourRecord = EncryptRecordL(*record, seq, recordType, EFalse);
		
	if (*ourRecord == *tlsProvRecord)
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF1(_L("Failed! Encrypted record is corrupt!"));
		SetTestStepResult(EFail);
		}
		
			
	delete ourRecord;
	CleanupStack::PopAndDestroy(2, record); // tlsProvRecord
	}
