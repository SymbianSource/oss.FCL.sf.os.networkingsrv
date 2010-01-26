/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file fake_ipsecnotif.h Fake IPSec notifier for testing
*/
#ifndef IPSECNOTIFIER_H_
#define IPSECNOTIFIER_H_

#include <e32std.h>
#include <twintnotifier.h>

#include <vpnnotifierdefs.h>

const TUid KCrystalScreenOutputChannel={0x10009D48}; //notifier should go to a dialog channel, not a led or sound channel

// Method at ordinal 1 to get a list of notifiers from this dll
IMPORT_C CArrayPtr<MNotifierBase2>* NotifierArray();

//////////////////////////////////////////////////////////////////////////////////
//
//	The CIPSecDialogNotifier class is registered with the dialog server and
//	is responsible for receiving and sending messages and creating dialogs
//	for user requests.
//
//////////////////////////////////////////////////////////////////////////////////

class CIPSecDialogNotifier : public CBase, public MNotifierBase2
	{
public:
    CIPSecDialogNotifier();
    
private:	
    //method used for launching the dialog

    TInt EnterPwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage );
    TInt ChangePwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage);
	TInt DefinePwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage );
	TInt EnterImportPwdL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage );
	TInt ShowNoteDlgL(TInt aDialog);
	// from MNotifierBase2
   
    void Release();
    TNotifierInfo RegisterL();
    TNotifierInfo Info() const;
    void StartL( const TDesC8& aBuffer, TInt aReturnValue, const RMessagePtr2& aMessage );
    TPtrC8 StartL( const TDesC8& aBuffer);
    void Cancel();
    TPtrC8 UpdateL( const TDesC8& aBuffer);

private:
    TNotifierInfo iInfo; 	    // notifier info
	};
    

#endif
