// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IP Shim Flow Base class.
// This class is derived from by the separate IP4 and IP6 shim flow classes.
// 
//

/**
 @file flow.cpp
*/

#include <e32std.h>
#include <e32test.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <comms-infras/ss_metaconnprov.h>
#include <comms-infras/ss_subconnflow.h>
#include "flow.h"
#include "nif.h"
#include "panic.h"
#include "ItfInfoConfigExt.h"
#include "IPProtoMessages.h"	// TCFIPProtoMessage
#include "IPProtoCPR.h"
#include "idletimer.h"
#include <networking/ipaddrinfoparams.h>
#include "IPProtoDeMux.h"
#include <comms-infras/ss_log.h>
#include <es_prot_internal.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace IpProtoCpr;
//
// CIPShimSubConnectionFlow
//

CIPShimSubConnectionFlow::CIPShimSubConnectionFlow(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
: CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf), iCleanupError(KErrUnknown),
  iAsyncBinderClose(*this)
	{
	LOG_NODE_CREATE(KIPProtoTag1, CIPShimSubConnectionFlow);
	}

CIPShimSubConnectionFlow::~CIPShimSubConnectionFlow()
	{
	ASSERT(iBinderList.Count() == 0);
    iConnectionInfo.Close();
	// iIntf doesn't need to be deleted explicitly, as it will be deleted
	// when the last Close() is issued on it (i.e. either by us or by the TCP/IP
	// stack).  At the time of writing, our call to Close() is the last one and
	// will delete it.
	LOG_NODE_DESTROY(KIPProtoTag1, CIPShimSubConnectionFlow);
	}

MFlowBinderControl* CIPShimSubConnectionFlow::DoGetBinderControlL()
	{
	return this;
	}


void CIPShimSubConnectionFlow::InitialiseDataMonitoringL(CIPShimIfBase* intf)
	{
	const TPacketActivity* packetActivity =
		static_cast<const TPacketActivity*>(AccessPointConfig().FindExtension(TPacketActivity::TypeId()));
	if (packetActivity)
		{
		intf->ShimNotify()->SetPacketActivityFlag(packetActivity->iPacketActivity);
		}
	else
		{
		__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("Packet activity provisioning information missing")));
    	User::Leave(KErrNotReady);
		}

	const TDataMonitoringConnProvisioningInfo* connProvisioningInfo =
		static_cast<const TDataMonitoringConnProvisioningInfo*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TDataMonitoringConnProvisioningInfo::iUid, TDataMonitoringConnProvisioningInfo::iId)));

	const TDataMonitoringSubConnProvisioningInfo* subConnProvisioningInfo =
		static_cast<const TDataMonitoringSubConnProvisioningInfo*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TDataMonitoringSubConnProvisioningInfo::iUid, TDataMonitoringSubConnProvisioningInfo::iId)));

    if(connProvisioningInfo && subConnProvisioningInfo)
    	{
		intf->ShimNotify()->SetDataVolumePtrs(connProvisioningInfo->iDataVolumesPtr, subConnProvisioningInfo->iDataVolumesPtr);
		intf->ShimNotify()->SetNotificationThresholdPtrs(connProvisioningInfo->iThresholdsPtr, subConnProvisioningInfo->iThresholdsPtr);
    	}
    else
    	{
		__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("Data monitoring provisioning information missing")));
    	User::Leave(KErrNotReady);
    	}
	}


// ============================================================================
//
// from Messages::ANode

void CIPShimSubConnectionFlow::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
/**
Receive function for incoming messages from SCPR.

@param aCFMessage message from SCPR
*/
    {
	CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);
	if(aMessage.IsMessage<TEBase::TCancel>())
		{
		if (iStarting )
			{
			StopFlow(KErrCancel);
			}
		}
	else if(aMessage.IsMessage<TEChild::TDestroy>())
		{
		Destroy();
		}
	else if(aMessage.IsMessage<TCFDataClient::TBindTo>())
		{
		TCFDataClient::TBindTo& msg = message_cast<TCFDataClient::TBindTo>(aMessage);
		BindToL(msg.iNodeId);
		ASSERT(iSubConnectionProvider == aSender);
		iSubConnectionProvider.PostMessage(Id(), TCFDataClient::TBindToComplete().CRef());
		}
	else if(aMessage.IsMessage<TCFDataClient::TStart>())
		{
		StartFlowL();
		}
	else if(aMessage.IsMessage<TCFDataClient::TStop>())
		{
		StopFlow(static_cast<TCFDataClient::TStop&>(aMessage).iValue);
		}
	else if(aMessage.IsMessage<TCFDataClient::TProvisionConfig>())
		{
		iAccessPointConfig.Close();		
		iAccessPointConfig.Open(static_cast<TCFDataClient::TProvisionConfig&>(aMessage).iConfig);
		ProcessProvisionConfigL();
		}
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	else if(aMessage.IsMessage<TCFScpr::TSetParamsRequest>())
		{
		UpdateIpAddressInfoL(static_cast<TCFScpr::TSetParamsRequest&>(aMessage));
		}
#else
	else if(aMessage.IsMessage<TCFScpr::TParamsRequest>())
		{
		UpdateIpAddressInfoL(static_cast<TCFScpr::TParamsRequest&>(aMessage));
		}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	else
		{
		Panic(EUnexpectedSubConnectionMsg);
		}
    }


//
// Dispatch functions for messages from SCPR
//

__CFLOG_STMT
(
void LOG_ADDRESS(TSockAddr addr)
	{
	TBuf<KMaxSockAddrSize> buff;
	TInetAddr iaddr(addr);
	iaddr.Output(buff);

	__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L("CIPShimSubConnectionFlow::UpdateIpAddressInfoL %S %d"), &buff, addr.Port()));
	}
)

//helper function
CIPProtoBinder * FindBinderForProtocol(const TDesC8& proto, RPointerArray<CIPProtoBinder> &array)
	{
	TInt count= array.Count();
	for (TInt i = 0; i < count; i++)
		{
		if (array[i]->ProtocolName().Compare(proto) == 0)
			{
			return array[i];
			}
		}
	return 0;
	}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
void CIPShimSubConnectionFlow::UpdateIpAddressInfoL(TCFScpr::TSetParamsRequest& aParamReq)
#else
void CIPShimSubConnectionFlow::UpdateIpAddressInfoL(TCFScpr::TParamsRequest& aParamReq)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	{

	//[401TODO] DL: deal with the granted also.
	const RCFParameterFamilyBundleC& newParamBundle = aParamReq.iFamilyBundle;

	CSubConIPAddressInfoParamSet* IPAddressInfoSet;
	RParameterFamily family = newParamBundle.FindFamily(KSubConIPAddressInfoFamily);
	if ( family.IsNull() )
		{
		 IPAddressInfoSet = NULL;
		}
    else
    	{
    	 IPAddressInfoSet = static_cast<CSubConIPAddressInfoParamSet*>
		 	(family.FindParameterSet(
		 			STypeId::CreateSTypeId(CSubConIPAddressInfoParamSet::EUid,
				 		CSubConIPAddressInfoParamSet::ETypeId)
						 ,RParameterFamily::ERequested));
		}

	if (IPAddressInfoSet)
        {
		TUint aParamSetNum = IPAddressInfoSet->GetParamNum();
		/** TODO: It should be only one in the queue, so remove the loop */
		for (TUint i=0; i<aParamSetNum; ++i)
			{

			CSubConIPAddressInfoParamSet::TSubConIPAddressInfo paramInfo(IPAddressInfoSet->GetParamInfoL(i));

			//The only protocol we are going to deal with is TCP/IP
			//as the flow association gets lost in when it goes through the current stack.
			if (paramInfo.iCliDstAddr.Family() == KAfInet6 ||
				paramInfo.iCliDstAddr.Family() == KAfInet)
				{
				__CFLOG_STMT(LOG_ADDRESS(paramInfo.iCliDstAddr);)
				__CFLOG_STMT(LOG_ADDRESS(paramInfo.iCliSrcAddr);)

				TInetAddr addr(paramInfo.iCliDstAddr);
				TUint32 ipv4Addr = addr.Address();

				if (iBinderList.Count())
					{
					CIPShimIfBase* nif = iBinderList[0]->iNif;
					ASSERT(nif);
					CIPProtoBinder *binder = 0;
					if(ipv4Addr)
						{
						//find ip4 binder
						binder = FindBinderForProtocol(KProtocolIp, iBinderList);
						}
					else
						{
						//find ip6 binder
						binder = FindBinderForProtocol(KProtocolIp6, iBinderList);
						}

					if (IPAddressInfoSet->GetOperationCode() == CSubConIPAddressInfoParamSet::EDelete)
						{
						nif->RemoveIpAddrInfo(binder, paramInfo);
						}
					else
						{
						nif->AddIpAddrInfoL(binder, paramInfo);
						}
					}
				}
			}
        }
	}


class TCleanupNifPair {
public:
	TCleanupNifPair(CIPShimIfBase* aNif) : iNif(aNif), iBinder(NULL) {}

	void Cleanup();
	static void CleanupReleaseNif(TAny* aThis);

	CIPShimIfBase* iNif;
	CIPProtoBinder* iBinder;
	};

void TCleanupNifPair::Cleanup()
	{
	ASSERT(iNif);

	if (iBinder)
		{
		iNif->UnbindFrom(iBinder);
		iBinder->UnbindFromLowerFlow();
		}
	iNif->Release(KErrAbort);
	}

void TCleanupNifPair::CleanupReleaseNif(TAny* aThis)
	{
	TCleanupNifPair* pair = static_cast<TCleanupNifPair*>(aThis);
	pair->Cleanup();
	}

void CIPShimSubConnectionFlow::BindToL(const Messages::TNodeId& aCommsBinder)
	{
	// Assumption that TCommsBinder object contains pointer to Flow
    if (aCommsBinder.IsNull())
     	{
     	User::Leave(KErrNotSupported);
     	}

	ANode& mcfNode = aCommsBinder.Node();
#if !defined(__GCCXML__)
	CSubConnectionFlowBase& flow = mcfnode_cast<CSubConnectionFlowBase>(mcfNode);
#else
	CSubConnectionFlowBase& flow = reinterpret_cast<CSubConnectionFlowBase&>(mcfNode);
#endif

	__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimSubConnectionFlow %08x:\tBindToL(): flow 0x%08x, protocol list '%S'"),
 				this, &flow, &iProtocolList));

	MFlowBinderControl* binderControl = flow.GetBinderControlL();
	ASSERT(binderControl);

	// Loop through all the protocols in the list and create binders for them
	TPtrC8 protoList(iProtocolList);
	TPtrC8 proto;
	TInt pos = KErrNotFound;

	do
		{
		pos = protoList.LocateF(',');
		if (pos == KErrNotFound)
			{
			proto.Set(protoList);
			}
		else
			{
			proto.Set(protoList.Left(pos));
			protoList.Set(protoList.Mid(pos + 1));
			}

		if (proto.Length() > 0)
			{
            CIPProtoBinder* protoBinder = FindBinderForProto(proto); 
            if (!protoBinder)
                {
                protoBinder = CIPProtoBinder::NewL(*this, proto);
                CleanupStack::PushL(protoBinder);

                CIPShimIfBase* nif = ProtocolIntf()->FindOrCreateNifL(proto,
                    reinterpret_cast<const TConnectionInfo&>(*iConnectionInfo.Ptr()));
                TCleanupNifPair cleanuppair(nif);
                CleanupStack::PushL(TCleanupItem(TCleanupNifPair::CleanupReleaseNif, &cleanuppair));

                __CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimSubConnectionFlow %08x:\tBindToL(): nif 0x%08x, binder 0x%08x"), this, nif, protoBinder));
                nif->BindToL(protoBinder);
                cleanuppair.iBinder = protoBinder;
    
                protoBinder->BindToLowerFlowL(*binderControl);
    
                iBinderList.AppendL(protoBinder);

                CleanupStack::Pop(2);

                InitialiseDataMonitoringL(nif);
                }
            else
                {
                InitialiseDataMonitoringL(protoBinder->iNif);
                }
			}
		}
	while (pos != KErrNotFound);

	NM_LOG((KIPProtoTag1, _L8("CIPShimSubConnectionFlow %08x:\tSynchronous call: From=%08x To=%08x Func=BindToL"), this, static_cast<Messages::ANode*>(this), &mcfNode));

	TInt binderCount = iBinderList.Count();
	for (TInt i = 0; i < binderCount; i++)
		{
		iBinderList[i]->StartL();
		}
	}

CIPProtoBinder* CIPShimSubConnectionFlow::FindBinderForProto(const TDesC8& aProtocol)
    {
    TInt i = iBinderList.Count();
    while (--i >= 0)
        {
        if (iBinderList[i]->iProtocolName == aProtocol)
            {
            __CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimSubConnectionFlow %08x:\tFindBinderForProto(): Found existing binder for proto '%S'"),
                this, &aProtocol));
            return iBinderList[i];
            }
        }
    __CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimSubConnectionFlow %08x:\tFindBinderForProto(): No binder for proto '%S'"),
        this, &aProtocol));
    return NULL;
    }

void CIPShimSubConnectionFlow::StartFlowL()
/**
Message from SCPR to start.
*/
	{
	ASSERT(!iStarting);

	iStarting = ETrue;

	PostDataClientStartedIfReady();
	}

void CIPShimSubConnectionFlow::StopFlow(TInt aError)
/**
Message from SCPR to stop.

The aError reason code is stored to be passed to the TCP/IP stack via IfUserInterfaceDown().

@param aError Reason code.
*/
	{
	ASSERT(aError != KErrNone);
	iStopCode = aError;

	for(TInt i =0; i < iBinderList.Count(); i++)
		{
		MarkBinderForClosure(iBinderList[i]);
		}
	CloseMarkedBinders();

	PostDataClientStoppedIfReady();
	}



void CIPShimSubConnectionFlow::ProcessProvisionConfigL()
/**
Message from SCPR providing configuration information.
*/
	{
    const TItfInfoConfigExt* ext = static_cast<const TItfInfoConfigExt*>(AccessPointConfig().FindExtension(
    		STypeId::CreateSTypeId(KIpProtoConfigExtUid, EItfInfoConfigExt)));
    if (ext)
        {
        TInt err = SetConnectionInfo(ext->iConnectionInfo);
		if (err == KErrNone)
			{
			err = SetProtocolList(ext->iProtocolList);
			}
        if (err != KErrNone)
            {
    	    User::Leave(err);
            }
	    }
	}



TInt CIPShimSubConnectionFlow::SetProtocolList(const TDesC8& aProtocolList)
/**
Save a pointer to the protocol list string in the provisioning information
*/
	{
	if (aProtocolList.Length() > 0)
		{
		iProtocolList.Set(aProtocolList);
		return KErrNone;
		}
	else
		{
		return KErrArgument;
		}
	}

void CIPShimSubConnectionFlow::Destroy()
/**
Message from SCPR to self destruct.
*/
	{
	// No-one should be bound to us from above if we are about to disappear.
	ASSERT(iBindCallCount == 0);

	MarkAllBindersForClosure();

	if (iAsyncBinderClose.IsActive())
		{
		// Can't destroy ourselves whilst an asynchronous binder close is pending.
		// Arrange for it to close all binders and initiate the destroy.
		//
		// Note that the async binder close may have been originally invoked with
		// just a single binder marked for closure, and we are now marking them all
		// for closure and requesting a destroy.  This is the main reason for using
		// the "marked for closure" flag - to deal with events that require further
		// binders to be closed whilst the async binder close is still pending.
		iDestroyPending = ETrue;
		}
	else
		{
		// No async binder close pending - close binders and destroy ourselves here.
		CloseMarkedBinders();
		DeleteThisFlow();
		}
	}

//
// Utility functions for sending messages to SCPR
//

void CIPShimSubConnectionFlow::PostDataClientStartedIfReady()
	{
	if (iBinderReady && iStarting)
    	{
    	iSubConnectionProvider.PostMessage(Id(), TCFDataClient::TStarted().CRef());
    	iStarting = EFalse;
    	iStarted = ETrue;
    	}
	}

void CIPShimSubConnectionFlow::PostDataClientStoppedIfReady()
	{
	if (iStopCode != KErrNone)
    	{
    	iSubConnectionProvider.PostMessage(Id(), TCFDataClient::TStopped(iStopCode).CRef());
    	iStopCode = KErrNone;
    	iStarted = EFalse;

		MarkAllBindersForClosure();

		if (!iAsyncBinderClose.IsActive())
			{
			// No async binder close pending - close binders
			CloseMarkedBinders();
			}
       	}
	}

void CIPShimSubConnectionFlow::Error(TInt aError)
	{
    ASSERT(iStarted || iStarting);
    ASSERT(iSubConnectionProvider.IsOpen());
    if (iStarting)
        {
        iSubConnectionProvider.PostMessage(Id(), TEBase::TError(TCFDataClient::TStart::Id(), aError).CRef());
        }
    else
        {
        iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(aError, MNifIfNotify::EDisconnect).CRef());
        }

    iStopCode = aError;
    iStarting = EFalse;
    }

MLowerControl* CIPShimSubConnectionFlow::GetControlL(const TDesC8& aProtocol)
    {
    if (aProtocol.Length())
        {
        User::Leave(KErrNotSupported);
        }
    return this;
    }

MLowerDataSender* CIPShimSubConnectionFlow::BindL(const TDesC8& /*aProtocol*/, MUpperDataReceiver* /*aReceiver*/, MUpperControl* /*aControl*/)
    {
    iBindCallCount++;
    if (iBindCallCount == 1)
    	{
	    iSubConnectionProvider.RNodeInterface::PostMessage(Id(), TCFControlProvider::TActive().CRef());
    	}
    return NULL;
    }

void CIPShimSubConnectionFlow::Unbind( MUpperDataReceiver* /*aReceiver*/, MUpperControl* /*aControl*/)
    {
    iBindCallCount--;
    if (iBindCallCount <= 0)
    	{
    	ASSERT(iBindCallCount == 0);
	    PostClientIdleIfIdle();
    	}
    }

CSubConnectionFlowBase* CIPShimSubConnectionFlow::Flow()
    {
    return this;
    }

void CIPShimSubConnectionFlow::OpenRoute()
	{
	// Post directly to cpr bypassing the scpr purely for efficiency
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimSubConnectionFlow %08x:\tOpenRoute()"), this);

	RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(ECFActivityOpenCloseRoute, iProtocolIntf->ControlProviderId()),
		TCFIPProtoMessage::TOpenCloseRoute(ETrue).CRef());
	}


void CIPShimSubConnectionFlow::CloseRoute()
	{
	// Post directly to cpr bypassing the scpr purely for efficiency
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimSubConnectionFlow %08x:\tCloseRoute()"), this);

	RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(ECFActivityOpenCloseRoute, iProtocolIntf->ControlProviderId()),
		TCFIPProtoMessage::TOpenCloseRoute(EFalse).CRef());
	}

void CIPShimSubConnectionFlow::PostClientIdleIfIdle()
    {
    if (iBindCallCount == 0)
        {
        iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TIdle().CRef());
        }
    }


TInt CIPShimSubConnectionFlow::Control(TUint aLevel, TUint aName, TDes8& aOption)
    {
    TInt err = KErrNotSupported;
    
    //It returns interface info alias TConnectionInfo in a form of an old fashioned descriptor
    //for SetOption
    if (aLevel == (TUint)KSOLProvider && aName == (TUint)KSoConnectionInfo)
        {
	    // Get the connection info from the config if it has not been stored yet in case the
	    // CTransportFlowShim locks the connection info for a host resolver and calls StartSending()
	    // on the host resolver before CIPShimSubConnectionFlow::ProcessProvisionConfigL() is called.
	    // Otherwise, if ProcessProvisionConfigL() is not called in time the CTransportFlowShim will
	    // lock to empty connection info.  ProcessProvisionConfigL() is called when the
	    // CIPShimSubConnectionFlow receives the request to bind to the bearer SCPR which is not
	    // synchronised in any way with the request to bind the CIPShimSubConnectionFlow to the
	    // CTransportFlowShim which triggers the call to StartSending().  We still return
	    // KErrNotReady when we are not bound so that a socket does not try to flow on too early
	    // (this was causing failures in te_spudnetworkside).  A host resolver does not check the
	    // error code when calling KSoConnectionInfo and simply uses the info if it's there anyway.
	    //
	    // See DEF128574.
	   	if(iConnectionInfo.Length() == 0)
	   		{
			const TItfInfoConfigExt* ext = static_cast<const TItfInfoConfigExt*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(KIpProtoConfigExtUid, EItfInfoConfigExt)));
			if (ext)
			    {
				aOption.Copy((const TUint8*)&ext->iConnectionInfo, sizeof(TConnectionInfo));
		   		err = KErrNotReady;
			    }
	   		}
	   	else
	   		{
	        if(iConnectionInfo.Length() == 0)
	        	{
	        	err = KErrNotReady;
	        	}
	        else
	        	{
		        aOption.Copy(iConnectionInfo);
		   		err = KErrNone;
	        	}
	   		}
        }
    
    return err;
    }

TInt CIPShimSubConnectionFlow::SetConnectionInfo(const TConnectionInfo& aInfo)
	{
	iConnectionInfo.Close();
	TInt ret = iConnectionInfo.Create(sizeof(TConnectionInfo));
	if (ret == KErrNone)
	    {
    	iConnectionInfo.Copy((TUint8*)&aInfo, sizeof(TConnectionInfo));
	    }
	return ret;
	}

void CIPShimSubConnectionFlow::BinderReady()
/**
Called from binder to indicate that it is ready.
*/
	{
	iBinderReady = ETrue;
	PostDataClientStartedIfReady();
	}

/**
Called from lower binder to indicate an error that renders the binder unusable

Ensure that the TCP/IP stack is notified and the binder closed down.

@param aError error code
@param aIf CIPShimIfBase object associated with the binder
*/
#ifdef _DEBUG
void CIPShimSubConnectionFlow::BinderError(TInt aError, CIPProtoBinder* aBinder)
#else
void CIPShimSubConnectionFlow::BinderError(TInt /*aError*/, CIPProtoBinder* aBinder)
#endif
	{
	__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimSubConnectionFlow %08x:\tBinderError(%d)"), this, aError));

	// NOTE:
	// - We are currently in the stack frame of the lower binder as it makes the Error() upcall.
	// - Calling CloseBinder() directly here will result in a recursive Unbind() and destructor
	//   downcall back into the binder.  Consequently, schedule asynchronous closure of the binder
	//   to ensure that the binder does not have the burden of dealing with this recursion.
	// - Note that CloseBinder() will also Close() the CIPShimIfBase, which may cause it to be destructed
	//   (if all other references on it are gone, in particular the TCP/IP stack)
	MarkBinderForClosure(aBinder);
	if (!iAsyncBinderClose.IsActive())
		{
		iAsyncBinderClose.Call();
		}
	}


void CIPShimSubConnectionFlow::SendMessageToSubConnProvider(const Messages::TSignatureBase& aCFMessage)
    {
    iSubConnectionProvider.PostMessage(Id(), aCFMessage);
    }

const Messages::TNodeId& CIPShimSubConnectionFlow::GetCommsId()
    {
    return NodeId();
    }


// MIpDataMonitoringNotifications

void CIPShimSubConnectionFlow::PostConnDataReceivedThresholdReached(TUint aVolume)
	{
	RClientInterface::OpenPostMessageClose(Id(), iProtocolIntf->ControlProviderId(),
		TCFDataMonitoringNotification::TDataMonitoringNotification(EReceived, aVolume).CRef());
	}

void CIPShimSubConnectionFlow::PostConnDataSentThresholdReached(TUint aVolume)
	{
	RClientInterface::OpenPostMessageClose(Id(), iProtocolIntf->ControlProviderId(),
		TCFDataMonitoringNotification::TDataMonitoringNotification(ESent, aVolume).CRef());
	}

void CIPShimSubConnectionFlow::PostSubConnDataReceivedThresholdReached(TUint aVolume)
	{
	iSubConnectionProvider.PostMessage(Id(),
		TCFDataMonitoringNotification::TDataMonitoringNotification(EReceived, aVolume).CRef());
	}

void CIPShimSubConnectionFlow::PostSubConnDataSentThresholdReached(TUint aVolume)
	{
	iSubConnectionProvider.PostMessage(Id(),
		TCFDataMonitoringNotification::TDataMonitoringNotification(ESent, aVolume).CRef());
	}

//
// Asynchronous Binder Close routines
//
// Class used to asynchronously unbind/destroy a binder from outside of the binder's call stack.
//

CAsyncBinderClose::CAsyncBinderClose(CIPShimSubConnectionFlow& aFlow)
  : CAsyncOneShot(EPriorityStandard), iFlow(aFlow)
  	{
  	}

void CAsyncBinderClose::RunL()
	{
	iFlow.AsyncBinderClose();
	}

void CIPShimSubConnectionFlow::AsyncBinderClose()
/**
Main method of asynchronous binder close callback.
*/
	{
	CloseMarkedBinders();
	if (iDestroyPending)
		{
		// Deal with a pending Destroy() that happened whilst the asynchronous binder close was pending.
		iDestroyPending = EFalse;
		DeleteThisFlow();
		}
	}

void CIPShimSubConnectionFlow::MarkAllBindersForClosure()
/**
Mark all binders for closure in the asynchronous binder close callback
*/
	{
	for (TInt index = 0 ; index < iBinderList.Count() ; ++index)
		{
		MarkBinderForClosure(iBinderList[index]);
		}
	}

void CIPShimSubConnectionFlow::MarkBinderForClosure(CIPProtoBinder* aBinder)
	{
	__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimSubConnectionFlow %08x:\tMarkBinderForClosure(%08x)"), this, aBinder));
	aBinder->iMarkedForClosure = ETrue;
	}

void CIPShimSubConnectionFlow::CloseMarkedBinders()
	{
	TInt count = iBinderList.Count();
	TBool reset = count > 0 ? ETrue : EFalse;
	for (TInt index = count -1  ; index >= 0; --index)
		{
		CIPProtoBinder* binder = iBinderList[index];
		if (binder->iMarkedForClosure)
			{
			CIPShimIfBase *nif = binder->iNif;
			if (nif) // nif may have unbound itself
				{
				nif->RemoveIpAddrInfo(binder);
				// The removal of interface name has to be extracted from Release() and
				// performed before UnbindFrom() as otherwise Release() would have no CIPProtoBinders
				// to navigate up to the Flow, which is required to remove the interface name.
				nif->RemoveInterfaceName(binder);
				nif->UnbindFrom(binder);
				nif->Release(iStopCode);
				}
			binder->UnbindFromLowerFlow();

			iBinderList.Remove(index);
			delete binder;
			}
		else
			{
			reset = EFalse;
			}
		}

	if (reset)
		{
		// If we've closed all binders in the list, and the list is non-empty, then reset it.
		iBinderList.Reset();
		}
	}

// Utility functions

GLDEF_C void Panic(TIPShimPanic aReason)
	{
	_LIT(KPanicCategory, "IPShim");

	User::Panic(KPanicCategory(), aReason);
	}

// =================================================================================
//
// Flow Factory methods
//

CIPShimFlowFactory* CIPShimFlowFactory::NewL(TAny* aConstructionParameters)
/**
Constructs a Default SubConnection Flow Factory

@param aConstructionParameters construction data passed by ECOM

@returns pointer to a constructed factory
*/
	{
	CIPShimFlowFactory* ptr = new (ELeave) CIPShimFlowFactory(TUid::Uid(KIPShimFlowImplUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	return ptr;
	}

CIPShimFlowFactory::~CIPShimFlowFactory()
	{
	}


CIPShimFlowFactory::CIPShimFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
	: CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
/**
Default SubConnection Flow Factory Constructor

@param aFactoryId ECOM Implementation Id
@param aParentContainer Object Owner
*/
	{
	}


CSubConnectionFlowBase* CIPShimFlowFactory::DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, TFactoryQueryBase& aQuery)
	{
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
	return new (ELeave) CIPShimSubConnectionFlow(*this, query.iSCprId, aProtocolIntf);
	}

CProtocolIntfFactoryBase* CIPShimFlowFactory::CreateProtocolIntfFactoryL(CProtocolIntfFactoryContainer& aParentContainer)
	{
	CProtocolIntfFactoryBase* factory = CIPShimProtocolIntfFactory::NewL(TUid::Uid(KIPShimFlowImplUid),
																		 aParentContainer);
    return factory;
	}

CIPShimProtocolIntf::CIPShimProtocolIntf(CProtocolIntfFactoryBase& aFactory,const Messages::TNodeId& aCprId)
	:CProtocolIntfBase(aFactory,aCprId)
	{
	}

CIPShimProtocolIntf::~CIPShimProtocolIntf()
	{
	// if there is still a nif open, then one of the ipshimflows
	// is not being cleaned up correctly
	ASSERT(iNifs.Count() == 0);
	iNifs.Close();
	}

void CIPShimProtocolIntf::DoFlowCreated(ESock::CSubConnectionFlowBase& /*aFlow*/)
	{
	// do nothing
	}

void CIPShimProtocolIntf::DoFlowBeingDeleted(ESock::CSubConnectionFlowBase& /*aFlow*/)
	{
	// do nothing
	}

CIPShimIfBase* CIPShimProtocolIntf::FindOrCreateNifL(const TDesC8& aProtocol, const TConnectionInfo& aConnectionInfo)
	{
	CIPShimIfBase* shim = NULL;

	for (TInt i = 0; i < iNifs.Count(); i++)
		{
		if (iNifs[i]->ProtocolName().Compare(aProtocol) == 0)
			{
			shim = iNifs[i];
			break;
			}
		}
	if (shim == NULL)
		{
		shim = CIPShimIfBase::NewL(aProtocol, this);
		
		CleanupStack::PushL(shim);
		iNifs.AppendL(shim);
		CleanupStack::Pop(shim);
		
		shim->SetConnectionInfo(aConnectionInfo);
		}

	return shim;
	}

void CIPShimProtocolIntf::NifDisappearing(CIPShimIfBase* aNif)
	{
	TInt index = iNifs.Find(aNif);
	// index can be KErrNotFound if there was an OOM whilst trying to append a CIPShimIfBase to iNifs.
	ASSERT(index >= KErrNotFound);
	if (index >= 0)
		{
		iNifs.Remove(index);
		}
	}

//TNodeCtxId(ECFActivityOpenCloseRoute, ControlProviderId())
void CIPShimProtocolIntf::OpenRoute()
	{
		// Post directly to cpr bypassing the scpr purely for efficiency
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimProtocolIntf %08x:\tOpenRoute()"), this);

	RClientInterface::OpenPostMessageClose(ControlProviderId(), ControlProviderId(),
		TCFIPProtoMessage::TOpenCloseRoute(ETrue).CRef());
	}

//TNodeCtxId(ECFActivityOpenCloseRoute, ControlProviderId())
void CIPShimProtocolIntf::CloseRoute()
	{
	// Post directly to cpr bypassing the scpr purely for efficiency
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimProtocolIntf %08x:\tCloseRoute()"), this);

	RClientInterface::OpenPostMessageClose(ControlProviderId(), ControlProviderId(),
		TCFIPProtoMessage::TOpenCloseRoute(EFalse).CRef());
	}


CIPShimProtocolIntfFactory* CIPShimProtocolIntfFactory::NewL(TUid aFactoryId,
															 ESock::CProtocolIntfFactoryContainer& aParentContainer)
	{
	CIPShimProtocolIntfFactory *fact = new(ELeave) CIPShimProtocolIntfFactory(aFactoryId, aParentContainer);
	CleanupStack::PushL(fact);
	fact->ConstructL();
	CleanupStack::Pop(fact);
	return fact;
	}

CProtocolIntfBase* CIPShimProtocolIntfFactory::DoCreateProtocolIntfL(TFactoryQueryBase& aQuery)
	{
	const TDefaultProtocolIntfFactoryQuery& query = static_cast<const TDefaultProtocolIntfFactoryQuery&>(aQuery);
	return new(ELeave) CIPShimProtocolIntf(*this, query.iCprId);
	}

CIPShimProtocolIntfFactory::CIPShimProtocolIntfFactory(TUid aFactoryId,
													   ESock::CProtocolIntfFactoryContainer& aParentContainer)
	:CProtocolIntfFactoryBase(aFactoryId, aParentContainer)
	{
	}
