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
 
#include "T_TLS_cntrl.h"

//
// Constructors
//
CController* CController::NewL()
	{
	CController* self = new(ELeave) CController;
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();		
	return self;
	}

void CController::ConstructL()
	{

	TInt i;

	// Create the kernel side timer object
	User::LeaveIfError( iTimer.CreateLocal() );
	
	// Create the array of CSSLTest objects
	for ( i=0; i<KMaxSSLConnections; i++ )
		{
		User::LeaveIfNull( iTLSTest[i] = CTLSTest::NewL() );
		}

	// Add this object to the active scheduler
	CActiveScheduler::Add( this );

#ifdef NODIALOGS
	// create a global semaphore that the dialog server searches for to
	// decide if "trust" dialogs should be displayed or not
	if ( iSemaphore.CreateGlobal( KSemaphoreName, 0 ) != KErrNone )
		{
		iTestStep->Log( _L("Semaphore creation failed.") );
		}
#endif
	}


//
// Give the controller a slightly higher priority than normal
//
CController::CController() : CActive(1) 
	{
	}

CController::~CController()
/** 
 * Destructor
 */
{
	Cancel();

#ifdef NODIALOGS
	iSemaphore.Close();
#endif

	for ( TInt i=0; i<KMaxSSLConnections; i++ )
		{
		delete iTLSTest[i];	
		}
}


void CController::Start( CTestStepTls * aTestStep )
/**
 * Start
 */
{
	iTestStep = aTestStep;

	for ( TInt i=0; i<KMaxSSLConnections; i++ )
		{
		iTLSTest[i]->SetConsole( iTestStep );
		}

	iRunState = EReadNextSite;

	iTimer.After( iStatus, KControllerDelayTime );
	SetActive();
}


void CController::RunL()
/**
 * RunL() method
 */
{
	TInt i; // used as a loop counter
	CActiveScheduler *currentScheduler;
	TBool start = ETrue;
	
	TPtrC aPtrResult;
	TPtrC* res=&aPtrResult;

	switch( iRunState )
	{
	case EReadNextSite:
		{	// try and read a site description from the file iAddress
			if ( iTestStep->GetStringFromConfig(KSectionName, KCfgIPAddress, aPtrResult))
				{
				iAddress.Copy( aPtrResult );
				}
			else
				{
				iAddress.Copy( KDefCfgIPAddress );
				}
		
			iTestStep->Log( _L("IPaddress: %S"), res);

			// iDNSName
			if ( iTestStep->GetStringFromConfig(KSectionName, KCfgDNSName, aPtrResult))
				{
				iDNSName.Copy( aPtrResult );
				}
			else
				{
				iDNSName.Copy( KDefCfgDNSName );
				}
		
			iTestStep->Log( _L("DNSName: %S"), res);

			// iPortNum
			if (!iTestStep->GetIntFromConfig(KSectionName, KCfgIPPort, iPortNum ))
				{
				iPortNum = KDefCfgIPPort;
				}
		
			iTestStep->Log( _L("PortNumber: %D"), iPortNum);

			// iPage
			if (iTestStep->GetStringFromConfig(KSectionName, KCfgPage, aPtrResult))
				{
				iPage.Copy( aPtrResult );
				}
			else
				{
				iPage.Copy( KDefCfgPage );
				}
		
			iTestStep->Log( _L("Page: %S"), res);

			// iCipher
			if (!iTestStep->GetIntFromConfig(KSectionName, KCfgCipher, iCipher ))
				{
				iCipher = KDefCfgCipher;
				}
			iTestStep->Log( _L("Cipher: %D"), iCipher);

			// iCipherSuites
			if ( iTestStep->GetStringFromConfig(KSectionName, KCfgCipherSuites, aPtrResult))
				{
				iCipherSuites.Copy( aPtrResult );
				}
			else
				{
				iCipherSuites.Copy( KDefCfgCipherSuites );
				}
		
			iTestStep->Log( _L("CipherSuites: %S"), res);

			// iSimpleGet
			if (!iTestStep->GetIntFromConfig(KSectionName, KCfgSimpleGet, iSimpleGet ))
				{
				iSimpleGet = KDefCfgSimpleGet;
				}
			iTestStep->Log( _L("SimpleGet: %D"), iSimpleGet);
		
			// iTestEndDelay
			if (!iTestStep->GetIntFromConfig(KSectionName, KCfgTestEndDelay, iTestEndDelay ))
				{
				iTestEndDelay = KDefCfgTestEndDelay;
				}
			iTestStep->Log( _L("TestEndDelay: %D"), iTestEndDelay);
	
			// iProtocol
			if ( iTestStep->GetStringFromConfig(KSectionName, KCfgProtocol, aPtrResult))
				{
				iProtocol.Copy( aPtrResult );
				}
			else
				{
				iProtocol.Copy( KDefCfgProtocol );
				}
			iTestStep->Log( _L("Protocol: %S"), res);

			// iUseGenericSocket
			if (!iTestStep->GetBoolFromConfig(KSectionName, KCfgUseGenericSocket, iUseGenericSocket ))
				{
				iUseGenericSocket = KDefUseGenericSocket;
				}
			iTestStep->Log( _L("UseGenericSocket: %D"), iUseGenericSocket);

			// iEAPKeyDerivation
			if (!iTestStep->GetBoolFromConfig(KSectionName, KCfgEAPKeyDerivation, iEAPKeyDerivation ))
				{
				iEAPKeyDerivation = KDefEAPKeyDerivation;
				}
			iTestStep->Log( _L("EAPKeyDerivation: %D"), iEAPKeyDerivation);

			// On to next state 
			iRunState = EFindFreeTest;			

			iTimer.After( iStatus, KControllerDelayTime );
			SetActive();
			break;
		} // case EReadNextSite
	case EFindFreeTest:
		{
			// check if this server name and port is already being tested by an object
			for ( i=0; i<KMaxSSLConnections; i++ )
			{
				if ( iTLSTest[i]->TestingSite( iAddress, iPortNum ) )
				{
					// This address and port is already being tested!
					start = EFalse;
					iRunState = EFindFreeTest;

					iTestStep->Log( _L("Test active %d"), i );
					break; // break from for loop
				}
			}
		
			if ( start )
			{
				// the server isnt already being tested, so test it!
				for ( i=0; i<KMaxSSLConnections; i++ )
				{
					// find the first unused test object
					if ( !iTLSTest[i]->InUse() )
					{
						// this test isn't active, so use it
						TInt maxConnections = KMaxSSLConnections;
						if ( maxConnections != 1 )
							{
							iTestStep->Log( _L("Using test object %d"), i );
							}

						iTLSTest[i]->ConnectL( iAddress, iPortNum, iPage, iCipherSuites, 
							iCipher, iSimpleGet, iTestEndDelay, iDNSName, iProtocol, iUseGenericSocket, iEAPKeyDerivation );

						iRunState = EWaitForComplete;
						break; // break from the for loop
					}
				} // 'for' statement
			} // 'if' statement

			iTimer.After( iStatus, KControllerDelayTime );
			SetActive();
			break;
		} // case EFreeTest
	case EWaitForComplete:
		{
			iRunState = ETestCompleted;

			for ( i=0; i<KMaxSSLConnections; i++ )
			{
				if ( iTLSTest[i]->InUse() )
				{
					iRunState = EWaitForComplete;
					break; // break from the for loop
				}
			}

			iTimer.After( iStatus, KControllerDelayTime );
			SetActive();
			break;
		} // case EWaitForComplete
	case ETestCompleted:
		{	// All the tests are finished, so now stop the active scheduler
			currentScheduler = CActiveScheduler::Current();
			currentScheduler->Stop();
			break;
		} // case ETestCompleted
	} // switch
}
	
void CController::DoCancel()
/**
 * DoCancel
 */
{
}
