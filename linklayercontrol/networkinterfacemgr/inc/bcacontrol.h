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

#ifndef CBCACONTROL_H_
#define CBCACONTROL_H_

#include <e32base.h>
#include <networking/bca.h>
#include <networking/bcafactory.h>
#include <comms-infras/ni_log.h>


class MBcaControlObserver;

class CScriptBcaControl : public CActive
	{
public:
	CScriptBcaControl(MBcaControlObserver* aObserver);
	~CScriptBcaControl();

	void StartLoadL();
	void Shutdown(TInt aError);
    
protected:
	// Inherited from CActive.
	virtual void RunL();
	virtual void DoCancel();

private:
	void ObserverShutdownComplete();
#ifdef __FLOG_ACTIVE
	const TDesC8& StateString();
#endif

private: // Unowned data.
	enum TBcaState
		{
		/** BCA ready to start */
		EIdling,
		/** Ioctl(KBCASetIapId) issued on BCA */
		ESettingIap,
		/** Ioctl(KBCASetBcaStack) issued on BCA */
		ESettingBcaStack,
		/** Open() issued on BCA */
		EOpeningBca,
		/** BCA Open() complete */
		EBcaOpened,
		/** BCA being closed */
		EClosing
		};
	
	MBcaControlObserver* iObserver;
	/** BCA */	
	BasebandChannelAdaptation::MBca* iMBca;
	/** used to load BCA library */
	TAutoClose<RLibrary> iBcaDll;
	/** BCA state */
	TBcaState iState;
	/** NIF shut down error*/
	TInt iError;
	};
	
#endif //CBCACONTROL_H_


