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
// sadb.cpp - IPv6/IPv4 IPSEC security association database
//

#include <networking/pfkeyv2.h>
#include "sadb.h"
#include "sa_crypt.h"
#include "pfkeymsg.h"
#include <networking/ipsecerr.h>
#include "ipseclog.h"
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#include "spdb.h"
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KSecurityAssocTimeoutOffset 256
__ASSERT_COMPILE(KSecurityAssocTimeoutOffset == _FOFF_DYNAMIC(CSecurityAssoc, iTimeout));
#else
#define KSecurityAssocTimeoutOffset _FOFF(CSecurityAssoc, iTimeout)
#endif

class CSecurityAssocTimeoutLinkage : public TimeoutLinkage<CSecurityAssoc, KSecurityAssocTimeoutOffset>
	/**
	* Relay Timeout callback.
	*
	* This "linkage" receives the timeout callback from RTimeout::iTimeout
	* and relays the event to CSecurityAssoc::TimerExprired() function. 
	*/
	{
public:
	static void Timeout(RTimeout &aLink, const TTime &aNow, TAny *aPtr)
		{
		Object(aLink)->TimerExpired(*(MAssociationManager*)aPtr, aNow);
		}
	};


TLifetime::TLifetime(const struct sadb_lifetime &aLifetime)
	/**
	* Construct a lifetime object from the PF_KEY sadb_lifetime value.
	*
	* @param aLifetime The PFKEY lifetime value.
	*/
	{
	iAllocations = aLifetime.sadb_lifetime_allocations;
	iBytes = aLifetime.sadb_lifetime_bytes;

	// Load the seconds values as is into TTime to be used
	// later in Freeze to convert into a real time stamp.
	iAddtime = aLifetime.sadb_lifetime_addtime;
	iUsetime = aLifetime.sadb_lifetime_usetime;
	}

TLifetime::TLifetime() : iAllocations(0), iBytes(0), iAddtime(0), iUsetime(0)
	/**
	* Default Lifetime initializer.
	*/
	{
	}


void TLifetime::Freeze(TTime &aTime, const TTime &aNow)
	/**
	* Freeze the lifetime.
	*
	* Convert the relative time (aTime) stored in a TTime as seconds
	* into absolute time counting from the given time stamp (aNow).
	*
	* @param aTime	The time to freeze
	* @param aNow	The current time.
	*/
	{
	TInt64 x = aTime.Int64();

	aTime = Time::MaxTTime();
	if (x <= 0)
		// Zero is used to indicate no limit. PFKEY has 64 bit
		// unsigned int values, but EPOC32 64 bit value is signed.
		// if the requested value is large enough to set the sign,
		// treat it the same as 0, use MaxTTime.
		return;
	if (x > aTime.Int64() / 1000000)
		// Conversion to Microseconds would cause overflow, use MaxTTime
		return;
	x *= 1000000;
	if (aTime.Int64() - x < aNow.Int64())
		// Adding x to now would overflow, use MaxTTime
		return;
	// It is safe to add interval to now
	aTime = aNow;
	aTime += TTimeIntervalMicroSeconds(x);
	}

CSecurityAssoc::CSecurityAssoc(const TPfkeyMessage &aMsg) : iTimeout(CSecurityAssocTimeoutLinkage::Timeout)
	/**
	* Construct SA
	*
	* This constructor is only used through the GETSPI and ADD.
	* It creates a larval SA
	*
	*		<base, address, SPI range>
	*		<base, SA, (lifetimes(HS),) address(SD), (address(P)), ...>
	*
	* Only base, address and SPI from SA are handled by the constructor.
	* ADD must call UpdateL() to get the remainging extensions processed!
	*
	* @li	Lifetimes are initialized, but the TIMER is not activated
	*		here, because timer activation may cause an immediate
	*		expiration and deletion of CSecurityAssoc.
	*
	* @param aMsg	The PFKEY message
	*/
	{
	// iBase must always be present!
	iType = aMsg.iBase.iMsg->sadb_msg_satype;
	iState = SADB_SASTATE_LARVAL;
	if (aMsg.iSa.iExt)
		iSPI = aMsg.iSa.iExt->sadb_sa_spi;
	
	// When src_specific is off from the SA spsecification, then
	// the incoming SA has the dst unspecified, and outgoing SA
	// has the src unspecified. Unspecified can be expressed either
	// by explicitly setting the address as unspecified or just
	// leaving the extension out from the PFKEY message. Need to
	// test the presense of extension in both src and dst.
	// (By default everything is unspecified).
	if (aMsg.iDstAddr.iExt)
		{
		iDst = aMsg.iDstAddr.iAddr;
		iInfo.iPortDst = aMsg.iDstAddr.iPort;
		}
	if (aMsg.iSrcAddr.iExt)
		{
		iSrc = aMsg.iSrcAddr.iAddr;
		iInfo.iPortSrc = aMsg.iSrcAddr.iPort;
		}
	if (aMsg.iProxyAddr.iExt)
		{
		iInfo.iSrc = aMsg.iProxyAddr.iAddr;
		}

	// Set some current times
	TTime time_now;
	time_now.UniversalTime();
	iCurrent.iAddtime = time_now;
	iCurrent.iUsetime = Time::NullTTime();	// as iUsed==0
	TLifetime::Freeze(iSoft.iAddtime, time_now);

	// Set the hard lifetime to the default Larval timeout
	iHard.iAddtime = KLifetime_LARVAL_DEFAULT;
	TLifetime::Freeze(iHard.iAddtime, time_now);

	// All Other fields are assumed to contain initial
	// default values (NULL, 0)
	}

CSecurityAssoc::CSecurityAssoc
	(
	const TSecurityAssocSpec &aSpec,
	const RIpAddress &aSrc,
	const RIpAddress &aDst,
	const RAssociationInfo &aInfo
	) : iTimeout(CSecurityAssocTimeoutLinkage::Timeout)
	/**
	* Construct "larval" SA.
	*
	* This constructor is used for the outgoing packet and it creates
	* a special LARVAL EGG SA (SPI == 0). Called only for ACQUIRE
	* processing.
	*
	* @param aSpec The SA template
	* @param aSrc The SA src address
	* @param aDst The SA dst address
	* @param aInfo Additional information
	*/
	{
	//
	// Contrary to PFKEY text, this implementation contains two kinds of
	// "larval" SA's. This constructor creates an "pre-larval" SA, an "egg"
	// which is recognized from the fact that SPI == 0 (this implementation
	// reserves 0 for internal use and never gives it out as an SPI value)
	iType = aSpec.iType;
	iSPI = 0;
	iDst = aDst;
	iSrc = aSrc;
	iInfo = aInfo;

	// Reference objects, if present need to be refcounted on copy
	if (iInfo.iSrcIdentity)
		iInfo.iSrcIdentity->Open();
	if (iInfo.iDstIdentity)
		iInfo.iDstIdentity->Open();

	iState = SADB_SASTATE_LARVAL;

	iFlags = aSpec.iPfs ? SADB_SAFLAGS_PFS : 0;
	iReplayCheck = (aSpec.iReplayWindowLength != 0);
	//
	// Larval egg does not activate algorithm engines
	// (they need to be created by update!)
	iAalg = aSpec.iAalg;
	iEalg = aSpec.iEalg;

	//
	// Set some base times
	//
	TTime time_now;
	time_now.UniversalTime();
	iCurrent.iAddtime = time_now;
	iCurrent.iUsetime = Time::NullTTime();	// as iUsed==0
	//
	// Set the hard lifetime from the specification, soft
	// lifetime is left as initial infinite. Unspecified
	// iLarvalLifetime is a request to use the default
	// timeout (one cannot request 'infinite' for this)
	iHard.iAddtime = aSpec.iLarvalLifetime ?
		aSpec.iLarvalLifetime : KLifetime_LARVAL_DEFAULT;
	TLifetime::Freeze(iSoft.iAddtime, time_now);
	TLifetime::Freeze(iHard.iAddtime, time_now);
	}

TInt CSecurityAssoc::UpdateL(MAssociationManager &aManager, const TPfkeyMessage &aMsg, CIpsecCryptoManager *aLib)
	/**
	* Update SA from PFKEY message.
	*
	* Initialize CSecurityAssoc from TPfkeyMessage. This is a "blind"
	* operation, most fields are just updated unconditionally, it is
	* assumed that the calling function has verified the legality of
	* this operation. (Some basic checks are done)
	*
	* @param aManager The association manager
	* @param aMsg The PFKEY message
	* @param aLib The crypto manager
	*/
	{
	TInt set_hard = 0;

	// Loading Security Association
	if (aMsg.iSa.iExt)
		{
		//
		// If state is LARVAL, need to "defuse" the internal
		// timeout from the HARD lifetime field, if set.
		// (do this only if aMsg doesn't already rewrite the time)
		//
		set_hard = (iState == SADB_SASTATE_LARVAL);
		if (aMsg.iSa.iExt->sadb_sa_state != SADB_SASTATE_MATURE)
			return KErrGeneral;	 // Only MATURE state can be set.
		if (iSPI && iSPI != aMsg.iSa.iExt->sadb_sa_spi)
			return KErrGeneral;	 // Once SPI is set, it cannot be changed!
		iSPI = aMsg.iSa.iExt->sadb_sa_spi;
		iFlags = aMsg.iSa.iExt->sadb_sa_flags;
		iReplayCheck = (aMsg.iSa.iExt->sadb_sa_replay != 0);

		// This information can only be set for new SA, it cannot
		// be changed or updated (applies to all 'selector' info,
		// including Identity.
		if (aMsg.iSrcAddr.iExt)
			{
			iSrc = aMsg.iSrcAddr.iAddr;
			iInfo.iPortSrc = aMsg.iSrcAddr.iPort;
			}
		if (aMsg.iDstAddr.iExt)
			{
			iDst = aMsg.iDstAddr.iAddr;
			iInfo.iPortDst = aMsg.iDstAddr.iPort;
			iInfo.iProtocol = aMsg.iDstAddr.iExt->sadb_address_proto;
			}
		if (aMsg.iProxyAddr.iExt)
			{
			iInfo.iSrc = aMsg.iProxyAddr.iAddr;
			//
			// Do a special hack: if the proxy address portion is unspecified
			// address, but scope id non-zero, then assume the scope id is
			// actually the interface index of the VPN tunnel interface.
			// (NOTE: this is only useful for inbound SA's?)
			if (iInfo.iSrc().IsUnspecified() && iInfo.iSrc().iScope != 0)
				{
				iTunnelIndex = iInfo.iSrc().iScope;
				iInfo.iSrc.Close();
				}
			}
		}

	// Updating identities should probably only be allowed
	// for larval SA's
	if (aMsg.iDstIdent.iExt && iInfo.iDstIdentity == NULL)
		iInfo.iDstIdentity = CIdentity::NewL(aMsg.iDstIdent.iData, aMsg.iDstIdent.iExt->sadb_ident_type);
	if (aMsg.iSrcIdent.iExt && iInfo.iSrcIdentity == NULL)
		iInfo.iSrcIdentity = CIdentity::NewL(aMsg.iSrcIdent.iData, aMsg.iSrcIdent.iExt->sadb_ident_type);

	//	time_now is used in initialize of the IV and
	//	for lifetimes, so declare and initialize it here
	TTime time_now;
	time_now.UniversalTime();

	// Activating crypto engines
	//		but only if neither of them have been activated before.
	//		If already active,just silently ignore the parameters
	//		in the Message.
	//		(the test below leaves NULL/NULL auth/encrypt situation
	//		open to later update!)
	if (aMsg.iSa.iExt && !iAeng && !iEeng)
		{
		iAalg = aMsg.iSa.iExt->sadb_sa_auth;

		if (iAalg)
			{
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT	
			if (iAalg == SADB_AALG_AES_XCBC_MAC)
				{
				iAeng = aLib->NewMacL(iAalg, aMsg.iAuthKey.iData);
				}
			else
				{
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT				
				iAeng = aLib->NewAuthL(iAalg, aMsg.iAuthKey.iData);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT				
				}
#endif 				
			if (!iAeng)
				User::Leave(KErrNotFound);	// Conf. Error: algorithm not installed!
			}	
		iEalg = aMsg.iSa.iExt->sadb_sa_encrypt;
		if (iEalg)
			{
			iEeng = aLib->NewEncryptL(iEalg, aMsg.iEncryptKey.iData);
			if (!iEeng)
				User::Leave(KErrNotFound);	// Conf. Error: algorithm not installed!
			}
		if (iEeng)
			{
			// NOTE: this must be allocated even if 0 length!
			delete iIV;		// ..just in case...
			iIV = NULL;		// ..just in case
			iIV = HBufC8::NewL(iEeng->IVSize());
			// The initial content of the IV should be some random
			// bit pattern (important thing is to have something that
			// changes, and is not always Zero, thus the time_now is
			// used). [Using uninitialized memory would be a security
			// risk -- it might even contain the key information!!!]
			// (Use of time is a dubious thing, as it might provide
			// information to be used in some other attacks, as it
			// reveals the exact CPU clock time; but it will do for now!)
			TPtr8 iv(iIV->Des());
			TPtr8 fill = TPtr8((TUint8 *)&time_now, sizeof(time_now), sizeof(time_now));
			// Do not use iv.MaxLength(), it may be larger IV size!
			TInt n = iEeng->IVSize();
			while (n > 0)
				{
				if (fill.Length() > n)
					fill.SetLength(n);
				iv.Append(fill);
				n -= fill.Length(); // This loop will hang, if fill.Length() == 0!!
				}
			}
		}

	//	Loading lifetimes
	//		(Done after all "leaving" and error returns, so that initial life
	//		timers won't get overridden by Update, if it fails
	if (aMsg.iHard.iExt)
		{
		iHard = TLifetime(*aMsg.iHard.iExt);
		if (iUsed)
			TLifetime::Freeze(iHard.iUsetime, iCurrent.iUsetime);
		TLifetime::Freeze(iHard.iAddtime, time_now);
		}
	else if (set_hard)
		{
		iHard = TLifetime();
		TLifetime::Freeze(iHard.iAddtime, time_now);
		}
	if (aMsg.iSoft.iExt)
		{
		iSoft = TLifetime(*aMsg.iSoft.iExt);
		if (iUsed)
			TLifetime::Freeze(iSoft.iUsetime, iCurrent.iUsetime);
		TLifetime::Freeze(iSoft.iAddtime, time_now);
		}

	// Load Traffic Selector
	if (aMsg.iTs.iExt)
		{
		iTS.Reset();
		RTrafficSelector dummy;
		for (TInt i = 0; i < aMsg.iTs.iExt->sadb_x_ts_numsel; ++i)
			{
			User::LeaveIfError(iTS.Append(dummy));
			RTrafficSelector &ts = iTS[i];
			const T_sadb_selector &in_ts = aMsg.iTs.Selector(i);
			ts.iProtocol = in_ts.sadb_x_selector_proto;
			ts.iPortSrc = in_ts.iSrc.Port();
			ts.iPortDst = in_ts.iDst.Port();
			User::LeaveIfError(ts.iSrc.Open(aManager.EndPointCollection(), in_ts.iSrc));
			User::LeaveIfError(ts.iDst.Open(aManager.EndPointCollection(), in_ts.iDst));
			}
		}

	// Replace CNatTraversal object to handle ESP UDP encapsulation (created if required)
	delete iNatTraversal;
	iNatTraversal = CNatTraversal::New(iFlags, aMsg.iPrivateExtension);

	// 'set_hard' is only defined when Update wants to change the SA
	// state into MATURE. The actual state change is delayed until
	// here, in case update fails before this. The SA will most likely
	// be unusable, but at least it won't be MATURE either...
	if (set_hard)
		iState = SADB_SASTATE_MATURE;
	return KErrNone;
	}

void CSecurityAssoc::Cleanup()
	/**
	* Release all memory resources attached to a SA.
	*/
	{
	iTimeout.Cancel();
	if (iInfo.iSrcIdentity)
		{
		iInfo.iSrcIdentity->Close();
		iInfo.iSrcIdentity = NULL;
		}
	if (iInfo.iDstIdentity)
		{
		iInfo.iDstIdentity->Close();
		iInfo.iDstIdentity = NULL;
		}

	iTS.Reset();

	delete iIV; iIV = NULL;

	// Detach all handles
	while (!iHandles.IsDetached())
		{
		RSecurityAssociation *handle = (RSecurityAssociation *)iHandles.iNext;
		ASSERT(handle->iAssociation == this);

		 /* Since the iAssociation was never set, so the iErrValue is set over here
	       * which will be used by sc_prt6.cpp to pass the error value
           * from the kmd to the tcp/ip stack
           */
		handle->SetError(iErrValue);
        		
		handle->None();
		(handle->iCallback)(*handle);
		}

	delete iEeng; iEeng = NULL;
	delete iAeng; iAeng = NULL;

	delete iNatTraversal; iNatTraversal = NULL;
	}

CSecurityAssoc::~CSecurityAssoc()
	/**
	* Destructor.
	*/
	{
	Cleanup();
	}

TInt CSecurityAssoc::Close()
	/**
	* Decrement the reference count.
	*
	* @return
	*	@li KErrNone, if association still exists
	*	@li KErrDied, if association has been deleted.
	*/
	{
	if (--iRefs < 0)
		{
		delete this;
		return KErrDied;
		}
	return KErrNone;
	}


TBool RTrafficSelector::operator<=(const RTrafficSelector &aSel) const
	/**
	* The <= comparison for selector values.
	*
	* Every field must fullfull the "<="-condition individually for
	* this to return true. This operator should only be used to
	* compare individual packet values against a min and max
	* range:
	* @code min <= packetdata <= max
	* @endcode
	*
	* @param aSel The selector (packet or max). With 'packet', this = min, and
	* with 'max', this is 'packet'.
	*
	* @see CSecurityAssoc::Match
	*/
	{
	return
		iProtocol <= aSel.iProtocol &&
		iPortSrc <= aSel.iPortSrc &&
		iPortDst <= aSel.iPortDst &&
		iSrc() <= aSel.iSrc() &&
		iDst() <= aSel.iDst();
	}

// This is defined based on the assumption that RArray<>::Reset()/Close()
// does not run destructor on the array elements. Because RTrafficSelector
// holds handles for addresses, they must be closed before the actual
// RArray methods are called.
void RTrafficSelectorSet::Reset()
	{
	/**
	* Release Traffic Selector space
	*/
	TInt i = Count();
	while (--i >= 0)
		{
		(*this)[i].iSrc.Close();
		(*this)[i].iDst.Close();
		}
	RArray<RTrafficSelector>::Reset();
	}


TInt CSecurityAssoc::MatchSpec
	(
	const TSecurityAssocSpec &aSpec,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	const CPropList *aPropList,
#endif	//SYMBIAN_IPSEC_VOIP_SUPPORT	
	const RIpAddress &aSrc,
	const RIpAddress &aDst,
	const RAssociationInfo &aInfo,
	const RTrafficSelector &aPkt
	) const
	/**
	* Compare SA to SA template and per packet information.
	*
	* Test if the SA matches the requirements.
	*
	* @param aSpec	The SA template
	* @param aSrc	The expected SA source address
	* @param aDst	The expected SA destination address
	* @param aInfo	The additional information (PFP information, etc)
	* @param aPkt	The packet information (to check agains TS)
	*
	* @return KErrNone, when a match occurs, or negative error,
	* when some mismatch is detected
	*/
	{
	if (iDst() != aDst())
		return EIpsec_MismatchedDestination;
	if (iSrc() != aSrc())
		return EIpsec_MismatchSource;
	
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT	
	if (((iFlags & SADB_SAFLAGS_PFS) == 0) != (aSpec.iPfs == 0))
		return EIpsec_MismatchedPFS;
	for (TInt i=0;i<aPropList->Count();i++)
		{
		CSecurityProposalSpec *proposal = aPropList->At(i);
		TInt ret=KErrNone;
	
		if (iType != proposal->iType) ret= EIpsec_MismatchedType;
		if (iAalg != proposal->iAalg) ret = EIpsec_MismatchedAuthAlg;
		if (iEalg != proposal->iEalg) ret = EIpsec_MismatchedEncryptAlg;
		if (ret == KErrNone) break;
		if ( (i == aPropList->Count()-1)&&(ret != KErrNone) ) return ret;
		}
#else
	if (iType != aSpec.iType)
		return EIpsec_MismatchedType;
	if (((iFlags & SADB_SAFLAGS_PFS) == 0) != (aSpec.iPfs == 0))
		return EIpsec_MismatchedPFS;
	if (iAalg != aSpec.iAalg)
		return EIpsec_MismatchedAuthAlg;
	if (iEalg != aSpec.iEalg)
		return EIpsec_MismatchedEncryptAlg;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT		

	
	// Fail ReplayCheck test only if the SA specification
	// requires replay check, but SA doesn't enable it.
	// But, allow SA to require replay check, even if spec doesn't.
	if (aSpec.iReplayWindowLength && !iReplayCheck)
		return EIpsec_MismatchReplayWindow;

	if (iInfo.iProtocol != aInfo.iProtocol)
		return EIpsec_MismatchProtocol;
	if (iInfo.iPortSrc != aInfo.iPortSrc)
		return EIpsec_MismatchSourcePort;
	if (iInfo.iPortDst != aInfo.iPortDst)
		return EIpsec_MismatchDestinationPort;
	
	// The "proxy" of PKFEY is stored in iInfo.iSrc of SA
	if (iInfo.iSrc() != aInfo.iSrc())
		return EIpsec_MismatchProxy;
	if (iInfo.iDst() != aInfo.iDst())
		return EIpsec_MismatchProxy;
	
	// *NOTE*
	//		Identities need to be in aInfo, because TSecurityAssocSpec can
	//		be shared between inbound and outboud, and identities are expressed
	//		as remote and local. The caller knowing the context must copy the
	//		pointers into RAssociationInfo properly!
	//
	//		As identity blocks are shared, do raw pointer compare first, it should
	//		optimize at least the processing of outbound SA's (as for those, the
	//		pointers are just references to the identities in TSecurityAssocSpec)!
	//
	if (iInfo.iDstIdentity != aInfo.iDstIdentity &&
		(iInfo.iDstIdentity == NULL ||		// No match if, either is NULL
		 aInfo.iDstIdentity == NULL ||		//
		 !iInfo.iDstIdentity->Match(*aInfo.iDstIdentity)))
		 return EIpsec_MismatchDestinationIdentity;
	if (iInfo.iSrcIdentity != aInfo.iSrcIdentity &&
		(iInfo.iSrcIdentity == NULL ||		// No match if, either is NULL
		 aInfo.iSrcIdentity == NULL ||		//
		 !iInfo.iSrcIdentity->Match(*aInfo.iSrcIdentity)))
		 return EIpsec_MismatchSourceIdentity;
		 
	// Finally, verify the Traffic Selector Match

	TInt i = iTS.Count();
	if (i == 0)
		return KErrNone;	// SA does not have traffic selectors (implicit match all)

	while (i > 1)
		{
		const RTrafficSelector &max = iTS[--i];
		const RTrafficSelector &min = iTS[--i];
		// Is packet info within range?
		if (min <= aPkt && aPkt <= max)
			return KErrNone;
		}

	return EIpsec_MismatchProxy;	// [ Need a new error code for TS mismatch! -- msa]
	}



int CSecurityAssoc::ReplayCheck(TUint32 aSeq)
	/**
	* Check a sequence number against a replay window.
	*
	* Perform the sequence number tracking and replay window
	* checking. After checking the iTestSeq will contain the
	* correct sequence number, and if ESN is used, the high
	* order part is assigned according to the ESN rules.
	*
	* @param aSeq The low order 32 bits of the sequence number
	*
	* @return
	*	- 0, if too old or duplicate,
	*	- 1, if not duplicate or after window
	*/
	{
	iTestInWindow = 0;
	iTestSeq = aSeq;
	iTestSeq.SetHigh(iRecvSeq.High());
	if (iFlags & SABD_SAFLAGS_ESN)
		{
		// Extended Sequence Number enabled. The authentication code
		// requires the correct high order bits, and this code must
		// be executed whether replay check is enabled or not
		//
		// *NOTE* The following is based modulo 2**32 (unsigned) arithmetic!
		//
		const TUint32 lower = iRecvSeq - KMaxReplayWindowLength + 1;
		if (lower <= -KMaxReplayWindowLength)
			{
			if (aSeq >= lower)
				{
				// After window begin
				if (aSeq > iRecvSeq)
					return 1;		// After window end, Check integrity only
				// Inside window, check replay bitmap
				}
			else
				{
				// Before window begin, wrapped around
				iTestSeq.SetHigh(iTestSeq.High() + 1);
				return 1;			// After window, check integrity only
				}
			}
		else
			{
			if (aSeq >= lower)
				{
				iTestSeq.SetHigh(iTestSeq.High() - 1);
				// Inside window, check replay bitmap
				}
			else
				{
				if (aSeq > iRecvSeq)
					{
					return 1;	// Check integrity only
					}
				// Inside window, check replay bitmap
				}
			}
		if (!iReplayCheck)
			return 1;	// ESN without replay check!
		}
	else if (!iReplayCheck)
		{
		// No replay check, Normal Sequence numbers
		return 1;
		}
	else
		{
		// Replay Check, Normal Sequence Numbers
		
		//LOG(Log::Printf(_L("SA[%u] seq=%u:%u"), this, iTestSeq.High(), (TInt)iTestSeq));
		if (aSeq == 0) 
			return 0;			// Zero is illegal sequence number
		if (aSeq > iRecvSeq)	// New larger sequence number
			return 1;
		const TUint32 diff = iRecvSeq - aSeq;
		if (diff >= KMaxReplayWindowLength)
			return 0;			// Too old or wrapped
		}
	iTestInWindow = 1;
	const TInt i = BitmapWord(aSeq);
	const TInt b = BitmapBit(aSeq);
//	LOG(Log::Printf(_L("SA[%u] RC seq=%u:%u iBitmap[%d] bit %d = %d"),
//		this, iTestSeq.High(), (TInt)iTestSeq, i, b, (iBitmap[i] & (1L << b)) != 0));
	return (iBitmap[i] & (1L << b)) == 0;
	}


void CSecurityAssoc::ReplayUpdate(TUint32 aSeq)
	/**
	* Commit/accept the sequence number.
	*
	* This assumes that the ReplayCheck with the same sequence
	* number was called before this. This function can only be
	* called, if the packet authenticates correctly (and thus
	* the sequence number is valid too).
	*
	* @param aSeq	The low order 32 bits of the sequence number.
	*/
	{
	ASSERT(aSeq == iTestSeq);

//	LOG(Log::Printf(_L("SA[%u] RU T=%u:%u"), this, iRecvSeq.High(), (TInt)iRecvSeq));
	if (!iReplayCheck)
		{
		iRecvSeq = iTestSeq;
		return;
		}

	if (!iTestInWindow)
		{
		// Need to adjust window, aSeq is the new leading window edge.
		const TUint32 diff = aSeq - iRecvSeq;
		if (diff < KMaxReplayWindowLength)
			{
			// need to clear bits iRecvSeq -> aSeq
			// brute force loop...
			while (aSeq != ++iRecvSeq)
				{
				const TInt ix = BitmapWord(iRecvSeq);
				const TInt ib = BitmapBit(iRecvSeq);
				iBitmap[ix] &= ~(1L << ib);
				}
			}
		else
			{
			iBitmap[0] = 0;
			iBitmap[1] = 0;
			iBitmap[2] = 0;
			iBitmap[3] = 0;
			iRecvSeq = iTestSeq;
			}
		}
	// *note*
	//	If window is not adjusted, the earlier ReplayCheck already
	//	guarantees that the sequence number is within the window.
	const TInt i = BitmapWord(aSeq);
	const TInt b = BitmapBit(aSeq);
	iBitmap[i] |= (1L << b);
//	LOG(Log::Printf(_L("SA[%u] RU seq=%u:%u iBitmap[%d] bit %d = %d"),
//		this, iTestSeq.High(), (TInt)iTestSeq, i, b, (iBitmap[i] & (1L << b)) != 0));
	}


TInt CSecurityAssoc::TimerInit(MAssociationManager &aManager)
	/**
	* Initialize timer for the SA, if lifetimes require it.
	*/
	{
	TTime now;
	now.UniversalTime();
	return TimerExpired(aManager, now);
	}

TInt CSecurityAssoc::MarkUsed(MAssociationManager &aManager)
	/**
	* SA has been used.
	*
	* MarkUsed is called when the SA has been used for something:
	* Check the count based lifetimes and record the "first use" time.
	*/
	{
	TInt result = CountExpired(aManager);
	if (result == KErrNone && !iUsed)
		{
		//
		// First Use, "freeze" the times
		//
		iUsed = 1;
		iCurrent.iUsetime.UniversalTime();
		TLifetime::Freeze(iHard.iUsetime, iCurrent.iUsetime);
		TLifetime::Freeze(iSoft.iUsetime, iCurrent.iUsetime);
		// Activate Timers (if required)
		result = TimerExpired(aManager, iCurrent.iUsetime);
		}
	return result;
	}


TInt CSecurityAssoc::TimerExpired(MAssociationManager &aManager, const TTime &aNow)
	/**
	* Examine the lifetime expiration.
	*
	* TimerExpired is called when the life status of this SA
	* should be re-examined relative to the current time (aNow).
	*
	* @li *NOTE*
	*		This method just does not only expire. It also
	*		re-activates the timer, if SA is not expired and
	*		has lifetimes based on time. Thus, this is used
	*		internally also to activate the timers for the
	*		first time.
	* @param aManager	Association Manager
	* @param aNow		The current time.
	*
	* @return
	*		KErrNone, if SA did not expire
	*		KErrDied, if SA expired
	*/
	{
	const TTime *time;
	TTimeIntervalSeconds delta;
	// 'inifinite' is the return value from SecondsFrom, and is set non-zero
	// when the time interval in seconds does not fit into 32 bit integer.
	//
	// If the expiration is so far in the future that 32bit integer
	// cannot hold the number of seconds, don't set the timer, just
	// assume no expiration! (I hope nobody sends Psion on a probe
	// to Alpha Centauri with this code, while trying to specify other
	// than infinite lifetime -- msa)
	TInt infinite;


	// Choose the nearest of limits for study. If SA is never used
	// yet, always checks the addtime only.
	time = (!iUsed || iHard.iAddtime < iHard.iUsetime) ?
		&iHard.iAddtime : &iHard.iUsetime;

	if (*time <= aNow)
		{
		// Hard lifetime expired
		iState = SADB_SASTATE_DEAD;
		aManager.Expired(*this, SADB_EXT_LIFETIME_HARD, iHard);
		aManager.Delete(this);
		return KErrDied;
		}
	infinite = time->SecondsFrom(aNow, delta);
	//
	// If SOFT expire has already occurred (DYING), then ignore
	// soft expire times (only generate SOFT Expire message once!)
	//
	if (iState != SADB_SASTATE_DYING)
		{
		// Choose the nearest of limits for study
		time = (!iUsed || iSoft.iAddtime < iSoft.iUsetime) ?
			&iSoft.iAddtime : &iSoft.iUsetime;
		if (*time <= aNow)	// Soft lifetime expired?
			{
			iState = SADB_SASTATE_DYING;
			aManager.Expired(*this, SADB_EXT_LIFETIME_SOFT, iSoft);
			}
		else
			{
			TTimeIntervalSeconds soft;
			if (time->SecondsFrom(aNow, soft) == 0 && (infinite || soft < delta))
				{
				// Non-infinite soft time specified, and is less than
				// than the hard time, use this as a base for the timer
				// setup.
				infinite = 0;	// Not infinite, a value exists.
				delta = soft;
				};
			}
		//
		// DoCallbacks is needed when DYING is set in above,
		// but is placed here, because PFKEY Update/Add message eventually
		// ends up here (via TimerInit) when it changes the state of
		// the SA, and callbacks are needed also then.. -- msa
		if (DoCallbacks() == KErrDied)
			return KErrDied;	// Hups, lost the SA!
		}
	//
	// SA is not dead, reactivate timer, if needed
	//
	if (!infinite)
		aManager.TimerOn(*this, delta.Int() > 0 ? delta.Int() : 1);
	return KErrNone;
	}

TInt CSecurityAssoc::CountExpired(MAssociationManager &aManager)
	/**
	* Examine the count based lifetime.
	*
	* CountExpired is called by when the life status of this SA
	* in respect of bytes or allocations counts should
	* be re-examined.
	*
	* @param aManager The association manager
	* @return
	*	KErrNone, if SA did not expire
	*	KErrDied, if SA expired (not available any more)
	*/
	{
	if ((iHard.iBytes != 0 && iHard.iBytes < iCurrent.iBytes) ||
		(iHard.iAllocations && iHard.iAllocations < iCurrent.iAllocations))
		{
		// Hard lifetime expired
		iState = SADB_SASTATE_DEAD;
		aManager.Expired(*this, SADB_EXT_LIFETIME_HARD, iHard);
		aManager.Delete(this);	// <-- *this* is DELETED!!!
		return KErrDied;
		}
	//
	// If SOFT expire has already occurred (DYING), then ignore
	// soft expire times (only generate SOFT Expire message once!)
	//
	if (iState != SADB_SASTATE_DYING)
		{
		if ((iSoft.iBytes != 0 && iSoft.iBytes < iCurrent.iBytes) ||
			(iSoft.iAllocations && iSoft.iAllocations < iCurrent.iAllocations))
			{
			iState = SADB_SASTATE_DYING;
			aManager.Expired(*this, SADB_EXT_LIFETIME_SOFT, iSoft);
			}
		}
	return KErrNone;
	}


TInt CSecurityAssoc::DoCallbacks()
	/**
	* Call the callbacks registered for the SA.
	*
	* This is called for every state change of SA (except for DEAD,
	* which is handled in the Delete() above)
	*
	* @return Either
	*	KErrNone, if SA instance still exists
	*	KErrDied, if SA instance was destroyed and none other!
	*/
	{
	//
	// *NOTE*
	//		As the callback may delete the handle (and may even
	//		cause deletion of handles before and after, things
	//		get somewhat difficult...
	//
	RCircularList list(iHandles);
	Open();	 // Can't have the SA being pulled from under us!
	while ((list.iNext) != &list)
		{
		RSecurityAssociation *r = (RSecurityAssociation *)list.iNext;
		r->iCallback(*r);
		if (list.iNext == r)
			{
			//
			// The handle was not deleted, move it back to the SA
			//
			r->Detach();
			iHandles.Attach(*r);
			}
		}
	return Close(); // Now, let it go, if someone deleted it!
	}
