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
// CNifIfBase and CProtocolBase shim layer functionality
// 
//

/**
 @file nif.cpp
*/

#include <e32base.h>
#include <networking/qos_if.h>
#include <nifmbuf.h>
#include <es_prot_internal.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_protflow.h>

#include "in_sock.h"
#include "in_iface.h"
#include "in6_if.h"

#include "flow.h"
#include "nif.h"
#include "notify.h"
#include "panic.h"
#include "ItfInfoConfigExt.h"

#include "nif4.h"		// for CIPShimIfBase::NewL()
#include "nif6.h"		// for CIPShimIfBase::NewL()

#include "IPProtoDeMux.h"

#include "../addressinfohook/inc/hookaddrinfo.h"


using namespace ESock;

//
// CIPShimIfBase methods
//

CIPShimIfBase* CIPShimIfBase::NewL(const TDesC8& aProtocol, CIPShimProtocolIntf *aIntf)
	{
	CIPShimIfBase* intf = NULL;

	if (aProtocol.CompareF(KProtocolIp()) == 0)
		{
		intf = CIPShimIfBase4::NewL(aProtocol);
		}
	else
	if (aProtocol.CompareF(KProtocolIp6()) == 0)
		{
		intf = CIPShimIfBase6::NewL(aProtocol);
		}
	else
		{
		// only support "ip" and "ip6" protocol names
		User::Leave(KErrArgument);
		}
	intf->SetProtocolIntf(aIntf);
	
	return intf;
	}


CIPShimIfBase::CIPShimIfBase(const TDesC8& aProtocolName)
	: CNifIfBase(), iBinderReady(EFalse), iProtIntf(NULL)
/**
CIPShimIfBase constructor
*/
	{
	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L8("CIPShimIfBase %08x:\tCIPShimIfBase(aProtocolName '%S')"), this, &aProtocolName);
	iProtocolName.Copy(aProtocolName);
	}

CIPShimIfBase::~CIPShimIfBase()
	{
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\t~CIPShimIfBase()"), this);
	delete iShimNotify;

	for (TInt i = 0; i < iProtoBinders.Count(); i++)
		{
		iProtoBinders[i]->Unbind();
		}
		
	iProtoBinders.Close();

/*	ASSERT(iProtIntf);
	iProtIntf->NifDisappearing(this);*/
	
	/*
	 * it's not legal to assume that this pointer isn't NULL. There can be
	 * OOM conditions in the constructor (which doesn't follow the official
	 * 2 way constructions [in nif4 or nif6]) which can cause not to initialize
	 * this pointer.
	 */
	if (iProtIntf)
		{
		iProtIntf->NifDisappearing(this);
		}
	}

void CIPShimIfBase::ConstructL()
	{
	iShimNotify = CIPShimNotify::NewL(this);
	// Setup the CNifIfBase::iNotify for the benefit of the TCP/IP stack to call us
	iNotify = static_cast<MNifIfNotify*>(iShimNotify);	
	}

void CIPShimIfBase::StartL()
	{
	if (iUpperProtocol)
		{
		return;
		}
		
	TBuf<KMaxProtocolNameSize> protocolName;
	protocolName.Copy(iProtocolName);
	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tStartL(): FindAndLoadProtocolL('%S')"), this, &protocolName);    	
	iUpperProtocol = SocketServExt::FindAndLoadProtocolL(protocolName);
	iUpperProtocol->Open();
	
	
	// Retrieve the MNifIfUser derived object from the upper protocol

	TNifIfUser ifuser;
	TInt err = iUpperProtocol->GetOption(KNifOptLevel, KNifOptGetNifIfUser, ifuser, 0);

	if (err == KErrNone)
		{
		iNifUser = ifuser();

		// Inform the upper protocol of our CNifIfBase derived interface.
		// (the second argument does not appear to be used in current SymbianOS, so pass 0).
		__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tStartL(): IfUserNewInterfaceL()"), this);    	

		CleanupClosePushL(*iUpperProtocol);
		iNifUser->IfUserNewInterfaceL(this, 0);	// Note: increments CNifIfBase reference
		CleanupStack::Pop();

		// Cleanup Note: if you ever add a leaving function after IfUserNewInterfaceL() above,
		// then you'll need to arrange for CleanupSignalNewInterface() to be called here (it is pushed
		// onto the cleanup stack later as there are no subsequent leaving functions in this routine.

		iNifUser->IfUserOpenNetworkLayer();				// Note: increments protocol reference count
		}
	else
		{
		__CFLOG_VAR((KIPProtoTag1, KIPProtoTag2, _L8("CIPShimIfBase %08x:\tStartL(): Error %d issuing KNifOptGetNifIfUser to protocol"), this, err));
		User::Leave(err);
		}
	}

void CIPShimIfBase::BindToL(CIPProtoBinder* aIPProtoBinder)
	{
	aIPProtoBinder->BindL(this);
	iProtoBinders.AppendL(aIPProtoBinder);
	}

void CIPShimIfBase::UnbindFrom(CIPProtoBinder* aIPProtoBinder)
	{
	TInt index = iProtoBinders.Find(aIPProtoBinder);

	// shouldn't try to unbind from a binder we're not bound to
	ASSERT(index >= 0);
	iProtoBinders.Remove(index);
	aIPProtoBinder->Unbind();
	}

//-=========================================
// MUpperDataReceiver methods
//-=========================================        
void CIPShimIfBase::Process(RMBufChain& aData)
	{
	iUpperProtocol->Process(aData, reinterpret_cast<CProtocolBase*>(this));
	}

//-=========================================
// MUpperControl methods
//-=========================================        
void CIPShimIfBase::StartSending()
	{
	if (!iBinderReady)
		{
		// Get config info (for first time)
		__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tStartSending() (first time)"), this);
		GetConfigFirstTime();
		iBinderReady = ETrue;
		}

	if (iUpperProtocol)
		{
		// Call StartSending(...) in the context of "downstream path ready for first time"
		__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tStartSending()"), this);
		iUpperProtocol->StartSending(reinterpret_cast<CProtocolBase*>(this));
		}
	}
	
void CIPShimIfBase::Error(TInt anError)
	{
	iProtoBinders[0]->Error(anError);
	}

#ifdef _DEBUG
void CIPShimIfBase::BindL(TAny* aId)
#else
void CIPShimIfBase::BindL(TAny* /*aId*/)
#endif
/**
Called from upper protocol to pass its CProtocolBase pointer
*/
	{
	//[401TODO] RZ: Is this assumption safe?
	ASSERT(iUpperProtocol == aId);
	
	if (iBinderReady && iUpperProtocol)
		{
		__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tBindL(%08x)"), this, aId);
		iUpperProtocol->StartSending(reinterpret_cast<CProtocolBase*>(this));
		}
	}

void CIPShimIfBase::CleanupInterface(TInt aError)
	{
	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %x:\tCleanupInterface(%d)"), this, aError);    	
	iUpperProtocol->Close();
	iUpperProtocol = NULL;
	
	if(iInterfaceNameRecorded && iProtoBinders.Count())
		{
        XInterfaceNames* itfNames = static_cast<XInterfaceNames*>(
			const_cast<Meta::SMetaData*>(iProtoBinders[0]->Flow().AccessPointConfig().FindExtension(XInterfaceNames::TypeId())));
		itfNames->RemoveInterfaceName(this);
		}

	// IfUserInterfaceDown may delete this object, so create a stack variable for the NifIfUser
	// so CloseNetworkLayer can be called on it.
	MNifIfUser* nifUser = iNifUser; 
	nifUser->IfUserInterfaceDown(aError, this);  	// Note: decrements CNifIfBase reference
	nifUser->IfUserCloseNetworkLayer();				// Note: decrements protocol reference count
	}

void CIPShimIfBase::RemoveInterfaceName(CIPProtoBinder* aBinder)
	{
	ASSERT(aBinder);
	
	if(iInterfaceNameRecorded)
		{
        XInterfaceNames* itfNames = static_cast<XInterfaceNames*>(
			const_cast<Meta::SMetaData*>(aBinder->Flow().AccessPointConfig().FindExtension(XInterfaceNames::TypeId())));
		itfNames->RemoveInterfaceName(this);
		}
	}


void CIPShimIfBase::Release(TInt aError)
	{
	/* CleanupInterface will delete the interface when it is done if refcount is 0 (it will decrement it itself).
	   If iRefCount 0 when release is called, Start must never have been
	   called on this nif.  */
	if (iProtoBinders.Count())
		{
		return;	//can't go away if we still have binders
		}
	   
	if (iRefCount == 0)
		{
		delete this;
		}
	else
		{
		CleanupInterface(aError);
		}
	}

TInt CIPShimIfBase::State()
	{
	return EIfDown;
	}

TInt CIPShimIfBase::ServiceHwAddrControl(TDes8& aOption)
/**
Service KSoIfHardwareAddr calls from upper protocol
*/
	{
	ASSERT(aOption.Length() == sizeof(TSoIfHardwareAddr));
	if (iProtoBinders.Count())
		{
		//[401TODO] RZ: we're probably need fixed array to avoid index changing
		return iProtoBinders[0]->Control(KSOLInterface, KSoIfHardwareAddr, aOption);		
		}
	else
		{
		__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tServiceHwAddrControl(): no binders"), this);
		return KErrNotReady;
		}
	}

TInt CIPShimIfBase::ServiceConnInfo(TDes8& aOption)
/**
Service KSoIfGetConnectionInfo calls from upper protocol
*/
	{
	ASSERT(aOption.Length() == sizeof(TSoIfConnectionInfo));
	TUint8* ptr = const_cast<TUint8*>(aOption.Ptr());
	TSoIfConnectionInfo& returnedInfo = reinterpret_cast<TSoIfConnectionInfo&>(*ptr);

	returnedInfo.iIAPId = iConnectionInfo.iIapId;
	returnedInfo.iNetworkId = iConnectionInfo.iNetId;

	return KErrNone;
	}

TInt CIPShimIfBase::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* aSource)
	{
	__CFLOG_3(KIPProtoTag1, KIPProtoTag2, _L("CIPShimIfBase %08x:\tControl(aLevel 0x%x, aName 0x%x)"), this, aLevel, aName);
	if (aLevel == KSOLInterface)
		{
		switch (aName)
			{
		case KSoIfInfo:
			/* FALLTHROUGH */
		case KSoIfInfo6:
			return ServiceInfoControl(aOption, aName);

		case KSoIfConfig:
			return ServiceConfigControl(aOption);

		case KSoIfHardwareAddr:
			return ServiceHwAddrControl(aOption);

		case KSoIfGetConnectionInfo:
			return ServiceConnInfo(aOption);			
		case 0x734:
			iHookAddressInfo = (CHookAddressInfo*)aSource;
			return KErrNone;
		default:
			break;
			}
		}
		
	if (iProtoBinders.Count())
		{
		//[401TODO] RZ: we're probably need fixed array to avoid index changing
		return iProtoBinders[0]->Control(aLevel, aName, aOption);
		}
	return KErrNotReady;	
	}

void CIPShimIfBase::Info(TNifIfInfo& aInfo) const
	{
	// Fill in the TNifIfInfo::iName field, which is used by TCP/IP
	if (iProtoBinders.Count())
		{
		//[401TODO] RZ: we're probably need fixed array to avoid index changing
		iProtoBinders[0]->GetName(aInfo.iName);

		if(!iInterfaceNameRecorded)
			{
			XInterfaceNames* itfNames = static_cast<XInterfaceNames*>(
				const_cast<Meta::SMetaData*>(iProtoBinders[0]->Flow().AccessPointConfig().FindExtension(XInterfaceNames::TypeId())));
			itfNames->AddInterfaceNameL(this, aInfo.iName);
			iInterfaceNameRecorded = ETrue;
			}
		}	

	//[401TODO] DL: get rid of magic number
	aInfo.iVersion = TVersion(78,96,12453);

	}

TInt CIPShimIfBase::Send(RMBufChain& aPdu, TAny* /*aSource*/)
/**
Called from upper protocol to send a data packet.

@param aPdu packet to send
@param aSource unused
@return 0 if upper protocol should block until receipt of StartSending(), else 1.
*/
	{
	// demultiplex packets into the approriate senders
	const RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPdu);
	CIPProtoBinder *binder = reinterpret_cast<CIPProtoBinder*>(info->iDstAddr.Port());
	
    //[401TODO] This check is there purely because of GuQoS interoperability
    //i.e.: GuQoS colloring packets for the Flow below IPShim. Get rid of
    //this if statement along with GuQoS.
	if (iProtoBinders.Count() == 1 || binder == NULL)
    	{
    	return iProtoBinders[0]->Send(aPdu);
    	}
    else
        {
        return binder->Send(aPdu);
        }
	}

TInt CIPShimIfBase::Notification(TAgentToNifEventType /*aEvent*/, void * /*aInfo*/)
/**
*/
	{
	return KErrNotSupported;
	}

void CIPShimIfBase::SetConnectionInfo(const TConnectionInfo& aConnectionInfo)
/**
Called from the Flow to provision the connection info to us.
*/
	{
	iConnectionInfo = aConnectionInfo;
	}

const TConnectionInfo& CIPShimIfBase::ConnectionInfo()
/**
Called from the Flow to provision the connection info to us.
*/
	{
	return iConnectionInfo;
	}

const TDesC8& CIPShimIfBase::ProtocolName()
/**
Return the associated protocol name.
*/
	{
	ASSERT(iProtocolName.Length());
	return iProtocolName;
	}

	
void CIPShimIfBase::AddIpAddrInfoL(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& aAddrInfo)
	{
	ASSERT(iHookAddressInfo);
	iHookAddressInfo->AddL(aBinder, aAddrInfo);
	}

void CIPShimIfBase::RemoveIpAddrInfo(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& aAddrInfo)
	{
	ASSERT(iHookAddressInfo);
    iHookAddressInfo->Remove(aBinder, aAddrInfo);	    
	}

void CIPShimIfBase::RemoveIpAddrInfo(CIPProtoBinder* aBinder)
	{
	if (iHookAddressInfo)
    	{
	    iHookAddressInfo->Remove(aBinder);
    	}
	}


CIPShimSubConnectionFlow& CIPShimIfBase::Flow()
	{
	ASSERT(iProtoBinders.Count() > 0);
	return iProtoBinders[0]->Flow();
	}
	
CIPShimProtocolIntf* CIPShimIfBase::ProtcolIntf()
	{
	ASSERT(iProtIntf);
	return iProtIntf;
	}
