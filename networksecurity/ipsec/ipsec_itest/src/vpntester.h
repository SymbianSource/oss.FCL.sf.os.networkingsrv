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

#ifndef __VPNTESTER_H__
#define __VPNTESTER_H__

#include <networking/testsuite.h>

#include <vpnapi.h>

#include "ts_ipsec_suite.h"

enum {EIdleBit = 0,
	EImportingBit, EImportedBit, 
	EPasswdChangingBit, EPasswdChangedBit,
	EErrorBit };

enum TPolicyState 
	{
	EIdle = 1 << EIdleBit,
	EImporting = 1 << EImportingBit, EImported = 1 << EImportedBit,
	EPasswdChanging = 1 << EPasswdChangingBit, EPasswdChanged = 1 << EPasswdChangedBit,
	EError = 1 << EErrorBit 
	};

// AO for use in vpn management
class CVpnTester : public CActive
	{
public:
	// Creates a conenction to the policy server, and returns
	static CVpnTester* NewLC(CTestSuiteIpsec* aTest)
		{
		CVpnTester* self = new(ELeave) CVpnTester(aTest);
		CleanupStack::PushL(self);
	__UHEAP_MARK;
		User::LeaveIfError(self->iServer.Connect());
	__UHEAP_MARKEND;

		aTest->Log(_L("Connected to Policy server"));
		return self;
		}

	~CVpnTester()
		{
		if (iPolicyList)
			delete iPolicyList;

		if (IsActive())
			Cancel();

		iServer.Close();
		}

	TInt StartPolicyImportL(const TDesC& aPolicyDir)
		{
		if (!IsActive())
			{
			iServer.ImportPolicy(aPolicyDir, iStatus);

			iPolicyState = EImporting;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt StartChangePsswdL()
		{
		if (!IsActive())
			{
			// Call Change
			iServer.ChangePassword(iPolIdPckg, iStatus);
			
			iPolicyState = EPasswdChanging;
			SetActive();
			return KErrNone;
			}
		else return KErrInUse;
		}

	TInt ListPoliciesL()
		{
		if (!IsActive())
			{
			iLogger->Log(_L("Getting the number of currently loaded policies. "));
			User::LeaveIfError(iServer.EnumeratePolicies(iNumPolicies));
			iLogger->Log(_L("The number of currently loaded policies is %d "), iNumPolicies);

			iLogger->Log(_L("Getting policy descriptions"));

			iPolicyList = new(ELeave) CArrayFixFlat<TVpnPolicyInfo>(2);
			TVpnPolicyDetails details;
			
			CleanupStack::PushL(iPolicyList);
			iPolicyList->SetReserveL(iNumPolicies);
			User::LeaveIfError(iServer.GetPolicyInfoList(iPolicyList));
			for (TInt count = 0; count < iNumPolicies; count++)
				{
				const TVpnPolicyInfo& pol = iPolicyList->At(count);
				iLogger->Log(_L("Policy %d description - "), count + 1);
				iLogger->Log(_L("Policy ID is %S"), &pol.iId);
				iLogger->Log(_L("Policy Name is %S"), &pol.iName);
				iLogger->Log(_L("Getting policy id %S details"), &pol.iId);
				User::LeaveIfError(iServer.GetPolicyDetails(pol.iId, details));
				iLogger->Log(_L("Policy description %S"), &details.iDescription);
				iLogger->Log(_L("Policy Contact is %S"), &details.iContactInfo);
				iLogger->Log(_L("Policy usagestatus %d"), details.iUsageStatus);
				iLogger->Log(_L("Policy PkiStatus %d"), details.iPkiStatus);
				iLogger->Log(_L(" "));
				iLogger->Log(_L(" "));
				}
			CleanupStack::Pop(iPolicyList);
			return KErrNone;
			}
		else return KErrInUse;
		}

	void DeletePolicyL()
		{
		ASSERT(EImported == iPolicyState);
		iErr = 0;
		for(TInt count = 0; count < iNumPolicies; count++)
			{
			const TVpnPolicyInfo& pol = iPolicyList->At(count);
			iErr = iServer.DeletePolicy(pol.iId);
			iLogger->Log(_L("The policy ID %S was unloaded with result %d"), &pol.iId, iErr);
			}
		if (iPolicyList) delete iPolicyList;
		iPolicyList = NULL;
		iPolicyState = EIdle;
		User::LeaveIfError(iErr);
		}

	TInt State()
		{ return (TInt) iPolicyState; }

	TInt GetResult()
		{ return iErr; }

	const TVpnPolicyId& GetId() const
		{ 
		//if (iPolicyId.Length() != 0)
		return iPolicyId;
		}

private:
	CVpnTester(CTestSuiteIpsec* aLogger): 
	   CActive(CActive::EPriorityStandard), 
		   iPolIdPckg(iPolicyId), iLogger(aLogger)
		{
		(CActiveScheduler::Current())->Add(this);
		}

	void DoCancel()
		{
		switch(iPolicyState)
			{
			case(EImporting):
				{
				iServer.CancelImport();
				iPolicyState = EIdle;
				break;
				}
			case(EPasswdChanging):
				{
				iServer.CancelChange();
				iPolicyState = EImported;
				break;
				}
			default:
				break;
			};
		}

	void RunL()
		{
		iErr = iStatus.Int();
		switch(iPolicyState)
			{
			case(EImporting):
				{
				if(iErr == KErrNone) 
					{
					// Loading worked
					iPolicyState = EImported;
					}
				else
					{
					// Loading failed
					iLogger->Log(_L("Policy Loading failed "));
					iPolicyState = EError;
					}
				break;
				}
			case(EPasswdChanging):
				{
				if(iErr == KErrNone) 
					{
					// Activating worked
					iPolicyState = EPasswdChanged;
					}
				else
					{
					// Activation failed
					iLogger->Log(_L("Policy ID %S passwd change failed, with error %d"), &iPolicyId, iErr);
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
	RVpnServ iServer;

	CArrayFixFlat<TVpnPolicyInfo>* iPolicyList;
	TInt iNumPolicies;
	TVpnPolicyId iPolicyId;
	TPckg<TVpnPolicyId> iPolIdPckg;
	
	TPolicyState  iPolicyState;
	TInt iErr;
	CTestSuite* iLogger;
	};

#endif
