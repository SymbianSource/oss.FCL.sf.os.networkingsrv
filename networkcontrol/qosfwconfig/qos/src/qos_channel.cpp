// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qos_channel.h"
#include "flowhook.h"
#include "qos_prot.h"
#include "policy_sap.h"
#include "negotiation.h"
#include "qoserr.h"
#include "interface.h"
#include "modules.h"


#ifdef _DEBUG
// Used only in DEBUG builds
static const TIp6Addr KAllOnes = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};
#endif

TIpAddress::TIpAddress(const TInetAddr &aAddr)
	/**
	* Construct internal uniform address from TInetAddr content.
	*
	* All addresses are held in IPv6 format with scope id. Convert IPv4
	* address into IPv4 mapped format, if not already done. The scope id
	* is not available in that case.
	*/
	{
	static const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };

	if (aAddr.Family() != KAfInet6)
		{
		// Build IPv4 Mapped address
		TUint32 v4addr = aAddr.Address();
		u.iAddr32[0] = 0;
		u.iAddr32[1] = 0;
		u.iAddr32[2] = v4Prefix.b;
		u.iAddr8[15] = (TUint8)v4addr;
		u.iAddr8[14] = (TUint8)(v4addr >>  8);
		u.iAddr8[13] = (TUint8)(v4addr >> 16);
		u.iAddr8[12] = (TUint8)(v4addr >> 24);
		iScope = 0;	// No Scope id, it is available only in KAfInet6 format
		}
	else
		{
		(*(TIp6Addr *)this) = aAddr.Ip6Address();
		iScope = aAddr.Scope();
		}
	}

TBool TIpAddress::Match(const TInetAddr &aAddr) const
	/**
	* Return TRUE, if the address matches.
	*
	* @param aAddr Address to compare (must be in KAfInet6 format!)
	*/
	{
	return 	IsEqual(aAddr.Ip6Address()) && (iScope == 0 || iScope == aAddr.Scope());
	}
	
TInt TIpAddress::operator==(const TIpAddress& aAddr) const
	{
	return IsEqual(aAddr) && iScope == aAddr.iScope;
	}


TQoSConnId::TQoSConnId(const TPfqosMessage &aMsg) :
	iSrcPort(aMsg.iSrcAddr.iExt->pfqos_port_max),
	iDstPort(aMsg.iDstAddr.iExt->pfqos_port_max),
	iProtocol(aMsg.iSelector.iExt->protocol),
	iDst(*aMsg.iDstAddr.iAddr)
	/**
	* Construct a flow identification from a PFQOS Message.
	*/
	{
	// ...for debug, assert that the selector information as specific as is possible.
	ASSERT(KAllOnes.IsEqual(aMsg.iDstAddr.iPrefix->Ip6Address()));
	ASSERT(aMsg.iSelector.iExt->protocol != 0);
	ASSERT(aMsg.iDstAddr.iAddr->Port() == aMsg.iDstAddr.iExt->pfqos_port_max);
	ASSERT(aMsg.iSrcAddr.iAddr->Port() == aMsg.iSrcAddr.iExt->pfqos_port_max);
	}

TQoSConnId::TQoSConnId(const CFlowContext& aFlow) :
	iSrcPort(aFlow.LocalPort()),
	iDstPort(aFlow.RemotePort()),
	iProtocol(aFlow.Protocol()),
	iDst(aFlow.RemoteAddr())
	/**
	* Construct a flow identification from CFlowContext
	*/
	{
	}
	
TBool TQoSConnId::Match(const CFlowContext& aFlow) const
	{
	return
		iDst.Match(aFlow.RemoteAddr()) &&
		iProtocol == aFlow.Protocol() &&
		iDstPort == aFlow.RemotePort() &&
		iSrcPort == aFlow.LocalPort();
	}


TInt TQoSConnId::operator==(const TQoSConnId &aId) const
	{
	return
		iDst == aId.iDst &&
		iProtocol == aId.iProtocol &&
		iDstPort == aId.iDstPort &&
		iSrcPort == aId.iSrcPort;
	}

class CSapItem : public CBase
	{
	friend class CInternalQoSChannel;
private:
	~CSapItem();

	CSapItem(CQoSProvider& aSap, CSapItem* aNext);

	//?? For some reason the implementation allows the same SAP to be added
	//?? to the same channel multiple times, and requires equivalent amount
	//?? removes (or does it?), before actually removed. Use iRefs to handle it,
	//?? but should also study whether this is really needed?
	//?? iRefs == 0 corresponds to the first Add.
	CSapItem* iNext;
	TInt iRefs;
	CQoSProvider& iSap;
	};


CSapItem::CSapItem(CQoSProvider& aSap, CSapItem* aNext) : iNext(aNext), iSap(aSap)
	{
	}

CSapItem::~CSapItem()
	{
	}


#ifdef _LOG

// TQosConnId
void TQoSConnId::Dump() const
	{
	TLogAddress dst(iDst, iDstPort);
	Log::Printf(_L("\t\tconn: remote=%S srcport=%d prot=%d"), &dst, iSrcPort, iProtocol);	
	}

// CQoSConn
void CQoSConn::Dump() const
	{
	iId.Dump();
	const TInt N = iFlows.Count();
	if (N == 0)
		{
		Log::Printf(_L("\t\t\thas no active hooks attached"));
		}
	else for (TInt i = 0; i < N; )
		{
		CFlowHook* hook = iFlows[i++];
		Log::Printf(_L("\t\t\t%d. HOOK[%u] on FLOW[%u] attached"), i, (TInt)hook, (TInt)&hook->Context());
		}
	}

// CInternalQoSChannel
void CInternalQoSChannel::Dump() const
	{
	Log::Printf(_L("\t\tchannel[%u] has connections:"), iChannelId);
	if (iConnections == NULL)
		{
		Log::Printf(_L("\t\tNO CONNECTIONS"));		
		}
	else for (CQoSConn* conn = iConnections; conn != NULL; conn = conn->iNext)
		{
		conn->Dump();
		}
	for (CSapItem* sap = iSapList; sap != NULL; sap = sap->iNext)
		{
		Log::Printf(_L("\t\tSAP[%u] refs=%d"), (TInt)&sap->iSap, sap->iRefs);
		}
	}
#endif




CInternalQoSChannel* CInternalQoSChannel::NewL
	(TInt aChannelId, const TPfqosMessage& aMsg, CQoSProvider* aSap, TDblQue<CInternalQoSChannel>& aList)
	/**
	* Create a new channel and install a flow catcher.
	*/
	{
	CInternalQoSChannel* channel = new (ELeave) CInternalQoSChannel(aChannelId, aList);
	CleanupStack::PushL(channel);
	channel->ConstructL(aMsg, *aSap);
	CleanupStack::Pop();
	return channel;
	}

CInternalQoSChannel::CInternalQoSChannel(TInt aChannelId, TDblQue<CInternalQoSChannel>& aList) : iChannelId(aChannelId)
	{
	LOG(Log::Printf(_L("new\tqos channel[%u] size=%d"), aChannelId, sizeof(CInternalQoSChannel)));
	
	aList.AddLast(*this);	// Linked to the list in constructor and remove in destruct.
	iStatus = EChannelClosed;
	iInterface = NULL;
	iModulesel = NULL;
	}

void CInternalQoSChannel::ConstructL(const TPfqosMessage& aMsg, CQoSProvider& aSap)
	{
	iSapList = new (ELeave) CSapItem(aSap, iSapList);
	SetQoSParametersL(aMsg);
	}

CInternalQoSChannel::~CInternalQoSChannel()
	{
	LOG(Log::Printf(_L("~\tqos channel[%u] status=%d -- start"), iChannelId, iStatus));
	// The destructor may call methods that apply Lock()/Unlock(). To prevent
	// accidental re-entry into destructor, lock this object "permanently".
	Lock();
	iLink.Deque();
	//?? If there are any flows attached, this will start a LeaveSession for each
	//?? of them, but they may take time...
	CloseChannel();

	// Remove connections (these don't have any flows anymore)
	CQoSConn* conn;
	while ((conn = iConnections) != NULL)
		{
		iConnections = conn->iNext;
		delete conn;
		}

	//?? Generate Channel deleted event?
	//?? ... and should it be KPfqosEventConfirm instead of the failure?
	//?? If any LeaveSessions were started above, this event happens before
	//?? the leave eventes, which may cause problems...
	DeliverEvent(NULL, KPfqosEventFailure, 0, EQoSChannelDeleted);

	CSapItem* sap_item;
	while ((sap_item = iSapList) != NULL)
		{
		iSapList = sap_item->iNext;
		delete sap_item;
		}
	LOG(Log::Printf(_L("~\tqos channel[%u] status=%d -- done"), iChannelId, iStatus));
	}

void CInternalQoSChannel::CloseChannel()
	/**
	* Return channel to the initial closed state.
	*/
	{
	if(iStatus != EChannelClosed)
		{
		// If there are attached hooks, remove them first.
		CFlowHook* hook;
		while ((hook = Hook()) != NULL)
			{
			hook->CloseQoS();	// This *must* remove the hook from channel (otherwise loop does not terminate)
			}

		// Empty the module list and call the CloseChannel for each module in the list.
		// (Note: the list includes the traffic control module, if opened)
		for (TInt i = iModuleList.Count(); --i >= 0; )
			{
			RModule* const module = iModuleList[i];
			LOG(Log::Printf(_L("\t\tCalling module[%S]::CloseChannel(channel[%u])"), &module->Name(), iChannelId));
			module->Module()->CloseChannel(iChannelId);
			delete module;
			}
		iModuleList.Reset();

		iStatus = EChannelClosed;
		iInterface = NULL;
		iModulesel = NULL;
		// To even exist, the channel still must have at least one SAP and
		// a channel choined (in form of CQoSConn without any flows attaced).
		// (unless there is delayed delete)
		ASSERT(iPendingDelete == 1 || (iConnections != NULL && iSapList != NULL));
		}
	}

TBool CInternalQoSChannel::DeleteIfUnlocked()
	// @return TRUE, if channel was deleted now
	{
	iPendingDelete = 1;
	if (iLocks == 0)
		{
		delete this;
		return TRUE;
		}
	return FALSE;
	}

TBool CInternalQoSChannel::Unlock()
	// @return TRUE, if channel was deleted now.
	{
	ASSERT(iLocks > 0);
	if (--iLocks == 0 && iPendingDelete)
		{
		delete this;
		return TRUE;
		}
	return FALSE;
	}

void CInternalQoSChannel::AddSapL(CQoSProvider* aSap)
	{
	for (CSapItem* sap_item = iSapList; sap_item != NULL; sap_item = sap_item->iNext)
		{
		if (&sap_item->iSap == aSap)
			{
			LOG(Log::Printf(_L("\tqos channel[%u] AddSapL SAP[%u] -- already recorded"), iChannelId, (TInt)aSap));
			++sap_item->iRefs; // This SAP already registered for this channel.
			return;
			}
		}
	iSapList = new (ELeave) CSapItem(*aSap, iSapList);
	LOG(Log::Printf(_L("\tqos channel[%u] AddSapL SAP[%u] -- add reference"), iChannelId, (TInt)aSap));
	}

void CInternalQoSChannel::RemoveSap(CQoSProvider* aSap, TBool aAllReferences)
	/**
	* Remove a reference or all refs to a SAP from a channel.
	*
	* @param aSap The SAP
	* @param aAllReference Remove all references, if TRUE
	*/
	{
	Lock();		// ...don't allow channel to destruct from underneath!
	CSapItem* sap_item = NULL;
	for (CSapItem** head = &iSapList; (sap_item = *head) != NULL; head = &sap_item->iNext)
		{
		if (&sap_item->iSap == aSap)
			{
			if (aAllReferences || --sap_item->iRefs < 0)
				{
				*head = sap_item->iNext;
				delete sap_item;
				LOG(Log::Printf(_L("\tqos channel[%u] RemoveSap SAP[%u] -- removed"), iChannelId, (TInt)aSap));
				break; // All done, there can be only one item per SAP.
				}
			else
				{
				LOG(Log::Printf(_L("\tqos channel[%u] RemoveSap SAP[%u] -- removed one reference"), iChannelId, (TInt)aSap));
				}
			}
		}
	if (iSapList == NULL)
		{
		// The channel self destructs when no SAPs are attached!
		DeleteIfUnlocked();
		}
	Unlock();	// .. this closes our own lock.
	}

TInt CInternalQoSChannel::Join(const TQoSConnId& aIdent, CProtocolQoS& aProtocol)
	/**
	* Internally add the connection identifier to the channel.
	*
	* @param aIdent Identifies the connection
	* @param aProtocol The QoS protoocl module (only for waking up the flows when needed)
	* @return KErrNone or KErrNoMemory
	*/
	{
	CQoSConn* conn = new CQoSConn(aIdent, *this);
	if (conn == NULL)
		return KErrNoMemory;
	conn->iNext = iConnections;
	iConnections = conn;

	// Check if the connection identified by aIdent already has a flow(s) opened
	// and if it has, then those flows must be reopened now, so that they will
	// attach to the channel now.
	//?? This needs fixing, only finds one flow. Should probably loop over
	//?? all flows originating from the connection... but, such condition
	//?? should very rare with connected sockets...
	CFlowHook* hook = aProtocol.FindHook(aIdent);
	if (hook)
		{
		hook->RestartQoS();
		}
	return KErrNone;
	}

TInt CInternalQoSChannel::Leave(const TQoSConnId& aId)
	/**
	* Forced leave of the connection from a channel.
	*/
	{
	CQoSConn** head = &iConnections;
	CQoSConn* conn;
	for ( ; (conn = *head) != NULL; head = &conn->iNext)
		{
		if (aId == conn->iId)
			{
			*head = conn->iNext;
			// Note: CQoSConn may activate Leave sessions, which upon completion can send
			// Leave Events (however, no events occur, if there are no matching CFlowContexts!)
			delete conn;
			if (iConnections == NULL)
				{
				// The channel self destructs when the last connection (catcher) is removed.
				//?? This happens even if there are SAP's attached? Should it?

				DeleteIfUnlocked();
				}
			return KErrNone;
			}
		}
	return KErrNotFound;
	}
	

CQoSConn* CInternalQoSChannel::Exists(const TQoSConnId& aId) const
	{
	for (CQoSConn* conn = iConnections; conn != NULL; conn = conn->iNext)
		{
		if (aId == conn->iId)
			{
			return conn;
			}
		}
	return NULL;
	}


CQoSConn* CInternalQoSChannel::Catched(CFlowHook* aFlow) const
	{
	for (CQoSConn* conn = iConnections; conn != NULL; conn = conn->iNext)
		{
		if (conn->iId.Match(aFlow->Context()))
			{
			conn->Attach(*aFlow);
			return conn;
			}
		}
	return NULL;
	}

CQoSConn::~CQoSConn()
	{
	// Need to detach all flows still using this connection!
	while (iFlows.Count() > 0)
		{
		CFlowHook* hook = iFlows[0];
		iFlows.Remove(0);
		hook->SetQoSChannel(NULL);
		(void)hook->StartPendingRequest(new CLeaveSession(*hook, iChannel.ChannelId()));
		}
	iFlows.Close();
	}


void CQoSConn::Detach(CFlowHook& aHook)
	/**
	* Detach the CFlowHook from a channel.
	*
	* The caller informs that it has removed the reference.
	*
	* This is only a notification that a flow context is being detached. It is not
	* a LEAVE Event! This only removes the CFlowHook reference. The flow identified
	* by the iId is still logically joined to the channel, although there may not
	* be any active flow context at this point.
	*/
	{
	ASSERT(aHook.Channel() == this);
	ASSERT(iFlows.Count() > 0);		// ... must not be called unless actually attached.
	const TInt i = iFlows.Find(&aHook);
	ASSERT(i >= 0);					// ... must not be called unless actually attached.
	if (i >= 0)
		{
		iFlows.Remove(i);
		}
	aHook.SetQoSChannel(NULL);		// Remove the back pointer.
	LOG(iChannel.Dump());
	}

void CQoSConn::Attach(CFlowHook& aHook)
	/**
	* Attach the CFlowHook to a channel
	* (does not generate join event! That comes from
	* the Join or Create channel session).
	*/
	{
	if (iFlows.Append(&aHook) == KErrNone)
		{
		aHook.SetQoSChannel(this);
		}
	LOG(iChannel.Dump());
	}
	
	
CFlowHook* CQoSConn::Hook(TInt aIndex)
	/**
	* Return Nth hook currently attached.
	*
	* @param aIndex Starts from 0 (for the first flow)
	* @return NULL, if not available
	*/
	{
	return (aIndex < iFlows.Count()) ? iFlows[aIndex] : NULL;
	}

CFlowHook* CInternalQoSChannel::Hook()
	/**
	* Return first attached hook or NULL.
	*/
	{
	for (CQoSConn* conn = iConnections; conn != NULL; conn = conn->iNext)
		{
		CFlowHook *hook = conn->Hook(0);
		if (hook)
			return hook;
		}
	return NULL;
	}

TInt CInternalQoSChannel::SetPolicy(TPfqosMessage& aMsg)
	{
	TRAPD(err, SetQoSParametersL(aMsg));
	if (err == KErrNone)
		{
		if (iStatus == EChannelReady)
			{
			CFlowHook* hook = Hook();
			if (hook)
				{
				hook->SetQoS(iQoS);				// set values
				iQoS = hook->QoSParameters();	// retrieve possibly modified values.
				iStatus = EChannelPending;
				iQoSChanged = 0;
				return hook->StartPendingRequest(new CNegotiateChannelSession(*hook, ChannelId()));
				//?? Should other flows be blocked while this is going on?
				}
			// No flows, cannot negotiate now
			}
		iQoSChanged = 1;
		}
	return err;
	}


TInt CQoSConn::HookReadyL(CFlowHook& aHook)
	{
	ASSERT(aHook.Channel() == this);
	return iChannel.HookReadyL(aHook);	
	}

TInt CInternalQoSChannel::HookReadyL(CFlowHook& aHook)
	/**
	* A hook assigned to this channel is asking whether it can go into ready state.
	*
	* @return
	*	@li EFlow_READY, if can proceed
	*	@li EFlow_PENDING, if channel creation or join is not yet ready
	*	@li < 0 or leaves in case of any fatal errors (the flow is aborted)
	*/
	{
	LOG(Log::Printf(_L("\tqos HOOK[%u] HookReady on channel[%u]"), (TInt)&aHook, iChannelId));
	switch (iStatus)
		{
	case EChannelClosed:
		// The first flow attaching to the channel.
		iInterface = aHook.Interface();
		iModulesel = aHook.Policies().iModules;

		// Save a copy of the modules which are being notified about this
		// channel. To be exact, should collect this list from succesfull
		// subrequest completions in CreateChannelSession -- now if there
		// is an error half way, this may generate some extra CloseChannel
		// calls.
			{
			ASSERT(iModuleList.Count() == 0);
			const TInt N = aHook.ModuleList().Count();
			for (TInt i = 0; i < N; ++i)
				{
				// Make a copy of the RModule.
				RModule *module = new RModule(*aHook.ModuleList()[i]);
				if (module == NULL || iModuleList.Append(module) != KErrNone)
					{
					// Out of memory!
					}
				}
			}

		// The HOOK SetQoS will adjust parameters, if "header mode" is set,
		// and then turns the "header mode" off to indicate that adjusts
		// have been made.
		aHook.SetQoS(iQoS);
		// Assign the possibly adjusted parameters back to channel.
		iQoS = aHook.QoSParameters();
		iQoSChanged = 0;			// Changed parameters being negotiated in CreateChannelSession
		iStatus = EChannelPending;
		return aHook.StartPendingRequest(new (ELeave) CCreateChannelSession(aHook, ChannelId()));

	case EChannelPending:
	default:
		return EFlow_PENDING;

	case EChannelReady:
		// Check that same interface is used
		if (aHook.Interface() != iInterface)
			{
			return EQoSInterface;
			}

		// Check that same modules are used
		if (aHook.Policies().iModules != iModulesel)
			{
			return EQoSModules;
			}

		aHook.SetQoS(iQoS);
		
		// Choose this hook as "victim" for QoS negotiation, if parameters
		// have been changed on the running channel.
		if (iQoSChanged)
			{
			iStatus = EChannelPending;
			iQoSChanged = 0;
			return aHook.StartPendingRequest(new CNegotiateChannelSession(aHook, ChannelId()));
			}

		if (aHook.ChannelJoined())
			return EFlow_READY;
		
		// This is the first time flow is seen, channel already created,
		// this flow needs to be joined to the channel now.
		iStatus = EChannelPending;
		return aHook.StartPendingRequest(new (ELeave) CJoinSession(aHook, ChannelId()));
		//?? Should other already joined flows be blocked while Join
		//?? request is being processed? Currently no.
		}
	}

void CInternalQoSChannel::RequestComplete(TInt /*aReason*/)
	{
	switch (iStatus)
		{
	case EChannelClosed:
		break;

	case EChannelReady:
	case EChannelPending:
	default:
		// Channel Session has completed.
		iStatus = EChannelReady;
		break;
		}

	// Wakeup all flows currently associated with this channe.
	for (CQoSConn* conn = iConnections; iStatus == EChannelReady && conn != NULL; conn = conn->iNext)
		{
		CFlowHook *hook;
		for (TInt i = 0; iStatus == EChannelReady && (hook = conn->Hook(i)) != NULL; ++i)
			{
			hook->Context().SetStatus(EFlow_READY);
			}
		}
	}

void CInternalQoSChannel::SetQoSParametersL(const TPfqosMessage& aMsg)
	{
	const struct pfqos_flowspec *flowspec = aMsg.iFlowSpec.iExt;
	if (flowspec == NULL)
		{
		User::Leave(KErrArgument);
		}
	
	// Uplink
	iQoS.SetUpLinkDelay(flowspec->uplink_delay);
	iQoS.SetUpLinkMaximumPacketSize(flowspec->uplink_maximum_packet_size);
	iQoS.SetUpLinkAveragePacketSize(flowspec->uplink_average_packet_size);
	iQoS.SetUpLinkPriority(flowspec->uplink_priority);
	iQoS.SetUpLinkMaximumBurstSize(flowspec->uplink_maximum_burst_size);
	iQoS.SetUplinkBandwidth(flowspec->uplink_bandwidth);
	iQoS.SetName(flowspec->name);

	// Downlink
	iQoS.SetDownLinkDelay(flowspec->downlink_delay);
	iQoS.SetDownLinkMaximumPacketSize(flowspec->downlink_maximum_packet_size);
	iQoS.SetDownLinkAveragePacketSize(flowspec->downlink_average_packet_size);
	iQoS.SetDownLinkPriority(flowspec->downlink_priority);
	iQoS.SetDownLinkMaximumBurstSize(flowspec->downlink_maximum_burst_size);
	iQoS.SetDownlinkBandwidth(flowspec->downlink_bandwidth);

	// Flags
	iQoS.SetFlags(flowspec->flags);


	//?? Dubious, but for now, just put a copy of parameters into each
	//?? flow currently attached.
	for (CQoSConn* conn = iConnections; iStatus == EChannelReady && conn != NULL; conn = conn->iNext)
		{
		CFlowHook *hook;
		for (TInt i = 0; (hook = conn->Hook(i)) != NULL; ++i)
			{
			hook->SetQoS(iQoS);
			}
		}

	// Delete old extensions
	TExtensionQueueIter iE(iExtension.Extensions());
	CExtension* extension;
	while ((extension=iE++)!=NULL)
		{
		extension->iNext.Deque();
		delete extension;
		}

	// Add new extensions (there should be "const" iterator!)
	TSglQueIter<CPfqosPolicyData> iter(((TPfqosMessage&)aMsg).iExtensions);
	CPfqosPolicyData *data;
	while ((data = iter++) != NULL)
		{
		iExtension.AddExtensionL(data->Data());
		}
	}

void CInternalQoSChannel::GetExtensionsL(TPfqosMessage& aMsg)
	{
	TExtensionQueueIter iter(iExtension.Extensions());
	CExtension* extension;
	while ((extension = iter++) != NULL)
		{
		aMsg.AddExtensionL(extension->Data(), extension->Type());
		}
	}

void CInternalQoSChannel::DeliverEvent(CFlowHook* aHook, TUint16 aEvent, TUint16 aValue, TInt aErrorCode)
	{
	TPfqosMessage msg;
	T_pfqos_msg base(EPfqosEvent);
	base.pfqos_msg_errno = aErrorCode;
	msg.iBase.iMsg = &base;

	T_pfqos_event event(EPfqosExtEvent, aEvent, aValue);
	msg.iEvent.iExt = &event;

	T_pfqos_flowspec spec(iQoS);
	msg.iFlowSpec.iExt = &spec;

	// Add flow identifying PF_QOS blocks (if flow known)
	TInetAddr msk;
	pfqos_address src, dst;
	pfqos_selector sel;
	if (aHook != NULL)
		aHook->FillFlowInfo(msg, src, dst, sel, msk);

	// Add channel id block
	T_pfqos_channel channel(iChannelId);
	msg.iChannel.iExt = &channel;

	msg.iNumModules = 0;

	Lock();	// ..prevent Channel destruction within the loop, if some Deliver destroys the channel!
	for (CSapItem* sap_item = iSapList; sap_item != NULL; )
		{
		//?? Ugh.. cannot see if this Deliver() can destruct the current
		//?? sap_item. Try to cope with sap_item going away (and if happens,
		//?? hope it's the only one to go, or this crashes anyway...)
		//?? Need Lock()/Unlock() for CSapItem too?
		CSapItem* next = sap_item->iNext;
		sap_item->iSap.Deliver(msg);
		sap_item = next;
		}
	Unlock();
	}

//
CQoSChannelManager* CQoSChannelManager::NewL(CProtocolQoS& aProtocol)
	{
	return new (ELeave) CQoSChannelManager(aProtocol);
	}

CQoSChannelManager::~CQoSChannelManager()
	{
	LOG(Log::Printf(_L("~\tqos Channel Manager[%u] -- start"), (TInt)this));
	while (!iChannels.IsEmpty())
		{
		CInternalQoSChannel* channel = iChannels.First();
		LOG(Log::Printf(_L("\tqos closing channel:")));
		LOG(channel->Dump());
		// Channel should not be locked here! If it is, there is a serious
		// problem somewhere!
		if (!channel->DeleteIfUnlocked())
			{
			//?? Should have some other solution (like "mathematically"
			//?? being sure that this never happens...)
			User::Panic(_L("QOS"), KErrGeneral);
			}
		}
	LOG(Log::Printf(_L("~\tqos Channel Manager[%u] -- done"), (TInt)this));
	}

CQoSChannelManager::CQoSChannelManager(CProtocolQoS& aProtocol) : iProtocol(aProtocol)
	{
	LOG(Log::Printf(_L("new\tqos Channel Manager[%u] size=%d"), (TInt)this, sizeof(CQoSChannelManager)));
	iChannels.SetOffset(_FOFF(CInternalQoSChannel, iLink));
	iNextChannelNumber = KQoSChannelNumberMin;
	}


CInternalQoSChannel* CQoSChannelManager::NewChannelL(const TPfqosMessage& aMsg, CQoSProvider* aSap)
	/**
	* Create a channel and join connection to it.
	*
	* Create a channel and record the first connection (socket) to it. There may or
	* may not be any actual CFlowHooks with this connection at this point.
	*
	* @param aMsg Only used for identifying the connection
	* @param aSap The controlling SAP.
	*/
	{	
	// Construct the connection identifier from the PFQOS message.
	const TQoSConnId ident(aMsg);
	LOG(Log::Printf(_L("\tqos NewChannel for connection:")));
	LOG(ident.Dump());
	// If the flow is already assigned to a channel, then
	// the channel cannot created.
	CInternalQoSChannel* channel = FindChannel(ident);
	if (channel)
		{
		LOG(Log::Printf(_L("\tqos NewChannel for connection failed -- already joined:")));
		LOG(channel->Dump());
		User::Leave(KErrAlreadyExists);
		}
	// Can create a new channel for the (first) flow
	channel = CInternalQoSChannel::NewL(AssignChannelId(), aMsg, aSap, iChannels);

	channel->Lock();
	// Create the QoS connection to the channel.
	TInt ret = channel->Join(ident, iProtocol);
	//?? Above is somewhat hard to follow, because if there is a matching flow,
	//?? it will call ResetQoS for it, and lots of things happen before it
	//?? comes back here (CFlowHook::ReadyL among other things...). Need to
	//?? use the Lock/Unlock thingie...
	if (channel->Unlock())
		{
		// Ouch! Someone deleted the channel already...
		User::Leave(KErrNotFound);
		}
	if (ret != KErrNone)
		{
		// Ouch! Above Join() failed for out of memory -- delete the channel here
		channel->DeleteIfUnlocked();
		User::Leave(ret); 
		}
	// Channel successfully created with 1 flow catcher and 1 sap attached.
	LOG(Log::Printf(_L("\tqos NewChannel -- complete:")));
	LOG(channel->Dump());
	return channel;
	}
	

CInternalQoSChannel* CQoSChannelManager::OpenChannelL(const TPfqosMessage& aMsg, CQoSProvider* aSap)
	/**
	* Open an existing channel defined by a connection.
	*
	* Open a channel associated with the connection.
	*
	* @param aMsg Only used for identifying the connection
	* @param aSap The controlling SAP.
	*/
	{
	// Construct the connection identifier from the PFQOS message.
	const TQoSConnId ident(aMsg);

	CInternalQoSChannel* channel = FindChannel(ident);	// Locate channel by connection ident.
	if (channel)
		{
		channel->AddSapL(aSap);
		LOG(Log::Printf(_L("\tqos OpenChannel SAP[%u] to channel[%u]"), (TInt)aSap, channel->iChannelId));
		LOG(channel->Dump());
		}
	else
		{
		LOG(Log::Printf(_L("\tqos OpenChannel SAP[%u] failed -- cannot find channel for:"), (TInt)aSap));
		LOG(ident.Dump());
		}
	return channel;
	}
	
TInt CQoSChannelManager::JoinChannel(const TPfqosMessage& aMsg, TInt aChannelId)
	/**
	* Join connection to an existing channel.
	*
	* Record additional connection (socket) to a channel. There may or
	* may not be any actual CFlowHooks with this connection at this point.
	*
	* @param aMsg Only used for identifying the connection
	* @param aSap The controlling SAP.
	*/
	{
	TQoSConnId ident(aMsg);	// Construct TQoSConnId
	CInternalQoSChannel* channel = FindChannel(ident);
	if (channel)
		{
		LOG(Log::Printf(_L("\tqos JoinChannel failed -- identified connection already in another channel")));
		LOG(channel->Dump());
		return KErrAlreadyExists; // The identified flow already has a channel. Cannot join a new channel.
		}
	else if ((channel = FindChannel(aChannelId)) == NULL)
		{
		LOG(Log::Printf(_L("\tqos JoinChannel failed -- channel %d does not exist"), aChannelId));
		LOG(ident.Dump());
		return KErrNotFound; // There is no channel with that id
		}
	else
		{
		// Channel found and this flow (selector) does not exist yet
		TInt ret = channel->Join(ident, iProtocol);
		LOG(Log::Printf(_L("\tqos JoincChannel result = %d for channel:"), ret));
		LOG(channel->Dump());
		return ret;
		}
	}
	
TInt CQoSChannelManager::LeaveChannel(const TPfqosMessage& aMsg, TInt aChannelId)
	/**
	* Leave connection from a channel.
	*
	* Remove connection (socket) to a channel. There may or
	* may not be any actual CFlowHooks with this connection at this point.
	*
	* @param aMsg Only used for identifying the connection
	* @param aSap The controlling SAP.
	*/
	{
	TQoSConnId ident(aMsg);	// Construct TQoSConnId
	CInternalQoSChannel* channel = FindChannel(aChannelId);
	CInternalQoSChannel* channel2 = FindChannel(ident);
	if (channel == NULL)
		{
		LOG(Log::Printf(_L("\tqos LeaveChannel failed -- channel %d does not exist"), aChannelId));
		return KErrNotFound;
		}
	else if (channel2 != channel)
		{
#ifdef _LOG
		if (channel2 == NULL)
			{
			Log::Printf(_L("\tqos LeaveChannel failed -- connection is not attached to channel"));
			channel->Dump();
			}
		else
			{
			Log::Printf(_L("\tqos LeaveChannel failed -- connection is not attached to channel"));
			channel->Dump();
			Log::Printf(_L("\t\t- connection attached currently:"));
			channel2->Dump();			
			}
#endif
		
		return KErrNotFound;	// the connection is assigned to a different channel or not joined
		}
	else
		{
		LOG(Log::Printf(_L("\tqos LeaveChannel -- leaving connection from:")));
		LOG(channel->Dump());
		channel->Leave(ident);
		return KErrNone;
		}
	}

TInt CQoSChannelManager::DetachFromOne(CQoSProvider* aSap, TUint aChannelId)
	/**
	* Detach a SAP from a channel.
	*
	* This only removes the SAP from channel, and the channel destructs
	* when the last SAP is removed.
	*
	* note: the same SAP can be attached multiple times to same channel, this
	* only removes one attachment
	*
	* @param aSap The SAP.
	* @param aChannelId Identify the channel
	* @return KErrNotFound, if channel does not exist
	*/
	{
	CInternalQoSChannel* channel = FindChannel(aChannelId);
	if (!channel)
		{
		return KErrNotFound;
		}
	// Only one reference removed...
	channel->RemoveSap(aSap, FALSE);
	return KErrNone;
	}

void CQoSChannelManager::DetachFromAll(CQoSProvider* aSap)
	/**
	* Remove all references to SAP.
	*
	* @param aSap The SAP.
	*/
	{
	TDblQueIter<CInternalQoSChannel> iter(iChannels);
	CInternalQoSChannel* channel;
	while ((channel = iter++) != NULL)
		{
		// Remove uncoditionally all references
		channel->RemoveSap(aSap, TRUE);
		}
	}


CInternalQoSChannel* CQoSChannelManager::FindChannel(TUint aChannelId)
	{
	TDblQueIter<CInternalQoSChannel> iter(iChannels);
	CInternalQoSChannel* channel;

	while ((channel = iter++) != NULL)
		{
		if (channel->ChannelId() == (TInt)aChannelId)
			{
			return channel;
			}
		}
	return NULL;
	}
	
CInternalQoSChannel* CQoSChannelManager::FindChannel(const TQoSConnId& aIdent)
	{
	TDblQueIter<CInternalQoSChannel> iter(iChannels);
	CInternalQoSChannel* channel;

	while ((channel = iter++) != NULL)
		{
		if (channel->Exists(aIdent))
			{
			return channel;
			}
		}
	return NULL;
	}

CQoSConn* CQoSChannelManager::AttachChannel(CFlowHook *aHook)
	/**
	* Test if flow belongs to a connection and if it does, attach it.
	*
	* @return NULL, if not in channel.
	*/
	{
	TDblQueIter<CInternalQoSChannel> iter(iChannels);
	CInternalQoSChannel* channel;

	while ((channel = iter++) != NULL)
		{
		CQoSConn* conn = channel->Catched(aHook);
		if (conn)
			{
			return conn;
			}
		}
#ifdef _LOG
	Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] does not match any channel"), (TInt)aHook, (TInt)&aHook->Context());
	TQoSConnId(aHook->Context()).Dump();
#endif
	return NULL;
	}


void CQoSChannelManager::InterfaceDetached(CInterface* aInterface)
	/**
	* Remove all references to the interface.
	*/
	{
	TDblQueIter<CInternalQoSChannel> iter(iChannels);
	CInternalQoSChannel* channel;

	while ((channel = iter++) != NULL)
		{
		if (channel->Interface() == aInterface)
			{
			channel->DeliverEvent(NULL, KPfqosEventFailure, 0, EQoSNoInterface);
			//?? why should interface detach delete of channel? Channels are
			//?? created without interface and interface is attached later.
			//?? It could only be closed.
			//?? channel->DeleteIfUnlocked();
			channel->CloseChannel();
			}
		}
	}

TInt CQoSChannelManager::AssignChannelId()
	{
	TUint i, channel_id;

	for (i = KQoSChannelNumberMin; i <= KQoSChannelNumberMax; i++)
		{
		channel_id = iNextChannelNumber++;
		if (iNextChannelNumber > KQoSChannelNumberMax)
			{
			iNextChannelNumber = KQoSChannelNumberMin;
			}
		if (FindChannel(channel_id) == NULL)
			{
			return channel_id;
			}
		}
	return KQoSChannelNone;
	}
