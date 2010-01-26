// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class declarations for User Prompt Service test/reference notifier implementation.
// 
//

/**
 @file upstestnotifier.h
 @internalComponent
 @test
*/

#ifndef UPSTESTNOTIFIER_H
#define UPSTESTNOTIFIER_H

#include <twintnotifier.h>
#include <ups/upsnotifierutil.h>

const TUint KUpsTestNotifierImplementation(0x10285887);
const TUint KOneSecond = 1000000;

IMPORT_C CArrayPtr<MNotifierBase2>* NotifierArray();

class CUpsTestNotifier;

class CFileObserver : public CActive
	{
public:
	CFileObserver(CUpsTestNotifier& aNotifier);
	void StartL();
	void RunL();
	void DoCancel();
private:
	CUpsTestNotifier& iNotifier;
	RFs iFs;
	};

/**
Test/reference implementation of a User Prompt Service notifier.
*/
class CUpsTestNotifier :  public CBase, public MNotifierBase2
	{
public:
	/**
	 * Construction/destruction
	 */
	static CUpsTestNotifier* NewL();
	~CUpsTestNotifier();


	/**
	 * Called when all resources allocated by notifiers should be freed.
	 */
	virtual void Release();

	/**
	 * Called when a notifier is first loaded to allow any initial construction that is required.
	 */
	virtual TNotifierInfo RegisterL();

	/**
	 * Return the priority a notifier takes and the channels it acts on.  The return value may be varied 
	 * at run-time.
	 */
	virtual TNotifierInfo Info() const;

	/**
	 * Start the notifier with data aBuffer and return an initial response.
	 */
	virtual TPtrC8 StartL(const TDesC8& aBuffer);

	/**
	 * Start the notifier with data aBuffer.  aMessage should be completed when the notifier is deactivated.
	 * May be called multiple times if more than one client starts the notifier.  The notifier is immediately
	 * responsible for completing aMessage.
	 */
	virtual void StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage);

	/**
	 * The notifier has been deactivated so resources can be freed and outstanding messages completed.
	 */
	virtual void Cancel();

	/**
	 * Update a currently active notifier with data aBuffer.
	 */
	virtual TPtrC8 UpdateL(const TDesC8& aBuffer);
	void CheckDrivenByFileL();
	
private:
	CUpsTestNotifier();
	void ConstructL();

	void DoStartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage);
	TBool CheckScriptedResponseL(UserPromptService::TPromptResult& aResult, TInt& aResultDelay);
	
	TNotifierInfo iInfo;
	TBool isDrivenByFile;	//Determines if the Notifier is driven by ini file or P&S
	TInt iNotifyCount;		//Maintains a count of the number of times the notifier has been invoked
	UserPromptService::TPromptResult iResult;	//Stores the button press result which should be fed back to the Dialogue Creator
	TInt iResultDelay;		//Delay in Sec before the result is returned back to the Dialogue Creator
	CFileObserver* iFileObserver;
	};

#endif // UPSTESTNOTIFIER_H
