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
 @file ciphersuitestep.cpp
 @internalTechnology
*/
#include "ciphersuitesstep.h"

#include <tlsprovinterface.h>

CCipherSuitesStep::CCipherSuitesStep()
	{
	SetTestStepName(KCipherSuitesStep);
	}

TVerdict CCipherSuitesStep::doTestStepPreambleL()
	{
	// No tls setup required.
	ConstructL();
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	// Reads if NULL ciphers suites are to be allowed from INI file.
	ReadUseNullCipher();
	if(UseNullCipher())
		{
		// Enables null cipher by setting appropiate parameter  
		atts->iAllowNullCipherSuites = ETrue;
 		}		
		
		
	
	// Reads PSK values if included in INI file.
	ReadPskToBeUsedL();
	
	// Reads if NULL ciphers suites are to be allowed from INI file.
	ReadUseNullCipher();
	
	
	// If cipher suite under test is uses PSK (Pre Shared Key)
	if(UsePsk())
		{
		// Populates values for PSK 
		atts->iPskConfigured = true;
		}


	if(UseNullCipher())
		{
		// Enables null cipher by setting appropiate parameter  
		atts->iAllowNullCipherSuites = ETrue;
 		}		
		
		
	// read the list of cipher suites (in order) that
	// we think we ought to be supporting.
	TInt numCiphers(0);
	if (!GetIntFromConfig(ConfigSection(), KNumCipherSuites, numCiphers))
		{
		User::Leave(KErrNotFound);
		}
		
	INFO_PRINTF1(_L("Reading expected cipher suited from ini."));
		
	for (TInt i = 1; i <= numCiphers; ++i)
		{
		TBuf<128> suiteTag;
		suiteTag.Append(KCipherSuiteBase);
		suiteTag.AppendNum(i);
		
		TInt cipherSuite(0);
		if (!GetHexFromConfig(ConfigSection(), suiteTag, cipherSuite))
			{
			User::Leave(KErrNotFound);
			}
			
		TTLSCipherSuite suite;
		suite.iLoByte = cipherSuite & 0xFF;
		suite.iHiByte = (((TUint)cipherSuite) >> 8) & 0xFF;
		
		iSuites.AppendL(suite);
		}
	return EPass;
	}

TVerdict CCipherSuitesStep::doTestStepL()
	{
	SetTestStepResult(EFail);
	INFO_PRINTF1(_L("Fetching TLS Provider supported cipher suites."));
	TInt err = GetCipherSuitesL();
	if (err != KErrNone)
		{
		INFO_PRINTF1(_L("Failed! Could not get supported cipher suites from TLS Provider!"));
		}
	else if (CipherSuites().Count() != iSuites.Count())
		{
		INFO_PRINTF3(_L("Failed! TLS Provider supports %d suites, expecting %d."),
			CipherSuites().Count(), iSuites.Count());
		}
	else
		{
		for (TInt i = 0; i < iSuites.Count(); ++i)
			{
	 	
			if (!(iSuites[i] == CipherSuites()[i]))
				{
				INFO_PRINTF1(_L("Failed! Cipher suites or order different from expected!"));
 				return TestStepResult();
				}
			}
		}
	
	// does nothing significant but increases code coverage.
	Provider()->ReConnectL();
	Provider()->ReConnectL();
	err = GetCipherSuitesL();
	Provider()->ConnectL();
	Provider()->ReConnectL();
		
	if (err != KErrNone)
		{
		INFO_PRINTF1(_L("Failed! Could not get supported cipher suites from TLS Provider!"));
		}
	else if (CipherSuites().Count() != iSuites.Count())
		{
		INFO_PRINTF3(_L("Failed! TLS Provider supports %d suites, expecting %d."),
			CipherSuites().Count(), iSuites.Count());
		}
	else
		{
		SetTestStepResult(EPass);
		for (TInt i = 0; i < iSuites.Count(); ++i)
			{
			if (!(iSuites[i] == CipherSuites()[i]))
				{
				INFO_PRINTF1(_L("Failed! Cipher suites or order different from expected!"));
			
				SetTestStepResult(EFail);
				break;
				}
			}
		}
	return TestStepResult();
	}
	
CCipherSuitesStep::~CCipherSuitesStep()
	{
	iSuites.Close();
	}
