// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file fake_ipsecnotif.cpp Fake IPSec notifier for testing
*/

#include <e32std.h>
#include <twintnotifier.h>

#include <secdlgImplDefs.h>

const TUid KCrystalScreenOutputChannel={0x10009D48}; //notifier should go to a dialog channel, not a led or sound channel

// Method at ordinal 1 to get a list of notifiers from this dll
IMPORT_C CArrayPtr<MNotifierBase2>* NotifierArray();

_LIT(KTestPassword, "pinkcloud");

class CTestSecDlgNotifier : public CBase, public MNotifierBase2
	{
public:
	static CTestSecDlgNotifier* NewL();
	CTestSecDlgNotifier() {}

private:

	TInt EnterPwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
		{
		TPINValue pinValue(KTestPassword());
		TPckg<TPINValue> pinValueBufPtr(pinValue);
		aMessage.WriteL(aReturnValue, pinValueBufPtr);
		
		(void)aBuffer;
		return KErrNone;
		}

	TInt ChangePwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
		{
		TTwoPINOutput output;
		output.iPINValueToCheck = KTestPassword;
		output.iNewPINValue = KTestPassword;
		TPckg<TTwoPINOutput> outputPckg(output);
		aMessage.WriteL(aReturnValue, outputPckg);
		
   		(void)aBuffer;
		return KErrNone;
		}

	virtual void StartL( const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage )
		{
		__ASSERT_DEBUG( aBuffer.Length() >= 4, _L( "Buffer is empty" ) );

		TUint operation = *reinterpret_cast<const TInt *>(aBuffer.Ptr()) & KSecurityDialogOperationMask;

		TInt dialogStatus = 0;
		switch (operation)
			{
			case EEnterPIN:
				dialogStatus = EnterPwdL( aBuffer, aReplySlot, aMessage );
				break;

			case EChangePIN:
				dialogStatus = ChangePwdL( aBuffer, aReplySlot, aMessage );
				break;

			case ESecureConnection:
			case ESignText:
			case EEnablePIN:
			case EDisablePIN:
			case EUnblockPIN:		
			case EUnblockPINInClear:
			case EPINBlocked:
				// these operations are not yet implemented in this test harness
				User::Leave(KErrNotFound);
				User::Panic(_L("CTestSecDlgNotifier"), 0);
				break;


			default:
				User::Panic(_L("CTestSecDlgNotifier"), 0);
			}

		aMessage.Complete( dialogStatus );
		}

	void Release() { delete this; }

	TNotifierInfo RegisterL()
		{
		iInfo.iUid = KUidSecurityDialogNotifier;
		iInfo.iChannel = KCrystalScreenOutputChannel;
		iInfo.iPriority = ENotifierPriorityLow;
		return iInfo;
		}

	TNotifierInfo Info() const { return iInfo; }
		
	virtual TPtrC8 StartL( const TDesC8& /*aBuffer*/ )
		{
		__ASSERT_DEBUG( EFalse, User::Panic( _L( "Synchronous StartL not used" ), 0 ) );
		return NULL;
		}

	void Cancel() {}

	TPtrC8 UpdateL( const TDesC8& /*aBuffer*/ )
		{
		__ASSERT_DEBUG( EFalse, User::Panic( _L( "UpdateL not used" ), 0 ) );
		return NULL;
		}

private:
	TNotifierInfo iInfo;
	};

//
// implementation of CTestSecDlgNotifier
//
/*
TInt CTestSecDlgNotifier::DefinePwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
    TIPSecDialogOutput newUsagePwd;
	newUsagePwd.iOutBuf.Copy(KTestPassword);
	newUsagePwd.iOutBuf2.Copy(KTestPassword);
	TPckgBuf<TIPSecDialogOutput> buf(newUsagePwd);
	aMessage.WriteL( aReturnValue, buf );
	(void)aBuffer;
	return KErrNone;
	}


TInt CTestSecDlgNotifier::EnterImportPwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
	TIPSecDialogOutput newImportPwd; 
	newImportPwd.iOutBuf.Copy(KTestPassword);
	TPckgBuf<TIPSecDialogOutput> buf(newImportPwd);
	aMessage.WriteL( aReturnValue, buf );
	(void)aBuffer;
	return KErrNone;
	}
*/

EXPORT_C CArrayPtr<MNotifierBase2>* NotifierArray()
	{
	CArrayPtrFlat<MNotifierBase2>* subjects = new (ELeave) CArrayPtrFlat<MNotifierBase2>( 2 );
	CTestSecDlgNotifier* notifier = new (ELeave) CTestSecDlgNotifier();
    CleanupStack::PushL( notifier );
    subjects->AppendL( notifier );
    CleanupStack::Pop( notifier );
    return subjects;
	}

#ifndef EKA2
GLDEF_C TInt E32Dll( TDllReason /*aReason*/ )
	{
    return KErrNone;
	}
#endif
