// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @test
 @internalComponent - Internal Symbian test code 
*/


#if (!defined TE_IPUPS_UPS_STEP_H)
#define  TE_IPUPS_UPS_STEP_H 
#include <test/testexecutestepbase.h>
#include "te_ipups_stepbase.h"
#include <ups/upsnotifierutil.h>

class CIpUpsStep : public CTeIpUpsStepBase
	{
public:
	CIpUpsStep();
	~CIpUpsStep();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();
	virtual TVerdict doTestStepPostambleL();
private:
	UserPromptService::CPolicy::TOptions ButtonToOption(const TPtrC& aButton);
	TPtrC TUpsDecisionToString(TUpsDecision aDecision);
	TUpsDecision TUpsDecisionFromString(const TPtrC& aDecision);
	TBool OptionsFlagToString(TUint aOptions, TDes& aOptionString, TBool aCheckAgainstExpectedOpt=ETrue);
	TBool VerifyAndPrintPromptDataL();
	void GetValueAt(const TInt aPos, const TPtrC& aArrayString, const TChar aDelimeter, TDes& aValue);
	
	void GetSessionAndSubSession(UserPromptService::RUpsSession& aUpsSession, UserPromptService::RUpsSubsession& aUpsSubsession);
	
	TInt iNotifyCount;
	UserPromptService::CPromptData* iPromptData;
	
	TInt 			iServiceUID;
	TPtrC 			iServerName;
	TPtrC 			iDestination;
	TPtrC			iOpaqueData;
	TBuf8<64>		iOpaqueDataStored;
	TPtrC			iDialogOption;
	TPtrC			iExpectedDialogOptions;
	TBool			iPlatSecPass;	
	TPtrC 			iExpectedUpsDecision;
	TInt			iPromptTriggerCount;
	TInt			iNoOfAuthoriseCalls;	
	TBool			iCancelPromptCall;
	
	TBool			iAlwaysOpenNewSession;
	
	TBool			iUseSameSubSession;
	TBool			iUseSameSubSessionAfterClose;
	TBool			iAlwaysOpenNewSubSession;
	TInt			iTestNotifierMode;
	};

_LIT(KIpUpsClientStep,"IpUpsStep");

_LIT(KIpUpsServiceId, "ServiceId");
_LIT(KIpUpsDestination, "Destination");
_LIT(KIpUpsOpaqueData, "OpaqueData");
_LIT(KIpUpsDialogOptionSelection, "DialogOptionSelection");
_LIT(KExpectedDialogOptions, "ExpectedDialogOptions");
_LIT(KIpUpsPlatSecPass, "PlatSecPass");
_LIT(KIpUpsExpectedDecision, "ExpectedUpsDecision");
_LIT(KIpUpsPromptTriggerCount, "PromptTriggerCount");
_LIT(KIpUpsNoOfAuthoriseCalls, "NoOfAuthoriseCalls");
_LIT(KIpUpsCancelPromptCall, "CancelPromptCall");

_LIT(KIpUpsAlwaysOpenNewSession, "AlwaysOpenNewSession");

_LIT(KIpUpsUseSameSubSession, "UseSameSubSession");
_LIT(KIpUpsUseSameSubSessionAfterClose, "UseSameSubSessionAfterClose");
_LIT(KIpUpsAlwaysOpenNewSubSession, "AlwaysOpenNewSubSession");

const TInt KMaxNoOfAuthoriseCalls = 10;
const TChar KIniFileDelimeter = ',';

#endif
