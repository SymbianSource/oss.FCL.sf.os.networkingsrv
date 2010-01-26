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

#include <e32std.h>
#include <e32base.h>

#include "upstestnotifier.h"
#include <e32property.h>
#include "upstestnotifierproperties.h"
#include <ups/policy.h>

_LIT(KScriptFile, "c:\\upstestnotifier.txt");

using namespace UserPromptService;

EXPORT_C CArrayPtr<MNotifierBase2>* NotifierArray()
/**
 * Lib main entry point
 */
	{
	CArrayPtrFlat<MNotifierBase2>* notifiers=new (ELeave)CArrayPtrFlat<MNotifierBase2>(1);
	CleanupStack::PushL(notifiers);
	notifiers->AppendL(CUpsTestNotifier::NewL());
	CleanupStack::Pop(notifiers);
	return(notifiers);
	}

CUpsTestNotifier* CUpsTestNotifier::NewL()
/**
 * Factory method for creating a techview user prompt service notifier.
 * @return The new notifier object.
 */
	{
	CUpsTestNotifier* self = new (ELeave) CUpsTestNotifier();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	
	//Define all the necessary P&S properties
	RProperty::Define(KUidPSUPSTestNotifCategory, KUnNotifyCount, KUnCountKeyType);
	RProperty::Define(KUidPSUPSTestNotifCategory, KUnNotifyValues, KUnNotifyValuesKeyType);
	RProperty::Define(KUidPSUPSTestNotifCategory, KUnStoredNotifyCount, KUnStoredCountKeyType);
	RProperty::Define(KUidPSUPSTestNotifCategory, KUtButtonPress, KUtButtonPressKeyType);
	RProperty::Define(KUidPSUPSTestNotifCategory, KUtButtonPressDelay, KUtButtonPressDelayKeyType);
	RProperty::Define(KUidPSUPSTestNotifCategory, KUtFileOverride, KUtFileOverrideKeyType);
	
	//Initialise the KUtFileOverride property
	User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUtFileOverride, KNoFileOverride));
	
	return self;
	}


CUpsTestNotifier::CUpsTestNotifier()
	:isDrivenByFile(EFalse), iNotifyCount(0), iResultDelay(0)
/**
 * Constructor
 */
	{
	iInfo.iUid = TUid::Uid(KUpsTestNotifierImplementation);	
	iInfo.iChannel = TUid::Null();
	iInfo.iPriority = ENotifierPriorityLow;
	}


void CUpsTestNotifier::ConstructL()
	{
	CheckDrivenByFileL();
	if (!isDrivenByFile)
		{
		iFileObserver = new (ELeave) CFileObserver(*this);
		iFileObserver->StartL();
		}
	}

void CUpsTestNotifier::CheckDrivenByFileL()
	{
	isDrivenByFile = CheckScriptedResponseL(iResult, iResultDelay);
	}

CUpsTestNotifier::~CUpsTestNotifier()
	{
	}


void CUpsTestNotifier::Release()
	{
	delete this;
	}


CUpsTestNotifier::TNotifierInfo CUpsTestNotifier::RegisterL()
/**
 * Called by notifier framework to get implmentation details
 */
	{
	return iInfo;
	}

CUpsTestNotifier::TNotifierInfo CUpsTestNotifier::Info() const
/**
 * Return the previously generated info about the notifier
 */
	{
	return iInfo;
	}

void CUpsTestNotifier::StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage)
/**
 * Start the notifier with data aBuffer.  aMessage should be completed when the notifier is deactivated.
 * May be called multiple times if more than one client starts the notifier.  The notifier is immediately
 * responsible for completing aMessage.
 */
	{
	iNotifyCount++;
	TRAPD(err, DoStartL(aBuffer, aReplySlot, aMessage));
	if(err != KErrNone)
		{
		aMessage.Complete(err);
		}
	}

TPtrC8 CUpsTestNotifier::StartL(const TDesC8& /*aBuffer*/)
/**
 * Start the notifier with data aBuffer.
 * May be called multiple times if more than one client starts the notifier.
 */
	{
	TPtrC8 ret(KNullDesC8);
	return (ret);
	}

void CUpsTestNotifier::Cancel()
	{
	}


TPtrC8 CUpsTestNotifier::UpdateL(const TDesC8& /*aBuffer*/)
	{
	TPtrC8 ret(KNullDesC8);
	return (ret);
	}

void CUpsTestNotifier::DoStartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage)
/**
 * Does the real work of handling the notifier event
 * @param aBuffer	   The buffer containing the prompt data.
 * @param aReplySlot  The slot in the messasge to write the result to.
 * @param aMessage	   The message from the User Prompt Service.
 */
	{
	TPromptResult result;
	TPckg<TPromptResult> resultPckg(result);
	TInt resultDelay;
	TInt configFileOverride(0);
	
	if (iFileObserver && iFileObserver->IsActive())
		{
		iFileObserver->Cancel();
		}

	//Check to see if the config file setting should be overridden by P&S
	User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUtFileOverride, configFileOverride));
	
	if (isDrivenByFile && (configFileOverride == KNoFileOverride))
		{
		result = iResult;
		resultDelay = iResultDelay;
		}
	else
		{
		//Publish notification details to test harness
		User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUnNotifyCount, iNotifyCount));
		User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUnNotifyValues, aBuffer));
		
		//Get Button press details from test harness
		User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUtButtonPress, resultPckg));
		User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUtButtonPressDelay, resultDelay));
		}
	
	if(resultDelay > 0)
		{
		User::After(resultDelay*KOneSecond);
		}

	aMessage.WriteL(aReplySlot, resultPckg);
	aMessage.Complete(KErrNone);
	
	}
	
TBool CUpsTestNotifier::CheckScriptedResponseL(TPromptResult& aResult, TInt& aResultDelay)
/**
 * Determine the mode of automation
 *  
 * If a scripted response has been supplied in KScriptFile then this will be used to determine
 * the response to send back to the dialogue creator. 
 * Otherwise the response shall be determined by retrieving values set by the test harness
 * via Publish and Subscribe
 *
 * @param	aResult		The result object to populate if a response has been rescripted.
 * @return	ETrue, if a response has been scripted; otherwise, EFalse is returned.
 */	
	{
	TBool useScriptedResponse(EFalse);
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	//TBuf<21> name(_L("!:\\upstestnotifier.txt"));
	//name[0] = fs.GetSystemDriveChar();
	RFile file;	
	TInt err = file.Open(fs, KScriptFile, EFileShareReadersOnly | EFileRead);
	if (err == KErrNone)
		{
		CleanupClosePushL(file);
		TInt size;
		User::LeaveIfError(file.Size(size));
		if (size < 1024)
			{
			RBuf8 buf8;
			buf8.CreateL(size);			
			CleanupClosePushL(buf8);		
			User::LeaveIfError(file.Read(buf8));
			
			RBuf buf;
			buf.CreateL(size);
			CleanupClosePushL(buf);
			buf.Copy(buf8);
			
			aResult.iDestination = KErrNotFound;
			if (buf.FindF(_L("SessionYes")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::ESessionYes;
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = SessionYes");
				useScriptedResponse = ETrue;
				}
			else if (buf.FindF(_L("SessionNo")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::ESessionNo;
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = SessionNo");
				useScriptedResponse = ETrue;
				}
			else if (buf.FindF(_L("Yes")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::EYes;
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = Yes");
				useScriptedResponse = ETrue;
				}
			else if (buf.FindF(_L("No")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::ENo;
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = No");
				useScriptedResponse = ETrue;
				}
			else if (buf.FindF(_L("Session")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::ESessionYes;
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = SessionYes");
				useScriptedResponse = ETrue;
				}
			else if (buf.FindF(_L("Always")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::EAlways;				
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = Always");
				useScriptedResponse = ETrue;
				}
			else if (buf.FindF(_L("Never")) != KErrNotFound)
				{
				aResult.iSelected = CPolicy::ENever;
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier scripted response = Never");
				useScriptedResponse = ETrue;
				}
			else if(buf.FindF(_L("PandS")) != KErrNotFound)
				{
				aResult.iDestination = 0;
				RDebug::Printf("UPS notifier in Publish and Subscribe mode");
				useScriptedResponse = EFalse;
				}
			else
				{
				RDebug::Printf("UPS notifier - unknown scripted response = %S. Using PandS", &buf8);
				useScriptedResponse = EFalse;
				}
			
			if (buf.FindF(_L("1SecDelay")) != KErrNotFound)
				{
				aResultDelay = 1;
				RDebug::Printf("UPS notifier scripted response delay = 1 sec");
				}
			else if (buf.FindF(_L("3SecDelay")) != KErrNotFound)
				{
				aResultDelay = 3;
				RDebug::Printf("UPS notifier scripted response delay = 3 sec");
				}
			else if (buf.FindF(_L("5SecDelay")) != KErrNotFound)
				{
				aResultDelay = 5;
				RDebug::Printf("UPS notifier scripted response delay = 5 sec");
				}
			else if (buf.FindF(_L("10SecDelay")) != KErrNotFound)
				{
				aResultDelay = 10;
				RDebug::Printf("UPS notifier scripted response delay = 10 sec");
				}
			else if (buf.FindF(_L("30SecDelay")) != KErrNotFound)
				{
				aResultDelay = 30;
				RDebug::Printf("UPS notifier scripted response delay = 30 sec");
				}
			else
				{
				aResultDelay = 0;
				RDebug::Printf("UPS notifier scripted response delay not set. Using 0 sec delay");
				}
			
			CleanupStack::PopAndDestroy(2, &buf8);	// buf8, buf
			}
		CleanupStack::PopAndDestroy(&file);
		}
	
	else if(err==KErrNotFound)
		{
		RDebug::Printf("upstestnotifier.txt file not found. Working in P&S mode.");
		}
	
	CleanupStack::PopAndDestroy(&fs);	// fs
	return useScriptedResponse;
	}

CFileObserver::CFileObserver(CUpsTestNotifier& aNotifier)
  : CActive(EPriorityStandard), iNotifier(aNotifier) 
	{
	CActiveScheduler::Add(this);
	}

void CFileObserver::RunL()
	{
	User::After(1000000);
	TRAPD(ret, iNotifier.CheckDrivenByFileL());
	if (ret != KErrNone)
		{
		RDebug::Printf("IP UPS notifier error %d checking for script file", ret);
		}
	iFs.Close();
	}

void CFileObserver::DoCancel()
	{
	iFs.NotifyChangeCancel();
	iFs.Close();
	}

void CFileObserver::StartL()
	{
	User::LeaveIfError(iFs.Connect());
	iFs.NotifyChange(ENotifyWrite, iStatus, KScriptFile());
	SetActive();
	}

// End of file
