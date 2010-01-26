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
// in_flow.cpp - stack side of the general part of the flow
//

#include <es_mbuf.h>
#include "in_flow.h"
#include "inet6log.h"
#include "iface.h"

static const TUint16 KNoFrag = 0xFFFF;
//
//	TFlowHook
//
class TFlowHook
	{
public:
	TFlowHook(MFlowHook &aHook, TInt aFrag) : iHook(aHook), iFrag(aFrag < 0 ? KNoFrag : (TUint16)aFrag) {}
	MFlowHook &iHook;			//< The Flow Hook
	const TUint16 iFrag;		//< The header size before OpenL (or 0xFFFF, if no fragmenting)
	TUint16 iMtu;				//< The Fragmenting MTU (only if iFrag != 0xFFFF)
	};

CFlowInternalContext::~CFlowInternalContext()
	{
	RemoveHooks();
	}

//	**********************
//	CFlowContext::AddHookL
//	**********************
//	aHook must never be NULL!
//
void CFlowInternalContext::AddHookL(MFlowHook *aHook, TInt aFrag)
	{
	//
	// Allow call with NULL aHook, does nothing then...
	//
	if (!aHook)
		return;
	//
	// iHookList is created only if actual hooks are required
	//
	if (iHookList == NULL)
		iHookList = new (ELeave) CArrayFixFlat<TFlowHook>(4);
	TFlowHook h(*aHook, aFrag);
	iHookList->AppendL(h);
	aHook->Open();
	}

//	*************************
//	CFlowContext::RemoveHooks
//	*************************
//
void CFlowInternalContext::RemoveHooks()
	{
	//
	// Remove hooks
	//
	if (iHookList)
		{
		for (TInt i = iHookList->Count(); i > 0; )
			{
			const TFlowHook &h = iHookList->At(--i);
			h.iHook.Close();
			}
		delete iHookList;
		iHookList = NULL;
		}
	}

//	*******************
//	CFlowContext::Start
//	*******************
//	Must be called once when the flow Open/Hook adding has
//	been completed, but before calling the hook Ready methods
//
void CFlowInternalContext::Start()
	{
	__ASSERT_DEBUG(iHead.iPacket.IsEmpty(), User::Panic(_L("iHead.iPacket not empty"), 0));

	iStart = iHead;					// Just save a copy of the iHead.
	iStart.iOffset = iHdrSize;		// Borrow the iOffset field to hold the initial header overhead
	}

//	*******************
//	CFlowContext::Reset
//	*******************
//	Can be called any time to set the flow into initial state
//	(called before hook Ready method calls)
//
void CFlowInternalContext::Reset()
	{
	iHead.iPacket.Free();
	iHead = iStart;
	//
	// Reset the initial (after Open-phase) header overhead
	//
	iHdrSize = iHead.iOffset;
	iHead.iOffset = 0;
	}

//
//	CFlowContext::RefreshHooks
//	**************************
//	(Utility for derived classes)
//	Call the Ready() methods of the registered hooks
//	and check if all of them are ready. If so, the state
//	of the flow will be EFlow_READY, otherwise the state
//	will be the state of the first hook that reports
//	not READY.
//
void CFlowInternalContext::RefreshHooks()
	{
	TInt err;
	iStatus = EFlow_READY;
	if (iHookList)
		{
		LOG(const MFlowHook *tmp = NULL);
		TRAP(err,
			for (TInt i = iHookList->Count(); iStatus == EFlow_READY && i > 0; )
				{
				TFlowHook &h = iHookList->At(--i);
				iStatus = h.iHook.ReadyL(iHead);
				LOG(tmp = &h.iHook);
				// Compute fragmenting MTU, if fragmenting is requested
				// (result is garbage if iFrag == KNoFrag, but it does not matter)
				h.iMtu = (TUint16)(PathMtu() - HeaderSize() - h.iFrag);
				}
			); // TRAP
		if (err != KErrNone)
			{
			LOG(Log::Printf(_L("\tMFlowHook[%u] ReadyL LEAVE WITH err=%d"), (TInt)tmp, err));
			// *NOTE*
			//		Must ensure that the status will be negative,
			//		because positive values are interpreted as
			//		"pending" signals and a later wakeup is
			//		expected. It is doubtful that a hook leaves
			//		unexpectedly and still has set for the wakeup!
			//		... though, I suppose this should never
			//		happen? Leave errors are always negative?
			//		-- msa
			//	Force KErrGeneral, if err is not negative!
			iStatus = err < 0 ? err : KErrGeneral;
			}
		LOG(if (iStatus != EFlow_READY) Log::Printf(_L("\tMFlowHook[%u] ReadyL iStatus = %d"), (TInt)tmp, iStatus));
		}
	}

//	CFlowInternalContext::ApplyHooksL
// **********************************
TInt CFlowInternalContext::ApplyHooksL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	{
	const TInt n = iHookList->Count();
	for (TInt i = 0; i < n; )
		{
		const TFlowHook &h = iHookList->At(i++);
		if (h.iFrag != KNoFrag && aInfo.iLength > h.iMtu)
			return --i;
		
		const TInt ret = h.iHook.ApplyL(aPacket, aInfo);
		if (ret > 0)
			{
			LOG(Log::Printf(_L("\tMFlowHook[%u] ApplyL returns %d, restarat ApplyL (*deprecated feature used*)"), (TInt)&h.iHook, ret));
			i = 0;			// Restart hooks.
			}
		else if (ret < 0)
			{
			LOG(Log::Printf(_L("\tMFlowHook[%u] ApplyL returns %d, packet thrown away"), (TInt)&h.iHook, ret));
			User::Leave(ret);
			}
		else if (iHookList == NULL)
			{
			// Returned KErrNone, but the iHookList has disappeared
			LOG(Log::Printf(_L("\tMFlowHook[%u] ApplyL disconnected the flow"), (TInt)&h.iHook));
			User::Leave(KErrDisconnected);
			}
		}
	return n;
	}
	
void CFlowInternalContext::ApplyHooksFragmentedL(TInt aStart, RMBufSendPacket &aPacket)
	{
	RMBufSendInfo *info = aPacket.Unpack();
	const TInt n = iHookList->Count();
	for (TInt i = aStart; i < n; )
		{
		const TFlowHook &h = iHookList->At(i++);
		const TInt ret = h.iHook.ApplyL(aPacket, *info);
		if (ret > 0)
			{
			LOG(Log::Printf(_L("\tMFlowHook[%u] ApplyL (fragment) returns %d, restart ApplyL (*deprecated feature used*)"), (TInt)&h.iHook, ret));
			i = 0;			// Restart hooks.
			}
		else if (ret < 0)
			{
			LOG(Log::Printf(_L("\tMFlowHook[%u] ApplyL (fragment) returns %d"), (TInt)&h.iHook, ret));
			User::Leave(ret);
			}
		else if (iHookList == NULL)
			{
			// Returned KErrNone, but the iHookList has disappeared
			LOG(Log::Printf(_L("\tMFlowHook[%u] ApplyL (fragment) disconnected the flow"), (TInt)&h.iHook));
			User::Leave(KErrDisconnected);
			}
		}
	aPacket.Pack();
	}
//
//	CFlowContext::ApplyHooks
//	************************
//	Apply hooks on flow to the packet. This has only two possible
//	returns
//
//	== KErrNone	- hooks applied successfully
//	!= KErrNone - error occurred, packet has been released	
//
TInt CFlowInternalContext::ApplyHooks(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, RMBufPktQ &aFragments, MNetworkServiceExtension &aExt)
	{
	TInt ret = KErrNone;
	if (iHookList)
		{
		TInt n = 0;
		TRAPD(err, n = ApplyHooksL(aPacket, aInfo));
		if (err != KErrNone)
			{
			// The ApplyL has left, free packet!
			LOG(Log::Printf(_L("\tApplyHooks (ApplyL) dropping packet with reason %d"), err));
			if (!aPacket.IsEmpty())
				{
				LOG(Log::Printf(_L("\tApplyHooks (ApplyL) packet is not empty, closing flow reference")));
				aInfo.iFlow.Close();
				}
			aPacket.Free();
			return err;
			}
		if (n < iHookList->Count())
			{
			// Packet too long for MTU and hook wants fragmenting
			// done before it.
			RMBufPktQ fragments;
			if (aExt.Fragment(aPacket, aInfo, iHookList->At(n).iMtu, fragments))
				{
				TRAP(err,
					while (fragments.Remove(aPacket))
						{
						ApplyHooksFragmentedL(n, aPacket);
						aFragments.Append(aPacket);
						}
						);
				ret = err;
				}
			aFragments.Append(fragments); // Just in case some was left there...
			LOG(if (ret != KErrNone) Log::Printf(_L("\tApplyHooks (ApplyL fragments) returning %d"), err));
			return ret;
			}
		}
	if (aInfo.iLength > PathMtu() && !aExt.Fragment(aPacket, aInfo, PathMtu(), aFragments))
		{
		// Fragmentation required, but failed for some reason. The loopback ICMP has already been sent,
		// Just return something != KErrNone for the caller to indicate that the aPacket has been
		// processed (and should be empty at this point). 
		ret = KErrNotSupported;
		}
	return ret;
	}
