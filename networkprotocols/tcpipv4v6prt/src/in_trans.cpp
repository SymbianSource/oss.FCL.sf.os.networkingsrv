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
// in_trans.cpp - transport layer protocol base
//

#include "in_trans.h"
#include "inet6log.h"

#include <e32math.h>
#include <comms-infras/nifif_internal.h>

// speed optimisations
#ifdef __ARMCC__
#pragma push
#pragma arm
#endif

#define SYMBIAN_NETWORKING_UPS

CProtocolInet6Transport::CProtocolInet6Transport()
	{
	// Initialize random seed value
	TTime  currentTime;
	currentTime.HomeTime();
	iSeed = currentTime.Int64() + User::TickCount();
	}


void CProtocolInet6Transport::InitL(TDesC& aTag)
	{
	CProtocolInet6Base::InitL(aTag);
	}


void CProtocolInet6Transport::StartL()
	{
	CProtocolInet6Base::StartL();
	}


CProtocolInet6Transport::~CProtocolInet6Transport()
	{
	}


TInt CProtocolInet6Transport::Send(RMBufChain& /*aPacket*/,CProtocolBase* /*aSourceProtocol=NULL*/)
	{
	Panic(EInet6Panic_NotSupported);
	return 0;
	}


//
// Bind request from a protocol above us
//
void CProtocolInet6Transport::BindL(CProtocolBase* /*aProtocol*/, TUint /*aId*/)
	{
	Panic(EInet6Panic_NotSupported);
	}


//
// Find SAP based on end-point addresses.
//
// This routine is rather involved, because it is used for a number of different
// purposes. The routine returns a "best match" socket provider based on source
// and destination socket addresses (IP address & port), or NULL if no mathing
// provider is found (see below for an explanation on how matches are ranked).
//
// The method supports the following forms:
//
//  o LocateProvider(minimum rank, local socket address);  or
//    LocateProvider(minimum rank, local socket address, unspecified socket address);
//
//    This form is used when binding a socket in order to check that the port is
//    not already in use. At least local port must be specified. Local address
//    may be unspecified.  The first provider that matches the local port (and
//    local address, if given) is returned.
//
//  o LocateProvider(minimum rank, local socket address, remote socket address);
//
//    This form is used in order to locate the correct socket provider, when
//    a transport PDU has been received. For this use, the complete local
//    and remote addresses must be specified.
//
//    Another use of this form is to verify the uniqueness of the source/destination
//    socket address pair when creating a new connection. This is only an issue if
//    the source port has been explicitly bound and local address reuse has been enabled.
//    In this case, the destination socket address MUST be unique among all connections
//    terminating in the same local socket address.
//
//    Note: for the latter use, local IP address may be left unspecified. In this case,
//    it matches all addresses. Local port is mandatory.
//
//  The minimum rank parameter specifies the minimum acceptable accuracy of the match.
//  The the match with the highest rank found is returned, or NULL if no match of
//  at least the requested rank is found.
//
//  Matches are ranked as follows (highest rank first):
//
//   EMatchExact or EMatchConnection:
//
//        Both local and remote socket addresses match. Local IP address may be
//        unspecified, if the connection is waiting for a flow to become ready.
//
//        This rank is only relevant if caller specified a remote address.
//        The returned socket provider handles a connected socket.
//
//   EMatchServerSpecAddr:
//
//        Local socket address matches and remote socket address is unspecified.
//
//        This means that the returned socket provider handles a server socket
//        that has been bound to a specific local address.
//
//   EMatchServerUnspecAddr:
//
//        Local port matches. Local IP address is unspecified. Remote socket address
//        is unspecified.
//
//        This means that the returned socket provider handles a server socket
//        that has been bound to an unspecified address.
//
//   EMatchLocalPort:
//
//        Local port must match. Local IP address or remote socket address are not tested.
//
//        This means that the returned socket provider will conflict with an attempted
//        bind() if the KSoReuseAddr socket option is not used.
//
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
CProviderInet6Transport* CProtocolInet6Transport::LocateSap(TProviderMatchEnum aRank, TUint aFamily, 
const TInetAddr& aLocalAddr, const TInetAddr& aRemoteAddr, CProviderInet6Base *aSap)
#else
CProviderInet6Transport* CProtocolInet6Transport::LocateSap(TProviderMatchEnum aRank, TUint aFamily,
		const TInetAddr& aLocalAddr, const TInetAddr& aRemoteAddr, CProviderInet6Base *aSap,
		TUint32 aSourceIfIndex)
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
	{
	ASSERT(aLocalAddr.Port() != KInetPortNone);

	CProviderInet6Transport *sapFound = NULL, *sap;
	TUint port = aLocalAddr.Port();
	TProviderMatchEnum rank = (TProviderMatchEnum)(aRank-1);

	// CProtocolInet6Base::LocateProvider() puts us into the right hashed queue.
	for (sap = (CProviderInet6Transport *)(aSap != NULL ? aSap : CProtocolInet6Base::LocateProvider(port));
			sap != NULL; sap = (CProviderInet6Transport *)sap->iNextSAP)
		{
		// If aSap non-NULL, return first match
		if (aSap != NULL && rank >= aRank)
			break;

		const TInetAddr& local = sap->iFlow.FlowContext()->LocalAddr();
		if (port != local.Port())
			continue;
#ifdef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
        // In case of loopback no need to lock interface
		if(!aLocalAddr.IsLoopback())
		    {	
		    // Port matches this SAP, so if this SAP has an interface that it is
		    // locked to, confirm the inbound packet is on that interface. That is,
		    // if the SAP is locked to an interface, this code ensures that the
		    // packet is only accepted if it is coming over the same interface
		    // that the SAP is locked to.
		    TScopeType scoping = sap->iFlow.FlowContext()->LockType();
		    TUint32 interfaceNum = sap->iFlow.FlowContext()->LockId();
		
		    // Check that the source interface index was supplied. Also check for lock id > 0.
		    // If lock id > 0 this implies that the interface is locked down for this SAP. Also
		    // ignore the magic value lock id of KNetworkIdFromAddress.
		    if ((aSourceIfIndex > 0) && (interfaceNum > 0) && (interfaceNum != KNetworkIdFromAddress))
			    {
			    // The sap currently being checked is locked to an interface. Check to
			    // see what type of lock (scoping) it is.
			    if (scoping == EScopeType_IF)
			   	    {
			   	    // This sap is locked to EScopeType_IF
	                if (interfaceNum != aSourceIfIndex)
	            	    {
	            	    // Not the right interface, go to next SAP
	                    continue;
	                    }
            	    }
			    else 
				    {
				    // Not locked to EScopeType_IF, so now need the network ID and IAP number from the
				    // interface, to check against one of those. Fetch this from the source interface
				    // based on the interface index.
				    const MInterface* iface = sap->iFlow.FlowContext()->Interfacer()->Interface(aSourceIfIndex);
				    if (iface)
					    {
					    if (scoping == EScopeType_IAP)
						    {
						    // This sap is locked to EScopeType_IAP. Check if the source interface
						    // is over the same IAP.
						    TUint id = iface->Scope(EScopeType_IAP);
						    if (id != interfaceNum)
							    {
							    // IAP does not match that required
							    continue;  
							    } 							
						    }
					    else if (scoping == EScopeType_NET )
						    { 
						    // This sap is locked to EScopeType_NET. Check if the source interface
						    // is over the same network ID.
						    TUint id = iface->Scope(EScopeType_NET);
						    if (id != interfaceNum) 
							    {
							    // Network ID does not match that required
							    continue;    
							    }
						    }
					    }
				    }
			    }
			}
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
		
		const CFlowContext *const flow = sap->iFlow.FlowContext();

		TBool localMatch = aLocalAddr.CmpAddr(local) &&
			(aLocalAddr.Scope() == 0 || aLocalAddr.Scope() == local.Scope());
		if (!sap->iSockFlags.iAddressSet || localMatch)
			{
			if (sap->iSockFlags.iConnected)
				{
				if (localMatch && aRemoteAddr.CmpAddr(flow->RemoteAddr())
					&& (aRemoteAddr.Scope() == 0 || aRemoteAddr.Scope() == flow->RemoteAddr().Scope()))
					{
					rank = EMatchExact;
					sapFound = sap;
					break;
					}
				}
			else
				{
				if (rank >= EMatchServerSpecAddr)
					continue;

				if (localMatch)
					{
					rank = EMatchServerSpecAddr;
					sapFound = sap;
					continue;
					}

				if (rank >= EMatchServerUnspecAddr)
					continue;

				if (sap->iSockFamily == aFamily || sap->iSockFamily == KAFUnspec || aFamily == KAFUnspec)
					{
					rank = EMatchServerUnspecAddr;
					sapFound = sap;
					continue;
					}
				}
			}

		if (rank >= EMatchLocalPort)
			continue;

		rank = EMatchLocalPort;
		sapFound = sap;
		}

#ifdef _LOG
	TBuf<50> src, dst;
	//TBuf<50> src2, dst2;
	aLocalAddr.OutputWithScope(src);
	aRemoteAddr.OutputWithScope(dst);
	Log::Printf(_L("\t%S LocateProvider({%S,%d} <--> {%S,%d})"),
		&ProtocolName(), &src, port, &dst, aRemoteAddr.Port());
	if (sapFound)
		{
		TBuf<50> src2, dst2;
		sapFound->iFlow.FlowContext()->LocalAddr().OutputWithScope(src2);
		sapFound->iFlow.FlowContext()->RemoteAddr().OutputWithScope(dst2);
		Log::Printf(_L("\t%S SAP[%u] found {%S,%d} <--> {%S,%d}, rank=%d, family=%x"),
			&ProtocolName(), (TInt)sapFound,
			&src2, port, &dst2, sapFound->iFlow.FlowContext()->RemotePort(),
			rank, aFamily);
		}
	else
		{
		Log::Printf(_L("\t%S SAP not found, best rank=%d, family=%x"), &ProtocolName(), rank, aFamily);
		}
#endif

	return (CProviderInet6Transport *)sapFound;
	}

#ifdef _LOG
#include "tcp.h"
void CProtocolInet6Transport::LogProviders(TUint aPort)
	{
	CProviderInet6Base *sap;
	TBool title = EFalse;

	for (sap = CProtocolInet6Base::LocateProvider(aPort); sap != NULL; sap = sap->iNextSAP)
		{
		if (aPort != sap->iFlow.FlowContext()->LocalPort())
			continue;

		if (!title)
			{
			title = ETrue;
			Log::Printf(_L("Active TCBs on %S port %d:"), &ProtocolName(), aPort);
			}

		TBuf<50> src, dst;
		sap->iFlow.FlowContext()->LocalAddr().OutputWithScope(src);
		sap->iFlow.FlowContext()->RemoteAddr().OutputWithScope(dst);
		Log::Printf(_L("\t%S SAP[%u] {%S,%d} <--> {%S,%d} [reuse=%d]   %s"),
			&ProtocolName(), (TInt)sap,
			&src, sap->iFlow.FlowContext()->LocalPort(),
			&dst, sap->iFlow.FlowContext()->RemotePort(),
			((CProviderInet6Transport*)sap)->iSockFlags.iReuse,
			(sap->iFlow.FlowContext()->Protocol() == KProtocolInetTcp) ?
			((CProviderTCP6*)sap)->TcpState() : _S("UDP"));
		}
	}
#endif


//
// Pick a free port by random from the range [32768,61000].
// Random ephemeral port selection will help counter certain
// types of attacks.
//
TUint CProtocolInet6Transport::AssignAutoPort()
	{
	TUint i, port = KInetMinAutoPort + Random(KInetMaxAutoPort - KInetMinAutoPort + 1);

	for (i = KInetMinAutoPort; i <= KInetMaxAutoPort; i++)
		{
		if (CProtocolInet6Base::LocateProvider(port) == NULL)
			return port;
		if (++port > KInetMaxAutoPort)
			port = KInetMinAutoPort;
		}
	return KInetPortNone;
	}


TInt CProtocolInet6Transport::GetIniValue(	const TDesC &aSection,
											const TDesC &aName,
											TInt aDefault,
											TInt aMin,
											TInt aMax,
											TBool aBoundMode) const
/**
Reads integer value for ini parameters from tcpip.ini file.

@param aSection		Section under which ini parameter belongs ([xxx] tag).
@param aName		Name of the ini parameter.
@param aDefault		Default value for the parameter, if not given in ini file.
@param aMin			Minimum limit for the parameter value.
@param aMax			Maximum limit for the parameter value.
@param aBoundMode	If ETrue, values that exceed min or max are truncated back to the min/max
					boundary. If EFalse, exceeding values are considered invalid, and default
					is used for those parameters.

@return	Integer value for the parameter.
*/											
	{
	LOG(_LIT(KFormat, "\t[%S] %S = %d"));
	LOG(_LIT(KFormatInv, "\t[%S] %S = %d is invalid"));

	TInt value;
	if (!Interfacer()->FindVar(aSection, aName, value))
		{
		value = aDefault;
		}
	else
		{		
		if (value < aMin)
			{
			LOG(Log::Printf(KFormatInv, &aSection, &aName, value));
			value = aBoundMode ? aMin : aDefault;
			}
		else if (value > aMax)
			{
			LOG(Log::Printf(KFormatInv, &aSection, &aName, value));
			value = aBoundMode ? aMax : aDefault;
			}
		}
	LOG(Log::Printf(KFormat, &aSection, &aName, value));
	return value;
	}


//
// Transport Layer Socket Providers
//

CProviderInet6Transport::CProviderInet6Transport(CProtocolInet6Base *aProtocol)
	: CProviderInet6Base(aProtocol)
	{
	iSockFlags.iRecvClose       = EFalse;
	iSockFlags.iSendClose       = EFalse;
	iSockFlags.iConnected       = EFalse;
	iSockFlags.iFlowStopped     = EFalse;
	iSockFlags.iNotify          = ETrue;
	iSockFlags.iReuse           = EFalse;
	iSockFlags.iAttached        = ETrue;
//	iSockFlags.iRawMode         = EFalse;
//	iSockFlags.iHeaderIncluded  = EFalse;
	iSockFamily                 = KAFUnspec;
	iAppFamily                  = KAFUnspec;
	}

CProviderInet6Transport::~CProviderInet6Transport()
	{
	}


void CProviderInet6Transport::LocalName(TSockAddr &aAddr) const
	{
	aAddr = iFlow.FlowContext()->LocalAddr();
	if (iAppFamily == KAfInet)
		TInetAddr::Cast(aAddr).ConvertToV4();
	}


TInt CProviderInet6Transport::SetLocalName(TSockAddr &aAddr)
	{
#ifdef _LOG
	TBuf<50> a; TInetAddr::Cast(aAddr).OutputWithScope(a);
	LOG(Log::Printf(_L("\t%S SAP[%u] SetLocalName({%S,%d}) [reuse=%d]"),
		&ProtocolName(), (TInt)this,
		&a, aAddr.Port(), iSockFlags.iReuse));
#endif

	LOG(((CProtocolInet6Transport*)iProtocol)->LogProviders(aAddr.Port()));

	TInetAddr addr = aAddr;

	// Bound already?
	if (iFlow.FlowContext()->LocalPort() != KInetPortNone)
		return KErrAlreadyExists;

	// Convert an IPv4 address to a V4 Mapped IPv6 address
	if (addr.Family() == KAfInet)
		addr.ConvertToV4Mapped();

	// Check source address
	if (addr.Family() != KAFUnspec)
		{
		iSockFamily = addr.IsV4Mapped() ? KAfInet : KAfInet6;
		if (!addr.IsUnspecified())
			{
			if (addr.Scope() == 0)
				{
				// Check address and pick a default scope id
				addr.SetScope(iProtocol->Interfacer()->LocalScope(addr.Ip6Address(),
					iFlow.FlowContext()->LockId(), iFlow.FlowContext()->LockType()));
				if (addr.Scope() == 0)
					return KErrNotFound;
				}
			else
				{
				// Check scoped address
				if (addr.Scope() != iProtocol->Interfacer()->LocalScope(addr.Ip6Address(),
					addr.Scope(), (TScopeType)(addr.Ip6Address().Scope() - 1)))
					return KErrNotFound;
				}
			}
		}

	// Autobind requested?
	if (addr.Port() == KInetPortNone)
		{
		addr.SetPort(((CProtocolInet6Transport*)iProtocol)->AssignAutoPort());
		if (addr.Port() == KInetPortNone)
			return KErrInUse;
		}
	else
		{
		if (addr.Port() > 65535)
			return KErrTooBig;

		if (!iSockFlags.iReuse && Protocol()->LocateSap(EMatchLocalPort, aAddr.Family(), addr))
			return KErrInUse;
		}

	iAppFamily = aAddr.Family();  // aAddr here to get the original family
	CProviderInet6Base::SetLocalName(addr);
	iSockFlags.iAddressSet = iFlow.FlowContext()->IsLocalSet();
	iProtocol->BindProvider(this);
	return KErrNone;
	}

void CProviderInet6Transport::RemName(TSockAddr &aAddr) const
	{
	aAddr = iFlow.FlowContext()->RemoteAddr();
	if (iAppFamily == KAfInet)
		TInetAddr::Cast(aAddr).ConvertToV4();
	}


TInt CProviderInet6Transport::SetRemName(TSockAddr &aAddr)
	{
	//__ASSERT_DEBUG(!iSockFlags.iConnected && iFlow.FlowContext()->RemoteAddr().IsUnspecified() && iFlow.FlowContext()->RemotePort() == KInetPortNone,
	//		 Panic(EInet6Panic_NotSupported));
	TInetAddr addr = aAddr; 
	
#ifdef _LOG
	TBuf<50> a; addr.OutputWithScope(a);
	LOG(Log::Printf(_L("\t%S SAP[%u] SetRemName({%S,%d})"), &ProtocolName(), (TInt)this, &a, addr.Port());)
#endif
	TUint port = addr.Port(); 
	if (port < 1)
		return KErrGeneral;
	else if (port > 65535)
		return KErrTooBig;

	if (addr.IsUnspecified())
		return KErrBadName;
	
	if (addr.Family() == KAfInet)
		addr.ConvertToV4Mapped();
	
	TInt family = addr.IsV4Mapped() ? KAfInet : KAfInet6;

	if(iSockFamily == KAFUnspec)
		iSockFamily = family;
	else if (iSockFamily != family)
		return KErrBadName;

	iFlow.SetRemoteAddr(addr);
	iAppFamily = aAddr.Family();

	iSockFlags.iConnected = ETrue;
	return KErrNone;
	}


void CProviderInet6Transport::AutoBind()
	{
	//__ASSERT_DEBUG(iFlow.FlowContext()->LocalPort()==KInetPortNone, Panic(EInet6Panic_BadBind));
	TInetAddr localAddr;

	// Socket is already bound?
	if (iFlow.FlowContext()->LocalPort()!=KInetPortNone)
		return;

	localAddr.SetPort(((CProtocolInet6Transport *)iProtocol)->AssignAutoPort());
	if (localAddr.Port()==KInetPortNone)
		{
		Error(KErrInUse);
		return;
		}

	iFlow.SetLocalAddr(localAddr);

#ifdef _LOG
	TBuf<50> addr; iFlow.FlowContext()->LocalAddr().OutputWithScope(addr);
	Log::Printf(_L("\t%S SAP[%u] AutoBind() --> {%S,%d}"), &ProtocolName(), (TInt)this, &addr, iFlow.FlowContext()->LocalPort());
#endif

	iProtocol->BindProvider(this);
	}

TInt CProviderInet6Transport::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	{
	TInt ret = KErrNotSupported;

	if (aLevel == KSolInetIp && aName == KSoReuseAddr)
		{
		TInt intValue;
		ret = GetOptionInt(aOption, intValue);
		if (ret == KErrNone)
			iSockFlags.iReuse = intValue ? TRUE : FALSE;
		#ifdef _LOG 
      	  Log::Printf(_L("SetOpt\t%S SAP[%u] KSoReuseAddr = %d err=%d"), 
      		           &ProtocolName(), (TInt)this, (TInt)iSockFlags.iReuse, ret); 
        #endif 
		}

	if (ret == KErrNotSupported)
		ret = CProviderInet6Base::SetOption(aLevel, aName, aOption);
	
#ifdef SYMBIAN_NETWORKING_UPS	
	if (ret == KErrNone && aLevel == KSOLProvider && aName == (TUint) KSoConnectionInfo)
		{
		iConnectionInfoReceived = 1;
		}
#endif	
	
	return ret;
	}


TInt CProviderInet6Transport::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	{
	TInt ret = KErrNotSupported;

	if (aLevel == KSolInetIp && aName == KSoReuseAddr)
		ret = SetOptionInt(aOption, iSockFlags.iReuse);

	if (ret == KErrNotSupported)
		ret = CProviderInet6Base::GetOption(aLevel, aName, aOption);

	return ret;
	}

void CProviderInet6Transport::CanSend()
	{
	LOG(Log::Printf(_L("\t%S SAP[%u] CanSend()"), &ProtocolName(), (TInt)this));
	// Do not enable data flow if the protocol doesn't want it.
	if(!iSockFlags.iFlowStopped && iSockFlags.iNotify)
		CProviderInet6Base::CanSend();
	}

void CProviderInet6Transport::Error(TInt aError, TUint aOperationMask)
	{
	if (iSockFlags.iNotify)
		CProviderInet6Base::Error(aError, aOperationMask);
	if (FatalState())
		iSockFlags.iNotify = EFalse;
	}

void CProviderInet6Transport::IcmpError(TInt aError, TUint aOperationMask, TInt aType, TInt aCode,
		const TInetAddr& aSrcAddr, const TInetAddr& aDstAddr, const TInetAddr& aErrAddr)
	{
	CProviderInet6Base::SaveIcmpError(aType, aCode, aSrcAddr, aDstAddr, aErrAddr);
	if (iAppFamily == KAfInet)
		{
		iLastError.iSrcAddr.ConvertToV4();
		iLastError.iDstAddr.ConvertToV4();
		iLastError.iErrAddr.ConvertToV4();
		}

	if (iSockFlags.iReportIcmp && aError != KErrNone)
		Error(aError, aOperationMask);
	}

#ifdef SYMBIAN_NETWORKING_UPS
TBool CProviderInet6Transport::ConnectionInfoSet()
	{
	return static_cast<TBool>(iConnectionInfoReceived);
	}


TBool CProviderInet6Transport::HasSocket()	
	{
	if (iSocket == NULL)
		{
	   	return EFalse;
	    }
	else
		{
		return ETrue;	
		}	
	}

void *CProviderInet6Transport::GetApiL(const TDesC8& aApiName, TUint* aVersion)
	{
	if (aApiName == _L8("MProviderBindings"))
		{
	    return EXPORT_API_L(MProviderBindings, static_cast<MProviderBindings*>(this), aVersion);
		}

	return NULL;	
	}
#endif

void RMBufSockQ::AppendL(const TDesC8& aData, TInt aLen)
	{
	if (aLen <= 0)
		return;

	RMBufChain seg;
	seg.AllocL(aLen);
	seg.CopyIn(aData);
	RMBufChain::Append(seg);
	}

TInt RMBufSockQ::AppendDes(const TDesC8& aData, TInt aLen)
	{
	if (aLen <= 0)
		return KErrNone;

	RMBufChain seg;
	TInt err = seg.Alloc(aLen);
	if(err == KErrNone)
		{
		seg.CopyIn(aData);
		RMBufChain::Append(seg);
		}
	return err;
	}


//
// Append at least aLength bytes from aChain.
// Leave the remainder in aChain. Return number
// of bytes removed from aChain.
//
TInt RMBufSockQ::AppendAtLeast(RMBufChain& aChain, TInt aLength)
	{
	TInt o, n;
	RMBuf* m, *p;

	if (!aChain.Goto(aLength, m, o, n, p))
		{
		aLength = aChain.Length();
		RMBufChain::Append(aChain);
		return aLength;
		}

	if (o != m->Offset())
		{
		p = m;
		m = m->Next();
		aLength += n;
		}

	p->Unlink();
	RMBufChain::Append(aChain);
	aChain = m;

	return aLength;
	}

//
// Remove at most aLength bytes from the queue.
// Place the result in aChain. Return number
// of bytes removed from the queue.
//
TInt RMBufSockQ::RemoveAtMost(RMBufChain& aChain, TInt aLength)
	{
	TInt o, n;
	RMBuf* m, *p;

	if (aLength <= 0 || iNext == NULL)
		return 0;

	if (!Goto(aLength, m, o, n, p))
		{
		aChain = iNext;
		iNext = NULL;
		return aChain.Length();
		}
	if(p != NULL)
	    p->Unlink();
	aChain = iNext;
	iNext = m;

	return aLength + m->Offset() - o;
	}

//
// Copy aLength bytes from aQueue at aOffset
//
void TDualBufPtr::CopyInL(const RMBufChain& aQueue, TInt aOffset, TInt aLength)
	{
	switch(iType)
		{
	case ERMBufChain:
		if (aLength > 0)
			aQueue.CopyL(*iChain, aOffset, aLength);
		break;

	case EDes8:
		iDesc->SetLength(aLength);
		if (aLength > 0)
			aQueue.CopyOut(*iDesc, aOffset);
		break;

	default:
		Panic(EInet6Panic_NotSupported);
		}
	}

//
// Copy aLength bytes from aQueue at aOffset
//
TInt TDualBufPtr::CopyIn(const RMBufChain& aQueue, TInt aOffset, TInt aLength)
	{
	switch(iType)
		{
	case ERMBufChain:
		if (aLength > 0)
			return aQueue.Copy(*iChain, aOffset, aLength);
		break;

	case EDes8:
		iDesc->SetLength(aLength);
		if (aLength > 0)
			aQueue.CopyOut(*iDesc, aOffset);
		break;

	default:
		Panic(EInet6Panic_NotSupported);
		}
	return KErrNone;
	}

//
// Copy into aChain at aOffset
//
void TDualBufPtr::CopyOut(RMBufChain& aChain, TInt aOffset) const
	{
	switch(iType)
		{
	case EDes8:
	case EDesC8:
		aChain.CopyIn(*iDesc, aOffset);
		break;

	default:
		Panic(EInet6Panic_NotSupported);
		}
	}

//
// Remove aLength bytes from the beginning of aQueue.
//
TInt TDualBufPtr::Consume(RMBufChain& aQueue, TInt aLength, RMBufAllocator& aAllocator)
	{
	RMBufChain tail;
	TInt err;

	switch(iType)
		{
	case ERMBufChain:
		if (aLength > 0)
			{
			err = aQueue.Split(aLength, tail, aAllocator);
			if (err == KErrNone)
				{
				iChain->Assign(aQueue);
				aQueue.Assign(tail);
				}
			else
				{
				aLength = RMBufSockQ::Cast(aQueue).RemoveAtMost(*iChain, aLength);
				LOG(Log::Printf(_L("TDualBufPtr::Consume(): Alloc problem. Returning less")));
				}
			}
		break;

	case EDes8:
		iDesc->SetLength(aLength);
		if (aLength > 0)
			{
			aQueue.CopyOut(*iDesc);
			aQueue.TrimStart(aLength);
			}
		break;

	default:
		Panic(EInet6Panic_NoData);
		}
	return aLength;
	}

//
// Append at least aLength bytes to aQueue
//
TInt TDualBufPtr::AppendL(RMBufChain& aQueue, TInt aLength)
	{
	switch(iType)
		{
	case ERMBufChain:
		aLength = RMBufSockQ::Cast(aQueue).AppendAtLeast(*iChain, aLength);
		break;

	case EDes8:
	case EDesC8:
		RMBufSockQ::Cast(aQueue).AppendL(*iDesc, aLength);
		break;

	default:
		Panic(EInet6Panic_NoData);
		}

	return aLength;
	}

//
// Append at least aLength bytes to aQueue. Panics if used with des-tyoe
//
TInt TDualBufPtr::Append(RMBufChain& aQueue, TInt aLength)
	{
	switch(iType)
		{
	case ERMBufChain:
		aLength = RMBufSockQ::Cast(aQueue).AppendAtLeast(*iChain, aLength);
		break;

	default:
		Panic(EInet6Panic_NoData);
		}

	return aLength;
	}

//
// Free buffer
//
void TDualBufPtr::Free()
	{
	switch(iType)
		{
	case ERMBufChain:
		iChain->Free();
		break;

	case EDes8:
		iDesc->SetLength(0);
		break;

	default:
		Panic(EInet6Panic_NoData);
		}
	}

#ifdef __ARMCC__
#pragma pop
#endif
