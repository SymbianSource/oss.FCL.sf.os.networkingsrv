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
// inet.h - main definitions for inet protocols
//



/**
 @internalComponent
*/
#ifndef __INET_H__
#define __INET_H__

#include <f32file.h>
#include <e32math.h>

#include <es_sock.h>
#include <es_prot.h>
#include <es_mbuf.h>
#include <es_ver.h>
#include <in_sock.h>
#include <nifman.h>	// For Nif:: calls!
#	include <comms-infras/nifif.h>	// ..for CNifIfBase in Epoc R6 and later

#include <nifmbuf.h>

#include <flow.h>
#include <in_bind.h>
#include "in_fmly.h"
#include "inet6log.h"
#include <es_prot_internal.h>

// Minimum MTU that must be supported by all IPv4 links
const TUint KInetStandardMtu = 576;

// Minimum MTU that must be supported by all IPv6 links
const TUint KInet6StandardMtu = 1280;

// Size of SAP hash table.
const TUint KInet6SAPTableSize = 67;

class CProtocolInet6Base;
class CProtocolInet6Transport;
class CProviderInet6Base;
class TInet6SAPIter;

//
// RMBuf paket queue with automatic asynchronous notification.
//
class RMBufAsyncPktQ : public RMBufPktQ
	{
public:
	RMBufAsyncPktQ() : iCallBack(NULL) {}
	~RMBufAsyncPktQ() { Cancel(); delete iCallBack; }
	void InitL(TCallBack& aCallBack, TInt aPriority = KInet6DefaultPriority)
		{
		RMBufPktQ::Init();
		iCallBack = new (ELeave) CAsyncCallBack(aCallBack, aPriority);
		}

	//
	// The automatic wakeup of the reader has been removed.
	// Call Wake() explicitly when you want to wake up the
	// reader, e.g., after adding packets to the queue. -ML
	//
	void Wake()				{ if (iCallBack != NULL) iCallBack->CallBack(); }
	void Cancel()			{ if (iCallBack != NULL) iCallBack->Cancel(); }
	CAsyncCallBack *AsyncCallBack() { return iCallBack; }

private:
	CAsyncCallBack* iCallBack;
	};

//
// Base for Internet Protocols
//

class CProtocolInet6Base : public CProtocolInet6Binder
	{
	friend class TInet6SAPIter;

public:
	CProtocolInet6Base();
	virtual ~CProtocolInet6Base();
	virtual void InitL(TDesC& aTag);
	virtual void StartL();

	virtual void BindL(CProtocolBase *aProtocol, TUint aId);
	virtual void Unbind(CProtocolBase *aProtocol, TUint aId = 0);
	virtual void Error(TInt anError,CProtocolBase* aSourceProtocol=NULL);

	virtual void BindProvider(CProviderInet6Base* aSAP);
	virtual void QueueBindProvider(CProviderInet6Base* aSAP);
	virtual void UnbindProvider(CProviderInet6Base* aSAP);
	virtual CProviderInet6Base* LocateProvider(TUint aPort);
	inline MInterfaceManager *Interfacer() const { return iNetwork->Interfacer(); }
	inline TUint SapCount() const { return iSapCount; }
	void IncSAPs();
	void DecSAPs();
protected:
	virtual TUint ProviderHashKey(TUint aPort) { return aPort % KInet6SAPTableSize; }
	TUint iSapCount;// SAP count for this protocol. Includes embryonic sockets.
private:
	CProviderInet6Base* iSAP[KInet6SAPTableSize];
#ifdef _LOG
	// The protocol name is cached here for log purposes
	// (to avoid exessive stack use for TServerProtocolDesc).
	// Initilized in base InitL from a call to Identify.
	TProtocolName iName;
public:
	const TDesC &ProtocolName() const { return iName; }
#endif
	};


//
// Internet Socket Provider Base
//

class CProviderInet6Base : public CServProviderBase, public MProviderNotify
	, public MProvdSecurityChecker
	{
	friend class CProtocolInet6Base;
	friend class CProtocolInet6Transport;
	friend class TInet6SAPIter;

public:
	virtual void InitL();
	CProviderInet6Base(CProtocolInet6Base* aProtocol);
	virtual ~CProviderInet6Base();

	virtual void AutoBind()						 	{ }
	virtual void LocalName(TSockAddr &aAddr) const	{ aAddr = iFlow.FlowContext()->LocalAddr(); }
	virtual TInt SetLocalName(TSockAddr &aAddr)		{ iFlow.SetLocalAddr(aAddr); return 0; }
	virtual void RemName(TSockAddr &aAddr) const	{ aAddr = iFlow.FlowContext()->RemoteAddr(); }
	virtual TInt SetRemName(TSockAddr &aAddr)		{ iFlow.SetRemoteAddr(aAddr); return 0; }
	virtual void ActiveOpen();
	virtual void ActiveOpen(const TDesC8 &aConnectionData);
	virtual TInt PassiveOpen(TUint aQueSize);
	virtual TInt PassiveOpen(TUint aQueSize,const TDesC8 &aConnectionData);
	virtual void Shutdown(TCloseType option,const TDesC8 &aDisconnectionData);
	virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8& aOption);
	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8& aOption) const;
	virtual void Ioctl(TUint aLevel, TUint aName, TDes8* anOption);
	virtual void CancelIoctl(TUint aLevel, TUint aName);
	virtual void Start();

	virtual void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol = NULL);

	virtual TUint Write(const TDesC8& aDesc,TUint options, TSockAddr* anAddr=NULL);
	virtual TInt Write(RMBufChain& aData, TUint aOptions, TSockAddr* anAddr=NULL);

	virtual void CanSend();
	virtual void NoBearer(const TDesC8& aConnectionParams);
	virtual void Bearer(const TDesC8 &aConnectionInfo);
	virtual TInt SecurityCheck(MProvdSecurityChecker *aSecurityChecker);
	inline void NoSecurityChecker() { iSecurityChecker = NULL;}
	inline TBool HasNetworkServices() { return iHasNetworkServices; }
	virtual void Error(TInt aError, TUint aOperationMask = MSocketNotify::EErrorAllOperations);
	virtual void SaveIcmpError(TInt aType, TInt aCode, const TInetAddr& aSrcAddr,
							 const TInetAddr& aDstAddr, const TInetAddr& aErrAddr);

	inline TBool FatalState() { return (iErrorMask & (MSocketNotify::EErrorFatal|MSocketNotify::EErrorConnect)) != 0; }
protected:
	static TInt GetOptionInt(const TDesC8 &anOption, TInt &aVal);
	static TInt SetOptionInt(TDes8 &anOption, TInt aVal);

	virtual TInt CheckPolicy(const TSecurityPolicy& aPolicy, const char *aDiagnostic);
	/**
	* Complete write processing for a datagram protocol
	*
	* The base class Write() implements a common processing for simple datagram
	* socket. After this setup work, the Write() calls DoWrite.
	*
	* @param aPacket	The packet to be sent	
	* @param aInfo		The info block of the packet
	* @param aOptions	The options as defined for CServProviderBase::Write
	* @param aOffset	Offset to the beginning of the upper layer header
	* @return		Result of the write
	*
	* The aOffset has following semantics:
	* - aOffset == 0, normal datagram without header included (DoWrite needs to add upper layer header)
	* - aOffset > 0, header included was present, and offset indicates upper layer header in packet).
	*
	* RMBufSendPacket has been prepared from the parameters of the Write
	* function and the information block is initialized as follows:
	*
	* - iSrcAddr	Family() == 0, Port() == 0
	* - iDstAddr	Family() == 0, Port() == 0
	* - iProtocol	== aProtocol
	* - iFlags		== aOptions & (KIpHeaderIncluded | KIpDontFragment)
	* - iLength		== aPacket.Length()
	* - iFlow		== 0
	*
	* If KIpHeaderIncluded is set, the following is true
	*
	* - iSrcAddr	Family() == KAfInet6 and address is loaded from the IPv4 or IPv6 header
	* - iDstAddr	Family() == KAfInet6 and address is loaded from the IPv4 or IPv6 header
	* - iProtocol	== KProtocolInetIP or KProtocolInet6Ip, depending on IP version.
	*
	* If aToAddr and KIpHeaderIncluded are both set, the aToAddr overrides whatever is
	* stored in the destination address of the included header.
	*
	* If aToAddr is defined, the scope and flow label (if present) are preserved in
	* the iDstAddr.
	*
	* The return value, 'err'
	* - err == KErrNoRMBufs, a special error return that blocks the socket
	* - err < 0, causes Error(err, MSocketNotify::EErrorSend) to be called for the socket.
	* - err == 0, the aPacket is sent out
	* - err > 0, blocks the socket (current packet is retried later)
	*/
	virtual TInt DoWrite(RMBufSendPacket &/*aPacket*/, RMBufSendInfo &/*aInfo*/, TUint /*aOptions*/, TUint /*aOffset*/) 
		{
		return KErrNotSupported;
		}

#ifdef _LOG
	inline const TDesC &ProtocolName() const {return iProtocol->ProtocolName();}
#endif

	// Basic socket info
	CProtocolInet6Base *const iProtocol;

	// Socket error status
	TSoInetLastErr iLastError;
	TUint iErrorMask;

	// Flow context
	RFlowContext iFlow;

	// SAP db hash link
	CProviderInet6Base* iNextSAP;

	// Interface and route enumeration state
	TUint iInterfaceIndex;
	TUint iRouteIndex;
	MProvdSecurityChecker *iSecurityChecker;
private:

	// iIsUser = 1, when SAP is counted as a "user" for the network layer (IncUsers done)
	// iIsUser = 0, when SAP is not counted as a "user" for the network layer
	// By default, the this class reqisters all SAPs as users for the network layer, and
	// this state can be explicitly cancelled by the application through a socket option.
	// (the user count affects the automatic shutdown process of the stack)
	TUint iIsUser:1;
protected:
	TUint iHasNetworkServices:1;// If true, socket has NetworkServices capability
	TUint iRawMode:1;			// If true, user receives datagram packets with IP header
	TUint iHeaderIncluded:1;	// If true, user sends datagram packets with IP header
	TUint iSynchSend:1;			// If true, block socket write to UDP socket in PENDING and HOLD states
	TInt iProtocolId;
	TInt iDomainSuffixIndex;	// Interface domain suffix enumeration state
	TName iActiveEnumInterface; // Name of the interface previously enumerated with option KSoInetNextInterface
	};

class TInet6SAPIter
	{
public:
	inline TInet6SAPIter(CProtocolInet6Base *aProto) : iProto(aProto), iSap(NULL), iKey(0) {}
	inline CProviderInet6Base* operator++(TInt)
		{
		CProviderInet6Base *ptr;
		for (ptr = iSap; ptr == NULL; ptr = iProto->iSAP[iKey++])
			if (iKey >= KInet6SAPTableSize)
				return NULL;
		iSap = ptr->iNextSAP;
		return ptr;
		}
private:
	CProtocolInet6Base *iProto;
	CProviderInet6Base *iSap;
	TUint iKey;
	};

// security policies
_LIT_SECURITY_POLICY_C1(KPolicyNetworkServices, ECapabilityNetworkServices);
_LIT_SECURITY_POLICY_C1(KPolicyNetworkControl, ECapabilityNetworkControl);

#endif
