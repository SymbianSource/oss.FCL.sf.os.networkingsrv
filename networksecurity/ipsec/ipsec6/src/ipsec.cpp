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
// ipsec.cpp - IPv6/IPv4 security policy protocol family
// The CProtololFamilybase implementation.
//



/**
 @file ipsec.cpp
*/
#include <nifmbuf.h>
#include "ipsec.h"

class CProtocolFamilyIpsec : public CProtocolFamilyBase
	{
public:
	CProtocolFamilyIpsec();
	~CProtocolFamilyIpsec();
public:	
	TInt Install();
	TInt Remove();
	CProtocolBase *NewProtocolL(TUint aSockType, TUint aProtocol);
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
private:
	};

#if __WINS__ && _DEBUG
int IPSEC_OBJECT_COUNT = 0;
#endif

void Panic(TIpsecPanic aPanic)
	/**
	* Panic the system.
	*
	* Only called in serious environmental (Socket Server) misbehaviour.
	*
	* @param aPanic The panic code.
	*/
	{
	_LIT(KIpsec, "IPSEC");
	
	User::Panic(KIpsec, aPanic);
	}

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase *Install(void); }
EXPORT_C CProtocolFamilyBase *Install()
	{
#if __WINS__ && _DEBUG
	ASSERT(IPSEC_OBJECT_COUNT == 0);
#endif
	return new CProtocolFamilyIpsec;
	}

CProtocolFamilyIpsec::CProtocolFamilyIpsec()
	{
	}


CProtocolFamilyIpsec::~CProtocolFamilyIpsec()
	{
#if __WINS__ && _DEBUG
	// The object count can be validly 1 here, because if the desctructor
	// of the last object can trigger the protocol shutdown, the object count
	// for that object is not yet decremented! (We get here from the destructor
	// of that object!)
	ASSERT(IPSEC_OBJECT_COUNT == 0 || IPSEC_OBJECT_COUNT == 1);
#endif
	}

TInt CProtocolFamilyIpsec::Install()
	{
	// Nothing to initialize at this point.
	return KErrNone;
	}


TInt CProtocolFamilyIpsec::Remove()
	{
	return KErrNone;
	}


TUint CProtocolFamilyIpsec::ProtocolList(TServerProtocolDesc *& aProtocolList)
	/**
	* Return the descriptions of impelemented protocols: SECPOL and PFKEY.
	*/
	{
	// This function should be a leaving fn

	TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[2]; // Esock catches this leave

	// Support SECPOL and PFKEY protocols

	IPSEC::IdentifySecpol(p[0]);
	IPSEC::IdentifyPfkey(p[1]);
	aProtocolList = p;
	return 2;
	}


CProtocolBase* CProtocolFamilyIpsec::NewProtocolL(TUint aSockType,TUint aProtocol)
	/**
	* Return new protocol instance: SECPOL or PFKEY.
	*/
	{
	if (aSockType == KSockRaw)
		{
		if (aProtocol == KProtocolSecpol)
			return IPSEC::NewSecpolL();
		else if (aProtocol == KProtocolKey)
			return IPSEC::NewPfkeyL();
		}
	User::Leave(KErrNotSupported);
	// NOTREACHED
	return NULL;
	}


//
// CProviderIpsecBase functions
//
CProviderIpsecBase::CProviderIpsecBase()
	{
	}

CProviderIpsecBase::~CProviderIpsecBase()
	{
	iRecvQ.Free();			// Release all pending buffers.
	iSAPlink.Deque();		// Dangerous, if not in the list!!!
							// (make sure NewSAPL() adds to the list!)
	}

TInt CProviderIpsecBase::SecurityCheck(MProvdSecurityChecker *aChecker)
	/**
	* Capability check for the IPsec sockets.
	*
	* Both SECPOL and PFKEY sockets require the NetworkControl capability.
	*
	* @param aChecker The policy checker.
	* @returns The result of the policy check.
	*/
	{
	_LIT_SECURITY_POLICY_C1(KPolicyNetworkControl, ECapabilityNetworkControl);
	return aChecker->CheckPolicy(KPolicyNetworkControl, "IPsec SAP");
	}

void CProviderIpsecBase::GetData(TDes8 &aDesc, TUint aOptions, TSockAddr* anAddr)
	/**
	* Return data to the application.
	*
	* This is shared between PFKEY and SECPOL.
	*
	* @param aDesc	The buffer to receive the packet
	* @param aOptions Flags.
	* @param anAddr	The source address of the packet
	*/
	{
	RMBufPacket packet;

	if (!iRecvQ.Remove(packet))
		Panic(EIpsecPanic_NoData);

	const RMBufPktInfo *const info = packet.Unpack();
	packet.CopyOut(aDesc);
	if (anAddr!=NULL)
		{
		*anAddr = info->iSrcAddr;
		}

	if (aOptions & KSockReadPeek)
		{
		packet.Pack();
		iRecvQ.Prepend(packet);
		iSocket->NewData(1);
		}
	else
		{
		iQueueLimit += info->iLength;
		packet.Free();
		}
	}
	
void CProviderIpsecBase::Deliver(RMBufChain& aPacket)
	/**
	* Queue packets for SAP.
	*
	* @param aPacket	The packet
	*/
	{
	if(iListening)
		{	
		iQueueLimit -= RMBufPacketBase::PeekInfoInChain(aPacket)->iLength;
		iRecvQ.Append(aPacket);
		iSocket->NewData(1);
		}
	else
		aPacket.Free();
	}

void CProviderIpsecBase::Shutdown(TCloseType aOption)
	{
	switch(aOption)
		{
		case EStopInput:
			iListening = 0;
			iRecvQ.Free();
			// *FALL THROUGH* to send the Error notify.
		case EStopOutput:
			// IPSEC SAPs do not currently have asynchronous output, all
			// messages are processed as they arrive.
			iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
			break;

		default:
			// After this, the socket will be destroyed.
			iListening = 0;
			if (aOption != EImmediate)
				{
				iSocket->CanClose();
				// *NOTE* No references to any member of this! CanClose
				// may delete this!
				}
		}
	}


// Default implementations for functions, which currently don't have any
// anything special to implement.

void CProviderIpsecBase::ActiveOpen()
	{
	// PFKEY & POLICY are data gram sockets. The Socket Server
	// should never call this function.
	iSocket->Error(KErrNotSupported);
	}

void CProviderIpsecBase::ActiveOpen(const TDesC8&)
	{
	// PFKEY & POLICY are data gram sockets. The Socket Server
	// should never call this function.
	ActiveOpen();
	}

TInt CProviderIpsecBase::PassiveOpen(TUint)
	{
	// PFKEY & POLICY are data gram sockets. The Socket Server
	// should never call this function.
	return KErrNotSupported;
	}

TInt CProviderIpsecBase::PassiveOpen(TUint,const TDesC8&)
	{
	// PFKEY & POLICY are data gram sockets. The Socket Server
	// should never call this function.
	return KErrNotSupported;
	}

void CProviderIpsecBase::Shutdown(TCloseType aOption, const TDesC8& /*aDisconnectionData*/)
	{
	Shutdown(aOption);
	}

void CProviderIpsecBase::AutoBind()
	{
	// PFKEY & POLICY sockets do not use addresses or ports.
	}

void CProviderIpsecBase::LocalName(TSockAddr &) const
	{
	// PFKEY & POLICY sockets do not use addresses or ports.
	}

TInt CProviderIpsecBase::SetLocalName(TSockAddr&)
	{
	// PFKEY & POLICY sockets do not use addresses or ports.
	return KErrNone;
	}

void CProviderIpsecBase::RemName(TSockAddr&) const
	{
	// PFKEY & POLICY sockets do not use addresses or ports.
	}

TInt CProviderIpsecBase::SetRemName(TSockAddr&)
	{
	// PFKEY & POLICY sockets do not use addresses or ports.
	return KErrNone;
	}

TInt CProviderIpsecBase::GetOption(TUint,TUint,TDes8&) const
	{
	return KErrNotSupported;
	}

void CProviderIpsecBase::Ioctl(TUint,TUint,TDes8*)
	{
	// PFKEY & POLICY sockets do not implement any Iocctl
	iSocket->Error(KErrNotSupported);
	}

void CProviderIpsecBase::CancelIoctl(TUint, TUint)
	{
	// PFKEY & POLICY sockets do not implement any Iocctl
	}

TInt CProviderIpsecBase::SetOption(TUint, TUint,const TDesC8 &)
	{
	return KErrNotSupported;
	}
