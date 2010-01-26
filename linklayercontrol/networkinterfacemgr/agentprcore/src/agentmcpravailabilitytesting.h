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

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef _AGENTMCPRAVAILABILITYTESTING_H_
#define _AGENTMCPRAVAILABILITYTESTING_H_

#include <comms-infras/ss_nodemessages.h>

#ifdef _DEBUG
const TUid KAvailabilityTestingPubSubUid={0x10272F42};

NONSHARABLE_CLASS(CAvailabilitySubscriber) : public CActive, public Messages::ASimpleNodeIdBase
	{
public:
	static CAvailabilitySubscriber* NewL(const Messages::TNodeCtxId& aAvailabilityActivity, TUint aApId);
	virtual ~CAvailabilitySubscriber();

private:
	CAvailabilitySubscriber(const Messages::TNodeCtxId& aAvailabilityActivity, TUint aApId);
	void StartL();
	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

private:
	Messages::TNodeCtxId iAvailabilityActivity;
	TInt iApId;
	RProperty iProperty;
	};
#endif

#endif // _AGENTMCPRAVAILABILITYTESTING_H_

