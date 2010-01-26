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
//

#ifndef CSDAVAILABILITYLISTENER_H
#define CSDAVAILABILITYLISTENER_H

#include <comms-infras/coremcpractivities.h>
#include <networking/pppconfig.h>
#include <etel.h>
#include <etelmm.h>

class CPppMetaConnectionProvider;

NONSHARABLE_CLASS(CCsdAvailabilityListener) : public CActive, public Messages::ASimpleNodeIdBase
/** Circuit switched availability listeners

@internalTechnology
*/
	{
	enum TState {EInitialising, EChecking, EAvailable, EUnAvailable};

public:
	static CCsdAvailabilityListener* NewL(const Messages::TNodeCtxId& aAvailabilityActivity, const CPppTsyConfig& aTsyProvision, TUint aApId);
	virtual ~CCsdAvailabilityListener();

private:
	CCsdAvailabilityListener(const Messages::TNodeCtxId& aAvailabilityActivity, const CPppTsyConfig& aTsyProvision, TUint aApId);
	void StartL();
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

	// CActive
	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();

private:
	const Messages::TNodeCtxId iAvailabilityActivity;
	const CPppTsyConfig* iTsyProvision;
	RMobilePhone iPhone;
	RTelServer iTelServer;
	TInt iApId;
	RMobilePhone::TMobilePhoneRegistrationStatus iRegStatus;
	TState iState;
	};

#endif
