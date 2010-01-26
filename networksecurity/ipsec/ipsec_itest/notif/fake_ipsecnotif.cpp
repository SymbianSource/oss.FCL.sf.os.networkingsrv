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

#include "fake_ipsecnotif.h"

_LIT(KTestPassword, "testtest");


EXPORT_C CArrayPtr<MNotifierBase2>* NotifierArray()
	{
	CArrayPtrFlat<MNotifierBase2>* subjects = new (ELeave) CArrayPtrFlat<MNotifierBase2>( 2 );
	CIPSecDialogNotifier* notifier = new (ELeave) CIPSecDialogNotifier();
    CleanupStack::PushL( notifier );
    subjects->AppendL( notifier );
    CleanupStack::Pop( notifier );
    return subjects;
	}

//
// implementation of CIPSecDialogNotifier
//


CIPSecDialogNotifier::CIPSecDialogNotifier()
	{
	}

TInt CIPSecDialogNotifier::EnterPwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
    TIPSecDialogOutput usagePwd; 
    usagePwd.iOutBuf.Copy(KTestPassword);
	TPckgBuf<TIPSecDialogOutput> buf(usagePwd);
	aMessage.WriteL( aReturnValue, buf );
	(void)aBuffer;
	return KErrNone;
	}


TInt CIPSecDialogNotifier::ChangePwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
    TIPSecDialogOutput changePwd; 
	changePwd.iOutBuf.Copy(KTestPassword);
	changePwd.iOutBuf2.Copy(KTestPassword);
	TPckgBuf<TIPSecDialogOutput> buf(changePwd);
	aMessage.WriteL( aReturnValue, buf );
	(void)aBuffer;
	return KErrNone;
	}


TInt CIPSecDialogNotifier::DefinePwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
    TIPSecDialogOutput newUsagePwd;
	newUsagePwd.iOutBuf.Copy(KTestPassword);
	newUsagePwd.iOutBuf2.Copy(KTestPassword);
	TPckgBuf<TIPSecDialogOutput> buf(newUsagePwd);
	aMessage.WriteL( aReturnValue, buf );
	(void)aBuffer;
	return KErrNone;
	}


TInt CIPSecDialogNotifier::EnterImportPwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
	TIPSecDialogOutput newImportPwd; 
	newImportPwd.iOutBuf.Copy(KTestPassword);
	TPckgBuf<TIPSecDialogOutput> buf(newImportPwd);
	aMessage.WriteL( aReturnValue, buf );
	(void)aBuffer;
	return KErrNone;
	}


TInt CIPSecDialogNotifier::ShowNoteDlgL(TInt /*aDialog*/)
	{
	return KErrNone;
	}


void CIPSecDialogNotifier::Release()
	{
    delete this;
	}


CIPSecDialogNotifier::TNotifierInfo CIPSecDialogNotifier::RegisterL()
	{
    //iInfo.iUid = KUidIPSecDialogNotifier;
	iInfo.iUid = KUidPkiDialogNotifier;
    iInfo.iChannel = KCrystalScreenOutputChannel;
    iInfo.iPriority = ENotifierPriorityLow;
    return iInfo;
	}


CIPSecDialogNotifier::TNotifierInfo CIPSecDialogNotifier::Info() const
	{
    return iInfo;
	}


void CIPSecDialogNotifier::StartL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage )
	{
    __ASSERT_DEBUG( aBuffer.Length() >= 4, _L( "Buffer is empty" ) );
    TPckgBuf<TVpnDialogInfo> operationBuf;

    aMessage.ReadL( 1, operationBuf );
    TVpnDialogInfo& dialogInput = operationBuf();
 
	TInt aDialog = dialogInput.NoteDialogId();

	TInt operation = dialogInput.DialogId();    
    // This records whether the dialog was cancelled or not
    TInt dialogStatus = KErrNone;
    switch ( operation )
    {
	case TPkiDialog::EEnterPwd:
	    dialogStatus = EnterPwdL( aBuffer, aReturnValue, aMessage );
     	break;
	case TPkiDialog::EChangePwd:
		dialogStatus = ChangePwdL( aBuffer, aReturnValue, aMessage );
		break;
	case TPkiDialog::EDefinePwd:
		dialogStatus = DefinePwdL( aBuffer, aReturnValue, aMessage );
		break;
	case TPkiDialog::EEnterImportPwd:
		dialogStatus = EnterImportPwdL( aBuffer, aReturnValue, aMessage );
		break;
	case TNoteDialog::EInfo:
	case TNoteDialog::EWarning:
	case TNoteDialog::EError:
		ShowNoteDlgL(aDialog);
		break;
		//Fall through			
		default:
	__ASSERT_DEBUG( EFalse, _L( "Illegal IPSEC UI operation type" ) );
    }

    aMessage.Complete( dialogStatus );
	}


TPtrC8 CIPSecDialogNotifier::StartL( const TDesC8& /*aBuffer*/ )
	{
	__ASSERT_DEBUG( EFalse, User::Panic( _L( "Synchronous StartL not used" ), 0 ) );
	return NULL;
	}


void CIPSecDialogNotifier::Cancel()
	{
	}


TPtrC8 CIPSecDialogNotifier::UpdateL( const TDesC8& /*aBuffer*/ )
	{
	__ASSERT_DEBUG( EFalse, User::Panic( _L( "UpdateL not used" ), 0 ) );
	return NULL;
	}

#ifndef EKA2
GLDEF_C TInt E32Dll( TDllReason /*aReason*/ )
/**
 * DLL entry point
 */
	{
    return KErrNone;
	}
#endif