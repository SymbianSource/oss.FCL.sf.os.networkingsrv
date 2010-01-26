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

#ifndef __POLICYTESTER_H__
#define __POLICYTESTER_H__

#include <networking/testsuite.h>

#include <ipsecpolapi.h>

#include "ts_ipsec_suite.h"

enum { EIdleBit = 0, ELoadingPolicyBit, ELoadedPolicyBit, 
	EActivatingPolicyBit, EActivatedPolicyBit, 
	EUnloadingPolicyBit, EUnloadedPolicyBit,
	EMatchingPolicyBit, EMatchedPolicyBit,
	EFindingAvailableSelectorsPolicyBit, EFoundAvailableSelectorsPolicyBit,
	EErrorBit };

enum TPolicyState
	{ 
	EIdle = 1 << EIdleBit,
	ELoadingPolicy = 1 << ELoadingPolicyBit,
	ELoadedPolicy = 1 << ELoadedPolicyBit, 
	EActivatingPolicy = 1 << EActivatingPolicyBit,
	EActivatedPolicy = 1 << EActivatedPolicyBit, 
	EUnloadingPolicy = 1 << EUnloadingPolicyBit,
	EUnloadedPolicy = 1 << EUnloadedPolicyBit,
	EMatchingPolicy = 1 << EMatchingPolicyBit,
	EMatchedPolicy = 1 << EMatchedPolicyBit,
	EFindingAvailableSelectorsPolicy = 1 << EFindingAvailableSelectorsPolicyBit,
	EFoundAvailableSelectorsPolicy = 1 << EFoundAvailableSelectorsPolicyBit,
	EError = 1 << EErrorBit 
	};

typedef TInt (*TCallbackFun) (TAny*);

// Active object for manipulating a given policy
class CPolicyTester : public CActive
	{
public:
	// Creates a conenction to the policy server, and returns
	static CPolicyTester* NewLC(CTestSuiteIpsec* aTest)
		{
		CPolicyTester* self = new(ELeave) CPolicyTester(aTest);
		CleanupStack::PushL(self);
	__UHEAP_MARK;
		User::LeaveIfError(self->iServer.Connect());
	__UHEAP_MARKEND;

		aTest->Log(_L("Connected to Policy server"));
		return self;
		}
	
	~CPolicyTester()
		{
		if (IsActive())
			Cancel();

		if (iPolicy)
			delete iPolicy;

        iServer.Close();
		}

	TInt StartPolicyLoadL(HBufC8* aPolicy)
		{
		if (!IsActive())
			{
			iPolicy = aPolicy;
			CleanupStack::PushL(iPolicy);
			// Call load
			iServer.LoadPolicy(*iPolicy, iPolicyId, iStatus);
			CleanupStack::Pop();

			iPolicyState = ELoadingPolicy;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt StartActivatePolicyL()
		{
		if (!IsActive())
			{
			User::LeaveIfError(iPolicyState != ELoadedPolicy);
			iServer.ActivatePolicy(iPolicyId(), iStatus);

			iPolicyState = EActivatingPolicy;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt StartUnloadPolicyL()
		{
		if (!IsActive())
			{
			User::LeaveIfError(iPolicyState != ELoadedPolicy||EActivatedPolicy);
			iServer.UnloadPolicy(iPolicyId(), iStatus);

			iPolicyState = EUnloadingPolicy;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt StartMatchPolicyL(const TDesC8& aSelector, TDes8& aMatchingSaSpec)
		{
		if (!IsActive())
			{
			User::LeaveIfError(iPolicyState != EActivatedPolicy);
			iServer.MatchSelector(aSelector, aMatchingSaSpec, iStatus);

			iPolicyState = EMatchingPolicy;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt GetPolicyError()
		{
		if (!IsActive())
			{
			TPolicyNameInfo policy;
			iServer.GetDebugInfo(policy, KConflictingPolicyInfo);
			iLogger->Log(_L("The polciy conflicted with earlier policy - "));
			iLogger->Log(policy);
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt State() { return (TInt) iPolicyState; }

	TInt GetResult() { return iErr; }

	TInt GetId() { return iPolicyId().iHandle; }

	void OnCompletion(TCallbackFun aCompleteCallback) { iCompletionCallBack = aCompleteCallback; }

	void OnTimeout() {}
	
	TInt StartAvailableSelectorsL(const TDesC8& aGateway, CArrayFixFlat<TIpsecSelectorInfo>* aSelectors)
		{
		if (!IsActive())
			{
			User::LeaveIfError(iPolicyState != EActivatedPolicy);
			iServer.AvailableSelectors(aGateway, aSelectors, iStatus);

			iPolicyState = EFindingAvailableSelectorsPolicy;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}


private:
	CPolicyTester(CTestSuiteIpsec* aLogger): 
	   CActive(CActive::EPriorityStandard), iLogger(aLogger)
		{
		(CActiveScheduler::Current())->Add(this);
		}

	void DoCancel()
		{
		switch(iPolicyState)
			{
			case(ELoadingPolicy):
				{
				iServer.CancelLoad();
				iPolicyState = EIdle;
				break;
				}
			case(EActivatingPolicy):
				{
				iServer.CancelActivate();
				iPolicyState = ELoadedPolicy;
				break;
				}
			case(EUnloadingPolicy):
				{
				iServer.CancelUnload();
				iPolicyState = ELoadedPolicy;
				break;
				}
			default:
				break;
			};
		}
	
	void RunL()
		{
		iErr = iStatus.Int();
		// Notify the caller with the policy ID
		TCallbackArgs args;
		args.iErrCode = iErr;
		args.iPolicyHandle = iPolicyId().iHandle;
		if(iCompletionCallBack != NULL)
			{
			TCallBack cb(iCompletionCallBack, &args);
			cb.CallBack();
			}

		// Notify the active scheduler about the state completion
		//CActiveScheduler
		switch(iPolicyState)
			{
			case(ELoadingPolicy):
				{
				if (iPolicy) delete iPolicy;
				iPolicy = NULL;
				
				if(iErr == KErrNone) 
					{
					// Loading worked
					iPolicyState = ELoadedPolicy;
					}
				else if (iErr == EInboundOutboundConflict) 
					{
					// Loading failed
					GetPolicyError();
					iPolicyState = EError;
					}
				break;
				}
			case(EActivatingPolicy):
				{
				if(iErr == KErrNone) 
					{
					// Activating worked
					iPolicyState = EActivatedPolicy;
					}
				else
					{
					// Activation failed
					iLogger->Log(_L("Policy ID %d activation failed, with error %d"), iPolicyId().iHandle, iErr);
					iPolicyState = EError;
					}
				break;
				}
			case(EUnloadingPolicy):
				{
				if(iErr == KErrNone) 
					{
					// Unloading worked
					iPolicyState = EUnloadedPolicy;
					}
				else
					{
					// Unloading failed
					iLogger->Log(_L("Policy ID %d unloading failed, with error %d"), iPolicyId().iHandle, iErr);
					iPolicyState = EError;
					}
				break;
				}
			case(EMatchingPolicy):
				{
				if(iErr == KErrNone) 
					{
					// Maching selector worked
					iPolicyState = EMatchedPolicy;
					}
				else
					{
					// Matching selector failed
					iLogger->Log(_L("Policy %d Matching selector failed, with error %d ID"), iPolicyId().iHandle, iErr);
					iPolicyState = EError;
					}
				break;
				}
			case(EFindingAvailableSelectorsPolicy):
				{
				if(iErr == KErrNone) 
					{
					// Finding Available Selectors worked
					iPolicyState = EFoundAvailableSelectorsPolicy;
					}
				else
					{
					// Finding Available Selectors failed
					iLogger->Log(_L("Policy %d Finding Available Selectors failed, with error %d ID"), iPolicyId().iHandle, iErr);
					iPolicyState = EError;
					}
				break;
				}
			default:
				break;
			}
		User::Leave(iPolicyState);		//TODO- come up with a better mechanism to return to the AS.
		}

private:
	RIpsecPolicyServ iServer;
	HBufC8* iPolicy;
	TPolicyHandlePckg iPolicyId;
	
	TPolicyState  iPolicyState;
	TInt iErr;
	CTestSuite* iLogger;
	TCallbackFun iCompletionCallBack;
	};

#endif
