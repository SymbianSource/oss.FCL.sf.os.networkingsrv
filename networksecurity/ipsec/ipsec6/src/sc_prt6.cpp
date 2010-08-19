// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// sc_prt.cpp - IPv6/IPv4 security check protocol
// An instance of a procotol that hooks into IP protocol instance,
// performing IPSEC checking and processing to packets.
//

#include <featdiscovery.h>
#include <featureuids.h>

#include <posthook.h>
#include <icmp6_hdr.h>
#include <udp_hdr.h>
#include <in_pkt.h>
#include <ext_hdr.h>
#include <in6_opt.h>
#include <in6_event.h>
#include <es_prot_internal.h>

#include "ipsec.h"
#include "sc.h"
#include "epdb.h"
#include "spdb.h"
#include "sadb.h"
#include <networking/ipsecerr.h>
#include "ipseclog.h"
#include "tunlintunl.h"

const TIp6Addr KOuterIsInner = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};

class CProtocolIpsec;
class CIpsecHook;

/**
* Mobility Protocol id.
* (this should be in insock.h?)
*/
const TUint KProtocolInetMh = 135;
/**
* SCTP Protocol id
* (this should be in insock.h?)
*/
const TUint KProtocolInetSctp = 132;

/**
* Max number of nested IPSEC.
*/
const int KIpsecMaxNesting = 10;

/**
* Max number of fragment bookkeeping records.
*/
const TInt KIpsecMaxFragmentInfo = 10;

class RHookSA : public RSecurityAssociation
	/**
	* Security association handle.
	*
	* Extends the RSecurityAssociation handle to include a back link
	* to the CIpsecHook which actually contains the RHookSA
	* (for outbound processing)
	*/
	{
public:
	static void Callback(RSecurityAssociation &aAssociation);

	RHookSA(CFlowContext &aFlow, CPolicyAction *const aItem);
	/**
	* The last state of the association.
	*
	* iState is mainly used to detect whether initial
	* acquire failed, when SA dies. If PENDING at DEAD,
	* then SA died before ever becoming ready,
	*/
	TInt iState;
	CPolicyAction *const iItem;
private:
	CFlowContext &iFlow; // ..to change the flow state when SA state changes (see CallBack)
	};

class TUpperLayerSnoop
	/**
	* A snooping class.
	* A simple class to access the first 4 bytes of the packet.
	* This is used to snoop the upper layer information.
	*
	* Thus max "port offset" = 2 (because port is 16 bits),
	* and max "type/code offset = 3.
	*/
	{
public:
	// Basic
	inline static TInt MinHeaderLength() {return 4; }
	inline static TInt MaxHeaderLength() {return 4; }
	// Access values
	inline TUint16 Port(TUint8 aOff) const { return (i[aOff] << 8) + i[aOff+1]; }
	inline TUint8 Byte(TUint8 aOff) const { return i[aOff]; }
private:
	TUint8 i[4];
	};

class TSnoopHeader
	/**
	* Describes the positions of the port/type/code information in a protocol headers.
	*/
	{
public:
	TUint8 iProtocol;	//< 0..255
	TUint8 iSelector;	//< 0..3 (ports, localport, type+code, type)
	TUint8 iO1;			//< Offset of the source port [0..2] or type [0..3]),
	TUint8 iO2;			//< Offset of the destination port [0..2] or code [0..3])
	};

const TUint KLoadPorts = 0;	//< Load local and remote port
const TUint KLoadLocal = 1;	//< Load local port only
const TUint KLoadType = 2;	//< Load Type only (into local port as 'type << 8')
const TUint KLoadBoth = 3;	//< Load Type and code (into local port as 'type << 8 | code')

/**
* The list of currently known protocols.
*
* This table defines the "known protocols" for IPsec, and this must be updated
* if some new protocol uses ports or type/code system, and which needs to
* selected in IPsec selectors.
*/
static const TSnoopHeader snooper[] =
	{
		{ KProtocolInetUdp,		KLoadPorts,	0, 2 },	// UDP uses src and dst port
		{ KProtocolInetTcp,		KLoadPorts,	0, 2 },	// TCP uses src and dst port
		{ KProtocolInetIcmp,	KLoadBoth,	0, 1 },	// ICMPv4 uses type and code
		{ KProtocolInet6Icmp,	KLoadBoth,	0, 1 },	// ICMPv6 uses type and code
		{ KProtocolInetMh,		KLoadType,	2, 0 },	// Mobile IPv6 uses type
		{ KProtocolInetSctp,	KLoadPorts, 0, 2 }	// SCTP uses src and dst port
	};
static const TSnoopHeader *const snooper_end = &snooper[sizeof(snooper)/sizeof(snooper[0])];


class CIpsecHook : public CIpsecReferenceCountObject, public MFlowHook
	/**
	* The MFlowHook for outbound IPsec processing.
	*
	* An instance of this class is attached to the flows for which
	* IPsec processing is required.
	*
	* The allocation size of this object is variable and it depends
	* the required IPsec processing. The variable length information
	* is at the end of fixed member variables consists of two
	* arrays of structures as follows:
	* @code
	* RHookSA[iCount]		// The handles for IPsec associations
	* RIpAdddress[iTunnels]	// The src addresses of the tunnels (if iTunnels > 0)
	* @endcode
	*/
	{
private:
	CIpsecHook(
		MAssociationManager &aMgr,
		CFlowContext &aFlow,
		const RPolicySelectorInfo &aInfo,
		const TInt aCount,
		const TInt aTunnels);
	~CIpsecHook();

	RHookSA &Assoc(TInt aIndex) const
		{ return ((RHookSA *)(((char *)this) + sizeof(*this)))[aIndex]; }
	RIpAddress &Tunnel(TInt aIndex) const
		{ return ((RIpAddress *)&Assoc(iCount))[aIndex]; }

public:
	// for CProtocolSecpol
	static CIpsecHook *NewL(
		MAssociationManager &aMgr,
		CFlowContext &aFlow,
		const RPolicySelectorInfo &aInfo,
		TInt aCount, CPolicyAction **aItems,
		TInt aTunnels, const RIpAddress *aSrc);

	// MFlowHook
	void Open();
	TInt ReadyL(TPacketHead &aHead);
	TInt ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	void Close();
	
private:
	// The internal state information

	MAssociationManager &iMgr;		//< The association manager.
	CFlowContext &iFlow;			//< The associated flow
	RPolicySelectorInfo iInfo;		//< The selector information extracted at OpenL
	const TUint8 iCount;        	//< Number of IPSEC actions (usually SA) to apply
	const TUint8 iTunnels;      	//< ..and how many of them include tunnel action

	// Followed by arrays of
	// - RHookSA[iCount]
	// - RIpAdddress[iTunnels]
	};



RHookSA::RHookSA(CFlowContext &aFlow, CPolicyAction *const aItem) : iItem(aItem), iFlow(aFlow)
	/**
	* Constructor.
	*
	* @param aFlow The flow.
	* @param aItem The action
	*/
	{
	iState = EFlow_PENDING;
	RSecurityAssociation::Init(Callback);
	aItem->Open();
	}

void RHookSA::Callback(RSecurityAssociation &aAssociation)
	/**
	*  Association has changed state.
	*
	* @param aAssociation The handle.
	*/
	{
	const RHookSA &assoc = (RHookSA &)aAssociation;

	switch (assoc.Status())
		{
		case SADB_SASTATE_MATURE:
		case SADB_SASTATE_DYING:
			// Either of these states means that SA is usable,
			// The previous state makes no difference.
			// In case of READY, should probably check all other
			// required SA's, and not try to wakeup the flow
			// until ALL of them are available. -- msa
			if (assoc.iState != KErrNone && assoc.iItem && assoc.iItem->iOptional)
				assoc.iFlow.SetChanged();	// When optional SA becomes available, "recompute" flow!
			assoc.iFlow.SetStatus(EFlow_READY);
			break;
		case SADB_SASTATE_DEAD:
			//
			// The SA is dead. Now it depends on the previous state
			// whether a new SA negotiation should be started or if
			// the flow should be shut down.
			if (assoc.iState == EFlow_PENDING)
				{
				//
				// Flow was waiting for negotiation, which expired
				//
				/* here the error value passed by the kmd server
				 * will be passed to the tcp/ip stack. This error
				 * value was set in the RSecurityAssociation class.
				 * 
				 */
				TInt errorValue = assoc.ReadErr();
				if(errorValue==EIpsec_Ok)
				    {
                       			 errorValue = EIpsec_AcquireFailed;
				    }
				assoc.iFlow.SetStatus(errorValue);
				break;
				}
			// SA was not a LARVAL SA. This means that a real SA
			// has just expired or been deleted for some reason,
			// the flow just needs to acquire a new SA.
			// *FALL THROUGH*
		default:
			// SA not available, but may be acquired, set/keep
			// the flow in pending state (a ReadyL call later
			// will activate the acquire, if needed).
			assoc.iFlow.SetStatus(EFlow_PENDING);
			break;
		}
	}


class TIpsecFragmentData
	/**
	* The state information for single IPsec transfor.
	*/
	{
public:
	RSecurityAssociation iSA;		//< The IPsec SA that was used
	TIpAddress iTunnel;				//< The outer Tunnel End Point (src).
	};

class CIpsecFragmentInfo : public CBase
	/**
	* The IPsec fragment tracking.
	*
	* When IPsec has been applied to tunneled fragments, it must
	* be guaranteed that all fragments are correctly protected
	* as required by the policy. However, the realy policy
	* requirement is only known after the final fragment has
	* arrived and when the full packet is available.
	*
	* This structure keeps track of applied IPsec for a
	* packet being assembled.
	*
	* This is variable sized object, and the tail end
	* is an array of state info:
	* @code TIpsecFragmentData[iCount]
	* @endcode
	*/
	{
private:
	CIpsecFragmentInfo(TUint aCount) : iCount(aCount) {}
public:
	static CIpsecFragmentInfo *New(TUint aCount);
	inline TIpsecFragmentData & operator[](TInt aIndex) const;
	~CIpsecFragmentInfo();

	CIpsecFragmentInfo *iNext;		//< The link to next info block.
	
	// Id + src + dst identify the fragment.
	
	TUint32 iId;					//< The fragment ID
	TIpAddress iSrc;				//< The (inner) source address
	TIpAddress iDst;				//< The (inner) destination address
	
	// The applied IPsec
	const TUint iCount;				//< The number of TIpsecFragmentData that follow after this.
	
	// Followed by array of TIpsecFragmentData[iCount]
	};

class CProtocolSecpol : public CProtocolPosthook, public MSecurityPolicyManager, MEventListener
	/**
	* Security Policy component of the IPsec.
	*
	* This protocol handles the Security Policy Database and also acts as a
	* hook for the IP layer.
	*/
	{
public:
	// ESOCK Protocol Basics
	CProtocolSecpol();
	~CProtocolSecpol();
	void BindToL(CProtocolBase *protocol);
	void Identify(TServerProtocolDesc *) const;
	CServProviderBase* NewSAPL(TUint aSockType);

	// for CProtocolIpsec (inbound transforms)
	TInt TransformL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);

	// Hook methods (inbound direction)
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);

	// Hook methods (outbound direction)
	MFlowHook *OpenL(TPacketHead &aHead, CFlowContext *aFlow);

	// MEventListener
	void Notify(TUint aEventClass, TUint aEventType, const void *aData);

private:
	TUint32 GetInterfaceIndex(const TDesC &aName);
	TBool UpdateInterfaceIndex(const TDesC &aName, TUint32 aIndex);
	void FixupInterfaceIndexes(CSecurityPolicy *aPolicy);

	// Security Policy Management section
	inline CSecurityPolicy *Policy() const { return iPolicy; }
	TInt SetPolicy(const TDesC &aPolicy, TUint &aOffset);

	void Deliver(RMBufPacketBase& aPacket);
	void NetworkAttachedL();
	void NetworkDetached();
	void InterfaceAttached(const TDesC &aName, CNifIfBase *aIf);

	TInt SelectSource(const RIpAddress &aDst, RIpAddress &aSrc) const;
	TInt CollectBundle(TPolicyFilterInfo &aFilter, RPolicySelectorInfo &aKey, const TInt aMaxItems,
		CPolicyAction **aItems, RIpAddress *aSrc, TInt &aTunnels) const;
	void UpdateTunnelInterface(RMBufRecvInfo &aInfo, const CSecurityAssoc *const aSa) /*const*/;
	void CheckPacketId(RMBufHookPacket &aPacket);
	TInt CheckFragmentPolicy();

	void CheckExceptionSelector(TBool &aException_flag) const; //To support UMA
	void CheckFeatureSupportL(TUid aFeature); //To check a Feature is enabled or not

	MAssociationManager *iAssociationManager;
	CProtocolIpsec *iProtocolIpsec;
	MEventService *iEventService;
	CSecurityPolicy* iPolicy;
	TDblQue<CProviderSecpol> iSAPlist;
	//
	// The working space for incoming packet processing
	//
	TInt8 iCountSA;			//< Number of slots used from the arrays below
	TUint iIsFragment:1;	//< Current packet is a fragment.
	TUint iIsTunnelMode:1;	//< The last transform was in tunnel mode.
	TUint32 iPacketId;		//< Id of the current packet.
	TUint32 iId;			//< The fragment id.
	TIpAddress iSrc;		//< The currently effective src address of incoming packet
	TIpAddress iDst;		//< The currently effective dst address of incoming packet
	RPolicySelectorInfo iPktInfo;		//< Incoming packet info
	RSecurityAssociation iSA[KIpsecMaxNesting];	//< Used Security associations.
	RIpAddress iTunnel[KIpsecMaxNesting];		//< Related tunnel addresses (outer source)
	RIpAddress iMyself[KIpsecMaxNesting];		//< Related destination adresses (outer).
	// List of incomplete fragmented packets.
	CIpsecFragmentInfo *iFrags;					//< Unfinished fragments.
	TBool iIPSecGANSupported; //To check whether FF_IPSEC_UMA_SUPPORT_ENABLE is defined and UMA supported

	};

void IPSEC::IdentifySecpol(TServerProtocolDesc &aEntry)
	/**
	* Return IPsec protocol description.
	*
	* @retval aEntry The protocol description.
	*/
	{
	_LIT(KSecpol, "secpol6");

	aEntry.iName = KSecpol;;
	aEntry.iAddrFamily = KAfIpsec;
	aEntry.iSockType = KSockRaw;
	aEntry.iProtocol = KProtocolSecpol;
	aEntry.iVersion = TVersion(1, 0, 0);
	aEntry.iByteOrder = ELittleEndian;
	aEntry.iServiceInfo = KSIConnectionLess | KSIMessageBased | KSIBroadcast;
	aEntry.iNamingServices = 0;
	aEntry.iSecurity = KSocketNoSecurity;
	aEntry.iMessageSize = 0xffff;
	aEntry.iServiceTypeInfo= ESocketSupport | ENeedMBufs;
	aEntry.iNumSockets = KUnlimitedSockets;
	}

CProtocolBase *IPSEC::NewSecpolL()
	/**
	* Create a new SECPOL protocol instance.
	*
	* @return The Secpol
	*/
	{
	return new (ELeave) CProtocolSecpol();
	}
	
//

static void NoCallback(RSecurityAssociation &)
	/** Dummy do nothing callback */
	{
	}


// CIpsecFragmentInfo
//

inline TIpsecFragmentData & CIpsecFragmentInfo::operator[](TInt aIndex) const
	/*
	* Return Nth fragment info.
	* @param aIndex The N value.
	*/
	{
	ASSERT((TUint)aIndex < iCount);
	return ((TIpsecFragmentData *)((char *)this + sizeof(*this)))[aIndex];
	}
		
CIpsecFragmentInfo *CIpsecFragmentInfo::New(TUint aCount)
	/**
	* Create a new fragment info.
	*
	* @param aCount The number of TIpsecFragmentData
	* @return Fragment info block.
	*/
	{
	CIpsecFragmentInfo *f = new (aCount * sizeof(TIpsecFragmentData)) CIpsecFragmentInfo(aCount);
	if (f)
		{
		for (TInt i = 0; i < f->iCount; ++i)
			(*f)[i].iSA.Init(NoCallback);
		}
	return f;
	}

CIpsecFragmentInfo::~CIpsecFragmentInfo()
	/**
	* Desctructor.
	*/
	{
	// Detach the security associations, if any
	for (TInt i = 0; i < iCount; ++i)
		{
		(*this)[i].iSA.None();
		}
	}

class CProtocolIpsec : public CIp6Hook
	/**
	* Internal protocol for ESP, AH, etc.
	*
	* Internal instance which gets to handle the
	* specific protocols AH, ESP and UPD for inbound
	* packets.
	*/
	{
public:
	CProtocolIpsec(CProtocolSecpol &aSecpol) : iSecpol(aSecpol)
		{};
	~CProtocolIpsec();
	void Identify(TServerProtocolDesc *aEntry) const
		{ IPSEC::IdentifySecpol(*aEntry); }
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
		{ return iSecpol.TransformL(aPacket, aInfo); }
	/**
	* The "master" parent.
	*
	* This "protocol" has no modifiable members, it borrows
	* everything from the parent CProtocolSecpol.
	*/
	CProtocolSecpol &iSecpol;
	};

CProtocolIpsec::~CProtocolIpsec()
	/**
	* Destructor.
	*
	* Cancel all binds with the network using CProtocolInet6Binder::Unbind(),
	* if the network is still attached
	*/
	{
	if (iSecpol.NetworkService())
		iSecpol.NetworkService()->Protocol()->Unbind(this, 0);
	}

//

#ifdef _LOG

static TInt MaskLength(TUint32 aAddr)
	/**
	* Return count of contiguous most significant 1-bits in TUint32
	*/
	{
	TInt count = 0;
	// obviously, this is "brute force" counting
	while (aAddr & 0x80000000)
		{
		count++;
		aAddr <<= 1;
		}
	return count;
	}

static TInt MaskLength(const TIp6Addr &aAddr)
	/**
	* Return count of contiguous most significant 1-bits in IPv6 address.
	*/
	{
	TInt count = 0;
	for (TUint i = 0; i < sizeof(aAddr.u.iAddr8) / sizeof(aAddr.u.iAddr8[0]); ++i)
		if (aAddr.u.iAddr8[i] == 0xFF)
			count += 8;
		else
			{
			count += MaskLength(aAddr.u.iAddr8[i] << 24);
			break;
			}
	return count;
	}

class TAddressBuf : public TBuf<70>
	{
public:
	TAddressBuf(const TIpAddress &aAddr, TUint16 aPort = 0);
	};

TAddressBuf::TAddressBuf(const TIpAddress &aAddr, TUint16 aPort)
	{
	TInetAddr addr(aAddr, 0);
	addr.SetScope(aAddr.iScope);
	addr.OutputWithScope(*this);
	if (aPort)
		{
		_LIT(KFormat, "#%u");
		AppendFormat(KFormat, (TInt)aPort);
		}
	}

static void LogSelectorInfo(const TDesC &aStr, const RPolicySelectorInfo &aSelector, const TPolicyFilterInfo &aFilter)
	{
	_LIT(KIn, "inbound ");
	_LIT(KOut, "outbound ");
	_LIT(KBoth, "");

	TAddressBuf local(aSelector.iLocal(), (aSelector.iFlags & KTransportSelector_PORTS) == 0 ? 0 : aSelector.iPortLocal);
	TAddressBuf remote(aSelector.iRemote(), aSelector.iPortRemote);

	const TUint mode = aFilter.iFlags & KPolicyFilter_SYMMETRIC;

	_LIT(KPorts, "%S%Sif=%d local=%S remote=%S prot=%d");
	_LIT(KTypeCode, "%S%Sif=%d local=%S remote=%S prot=%d type=%d code=%d");

	Log::Printf((aSelector.iFlags & KTransportSelector_PORTS) == 0 ? KTypeCode() : KPorts(),
		&aStr,
		mode == KPolicyFilter_INBOUND ? &KIn() : mode == KPolicyFilter_OUTBOUND ? &KOut() : &KBoth(),
		(TInt)aFilter.iIndex,
		&local, &remote,
		(TInt)aSelector.iProtocol,
		(TInt)((aSelector.iPortLocal >> 8) & 0xFF),
		(TInt)(aSelector.iPortLocal & 0xFF));
	}

static void LogBundleItem(const TDesC &aStr, const CPolicyAction &aItem, const CSecurityAssoc *aSA = NULL)
	{
	_LIT(KTunnel, "tunnel");
	_LIT(KOptional, "?");
	_LIT(KObligatory, "");

	TBuf<15> spi;
	if (aSA)
		{
		spi.Format(_L(" spi %u"), ByteOrder::Swap32(aSA->SPI()));
		}
	TAddressBuf tmp(aItem.iTunnel());
	Log::Printf(_L("%S%S%S(%S)%S"),
		&aStr,
		aItem.iOptional ? &KOptional : &KObligatory, 
		aItem.iSpec == NULL ? &KTunnel : aItem.iSpec->iName,
		&tmp,
		&spi);
	}

static void LogAddressSelector(TDes &aBuf, const TDesC &aLabel, const TIpAddress &aAddr, const TIpAddress &aMask)
	{
	if (!aMask.IsUnspecified() || aMask.iScope)
		{
		TBuf<70> str;
		TInetAddr addr(aAddr, 0);
		addr.SetScope(aAddr.iScope);
		addr.Output(str);
		aBuf.AppendFormat(_L("%S%S "), &aLabel, &str);
		const TInt mlen = MaskLength(aMask);
		if (mlen < 128)
			{
			// Non-trivial mask, output it.
			TInetAddr msk(aMask, 0);
			if (aAddr.IsV4Mapped() && mlen >= 64)
				msk.SetAddress(BigEndian::Get32(&aMask.u.iAddr8[12]));
			msk.Output(str);
			aBuf.AppendFormat(_L(" mask=%S "), &str);
			}
		}
	}

static void LogPolicySelector(const TDesC &aStr, const CPolicySelector &aPS)
	{
	// The following code assumes that partial masks (not covering the full field) are
	// only possible for address fields. For others, the non-zero mask is assumed to be
	// all-ones.
	TBuf<200> buf(aStr);
	if (aPS.iFilterData & KPolicyFilter_FINAL)
		buf.Append(_L("final "));
	if ((aPS.iFilterMask & KPolicyFilter_MERGE) == 0)
		buf.Append(_L("merge "));
	switch (aPS.iFilterData & KPolicyFilter_SYMMETRIC)
		{
	case KPolicyFilter_INBOUND:
		buf.Append(_L("inbound "));
		break;
	case KPolicyFilter_OUTBOUND:
		buf.Append(_L("outbound "));
		break;
	default:
		break;
		}
	//UMA support REQ417-40027
	switch (aPS.iFilterData & KPolicyFilter_Exception)
	    {
	    case KPolicyFilter_Exception:
	        buf.Append(_L("UMAExceptionTrafficSelector"));
	        break;

	    default:
	        break;
	    }

	if (aPS.iInterface)
		buf.AppendFormat(_L("if=%d "), aPS.iInterface->iInterfaceIndex);
	for (CTransportSelector *ts = aPS.iTS; ts != NULL; ts = ts->iOr)
		{
		LogAddressSelector(buf, _L("remote="), ts->iData.iRemote(), ts->iMask.iRemote());
		LogAddressSelector(buf, _L("local="),  ts->iData.iLocal(),  ts->iMask.iLocal());
		if (ts->iData.iFlags & KTransportSelector_PORTS)
			{
			if (ts->iMask.iPortLocal)
				buf.AppendFormat(_L("local_port=%u "), ts->iData.iPortLocal);
			if (ts->iMask.iPortRemote)
				buf.AppendFormat(_L("remote_port=%u "), ts->iData.iPortRemote);
			}
		else
			{
			if (ts->iMask.iPortLocal & 0xFF00)
				buf.AppendFormat(_L("type=%u "), ts->iData.iPortLocal >> 8);
			if (ts->iMask.iPortLocal & 0x00FF)
				buf.AppendFormat(_L("code=%u"), ts->iData.iPortLocal & 0xFF);
			}
		Log::Write(buf);
		buf = _L("\t\t| ");
		}
	Log::Write(buf);
	}

#endif

//


CProtocolSecpol::CProtocolSecpol()
	/**
	* Constructor.
	*/
	{
	iSAPlist.SetOffset(_FOFF(CProviderSecpol,iSAPlink));

	// The inbound associations don't need any special processing
	// for SA status changes. Just put dummy callback routine in.
	for (TInt i = KIpsecMaxNesting; --i >= 0; )
		iSA[i].Init(NoCallback);
	}

CProtocolSecpol::~CProtocolSecpol()
	/**
	* Destructor.
	*/
	{
	// Deregister event notifications, if still active
	if (iEventService)
		iEventService->RemoveListener(this);
	
	// Detach all linked assocations, if any left
	for (TInt i = KIpsecMaxNesting; --i >= 0; )
		iSA[i].None();

	delete iPolicy;
	iPolicy = NULL;

	delete iProtocolIpsec;

	// Cleanup the fragment information
	while (iFrags)
		{
		CIpsecFragmentInfo *tmp = iFrags;
		iFrags = iFrags->iNext;
		delete tmp;
		}

	if (iAssociationManager)
		{
		iAssociationManager->Close();
		iAssociationManager = NULL;
		}

	if (NetworkService())
		{
		// The IPsec is going away, all flows need to be
		// re-connected...
		NetworkService()->SetChanged();
		}

	// If assumptions are correct, iSAPlist should be empty!!
	ASSERT(iSAPlist.IsEmpty());
#if __WINS__
	LOG(Log::Printf(_L("CProtocolSecpol::~CProtocolSecpol() done IPSEC_OBJECT_COUNT=%d"), IPSEC_OBJECT_COUNT));
#endif
	}

void CProtocolSecpol::Identify(TServerProtocolDesc *aInfo) const
	/**
	* @retval aInfo A minimal TServerProtocolDesc to satisfy the Socket server.
	*/
	{
	IPSEC::IdentifySecpol(*aInfo);
	}

TUint32 CProtocolSecpol::GetInterfaceIndex(const TDesC &aName)
	/**
	* Find interface index by interface name.
	*
	* Query the interface index from the network layer. The query uses
	* @code
	* MInterfaceManager::GetOption(KSolInetIfQuery, KSoInetIfQueryByName, opt),
	* @endcode
	* where the opt is TSoInetIfQuery.
	*
	* @param aName The interface name
	* @return The interface index (= 0, if not found)
	*/
	{
	MInterfaceManager *const mgr = NetworkService()->Interfacer();
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iName = aName;
	opt().iIndex = 0;
	// Ignore GetOption error. If interface does not exist yet,
	// this returns ZERO.
	(void)mgr->GetOption(KSolInetIfQuery, KSoInetIfQueryByName, opt);
	return opt().iIndex;
	}

TBool CProtocolSecpol::UpdateInterfaceIndex(const TDesC &aName, TUint32 aIndex)
	/**
	* Update interface index in the cached interface in the SPD.
	*
	* @param aName The interface name
	* @param aIndex The interface index.
	* @return ETrue, if name exists, and EFalse otherwise.
	*
	* The CSelectorInterface contains a cached interface in CSecurityPolicy.
	*
	* If the aIndex == 0, then ask the interface index (GetInterfaceIndex())
	* from the network service.
	*
	* If the aIndex > 0, assume it is correct for the named interface
	* (not verified).
	*/
	{
	if (iPolicy == NULL)
		return EFalse;
	for (CSelectorInterface *si = iPolicy->iInterfaces; si != NULL; si = si->iNext)
		{
		if (aName.Compare(*si->iName) == 0)
			{
			if (aIndex != 0 || (aIndex = GetInterfaceIndex(aName)) != 0)
				{
				LOG(Log::Printf(_L("<>\tipsec Interface Index Update IF [%S] index %d -> %d"),
						&aName, si->iInterfaceIndex, aIndex));
				si->iInterfaceIndex = aIndex;
				}
			// The name was located!
			return ETrue;
			}
		}
	// The name not located.
	return EFalse;
	}


// CProtocolSecpol::InterfaceAttached
// **********************************
void CProtocolSecpol::InterfaceAttached(const TDesC &aName, CNifIfBase *)
	/**
	* Interface has attached to the network layer.
	*
	* This exists only because the interface create/destroy does not
	* currently generate events. And, this event only occurs when a
	* real NIF attached.
	*
	* The IPsec will not know if someone creates a "virtual" interface
	* and does not specify any addresses, or otherwise modify it.
	*
	* At this point the interface index is not known, use the UpdateInterfaceIndex()
	* without specified index (aIndex = 0), and it will query the index, if this
	* interface is referenced in the current policy.
	*
	* @param aName The interface name
	* @param aIf Not used.
	*/
	{
	(void)UpdateInterfaceIndex(aName, 0);
	}


// CProtocolSecpol::Notify
// ***********************
void CProtocolSecpol::Notify(TUint aEventClass, TUint aEventType, const void *aData)
	{
	(void)aEventType; // (just silence warning for release compilation)
	if (aEventClass != EClassInterface)
		return;
	
	const TInetInterfaceInfo &ii = *(TInetInterfaceInfo *)aData;

	if (UpdateInterfaceIndex(ii.iName, ii.iIndex))
		return;
	LOG(Log::Printf(_L("<>\tipsec Interface Event(%d) IF [%S] index %d not used in policy"),
					aEventType, &ii.iName, ii.iIndex));
	}


//	*********************************
//	CProtocolSecpol::NetworkAttachedL
//	*********************************
//	Do the bindings to the IP layer
void CProtocolSecpol::NetworkAttachedL()
	/**
	* The bind to TCP/IP stack has been processed.
	*
	* The TCP/IP stack (MNetworkService) is now available and IPSEC can
	* add the required hooks to it and do other initializations.
	*/
	{
	// If policy is loaded, need to redo the interfaces
	// (network may have been down, and indexes changed)
	if (iPolicy)
		FixupInterfaceIndexes(iPolicy);

	// Create inbound internal protocol only when really needed.
	if (iProtocolIpsec == NULL)
		iProtocolIpsec = new (ELeave) CProtocolIpsec(*this);

	NetworkService()->BindL(iProtocolIpsec, BindHookFor(KProtocolInetAh));
	NetworkService()->BindL(iProtocolIpsec, BindHookFor(KProtocolInetEsp));
	NetworkService()->BindL(iProtocolIpsec, BindHookFor(KProtocolInetUdp));

	NetworkService()->BindL(this, BindHookAll());    // All inbound packets
	NetworkService()->BindL(this, BindFlowHook(1));  // All outbound packets

	// To handle tunneled fragments within IPSEC, need to catch the IPv6
	// Fragment Header and IPv4-in-IP headers
	NetworkService()->BindL(iProtocolIpsec, BindHookFor(KProtocolInet6Fragment));
	NetworkService()->BindL(iProtocolIpsec, BindHookFor(KProtocolInetIpip));

	iEventService = IMPORT_API_L(NetworkService()->Interfacer(), MEventService);
	iEventService->RegisterListener(this, EClassInterface);
	}

//	********************************
//	CProtocolSecpol::NetworkDetached
//	********************************
void CProtocolSecpol::NetworkDetached()
	{
	/**
	* The biding to TCP/IP is being cut.
	*
	* Do only the internal housekeeping. The calling CProtocolPostHook code
	* does all the necessary protocol Unbinding from the network, this does
	* not need Unbind explicitly any of the bindings done in NetworkAttachedL.
	*/
	ASSERT(iEventService != NULL);

	iEventService->RemoveListener(this);
	iEventService = NULL;
	delete iProtocolIpsec;
	iProtocolIpsec = NULL;
	}

//  ************************
//  CProtocolSecpol::BindToL
//  ************************
void CProtocolSecpol::BindToL(CProtocolBase *aProtocol)
	/**
	* Bind Secpol protocol to other protocols.
	*
	* This calls comes from the socket server, when it processed the "bindto/bindfrom" directives
	* of the ESK files: "[secpol] bindto= ip6,pfkey". The secpol only accepts protocols
	* KProtocolInet6Ip and KProtocolKey. Trying to bind anything else will result an error leave.
	*
	* The base class CProtocolPostHook::DoBindToL handles the KProtoclInet6Ip (network) binding
	* and calls NetworkAttachedL method, it that happens.
	* This code only needs to recognize the PFKEY (KProtocolKey) binding. The PFKEY provides
	* MAssociationManager.
	*
	* @param aProtocol The target protocol to bind to.
	*/
	{
	const TUint id = (TUint)DoBindToL(aProtocol);
	if (iAssociationManager == NULL && (iAssociationManager = IPSEC::FindAssociationManager(aProtocol, id)) != NULL)
		{
		iAssociationManager->Open();
		}
	else if (id)
		User::Leave(KErrGeneral);
	}

// **********
// CIpsecHook
// **********
CIpsecHook::CIpsecHook(
	MAssociationManager &aMgr,
	CFlowContext &aFlow,
	const RPolicySelectorInfo &aInfo,
	const TInt aCount,
	const TInt aTunnels) :
   iMgr(aMgr), iFlow(aFlow), iInfo(aInfo), iCount((TUint8)aCount), iTunnels((TUint8)aTunnels)
	{
	iMgr.Open();	// Association manager needs exist while this CIpsecHook exists.
	};

// ***********************
// CIpsecHook::~CIpsecHook
// ***********************
CIpsecHook::~CIpsecHook()
	{
	// Detach associated Security Assocations
	for (TInt i = 0; i < iCount; ++i)
		{
		RHookSA &h = Assoc(i);
		h.None();
		h.iItem->Close();
		}

	// Close tunnel src address endpoints
	for (TInt j = 0; j < iTunnels; ++j)
		{
		RIpAddress &addr = Tunnel(j);
		addr.Close();
		}

#ifdef IPSEC_OBJECT_TRACKING
	// Close the remote/local RIpAddress (not really
	// necessary, but automatic destructors are run
	// so late that object counting debug would cause
	// spurious panic...
	iInfo.iRemote.Close();
	iInfo.iLocal.Close();
#endif

	iMgr.Close();	// Release the association manager
	LOG(Log::Printf(_L("\tMFlowHook[%u] destruct"), (TUint)this));
	}

// ****************
// CIpsecHook::NewL
// ****************
CIpsecHook *CIpsecHook::NewL
	(MAssociationManager &aMgr,
	CFlowContext &aFlow,
	const RPolicySelectorInfo &aInfo,
	TInt aCount, CPolicyAction **aItems,
	TInt aTunnels, const RIpAddress *aSrc)
	/**
	* Create a new IPSEC hook handle object.
	*
	* Allocating space only as much as is absolutely necessary.
	*
	* @param aMgr		The association manager
	* @param aFlow		The flow context
	* @param aInfo		The policy selector information
	* @param aCount		The number of transforms (includes plain tunnels)
	* @param aItems		The transforms (aCount)
	* @param aTunnels	The number of tunnels within transforms
	* @param aSrc		The tunnel source addresses
	*/
	{
	const TInt extra = aCount * sizeof(RHookSA) + aTunnels * sizeof(RIpAddress);

	CIpsecHook *const handle = new (extra) CIpsecHook(aMgr, aFlow, aInfo, aCount, aTunnels);
	if (handle == NULL)
		User::Leave(KErrNoMemory);
	
	// Initialize RAssociation Hooks
	for (TInt i = 0; i < aCount; ++i)
		(void)new (&handle->Assoc(i)) RHookSA(aFlow, aItems[i]);

	// Initialize tunnel srsc addresses
	for (TInt j = 0; j < aTunnels; ++j)
		{
		(void)new (&handle->Tunnel(j)) RIpAddress(aSrc[j]);
		}
	return handle;
	}

TInt CProtocolSecpol::SelectSource(const RIpAddress &aDst, RIpAddress &aSrc) const
	/**
	* Select the source address for a destination.
	*
	* @param aDst The destination address
	* @retval aSrc The selected source address
	* @return KErrNone
	*/
	{
	TIp6Addr src;
	if (NetworkService()->Interfacer()->CheckRoute(aDst(), aDst().iScope, src) != KErrNone)
		return EIpsec_NoInnerSource;    // Could not find the src addres!
	// ARGHH! Checkroute does not return the source scope id...
	// NEED to fetch some value by uncertain methods..
	TUint32 scope = NetworkService()->Interfacer()->LocalScope(src, aDst().iScope, (TScopeType)(aDst().Scope()-1));
	aSrc.Set(src, scope);
	return KErrNone;
	}


static void AfterTunnelAction(RPolicySelectorInfo &aInfo)
	/**
	* Change policy selector to match state after applied tunnel.
	*
	* - ports = 0
	* - protocol #KProtocolInetIpip or #KProtocolInet6Ipip
	* 
	* Internal help function. Assume iLocal/iRemote have already
	* been updated, if required.
	*/
	{
	aInfo.iPortLocal = 0;
	aInfo.iPortRemote = 0;
	aInfo.iFlags &= ~KTransportSelector_PORTS;
	aInfo.iProtocol = aInfo.iRemote().IsV4Mapped() ? KProtocolInetIpip : KProtocolInet6Ipip;
	}

TInt CProtocolSecpol::CollectBundle(TPolicyFilterInfo &aFilter, RPolicySelectorInfo &aKey, const TInt aMaxItems,
	 CPolicyAction **aItems , RIpAddress *aSrc, TInt &aTunnels) const
	/**
	* Match the packet against the policy selectors and collect the IPsec actions.
	*
	* The function may modify the aFilter and aKey parameters.
	* 
	* @retval aFilter The filter
	* @retval aKey The packet information
	* @param aMaxItems Max limit for actions.
	* @retval aItems The collected actions.
	* @retval aSrc The current source address.
	* @retval aTunnels The number of tunnels.
	* @return
	*	@li >= 0. The number of actions.
	*	@li EIpsec_NoSelectorMatch, packet does not match policy
	*	@li EIpsec_MaxTransforms, too many actions triggered.
	*	@li EIpsec_NoInnerSource, cannot select tunnel source (error is now misnamed).
	*/
	{
	TInt result = EIpsec_NoSelectorMatch;
	aTunnels = 0;
	TInt i = 0;
	
	//UMA support REQ417-40027
   TBool exception_flag = EFalse;
   TBool exception_drop_flag = EFalse;
   
   
   if (iIPSecGANSupported)
       {
      //Check for exception selector being loaded or not. This is required for Drop mode policy
      //If policy is set as 
      //                  inbound = drop
      //                  outbound = drop
      // then instead returning Excetion selectors should be checked.
       CheckExceptionSelector(exception_flag);
       }
   

	for (CPolicySelector *ps = iPolicy->iSelectors; ps != NULL; ps = ps->iNext)
		{
		// 1. Check filtering
		if (((aFilter.iFlags ^ ps->iFilterData) & ps->iFilterMask) ||
			(ps->iInterface && ps->iInterface->iInterfaceIndex != aFilter.iIndex))
			continue;	// -- this selector filtered out, try next.
		// 2. check transport selectors (empty TS matches all)
		if (ps->iTS == NULL || ps->iTS->Match(aKey))
			{
			if (ps->iFilterData & KPolicyFilter_DROP)
                {
                // UMA support REQ 417-40027
                if (iIPSecGANSupported && exception_flag)
			       {
                    //Work around for Exceptions                
                    exception_drop_flag = ETrue;
                    break;	
			       }

                return EIpsec_NoSelectorMatch;
                }

			const TInt N = ps->iActions.Count();
			for (TInt k = 0; k < N; k++)
				{
				CPolicyAction *const action = ps->iActions[k];
				if (action == NULL)
					continue;
				if (i == aMaxItems)
					return EIpsec_MaxTransforms;
				aItems[i++] = action;
				if (!action->iTunnel().IsNone())
					{
					// After tunneling, any futher matching is based on the
					// selectors of a tunnel (addresses only, no ports)
					aFilter.iFlags |= KPolicyFilter_TUNNEL;
					if (!KOuterIsInner.IsEqual(action->iTunnel()))
						{
						aKey.iRemote = action->iTunnel;
						if (SelectSource(aKey.iRemote, aKey.iLocal) != KErrNone)
							return EIpsec_NoInnerSource;
						}
					AfterTunnelAction(aKey);
					// Return list of source addresses, if the target array is supplied
					if (aSrc)
						aSrc[aTunnels] = aKey.iLocal;
					aTunnels++;
                    }//if action
                }//for
//UMA support REQ417-40027
            if(iIPSecGANSupported)
                {
                    if(!exception_drop_flag)
         	  		{
                    //Check if drop mode is set with exception selectors...out from this result loop
                    //and jump onto Expection selector handling.
                    result = i;
                    if (ps->iFilterData & KPolicyFilter_FINAL)
                        break;  // Final selector, no merge allowed!
                    aFilter.iFlags |= KPolicyFilter_MERGE;  // Mark "merge only".
     				}
                }
            else
                {
            result = i;
            if (ps->iFilterData & KPolicyFilter_FINAL)
                break;  // Final selector, no merge allowed!
            aFilter.iFlags |= KPolicyFilter_MERGE;  // Mark "merge only".
                }
            }
          }
	if (iIPSecGANSupported)
	    {
    if (result == EIpsec_NoSelectorMatch && exception_flag)
        {
        // No selector matches above. Check for Exception selector     
        for (CPolicySelector *ps = iPolicy->iSelectors; ps != NULL; ps = ps->iNext)
            {
            if(((ps->iFilterData & KPolicyFilter_Exception) !=  KPolicyFilter_Exception))
                {
                //Skip till Exception selector is loaded 
                continue;
                }//if
            else
                {
                if (ps->iInterface && ps->iInterface->iInterfaceIndex != aFilter.iIndex)
                    {
                    continue; ////Sanity Check;
                    }
                if(aFilter.iFlags & KPolicyFilter_OUTBOUND )
                    {
                    //Check for Scopes for packet going out. Scope should be the tunnel end networkID configured for Tunnel need Exception
                    TIpAddress iAddr = aKey.iLocal();
                    TInt scope = iAddr.iScope;
                    if(scope == ps->iScope_Exception)
                        {
                        LOG(Log::Printf(_L("Exception outbound- \n exception scope = %d\n, packet scope = %d\n"), ps->iScope_Exception, scope));
                        result = NULL;
                        break;
                        }
                    }//outbound filter
                if(aFilter.iFlags & KPolicyFilter_INBOUND)
                    {
                    //TODO comments
                    TIpAddress iAddr = aKey.iRemote();
                    TInt scope = iAddr.iScope;
                    if(scope== ps->iScope_Exception)
                        {
                        LOG(Log::Printf(_L("Exception inbound- \n exception scope = %d\n ,packet scope = %d\n"), ps->iScope_Exception, scope));
                        result = NULL;
                        break;
                        }
                    }//INBOUND
                }//else            
            }//for
            }//exception CHECK
        }///if (iIPSecGANSupported)
		return result;
	}



MFlowHook *CProtocolSecpol::OpenL(TPacketHead &aHead, CFlowContext *aFlow)
	/**
	* Attach IPsec processing to a flow (if needed).
	*
	* OpenL is called once when the flow is opened. It should decide
	* whether further calls are necessary (ReadyL/ApplyL) for this flow.
	*
	* When called, the IPsec selector information is in TPacketHead.
	*	
	* If OpenL() notices that the flow will never be opened,
	* it will leave with error, causing the flow to be cancelled.
	*
	* The OpenL tasks:
	*
	* @li	Consult the Security Policy and find out what actions
	*		need to be done for this flow: reject, pass clear or
	*		apply IPsec.
	* @li	If there are IPsec actions, then allocate a
	*		CIpHook object to handle the processing of the flow.
	* @li	If the actions include tunneling, update the flow
	*		selector information to match the outermost tunnel.

	* @param aHead	The flow information
	* @param aFlow	The flow context (always NON-NULL)
	*
	* @return CIpsecHook or NULL.
	*/
	{
	if (Policy() == NULL)
		{
		LOG(Log::Printf(_L("CProtocolSecpol::OpenL() -- No IPSEC policy loaded")));
		return NULL;    // No Policy, no IPSEC available
		}
	if (!iAssociationManager)
		{
		LOG(Log::Printf(_L("CProtocolSecpol::OpenL() -- PFKEY not avaiable")));
		return NULL;    // No CProtocolKey, no IPSEC available
						// (should this cause packet drops instead?)
		}

	TPolicyFilterInfo filter;
	filter.iIndex = aHead.iInterfaceIndex;
	filter.iFlags = KPolicyFilter_OUTBOUND;

	iPktInfo.FillZ();
	iPktInfo.iProtocol = aHead.iProtocol;
	for (const TSnoopHeader *f = snooper; f < snooper_end; ++f)
		{
		if (iPktInfo.iProtocol == f->iProtocol)
			{
			if (f->iSelector <= KLoadLocal)
				{
				iPktInfo.iFlags |= KTransportSelector_PORTS;
				iPktInfo.iPortLocal = aHead.iSrcPort;
				iPktInfo.iPortRemote = aHead.iDstPort;
				}
			else
				{
				iPktInfo.iPortLocal = (TUint16)((aHead.iIcmpType << 8) | aHead.iIcmpCode);
				iPktInfo.iPortRemote = iPktInfo.iPortLocal;
				}	
			break;
			}
		}
	iPktInfo.iLocal.Set(aHead.ip6.SrcAddr(), aHead.iSrcId);
	iPktInfo.iRemote.Set(aHead.ip6.DstAddr(), aHead.iDstId);

	CPolicyAction *items[KIpsecMaxNesting];		// Collected actions [0..count-1]
	
	TInt tunnels = 0;
	LOG(LogSelectorInfo(_L("OpenL\t"), iPktInfo, filter));
	RPolicySelectorInfo final_info = iPktInfo;
	TInt count = CollectBundle(filter, final_info, KIpsecMaxNesting, items, iMyself, tunnels);
	if (count < 0)
		{
		LOG(Log::Printf(_L("\t*DROP*")));
		User::Leave(count);
		}
	else if (count == 0)
		{
		ASSERT(tunnels == 0);
		LOG(Log::Printf(_L("\t*PASS*")));
		return NULL;
		}
	ASSERT(tunnels <= count);

	CIpsecHook *handle = CIpsecHook::NewL(*iAssociationManager, *aFlow, iPktInfo, count, items, tunnels, iMyself);

	LOG(Log::Printf(_L("\tMFlowHook[%u] attached"), (TUint)handle));

	if (tunnels)
		{
		// If tunneling happened, then update the packet head
		// information accordingly.
		
		aHead.ip6.SetDstAddr(final_info.iRemote());
		aHead.iDstId = final_info.iRemote().iScope;
		aHead.iSourceSet = 1;
		aHead.ip6.SetSrcAddr(final_info.iLocal());
		aHead.iSrcId = final_info.iLocal().iScope;
		aHead.iProtocol = final_info.iProtocol;
		aHead.iDstPort = 0;
		aHead.iSrcPort = 0;
		aHead.iIcmpType = 0;
		aHead.iIcmpCode = 0;
		// should set proper tunnel protocol? 4 or 41 depending on outermost inner header?
		aHead.ip6.SetVersion(aHead.ip6.DstAddr().IsV4Mapped() ? 4 : 6);

		// Comment on setting: aHead.iFragment = 1
		// ---------------------------------------
		// It is possible to request fragmentation before the
		// IPSEC processing, if the IPSEC is adding the tunnel.
		// *HOWEVER* This would be legal ONLY IF the tunneling
		// SA is the FIRST (or only) transformation. If the policy
		// actions says something like
		//		ESP + AH(tunnel)
		// then ESP is in transport mode, and applying that to
		// fragments is not ALLOWED. The fragmenting should happen
		// AFTER the ESP transformation, and before the tunneling.
		//
		// This is not possible with current IPSEC hook.
		// To enable such feature, either
		// 1) IPSEC must do the fragmenting self
		// 2) IPSEC processing needs to be split into two
		//	flow hooks, the first doing the ESP (transport only), and
		//	the second requesting fragmentation and doing tunnel + AH
		//	(and possible remaining transforms).
		}
	return handle;
	}

void CIpsecHook::Close()
	/**
	* Decrement reference count.
	*/
	{
	CIpsecReferenceCountObject::Close();
	}

void CIpsecHook::Open()
	/**
	* Increment reference count.
	*/
	{
	CIpsecReferenceCountObject::Open();
	}

TInt CIpsecHook::ReadyL(TPacketHead &aHead)
	/**
	* ReadyL implementation for IPsec MFlowHook.
	*
	* The ReadyL calls "propagate" interface ready state up the
	* flow. The calls to hooks are made in reverse order, the
	* "closest to interface" is called first.
	*
	* This function has following tasks
	*
	* @li	Verify that all Security Associations (SA) required by the
	*		actions (CPolicyAction) collected in OpenL, are
	*		available (call MAssociationManager::Acquire() for
	*		each).
	* @li	If any of the required SA's is not yet available, then
	*		block the flow (and ReadyL) process by returning
	*		EFlow_PENDING. When the SA's become available, there
	*		will be a callback (RHookSA::Callback()). which can
	*		set the flow back to READY state and trigger a rerun
	*		of the ReadyL chain.
	*
	* @param aHead The packet flow information.
	* @return
	* 	@li	== 0, hook is ready, proceed to refresh the next one or
	*		mark the flow as READY, if this was the first hook
	*	@li	> 0 (EFlow_PENDING), hook is not ready, the ReadyL calling
	*		is stopped and flow is set to pending state.
	*	@li	< 0, hook detected a permanent error condition. The flow
	*		goes into error state.
	*
	*  *WARNING*
	*      The ReadyL method can be called multiple times!
	*/
	{
	TInt result = KErrNone;
	TInt overhead = 0;

	// ... could ASSERT that current dst/src in packet head is
	// exactly the same as at the end of the OpenL above.
	
	// Restore upper layer state
	aHead.ip6.SetSrcAddr(iInfo.iLocal());
	aHead.iSrcId = iInfo.iLocal().iScope;
	aHead.ip6.SetDstAddr(iInfo.iRemote());
	aHead.iDstId = iInfo.iRemote().iScope;
	if (iInfo.iFlags & KTransportSelector_PORTS)
		{
		aHead.iSrcPort = iInfo.iPortLocal;
		aHead.iDstPort = iInfo.iPortRemote;
		aHead.iIcmpType = 0;
		aHead.iIcmpCode = 0;
		}
	else
		{
		aHead.iSrcPort = 0;
		aHead.iDstPort = 0;
		aHead.iIcmpType = (TUint8)iInfo.iPortLocal;
		aHead.iIcmpCode = (TUint8)(iInfo.iPortLocal >> 8);
		}
	aHead.ip6.SetVersion(aHead.ip6.DstAddr().IsV4Mapped() ? 4 : 6);

	const RIpAddress *src = &iInfo.iLocal;
	const RIpAddress *dst = &iInfo.iRemote;

	LOG(Log::Printf(_L("ReadyL\tMFlowHook[%u]"), (TUint)this));

	// Try to acquire the actual SA's required by the bundle

	//
	// Goes through all SA's, even if some of them fail. This is to
	// get all possible ACQUIRE requests queued on one packet.
	//
	// When entering ready, the head contains the final destination
	// address as defined by possible tunnels. However, this can be
	// ignored, as we have the saved original destination which should
	// be in the packet head after this ReadyL. (And the final destionation
	// should be original destination or the last tunnel from the bundle.
	TInt j = 0;
	TInt tunnel_mode = 0;
	RPolicySelectorInfo info = iInfo;
	for (TInt i = 0; i < iCount; ++i)
		{
		RHookSA &h = Assoc(i);

		// Save current dst/src for the case of optional item skipped.
		const RIpAddress *const dst_opt = dst;
		const RIpAddress *const src_opt = src;

		if (!h.iItem->iTunnel().IsNone())
			{
			if (!KOuterIsInner.IsEqual(h.iItem->iTunnel()))
				dst = &h.iItem->iTunnel;	// destination changed
			src = &Tunnel(j++);				// Outer Source Address!
			tunnel_mode = 1;
			}
		else
			tunnel_mode = 0;

		CSecurityAssoc *sa = NULL;
		if (h.iItem->iSpec == NULL)
			h.iState = KErrNone;	// Always ready, no SA needed.
		else
			{
			// Tranformation needs an IPSEC Security Association
			// Because ReadyL can be called multiple times, it is
			// possible to have associations already setup at this
			// point (or only some of them not available yet).
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
			if(h.iItem->iSpec->iPropList->Count() > MAX_PROPOSALS_PER_SA)
				{
				User::Leave(EIpsec_MaxTransforms);
				}
			TInt res2 = iMgr.Acquire(sa, h.iItem->iSpec->iSpec, h.iItem->iSpec->iPropList, h.iItem->iTS, *src, *dst, info, tunnel_mode); 
#else
			TInt res2 = iMgr.Acquire(sa, h.iItem->iSpec->iSpec, h.iItem->iTS, *src, *dst, info, tunnel_mode);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
			// Note: Acquire returns
			//  - KErrNone = EFlow_READY, when SA already exists and is usable
			//  - KRequestPending, when a larval SA is created and keymanagement Acquire in progress
			//  - KErrNotFound, when SA cannot be allocated for some reason (fatal error, abort flow!)
			if (res2 == KRequestPending)
				{
				// The following test is temporary work-around for a
				// bug in PFKEY implementation: the PKFEY does not properly
				// check whether the SA reply to ACQUIRE matches request. It
				// just changes the matching egg SA into MATURE state.
				// If above acquire returns pending (no matching SA exists),
				// and we already have a "MATURE" SA assigned, then assume
				// the key manager is not providing a correct SA for the ACQUIRE.
				// To prevent continous ACQUIRE looping, abort flow.
				const CSecurityAssoc *const sa2 = h.Association();
				if (sa != sa2 && sa2 && sa2->State() == SADB_SASTATE_MATURE)
					res2 = EIpsec_AcquireFailed;
				else
					res2 = EFlow_PENDING;
				}
			h.iState = res2;       // Remember the last result for SA.
#ifdef _LOG
			if (res2 > 0)
				LogBundleItem(_L("\t\tWaiting SA: "), *h.iItem, sa);
			else if (res2 == 0)
				LogBundleItem(_L("\t\tSA ready: "), *h.iItem, sa);
			else
				LogBundleItem(_L("\t\tSA unavailable: "), *h.iItem, sa);
#endif
			//
			// Keep the first error as final error... however, fatal error
			// state ( < 0) should override any pending error state ( > 0).
			//
			if (h.iItem->iOptional)
				{
				if (res2 != KErrNone)
					{
					// Optional item not ready, cancel address changes.
					// (this is not 100% correct--now the following
					// SA's are requested with the old addresses, and
					// will become incorrect, if the optional SA appears
					// at some later stage without ReadyL being rerun!)
					dst = dst_opt;
					src = src_opt;
					tunnel_mode = 0;
					}
				}
			else 
				{
				// Only obligatory items affect final result.
				if (result == KErrNone || (result > 0 && res2 < 0))
					result = res2;
				}
			// For after tunnel, the pktinfo must be changed too..
			if (tunnel_mode)
				{
				info.iRemote = *dst;
				info.iLocal = *src;
				AfterTunnelAction(info);
				}
			}
		//
		// Whatever the result was, attach the new SA (possibly NULL or same as before)
		// to the handle.
		//
		h.Reset(sa);
		//
		// Find out how much overhead this transform adds to a packet
		//
		overhead += iMgr.Overhead(sa, h.iItem->iTunnel());
		}

	if (result == KErrNone)
		{
		// If all SA's are available, the header space requirement
		// is now known (coded in a way that allows this to change
		// dynamicalle, if required)
		iFlow.iHdrSize += overhead;
		}
	return result;
	}

TInt CIpsecHook::ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	/**
	* Apply the IPsec actions to the outgoing packet.
	*
	*  This is part of IPv6 outgoing packet hook processing.
	* @li	aPacket is the outgoing packet. It is *FULL* packet
	*		and already includes the outgoing IPv6 header. If
	*		the hook needs to do any modifications to the outgoing
	*		packet, it must make them into aPacket.
	* @li	aPacket has an info block associated. The info->iLength
	*      *MUST* be maintained, if any change affects it.
	*
	* @param aPacket	The IP packet to process (unpacked).
	* @param aInfo		The unpacked info block associated with the aPacket.
	*
	* @return (as specified for the hook mechanism)
	* @li   Any Leave causes the packet to be dropped
	* @li  = 0, pass on to the next output hook
	* @li  < 0, the hook took "ownership" of the packet (usually
	*      just dropped it explicitly), "Send aborted" return!
	* @li  > 0, This return value is not used at this
	*      point. Do not return with > 0!
	*/
	{
	TInt tunnel = 0;
	const TIpAddress *dst = &iInfo.iRemote();
	for (TInt i = 0; i < iCount; ++i)
		{
		const RHookSA &h = Assoc(i);

		// Save current dst for the case of optional item skipped.
		// (src does not need to be saved, because for non-tunnels,
		// the source is already in the packet, and for tunnels,
		// the source is always taken from the Tunnel(x). And even
		// the saved dst is only needed for the case where outer tunnel
		// dst is defined to be same as inner).
		const TIpAddress *const dst_opt = dst;

		const TIpAddress *tunnel_address = &h.iItem->iTunnel();
		if (!tunnel_address->IsNone())
			{
			// Pass outer src in the info
			TInetAddr::Cast(aInfo.iSrcAddr).SetAddress(Tunnel(tunnel++)());
			// [should also set aInfo.iDstAddr ?]
		
			// Use the previous dst as tunnel address, if tunnel is defined as all-ones
			if (KOuterIsInner.IsEqual(*tunnel_address))
				tunnel_address = dst;
			else
				dst = tunnel_address;
			}
		// *NOTE*
		//  The IPSEC engine ApplyL does not follow the hook Apply return values
		//  fully. It never reseases the packet and error return is just a signal
		//  that transformation failed. Perhaps should do something? For now,
		//  just convert error return to leave! -- msa
		if (h.iState == KErrNone)
			User::LeaveIfError(iMgr.ApplyL(h.Association(), aPacket, aInfo, *tunnel_address));
		else if (!h.iItem->iOptional)
			User::Leave(EIpsec_LostSA);
		else
			dst = dst_opt;
		//
		// ApplyL may notice that SA is DYING or already dead, when DYING the flow
		// should be set to pending after this packet (and renegotiation of SA
		// sould be activated). If DEAD, the same happens, but additionally the
		// packet is dropped.
		}
	return KErrNone;
	}

// CProtocolSecpol::UpdateTunnelInterface
// **************************************
// The problem: when applying IPSEC detunneling, the inner source and destination
// addresses must be acceptable for the "inbound interface". When VPN tunnels are
// in use, the addresses are normally not valid for the real interface and IPsec
// must update the iInterfaceIndex to point to the proper VPN interface, so that
// the checks pass.
//
// Just using the inner destination and searching for any interface that would
// accept packets for that address only work for unique addresses. If there are
// overlapping ranges (10-nets) or IPv6 scoped addresses (site or link local),
// then the scope id is required.
//
// Assume CSecurityAssoc::TunnelIndex() must return the interface index of the
// associated VPN tunnel interface, if non-zero. (this does not care how
// such thing got associated with the SA...)
void CProtocolSecpol::UpdateTunnelInterface(RMBufRecvInfo &aInfo, const CSecurityAssoc *const aSa)
	{
	if (aSa && aSa->TunnelIndex())
		aInfo.iInterfaceIndex = aSa->TunnelIndex();
	}

void CProtocolSecpol::CheckPacketId(RMBufHookPacket &aPacket)
	/**
	* Detect when packet has changed.
	*
	* The IPsec hook can get multiple calls from the same packet
	* to various hooks that it has installed, and they collect state
	* information into the member variables of the CProtocolSecpol.
	*
	* The state information contains
	* - IPsec transforms done
	* - Fragment information 
	*
	* Sometimes, some other hook or component just throws the packet
	* away. IPsec needs to reset the collected information, if packet
	* is changed.
	*
	* The network layer tags each packet with id, which can be used
	* for this purposed (RMBufRecvInfo::HookValue(0)).
	*/
	{
	const TUint32 packet_id = aPacket.HookValue(0);
	ASSERT(packet_id);			// The packet id must be present!
	if (packet_id == iPacketId)
		return;
	
	// A new packet is being processed, scrap previous
	// SA and tunnel information.
	if (iIsFragment)
		{
		CIpsecFragmentInfo **h = &iFrags;
		CIpsecFragmentInfo *f;
		
		// Count the number of currently stored states,
		// and release all that are over the maximum
		// count (from the end of chain, oldest ones first).
		for (TInt count = 0; (f = *h) != NULL;)
			{
			if (++count >= KIpsecMaxFragmentInfo)
				{
				// Too many Fragment states stored, delete
				// the entry
				*h = f->iNext;
				delete f; 
				}
			else
				h = &f->iNext;
			}
		// The other packet was a fragment, need to save
		// the information for later use. Adds the new
		// information in front of the list.
		f = CIpsecFragmentInfo::New(iCountSA);
		if (f)
			{
			f->iId = iId;
			f->iSrc = iSrc;
			f->iDst = iDst;
			for (TInt i = 0; i < f->iCount; ++i)
				{
				// Copy a reference to the SA
				(*f)[i].iSA.Reset(iSA[i].Association());
				(*f)[i].iTunnel = iTunnel[i]();
				}
			f->iNext = iFrags;
			iFrags = f;
			}
		}
	iPacketId = packet_id;
	iCountSA = 0;
	iIsFragment = 0;
	iIsTunnelMode = 0;
	}
	
TInt CProtocolSecpol::CheckFragmentPolicy()
	/**
	* Verify fragment policy.
	*
	* A fragment has been detected. Need to check that if there are other fragments
	* of the same packet, they all have been protected with the same IPsec.
	*/
	{
	CIpsecFragmentInfo **h = &iFrags;
	CIpsecFragmentInfo *f;
	for (;(f = *h) != NULL; h = &f->iNext)
		{
		if (f->iId != iId || f->iSrc != iSrc || f->iDst != iDst)
			continue;

		// A matching fragment information already exists in the
		// list. This means that we have already received other
		// fragment(s) of this packet and now must check that all
		// applied IPSEC is same for all fragments.
		for (TInt i = 0; i < f->iCount; ++i)
			{
			if ((*f)[i].iSA.Association() != iSA[i].Association() ||
				(*f)[i].iTunnel != iTunnel[i]())
				{
				// A mismatch has been detected. Either this or previous
				// fragment had invalid IPSEC applied. There is no way
				// to know which one, thus just drop the current.
				return EIpsec_FragmentMismatch;
				}
			}
		// All applied IPSEC matches, release the stored information
		*h = f->iNext;
		delete f;
		break;
		}
	return KErrNone;
	}

TInt CProtocolSecpol::TransformL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	/**
	* Do IPsec transforms for incoming packet.
	*
	* The CProtocolIpsec binds to AH, ESP and UDP (for NAT traversal), but delegates
	* actual processing to this main class -- CProtocolIpsec::ApplyL() just calls
	* this TransformL() directly.
	*
	* Additionally, CProtocolIpsec hooks the IPv6 Fragment header to detect
	* IPv6 fragments.
	*
	* @li	Recognize the AH and ESP, and unwrap them from the packet,
	*		and remember the transforms as packet state.
	* @li	Recognize the special UDP that does the NAT traversal
	* @li	Recognize fragments that are uncovered by detunneling
	*
	* @param aPacket The incoming packet
	* @param aInfo The packet info.
	* @return KIp6Hook_PASS, KIp6Hook_DONE or error, depending on conditions.
	*/
	{
	TInt reason = KErrNotSupported;
	CheckPacketId(aPacket);	// Do the packet id check

	for (;;)
		{
		if (!iAssociationManager)
			break;              // No CProtocolKey ==> Drop Packet!
		//
		// A reminder: Nothing says that the same SA cannot apply
		// multiple times to a packet (although it would be a weird
		// policy to define). Thus, beware that table may include
		// multiple pointers to the same SA. However, iProtocolKey->ApplyL
		// returns an error, if the SA expires and the packet is dropped

		const TUint id = aInfo.iProtocol;
		const TInt is_udp = (id == KProtocolInetUdp);

		if (aInfo.iIcmp)
			{
			// An ICMP Error report, where the returned packet contains
			// ESP or AH header supposedly generated by this host...
			if (is_udp)
				return KIp6Hook_PASS;
			reason = EIpsec_IcmpError;
			break;          // Not much one can do...
			}

		if (id == KProtocolInetEsp)
			// There will be ESP processing, for temporary security fix,
			// prevent this packet from being used in ICMP Error
			// reports -- msa
			aInfo.iFlags |= KIpNeverIcmpError;
		else if (id == KProtocolInetIpip)
			{
			// Extract Fragment information from the IPv4 IP Header
			TInet6Packet<TInet6HeaderIP4> ip(aPacket, aInfo.iOffset);
			if (ip.iHdr == NULL)
				{
				reason = EIpsec_CorruptPacketIn;
				break;	// Drop! (packet too short)				
				}
			if (ip.iHdr->MF() != 0 || ip.iHdr->FragmentOffset() != 0)
				{
				iIsFragment = 1;
				iId = ip.iHdr->Identification() | (ip.iHdr->Protocol() << 16);
				// What should the scope id be?
				iSrc.SetAddress(ip.iHdr->SrcAddr());
				iDst.SetAddress(ip.iHdr->DstAddr());
				reason = CheckFragmentPolicy();
				if (reason < 0)
					break;
				return KIp6Hook_PASS;
				}
			return KIp6Hook_PASS;
			}
		else if (id == KProtocolInet6Fragment)
			{
			// Extract Fragment information from the IPv6 Fragment Header
			TInet6Packet<TInet6HeaderFragment> fh(aPacket, aInfo.iOffset);
			if (fh.iHdr == NULL)
				{
				reason = EIpsec_CorruptPacketIn;
				break;	// Drop! (packet too short)				
				}
			iIsFragment = 1;
			iId = fh.iHdr->Id();
			iSrc.SetAddress(aInfo.iSrcAddr);
			iDst.SetAddress(aInfo.iDstAddr);
			reason = CheckFragmentPolicy();
			if (reason < 0)
				break;
			return KIp6Hook_PASS;
			}

		if (iCountSA == KIpsecMaxNesting)
			{
			reason = EIpsec_MaxTransforms;
			break;              // Too deep nesting!!!
			}

		iMyself[iCountSA].Set(TInetAddr::Cast(aInfo.iDstAddr));
		CSecurityAssoc *sa = NULL;
		TIpAddress tunnel;
		reason = iAssociationManager->ApplyL(sa, aPacket, aInfo, id, tunnel);
		iTunnel[iCountSA].Set(tunnel, tunnel.iScope);

		if (reason == EIpsec_NotANATTPacket)
			return KIp6Hook_PASS;	// The packet was a normal UDP packet, pass through
		else if (reason < 0)
			{
			//
			// ApplyL wants the packet dropped
			//
			break;
			}
		//
		// If id (= protocol) was UDP, do not increment iCountSA
		// The current packet has been an UDP capsulated ESP packet from
		// which the UDP header is now been removed
		if (!is_udp)
			{
			// If detunneling happened, there is a need to change
			// the interface index based on inner destination! -- msa
			if (!iTunnel[iCountSA]().IsNone())
				{
				UpdateTunnelInterface(aInfo, sa);
				iIsTunnelMode = 1;		// (should actually take this flag from the SA --msa)				
				};
			iSA[iCountSA++].Reset(sa);	// Attach the returned sa to the handle
			}
		//
		// The packet has been changed, report this to the
		// caller.
		aInfo.iProtocol = reason;
		return KIp6Hook_DONE;
		}
	//
	// A Packet is dropped, deliver a copy to potentially interested
	// parties (should have some limiter here?)
	//
	//
	// *NOTE*
	//  The reason for the packet drop is now passed in the Port
	//  field of the info.iSrcAddr. Somewhat kludgy... --msa
	//
	iCountSA = 0;
	LOG(Log::Printf(_L("IPSEC Transform (%d) failed %d"), aInfo.iProtocol, reason));
	aInfo.iSrcAddr.SetPort(reason);
	Deliver(aPacket);
	aPacket.Free();
	return reason;
	}

TInt CProtocolSecpol::ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	/**
	* Peek at the packet before "upper layer".
	*
	* The Secpol installs a hook that request the stack to show each packet
	* to IPsec just before it is given to the final upper layer protocol,
	* such as UDP and TCP, but also any other protocol which is not
	* handled as an extension header. Note that this includes IP-in-IP
	* tunneling protocols.
	*
	* The method compares an incoming packet and accumulated IPsec state
	* against the security policy and decides whether the packet is to be
	* allowed in.
	*
	* @param aPacket The packet in "unpacked" state
	* @param aInfo The packet information block
	* @return As specified for the hook mechanism
	*
	* -	any Leave causes packet to be dropped
	* -	return < 0, the hook took "ownership" of the packet (usually just dropped it explicitly)
	* -	return == KIp6Hook_PASS, pass the packet to the next hook
	*
	* Note, the return "KIp6Hook_DONE" is not used by Secpol here.
	*/
	{
	CSecurityPolicy *const policy = Policy();
	if (policy == NULL)
		return KIp6Hook_PASS;   // No Policy ==> No security Check, pass all
		
	if (iIsTunnelMode)
		{
		// This IPSEC on top of IP-tunnel, and detunneling is being delegated to the
		// IP layer. As it is treated as "transport protocol", IPSEC gets called
		// for a policy check. But, if the IPSEC is in tunnel mode, the policy check
		// needs to be delayed until some inner transport header.. 
		ASSERT(aInfo.iProtocol == KProtocolInetIpip || aInfo.iProtocol == KProtocolInet6Ipip);
		iIsTunnelMode = 0;
		return KIp6Hook_PASS;
		}

	TInt reason = KErrNotSupported;
	const TUint id = aInfo.iProtocol;
	TPolicyFilterInfo filter;

	if (!iAssociationManager)
		User::Leave(reason);     // No CProtocolKey ==> Drop Packet!
	CheckPacketId(aPacket);

	//
	// Now is the time to check the whether the accumulated
	// IPSEC matches the policy...
	//
	//  The topmost header in packet now holds the packet that
	//  should correspond something to which the policy is
	//  applied.
	//
	// For Inbound, Local = Destination address, of the packet
	//              Remote = Source Address of the packet
	filter.iIndex = aInfo.iInterfaceIndex;
	filter.iFlags = KPolicyFilter_INBOUND;
	iPktInfo.FillZ();
	// (The addresses in info are already in IPv6 format)
	iPktInfo.iLocal.Set(TInetAddr::Cast(aInfo.iDstAddr));
	iPktInfo.iRemote.Set(TInetAddr::Cast(aInfo.iSrcAddr));

	// Retrieve all available selector information from the packet
	// (requires some layer breaking snooping...)
	for (const TSnoopHeader *f = snooper; f < snooper_end; ++f)
		{
		if (f->iProtocol == id)
			{
			TInet6Packet<TUpperLayerSnoop> snoop(aPacket, aInfo.iOffset);
			const TUpperLayerSnoop *const hdr = snoop.iHdr;
			if (hdr == NULL)
				{
				reason = EIpsec_CorruptPacketIn;
				goto drop;
				}
			switch (f->iSelector)
				{
				case KLoadPorts:
					iPktInfo.iPortRemote = hdr->Port(f->iO1);	// src port is remote port for incoming
					/* Fall through */
				case KLoadLocal:
					iPktInfo.iPortLocal = hdr->Port(f->iO2);	// dst port is local port for incoming
					iPktInfo.iFlags |= KTransportSelector_PORTS;
					break;
				case KLoadBoth:
					iPktInfo.iPortLocal = hdr->Byte(f->iO2);		// ...load code to lower 8 bits.
					/* Fall through */
				case KLoadType:
					iPktInfo.iPortLocal |= hdr->Byte(f->iO1) << 8;	// ..load type to upper 8 bits.
					iPktInfo.iPortRemote = iPktInfo.iPortLocal;
					break;
				default:
					break;
				}
			break;	
			}
		}
	iPktInfo.iProtocol = (TUint8)id;

	if (aInfo.iIcmp == 0)
		{
		//
		// However, if this call is normal processing call for ICMP, check if the
		// ICMP is actually an error report. In such case trust the main loop
		// to call this hook again with iIcmp != 0 (it's a bit delicate, as
		// the check for ERROR ICMP must *exactly* match the check in the
		// IP layer, or this will pass ICMP packets that are not allowed by
		// the policy!!!
		//
		if (id == KProtocolInet6Icmp)
			{
			if (iPktInfo.iPortLocal < (128<<8))
				return KIp6Hook_PASS;               // Wait for error processing call!
			}
		else if (id == KProtocolInetIcmp)
			{
			switch (iPktInfo.iPortLocal >> 8)
				{
				case KInet4ICMP_Unreachable:        // Destination unreachable
				case KInet4ICMP_SourceQuench:       // Source Quench
				case KInet4ICMP_Redirect:           // Redirect request
				case KInet4ICMP_TimeExceeded:       // Time Exceeded
				case KInet4ICMP_ParameterProblem:   // Parameter Problem
					return KIp6Hook_PASS;           // Wait for error processing call!
				default:
					break;
				}
			}
		// Not an ICMP or normal ICMP, do the policy check with current info
		}

    //Check for the UMA feature support
    CheckFeatureSupportL(NFeature::KFeatureIdFfIpsecUmaSupportEnable);
	for (;;)
		{
		CPolicyAction *items[KIpsecMaxNesting];
		TInt tunnels = 0;

		RPolicySelectorInfo final_info = iPktInfo;
		const TInt count = CollectBundle(filter, final_info, KIpsecMaxNesting, items, NULL, tunnels);
		if (count < 0)
			{
			reason = count;
			break;
			}

		const RIpAddress *dst = &iPktInfo.iLocal;		// initial destination is the inner dst (iLocal of incoming)
		const RIpAddress *src = &iPktInfo.iRemote;		// initial source is the inner src (iRemote of incoming)

		for (TInt j = 0; j < count; j++)
			{
			const CPolicyAction &item = *items[j];

			// Need to save current src/dst in case optional
			// processing is done (only pointers saved).
			const RIpAddress *const dst_opt = dst;
			const RIpAddress *const src_opt = src;


			if (--iCountSA < 0)
				{
				reason = EIpsec_TooFewTransforms;
				goto optional;
				}
			//
			// Verify that the tunnel address (if any) matches with
			// the policy entry.
			if (!item.iIsTunnel)
				{
				if (!iTunnel[iCountSA]().IsNone())
					{
					reason = EIpsec_TunnelMismatch;
					goto optional;
					}
				}
			else
				{
				//
				// If a tunnel is specified, all remaining SA's have the src
				// address of the this tunnel
				//
				if (item.iTunnel().IsNone())
					src = &iTunnel[iCountSA];	// Match tunnel from any source.
				else if (!KOuterIsInner.IsEqual(item.iTunnel()))
					src = &item.iTunnel;		// == Outer src

				// From the result of TAHI Tunnel test, there is some reason that cause the 
				// "src" not eual to iTunnel[iCountSA] but equal to iMyself[iCountSA], that 
				// will leads to "EIpsec_TunnelMismatch" error in original code.
				if (iTunnel[iCountSA]() != (*src)())
					{
					if (iMyself[iCountSA]() != (*src)())
					    {
					    reason = EIpsec_TunnelMismatch;
					    goto optional;
					    }
					src = &iTunnel[iCountSA];   //set "src" to tunnel address
					}
				dst = &iMyself[iCountSA];
				}

			if (item.iSpec)
				{
				// An IPSEC SA is required
				if (iSA[iCountSA].Association() == NULL)
					{
					//
					// SA was present when the AH or ESP was processed, but now appears to have
					// disappeared. Expired, because it was used in two transforms, and latter
					// expired? I don't expect to see this error, but just in case... -- msa
					//
					reason = EIpsec_LostSA;
					goto drop;
					}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT					
				else if ((reason = iAssociationManager->Verify(iSA[iCountSA].Association(), item.iSpec->iSpec,item.iSpec->iPropList, *src, *dst, iPktInfo)) == KErrNone)
#else
				else if ((reason = iAssociationManager->Verify(iSA[iCountSA].Association(), item.iSpec->iSpec, *src, *dst, iPktInfo)) == KErrNone)
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT			
					{
					if (item.iIsTunnel)
						{
						// If it was a tunnel mode, need update the packet specific
						// information for the remaining SA's.
						iPktInfo.iRemote = *src;
						iPktInfo.iLocal = *dst;
						AfterTunnelAction(iPktInfo);
						}
					continue;
					}
				}
			else if (iSA[iCountSA].Association())
				{
				// Note: Above test may not work in general: if the packet contains two
				// IPSEC layers using the same SA, and if the later application expires
				// the SA, the iAssociation member of the first one disappears too!!
				// [in such case, instead of reporting unrequired SA, it would just
				//  accept it--probably not a serious mishap...]
				// However, would not get this far? Expiry would drop the packet at
				// apply phase or the other SA would not verify anyway? -- msa
				reason = EIpsec_UnrequiredSA;
				}
			else
				continue;

optional:	// Only gets here in case there is a mismatch. If
			// the item is optional, then retest the current
			// with the next item.
			if (!item.iOptional)
				goto drop;
			//
			// Restore to same state for the next item
			//
			++iCountSA;
			dst = dst_opt;
			src = src_opt;
			}
#ifdef __WINS__
		if (iCountSA > 0)
#else /*__WINS__*/
		if ((KIPsecTunnelInTunnel == 0) && (iCountSA > 0))
#endif /*__WINS__*/ 
			{
			reason = EIpsec_TooManyTransforms;
			break;
			}
#ifndef __WINS__
		//This is for logging only would be removed during MCL submission
		else if(KIPsecTunnelInTunnel == 1)
			{
			//This is for logging only would be removed during MCL submission
			LOG(Log::Printf(_L("\nKIPsecTunnelInTunnel=1 enabled")));
			}
#endif /* __WINS__*/
		// *NOTE*
		//  There is no need to detach the SA's from the handles. Handle does
		//  not prevent SA from being deleted! Just clearing iCountSA is OK!
		//
		return KIp6Hook_PASS;
		}
drop:
	// A Packet is dropped, deliver a copy to potentially interested
	// parties (should have some limiter here?)
	//
	//
	// *NOTE*
	//  The reason for the packet drop is now passed in the Port
	//  field of the info.iSrcAddr. Somewhat kludgy... --msa
	//
	LOG(LogSelectorInfo(_L("incoming: "), iPktInfo, filter));
	LOG(Log::Printf(_L("\tpolicy check failed %d"), reason));

	iCountSA = 0;
	aInfo.iSrcAddr.SetPort(reason);
	Deliver(aPacket);
	aPacket.Free();
	return reason;
	}

//

CServProviderBase* CProtocolSecpol::NewSAPL(TUint aSockType)
	/**
	* Create a new instance of CProviderSecpol for a socket opened by an application.
	*
	* This socket can be used to load the security policy and to listen
	* what packets are dropped due to current policy.
	*
	* This is called from socket server.
	*
	* @param aSockType Must be KSockRaw
	* @return new CProviderSecpol instance.
	*/
	{
	if (aSockType!=KSockRaw)
		User::Leave(KErrNotSupported);
	CProviderSecpol *pSAP = new (ELeave) CProviderSecpol(*this);
	iSAPlist.AddLast(*pSAP);
	return pSAP;
	}

void CProtocolSecpol::FixupInterfaceIndexes(CSecurityPolicy *aPolicy)
	{
	/**
	* Complete the interfaces with index.
	*
	* If the policy contains interface specific policies, then
	* one must at this point convert the interface names into
	* actual interface indexes.
	*/
	if (aPolicy && aPolicy->iInterfaces && NetworkService())
		{
		// First, find the real interface indexes
		for (CSelectorInterface *si = aPolicy->iInterfaces; si != NULL; si = si->iNext)
			{
			si->iInterfaceIndex = GetInterfaceIndex(*si->iName);
			}
		}
	}

TInt CProtocolSecpol::SetPolicy(const TDesC &aPolicy, TUint &aOffset)
	/**
	* Replace the current security policy database with a new one.
	*
	* Called from CProviderSecpol::Write(). The tasks are
	*
	* @li	Convert the policy definition string into internal format (CSecurityPolicy)
	*		using the parser CSecurityPolicy::SetPolicy().
	* @li	Complete the CPolicyInterface objects with indexes (FixupInterfaceIndexes()).
	* @li	The policy parser creates the algorithm map (CAlgorithmList) as a "side effect".
	*		Pass the new algorithm map to the MAssociationManager::SetAlgorithms().
	* @li	Notify stack that all flows must be re-negotiated (MNetworkService::SetChanged()).
	*
	* @param aPolicy	The new security policy
	* @retval aOffset	Current parsing point (also starting point initially)
	* @return KErrNone, if success and < 0 otherwise
	*
	* In case of error, the aOffset indicates the position in
	* aPolicy at which the parsing terminated.
	*/
	{
	if (iAssociationManager == NULL)
		{
		LOG(Log::Printf(_L("CProtocolSecpol::SetPolicy -- cannot load because PFKEY is not running")));
		return KErrNotReady;
		}

	const TInt res = CSecurityPolicy::SetPolicy(iPolicy, aPolicy, aOffset, iAssociationManager->EndPointCollection());
	if (res != KErrNone)
		return res;

	if (iPolicy == NULL)
		{
		LOG(Log::Printf(_L("New policy is empty")));
		return KErrNone;
		}
	else
		{
		FixupInterfaceIndexes(iPolicy);
#ifdef _LOG
		Log::Printf(_L("New policy:"));
		iAssociationManager->EndPointCollection().LogPrint(_L("ep\t%S = { %S }"));
		TInt i = 0;
		for (CPolicySelector *ps = iPolicy->iSelectors; ps != NULL; ps = ps->iNext)
			{
			const TInt N = ps->iActions.Count();
			TBuf<6> seq;
			seq.AppendFormat(_L("%3d.\t"), ++i);
			LogPolicySelector(seq, *ps);
			if (ps->iFilterData & KPolicyFilter_DROP)
				Log::Printf(_L("\t\t*DROP*"));
			//UMA support REQ417-40027
			if(ps->iFilterData & KPolicyFilter_Exception)
			    Log::Printf(_L("\t\t = { UMAException %d }"),ps->iScope_Exception);
				
			else if (N == 0)
				Log::Printf(_L("\t\t*PASS*"));
			else
				{
				for (TInt i = 0; i < N; ++i)
					LogBundleItem(_L("\t\t"), *ps->iActions[i]);
				}
			}
		Log::Printf(_L("---"));
#endif
		}

	// Currently, the algorithm map results as a side effect
	// from the policy loading operation (it really should be
	// independent configuration file). Pass the map to the
	// PFKEY protocol.
	iAssociationManager->SetAlgorithms(iPolicy->iAlgorithms);

	if (NetworkService())
		{
		// New policy has been installed, request rechecking of
		// all open flows...
		NetworkService()->SetChanged();
		}

	return KErrNone;
	}

void CProtocolSecpol::Deliver(RMBufPacketBase& aPacket)
	/**
	* Deliver a copy of dropped packets to policy sockets.
	*
	* This method sends a copy of the packet to all currently open Policy sockets.
	* This is called from the CProtocolSecpol ApplyL methods for each packet that
	* is dropped. A copy of the packet is made for each open policy socket and
	* delivered by a Deliver method of the SAP (see CProviderSecpol::Deliver).
	*
	* An application that opens a policy socket, but is not reading it, should close
	* the input side (Shutdown), so that the queue of the dropped packets will
	* not waste memory resources in the kernel.
	*
	* @param aPacket The packet to be dropped
	*/
	{
	CProviderSecpol* sap;
	TDblQueIter<CProviderSecpol> iter(iSAPlist);

	while (sap = iter++, sap != NULL)
		if (sap->IsReceiving())
			{
			RMBufRecvPacket copy;
			TRAPD(err, aPacket.CopyL(copy); aPacket.CopyInfoL(copy));
			if (err == KErrNone)
				{
				copy.Pack();
				sap->Deliver(copy);
				}
			// Can allways call free (if Deliver() took the packet, this is NOP)
			copy.Free();
			}
	}

void CProtocolSecpol::CheckFeatureSupportL(TUid aFeature)
    {
    // Check Gan support from feature manager
    iIPSecGANSupported = CFeatureDiscovery::IsFeatureSupportedL(aFeature);
    if(iIPSecGANSupported != (TInt)ETrue)
		{
		LOG(Log::Printf(_L("CProtocolSecpol::CheckFeatureSupport Error Checking Feature Support ")));
		}
		else
		{
		LOG(Log::Printf(_L("CProtocolSecpol::CheckFeatureSupport %d Feature Supported %d"),aFeature,iIPSecGANSupported));
		}
    }

//UMA support
//Check for exception selector being loaded or not. This is required for Drop mode policy
 //If policy is set as 
 //                  inbound = drop
 //                  outbound = drop
 // then instead returning Excetion selectors should be checked.
void CProtocolSecpol::CheckExceptionSelector(TBool &aException_flag) const
    {
    aException_flag = EFalse;
    if(iIPSecGANSupported)
        {
         for(CPolicySelector *policy_Exception = iPolicy->iSelectors; policy_Exception != NULL ; policy_Exception = policy_Exception->iNext)
           {
           if(((policy_Exception->iFilterData & KPolicyFilter_Exception) ==  KPolicyFilter_Exception))
               {
               aException_flag = ETrue;
               break;
               }//if
           }//for
         }//if
    else
        {
         LOG(Log::Printf(_L("CProtocolSecpol::CheckExceptionSelector  Feature is not Supported")));
        }
    }

