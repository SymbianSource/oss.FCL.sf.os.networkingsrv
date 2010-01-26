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

#include <etelmm.h>
#include <comms-infras/es_availability.h>
#include <comms-infras/ss_mcprnodemessages.h>
#include "csdavailabilitylistener.h"
#include <elements/nm_messages_base.h>
#include <comms-infras/ss_nodemessages_availability.h>

using namespace Messages;
using namespace ESock;

static void GetPhoneInfoL(RTelServer& aTelServer, const TDesC& aLoadedTsyName, RTelServer::TPhoneInfo& aInfo)
	{
	TInt count;
	User::LeaveIfError(aTelServer.EnumeratePhones(count));
	if (count<=0)
		{
		User::Leave(KErrNotFound);
		}

	TBool found = EFalse;
	for (TInt i=0; i < count; i++)
		{
		TBuf<KCommsDbSvrMaxFieldLength> currentTsyName;
		User::LeaveIfError(aTelServer.GetTsyName(i,currentTsyName));

		TInt r=currentTsyName.Locate('.');
		if (r!=KErrNotFound)
			{
			currentTsyName.SetLength(r);
			}
		if (currentTsyName.CompareF(aLoadedTsyName)==KErrNone)
			{
			User::LeaveIfError(aTelServer.GetPhoneInfo(i,aInfo));
			found = ETrue;
			break;
			}
		}

	if (!found)
		{
		User::Leave(KErrNotFound);
		}
	}

CCsdAvailabilityListener* CCsdAvailabilityListener::NewL(const TNodeCtxId& aAvailabilityActivity, const CPppTsyConfig& aTsyProvision, TUint aApId)
	{
	CCsdAvailabilityListener* self = new (ELeave) CCsdAvailabilityListener(aAvailabilityActivity, aTsyProvision, aApId);
	CleanupStack::PushL(self);
	self->StartL();
	CleanupStack::Pop(self);
	return self;
	}

// CActive
void CCsdAvailabilityListener::RunL()
	{
	User::LeaveIfError(iStatus.Int());


	switch (iState)
		{
		case EInitialising:
			iPhone.GetNetworkRegistrationStatus(iStatus, iRegStatus);
			iState = EChecking;
			SetActive();
			break;
		case EChecking:
			//Fall through to EAttached / EUnAttached
		case EAvailable:
		case EUnAvailable:
			if (iRegStatus == RMobilePhone::ERegisteredOnHomeNetwork
				|| iRegStatus == RMobilePhone::ERegisteredRoaming)
				{
				if (iState!=EAvailable) //Could be EUnAvailable or EChecking
					{
					TAvailabilityStatus availabilityStatus(TAvailabilityStatus::EMaxAvailabilityScore);
					RClientInterface::OpenPostMessageClose(Id(), iAvailabilityActivity, TCFAvailabilityControlClient::TAvailabilityNotification(availabilityStatus).CRef());
					iState = EAvailable;
					}
				}
			else
				{
				if (iState!=EUnAvailable) //Could be EAttached or EChecking
					{
					TAvailabilityStatus availabilityStatus(TAvailabilityStatus::EMinAvailabilityScore);
					RClientInterface::OpenPostMessageClose(Id(), iAvailabilityActivity, TCFAvailabilityControlClient::TAvailabilityNotification(availabilityStatus).CRef());
					iState = EUnAvailable;
					}
				}

			iPhone.NotifyNetworkRegistrationStatusChange(iStatus, iRegStatus);
			SetActive();
			break;

		default:
			ASSERT(EFalse);
		}
	}

TInt CCsdAvailabilityListener::RunError(TInt aError)
	{
	RClientInterface::OpenPostMessageClose(Id(), iAvailabilityActivity,
		TEBase::TError(TCFAvailabilityProvider::TAvailabilityNotificationRegistration::Id(), aError).CRef());

	return KErrNone;
	}

void CCsdAvailabilityListener::DoCancel()
	{
	switch (iState)
		{
		case EInitialising:
			iPhone.InitialiseCancel();
			break;
		case EChecking:
			iPhone.CancelAsyncRequest(EMobilePhoneGetNetworkRegistrationStatus);
			//Fall through to EAttached / EUnAttached
		case EAvailable:
		case EUnAvailable:
			iPhone.CancelAsyncRequest(EMobilePhoneNotifyNetworkRegistrationStatusChange);
			break;
		default:
			ASSERT(EFalse);
		}
	}

CCsdAvailabilityListener::~CCsdAvailabilityListener()
	{
	Cancel();
	iPhone.Close();
	iTelServer.Close();
	}

CCsdAvailabilityListener::CCsdAvailabilityListener(const TNodeCtxId& aAvailabilityActivity, const CPppTsyConfig& aTsyProvision, TUint aApId)
:	CActive(EPriorityNormal),
	iAvailabilityActivity(aAvailabilityActivity),
	iTsyProvision(&aTsyProvision),
	iApId(aApId),
	iState(EInitialising)
	{
	CActiveScheduler::Add(this);
	}

void CCsdAvailabilityListener::StartL()
	{
	User::LeaveIfError(iTelServer.Connect());
	User::LeaveIfError(iTelServer.LoadPhoneModule(iTsyProvision->TsyName()));
	User::LeaveIfError(iTelServer.SetExtendedErrorGranularity(RTelServer::EErrorExtended));

	RTelServer::TPhoneInfo phoneInfo;
	GetPhoneInfoL(iTelServer, iTsyProvision->TsyName(), phoneInfo);
	User::LeaveIfError (iPhone.Open(iTelServer, phoneInfo.iName));

	RPhone::TStatus phoneStatus;
	iPhone.GetStatus(phoneStatus);

	if(phoneStatus.iMode==RPhone::EModeUnknown)
		{
		iPhone.Initialise(iStatus);
		}
	else
		{
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrNone);
		}
	SetActive();
	}

void CCsdAvailabilityListener::ReceivedL(const TRuntimeCtxId& /*aSender*/, const TNodeId& /*aRecipient*/, TSignatureBase& aMessage)
	{
	(void)aMessage;
	ASSERT(TEBase::ERealmId == aMessage.MessageId().Realm());
    ASSERT(TEBase::TCancel::EId == aMessage.MessageId().MessageId());
	delete this;
	};

