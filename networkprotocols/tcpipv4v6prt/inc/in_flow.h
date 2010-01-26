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



/**
 @internalComponent
*/
#ifndef __IN_FLOW_H__
#define __IN_FLOW_H__

#include <flow.h>


//	************
//	CFlowContext
//	************
//	A base class of the Flow Context, cannot be instantiated as is
//
class TFlowHook;
class MNetworkServiceExtension;
class CFlowInternalContext : public CFlowContext
	{
	friend class MFlowManager;
	friend class RFlowContext;
protected:
	CFlowInternalContext(const void *aOwner, MFlowManager *aManager)
		: CFlowContext(aOwner, aManager) {}
	CFlowInternalContext(const void *aOwner, MFlowManager *aManager, CFlowContext &aFlow)
		: CFlowContext(aOwner, aManager, aFlow) {}
	virtual ~CFlowInternalContext();
public:
	//
	// Hook management part
	//
	//
	// Hook Calling Sequences:
	//			Transport	Hooks		Flow		Interface
	//	Setup/prepare
	//			----->		Open() --->	StartL()
	//	After interface is ready
	//			<-----		Ready() <-- Reset()
	//	Packet pipeline (flow state = READY)
	//			----->		ApplyL() -->
	//	Shutdown
	//			<-----		Close() <--	
	//
	/**
	*/
	TInt ApplyHooks(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, RMBufPktQ &aFragments, MNetworkServiceExtension &aExt);
	/**
	// Attach the outbound flow hook handler to the flow.
	//
	// Called from CProtocolIP::FlowSetupHooks, when the OpenL
	// returns a non-NULL handle for the hook.
	//
	// If the parameter aFrag is non-negative, then it gives the
	// total header space requirement from the preceding hooks, and
	// requests that the fragmentation process to the path MTU
	// must be done before calling the ApplyL of this hook.
	//
	// @param aHook	The handler for the packets of the flow
	// @param aFrag Request fragmenation before this hook, if > -1
	*/
	void AddHookL(MFlowHook *aHook, TInt aFrag);
	/**
	// Recompute TPacketHead::iInterfaceIndex. Also, find non-zero
	// TPacketHead::iDstId, if unspecified after the hook (= 0).
	//
	// Use the current addressing information in the aHead and locate
	// a route. If a route is found, fill in iInterfaceIndex and iDstId
	// (if unspecified).
	//
	// This is internal and function which is called after OpenL
	// in CProtocolIP::FlowSetupHooks, in case the hook changed
	// the addresses of the TPacketHead (tunneling or some other
	// reason).
	//
	// @param	aHead
	//		current flow information
	// @return
	//	@li	EFlow_READY, if route found
	//	@li	EFlow_PENDING, if route not found (abort OpenL phase)
	*/
	virtual TInt RouteFlow(TPacketHead &aHead) = 0;

	/**
	// Remove all hooks from the flow.
	*/
	void RemoveHooks();
	// IsChanged should probably be in the base CFlowContext. -- msa
	inline TBool IsChanged()
		/**
		* Return the state iChanged bit.
		*/
		{
		return iChanged != 0;
		}
protected:
	/**
	// Start will save/freeze "interface end" state of the
	// flow information (TPackeHead).
	//
	// Start is called after all hooks have been added and their
	// preparation work (OpenL methods) has been completed. After
	// this the flow is ready for hook ReadyL() methods.
	*/
	void Start();
	/**
	// Reset is called to before the ReadyL() method chaing is
	// called (restores the flow to the state saved by Start())
	*/
	void Reset();
	/**
	// Call all ReadyL of all attached hooks
	*/
	void RefreshHooks();
private:
	TInt ApplyHooksL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	void ApplyHooksFragmentedL(TInt aStart, RMBufSendPacket &aPacket);
	/**
	// Records the currently attached hooks
	*/
	CArrayFixFlat<TFlowHook> *iHookList;
protected:
	/**
	// The flow parameters after the OpenL hooks have been run.
	// (see Start() and Reset()).
	*/
	TPacketHead iStart;
	};
#endif
