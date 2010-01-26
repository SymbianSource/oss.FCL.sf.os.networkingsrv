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

#ifdef _DEBUG

#include <e32property.h>
#include <e32base.h>

#include <comms-infras/coremcpractivities.h>
#include <comms-infras/agentmcpr.h>
#include <elements/nm_messages_base.h>
#include <comms-infras/ss_nodemessages_availability.h>

#include "agentmcpravailabilitytesting.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCgntm, "NifManAgtPrCgntm");
#endif

using namespace Messages;
using namespace ESock;

CAvailabilitySubscriber* CAvailabilitySubscriber::NewL(const Messages::TNodeCtxId& aAvailabilityActivity, TUint aApId)
	{
	CAvailabilitySubscriber* self = new (ELeave) CAvailabilitySubscriber(aAvailabilityActivity, aApId);
	CleanupStack::PushL(self);
	self->StartL();
	CleanupStack::Pop(self);
	return self;
	}

CAvailabilitySubscriber::CAvailabilitySubscriber(const Messages::TNodeCtxId& aAvailabilityActivity, TUint aApId)
:	CActive(EPriorityNormal),
	iAvailabilityActivity(aAvailabilityActivity),
	iApId(aApId)
	{
	}

void CAvailabilitySubscriber::StartL()
	{
	TInt score = TAvailabilityStatus::EMaxAvailabilityScore;
	if(iProperty.Attach(KAvailabilityTestingPubSubUid, iApId)==KErrNone)
		{
		CActiveScheduler::Add(this);
		iProperty.Subscribe(iStatus);
		SetActive();
		iProperty.Get(score); //If Get() is unsuccessful score must not be modified!
		}

	if (score == TAvailabilityStatus::EUnknownAvailabilityScore)
		{
		//EUnknownAvailabilityScore for testing allows
		//us to stall the returning of availability,
		//for example to allow a tier query to stay
		//in progress long enough to guarantee that
		//a cancel will be serviced
		}
	else
		{
		TAvailabilityStatus status(score);
		RClientInterface::OpenPostMessageClose(NodeId(), iAvailabilityActivity,
			TCFAvailabilityControlClient::TAvailabilityNotification(status).CRef());
		}
	}

void CAvailabilitySubscriber::RunL()
	{
	User::LeaveIfError(iStatus.Int());

	TInt publishedValue;
	User::LeaveIfError(iProperty.Get(publishedValue));

	TAvailabilityStatus availabilityStatus(publishedValue);
	TCFAvailabilityControlClient::TAvailabilityNotification msg(availabilityStatus);
	RClientInterface::OpenPostMessageClose(NodeId(), iAvailabilityActivity, msg);

	// .. and repeat..
	iProperty.Subscribe(iStatus);
	SetActive();
	}

TInt CAvailabilitySubscriber::RunError(TInt aError)
	{
	TEBase::TError msg(TCFAvailabilityProvider::TAvailabilityNotificationRegistration::Id(), aError);
	RClientInterface::OpenPostMessageClose(NodeId(), iAvailabilityActivity, msg);
	return KErrNone;
	}

void CAvailabilitySubscriber::DoCancel()
	{
	iProperty.Cancel();
	}

/*virtual*/ CAvailabilitySubscriber::~CAvailabilitySubscriber()
	{
	this->Cancel(); // object must be stoppable by descruction due to cleanup restrictions
	iProperty.Close();
	}

/*virtual*/ void CAvailabilitySubscriber::ReceivedL(const TRuntimeCtxId& /*aSender*/, const TNodeId& /*aRecipient*/, TSignatureBase& aMessage)
	{
	__ASSERT_DEBUG(TEBase::ERealmId == aMessage.MessageId().Realm(), User::Panic(KSpecAssert_NifManAgtPrCgntm, 1));
    __ASSERT_DEBUG(TEBase::TCancel::EId == aMessage.MessageId().MessageId(), User::Panic(KSpecAssert_NifManAgtPrCgntm, 2));
	delete this;
	};

#endif


