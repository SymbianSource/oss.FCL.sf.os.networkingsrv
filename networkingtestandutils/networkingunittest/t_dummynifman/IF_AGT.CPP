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

/**
 @file
*/

#include "IF_DEF.H"
#include "NI_STD.H"
#include "Ni_Log.h"
#include <in_sock.h>
#include <es_prot.h>
#include <in_iface.h>
#include "NIFConfigurationControl.h"
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifvar_internal.h>
#include <nifman_internal.h>
#endif

const TInt KAsyncUnBindPriority = 10000;
const TInt KTimerTick = 1000000;
const TInt KTimerCorrectionPeriod = 4;
/**
Don't ever want to set a timer for much less than 1/5 of a second
*/
const TInt KMinTimerTick = KTimerTick / 5;

CBinderRef::CBinderRef(MNifIfUser* aNetwork, CNifAgentRef& aAgentRef, TAny* aId)
/**
Constructor
*/
	: CActive(KAsyncUnBindPriority)
	{

	aAgentRef.iBinders.AddLast(*this);
	iAgentRef = &aAgentRef;
	iNetworkLayer = aNetwork;
	iNetworkLayer->IfUserOpenNetworkLayer();
	iId=aId;
	CActiveScheduler::Add(this);
	}

CBinderRef::~CBinderRef()
/**
Destructor - deque from link
*/
	{

	Cancel();
	iLink.Deque();
	iAgentRef->iTimerDisableCount++;
	iNetworkLayer->IfUserCloseNetworkLayer();
	iAgentRef->iTimerDisableCount--;
	if(iLinkLayer)
		iLinkLayer->Close();
	}

void CBinderRef::BindL(CNifIfLink* aInterface)
/**
Bind this interface to object returned by interface
*/
	{

	__ASSERT_DEBUG(!iLinkLayer, Panic(ENifManPanic_LinkLayerAlreadyExists));

	LOG( NifmanLog::Printf(_L("BinderRef %x:\tBinding interface(%x) to network layer(%x)"), this, aInterface, iNetworkLayer); )

	TServerProtocolDesc inf;
	iNetworkLayer->IfUserProtocol()->Identify(&inf);
	CNifIfBase* linklayer = aInterface->GetBinderL(inf.iName);
	TAny* id=iId;

	if (linklayer!=NULL)
		{
		LOG( NifmanLog::Printf(_L("BinderRef %x:\tLink layer for binding interface(%x) to network layer(%x) is '%S' (%x)"), this, aInterface, iNetworkLayer, &inf.iName, linklayer); )
		linklayer->Open();
		CleanupStack::PushL(TCleanupItem(CNifIfBase::Cleanup, linklayer));
		iId=0; // If new interface fails protocol does not get an IfUserBindFailure
		LOG( NifmanLog::Printf(_L("BinderRef %x:\tInforming network layer(%x) of new link layer(%x)"), this, iNetworkLayer, linklayer); )
		iNetworkLayer->IfUserNewInterfaceL(linklayer, id);
		CleanupStack::Pop();
		iLinkLayer = linklayer;
		}
	else
		{
		iId=0; // If new interface fails protocol does not get an IfUserBindFailure
		LOG( NifmanLog::Printf(_L("BinderRef %x:\tInforming network layer(%x) of new link layer(%x)"), this, iNetworkLayer, aInterface); )
		iNetworkLayer->IfUserNewInterfaceL(aInterface, id);
		iLinkLayer = aInterface;
		iLinkLayer->Open();
		}

	LOG( NifmanLog::Printf(_L("BinderRef %x:\tSuccessfully bound network layer(%x) to link layer(%x)"), this, iNetworkLayer, iLinkLayer); )
	}

void CBinderRef::LinkDown(TInt aError, TBool)
/**
Lower layer link down error
*/
	{

	__ASSERT_DEBUG(iLinkLayer, Panic(ENifManPanic_LinkLayerNotEstablished));
	if(!IsActive())
		{
		TRequestStatus* p = &iStatus;
		User::RequestComplete(p, aError);
		SetActive();
		}
	}

void CBinderRef::UnBind(TInt aError,TBool)
/**
Notify fact that link layer has dissappear even before it started
*/
	{

	if(iLinkLayer) // means binding happened successfully
	    iNetworkLayer->IfUserInterfaceDown(aError, iLinkLayer);
	else if(iId)
		iNetworkLayer->IfUserBindFailure(aError, iId);
	delete this;
	}

void CBinderRef::DoCancel()
	{

	}

void CBinderRef::RunL()
	{

	UnBind(iStatus.Int());
	}

CNifAgentRef::CNifAgentRef(CObject& aFactory)
/**
Constructor
*/
:
	iIsSubClassed(EFalse)
	{

	iBinders.SetOffset(_FOFF(CBinderRef, iLink));
	SetOwner(&aFactory);
	Owner()->Open();
	SocketServExt::OpenSession();
	}

CNifAgentRef::CNifAgentRef(TInt)
//
// C'tor
//
:
	iIsSubClassed(EFalse)
	{
	}

CNifAgentRef::~CNifAgentRef()
/**
Destructor
*/
	{

	if(iIsSubClassed)
		{
		TInt ac = CObject::AccessCount();
		if(ac==1)
			CObject::Dec();
		return;
		}

	LOG( NifmanLog::Printf(_L("AgentRef %x:\tDestructor called"), this); )

	BindFailAll(KErrDied);

	ServiceClosed();
	SocketServExt::CloseSession();
	iSessions.Reset();

	// Inform interface state change observers that the interface is going down.
	CNifMan::Global()->AgentGoingDown(*this);

	// tell the network controller that this agent is no-longer available
	if(iNetCon)
		{
		iNetCon->AgentConnectionFailure(iAgent, KErrNone);
		iNetCon = NULL;
		}

	// If we're using the extended management compatibility layer, delete it
	if(iNifExtndMngmntCompatibilityLayer)
		delete iNifExtndMngmntCompatibilityLayer;

	delete iAgent;
	Owner()->Close();
	TimerDelete(); // will also cancel timer
	}

CNifAgentRef* CNifAgentRef::NewL(const TDesC& aName, CNifAgentFactory& aFactory)
/**
create new agent reference
*/
	{
	CNifAgentRef* ref = new (ELeave) CNifAgentRef(aFactory);
	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, ref));
	ref->ConstructL(aName, aFactory);
	CleanupStack::Pop();
	return ref;
	}

void CNifAgentRef::ConstructL(const TDesC& aName, CNifAgentFactory& aFactory)
/**
Dynamic construction
*/
	{

	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tConstructL('%S', aFactory 0x%x)"), this, &aName); )

	// find the network controller if there is one
	iNetCon = CNifMan::Global()->NetworkController();

	TimerConstructL(ESocketTimerPriority);
	iAgent = aFactory.NewAgentL(aName);
	iAgent->iNotify = this;
	CNifMan::Global()->AddAgentL(this);
	Progress(KConnectionUninitialised, KErrNone);

	(void)RefreshConnectionInfo();
	}

void CNifAgentRef::RefreshConnectionInfo()
/**
Update the IAP id and Network ID of this agent
*/
	{

	_LIT(KIAPId, "IAP\\Id");
	_LIT(KIAPNetwork, "IAP\\IAPNetwork");

	TSoIfConnectionInfo info;
	TInt err = ReadInt(KIAPId(), info.iIAPId);
	if(err!=KErrNone)
		return;

	err = ReadInt(KIAPNetwork(), info.iNetworkId);
	if(err!=KErrNone)
		return;

	SetConnectionInfo(info);
	}

TName CNifAgentRef::Name() const
/**
Override with name of agent
*/
	{
	if(iAgent)
		{
		TNifAgentInfo info;
		iAgent->Info(info);
		return info.iName;
		}
	else
		return CObject::Name();
	}

TInt CNifAgentRef::Open()
	{

	TInt ret = CObject::Open();

	LOG( NifmanLog::Printf(_L("AgentRef %x:\tOpen() - new reference count is %d"), this, AccessCount()); )

	return ret;
	}

void CNifAgentRef::Close()
/**
Close - only destroy if in an idle state
*/
	{

	ASSERT(AccessCount()>0);

	Dec();

	LOG( NifmanLog::Printf(_L("AgentRef %x:\tClose() - new reference count is %d"), this, AccessCount()); )

	if (AccessCount()==0 && iState==EIdle)
		{
		delete this;
		}
	}

TInt CNifAgentRef::AddNetworkLayer(MNifIfUser* aNetwork, TAny* aId)
/**
First search to see if it already exists
Create the binding reference
If the interface exists go straight to binding
*/
	{
	LOG( NifmanLog::Printf(_L("AgentRef %x:\tAdding a new binder for network layer(%x)"), this, aNetwork); )

	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b;
	while((b=iter++)!=0)
		{
		if(b->iNetworkLayer==aNetwork)
			{
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tFound existing binder(%x) between network layer(%x) and link layer(%x)"), this, b, aNetwork, b->iLinkLayer); )
			return KErrAlreadyExists;
			}
		}

	// Network layer trying to bind but interface in progress
	if(!aId || (iState==EConnecting && !iInterface) || iState==EIdle || iState==EDisconnecting)
		{
		b= new CBinderRef(aNetwork, *this, aId);
		if(!b)
			return KErrNoMemory;

		if(iInterface && !aId)
			{
			TRAPD(res, b->BindL(iInterface));
			if(res!=KErrNone)
				{
				LOG( NifmanLog::Printf(_L("AgentRef %x:\tError(%d) binding interface(%x) to network layer(%x)"), this, res, iInterface, aNetwork); )
				b->UnBind(res);
				}
			return res;
			}
		return KErrNone;
		}
	return KErrLocked;
	}

void CNifAgentRef::LinkDownAll(TInt aError,TBool)
/**
Link has gone down
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tLinkDownAll(%d)"), this, aError); )

	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* p;
	while((p=iter++)!=0)
		p->LinkDown(aError);
	}

void CNifAgentRef::BindFailAll(TInt aError)
/**
Unbind all those which are bound
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tBindFailAll(%d)"), this, aError); )

	Progress(KConnectionFailure, aError);

	while(!iBinders.IsEmpty())
		iBinders.First()->UnBind(aError);
	}

void CNifAgentRef::SetUsageProfile(TUint aProfile)
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tSetUsageProfile(%d)"), this, aProfile); )
	switch (aProfile)
		{
	case KConnProfileMedium:
		if (iUserCount++ == 0)
			{
			// Move from short to medium timer
			if (iTimerMode == ETimerShort)
				{
				LOG( NifmanLog::Printf(_L("AgentRef %x:\tUsage profile %d - timer mode set to Medium"), this, aProfile); )
				SetTimerMode(ETimerMedium);
				}
			}
		break;
	case KConnProfileNone:
		__ASSERT_DEBUG(iUserCount >= 1, Panic(ENifManPanic_NegativeCloseCount));

		if (--iUserCount == 0)
			{
			// Moving from medium to short timer
			if (iTimerMode == ETimerMedium && iRouteCount == 0)
				{
				// Moving from medium to short timer
				LOG( NifmanLog::Printf(_L("AgentRef %x:\tUsage profile %d - timer mode set to Short"), this, aProfile); )
				SetTimerMode(ETimerShort);
				}
			}
		break;
	default:
		Panic(ENifManPanic_UsageProfileOutOfRange);
		}

	}

TInt CNifAgentRef::IsNetworkLayerActive()
/**
Return state to client side
*/
	{

	if(iBinders.IsEmpty())
		return KErrNotReady;

	return IsAnyNetworkLayerOpen();
	}

TBool CNifAgentRef::IsAnyNetworkLayerOpen()
/**
Is any network layer open
*/
	{

	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b;
	while((b=iter++)!=0)
		{
		if(b->iNetworkLayer->IfUserIsNetworkLayerActive())
			{
		    return ETrue;
			}
		}

	return EFalse;
	}

TBool CNifAgentRef::AreAllNetworkLayersOpen()
/**
Are all link layers open
*/
	{

	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b;

	while((b=iter++)!=0)
		{
		TIfStatus linkStatus=TIfStatus(b->iLinkLayer->State());

		if(linkStatus==EIfPending || linkStatus==EIfDown)
			{
			return EFalse;
			}
		}
	return ETrue;
	}

void CNifAgentRef::NetworkLayerClosed(MNifIfUser* aNetworkLayer)
/**
Network layer no longer has any clients
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tNetworkLayerClosed(MNifIfUser %x)"), this, aNetworkLayer); )

	// is aNetworkLayer one of our bindees?
	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b;
	TBool found(EFalse);
	while((b=iter++)!=0 && !found)
		{
		if(b->iNetworkLayer == aNetworkLayer)
			found = ETrue;
		}

	if(!found)
		return;

	if(iTimer->IsActive() || iTimerDisableCount>0)
		return;

	if(iState==ELinkUp || iState==EIdle)
		{
		ResetTimer();
		}
	}

TInt CNifAgentRef::GetTimerThresholds()
	{

	TUint32 value;
	TInt ret = ReadInt(TPtrC(LAST_SESSION_CLOSED_TIMEOUT), value);
	if (ret!= KErrNone)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tCould not read short timeout value - disabling this timer"), this); )
		value = KMaxTUint32;
		}

	iTickThreshold[ETimerShort] = (TInt) value;
	LOG( NifmanLog::Printf(_L("AgentRef %x:\tShort timeout value set to %d ticks"), this, iTickThreshold[ETimerShort]); )

	ret = ReadInt(TPtrC(LAST_SOCKET_CLOSED_TIMEOUT), value);
	if (ret!= KErrNone)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tCould not read medium timeout value - disabling this timer"), this); )
		value = KMaxTUint32;
		}

	iTickThreshold[ETimerMedium] = (TInt) value;
	LOG( NifmanLog::Printf(_L("AgentRef %x:\tMedium timeout value set to %d ticks"), this, iTickThreshold[ETimerMedium]); )

	ret = ReadInt(TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT), value);
	if (ret!= KErrNone)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tCould not read long timeout value - disabling this timer"), this); )
		value = KMaxTUint32;
		}

	iTickThreshold[ETimerLong] = (TInt) value;
	LOG( NifmanLog::Printf(_L("AgentRef %x:\tLong timeout value set to %d ticks"), this, iTickThreshold[ETimerLong]); )

	iTickThreshold[ETimerImmediate] = 0;
	return KErrNone;
	}

void CNifAgentRef::TimerComplete(TInt)
/**
Idle timer one second tick.

Determine whether a change of timer mode is required depending on what objects are currently
using the interface:

Switch to short timer mode if there are no ESock RConnection objects and no protocol SAP's.
Switch to medium timer mode if there are only RConnection objects.
Switch to long timer mode if there are any protocol SAP's.

If there has been any packet activity, then reset the long timer.
If there has been any connectin control activity (Start or Attach operations), then reset the medium timer.

Note that the RConnection objects referred to here are of the "normal" type
rather than the "monitoring" type.  Monitoring RConnection objects do not affect
the idle timer and will not be reported to Nifman by ESock.

The one second polling implementation of the idle timer is used because there is no other means to
determine when the protocol no longer has any SAP's excepts via IsAnyNetworkLayerOpen().

*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tTimer Complete %d/%d ticks"), this, iExpiredTicks+1, iTickThreshold[iTimerMode]); )

	if (iTimerMode == ETimerImmediate)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tIdle timeout completed - stopping interface"), this); )

		Stop(KErrCancel);
		return;
		}

	__ASSERT_DEBUG(iState==ELinkUp, Panic(ENifManPanic_BadLinkUpState));

	// Determine if we need to alter the timer mode
	TTimerType newMode = DecideTimerMode();
	iConnectionControlActivity = EFalse;		// (do not reset before DecideTimerMode())

	if (newMode != ETimerUnknown)				// set new timer mode if required
		SetTimerMode(newMode);

	if (iPeriodActivity)
		{
		iPeriodActivity = EFalse;

		// Reset the timer on packet activity (if the timer mode hasn't just been changed).
		// (Should this reset only the Long timer, or should it reset the timer in all modes ?)

		if (newMode == ETimerUnknown)
			{
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer reset due to packet activity"), this); )
			ResetTimer();
			}
		}
	else
		{
		if (newMode == ETimerUnknown)
			{
			// No Activity and no change in timer state, check if timer has expired
			if (iTickThreshold[iTimerMode] != (TInt)KMaxTUint32 &&
				iTickThreshold[iTimerMode] <= ++iExpiredTicks)
				{
				// Timer expired!
				iOrigReason = KErrTimedOut;
				iState = EStopping;

				LOG( NifmanLog::Printf(_L("AgentRef %x:\tIdle timeout completed - stopping interface"), this); )

				iInterface->Stop(KErrTimedOut, MNifIfNotify::EDisconnect);
				}
			else
				{
				StartNextTick();
				}
			}
		}
	}

CNifAgentRef::TTimerType CNifAgentRef::DecideTimerMode()
/**
Determine the mode that the idle timer should be running in based on current conditions.
                      
@return idle timer mode
*/
	{
	TTimerType newMode = ETimerUnknown;

	switch (iTimerMode)
		{
	case ETimerShort:
		if (iRouteCount > 0)				// any Socket Flows or ESock Sessions ?
			{
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer mode changed from Short to Long due to presence of flows"), this); )

			newMode = ETimerLong;
			}
		else if (iUserCount > 0)			// any RConnection objects ?
			{
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer mode changed from Short to Medium due to presence of RConnection objects"), this); )

			newMode = ETimerMedium;
			}
		break;
	case ETimerMedium:
		if (iConnectionControlActivity)
			{
			// there has been connection Start/Attach type activity, so reset medium timer

			LOG( NifmanLog::Printf(_L("AgentRef %x:\tMedium timer reset due to connection control activity"), this); )

			newMode = ETimerMedium;
			}
		else if (iRouteCount > 0)
			{
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer mode changed from Medium to Long due to presence of flows"), this); )

			newMode = ETimerLong;
			}
		else if (iUserCount == 0)
			{
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer mode changed from Medium to Short due to absence of flows and RConnection objects"), this); )

			newMode = ETimerShort;
			}
		break;
	case ETimerLong:
		if (iRouteCount == 0)
			{
			if (iUserCount > 0)
				{
				LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer mode changed from Long to Medium due to presence of RConnection objects"), this); )

				newMode = ETimerMedium;
				}
			else
				{
				LOG( NifmanLog::Printf(_L("AgentRef %x:\tTimer mode changed from Long to Short due to absence of flows and RConnection objects"), this); )

				newMode = ETimerShort;
				}
			}
		break;
	default:
		break;
		}

	return (newMode);
	}

void CNifAgentRef::ResetTimer()
/**
Restart the Idle Timer.
Used when switching the timer into a different mode of operation.
*/
	{
	LOG (
		TBuf<9> mode;	// enough for "Immediate"
		TInt len(0);
		switch(iTimerMode)
			{
			case ETimerLong:
				mode = _L("Long");
				len = iTickThreshold[ETimerLong];
				break;
			case ETimerMedium:
				mode = _L("Medium");
				len = iTickThreshold[ETimerMedium];
				break;
			case ETimerShort:
				mode = _L("Short");
				len = iTickThreshold[ETimerShort];
				break;
			case ETimerImmediate:
				mode = _L("Immediate");
				break;
			default:
				mode = _L("Unknown");
			}
		NifmanLog::Printf(_L("AgentRef %x:\tTimer mode set to %S (%d ticks)"), this, &mode, len);
		)

	// if we are not in the packet activity monitoring mode, reset the activity flag.
	if (iTimerMode != ETimerLong)
			iPeriodActivity = EFalse;

	if(iState!=ELinkUp || iTimer->IsActive() || iTimerDisableCount>0)
		return;

	iExpiredTicks = 0;
	iTotalTimerDrift = 0;
	iDriftCheckTime.HomeTime();
	TimerAfter(KTimerTick);
	}

void CNifAgentRef::StartNextTick()
/**
 * Start the next one second tick for the Idle Timer.
 *
 * As the Idle Timer is implemented as a repeated one second timeout rather than a single timeout period,
 * this routine is necessary to ensure overall accuracy.  The period of each "one second" tick
 * is adjusted slightly to compensate for any accumulated inaccuracies.
 */
	{
	/*
	The inactivity timeout period is made up of a number of successive
	one second timer periods up to the desired inactivity timeout.
	This is done because certain operations are needed every second.
	However, this can result in cumulative errors in the final timeout
	period.  An attempt is made here to keep the final timeout period
	accurate by adjusting the duration of a timer tick every so often
	to compensate for any observed drift.  This is only best-effort
	synchronisation which ignores drift that seems way out - as could
	happen if user altered system time, for example,
	*/

	if (iExpiredTicks % KTimerCorrectionPeriod == 0)
		{
		TTime currentTime;
		currentTime.HomeTime();
		
		// Time interval for timer synch & high limit for validity of observed timer drift
		const TTimeIntervalMicroSeconds KTimeCheckInterval(KTimerCorrectionPeriod * KTimerTick);
			
		iDriftCheckTime += KTimeCheckInterval;

		// Only act on latest timer drift if it's within sensible limits.
		// Might not be if user has reset system time, for example.
		if ( currentTime > iDriftCheckTime )
			{
			TInt64 t = currentTime.MicroSecondsFrom(iDriftCheckTime).Int64();
			if ( t < KTimeCheckInterval.Int64())
				iTotalTimerDrift += I64LOW(t);
			}
		else
			{
			TInt64 t = iDriftCheckTime.MicroSecondsFrom(currentTime).Int64();
 			if (t < KTimerTick-KMinTimerTick)
				iTotalTimerDrift -= I64LOW(t);
			}
		
		iDriftCheckTime = currentTime;

		if (iTotalTimerDrift > KTimerTick - KMinTimerTick)
			TimerAfter(KMinTimerTick);
		else if (iTotalTimerDrift > 0)
			TimerAfter(KTimerTick - iTotalTimerDrift);
		else
			TimerAfter(KTimerTick);			
		}
	else
		TimerAfter(KTimerTick);
	}

void CNifAgentRef::NegotiationFailed(CNifIfBase* aIf, TInt aError)
/**
Negotiation failed for an interface
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tNegotiationFailed(aIf %d, aError %d)"), this, aIf, aError); )

	if(aIf==iInterface)
		return;

	CBinderRef* b = FindBinder(aIf);

	if(b)
		{
		b->UnBind(aError);   // deletes b
		b = NULL;
		}
	}

CBinderRef* CNifAgentRef::FindBinder(CNifIfBase* aIf)
/**
Find the binder for a given interface
*/
	{

	CBinderRef* p=0;
	TDblQueIter<CBinderRef> iter(iBinders);

	while((p=iter++)!=0)
		if(p->iLinkLayer==aIf)
			return p;

	return NULL;
	}

CBinderRef* CNifAgentRef::FindBinder(MNifIfUser* aNetworkLayer)
/**
Find the binder for a given network layer
*/
	{

	CBinderRef* b=0;
	TDblQueIter<CBinderRef> iter(iBinders);

	while((b=iter++)!=0)
		if(b->iNetworkLayer==aNetworkLayer)
			return b;

	return NULL;
	}

void CNifAgentRef::BinderLayerDown(CNifIfBase* aBinderIf, TInt aReason, TAction aAction)
/**
The CNifIfBase object that binds the link-layer to the network-layer has gone down
Rebind link-layer to the given network-layer if necessary
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tBinderLayerDown(aIf %d, aReason %d, aAction %d)"), this, aBinderIf, aReason, aAction); )

	if(aBinderIf==iInterface)
		return;

	CBinderRef* b = FindBinder(aBinderIf);

	if(b)
		{
		MNifIfUser* networkLayer = b->iNetworkLayer;

		networkLayer->IfUserOpenNetworkLayer();
		b->UnBind(aReason);   // deletes b
		b = NULL;

		if(aAction==EReconnect && networkLayer)
			{
			TInt res = AddNetworkLayer(networkLayer);
			if(res==KErrNone)
				{
				b = FindBinder(networkLayer);
				iInterface->Restart(b->iLinkLayer);
				}
			}

		networkLayer->IfUserCloseNetworkLayer();
		}
	}

TInt CNifAgentRef::Start(TAgentConnectType aType)
/**
Start but return KErrAlready if in progress
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tStart(TAgentConnectType %d) iState=%d"), this, aType, iState); )

/**
Start but return KErrAlready if in progress
*/
	if (iState==EIdle)
		{
		iState=EConnecting;
		iConnectType=aType;
		GetTimerThresholds();
		iAgent->Connect(iConnectType);
		// inform interface state change observers that an interface is coming up
		CNifMan::Global()->AgentGoingUp(*this);
		return KErrNone;
		}
	else if(iState==EDisconnecting && !iReconnectAfterDisconnect)
		{
		iReconnectAfterDisconnect=ETrue;
		iConnectType=aType;             // set the type to start when disconnection complete
		return KErrNone;
		}

	return KErrAlreadyExists;
	}

TInt CNifAgentRef::Start(CStoreableOverrideSettings* aOverrides, TAgentConnectType aType)
//
// Start but return KErrAlready if in progress
//
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tStart(aOverrides %x, TAgentConnectType %d) iState=%d"), this, aOverrides, aType, iState); )

	if (iState==EIdle)
		{
		iState=EConnecting;
		iConnectType=aType;
		GetTimerThresholds();
		iAgent->Connect(iConnectType, aOverrides);
		// inform interface state change observers that an interface is coming up
		CNifMan::Global()->AgentGoingUp(*this);
		return KErrNone;
		}
	else if(iState==EDisconnecting && !iReconnectAfterDisconnect)
		{
		iReconnectAfterDisconnect=ETrue;
		iConnectType=aType;             // set the type to start when disconnection complete
		return KErrNone;
		}

	return KErrAlreadyExists;
	}

void CNifAgentRef::SendIoctlMessageL(const RMessage2& aMessage)
/**
 * SendIoctlMessageL forwards Ioctl request to the target
 * Important - message must be completed by the target. There is no notification back
 *       to the caller => meant for forwarding messages the forwarding path is not really
 *       interested in apart from "routing informations"
 * @param aMessage forwarded message (it's the caller's resposibility to forward just Ioctl
 *                   messages)
 */
	{
   	if (ipNifConfigurationControl)
   		{
         ipNifConfigurationControl->SendIoctlMessageL(aMessage);
   		}
   	else
   		{
         User::Leave(KErrNotReady);
   		}
	}

void CNifAgentRef::SendCancelIoctl()
/**
  * forwards cancel ioctl request to the target
  *
  */
  	{
   	if (ipNifConfigurationControl)
   		{
         ipNifConfigurationControl->Cancel();
   		}
  	}

TInt CNifAgentRef::Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption)
/**
Control either the interface or the agent
*/
	{
	const TUint KCOLInterface = 100; 
	if(iInterface && aOptionLevel==KCOLInterface)
		return iInterface->Control(aOptionLevel, aOptionName, aOption);

	if(iAgent && aOptionLevel==KCOLAgent)
		return iAgent->Control(aOptionLevel, aOptionName, aOption);

	return KErrNotSupported;
	}
TInt CNifAgentRef::Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, const RMessagePtr2* aMessage)
/**
Control either the interface or the agent
*/
	{
	const TUint KCOLInterface = 100;
	if(iInterface && aOptionLevel==KCOLInterface)
		return iInterface->Control(aOptionLevel, aOptionName, aOption);

	if(iAgent && aOptionLevel==KCOLAgent)
		{	
		if(!aMessage)
			{
			return KErrArgument;
			}
				
		RThread thread;
			
		TInt ret = aMessage->Client(thread);
		if(ret != KErrNone)
			return ret;

		RProcess process;
		ret = thread.Process(process);
		thread.Close();
			
		if(ret != KErrNone)
			return ret;
		
		ret = iAgent->Control(aOptionLevel, aOptionName, aOption, process);
		
		process.Close();
			
		return ret;	
		}
	
#ifdef _DEBUG
	// This code is here to support tests T_NifmanConnectionControl_0024 
	// through 0029. KCOLLinkLayerTestLevel allows control requests to be 
	// passed-on to the link layer (specifically, PPP). Currently, this is 
	// only supported in debug builds and for testing purposes only.
	if (aOptionLevel == KCOLLinkLayerTestLevel)
		{
		TDblQueIter<CBinderRef> iter(iBinders);
		CBinderRef* b;
		TInt ret = KErrNone;
		while ((b=iter++) != NULL && ret == KErrNone)
			{
			if (b->iLinkLayer != KErrNone)
				ret = b->iLinkLayer->Control(KSOLInterface, aOptionName, aOption);
			}
		return ret;
		}
#endif

  	return KErrNotSupported;
	}
void CNifAgentRef::Stop(TInt aError)
/**
Stop if required
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tStop() iState=%d"), this, iState); )

	switch(iState)
		{
	case EIdle:
	case EDisconnecting:
	case EStopping:
		iReconnectAfterDisconnect=EFalse;
		break;

	case EConnecting:
		BindFailAll(aError);
		iAgent->CancelConnect();
		iState=EDisconnecting;
		iAgent->Disconnect(aError);
		break;

	case EStarted:
	case ELinkUp:
		{
		// Stop the link
		TimerCancel();
		iOrigReason=aError;
		iState=EStopping;
		iInterface->Stop(aError,MNifIfNotify::EDisconnect);
		}
		break;

	case EReconnecting:
		iAgent->CancelReconnect();
		iOrigReason=aError;
		iState=EStopping;
		iInterface->Stop(aError,MNifIfNotify::EDisconnect);
		break;

	default: // Fall through. This prevents compiler warning on THUMB.
		;
		}
	}

void CNifAgentRef::Stop(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aError)
/**
Stop a subconnection

@note This cannot be used to stop an entire interface - CNifIfLink::Stop() should be used for this
@param aSubConnectionUniqueId The id of the subconnection to be stopped
@param aError The reason for stopping the interface - this should be a defined error code
@return KErrNone if successful, otherwise one of the system-wide error codes
*/
	{
	iNifExtndMngmtInterface->Stop(aSubConnectionUniqueId, aError);
	}

void CNifAgentRef::LinkLayerUp()
/**
Link layer is now fully functional
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tLinkLayerUp() iState=%d"), this, iState); )

	switch(iState)
		{
	case EStarted:
	case ELinkUp:
		iState=ELinkUp;
		SetTimerMode(ETimerMedium);

		if(!(iAgentExtndMngmtInterface && iNifExtndMngmtInterface))
			{
			// switch to compatibility layer
			iNifExtndMngmntCompatibilityLayer = new CNifExtendedManagementCompatibilityLayer(this);

			if(iNifExtndMngmntCompatibilityLayer)
					{
					// Set start time in compatibility layer
					TTime now;
					now.UniversalTime();
					iNifExtndMngmntCompatibilityLayer->SetTimeStarted(now);

					// Point interfaces at compatibility layer
					iNifExtndMngmtInterface = iNifExtndMngmntCompatibilityLayer;
					iAgentExtndMngmtInterface = iNifExtndMngmntCompatibilityLayer;
					}
			else
				{
				LOG(NifmanLog::Printf(_L("AgentRef %x:\tERROR - Could not create extended management interface"), this));
				}
			}
      if ( !ipNifConfigurationControl )
         {
         //unfortunatelly due to lack of consistency what is leave and what not
         //we have to go for TRAPD
         TRAPD( res, ipNifConfigurationControl = CNifConfigurationControl::NewL( *this );
               ipNifConfigurationControl->ConfigureNetworkL() );
         if ( res != KErrNone )
           {//report the error
            Progress( KLinkLayerOpen, res );
            }
         }

		break;
	default:
#ifdef _DEBUG
		Panic(ENifManPanic_BadLinkUpState);
#else
		;
#endif
		}

	}

void CNifAgentRef::LinkLayerDown(TInt aReason, TAction aAction)
/**
Link Layer has gone down - try to reconnect if required
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tLinkLayerDown() iState=%d"), this, iState); )

	//Notify the agent about the disconnection
	iAgent->Notification(ENifToAgentEventTypeLinkLayerDown,&aReason);

	TimerCancel();
	if (iAuthenticating)
		{
		iAgent->CancelAuthenticate();
		AuthenticateComplete(KErrCancel);
		}

	// Get the last error that occurred from the agent.  It may be more meaningful than the NIF one!
	TInt lastErr=KErrNone;
	iAgent->GetLastError(lastErr);
	if (lastErr==KErrNone)
		lastErr=aReason;

	delete iNifExtndMngmntCompatibilityLayer;
	iNifExtndMngmntCompatibilityLayer = NULL;

   	delete ipNifConfigurationControl;
   	ipNifConfigurationControl = NULL;
	iAgentExtndMngmtInterface = NULL;
	iNifExtndMngmtInterface = NULL;

	switch(iState)
		{
	case EReconnecting:
		if ( aAction == EDisconnect )
			{
			LinkDownAll(lastErr);
			iAgent->CancelReconnect();
			iState=EDisconnecting;
			iAgent->Disconnect(iOrigReason);
			}
		break;

	case EStarted:
	case ELinkUp:
		if ( aAction == EReconnect )
			{
			TUint32 val;
			if(iRouteCount > 0 || iUserCount > 0
				|| (ReadInt(NIF_RECONNECTIFNOCLIENT, val)==KErrNone && val) )
				{
				iOrigReason = lastErr;
				iState = EReconnecting;

				iAgent->Reconnect();
				}
			else
				aAction = EDisconnect;
			}

		if ( aAction == EDisconnect )
			{
			LinkDownAll(lastErr);
			iState = EDisconnecting;
			iAgent->Disconnect(lastErr);
			}
		else if (aAction == ECallBack)
			{
			iState = EConnecting;
			iConnectType=EAgentStartCallBack;
			iAgent->Connect(iConnectType);
			}
		break;

	case EStopping:
		LinkDownAll(iOrigReason);
		iState = EDisconnecting;
		iAgent->Disconnect(iOrigReason);
		break;

	default:
#ifdef _DEBUG
		;//Panic(ENifManPanic_BadLinkDownState);
#else
		;
#endif
		}
	}


TInt CNifAgentRef::Authenticate(TDes& aUsername, TDes& aPassword)
	{

	if (iAuthenticating)
		return KErrAlreadyExists;

	iAuthenticating=ETrue;
	iAgent->Authenticate(aUsername, aPassword);
	return KErrNone;
	}

void CNifAgentRef::CancelAuthenticate()
	{

	iAuthenticating=EFalse;
	iAgent->CancelAuthenticate();
	}

void CNifAgentRef::AuthenticateComplete(TInt aStatus)
	{

	__ASSERT_DEBUG(iAuthenticating, Panic(ENifManPanic_NotAuthenticate));

	if(iAuthenticating)
		{
		iAuthenticating=EFalse;
		iInterface->AuthenticateComplete(aStatus);
		}
	}

TInt CNifAgentRef::GetExcessData(TDes8& aBuffer)
	{

	return iAgent->GetExcessData(aBuffer);
	}

TInt CNifAgentRef::Notification(TNifToAgentEventType aEvent, void* aInfo)
	{

	return iAgent->Notification(aEvent, aInfo);
	}

TInt CNifAgentRef::DoReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage )
	{
	return iAgent->ReadInt(aField,aValue,aMessage);
	}
/**
Updates a cached (Nifman internal) value of Nifman idle timeout, 
or writes a value to the specified field in CommDB (Currently not supported).

If the field specifies one of 3 Nifman's idle timeouts:
LastSessionClosedTimeout, LastSocketClosedTimeout or LastSocketActivityTimeout
updates the cached (Nifman's) value of that timeout.
Otherwise, writes the value to CommDb.(Currently not supported)
Note: this function was intended to write the aValue to CommDB. 
For Nifman idle timeouts, this is not done.

@param aField the field to update
@param aValue the value to write to the field
@return KErrNone if the field specifies one of Nifman's idle timeouts.
		KErrNotSupported or other system-wide error code otherwise.
@post If aField specifies Nifman idle timeout, the timeout is updated.
*/
TInt CNifAgentRef::DoWriteInt(const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage )
	{
	return iAgent->WriteInt(aField, aValue, aMessage);
	}

TInt CNifAgentRef::DoReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage )
	{
	return iAgent->ReadDes(aField, aValue,aMessage);
	}

TInt CNifAgentRef::DoReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage )
	{
	return iAgent->ReadDes(aField, aValue,aMessage);
	}


TInt CNifAgentRef::DoWriteDes(const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage )
	{

	return iAgent->WriteDes(aField, aValue,aMessage);
	}

TInt CNifAgentRef::DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage)
	{
	return iAgent->WriteDes(aField, aValue,aMessage);
	}


TInt CNifAgentRef::DoReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage )
	{

	return iAgent->ReadBool(aField, aValue,aMessage);
	}

TInt CNifAgentRef::DoWriteBool(const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage )
	{

	return iAgent->WriteBool(aField, aValue, aMessage);
	}

//

void CNifAgentRef::ConnectComplete(TInt aStatus)
/**
Up call from agent
*/
	{

	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tConnectComplete(%d) iState=%d"), this, aStatus, iState); )

	__ASSERT_DEBUG(iState==EConnecting, Panic(ENifManPanic_BadState));

	if (aStatus==KErrNone)
		{
		__ASSERT_DEBUG(iInterface, Panic(ENifManPanic_InterfaceDoesNotExist));
		iState = EStarted;
		aStatus=iInterface->Start();
		}

	if(aStatus!=KErrNone)
		{

		BindFailAll(aStatus);

		iState = EDisconnecting;
		iAgent->Disconnect(aStatus);
	    }
	}


void CNifAgentRef::ReconnectComplete(TInt aStatus)
/**
Valid complete must be in state
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tReconnectComplete(%d) iState=%d"), this, aStatus, iState); )
	
	__ASSERT_DEBUG(iState==EReconnecting, Panic(ENifManPanic_BadState));
	if(aStatus==KErrNone)
		{
		iState = EConnecting;
		iConnectType=EAgentReconnect;                                 
		iAgent->Connect(iConnectType);
		}
	else
		{
		LinkDownAll(iOrigReason);
		iState=EDisconnecting;
		iAgent->Disconnect(iOrigReason);
		}
	}

void CNifAgentRef::ServiceStarted()
	{

	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tServiceStarted() iState=%d"), this, iState); )
	
	__ASSERT_DEBUG(iState==EConnecting, Panic(ENifManPanic_BadState));
	__ASSERT_DEBUG(!iInterface, Panic(ENifManPanic_InterfaceAlreadyExists));

	// Read Interface name and load the required interface
	TName nif;
	TInt res=iAgent->ReadDes(TPtrC(IF_NAME), nif);
	if(res==KErrNone)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tLoading interface '%S'"), this, &nif); )
		TRAP(res, iInterface=static_cast<CNifIfLink*>(Nif::CreateInterfaceL(nif, this));)

		LOG(
			if(res!=KErrNone)
				{
				NifmanLog::Printf(_L("AgentRef %x:\tCould not load interface '%S' - error %d"), this, &nif, res);
				}
			)
		}

	if(res==KErrNone)
		{
		iInterface->Open();
		res=iAgent->ReadDes(TPtrC(SERVICE_IF_NETWORKS), nif);



		// Get the pointers to the Extended Management Interfaces for the nif and agent
		TPckg<MNifIfExtendedManagementInterface*> nifIfEMIPtrPckg(iNifExtndMngmtInterface);
		iInterface->Control(KCOLInterface, KCOGetNifEMIPtr, nifIfEMIPtrPckg);
		// Once agents start and manage subconnection, set this to iAgent rather than iInterface
		TPckg<MNifAgentExtendedManagementInterface*> nifAgentEMIPtrPckg(iAgentExtndMngmtInterface);
		iInterface->Control(KCOLAgent, KCOGetAgentEMIPtr, nifAgentEMIPtrPckg);
		// if these calls do not succeed, the nif does not support extended management functionality so a compatibility layer will be loaded when the nif calls LinkLayerUp()
		}

	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b;

	if(res==KErrNone)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tBinding interface to protocol(s): '%S'"), this, &nif); )
		TLex lex(nif);
		while(res==KErrNone)
			{

			TPtrC token;
			lex.Mark();
			TChar ch;
			ch=lex.Peek();

			while(!lex.Eos() && (ch=lex.Peek())!=TChar(','))
				lex.Inc();

			token.Set(lex.MarkedToken());
			if(!token.Length())
				break;

			if(ch==TChar(','))
				lex.Inc();

			CProtocolBase* p=0;
			TRAP(res, p=SocketServExt::FindAndLoadProtocolL(token));
			if(res!=KErrNone)
				{
				LOG( NifmanLog::Printf(_L("AgentRef %x:\tError(%d) loading protocol '%S'"), this, res, &token); )
				res=KErrNone;
				continue;
				}

			TNifIfUser ifuser;
			p->Open();
			res=p->GetOption(KNifOptLevel, KNifOptGetNifIfUser, ifuser, 0);

			if(res==KErrNone)
				{
				iter.SetToFirst();				
				while((b=iter++)!=0)
					{
					if(b->iNetworkLayer==ifuser())
						break;
					}

				if(b==0)
					{
					res=AddNetworkLayer(ifuser(), 0);
					LOG(
						if(res!=KErrNone)
							NifmanLog::Printf(_L("AgentRef %x:\tError(%d) when adding network layer for protocol '%S'"), this, res, &token);
						)
					}
				else
					{
					TRAP(res, b->BindL(iInterface));
					if(res!=KErrNone)
						{
						LOG( NifmanLog::Printf(_L("AgentRef %x:\tError(%d) binding protocol '%S' to interface"), this, res, &token); )
						b->UnBind(res);
						}
					}
				}
			iTimerDisableCount++;
			p->Close();
			iTimerDisableCount--;
			}                                               
		}

	if(res==KErrNone)
		{
		iter.SetToFirst();
		while((b=iter++)!=0)
			{
			if(!b->iLinkLayer)
				b->UnBind(KErrNotFound);
			}
		if(iBinders.IsEmpty())
			{
			res=KErrNotFound;
			LOG( NifmanLog::Printf(_L("AgentRef %x:\tError(%d) - could not bind any protocol to interface"), this, res); )
			}
		}

	if(res!=KErrNone)
		{
		LOG( NifmanLog::Printf(_L("AgentRef %x:\tError(%d) binding interface to protocols - starting disconnect"), this, res); )
		BindFailAll(res);
		iAgent->CancelConnect();
		iState=EDisconnecting;
		iAgent->Disconnect(res);
		ServiceClosed();
		}
	}

void CNifAgentRef::ServiceClosed()
/**
Service closed - note closing the interface will cause mean it is deleted
If it is the last reference on it
*/
	{
	
	if(iInterface)
		iInterface->Close();
	iInterface=0;
	}

void CNifAgentRef::DisconnectComplete()
/**
If the reference count is zero delete this
*/
	{

	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tDisconnectComplete() iState=%d"), this, iState); )

	__ASSERT_DEBUG(iState==EDisconnecting, Panic(ENifManPanic_BadState));

	ServiceClosed();
	iState = EIdle;

	if(iReconnectAfterDisconnect)
		{
		iReconnectAfterDisconnect=EFalse;
		Start(iConnectType); // This call will always initiate the connect process again                        
		}
	else 
		{
		iConnectType=EAgentNone;                      

		Progress(KConnectionUninitialised, KErrNone);

		// This Close() matches the call to Nif::CreateAgentL() in NetCon which initially sets the
		// CObject count to one.
		Close();
		}
	}

void CNifAgentRef::Progress(TInt aStage, TInt aError)
/**
Progress upcall for the Agent or NIF
Notify all attached IPC clients
*/
	{

	iProgress.iStage = aStage;
	iProgress.iError = aError;

	TUint numSessions = iSessions.Count();

	// Must handle linkLayer up progress from all binders
	if(KErrNone == aError && KLinkLayerOpen == aStage)
		{
		if(AreAllNetworkLayersOpen())
			{
			// Enable the timers
			SetTimerMode(ETimerMedium);
			}
		else
			{
			// Don't send link up until all network layers are open
			return;
			}
		}

	for (TUint i = 0; i < numSessions; ++i)
		{
		LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tSending progress (%d, %d) to session %x"), this, aStage, aError, iSessions[i]); )

		iSessions[i]->ProgressNotification(aStage, aError);
		}
	}

void CNifAgentRef::Progress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)
/**
Progress upcall relating to a subconnection from agent or nif

@param aSubConnectionUniqueId The subconnection to which this notification refers
@param aStage The progress stage that has been reached
@param aError Any errors that have occured
*/
	{
	TUint numSessions = iSessions.Count();

	for (TUint i = 0; i < numSessions; ++i)
		{
		LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tSending progress for subconnection %d (%d, %d) to session %x"), this, aSubConnectionUniqueId, aStage, aError, iSessions[i]); )

		iSessions[i]->ProgressNotification(aSubConnectionUniqueId, aStage, aError);
		}
	}


void CNifAgentRef::IfProgress(TInt aStage, TInt aError)
/**
Upcall from the NIF or the configuration daemon
Assert that the stage value is within the range assigned to NIFs
i.e. KMinNifProgress <= aStage <= KMaxNifProgress

*/
	{
	// (aStage >= KMinNifProgress) removed to allow nifs that don't support agents (this is bad design in the nif!)
	__ASSERT_DEBUG((aStage <= KMaxNifProgress), Panic(ENifManPanic_NifProgressOutOfRange));

	Progress(aStage, aError);
	}

void CNifAgentRef::IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)
/**
Upcall from the nif indicating that a new progress state has been reached on a subconnection

@pre The stage value is within the range assigned to nifs i.e. KMinNifProgress <= aStage <= KMaxNifProgress
@param aSubConnectionUniqueId The subconnection to which this notification refers
@param aStage The progress stage that has been reached
@param aError Any errors that have occured
*/
	{
	__ASSERT_DEBUG((aStage >= KMinNifProgress && aStage <= KMaxNifProgress), Panic(ENifManPanic_NifProgressOutOfRange));

	Progress(aSubConnectionUniqueId, aStage, aError);
	}

void CNifAgentRef::AgentProgress(TInt aStage, TInt aError)
/**
Upcall from the Agent
Assert that the stage value is within the range assigned to Agents
i.e. KMinAgtProgress <= aStage <= KMaxAgtProgress
*/
	{

	__ASSERT_DEBUG((aStage >= KMinAgtProgress && aStage <= KMaxAgtProgress), Panic(ENifManPanic_AgtProgressOutOfRange));

	Progress(aStage, aError);
	}

void CNifAgentRef::AgentProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)
/**
Upcall from the agent indicating that a new progress state has been reached on a subconnection

@pre The stage value is within the range assigned to agents i.e. KMinAgtProgress <= aStage <= KMaxAgtProgress
@param aSubConnectionUniqueId The subconnection to which this notification refers
@param aStage The progress stage that has been reached
@param aError Any errors that have occured
*/
	{
	__ASSERT_DEBUG((aStage >= KMinAgtProgress && aStage <= KMaxAgtProgress), Panic(ENifManPanic_AgtProgressOutOfRange));

	Progress(aSubConnectionUniqueId, aStage, aError);
	}

TInt CNifAgentRef::Notification(TAgentToNifEventType aEvent, TAny* aInfo)
	{

	if (aEvent==EAgentToNifEventTypeModifyInitialTimer)
		{
		TDblQueIter<CBinderRef> iter(iBinders);
		CBinderRef* b;
		TInt ret=KErrNone;
		while ((b=iter++)!=NULL && ret==KErrNone)
			{
			if (b->iLinkLayer!=KErrNone)
				ret=b->iLinkLayer->Notification(aEvent,aInfo);
			}
		return ret;
		}
	else if(aEvent==EAgentToNifEventTypeDisableConnection)
		{
		// Request from agent to disconnect
		Stop(KErrCancel);
		return KErrNone;
		}
	else 
		{
		__ASSERT_DEBUG(iInterface!=NULL, User::Invariant());
		return iInterface->Notification(aEvent,aInfo);
		}
	}

TInt CNifAgentRef::IncomingConnectionReceived()
	{

	if (iState!=EIdle)
		return KErrInUse;
		
	iState=EConnecting;
	iConnectType = EAgentStartDialIn;
	return KErrNone;
	}

void CNifAgentRef::DisableTimers()
/**
Disable timers
*/
	{
	
	if(iTimerDisableCount==0)
		TimerCancel();
	iTimerDisableCount++;
	}

void CNifAgentRef::EnableTimers()
/**
Enable timers
*/
	{
	
	--iTimerDisableCount;
	if(iTimerDisableCount==0)
		ResetTimer();
	}

TBool CNifAgentRef::MatchesIdOrInterface(TAny* aId, CNifIfBase* aIf)
/**
Does this agent have any reference to the interface or id
*/
	{

	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b;
	while((b=iter++)!=0)
		{
		if(aId && b->iId==aId)
			return ETrue;
		if(aIf && b->iLinkLayer==aIf)
			return ETrue;
		}
	return EFalse;
	}

EXPORT_C CNifAgentBase::CNifAgentBase()
/**
Constructor
*/
	{
	}

EXPORT_C TInt CNifAgentBase::Control(TUint, TUint, TDes8&)
/**
This version of the Control() function is now deprecated.
Use the version of CNifAgentBase::Control() that takes four parameters
*/
	{
	return KErrNotSupported;
	}

/**
   Control() with capability checking
*/
EXPORT_C TInt CNifAgentBase::Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, const RProcess& aProcess)
	{
	TBool result( CheckControlPolicy(aOptionLevel, aOptionName, aProcess) );
	
	//if we got Permission (ETrue i.e. TRUE i.e. 1) then return call to Control(), else return KErrPermissionDenied
	return ( result? Control(aOptionLevel, aOptionName, aOption) : KErrPermissionDenied );
	}

/**
   Read an integer
   @param aField   The field name
   @param aValue   Returned value
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadInt( aField, aValue, aMessage );
	}

/**
   Read an integer
   @param aField   The field name
   @param aValue   Returned value
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadInt(const TDesC& aField, TUint32& aValue )
	{
	return ReadInt( aField, aValue, NULL );
	}

/**
   Write an integer
   @param aField   The field name
   @param aValue   The value
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteInt( aField, aValue, aMessage );
	}

/**
   Write an integer
   @param aField   The field name
   @param aValue   The value
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteInt(const TDesC& aField, TUint32 aValue )
	{
	return WriteInt( aField, aValue, NULL );
	}

/**
   Read a descriptor
   @param aField   The field name
   @param aValue   Returned value
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadDes( aField, aValue, aMessage );
	}

/**
   Read a descriptor
   @param aField   The field name
   @param aValue   Returned value
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes8& aValue )
	{
	return ReadDes( aField, aValue, NULL );
	}

/**
   Write a descriptor
   @param aField   The field name
   @param aValue   value to write
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteDes( aField, aValue, aMessage );
	}

/**
   Write a descriptor
   @param aField   The field name
   @param aValue   value to write
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC8& aValue )
	{
	return WriteDes( aField, aValue, NULL );
	}

/**
   Read a descriptor
   @param aField   The field name
   @param aValue   Returned value
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadDes( aField, aValue, aMessage );
	}

/**
   Read a descriptor
   @param aField   The field name
   @param aValue   Returned value
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes16& aValue )
	{
	return ReadDes( aField, aValue, NULL );
	}

/**
   Write a descriptor
   @param aField   The field name
   @param aValue   value to write
   @param aMessage For capability checking
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteDes( aField, aValue, aMessage );
	}

/**
   Write a descriptor
   @param aField   The field name
   @param aValue   value to write
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC16& aValue )
	{
	return WriteDes( aField, aValue, NULL );
	}

/**
   Read a boolean value
   @param aField Field name
   @param aValue Returned value
   @param aMessage For capability checking
   @return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadBool( aField, aValue, aMessage );
	}

/**
   Read a boolean value
   @param aField Field name
   @param aValue Returned value
   @return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadBool(const TDesC& aField, TBool& aValue )
	{
	return ReadBool( aField, aValue, NULL );
	}

/**
   Write a boolean value
   @param aField Field name
   @param aValue Value to write
   @param aMessage for capability checking
   @return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteBool( aField, aValue, aMessage );
	}

/**
   Write a boolean value
   @param aField Field name
   @param aValue Value to write
   @return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteBool(const TDesC& aField, TBool aValue )
	{
	return WriteBool( aField, aValue, NULL );
	}

/**
   Read a long descriptor value
   @param aField Field name
   @param aMessage For capability checking
   @return HBufC containing value
*/
EXPORT_C HBufC* CNifAgentBase::ReadLongDesLC(const TDesC& aField,const RMessagePtr2* aMessage)
	{
	return DoReadLongDesLC( aField, aMessage );
	}

/**
   Read a long descriptor value
   @param aField Field name
   @return HBufC containing value
*/
EXPORT_C HBufC* CNifAgentBase::ReadLongDesLC(const TDesC& aField )
	{
	return ReadLongDesLC( aField, NULL );
	}

/**
   Ensure that a client has the required capabilites to read a field
   @param aField  Field Name
   @param aMessage Contains client capabilites to validate
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	if( aMessage )
		{
		return DoCheckReadCapability( aField, aMessage );
		}
	else
		{
		return KErrNone;
		}
	}

/**
   Ensure that a client has the required capabilites to write a field
   @param aField  Field Name
   @param aMessage Contains client capabilites to validate
   @return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	if( aMessage )
		{
		return DoCheckWriteCapability( aField, aMessage );
		}
	else
		{
		return KErrNone;
		}
	}


/**
   Ensure that a client has the required capabilites to read a field
   @param aField  Field Name
   @param aMessage Contains client capabilites to validate
   @return One of the system-wide error codes
*/
TInt CNifAgentBase::DoCheckReadCapability( const TDesC& /*aField*/, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNone;
	}

/**
   Ensure that a client has the required capabilites to write a field
   @param aField  Field Name
   @param aMessage Contains client capabilites to validate
   @return One of the system-wide error codes
*/
TInt CNifAgentBase::DoCheckWriteCapability( const TDesC& /*aField*/, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNone;
	}
	
/**
   Check that a client has the required capabilties
   @param aOptionLevel 
   @param aOptionName 
   @param aOption
   @param aMessage 
   @returns KErrNone, if the client has the required capabilites, otherwise one of the standard Symbian OS error codes
 */
TBool CNifAgentBase::CheckControlPolicy(TUint aLevel, TUint /*aOption*/, const RProcess& /*aProcess*/)
	{
	TBool result;
	
	switch (aLevel)
		{
	case KCOLAgent:
		result = ETrue;
		break;
		
	default:
		result = EFalse;
		break;
		}
	
	return result;
	}


void CNifAgentRef::AddObserverL(MNifSessionNotify* aObserver)
/**
Add an observer of Progress notifications

@param aObserver Observer class
*/
	{

	TInt err = iSessions.InsertInAddressOrder(aObserver);
	if(err!=KErrNone && err!=KErrAlreadyExists)
		{
		User::Leave(err);
		}
	else if (err==KErrAlreadyExists)
		{
		LOG_DETAILED(
					NifmanLog::Printf(_L("AgentRef %x:\tObserver %x already added"), this, aObserver);
					NifmanLog::Printf(_L("AgentRef %x:\tCurrent number of observers is %d"), this, iSessions.Count());
					)
		return;
		}

	// Increment the CObject count to prevent instance disappearing whilst
	// it has observers.
	Open();

	LOG( NifmanLog::Printf(_L("AgentRef %x:\tAdded observer %x"), this, aObserver); )
	
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tCurrent number of observers is %d"), this, iSessions.Count()); )
	}

void CNifAgentRef::RemoveObserver(MNifSessionNotify* aObserver)
/**
Remove an observer of Progress notifications
@note that the CNifAgentRef can be destroyed as a result of this method.
@param aObserver Observer class to remove
 */
	{
	TInt index = iSessions.Find(aObserver);

	if(index<0)
		{
		LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tCould not find observer %x"), this, aObserver); )
		return;
		}

	iSessions.Remove(index);

	LOG( NifmanLog::Printf(_L("AgentRef %x:\tRemoved observer %x"), this, aObserver); )

	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tCurrent number of observers is %d"), this, iSessions.Count()); )

	// Decrement CObject count - which can delete "this" so do not
	// access "this" after this call.
	Close();
	}

void CNifAgentRef::OpenRoute()
	{
	iRouteCount++;
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tOpenRoute() count %d"), this, iRouteCount); )
	}

TInt CNifAgentRef::PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer)
/**
Indication from the network layer that it has sent or received a packet, and of the size of that packet

@param aDirection Whether the packet was sent or received
@param aBytes The number of bytes in the packet
@param aResetTimer Whether this packet should reset the idle timers
@return KErrNone if successful, otherwise one of the system-wide error codes
*/
	{
	if (aResetTimer)
		iPeriodActivity = ETrue;
	
	if(iNifExtndMngmntCompatibilityLayer)
		{
		switch(aDirection)
			{
			case EIncoming:
				return iNifExtndMngmntCompatibilityLayer->DataReceived(aBytes);
				// break;
			case EOutgoing:
				return iNifExtndMngmntCompatibilityLayer->DataSent(aBytes);
				// break;
			default:
				return KErrArgument;
			}
		}
	return KErrNone;
	}

void CNifAgentRef::CloseRoute()
	{
	LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tCloseRoute() count %d"), this, iRouteCount-1); )
	ASSERT(iRouteCount > 0);
	if (--iRouteCount == 0 && !iTimer->IsActive())
		{
		// if the number of calls to CloseRoute() matches those to OpenRoute(), then ensure that
		// the idle timer is running.
		TTimerType newMode = DecideTimerMode();
		if (newMode != ETimerUnknown)
			SetTimerMode(newMode);
		else
			ResetTimer();
		}
	}

//
TInt CNifAgentRef::GetInterfaceName(TUint aIndex, TDes& aInterfaceName)
/**
Retrieve underlying Nif interface name(s).
Used to retrieve the interface names corresponding to each CBinderRef
associated with the current interface.

@params aIndex index of CBinderRef whose interface name is to be retrieved
@params aInterfaceName receives the interface name.
@return KErrNone if name was retrieved, KErrNotFound if there is no CBinderRef corresponding to aIndex.
*/
	{
	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b = NULL;

	while(aIndex-- > 0 && (b=iter++)!=0)
		;

	if (b)
		{
		TNifIfInfo info;
		b->iLinkLayer->Info(info);
		aInterfaceName.Copy(info.iName);
		return KErrNone;
		}
	else
		return KErrNotFound;
	}

TInt CNifAgentRef::GetInterfaceType(TConnectionType& aConnectionType)
/**
Find out the type of this connection (eg. CSD, GPRS, CDMA, Ethernet, some more specific subtype)

@param aConnectionType On return, contains the type of the connection
@return KErrNone, or one of the system-wide error codes
*/
	{
	if(iAgentExtndMngmtInterface)
		{
		return iAgentExtndMngmtInterface->GetInterfaceType(aConnectionType);
		}
	else
		{
		return KErrNotReady;
		}
	}

void CNifAgentRef::NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume)
/**
Called by the nif when the amount of data sent exceeds the granularity set by SetDataSentNotificationGranularity()

@param aSubConnectionUniqueId The id of the subconnection that this notification refers to
@param aUplinkVolume The number of bytes sent by the subconnection so far
*/
	{
	TUint numSessions = iSessions.Count();

	for (TUint i = 0; i < numSessions; ++i)
		{
		LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tNotifying data sent for subconnection %d (%d) to session %x"), this, aSubConnectionUniqueId, aUplinkVolume, iSessions[i]); )

		iSessions[i]->NotifyDataSent(aSubConnectionUniqueId, aUplinkVolume);
		}
	}

void CNifAgentRef::NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume)
/**
 * Called by the nif when the amount of data received exceeds the granularity set by SetDataReceivedNotificationGranularity()
 * @param aSubConnectionUniqueId The id of the subconnection that this notification refers to
 * @param aDownlinkVolume The number of bytes received by the subconnection so far
 */
	{
	TUint numSessions = iSessions.Count();

	for (TUint i = 0; i < numSessions; ++i)
		{
		LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tNotifying data received for subconnection %d (%d) to session %x"), this, aSubConnectionUniqueId, aDownlinkVolume, iSessions[i]); )

		iSessions[i]->NotifyDataReceived(aSubConnectionUniqueId, aDownlinkVolume);
		}
	}

void CNifAgentRef::NetworkAdaptorEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* /*aSource*/)
/**
Handle events from network adaptors

@note Currently this just repackages subconnection events and passes them to CNifSession, 
however in the future it may need to be expanded to act as an event distribution system that 
supports dynamic client registration and at that point need to be in its own class
@note Now updated to also support Stop() commands being issued from the agent
@param aEventType The type of the event
@param aEvent The event that occured
@param aEventData Any arguments or additional data to do with the event; the interpretation of 
this depends on the event
@param aSource The source of the event - this will be used when one receiver is receiving 
events from many producers
*/
	{
	LOG(NifmanLog::Printf(_L("Nifman: CNifAgentRef(%x): Network adaptor event received (event type: %d, event: %d)"), this, aEventType, aEvent));
	// repackage subconnection events and send to CNifSession
	const TSubConnectionEvent* subConnectionEvent = NULL;

	if(aEventType==ESubConnectionEvent)
		{
		// if client has requested all subconnection events send it up
		if(iSendAllSubConnectionEvents)
			{
			//@todo For second release, send other events
			}
		if(aEvent==ESubConnectionOpened)
			{
			ASSERT(aEventData.Length()==sizeof(TSubConnectionOpenedEvent));
			subConnectionEvent = reinterpret_cast<const TSubConnectionOpenedEvent*>(aEventData.Ptr());	//lint !e826 // passed as TDesC8
			LOG(NifmanLog::Printf(_L("Nifman: CNifAgentRef(%x): Network adaptor event - subconnection opened (id: %d)"), this, subConnectionEvent->iSubConnectionUniqueId));
			}
		else
			{
			if(aEvent==ESubConnectionClosed	)
				{
				ASSERT(aEventData.Length()==sizeof(TSubConnectionClosedEvent));
				subConnectionEvent = reinterpret_cast<const TSubConnectionClosedEvent*>(aEventData.Ptr());	//lint !e826 // passed as TDesC8
				LOG(NifmanLog::Printf(_L("Nifman: CNifAgentRef(%x): Network adaptor event - subconnection closed (id: %d, data sent: %d, data received: %d)"), this, subConnectionEvent->iSubConnectionUniqueId, reinterpret_cast<const TSubConnectionClosedEvent*>(subConnectionEvent)->iTotalUplinkDataVolume, reinterpret_cast<const TSubConnectionClosedEvent*>(subConnectionEvent)->iTotalDownlinkDataVolume));
				}
			}
		// Pass event to all attached sessions
		if(subConnectionEvent)
			{
			TUint numSessions = iSessions.Count();

			for (TUint i = 0; i < numSessions; ++i)
				{
				LOG_DETAILED( NifmanLog::Printf(_L("AgentRef %x:\tNotifying event %d on subconnection %d to session %x"), this, aEvent, subConnectionEvent->iSubConnectionUniqueId, iSessions[i]); )
				iSessions[i]->SubConnectionEvent(*subConnectionEvent);
				}
			}
		}

	// Just ignore other events for now (there shouldn't be any others anyway)
	}

void CNifAgentRef::NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource)
/** 
Notification from the nif that an event has occured. Nifs can send any events they wish, but typically this might refer to context creation or QoS changes (in monolithic nifs that don't have agents), or control settings

@param aEventType The type of the event
@param aEvent The event that occured
@param aEventData Any arguments or additional data to do with the event; the interpretation of this depends on the event
@param aSource The source of the event - this will be used when one receiver is receiving events from many producers
*/
	{
	NetworkAdaptorEvent(aEventType, aEvent, aEventData, aSource);
	}

void CNifAgentRef::AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource)
/** 
Notification from the agent that an event has occured. Agents can send any events they wish, but typically this might refer to context creation, QoS changes, or control settings

@param aEventType The type of the event
@param aEvent The event that occured
@param aEventData Any arguments or additional data to do with the event; the interpretation of this depends on the event
@param aSource The source of the event - this will be used when one receiver is receiving events from many producers
*/
	{
	NetworkAdaptorEvent(aEventType, aEvent, aEventData, aSource);
	}

TInt CNifAgentRef::EnumerateSubConnections(TUint& aCount)
/**
Discover how many subconnections exist on an interface

@note All interfaces must support at least one subconnection
@param aCount On return, contains the number of subconnections that currently exist on this interface
@return KErrNone, or one of the system-wide error codes
*/
	{
	if(iAgentExtndMngmtInterface)
		{
		return iAgentExtndMngmtInterface->EnumerateSubConnections(aCount);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::GetSubConnectionInfo(TUint aIndex, TDes8& aSubConnectionInfo)
/**
Get information about a subconnection with no prior knowledge of the unique id of any subconnetion

@note aSubConnectionInfo should contain a class derived from TSubConnectionInfo; the class should be suitable for type of interface being queried, however it is up to the interface to check this.
@note The type of the class can be determined by checking the iConnectionType member
@note Unique IDs for subconnections are on a per-nif basis.  It is the responsibility of the nif or agent to generate these IDs and to ensure they are unique across the connection
@param aIndex A number between one and the total number of subconnections returned by EnumerateSubConnections()
@param aSubConnectionInfo Should contain a class derived from TSubConnectionInfo and suitable for the type of the interface being queried; on return, contains the class with all its members filled in
@return KErrNone, or one of the system-wide error codes; in particular KErrArgument if the class type is unsuitable
*/
	{
	if(iAgentExtndMngmtInterface)
		{
		return iAgentExtndMngmtInterface->GetSubConnectionInfo(aIndex, aSubConnectionInfo);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::GetSubConnectionInfo(TDes8& aSubConnectionInfo)
/**
Get information about the subconnection with the supplied unique ID

@note aSubConnectionInfo should contain a class derived from TSubConnectionInfo; the class should be suitable for type of interface being queried, however it is up to the interface to check this.
@note The type of the class can be determined by checking the iConnectionType member
@param aSubConnectionInfo Should contain a class derived from TSubConnectionInfo and suitable for the type of the interface being queried, and with the iSubConnectionUniqueId member set to the appropriate unique ID
@return KErrNone, or one of the system-wide error codes; in particular KErrArgument if the class type is unsuitable
*/
	{
	if(iAgentExtndMngmtInterface)
		{
		return iAgentExtndMngmtInterface->GetSubConnectionInfo(aSubConnectionInfo);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::AllSubConnectionNotificationEnable()
/**
*/
	{
	// turn on all subconnection event notification
	iSendAllSubConnectionEvents = ETrue;
	return KErrNone;
	}

TInt CNifAgentRef::CancelAllSubConnectionNotification()
/**
*/
	{
	// turn off all subconnection event notification
	iSendAllSubConnectionEvents = EFalse;
	return KErrNone;
	}

TInt CNifAgentRef::DataTransferred(TSubConnectionUniqueId aSubConnectionUniqueId, TUint& aUplinkVolume, TUint& aDownlinkVolume)
/**
Gets the data volumes transferred in either direction by a (sub)connection when the call is made

@param aSubConnectionUniqueId An identifier for a subconnection on this interface; an ID of zero means the query refers to the entire interface
@param aSentBytes The number of bytes sent over this (sub)connection
@param aReceivedBytes The number of bytes received over this (sub)connection
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Add layer to arguments using TConnectionLayer
*/
	{
	if(iNifExtndMngmtInterface)
		{
		return iNifExtndMngmtInterface->GetDataTransferred(aSubConnectionUniqueId, aUplinkVolume, aDownlinkVolume);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::SetDataSentNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkGranularity)
/**
Requests that the interface notifies Nifman each time it has sent aGranularity additional bytes

@note The notification to Nifman is made through the MNifIfNotify interface
@param aSubConnectionUniqueId An identifier for a subconnection on this interface; an ID of zero means the query refers to the entire interface
@param aGranularity The number of additional bytes that should be sent before notifying Nifman
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Add layer to arguments using TConnectionLayer
*/
	{
	if(iNifExtndMngmtInterface)
		{
		return iNifExtndMngmtInterface->SetDataSentNotificationGranularity(aSubConnectionUniqueId, aUplinkGranularity);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::DataSentNotificationCancel(TSubConnectionUniqueId aSubConnectionUniqueId)
/**
Cancels a request for notification made using SetDataSentNotificationGranularity()

@pre An outstanding request for notification about data volumes sent on this (sub)connection
@param aSubConnectionUniqueId The subconnection ID used to make the request
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Add layer to arguments using TConnectionLayer
*/
	{
	if(iNifExtndMngmtInterface)
		{
		return iNifExtndMngmtInterface->CancelDataSentNotification(aSubConnectionUniqueId);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::SetDataReceivedNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkGranularity)
/**
Requests that the interface notifies Nifman each time it has received aGranularity additional bytes

@note The notification to Nifman is made through the MNifIfNotify interface
@param aSubConnectionUniqueId An identifier for a subconnection on this interface; an ID of zero means the query refers to the entire interface
@param aGranularity The number of additional bytes that should be received before notifying Nifman
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Add layer to arguments using TConnectionLayer
*/
	{
	if(iNifExtndMngmtInterface)
		{
		return iNifExtndMngmtInterface->SetDataReceivedNotificationGranularity(aSubConnectionUniqueId, aDownlinkGranularity);
		}
	else
		{
		return KErrNotReady;
		}
	}

TInt CNifAgentRef::DataReceivedNotificationCancel(TSubConnectionUniqueId aSubConnectionUniqueId)
/**
Cancels a request for notification made using SetDataReceivedNotificationGranularity()

@pre An outstanding request for notification about data volumes received on this (sub)connection
@param aSubConnectionUniqueId The subconnection ID used to make the request
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Add layer to arguments using TConnectionLayer
*/
	{
	if(iNifExtndMngmtInterface)
		{
		return iNifExtndMngmtInterface->CancelDataReceivedNotification(aSubConnectionUniqueId);
		}
	else
		{
		return KErrNotReady;
		}
	}


//PREQ399_REMOVE
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
CNifIfBase* CNifAgentRef::GetNif(TInt aIndex)
{
	TDblQueIter<CBinderRef> iter(iBinders);
	CBinderRef* b = iter;
	while(aIndex-- && b )
    	{
    	iter++;
    	b = iter;
    	}
    return b ? b->iLinkLayer : NULL;
}
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS //PREQ399_REMOVE


