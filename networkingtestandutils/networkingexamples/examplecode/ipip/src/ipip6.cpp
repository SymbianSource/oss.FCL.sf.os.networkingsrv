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

#include "ipip6.h"
#include "hookdefs.h"

void Panic(TIPIPHookExPanic aPanic)
	{
	User::Panic(_L("IpipExampleHook panic"), aPanic);
	}

LOCAL_D void FillIdentification(TServerProtocolDesc& anEntry);

/**
 *	CFlowHook
 */	
CFlowHook* CFlowHook::NewLC(TPacketHead &aHead, CIpIpActionSpec& aAction, CProtocol_t& aProtocol)
	{
	CFlowHook* self = new(ELeave) CFlowHook(aHead, aAction, aProtocol);
	CleanupStack::PushL(self);
	return self;
	}

CFlowHook::CFlowHook(TPacketHead &aHead, CIpIpActionSpec& aAction, CProtocol_t& aProtocol) 
	: iPacketHead(aHead), iAction(aAction), iProtocol(aProtocol), iAccessCount(1)
	{}

TInt CFlowHook::ReadyL(TPacketHead& aHead)
	{
#if defined(_DEBUG)
	__ASSERT_DEBUG( iPacketHead.iInterfaceIndex == aHead.iInterfaceIndex, Panic(EIpipPanic_BadHeader));
#else
	(void)aHead;
#endif
	return KErrNone;
	}

TInt CFlowHook::ApplyL(RMBufSendPacket& aPacket, RMBufSendInfo& aInfo)
	{
	return iAction.ApplyL(aPacket, aInfo);
	}

void CFlowHook::Open()
	{
	iAccessCount++;
	}

void CFlowHook::Close()
	{
	if (--iAccessCount == 0)
		{
		// remove self from protocol's list
		iProtocol.RemoveFlowFromList(this);
		}
	}

/**
 *	CSap_t
 */
CSap_t::CSap_t(CProtocol_t& aProtocol) : iProtocol(aProtocol)
	{}

void CSap_t::Ioctl(TUint level,TUint name,TDes8* anOption)
	{
	(void)name;
	(void)level;
	iProtocol.Ioctl(*anOption);
	iSocket->IoctlComplete(NULL);
	}

/**
 *	CProtocol_t
 */
CProtocol_t* CProtocol_t::NewL()
	{
	CProtocol_t* self = new(ELeave) CProtocol_t();
	CleanupStack::PushL(self);
	self->CreateL();
	CleanupStack::Pop();
	return self;
	}

void CProtocol_t::CreateL()
	{
	iPolicyHolder = new(ELeave) CPolicyHolder();
	}

CProtocol_t::~CProtocol_t()
	{
	delete iPolicyHolder;
	iHookList.ResetAndDestroy();
	if (iIPProtocol)
		{
		iIPProtocol->Unbind(( CProtocolBase* )this);
		iIPProtocol->Close();
		}
	}

void CProtocol_t::Identify(TServerProtocolDesc* aProtocolDesc)const 
	{
	FillIdentification(*aProtocolDesc);
	}

void CProtocol_t::BindL(CProtocolBase* /*protocol*/, TUint /*id*/)
	{
	// We should not overwrite the existing esk files
	Panic(EIpipPanic_BadBind);
	}

void CProtocol_t::BindToL(CProtocolBase* protocol)
	{
	__ASSERT_DEBUG(this!=protocol, Panic(EIpipPanic_BadBind));

	//Assert that the protocol Binding to us is the IP protocol
	TUint id;
		{
		TServerProtocolDesc info;
		protocol->Identify(&info);
		id = info.iProtocol;
		}

	__ASSERT_DEBUG(id == KProtocolInet6Ip, Panic(EIpipPanic_BadBind));

	if (id == KProtocolInet6Ip)
		{
		iIPProtocol = (CProtocolInet6Binder *)protocol;
		protocol->Open();      // increase ref count

		// Bind to receive incoming packets of type ipip and ipip6
		protocol->BindL( ( CProtocolBase* )this, BindHookFor( KProtocolInet6Ipip ) );
		protocol->BindL( ( CProtocolBase* )this, BindHookFor( KProtocolInetIpip ) );
		// Bind to outgoing flows
		protocol->BindL( ( CProtocolBase* )this, BindFlowHook() );
		}
	else 
		{
		User::Leave(KErrGeneral);
		}
	}

void CProtocol_t::Ioctl(TDesC8& option)
	{
	ASSERT(option.Length() == sizeof(TIPIPPolicyMsg));

	TIPIPPolicyMsg& msg = *(TIPIPPolicyMsg*)(option.Ptr());
	TInt err = KErrNone;
	// if add messgae
	if (msg.iOption == EIPIPAddPolicy)
		{
		// Create a Packet spec
		// Create action spec
		// Create a policy Spec
		// Add policy to the policy holder
		TRAP(err,
			CIPIPPacketSpec* packet = CIPIPPacketSpec::NewLC(msg.iPacket.iSrcAddr, msg.iPacket.iDestAddr);
			CIpIpActionSpec* action = CIpIpActionSpec::NewLC(msg.iAction.iSrcAddr, msg.iAction.iDestAddr);
			iPolicyHolder->AddSpecL(*packet, *action);
			CleanupStack::Pop(2); //packet, action
			);
		}
	// else if delete message
	else if (msg.iOption == EIPIPDeletePolicy)
		{
		// Find the policy that matches this spec.
		// Todo - deal with situation if any flow hooks are using this action spec
		}
		
	if ( (err!=KErrNone) && (err!=KErrAlreadyExists) )
		{
		Panic(EIpipPanic_IoctlFailed);
		}
	}

MFlowHook* CProtocol_t::OpenL(TPacketHead &aHead, CFlowContext *aFlow)
	{
	(void)aFlow;
	
	// Match the address against the stored policy
	TInetAddr src(aHead.ip6.SrcAddr(), aHead.iSrcPort);
	TInetAddr dst(aHead.ip6.DstAddr(), aHead.iDstPort);
	CIPIPPacketSpec packet(src, dst);

 	CIpIpActionSpec *const action = (CIpIpActionSpec*)iPolicyHolder->MatchSpec(packet, EFalse);
	if (action != NULL)
		{
		CFlowHook* hook = CFlowHook::NewLC(aHead, *action, *this);
		User::LeaveIfError(iHookList.Append(hook));
		CleanupStack::Pop();
		return hook;
		}
	return NULL;
	}

// Incoming packet
TInt CProtocol_t::ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	{
	CIPIPPacketSpec packet(aInfo.iSrcAddr, aInfo.iDstAddr);
	CIpIpActionSpec *const action = (CIpIpActionSpec*)iPolicyHolder->MatchSpec(packet, ETrue);

	if (action != NULL)
		{
		return action->ApplyL(aPacket, aInfo);
		}

	return KErrNone;
	}

void CProtocol_t::RemoveFlowFromList(const CFlowHook* aFlow)
	{
	TInt index = iHookList.Find(aFlow);
	if (index != KErrNotFound)
		{
		iHookList.Remove(index);
		}
	else 
		{
		Panic(EIpipPanic_BadIndex);
		}
	}

CServProviderBase* CProtocol_t::NewSAPL(TUint aProtocol)
	{
	(void)aProtocol;
	CSap_t* sap = new(ELeave) CSap_t(*this);

	return sap;
	}

/**
 *	CProtocolFamily_t
 */	
CProtocolFamily_t::CProtocolFamily_t()
	{
	__DECLARE_NAME(_S("CProtocolFamily_t"));
	}

CProtocolFamily_t::~CProtocolFamily_t()
	{
	}

TInt CProtocolFamily_t::Install()
	{
	return KErrNone;
	}

TInt CProtocolFamily_t::Remove()
	{
	return KErrNone;
	}

TUint CProtocolFamily_t::ProtocolList(TServerProtocolDesc *& aProtocolList)
	{
	TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[1]; // Esock catches this leave
	FillIdentification(p[0]);
	aProtocolList = p;
	return 1;
	}

CProtocolBase* CProtocolFamily_t::NewProtocolL(TUint /*aSockType*/,
												   TUint aProtocol)
	{
	if (aProtocol != KProtocolInetIPIPHookEx)
		{
		User::Leave(KErrNotSupported);
		}

	return CProtocol_t::NewL();
	}

void FillIdentification(TServerProtocolDesc& anEntry)
	{
	anEntry.iName=KProtocolIPIPName;
	anEntry.iAddrFamily=KAfInetIPIPHookEx;
	anEntry.iSockType=KSockDatagram;
	anEntry.iProtocol=KProtocolInetIPIPHookEx;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=KSocketNoSecurity;
	anEntry.iMessageSize=0xffff;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=KUnlimitedSockets;
	anEntry.iServiceTypeInfo=ESocketSupport;
	}

//
// Entrypoint
//
GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase* Install(void); }
EXPORT_C CProtocolFamilyBase* Install(void)
	{
	CProtocolFamily_t* protocol = new CProtocolFamily_t();
	if (protocol)
		{
		return protocol;
		}
	else 
		{
		return NULL;
		}
	}
