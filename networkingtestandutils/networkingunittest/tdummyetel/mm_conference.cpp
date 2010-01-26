// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ETELEXT.H"
#include "Et_clsvr.h"
#include "ETELMM.H"
#include "mm_hold.h"

/************************************************************************/
//
//  RMobileConferenceCall
//
/************************************************************************/


EXPORT_C RMobileConferenceCall::RMobileConferenceCall()
	: iMmPtrHolder(NULL)
	{
	}

EXPORT_C void RMobileConferenceCall::ConstructL()
	{
	__ASSERT_ALWAYS(iMmPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iMmPtrHolder = CMobileConferenceCallPtrHolder::NewL(CMobileConferenceCallPtrHolder::EMaxNumberConferenceCallPtrSlots,
		CMobileConferenceCallPtrHolder::EMaxNumberConferenceCallPtrCSlots);
	}

EXPORT_C void RMobileConferenceCall::Destruct()
	{
	delete iMmPtrHolder;
	iMmPtrHolder = NULL;
	}

EXPORT_C TInt RMobileConferenceCall::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileConferenceCall subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=0,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelConferenceCall);	
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,&name));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RMobileConferenceCall::Close()
/**
 * This method closes the RMobileConferenceCall subsession
 */
 	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C TInt RMobileConferenceCall::GetCaps(TUint32& aCaps) const
/**
 * This method returns the conference call's capabilities
 *
 * \retval aCaps A descriptor that will contain the conference call capabilities
 * \return KErrNone
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobileConferenceCallGetCaps,ptr1);
	}

EXPORT_C void RMobileConferenceCall::NotifyCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if there are any changes in the conference call's capabilities
 *
 * \retval aCaps A descriptor that will contain the new conference call capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobileConferenceCallPtrHolder::ESlot1NotifyConferenceCallCapsChange,aCaps);

	Get(EMobileConferenceCallNotifyCapsChange,aReqStatus,ptr1);
	}

EXPORT_C void RMobileConferenceCall::CreateConference(TRequestStatus& aReqStatus) const
/**
 * This method creates a conference call by joining two single call objects.
 * One of the calls must be on hold and the other must be active & connected
 *
 * Request will complete either when the network confirms that the conference call is created or after
 * the phone has sent the request, depending upon availability of confirmation messages from the serving network
 *
 */
	{
	Blank(EMobileConferenceCallCreateConference,aReqStatus);
	}

EXPORT_C void RMobileConferenceCall::AddCall(TRequestStatus& aReqStatus, const TName& aCallName) const
/**
 * This method adds another party to an ongoing conference call
 * Either the single call or conference call must be on hold and the other must be active & connected
 *
 * Request will complete when the network confirms that the call has been added to the conference
 *
 * \param aCallName The name of the single call to be added to the conference
 *
 */
	{
	Set(EMobileConferenceCallAddCall,aReqStatus,aCallName);
	}

EXPORT_C void RMobileConferenceCall::Swap(TRequestStatus& aReqStatus) const
/**
 * This method swaps the conference call to its opposite state, either active or on hold
 * It will complete when the network confirms that the conference call has changed state
 */
	{
	Blank(EMobileConferenceCallSwap,aReqStatus);
	}

EXPORT_C void RMobileConferenceCall::HangUp(TRequestStatus& aReqStatus) const
/**
 * This method terminates a conference call and also terminates each of the individual calls within it
 */
	{
	Blank(EMobileConferenceCallHangUp,aReqStatus);
	}

EXPORT_C TInt RMobileConferenceCall::EnumerateCalls(TInt& aCount) const
/**
 * This method returns the number of individual calls within the conference call
 *
 * \retval aCount The number of calls - will be equal to 0 or 2+
 * \return KErrNone
 */
	{
	TPckg<TInt> ptr1(aCount);
	return Get(EMobileConferenceCallEnumerateCalls,ptr1);
	}

EXPORT_C TInt RMobileConferenceCall::GetMobileCallInfo(TInt aIndex, TDes8& aCallInfo) const
/**
 * This method returns the information related to one of the individual calls within the conference call
 *
 * \param aIndex The index of the call = 0 to aCount-1 where aCount is the returned value from EnumerateCalls
 * \retval aCallInfo A descriptor containing the large block of call information
 * \return KErrNone
 * \exception KErrNotFound if call information is not available
 */
	{
	TPckg<TUint> ptr1(aIndex);
	return Get(EMobileConferenceCallGetMobileCallInfo,ptr1,aCallInfo);	
	}

EXPORT_C TInt RMobileConferenceCall::GetConferenceStatus(TMobileConferenceStatus& aStatus) const
/**
 * This method returns the current state of the conference call
 *
 * \retval aStatus Will contain the current state
 * \return KErrNone
 */
	{
	TPckg<TMobileConferenceStatus> ptr1(aStatus);
	return Get(EMobileConferenceCallGetConferenceStatus,ptr1);
	}

EXPORT_C void RMobileConferenceCall::NotifyConferenceStatusChange(TRequestStatus& aReqStatus, TMobileConferenceStatus& aStatus) const
/**
 * This notification will complete when there is a change in the state of the conference call
 *
 * \retval aStatus Will contain the new state
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));	

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobileConferenceCallPtrHolder::ESlot1NotifyConferenceStatusChange,aStatus);

	Get(EMobileConferenceCallNotifyConferenceStatusChange,aReqStatus,ptr1);
	}

EXPORT_C void RMobileConferenceCall::NotifyConferenceEvent(TRequestStatus& aReqStatus, TMobileConferenceEvent& aEvent, TName& aCallName) const
/**
 * This notification will complete when a conference call event occurs
 *
 * \retval aEvent Will contain the conference event
 * \retval aCallName Will contain the name of the individual call, if the event is a call added or dropped from a conference
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));	

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobileConferenceCallPtrHolder::ESlot1NotifyConferenceEvent,aEvent);

	Get(EMobileConferenceCallNotifyConferenceEvent,aReqStatus,ptr1,aCallName);	
	}

