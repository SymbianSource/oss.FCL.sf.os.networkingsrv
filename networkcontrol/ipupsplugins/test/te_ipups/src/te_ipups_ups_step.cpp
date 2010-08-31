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

#include "te_ipups_ups_step.h"  
#include "upstestnotifierproperties.h"
#include <s32mem.h> 

static TInt ThreadFunction(TAny *)
	{
	return KErrNone;
	}
	
CIpUpsStep::~CIpUpsStep()
/**
 * Destructor
 */
	{  	
	} // End of function


CIpUpsStep::CIpUpsStep()
/**
 * Constructor
 */
	{
	SetTestStepName(KIpUpsClientStep);
	} // End of function


TVerdict CIpUpsStep::doTestStepPreambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	TSecurityPolicy nullPolicy(ECapability_None);
    TInt err;
    
    //Properties modified to be returned to test harness from test notifier
    err = RProperty::Define(KUidPSUPSTestNotifCategory, KUnNotifyCount, KUnCountKeyType, nullPolicy, nullPolicy);
    if (err != KErrAlreadyExists && err != KErrNone)
    	{
    	User::LeaveIfError(err);
    	}
    
    err = RProperty::Define(KUidPSUPSTestNotifCategory, KUnNotifyValues, KUnNotifyValuesKeyType, nullPolicy, nullPolicy);
    if (err != KErrAlreadyExists && err != KErrNone)
    	{
    	User::LeaveIfError(err);
    	}
    
    //Properties test notifier requires from test harness about the button press and delay
    err = RProperty::Define(KUidPSUPSTestNotifCategory, KUtButtonPress, KUtButtonPressKeyType, nullPolicy, nullPolicy);
    if (err != KErrAlreadyExists && err != KErrNone)
    	{
    	User::LeaveIfError(err);
    	}
    
    err = RProperty::Define(KUidPSUPSTestNotifCategory, KUtButtonPressDelay, KUtButtonPressDelayKeyType, nullPolicy, nullPolicy);
    if (err != KErrAlreadyExists && err != KErrNone)
    	{
    	User::LeaveIfError(err);
    	}
    
    //Get the testNotifier working mode, filemode or P&S mode
    User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUtFileOverride, iTestNotifierMode));
    
    //Set the property to override working of testNotifier in P&S mode instead of filemode
    User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUtFileOverride, KFileOverride));
    
    //Initialise optional data iNoOfAuthoriseCalls (should be '1' minimum).
    iNoOfAuthoriseCalls = 1;
    
    //Initialise optional data iPromptTriggerCount (Initialising with '-1' means no check if user not specfied in INI file).
    iPromptTriggerCount = -1;
    
    //Read data from INI file
	GetHexFromConfig(ConfigSection(), KIpUpsServiceId, iServiceUID);
	GetStringFromConfig(ConfigSection(),KIpUpsDestination, iDestination);
	GetStringFromConfig(ConfigSection(),KIpUpsOpaqueData, iOpaqueData);
	GetStringFromConfig(ConfigSection(),KIpUpsDialogOptionSelection, iDialogOption);
	GetBoolFromConfig(ConfigSection(),KIpUpsPlatSecPass, iPlatSecPass);	
	GetStringFromConfig(ConfigSection(),KIpUpsExpectedDecision, iExpectedUpsDecision);	
	GetIntFromConfig(ConfigSection(),KIpUpsPromptTriggerCount, iPromptTriggerCount);
    GetIntFromConfig(ConfigSection(),KIpUpsNoOfAuthoriseCalls, iNoOfAuthoriseCalls);    
    GetBoolFromConfig(ConfigSection(),KIpUpsCancelPromptCall, iCancelPromptCall);
    
    GetBoolFromConfig(ConfigSection(),KIpUpsAlwaysOpenNewSession, iAlwaysOpenNewSession);
        
    GetBoolFromConfig(ConfigSection(),KIpUpsUseSameSubSession, iUseSameSubSession);
    GetBoolFromConfig(ConfigSection(),KIpUpsUseSameSubSessionAfterClose, iUseSameSubSessionAfterClose);
    GetBoolFromConfig(ConfigSection(),KIpUpsAlwaysOpenNewSubSession, iAlwaysOpenNewSubSession);
    
    //Keep the initial count of the notifier recorded, should be '0'
    User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnNotifyCount, iNotifyCount));
    
	SetTestStepResult(EPass);	
	return TestStepResult();
	} // End of function.


TVerdict CIpUpsStep::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{	
	UserPromptService::RUpsSession 		upsSession;
    UserPromptService::RUpsSubsession 	upsSubSession[KMaxNoOfAuthoriseCalls];
        
    RThread dummyThread[KMaxNoOfAuthoriseCalls];
    TRequestStatus threadStatus; 	
 	
 	TInt notifyCountBeforeTest = iNotifyCount;
 	TInt notifyCountAfterTest = 0;
	
	TServiceId serviceId = TUid::Uid(iServiceUID);
	
	TInt subSessionCntr = 0;
	
	//Repeat call to Authorise as specified in INI file (through NoOfAuthoriseCalls data)
    for (TInt cntr=0; cntr<iNoOfAuthoriseCalls && cntr<KMaxNoOfAuthoriseCalls; cntr++)
    	{
    	if (upsSession.Handle() == KNullHandle)
    		{
    		User::LeaveIfError(upsSession.Connect());
    		INFO_PRINTF1(_L("UPS Session Started."));
    		}
    	
    	if (upsSubSession[subSessionCntr].SubSessionHandle() == KNullHandle)
    		{
    		//Create dummy thread with different names
    		TBuf<32> dummyThreadName;
    		dummyThreadName.Copy(_L("DummyThread_0123456789"));
    		
    		User::LeaveIfError(dummyThread[subSessionCntr].Create(dummyThreadName.Right(dummyThreadName.Length()-subSessionCntr), ThreadFunction, 4096, 4096, 4096, 0, EOwnerThread));
    		
			dummyThread[subSessionCntr].Rendezvous(threadStatus);
			dummyThread[subSessionCntr].Resume();
 			User::WaitForRequest(threadStatus);
 			
    		upsSubSession[subSessionCntr] = UserPromptService::RUpsSubsession();
    		User::LeaveIfError(upsSubSession[subSessionCntr].Initialise(upsSession, dummyThread[subSessionCntr]));
    		INFO_PRINTF1(_L("UPS SubSession Initialised."));
    		}
		
		//Get the dialog option for this iteration (specified in the INI file through 
	    //DialogOptionSelection data, separated using KIniFileDelimeter).
		TBuf<32> dialogOption;
		GetValueAt(cntr, iDialogOption, KIniFileDelimeter, dialogOption);
		
	    UserPromptService::TPromptResult promptResult;
	    promptResult.iSelected = ButtonToOption(dialogOption);
	    
		TPckg<UserPromptService::TPromptResult> resultPckg(promptResult);
		
		//Set the Dialog Option selection by user (specifies through DialogOptionSelection data value in INI file).
	    User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUtButtonPress, resultPckg));
		User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUtButtonPressDelay, iCancelPromptCall?1:0));
		
		INFO_PRINTF2(_L("Dialog Option Selection ( %S )."), &dialogOption);
		
		//Get the Notify Count BEFORE call to Authorise
		User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnNotifyCount, notifyCountBeforeTest));
		
	    TUpsDecision upsDecision;
	    TRequestStatus status;
	    
	    if ( iOpaqueData.Length() > 0 )
	    	{	    	
	    	HBufC8* converter = HBufC8::NewLC(iOpaqueData.Length());
	    	converter->Des().Copy(iOpaqueData);	             
	       	iOpaqueDataStored = converter->Ptr();
	       	CleanupStack::PopAndDestroy(); //converter
	       	
	       	INFO_PRINTF1(_L("Opaque data present."));
	    	upsSubSession[subSessionCntr].Authorise(iPlatSecPass, serviceId, iDestination, iOpaqueDataStored, upsDecision, status);
	    	}
	    else
	    	{
	    	INFO_PRINTF1(_L("Opaque data NOT present."));
	    	upsSubSession[subSessionCntr].Authorise(iPlatSecPass, serviceId, iDestination, upsDecision, status);
	    	}
	    

	    if (iCancelPromptCall)
	    	{
	    	upsSubSession[subSessionCntr].CancelPrompt();
	    	INFO_PRINTF1(_L("CancelPrompt called on UPS SubSession."));
	    	}
	    
	    User::WaitForRequest(status);
	    
	    SetTestStepError(status.Int());
	    User::LeaveIfError(status.Int());
	    
	    //Get the Notify Count AFTER call to Authorise
	    User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnNotifyCount, notifyCountAfterTest));
	    
	    INFO_PRINTF3(_L("NotifyCount Before Authorise ( %d ) <> NotifyCount After Authorise ( %d )"), notifyCountBeforeTest, notifyCountAfterTest);
	    
	    if ( iExpectedUpsDecision.Length() > 0 )
	    	{
	    	//Get the expected UPS decision for this iteration (specified in the INI file through 
	    	//ExpectedUpsDecision data, separated using KIniFileDelimeter).
	    	TBuf<32> expectedUpsDecision;
	    	GetValueAt(cntr, iExpectedUpsDecision, KIniFileDelimeter, expectedUpsDecision);
	    	
	    	TPtrC upsDecisionString = TUpsDecisionToString(upsDecision);
	    	
	    	INFO_PRINTF3(_L("UPS Decision Expected ( %S ) <> UPS Decision Returned ( %S )"), &expectedUpsDecision, &upsDecisionString);
	        
	    	//Compare the expected and returned UPS decision
	    	TEST( expectedUpsDecision.Compare(TUpsDecisionToString(upsDecision)) == 0);
	    	}
	    
		VerifyAndPrintPromptDataL();
		
		//Check what user requested to do with SubSession for further iterations (if any).
		if ( iUseSameSubSession )
			{
			INFO_PRINTF2(_L("Using UPS SubSession ( %d )."), subSessionCntr);
			}
		if ( iUseSameSubSessionAfterClose || iAlwaysOpenNewSession )
			{
			upsSubSession[subSessionCntr].Close();
			INFO_PRINTF2(_L("UPS SubSession Closed ( %d )."), subSessionCntr);
			dummyThread[subSessionCntr].Close();
			}
		else if ( iAlwaysOpenNewSubSession )
			{
			++subSessionCntr;
			}
			
		if ( iAlwaysOpenNewSession  )
			{
			upsSession.Close();
			INFO_PRINTF1(_L("UPS Session Closed."));
			}
	    } //End - for loop
     
    
    //As with multiple clients , its better not to try to synchronize the calls which change the notify count.
    //When all the concurrent calls are done , check the count finally through test step notifycount.
    if (iPromptTriggerCount >= 0)
    	{
    	TEST( notifyCountAfterTest == (iNotifyCount+iPromptTriggerCount) );
    	}    
    
    //Ensure all sub sessions are closed
    for (TInt cntr=0; cntr<subSessionCntr; cntr++)
    	{
    	if ( upsSubSession[cntr].SubSessionHandle() != KNullHandle )
    		{
    		INFO_PRINTF2(_L("UPS SubSession Closed ( %d )."), cntr);
    		upsSubSession[cntr].Close();
    		dummyThread[cntr].Close();
    		}
    	}
    
    //Ensure session is closed
    if ( upsSession.Handle() != KNullHandle )
    	{
    	upsSession.Close();
		INFO_PRINTF1(_L("UPS Session Closed."));
    	}    
	
	return TestStepResult();
	}  // End of function


TVerdict CIpUpsStep::doTestStepPostambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	//ReSet the property to specify working of testNotifier in P&S or filemode
    User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUtFileOverride, iTestNotifierMode));
    
	return TestStepResult();
	} // End of function.

TBool CIpUpsStep::VerifyAndPrintPromptDataL()
/**
 * @return - TBool - ETrue of prompt data returned matched the data expected as specified in INI file, else EFalse.
 * 
 */
	{
	//Get the Prompt Information
	const TInt KMaxPromptDataLenght = 512;
    HBufC8* buf=HBufC8::NewLC(KMaxPromptDataLenght);
    TPtr8 bufPtr(buf->Des());

	User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnNotifyValues, bufPtr));
	
	RDesReadStream promptDataStream(bufPtr);
		
	iPromptData = UserPromptService::CPromptData::NewL();
	CleanupStack::PushL(iPromptData);
	iPromptData->InternalizeL(promptDataStream);
	
	INFO_PRINTF2(_L("Client Name ( %S )"), &iPromptData->iClientName);
	INFO_PRINTF2(_L("Vendor Name ( %S )"), &iPromptData->iVendorName);
	INFO_PRINTF2(_L("Destination ( %S )"), &iPromptData->iDestination);	
	INFO_PRINTF2(_L("ClientSid   ( %08x )"), iPromptData->iClientSid.iId);
	INFO_PRINTF2(_L("ServerSid   ( %08x )"), iPromptData->iServerSid.iId);
	INFO_PRINTF2(_L("ServiceId   ( %08x )"), iPromptData->iServiceId);
	
	TInt count = iPromptData->iDescriptions.Count();
   	for (TInt cntr = 0; cntr < count; ++cntr)
      {
      INFO_PRINTF3(_L("Descriptions(%d) : %S "), cntr, iPromptData->iDescriptions[cntr]);
      }	
	
	//Print Dialog Flags
	TBuf<64> dialogFlags;
	
	if ( iPromptData->iFlags & UserPromptService::ETrustedClient )
		{
		dialogFlags.Append(_L(" TrustedClient "));
		}
	if ( iPromptData->iFlags & UserPromptService::EBuiltInApp )
		{
		dialogFlags.Append(_L(" BuiltInApp "));
		}
	INFO_PRINTF2(_L("DialogFlags Set to  ( %S )"), &dialogFlags);
	
	TPtrC expectedDialogOptions;
	TBuf<64> bufOptions;
	
	GetStringFromConfig(ConfigSection(),KExpectedDialogOptions, iExpectedDialogOptions);
	
	if (iExpectedDialogOptions.Length() > 0)
		{
		INFO_PRINTF2(_L("Options Expected ( %S )"), &iExpectedDialogOptions);
		
		//Check if options presented are as expected		
		TBool result = OptionsFlagToString(iPromptData->iOptions, bufOptions);		
		TEST(result != EFalse);
		}
	else
		{
		OptionsFlagToString(iPromptData->iOptions, bufOptions, EFalse);
		}
	
	INFO_PRINTF2(_L("Options Presented ( %S )"), &bufOptions);
	
	TEST(iPromptData->iServiceId.iUid == iServiceUID);
	TEST(iPromptData->iDestination == iDestination);
	
	CleanupStack::PopAndDestroy(2); //buf, iPromptData
	
	return EFalse;
	} // End of function.

TBool CIpUpsStep::OptionsFlagToString(TUint aOptions, TDes& aOptionString, TBool aCheckAgainstExpectedOpt)
/** OptionsFlagToString converts the dialog option(s) presented to string and check if the presented option
 * 	is amongst the one which is expected(from INI file) or not if aCheckAgainstExpectedOpt is set to ETrue.
 *
 * @return - TBool - ETrue if presented options are the ones which are expected as specified in the INI file.
 * 
 */
	{
	_LIT(KOptionYes, "Yes");
	_LIT(KOptionNo, "No");	
	_LIT(KOptionSessionYes, "SessionYes");
	_LIT(KOptionAlways, "Always");
	_LIT(KOptionNever, "Never");
	_LIT(KOptionSessionNo, "SessionNo");
	
	const TPtrC policyOptions[] = {KOptionYes(), KOptionNo(), KOptionSessionYes(), KOptionAlways(),
									KOptionNever(), KOptionSessionNo()};
		
	TInt optionsCntr = 0;
	TBool isOptionExpected = ETrue;
		
	aOptionString.Append(_L("-"));
	
	if (aOptions & UserPromptService::CPolicy::EYes)
		{
		aOptionString.Copy(policyOptions[optionsCntr]);
		aOptionString.Append(_L("-"));
		
		if (aCheckAgainstExpectedOpt)
			{
			if (iExpectedDialogOptions.FindF(policyOptions[optionsCntr]) == KErrNotFound)
				{
				isOptionExpected = EFalse;
				}
			}
		}
	++optionsCntr;
	
	if (aOptions & UserPromptService::CPolicy::ENo)
		{		
		aOptionString.Append(policyOptions[optionsCntr]);
		aOptionString.Append(_L("-"));
		
		if (aCheckAgainstExpectedOpt)
			{
			if (iExpectedDialogOptions.FindF(policyOptions[optionsCntr]) == KErrNotFound)
				{
				isOptionExpected = EFalse;
				}
			}
		}
	++optionsCntr;
	
	if (aOptions & UserPromptService::CPolicy::ESessionYes)
		{		
		aOptionString.Append(policyOptions[optionsCntr]);
		aOptionString.Append(_L("-"));
		
		if (aCheckAgainstExpectedOpt)
			{
			if (iExpectedDialogOptions.FindF(policyOptions[optionsCntr]) == KErrNotFound)
				{
				isOptionExpected = EFalse;
				}
			}
		}
	++optionsCntr;
	
	if (aOptions & UserPromptService::CPolicy::EAlways)
		{		
		aOptionString.Append(policyOptions[optionsCntr]);
		aOptionString.Append(_L("-"));
		
		if (aCheckAgainstExpectedOpt)
			{
			if (iExpectedDialogOptions.FindF(policyOptions[optionsCntr]) == KErrNotFound)
				{
				isOptionExpected = EFalse;
				}
			}
		}
	++optionsCntr;
	
	if (aOptions & UserPromptService::CPolicy::ENever)
		{		
		aOptionString.Append(policyOptions[optionsCntr]);
		aOptionString.Append(_L("-"));
		
		if (aCheckAgainstExpectedOpt)
			{
			if (iExpectedDialogOptions.FindF(policyOptions[optionsCntr]) == KErrNotFound)
				{
				isOptionExpected = EFalse;
				}
			}
		}
	++optionsCntr;
	
	if (aOptions & UserPromptService::CPolicy::ESessionNo)
		{		
		aOptionString.Append(policyOptions[optionsCntr]);
		aOptionString.Append(_L("-"));
		
		if (aCheckAgainstExpectedOpt)
			{
			if (iExpectedDialogOptions.FindF(policyOptions[optionsCntr]) == KErrNotFound)
				{
				isOptionExpected = EFalse;
				}
			}
		}
		
	return isOptionExpected;
	} // End of function.

/** GetValueAt provides the value of data at a specific index (specified by aPos)
*	String containing the values is specified thorugh aArrayString and multiple values 
*	are separated by delimeter aDelimeter.
*	aValue is returned. 
*/
void CIpUpsStep::GetValueAt(const TInt aPos, const TPtrC& aArrayString, const TChar aDelimeter, TDes& aValue)
	{
	TInt posCntr=0;	
	TInt itemCntr = -1;	
	
	//Initialise it with blank string to avoid any previous copies
	aValue.Copy(_L(""));
	
	while (posCntr < aArrayString.Length() && itemCntr != aPos)
		{
		if (aArrayString[posCntr] != aDelimeter)
			{
			aValue.Append(aArrayString[posCntr]);
			}
		else
			{
			++itemCntr;
			//Is this the item we are looking for, if not, make space to next one
			if (itemCntr != aPos)
				{
				aValue.Copy(_L(""));
				}
			}
		++posCntr;
		}	
	}

/* TUpsDecisionToString converts TUpsDecision to string.
*/
TPtrC CIpUpsStep::TUpsDecisionToString(TUpsDecision aDecision)
	{
	if(aDecision == EUpsDecYes)
		{
		return _L("Yes");
		}
	else if(aDecision == EUpsDecNo)
		{
		return _L("No");
		}
	else if(aDecision == EUpsDecSessionYes)
		{
		return _L("SessionYes");
		}
	else if(aDecision == EUpsDecSessionNo)
		{
		return _L("SessionNo");
		}
	else 
		{
		ERR_PRINTF1(_L("Invalid UPS Descision, returning No by default."));
		return _L("No");
		}
	} // End of function.

/* TUpsDecisionFromString converts string value to TUpsDecision.
*/
TUpsDecision CIpUpsStep::TUpsDecisionFromString(const TPtrC& aDecision)
	{
	if(aDecision.CompareF(_L("Yes"))==0)
		{
		return EUpsDecYes;
		}
	else if(aDecision.CompareF(_L("No"))==0)
		{
		return EUpsDecNo;
		}
	else if(aDecision.CompareF(_L("SessionYes"))==0)
		{
		return EUpsDecSessionYes;
		}
	else if(aDecision.CompareF(_L("SessionNo"))==0)
		{
		return EUpsDecSessionNo;
		}
	else 
		{
		ERR_PRINTF1(_L("Invalid UPS Descision, returning No by default."));
		return EUpsDecNo;
		}
	} // End of function.

/* ButtonToOption converts string value of button presented to CPolicy::TOptions.
*/
UserPromptService::CPolicy::TOptions CIpUpsStep::ButtonToOption(const TPtrC& aButton)
	{
	if(aButton.CompareF(_L("Yes"))==0)
		{
		return UserPromptService::CPolicy::EYes;
		}
	else if(aButton.CompareF(_L("No"))==0)
		{
		return UserPromptService::CPolicy::ENo;
		}
	else if(aButton.CompareF(_L("Session"))==0)
		{
		return UserPromptService::CPolicy::ESession;
		}
	else if(aButton.CompareF(_L("SessionYes"))==0)
		{
		return UserPromptService::CPolicy::ESessionYes;
		}
	else if(aButton.CompareF(_L("Always"))==0)
		{
		return UserPromptService::CPolicy::EAlways;
		}	
	else if(aButton.CompareF(_L("Never"))==0)
		{
		return UserPromptService::CPolicy::ENever;
		}
	else if(aButton.CompareF(_L("SessionNo"))==0)
		{
		return UserPromptService::CPolicy::ESessionNo;
		}	
	else 
		{
		ERR_PRINTF1(_L("Invalid Button Option, returning Policy Option No by default."));
		return UserPromptService::CPolicy::ENo;
		}				
	}  // End of function.
