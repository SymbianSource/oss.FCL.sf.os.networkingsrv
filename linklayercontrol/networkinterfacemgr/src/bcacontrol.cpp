// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "Ni_Log.h"
#include <nifutl.h>
#include "bcacontrol.h"
#include <networking/bcafactory.h>

using namespace BasebandChannelAdaptation;

typedef MBcaFactory* (*TNewBcaFactoryL)();

#ifdef _DEBUG

const TInt KBcaUnknownState = 1;

static void Panic(TInt aCode)
	{
	_LIT(KAgentBcaPanic, "AgentBca");
	User::Panic(KAgentBcaPanic, aCode);
	}

#endif

#ifdef __FLOG_ACTIVE

const TDesC8& CScriptBcaControl::StateString()
	{
	_LIT8(KIdling, "Idling");
	_LIT8(KSettingIap, "SettingIap");
	_LIT8(KSettingBcaStack, "SettingBcaStack");
	_LIT8(KOpeningBca, "OpeningBca");
	_LIT8(KBcaOpened, "BcaOpened");
	_LIT8(KClosing, "Closing");
	_LIT8(KUnknown, "UNKNOWN");
	
	switch (iState)
		{
	case EIdling:
		return KIdling();
	case ESettingIap:
		return KSettingIap();
	case ESettingBcaStack:
		return KSettingBcaStack();
	case EOpeningBca:
		return KOpeningBca();
	case EBcaOpened:
		return KBcaOpened();
	case EClosing:
		return KClosing();
	default:
		return KUnknown();
		}
	}

#endif

CScriptBcaControl::CScriptBcaControl(MBcaControlObserver* aObserver)
/**
Constructor. Performs standard active object initialisation.

@param aObserver Reference to the observer of this state machine
@param aTheLogger The logging object
*/
	: CActive(EPriorityNormal), 
	  iObserver(aObserver), 
	  iMBca(NULL),
	  iState(EIdling),
	  iError(KErrNone)
	{
	CActiveScheduler::Add(this);
	}
	
CScriptBcaControl::~CScriptBcaControl()
/**
Destructor.
*/
	{
	LOG( NifmanLog::Printf(_L8("CScriptBcaControl %08x:\t~CScriptBcaControl"), this));
	Cancel();
	if(iMBca)
		{
		iMBca->Release();		// causes BCA to destroy itelf
		iMBca = NULL;
		}
	// Library will be closed when iBcaDll is destroyed.
	}

void CScriptBcaControl::RunL()
/**
Called after request is completed. 
*/
	{
	LOG( NifmanLog::Printf(_L8("CScriptBcaControl %08x:\tRunL(), iState '%S'"), this, &StateString()));
	switch (iState)
		{
		case ESettingIap:
			{
			// In this state, Ioctl has called to set IAP ID.  Check the result of
			// Ioctl, then either set the BCA stack with another Ioctl call, 
			// open the BCA (if there's no BCA stack to set), or stop the NIF.
			if(iStatus == KErrNone || iStatus == KErrNotSupported)
				{
#ifdef __FLOG_ACTIVE
				if(iStatus == KErrNotSupported)
					{
					NifmanLog::Printf(_L8("\tBCA does not support KBCASetIapId"));
					}
				else
					{
					NifmanLog::Printf(_L8("\tBCA supports KBCASetIapId"));
					}
#endif				
				const TDesC& bcaStack = iObserver->BcaStack();
				if(bcaStack.Length())
					{
					TBuf8<KMaxName> remainingBcaStack8;
					remainingBcaStack8.Copy(bcaStack);
					iMBca->Ioctl(iStatus, KBcaOptLevelGeneric,KBCASetBcaStack,remainingBcaStack8);
					}
				else
					{
					TRequestStatus* statusPtr=&iStatus;
					User::RequestComplete(statusPtr,KErrNone);
					}
				iState = ESettingBcaStack;
				SetActive();	
				}
			else
				{
				LOG( NifmanLog::Printf(_L8("\tERROR %d in KBCASetIapId"), iStatus.Int()));
				iObserver->Stop(iStatus.Int());
				}
			
			break;
			}
			
		// In this case, we receive the result of Ioctl call to set Bca Stack.
		// Check the result of Ioctl, then Open the Bca or stop the NIF
		case ESettingBcaStack:
			{
			if(iStatus == KErrNotSupported || iStatus == KErrNone)
				{
				if(iStatus == KErrNotSupported)
					{
					LOG( NifmanLog::Printf(_L8("\tBCA does not support KBCASetBcaStack")));
					}
				else
					{
					LOG( NifmanLog::Printf(_L8("\tBCA supports KBCASetBcaStack")));
					}
				iMBca->Open(iStatus, iObserver->Port());
				iState = EOpeningBca;
				SetActive();	
				}
			else
				{
				LOG( NifmanLog::Printf(_L8("\tERROR %d in KBCASetBcaStack"), iStatus.Int()));
				iObserver->Stop(iStatus.Int());
				}
			break;
			}
		
		// In this state, BCA Open is called. Checks the result of Open.
		// If it is successful,then start the NIF. Otherwise stops the NIF.
		case EOpeningBca:
			{
			if(iStatus != KErrNone && iStatus !=  KErrAlreadyExists)
				{
				LOG( NifmanLog::Printf(_L8("\tERROR %d in BCA Open"), iStatus.Int()));
				iObserver->Stop(iStatus.Int());
				}
			else
				{
				LOG( NifmanLog::Printf(_L8("\tBCA Opened")));
				// Issue InitializeComplete() to observer to indicate BCA initialisation has completed.
				iObserver->InitializeComplete();
				iState = EBcaOpened;
				}
			break;
			}

		// In this state, BCA is Shutdown, shutdown the NIF.
		case EClosing:
			{
			// linklayer shutdown
			if (iStatus.Int() != KErrNone)
				{
				LOG( NifmanLog::Printf(_L8("\tBCA Closed (error %d ignored)"), iStatus.Int()));
				}
			else
				{
				LOG( NifmanLog::Printf(_L8("\tBCA Closed")));
				}
			// Indicate to observer that BCA shutdown has completed.
			ObserverShutdownComplete();
			iState = EIdling;
			break;
			}
		// Wrong state.
		default:
			{
			LOG( NifmanLog::Printf(_L8("ERROR CScriptBcaControl::RunL(): Unknown state %d"), iState));
			__ASSERT_DEBUG(0, Panic(KBcaUnknownState));
			break;
			}
		}

	}

void CScriptBcaControl::ObserverShutdownComplete()
/**
Indicate to the observer that the BCA has been closed down.
*/
	{
	iObserver->SetBca(NULL);
	iObserver->ShutdownComplete(iError);
	}

void CScriptBcaControl::DoCancel()
/**
Cancel active request. 
*/
	{
	LOG( NifmanLog::Printf(_L8("CScriptBcaControl %08x:\tDoCancel(), iState '%S'"), this, &StateString()));
	switch (iState)
		{
		case EIdling:
		case EBcaOpened:
			break;
		case ESettingIap:
		case ESettingBcaStack:
			if(iMBca)
				{
				iMBca->CancelIoctl();
				}
			iState = EIdling;
			break;
		case EOpeningBca:
		case EClosing:
		    if(iMBca)
			    {
			    iMBca->Close();
			    // Assume at this point that the iStatus has completed.  This can either be in the
			    // Close() itself, or in the earlier Open() (as in c32Bca).
				ObserverShutdownComplete();
			    }
			iState = EIdling;
			break;
		default:
			LOG( NifmanLog::Printf(_L8("ERROR CScriptBcaControl::DoCancel(): Unknown state %d"), iState));
			__ASSERT_DEBUG(0, Panic(KBcaUnknownState));
			break;
		}
	}
	
void CScriptBcaControl::StartLoadL()
/**
Load the BCA library and begin initialisation.

After loading the BCA, an Ioctl(KBCASetIapId) is issued to set the IAP.  This starts off the
initialisation state machine.  If initialisation completes successfully, MBcaControlObserver::InitializeComplete()
is called.  If there is an error during initialisation, MBcaControlObserver::Stop() is called.
*/
	{
	LOG( NifmanLog::Printf(_L("CScriptBcaControl %08x:\tStartLoadL(), BcaName '%S'"), this, &iObserver->BcaName()));
	
	// Load BCA DLL and get factory function
	User::LeaveIfError(iBcaDll.iObj.Load(iObserver->BcaName()));
	
	TNewBcaFactoryL newBcaFactoryProcL = (TNewBcaFactoryL)iBcaDll.iObj.Lookup(1);
	if (NULL == newBcaFactoryProcL)
		{
		LOG( NifmanLog::Printf(_L8("Library entry point found error %d"), KErrBadLibraryEntryPoint));
		User::Leave(KErrBadLibraryEntryPoint);	
		}
	
	MBcaFactory* bcaFactory = (*newBcaFactoryProcL)();
	if(!bcaFactory)
		{
		LOG( NifmanLog::Printf(_L8("BcaFactory creation error %d"), KErrCompletion));
		User::Leave(KErrGeneral);	
		}
	CleanupReleasePushL(*bcaFactory);

	// Create BCA instance
	iMBca = bcaFactory->NewBcaL();
	CleanupStack::PopAndDestroy(bcaFactory);
	
	iObserver->SetBca(iMBca);	// pass BCA pointer to observer

	// Retrieve IAP and issue KBCASetIapId towards BCA.
	TPckg<TUint32> opt(iObserver->IapId());

	iState = ESettingIap;
	iStatus = KRequestPending;
	SetActive();
	iMBca->Ioctl(iStatus, KBcaOptLevelGeneric, KBCASetIapId, opt);
	}

	
void CScriptBcaControl::Shutdown(TInt aError)
/**
Shutdown the BCA.

Once shutdown has completed, MBcaControlObserver::ShutdownComplete() is called.

@param aError the error code to reflect back to the observer after shutting down the BCA. 
*/
	{
	LOG( NifmanLog::Printf(_L8("CScriptBcaControl %08x:\tShutdown(aError %d)"), this, aError));
	if (iMBca)		// could be NULL if StartLoadL() failed
		{
		if (iState != EIdling)
			{
			Cancel();
	
			iError = aError;
			iState = EClosing;
	
			iStatus = KRequestPending;
			SetActive();
			iMBca->Shutdown(iStatus);
			}
		else
			{
			LOG( NifmanLog::Printf(_L8("\tBCA already idle - no need to shutdown"), this));
			// This could be considered a (soft) error caused by calling Shutdown() twice due, in turn,
			// to CommClose() being called twice. 
			ObserverShutdownComplete();
			}
		}
	else
		{
		LOG( NifmanLog::Printf(_L8("\tNo BCA present to shutdown"), this, aError));
		}
	}

