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
// inet.cpp - inet protocol base
//

#include <in_pkt.h>
#include <ext_hdr.h>
#include <comms-infras/nifif.h>
#include <comms-infras/nifif_internal.h>
#include "inet.h"
#include "networkinfo.h"
#include "inet6log.h"

//
//  CProtocolInet6Base
//
//

CProtocolInet6Base::CProtocolInet6Base()
	{
	// Nothing here -- rely on CBase zero fill!
	}

void CProtocolInet6Base::InitL(TDesC& /*aTag*/)
	{
#ifdef _LOG
	// Fetch the protocol name for logging purposes
	TServerProtocolDesc info;
	Identify(&info);
	iName = info.iName;
#endif
	}

void CProtocolInet6Base::BindL(CProtocolBase* /*aProtocol*/, TUint /*aId*/)
	{
	Panic(EInet6Panic_NotSupported);
	}

void CProtocolInet6Base::Unbind(CProtocolBase* /*aProtocol*/, TUint /*aId*/)
	{
	}

void CProtocolInet6Base::StartL()
	{
	}

//
//	*NOTE*
//		In the INET6 stack the Error upcall is passed up from the drivers
//		but, it does not require any actions by default (doing something
//		here is only useful for protocols modules that want to know about
//		changes in the interfaces and possibly doing there own
//		reconfiguring after this (for example, 6to4).
//
//		In INET6 the interface manager takes care of notifying individual
//		flows about the interface changes that affect them. -- msa
//
//	This here is required, because the base CProtocolBase::Error()
//	calls Panic. A similar consideration applies to the StartSending(),
//	but as the CProtocolBase::StartSending() is already a NOP, nothing
//	needs to be done here.
//
void CProtocolInet6Base::Error(TInt /*anError*/, CProtocolBase* /*aSourceProtocol*/)
	{
	}


CProtocolInet6Base::~CProtocolInet6Base()
	{
	// All SAP's should have been detached from this by now!
	ASSERT(iSapCount == 0);
	}


//
// SAP counting
//
void CProtocolInet6Base::IncSAPs()
	{
	iSapCount++;
	}

void CProtocolInet6Base::DecSAPs()
	{
	ASSERT(iSapCount > 0);
	iSapCount--;
	}

//
// SAP database routines
//
void CProtocolInet6Base::BindProvider(CProviderInet6Base* aSAP)
	{
#ifdef _LOG
	TBuf<50> local; aSAP->iFlow.FlowContext()->LocalAddr().OutputWithScope(local);
	TBuf<50> remote; aSAP->iFlow.FlowContext()->RemoteAddr().OutputWithScope(remote);
	Log::Printf(_L("\t%S SAP[%u] BindProvider({%S,%d} <--> {%S,%d})"),
				&ProtocolName(), (TInt)aSAP,
				&local, aSAP->iFlow.FlowContext()->LocalPort(), &remote, aSAP->iFlow.FlowContext()->RemotePort());
#endif

	TUint key = ProviderHashKey(aSAP->iFlow.FlowContext()->LocalPort());
	aSAP->iNextSAP = iSAP[key];
	iSAP[key] = aSAP;
	}

void CProtocolInet6Base::QueueBindProvider(CProviderInet6Base* aSAP)
	{
#ifdef _LOG
	TBuf<50> local; aSAP->iFlow.FlowContext()->LocalAddr().OutputWithScope(local);
	TBuf<50> remote; aSAP->iFlow.FlowContext()->RemoteAddr().OutputWithScope(remote);
	Log::Printf(_L("\t%S SAP[%u] QueueBindProvider(): {%S,%d} <--> {%S,%d}"),
			&ProtocolName(), (TInt)this,
			&local, aSAP->iFlow.FlowContext()->LocalPort(), &remote, aSAP->iFlow.FlowContext()->RemotePort());
#endif

	TUint key = ProviderHashKey(aSAP->iFlow.FlowContext()->LocalPort());
	CProviderInet6Base **sap = &iSAP[key];
	while (*sap != NULL)
		sap = &(*sap)->iNextSAP;
	*sap = aSAP;
	}

void CProtocolInet6Base::UnbindProvider(CProviderInet6Base* aSAP)
	{
	CProviderInet6Base **sapPtr;
	TUint key;

	if (aSAP->iFlow.FlowContext() == NULL)
		return;

	key = ProviderHashKey(aSAP->iFlow.FlowContext()->LocalPort());
	for (sapPtr = &iSAP[key]; *sapPtr != NULL; sapPtr = &((*sapPtr)->iNextSAP))
		if (*sapPtr == aSAP)
			{
#ifdef _LOG
			TBuf<50> local; aSAP->iFlow.FlowContext()->LocalAddr().OutputWithScope(local);
			TBuf<50> remote; aSAP->iFlow.FlowContext()->RemoteAddr().OutputWithScope(remote);
			Log::Printf(_L("\t%S SAP[%u] UnbindProvider({%S,%d} <--> {%S,%d})"),
					&ProtocolName(), (TInt)aSAP,
					&local, aSAP->iFlow.FlowContext()->LocalPort(), &remote, aSAP->iFlow.FlowContext()->RemotePort());
#endif
			*sapPtr = (*sapPtr)->iNextSAP;
			aSAP->iNextSAP = NULL;
			return;
			}
	}

CProviderInet6Base* CProtocolInet6Base::LocateProvider(TUint aPort)
	{
	CProviderInet6Base *sap;
	TUint key = ProviderHashKey(aPort);

	for (sap = iSAP[key]; sap != NULL; sap = sap->iNextSAP)
		if (sap->iFlow.FlowContext()->LocalPort() == aPort)
			return sap;

	return NULL;
	}

void CProviderInet6Base::InitL()
	{
	TInt err = iFlow.Open(iProtocol->NetworkService());
	if (err != KErrNone)
 		User::Leave(err);

	iFlow.SetNotify(this);

	iLastError.iStatus  = KErrNone;
	iLastError.iErrType = 0;
	iLastError.iErrCode = 0;
	iErrorMask          = 0;
	}

void CProviderInet6Base::Start()
	{
#if 0
  // XXX - Setting socket write buffer to 1024 seems to seriously
  // hamper TCP performance over loopback. Default is 4096 and 2048
  // also works ok. Have to look at this later. -ML
  TVersion v = RNif::Version();
  TUint32 ver = (v.iMajor << 24) | (v.iMinor << 16) | v.iBuild;
  if (ver >= 0x01000044)
	Nif::SetSocketState(ENifBuffers1024, this);
#endif
	}


//
//  CProviderInet6Base
//
//

CProviderInet6Base::CProviderInet6Base(CProtocolInet6Base* aProtocol) : iProtocol(aProtocol)
	{
	LOG(Log::Printf(_L("\t%S SAP[%u] New"), &ProtocolName(), (TInt)this));
	iNextSAP = NULL;
	iProtocol->Open();
	iProtocol->IncSAPs();
	if (iIsUser == 0)
		{
		iProtocol->NetworkService()->IncUsers();
		iIsUser = 1;
		}
	}

CProviderInet6Base::~CProviderInet6Base()
	{
	((CProtocolInet6Base*)iProtocol)->UnbindProvider(this);
	CFlowContext *flow = iFlow.FlowContext();
	if (flow) 
		{
		TPckgBuf< TInt > opt(0);  // Currently there is no use for the option parameter
		flow->SetOption(KSOLProvider, KSoFlowClosing, opt);
		}
	iFlow.Close();
	if (iIsUser == 1)
		{
		iProtocol->NetworkService()->DecUsers();
		iIsUser = 0;
		}
	iProtocol->DecSAPs();
	LOG(Log::Printf(_L("\t%S SAP[%u] Deleted"), &ProtocolName(), (TInt)this));
	iProtocol->Close();
	}


void CProviderInet6Base::ActiveOpen()
	{
	Panic(EInet6Panic_NotSupported);
	}


TInt CProviderInet6Base::PassiveOpen(TUint /*aQueSize*/)
	{
	Panic(EInet6Panic_NotSupported);
	return 0;
	}


void CProviderInet6Base::ActiveOpen(const TDesC8& /*aConnectionData*/)
	{
	Panic(EInet6Panic_NotSupported);
	}


TInt CProviderInet6Base::PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/)
	{
	Panic(EInet6Panic_NotSupported);
	return 0;
	}

void CProviderInet6Base::Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/)
	{
	Panic(EInet6Panic_NotSupported);
	}

// *NOTE*
//	Why are level KSoInetIfCtrl and KSoInetRtCtrl handled at this
//	level? Why not just let iProtocol->SetOption() handle them (as
//	would be logical)? The reason is KSoInetEnum*/KSoInetNext*, which
//	need a SAP specific context, and that is *NOT* provided by the
//	iProtocol->SetOption interface. However, only these options are
//	handled here, rest of the KSoInetRtCtrl/KSoInetIfCtrl are passed
//	to the protocol -- msa
//
TInt CProviderInet6Base::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	{
	if (aLevel == KSolInetIp)
		{
		// Prefetch the option parameter (error return is ignored,
		// if the option is not implemented at this level).
		TInt val;
		const TInt ret = GetOptionInt(aOption, val);

		switch (aName)
			{
		case KSoHeaderIncluded:
		case KSoRawMode:
			// *note* Need to arrange KErrNotSupported for TCP! -- msa

			// Implementation of setting HeaderIncluded and RawMode options
			if (ret != KErrNone)
				return ret;
			// [The raw/headerincluded logic here is unnecessarily convoluted.
			// In EPOC they could be handled independently, as there is no
			// special priviledges required for setting the raw mode either.
			// But, because it is specified to work in this way, sigh ...
			// -- msa]
			//
			// Only allow setting of iHeaderIncluded when iRawMode is set
			// Clearing is always allowed.
			// When iRawMode==0, then *ALWAYS* iHeaderIncluded == 0
			if (aName == KSoRawMode)
				{
				if (val)
					iRawMode = 1;
				else
					{
					iRawMode = 0;
					iHeaderIncluded = 0;
					}
				}
			else if (val == 0)
				iHeaderIncluded = 0;
			else if (iRawMode)
				{
				iHeaderIncluded = 1;
				}
			else
				return KErrNotSupported; // attempted to set KSoHeaderIncluded without Raw mode
#ifdef _LOG
			Log::Printf(_L("SetOpt\t%S SAP[%u] RawMode=%d HeaderInclude=%d"),
				&ProtocolName(), (TInt)this, iRawMode, iHeaderIncluded);
#endif
			return KErrNone;

		case KSoUserSocket:
			if (ret != KErrNone)
				return ret;
#ifdef _LOG
			Log::Printf(_L("SetOpt\t%S SAP[%u] KSoUserSocket %d -> %d"),
				&ProtocolName(), (TInt)this, iIsUser, val);
#endif
			if ((TUint)val == iIsUser)
				return KErrNone;	// No change in state
			else if (val == 0)
				{
				iProtocol->NetworkService()->DecUsers();
				iIsUser = 0;
				return KErrNone;
				}
			else if (val == 1)
				{
				iProtocol->NetworkService()->IncUsers();
				iIsUser = 1;
				return KErrNone;
				}
			else
				return KErrArgument;
		default:
			// For now, give generic LOG here, but should really log at each implementation -- msa
			LOG(Log::Printf(_L("SetOpt\t%S SAP[%u] level=KSolInetIp name=%d"), &ProtocolName(), (TInt)this, aName));
			break;
			}
		}
	else if (aLevel == KSolInetIfCtrl)
		{
		if (aName == STATIC_CAST(TUint, KSoInetEnumInterfaces))	// See *NOTE* above!
			{
			iInterfaceIndex = 0;
			LOG(Log::Printf(_L("SetOpt\t%S SAP[%u] KSoInetEnumInterfaces"), &ProtocolName(), (TInt)this));
			return KErrNone;
			}
		}
	else if (aLevel == KSolInetRtCtrl)
		{
		if (aName == STATIC_CAST(TUint, KSoInetEnumRoutes))	// See *NOTE* above!
			{
			iRouteIndex = 0;
			LOG(Log::Printf(_L("SetOpt\t%S SAP[%u] KSoInetEnumRoutes"), &ProtocolName(), (TInt)this));
			return KErrNone;
			}
		}
	else if (aLevel == KNifOptLevel)
		{
		return KErrNotSupported;
		}
	if (iFlow.FlowContext())
		{
		const TInt ret = iFlow.FlowContext()->SetOption(aLevel, aName, aOption);
		if (ret != KErrNotSupported)
			return ret;
		}
	return iProtocol->Interfacer()->SetOption(aLevel, aName, aOption, *this);
	}

TInt CProviderInet6Base::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	{
	LOG(Log::Printf(_L("GetOpt\t%S SAP[%u] level=%x, name=%x, len=%d"), &iProtocol->ProtocolName(), (TInt)this, aLevel, aName, aOption.Length());)
	if (aLevel == KSolInetIp)
		switch (aName)
		{
		case KSoInetLastError:
			if (STATIC_CAST(TUint, aOption.MaxLength()) >= sizeof(iLastError))
				{
				aOption.SetLength(sizeof(TSoInetLastErr));
				aOption.Copy((TUint8*)&iLastError, sizeof(iLastError));
				return KErrNone;
				}
			return KErrTooBig;
			
		case KSoHeaderIncluded:
			return SetOptionInt(aOption, iHeaderIncluded);
		case KSoRawMode:
			return SetOptionInt(aOption, iRawMode);
		case KSoUserSocket:
			return SetOptionInt(aOption, iIsUser != 0);

		default:
			break;
		}
	else if (aLevel == KSolInetIfCtrl && aName == STATIC_CAST(TUint, KSoInetNextInterface))	// See *NOTE* above!
		{
		TSoInetInterfaceInfo& opt = *(TSoInetInterfaceInfo*)aOption.Ptr();
		if (STATIC_CAST(TUint, aOption.MaxLength()) < sizeof(TSoInetInterfaceInfo))
			return KErrTooBig;
		(void) new (&opt) TSoInetInterfaceInfo;	// Make sure descriptors are correct.
		aOption.SetLength(sizeof(TSoInetInterfaceInfo));
		((CProviderInet6Base *)this)->iInterfaceIndex = iProtocol->Interfacer()->InterfaceInfo(iInterfaceIndex, opt);
		if (iInterfaceIndex > 0)
			return KErrNone;
		else
			return KErrNotFound;
		}
	else if (aLevel == KSolInetRtCtrl && aName == STATIC_CAST(TUint, KSoInetNextRoute))	// See *NOTE* above!
		{
		TSoInetRouteInfo& opt = *(TSoInetRouteInfo*)aOption.Ptr();
		if (STATIC_CAST(TUint, aOption.MaxLength()) < sizeof(TSoInetRouteInfo))
			return KErrTooBig;
		aOption.SetLength(sizeof(TSoInetRouteInfo));
		(void) new (&opt) TSoInetRouteInfo; // Make sure descriptors are correct.
		((CProviderInet6Base *)this)->iRouteIndex = iProtocol->Interfacer()->RouteInfo(iRouteIndex, opt);
		if (iRouteIndex > 0)
			return KErrNone;
		else
			return KErrNotFound;
		}
	else if (aLevel == KNifOptLevel)
		{
		return KErrNotSupported;
		}
	//
	// if none of the above "supported", falls here...
	//
	CFlowContext *flow = iFlow.FlowContext();
	if (flow)
		{
		const TInt ret = flow->GetOption(aLevel, aName, aOption);
		if (ret != KErrNotSupported)
			return ret;
		}
	return iProtocol->Interfacer()->GetOption(aLevel, aName, aOption, *((CProviderInet6Base *)this));
	}

void CProviderInet6Base::Ioctl(TUint aLevel, TUint aName, TDes8* /*anOption*/)
	{
	LOG(Log::Printf(_L("Ioctl\t%S SAP[%u] %x, %x)"), &ProtocolName(), (TInt)this, aLevel, aName));
	if (aLevel == KSolInetIp && aName == KIoctlInetLastError)
		{
		TPckg<TSoInetLastErr> lastError(iLastError);
		iSocket->IoctlComplete(&lastError);
		}
	else
		Error(KErrNotSupported, MSocketNotify::EErrorIoctl);
	}

void CProviderInet6Base::CancelIoctl(TUint aLevel, TUint aName)
	{
#ifdef _LOG
	Log::Printf(_L("CancelIoctl\t%S SAP[%u] %x, %x)"), &ProtocolName(), (TInt)this, aLevel, aName);
#else
	(void)aLevel;	// silence compiler warning
	(void)aName;	// silence compiler warning
#endif
	// Error call removed. Fixes defect IP/76. -MikaL
	//Error(KErrNotSupported);
	}


void CProviderInet6Base::Process(RMBufChain& /*aPacket*/, CProtocolBase* /*aSourceProtocol*/)
	{
	Panic(EInet6Panic_NotSupported);
	}

//
// Error report routine.
//
// Soft errors are not immediately reported to the socket server.
// A soft error is indicated by a zero aOperationMask.
//
// The socket error can be cleared by calling this routing with
// aError == KErrNone.
//
void CProviderInet6Base::Error(TInt aError, TUint aOperationMask)
	{
	if (aError <= KErrNone && !FatalState())
		{
		iLastError.iStatus = aError;
		if (aError == KErrNone)
			iErrorMask = aOperationMask;
		else
			iErrorMask |= aOperationMask;
		if (iSocket && aOperationMask)
			{
			LOG(Log::Printf(_L("\t%S SAP[%u] Error %d, mask %b"),
						&ProtocolName(), (TInt)this, aError, aOperationMask));
			iSocket->Error(aError, aOperationMask);
	 		}
		else
			{
			LOG(Log::Printf(_L("\t%S SAP[%u] Error %d, mask %b pending"),
						&ProtocolName(), (TInt)this, aError, aOperationMask));
			}
		}
	else
		{
		LOG(Log::Printf(_L("\t%S SAP[%u] Error %d, mask %b ignored"),
					&ProtocolName(), (TInt) this, aError, aOperationMask));
		}
	}

void CProviderInet6Base::SaveIcmpError(TInt aType, TInt aCode, const TInetAddr& aSrcAddr,
									 const TInetAddr& aDstAddr, const TInetAddr& aErrAddr)
	{
	iLastError.iErrType = aType;
	iLastError.iErrCode = aCode;
	iLastError.iSrcAddr = aSrcAddr;
	iLastError.iDstAddr = aDstAddr;
	iLastError.iErrAddr = aErrAddr;
	}

void CProviderInet6Base::CanSend()
	{
	if(iSocket && /*iFlow.Status() == EFlow_READY && */!FatalState())
		{
		LOG(Log::Printf(_L("\t%S SAP[%u] CanSend() Flow UNBLOCKED"), &ProtocolName(), (TInt)this));
		iSocket->CanSend();
		}
	}

void CProviderInet6Base::NoBearer(const TDesC8& aConnectionParams)
	{
	if(iSocket)
		{
		LOG(Log::Printf(_L("\t%S SAP[%u] NoBearer()"), &ProtocolName(), (TInt)this));
		iSocket->NoBearer(aConnectionParams);
		}
	}

void CProviderInet6Base::Bearer(const TDesC8 &aConnectionInfo)
	{
	if(iSocket)
		{
		LOG(Log::Printf(_L("\t%S SAP[%u] Bearer()"), &ProtocolName(), (TInt)this));
		iSocket->Bearer(aConnectionInfo);
		}
	}

TInt CProviderInet6Base::CheckPolicy(const TSecurityPolicy& aPolicy, const char *aDiagnostic)
	{
	return iSecurityChecker ? iSecurityChecker->CheckPolicy(aPolicy, aDiagnostic) : KErrDisconnected;
	}

TInt CProviderInet6Base::SecurityCheck(MProvdSecurityChecker *aSecurityChecker)
	{
	iSecurityChecker = aSecurityChecker;
	iHasNetworkServices = CheckPolicy(KPolicyNetworkServices, 0) == KErrNone;
	return KErrNone;
	}

TInt CProviderInet6Base::GetOptionInt(const TDesC8 &anOption, TInt &aVal)
	{
 	if (STATIC_CAST(TUint, anOption.Length()) < sizeof(aVal))
		return KErrTooBig;
	Mem::Copy(&aVal, anOption.Ptr(), sizeof(aVal));
	return KErrNone;
	}

TInt CProviderInet6Base::SetOptionInt(TDes8 &anOption, TInt aVal)
	{
	if (STATIC_CAST(TUint, anOption.MaxLength()) < sizeof(aVal))
		return KErrTooBig;
	anOption.SetLength(sizeof(aVal));
	anOption.Copy((TUint8*)&aVal, sizeof(aVal));
	return KErrNone;
	}

TInt CProviderInet6Base::Write(RMBufChain& aData, TUint aOptions, TSockAddr* aToAddr)
	{
	RMBufSendPacket packet;
	packet.Assign(aData);	// *beware!* It is legal for aData to be EMPTY!
#ifdef _LOG
	TBuf<70> tmp(_L("NULL"));
#endif

	TInt offset = 0;
	RMBufSendInfo* info = NULL;
	TInt err(0);
	// Allocate the info block for the packet.
	info = packet.NewInfo();
	// Because many RMBufChain methods do not handle the empty 
	// chain "logically", must make sure the chain has at least
	// one RMBuf in it (without no content). Allocate one and
	// make its content empty.
	if (info)
		{
		if (packet.IsEmpty())
			{
			err = packet.Alloc(0);	// <-- *WARNING* Assumed to allocate single RMBuf
			if (err == KErrNone)
				{
		    	packet.First()->AdjustStart(packet.Length());
				info->iLength = 0;
				}
			}
		else
			info->iLength = packet.Length();
		}
	else
		err = KErrNoMBufs; // The info allocation was not successful
	
	
	if(err < 0)
		{
		// Treat all leaves in above as no MBufs.
		LOG(Log::Printf(_L("Write\t%S SAP[%u] No Mbufs (%d)"), &ProtocolName(), (TInt)this, err));
		err = KErrNoMBufs;
		goto notify_error;
		}

#ifdef _LOG
	if (aToAddr)
		TInetAddr::Cast(*aToAddr).OutputWithScope(tmp);
	Log::Printf(_L("Write\t%S SAP[%u] len=%d, opt=%x, to=%S"), &ProtocolName(), (TInt)this, info->iLength, aOptions, &tmp);
#endif

	//
	// Initialize the info
	//
	TInetAddr::Cast(info->iSrcAddr).Init(0);
	info->iSrcAddr.SetPort(0);
	TInetAddr::Cast(info->iDstAddr).Init(0);
	info->iDstAddr.SetPort(0);
	//
	// Get the protocol number to use.
	// iProtocolId must be NON-ZERO for anything else, except
	// for IP/IP6 raw sockets. Only those sockets have specified
	// that the protocol number can be given in port field!
	info->iProtocol = iProtocolId != 0 ? iProtocolId :
		aToAddr != NULL ? aToAddr->Port() : 0;
	info->iFlags = aOptions & (KIpHeaderIncluded | KIpDontFragment);
	if (iHeaderIncluded)
		info->iFlags |= KIpHeaderIncluded;
	(void)new (&info->iFlow) RFlowContext();

	//
	// Prepare iDstAddr from aToAddr, if specified
	//
	if (aToAddr)
		{
		info->iDstAddr = *aToAddr;	// Copy as is, including port, flow label and scope.
		if (info->iDstAddr.Family() != KAfInet6)
			TInetAddr::Cast(info->iDstAddr).ConvertToV4Mapped();
		}
	//
	// Prepare from included header, if present
	//
	err = KErrNotSupported;
	if (info->iFlags & KIpHeaderIncluded)
		{
		// The packet starts with an IPv4 or IPv6 header.
		TInt next_header = info->iProtocol;
		TIpHeader *const ip = ((RMBufPacketPeek &)packet).GetIpHeader();
		if (!ip)
			{
			LOG(Log::Printf(_L("\t%S SAP[%u] HeaderIncluded, short/bad IP header"), &ProtocolName(), (TInt)this));
			goto notify_error;
			}
		switch (ip->ip4.Version())
			{
			case 4:
				offset = ip->ip4.HeaderLength();
				// If protocol is ZERO in header, default to aProtocol. This is
				// a backward compatibility issue. Applications should not rely
				// on this. If they are providing the IPv4 header, they SHOULD
				// ALSO fill the proper protocol number in there.
				if (!ip->ip4.Protocol())
					ip->ip4.SetProtocol(next_header);
				else
					next_header = ip->ip4.Protocol();
				TInetAddr::Cast(info->iSrcAddr).SetV4MappedAddress(ip->ip4.SrcAddr());
				if (aToAddr)
					ip->ip4.SetDstAddr(TInetAddr::Cast(info->iDstAddr).Address());
				else
					TInetAddr::Cast(info->iDstAddr).SetV4MappedAddress(ip->ip4.DstAddr());
				info->iProtocol = KProtocolInetIp;
				break;
			case 6:
				offset = ip->ip6.HeaderLength();
				// Note: ZERO NextHeader() does not cause default processing. In
				// IPv6 ZERO is the Hop-by-Hop extension header and potentially
				// provided by the application.
				next_header = ip->ip6.NextHeader();
				TInetAddr::Cast(info->iSrcAddr).SetAddress(ip->ip6.SrcAddr());
				if (aToAddr)
					ip->ip6.SetDstAddr(TInetAddr::Cast(info->iDstAddr).Ip6Address());
				else
					TInetAddr::Cast(info->iDstAddr).SetAddress(ip->ip6.DstAddr());
				info->iProtocol = KProtocolInet6Ip;
				break;
			default:
				LOG(Log::Printf(_L("\t%S SAP[%u] HeaderIncluded, bad IP version (= %d)"), &ProtocolName(), (TInt)this, (TInt)ip->ip4.Version()));
				goto notify_error;
			}
		//
		// Need to locate the upper layer header
		//
		while (info->iProtocol != next_header)
			{
			TInet6Packet<TInet6HeaderExtension> hdr(packet, offset);
			if (hdr.iHdr == NULL || !TPacketPoker::IsExtensionHeader(next_header))
				{
				LOG(Log::Printf(_L("\t%S SAP[%u] HeaderIncluded, unknown extension hdr (= %d)"), &ProtocolName(), (TInt)this, next_header));
				goto notify_error;				
				}
			offset += hdr.iHdr->HeaderLength();
			next_header = hdr.iHdr->NextHeader();
			}
		}

	iFlow.SetProtocol(info->iProtocol);
	err = DoWrite(packet, *info, aOptions, offset);
	if (err == KErrNone)
		{
		// If packet is Empty, assume DoWrite handled it, and
		// don't send it to the protocol.
		if (!packet.IsEmpty())
			{
			packet.Pack();
			iProtocol->Send(packet);
			}
		goto packet_done;
		}
	// DoWrite indicated problem:
	//	 err > 0, flow cannot receive packet
	//	 err < 0, some error condition
	//
notify_error:
	if (err >= 0)
		err = 0;	// Return 0 to indicate that packet has not been accepted
	else if (err != KErrNoMBufs)
		{
		//
		// Cannot send the packet, report error and drop packet.
		//
		Error(err, MSocketNotify::EErrorSend);
		goto packet_done;
		}
	else
		{
		if (iSynchSend == 0)
			{
			// Out of buffer space (KErrNoMBufs). If iSynchSend is not set, just
			// drop the packet silently,
			LOG(Log::Printf(_L("\t%S SAP[%u] Dropping packet for KErrNoMBufs"), &ProtocolName(), (TInt)this));
			goto packet_done;
			}
		LOG(Log::Printf(_L("\t%S SAP[%u] Blocking for KErrNoMBufs"), &ProtocolName(), (TInt)this));
		}

	// note: err == 0 or KErrNoMBufs!
	aData.Assign(packet);	// Try to return original chain (approximately correct only
							// if DoWrite does not add anything to the chain!)
	packet.Free();			// Need to release info!
	return err;
packet_done:
	// Packet has been sent or dropped
	packet.Free();			// Cleanup, not always needed (does nothing then).
	return 1;				// Return 1 to indicate that packet has been accepted
	}

TUint CProviderInet6Base::Write(const TDesC8& aDesc, TUint aOptions, TSockAddr* aAddr)
	{
	LOG(Log::Printf(_L("Write\t%S SAP[%u] Desclen=%d, opt=%x)"), &ProtocolName(), (TInt)this, aDesc.Length(), aOptions));
	RMBufChain data;
	if (aDesc.Length() > 0)
		{
		TRAPD(err, data.CreateL(aDesc));
		if (err != KErrNone)
			{
			data.Free();	// just to be sure..
			return 1;
			}
		}
	const TInt ret = Write(data, aOptions, aAddr) != 0; // Return 0 or 1 only.
	data.Free();
	return ret;
	}
