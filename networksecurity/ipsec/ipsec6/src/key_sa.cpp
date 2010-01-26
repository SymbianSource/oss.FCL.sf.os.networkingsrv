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
// key_sa.cpp - IPv6/IPv4 IPSEC PFKEY interface to SADB
// CProtocolKey methods that interface with the Security AssociationDatabase (SAD)
//



/**
 @file key_sa.cpp
*/
#include "ipaddress.h"
#include "ipsec.h"
#include "sadb.h"
#include "pfkey.h"
#include "pfkeymsg.h"
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#include "spdb.h"
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
// There should be some file just defining these numbers and nothing else... -- msa
const TUint KProtocolInetEsp    = 50;
const TUint KProtocolInetAh     = 51;
const TUint KProtocolInetIpip   = 4;

class TAcquireMessage : public TPfkeyMessage
	/**
	* An internal, transient representation of the ACQUIRE message.
	*
	* The base TPfkeyMessage contains const pointers to SADB structures
	* defined in pfkeyv2.h. TAcquireMessage extends TPfkeyMessage in such
	* way that the storage required for these base structures are allocated
	* internally here(for those extensions that are needed by the ACQUIRE)
	*/
	{
	friend class CProtocolKey;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	TAcquireMessage(const CSecurityAssoc &aSA, const CPropList *aPropList, TUint8 aTunnel, const RTrafficSelectorSet &aTS);
#else
	TAcquireMessage(const CSecurityAssoc &aSA, const TSecurityAssocSpec &aSpec, TUint8 aTunnel, const RTrafficSelectorSet &aTS);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	T_sadb_msg i_base;				//< Base Message Header
	T_sadb_address i_src_addr;		//< Source Address Extension
	T_sadb_address i_dst_addr;		//< Destination Address Extension
	T_sadb_address i_proxy_addr;	//< Proxy Address Extension
	T_sadb_ident i_src_ident;		//< Source Identity Extension
	T_sadb_ident i_dst_ident;		//< Destination Identity Extension
	T_sadb_ident i_src_endpoint;	//< Source Endpoint Extension
	T_sadb_ident i_dst_endpoint;	//< Destination Endpoint Extension
	T_sadb_sens i_sens;				//< Sensitivity Extension
	T_sadb_ts i_ts;					//< Traffic Selector
	T_sadb_prop i_prop;				//< Proposal Extension
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	struct sadb_comb i_comb[MAX_PROPOSALS_PER_SA];	 //< Proposed Parameter combination(s) (Only one generated for now)
#else
	struct sadb_comb i_comb[1];		//< Proposed Parameter combination(s) (Only one generated for now)
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	};

#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
TAcquireMessage::TAcquireMessage(const CSecurityAssoc &aSA, const CPropList *aPropList, TUint8 aTunnel, const RTrafficSelectorSet &aTs)
#else
TAcquireMessage::TAcquireMessage(const CSecurityAssoc &aSA, const TSecurityAssocSpec &aSpec, TUint8 aTunnel, const RTrafficSelectorSet &aTs)
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
:
	i_base((TUint8)SADB_ACQUIRE, aSA.iType, aSA.iSendSeq),
	i_src_addr(SADB_EXT_ADDRESS_SRC, aSA.iInfo.iProtocol),
	i_dst_addr(SADB_EXT_ADDRESS_DST, aSA.iInfo.iProtocol),
	i_proxy_addr(SADB_EXT_ADDRESS_PROXY, aSA.iInfo.iProtocol),
	i_src_ident(SADB_EXT_IDENTITY_SRC),
	i_dst_ident(SADB_EXT_IDENTITY_DST),
	i_src_endpoint(SADB_X_EXT_ENDPOINT_SRC),
	i_dst_endpoint(SADB_X_EXT_ENDPOINT_DST),
	i_sens(),
	i_ts(aTs.Count()),
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	i_prop(aSA.iReplayCheck ? KMaxReplayWindowLength : 0,  aPropList->Count())
#else
	i_prop(aSA.iReplayCheck ? KMaxReplayWindowLength : 0, 1)
#endif
	/**
	* Constructing a TAcquireMessage.
	*
	* Construct from a Larval/egg SA and Specification
	* (Some information is duplicated in SA and Specification, and could
	* equally well be accessed from either one; some info, such as lifetimes
	* is only in the specificiataion (SA lifetimes are at this stage 'undefined',
	* although the HARD is "borrowed" for expiring the larval SA in case no
	* key negotiator replies).
	*/
	{
	iError = KErrNone;
	iBase.iMsg = &i_base;
	iSrcAddr.iExt = &i_src_addr;
	iSrcAddr.iAddr = aSA.iSrc;
	iSrcAddr.iPort = aSA.iInfo.iPortSrc;
	iDstAddr.iExt = &i_dst_addr;
	iDstAddr.iAddr = aSA.iDst;
	iDstAddr.iPort = aSA.iInfo.iPortDst;
	if (!aSA.iInfo.iSrc().IsNone())
		{
		iProxyAddr.iExt = &i_proxy_addr;
		iProxyAddr.iAddr = aSA.iInfo.iSrc;
		}
	//
	//
	//
	if (aSA.iInfo.iSrcIdentity)
		{
		// Build Source Identity Extension for the PFKEY
		iSrcIdent.iExt = &i_src_ident;
		iSrcIdent.iData.Set(aSA.iInfo.iSrcIdentity->Identity());
		i_src_ident = T_sadb_ident(SADB_EXT_IDENTITY_SRC, iSrcIdent.iData.Length());
		i_src_ident.sadb_ident_type = aSA.iInfo.iSrcIdentity->Type();
		}
	if (aSA.iSrc.IsNamed())
		{
		iSrcEndpoint.iExt = &i_src_endpoint;
		iSrcEndpoint.iData.Set(aSA.iSrc.Name());
		i_src_endpoint = T_sadb_ident(SADB_X_EXT_ENDPOINT_SRC, iSrcEndpoint.iData.Length());
		i_src_endpoint.sadb_ident_type = 0;
		}
	if (aSA.iInfo.iDstIdentity)
		{
		// Build Destination Identity Extension for the PFKEY
		iDstIdent.iExt = &i_dst_ident;
		iDstIdent.iData.Set(aSA.iInfo.iDstIdentity->Identity());
		i_dst_ident = T_sadb_ident(SADB_EXT_IDENTITY_DST, iDstIdent.iData.Length());
		i_dst_ident.sadb_ident_type = aSA.iInfo.iDstIdentity->Type();
		}
	if (aSA.iDst.IsNamed())
		{
		iDstEndpoint.iExt = &i_dst_endpoint;
		iDstEndpoint.iData.Set(aSA.iDst.Name());
		i_dst_endpoint = T_sadb_ident(SADB_X_EXT_ENDPOINT_DST, iDstEndpoint.iData.Length());
		i_dst_endpoint.sadb_ident_type = 0;
		}

	if (i_ts.sadb_x_ts_numsel)
		{
		iTs.iExt = &i_ts;
		iTs.iTS = &aTs;
		}

	// Sensitivity Extension
	/* Optional, leave out */

	// Proposal Extension
	iProposal.iExt = &i_prop;
	iProposal.iComb = &i_comb[0];
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	iProposal.iNumComb = aPropList->Count();
	for (TInt propIdx = 0; propIdx < iProposal.iNumComb; propIdx++)
		{
		CSecurityProposalSpec *proposal = aPropList->At(propIdx);
		i_comb[propIdx].sadb_comb_auth = proposal->iAalg;
		i_comb[propIdx].sadb_comb_encrypt = proposal->iEalg;
		// *_flags type mismatch originates from rfc2367 (PFKEY),
		// silence warning by typecast!
		i_comb[propIdx].sadb_comb_flags = (uint16_t)(aSA.iFlags | (aTunnel ? SADB_SAFLAGS_TUNNEL : 0));
		if (i_comb[propIdx].sadb_comb_auth)
			{
			i_comb[propIdx].sadb_comb_auth_minbits = proposal->iMinAuthBits;
			i_comb[propIdx].sadb_comb_auth_maxbits = proposal->iMaxAuthBits;
			}
		if (i_comb[propIdx].sadb_comb_encrypt)
			{
			i_comb[propIdx].sadb_comb_encrypt_minbits = proposal->iMinEncryptBits;
			i_comb[propIdx].sadb_comb_encrypt_maxbits = proposal->iMaxEncryptBits;
			}
		i_comb[propIdx].sadb_comb_soft_allocations = proposal->iSoft.sadb_lifetime_allocations;
		i_comb[propIdx].sadb_comb_hard_allocations = proposal->iHard.sadb_lifetime_allocations;
		i_comb[propIdx].sadb_comb_soft_bytes = proposal->iSoft.sadb_lifetime_bytes;
		i_comb[propIdx].sadb_comb_hard_bytes = proposal->iHard.sadb_lifetime_bytes;
		i_comb[propIdx].sadb_comb_soft_addtime = proposal->iSoft.sadb_lifetime_addtime;
		i_comb[propIdx].sadb_comb_hard_addtime = proposal->iHard.sadb_lifetime_addtime;
		i_comb[propIdx].sadb_comb_soft_usetime = proposal->iSoft.sadb_lifetime_usetime;
		i_comb[propIdx].sadb_comb_hard_usetime = proposal->iHard.sadb_lifetime_usetime;
		}
#else
	iProposal.iNumComb = 1;
	i_comb[0].sadb_comb_auth = aSpec.iAalg;
	i_comb[0].sadb_comb_encrypt = aSpec.iEalg;
	// *_flags type mismatch originates from rfc2367 (PFKEY),
	// silence warning by typecast!
	i_comb[0].sadb_comb_flags = (uint16_t)(aSA.iFlags | (aTunnel ? SADB_SAFLAGS_TUNNEL : 0));
	if (i_comb[0].sadb_comb_auth)
		{
		i_comb[0].sadb_comb_auth_minbits = aSpec.iMinAuthBits;
		i_comb[0].sadb_comb_auth_maxbits = aSpec.iMaxAuthBits;
		}
	if (i_comb[0].sadb_comb_encrypt)
		{
		i_comb[0].sadb_comb_encrypt_minbits = aSpec.iMinEncryptBits;
		i_comb[0].sadb_comb_encrypt_maxbits = aSpec.iMaxEncryptBits;
		}
	i_comb[0].sadb_comb_soft_allocations = aSpec.iSoft.sadb_lifetime_allocations;
	i_comb[0].sadb_comb_hard_allocations = aSpec.iHard.sadb_lifetime_allocations;
	i_comb[0].sadb_comb_soft_bytes = aSpec.iSoft.sadb_lifetime_bytes;
	i_comb[0].sadb_comb_hard_bytes = aSpec.iHard.sadb_lifetime_bytes;
	i_comb[0].sadb_comb_soft_addtime = aSpec.iSoft.sadb_lifetime_addtime;
	i_comb[0].sadb_comb_hard_addtime = aSpec.iHard.sadb_lifetime_addtime;
	i_comb[0].sadb_comb_soft_usetime = aSpec.iSoft.sadb_lifetime_usetime;
	i_comb[0].sadb_comb_hard_usetime = aSpec.iHard.sadb_lifetime_usetime;
#endif
	}


class TExpireMessage : public TPfkeyMessage
	/**
	* An internal, transient representation of the EXPIRE message
	*
	* The base TPfkeyMessage contains const pointers to SADB structures
	* defined in pfkeyv2.h. TExpireMessage extends TPfkeyMessage in such
	* way that the storage required for these base structures are allocated
	* internally here(for those extensions that are needed by the EXPIRE)
	*/
	{
public:
	TExpireMessage(const CSecurityAssoc &aSA,
		const T_sadb_lifetime &aExpired,
		TUint32 aSeq);
private:
	T_sadb_msg i_base;              //< Base Message Header
	T_sadb_sa i_sa;
	T_sadb_address i_src_addr;      //< Source Address Extension
	T_sadb_address i_dst_addr;      //< Destination Address Extension
	T_sadb_lifetime i_current;      //< Current Life times
	};

TExpireMessage::TExpireMessage
	(
	const CSecurityAssoc &aSA,
	const T_sadb_lifetime &aExpired,
	TUint32 aSeq
	)
:
	i_base(SADB_EXPIRE, aSA.iType, aSeq),
	i_sa(aSA.iSPI, aSA.iReplayCheck ? KMaxReplayWindowLength : 0,
		aSA.iState, aSA.iAalg, aSA.iEalg, aSA.iFlags),
	i_src_addr(SADB_EXT_ADDRESS_SRC, aSA.iInfo.iProtocol),
	i_dst_addr(SADB_EXT_ADDRESS_DST, aSA.iInfo.iProtocol),
	i_current(aSA.iCurrent)
	/**
	* Constructing a TExpireMessage from a SA and expired lifetime
	*/
	{
	iError = KErrNone;
	iBase.iMsg = &i_base;
	iSa.iExt = &i_sa;
	iSrcAddr.iExt = &i_src_addr;
	iSrcAddr.iAddr = aSA.iSrc;
	iSrcAddr.iPort = aSA.iInfo.iPortSrc;
	iDstAddr.iExt = &i_dst_addr;
	iDstAddr.iAddr = aSA.iDst;
	iDstAddr.iPort = aSA.iInfo.iPortDst;
	iCurrent.iExt = &i_current;
	if (aExpired.sadb_lifetime_exttype == SADB_EXT_LIFETIME_HARD)
		iHard.iExt = &aExpired;
	else if (aExpired.sadb_lifetime_exttype == SADB_EXT_LIFETIME_SOFT)
		iSoft.iExt = &aExpired;
	}


CSecurityAssoc *CProtocolKey::Lookup(TUint8 aType, TUint32 aSPI, const TIpAddress &aDst, TInt &aHash) const
	/**
	* Find a specific (Association Type, Destination Address, SPI) security association.
	*
	* @param aType		the Association Type (AH or ESP)
	* @param aSPI		the SPI number
	* @param aDst		the destination address (maybe NONE)
	* @retval aHash	the hash value (index to iHash) corresponding
	*					to the triplet. This is always returned regardless
	*					of the search result (NULL or not)
	* @returns
	* @li non NULL,   pointer to CSecurityAssociation, if found
	* @li NULL,       if the requested association does not exist
	*/
	{
	CSecurityAssoc *sa;
	aHash = Hash(aDst, aType);
	sa = iHash[aHash];
	while (sa)
		{
		if (sa->iType == aType &&
			sa->iDst() == aDst &&
			sa->iSPI == aSPI)
			break;
		sa = sa->iNext;
		}
	return sa;
	}

CSecurityAssoc *CProtocolKey::Lookup(TUint8 aType, TUint32 aSPI, const TIpAddress &aDst) const
	/**
	* @see MAssociationManager::Lookup
	*/
	{
	TInt ignore;

	TIpAddress dst = aDst;
	for (TInt done = 0;; done = 1)
		{
		CSecurityAssoc *const sa = Lookup(aType, aSPI, dst, ignore);
		if (sa)
			{
			if (sa->iState == SADB_SASTATE_MATURE || sa->iState == SADB_SASTATE_DYING)
				return sa;
			}
		if (done)
			break;
		//
		// No incoming SA found using explicit destination address. Check for
		// SA with dst=ANY (= any address of the current node).
		//
		dst.SetAddressNone();
		}
	return NULL;
	}

void CProtocolKey::Delete(CSecurityAssoc *aSa)
	/**
	* @see MAssociationManager::Delete
	*/
	{
	if (aSa)    // Allow calling with NULL pointer (do nothing)
		{
		CSecurityAssoc *p, **pp;

		for (pp = &iHash[Hash(aSa->iDst().Address(), aSa->iType)];
			(p = *pp) != NULL;
			pp = &p->iNext) 
			if (p == aSa)
				{
				*pp = p->iNext;     // Removes aSa from the list
				// Note: SA is logically deleted from SAD, but the instance
				// may still persist for a while, if this Delete got called by
				// some method still having a handle to the SA instance. Then,
				// actual destruction is delayed until control returns to that
				// point (if caller protected the reference by Open/Close).
				(void)aSa->Close(); 
				return;
				}
		Panic(EIpsecPanic_DeleteSA);
		}
	}

void CProtocolKey::Expired(const CSecurityAssoc &aSa, TInt aType, const TLifetime &aLifetime)
	/**
	* @see MAssociationManager::Delete
	*/
	{
	T_sadb_lifetime lifetime((TUint8)aType, aLifetime, aSa.iCurrent);
	TExpireMessage expired(aSa, lifetime, ++iSequenceNumber);
	DeliverRegistered(expired);
	}
	
TInt CProtocolKey::Verify
	/**
	* @see MAssociationManager::Verify
	*/
	(
	const CSecurityAssoc *aSa,
	const TSecurityAssocSpec &aSpec,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	const CPropList *aPropList,
#endif 
	const RIpAddress &aSrc,
	const RIpAddress &aDst,
	const RPolicySelectorInfo &aInfo
	)
	{
	// The PFP processing: the specific value in RAssociationInfo needs
	// to be replaced by a special "don't care" value that is searched for.
	// Map remote/local into src/dst for incoming pacaket:
	//		association src <- "packet info remote"
	//		association dst <- "packet info local"
	RAssociationInfo info;
	if (aSpec.iMatchProxy || aSpec.iMatchRemote)
		info.iSrc = aInfo.iRemote;
	if (aSpec.iMatchLocal)
		info.iDst = aInfo.iLocal;
	info.iPortSrc = aSpec.iMatchRemotePort ? aInfo.iPortRemote : 0;
	info.iPortDst = aSpec.iMatchLocalPort ? aInfo.iPortLocal : 0;
	info.iProtocol = aSpec.iMatchProtocol ? aInfo.iProtocol : 0;
	// Identities from spec: src <- remote, dst = local
	// Reference counting for TAssociationInfo is not
	// required, as it only has temporary duration.
	info.iDstIdentity = aSpec.iIdentityLocal;
	info.iSrcIdentity = aSpec.iIdentityRemote;

	// The TS processing needs the actual values from the packet
	RTrafficSelector ts;
	ts.iSrc = aInfo.iRemote;
	ts.iDst = aInfo.iLocal;
	ts.iPortSrc = aInfo.iPortRemote;
	ts.iPortDst = aInfo.iPortLocal;
	ts.iProtocol = aInfo.iProtocol;

	RIpAddress src = aSrc;
	if (!aSpec.iMatchSrc)
		{
		if (!aDst().IsMulticast())
			{
			RIpAddress none;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
			return aSa->MatchSpec(aSpec,aPropList, src, none, info, ts);
#else
			return aSa->MatchSpec(aSpec, src, none, info, ts);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
			}
		src.Close();
		}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	return aSa->MatchSpec(aSpec, aPropList, src, aDst, info, ts);
#else
	return aSa->MatchSpec(aSpec, src, aDst, info, ts);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	}

static void AddressMask2Range(const TIpAddress &aAddr, const TIpAddress &aMask, TIpAddress &aMin, TIpAddress &aMax)
	/**
	* Convert address mask into address range.
	*/
	{
	for (TInt i = 0; i < 4; ++i)
		{
		aMin.u.iAddr32[i] = aAddr.u.iAddr32[i] & aMask.u.iAddr32[i];
		aMax.u.iAddr32[i] = aAddr.u.iAddr32[i] | ~aMask.u.iAddr32[i];
		}
	// Scope should be either 0 or same on both min and max (range does not
	// make sense with scope id's (somewhat icky...)
	aMax.iScope = aMin.iScope = aAddr.iScope & aMask.iScope;
	}

TInt CProtocolKey::Acquire
	/**
	* @see MAssociationManager::Acquire
	*/
	(
	CSecurityAssoc * &aSA,
	const TSecurityAssocSpec &aSpec,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	const CPropList *aPropList,
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	const CTransportSelector *aTS,
	const RIpAddress &aSrc,
	const RIpAddress &aDst,
	const RPolicySelectorInfo &aInfo,
	TBool aTunnel
	)
	{
	aSA = NULL;             // By default, assume no SA

	const TInt i = Hash(aDst().Address(), aSpec.iType);

	
	// The PFP processing: the specific value in RAssociationInfo needs
	// to be replaced by a special "don't care" value that is searched for.
	RAssociationInfo info;
	if (aSpec.iMatchProxy || aSpec.iMatchLocal)
		info.iSrc = aInfo.iLocal;
	if (aSpec.iMatchRemote)
		info.iDst = aInfo.iRemote;
	info.iPortSrc = aSpec.iMatchLocalPort ? aInfo.iPortLocal : 0;
	info.iPortDst = aSpec.iMatchRemotePort ? aInfo.iPortRemote : 0;
	info.iProtocol = aSpec.iMatchProtocol ? aInfo.iProtocol : 0;
	// Identities from Spec: dst <- remote, src <- local
	// Reference counting for TAssociationInfo is not
	// required, as it only has temporary duration.
	info.iDstIdentity = aSpec.iIdentityRemote;
	info.iSrcIdentity = aSpec.iIdentityLocal;
	
	// The TS processing needs the actual values from the packet
	RTrafficSelector pkt;
	pkt.iSrc = aInfo.iLocal;
	pkt.iDst = aInfo.iRemote;
	pkt.iPortSrc = aInfo.iPortLocal;
	pkt.iPortDst = aInfo.iPortRemote;
	pkt.iProtocol = aInfo.iProtocol;

	RIpAddress src;
	if (aSpec.iMatchSrc)
		src = aSrc;
	
	// Try to find a matching SA
	CSecurityAssoc *sa = iHash[i];
	for (sa = iHash[i]; sa; sa = sa->iNext)
		{
#ifdef 	SYMBIAN_IPSEC_VOIP_SUPPORT	
		if (sa->MatchSpec(aSpec, aPropList, src, aDst, info, pkt) == KErrNone)
#else
		if (sa->MatchSpec(aSpec, src, aDst, info, pkt) == KErrNone)		
#endif	//SYMBIAN_IPSEC_VOIP_SUPPORT	
			{
			// An SA exists. If state is MATURE or DYING (= soft lifetime
			// expired), allow a use of it. If not MATURE or DYING, the
			// state *MUST* be LARVAL (at least in current implementation,
			// and could actually use this in the test below, but it would
			// make reading harder, and a potential trap for future changes)
			//
			aSA = sa;
			if (sa->iState == SADB_SASTATE_MATURE ||
				sa->iState == SADB_SASTATE_DYING)
				return KErrNone;
			else
				return KRequestPending;
			}
		}

	//  Cannot find a matching SA, need to get it from the some
	//  key management daemon

	//
	// Create a larval "egg" SA (with zero SPI)
	// (This egg will match future Acquires for the same connection
	// in above loop. Larval eggs are the "acquire request queue" in
	// in this implementation.
	aSA = sa = new CSecurityAssoc(aSpec, src, aDst, info);
	if (sa)
		{
		// *NOTE*
		//      This adds new SA to the *FRONT* of the iHash[i]. This is a *FEATURE*. When
		//      an ACQUIRE is looking for a matching SA, the list is scanned from beginning
		//      (see above) and it is better that the search always picks the most recent
		//      (newest) SA for *OUTBOUND* traffic. (useful when other side has lost SAs
		//      without telling this side, and negotiates new similar ones)
		sa->iNext = iHash[i];
		iHash[i] = sa;
		//
		// Borrow the field iSendSeq for storing the sequence number of the
		// ACQUIRE message, and build the message from SA
		//
		sa->iSendSeq = ++iSequenceNumber;

		// Build the Traffic Selector from the policy selector
		RTrafficSelectorSet ts;
		TInt success = 0;
		// *NOTE* Need to use an empty "dummy" in appending to RArray,
		// because Append does not execute assign or copy constructor
		// of RTrafficSelector (for RIpAddress members!).
		RTrafficSelector dummy;
		// 1st, append the (low,high) selector, which
		// represents the packet information
		success |= ts.Append(dummy);
		success |= ts.Append(dummy);
		if (success == 0)
			{
			ts[0] = pkt;
			ts[1] = pkt;
			}
		// 2nd, generate a (low,high) pair from each policy selector
		while (aTS && success == 0)
			{
			const TInt i = ts.Count();
			success |= ts.Append(dummy);
			success |= ts.Append(dummy);
			RTrafficSelector &min = ts[i];
			RTrafficSelector &max = ts[i+1];

			// *NOTE* Translation of masks to ranges works perfectly only with masks
			// of form ~(2**N - 1). For other masks, the range selector passes more
			// than the original mask selector.
			if (aSpec.iMatchProtocol)
				{
				min.iProtocol = pkt.iProtocol;
				max.iProtocol = pkt.iProtocol;
				}
			else
				{
				min.iProtocol = aTS->iData.iProtocol & aTS->iMask.iProtocol;
				max.iProtocol = aTS->iData.iProtocol | ~aTS->iMask.iProtocol;
				}
			if (aSpec.iMatchLocalPort)
				{
				min.iPortSrc = pkt.iPortSrc;
				max.iPortSrc = pkt.iPortSrc;
				}
			else
				{
				min.iPortSrc = aTS->iData.iPortLocal & aTS->iMask.iPortLocal;
				max.iPortSrc = aTS->iData.iPortLocal | ~aTS->iMask.iPortLocal;
				}
			if (aSpec.iMatchRemotePort)
				{
				min.iPortDst = pkt.iPortDst;
				max.iPortDst = pkt.iPortDst;
				}
			else
				{
				min.iPortDst = aTS->iData.iPortRemote & aTS->iMask.iPortRemote;
				max.iPortDst = aTS->iData.iPortRemote | ~aTS->iMask.iPortRemote;
				}

			TIpAddress addr_min;
			TIpAddress addr_max;
			if (aSpec.iMatchProxy || aSpec.iMatchLocal)
				{
				min.iSrc = pkt.iSrc;
				max.iSrc = pkt.iSrc;
				}
			else
				{
				AddressMask2Range(aTS->iData.iLocal(), aTS->iMask.iLocal(), addr_min, addr_max);
				// Conversion of TIPAddress to IPV4 format in case src address is in IPV4 format 
 				if(ts[0].iSrc().IsV4Mapped())
 					{
 					const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };
 					addr_min.u.iAddr32 [0] = addr_max.u.iAddr32 [0] = v4Prefix.a[0];
 					addr_min.u.iAddr32 [1] = addr_max.u.iAddr32 [1] = v4Prefix.a[1];
 					addr_min.u.iAddr32 [2] = addr_max.u.iAddr32 [2] = v4Prefix.b;
 					}

				min.iSrc.Open(iEndPointCollection, addr_min);
				max.iSrc.Open(iEndPointCollection, addr_max);
				}
			if (aSpec.iMatchRemote)
				{
				min.iDst = pkt.iDst;
				max.iDst = pkt.iDst;
				}
			else
				{
				AddressMask2Range(aTS->iData.iRemote(), aTS->iMask.iRemote(), addr_min, addr_max);
				// Conversion of TIPAddress to IPV4 format in case destn address is in IPV4 format 
 				if(ts[0].iDst().IsV4Mapped())
 					{
 					const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };
 					addr_min.u.iAddr32 [0] = addr_max.u.iAddr32 [0] = v4Prefix.a[0];
 					addr_min.u.iAddr32 [1] = addr_max.u.iAddr32 [1] = v4Prefix.a[1];
 					addr_min.u.iAddr32 [2] = addr_max.u.iAddr32 [2] = v4Prefix.b;
 					}

				min.iDst.Open(iEndPointCollection, addr_min);
				max.iDst.Open(iEndPointCollection, addr_max);
				}
			aTS = aTS->iOr;
			}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		TAcquireMessage msg = TAcquireMessage(*sa, aPropList, aTunnel, ts);
#else
		TAcquireMessage msg = TAcquireMessage(*sa, aSpec, aTunnel, ts);
#endif
		DeliverRegistered(msg);
		// Need to set some default lifetime to the egg SA
		if (sa->TimerInit(*this) == KErrNone)
			return KRequestPending;
		}
	aSA = NULL;
	return KErrNotFound;
	}

TInt CProtocolKey::Overhead(const CSecurityAssoc *const aSa, const TIpAddress &aTunnel) const
	/**
	* Return the header overhead.
	*
	* Return additional required header space for the transformation specified by
	* the parameters.
	*
	* @param aSa The Security Associtaion (or NULL)
	* @param aTunnel The tunnel end point addresss (or unspecifed)
	*/
	{
	TInt overhead = 0;
	if (!aTunnel.IsNone())
		overhead += iEngineIPIP.Overhead(aTunnel);
	if (aSa)
		{
		if (aSa->iType == SADB_SATYPE_AH)
			overhead += iEngineAH.Overhead(*aSa);
		else if (aSa->iType == SADB_SATYPE_ESP)
			{
			overhead += iEngineESP.Overhead(*aSa);
			overhead += iEngineNATT.Overhead();         
			}
		}
	return overhead;
	}

int CProtocolKey::ApplyL
	/**
	* @see MAssociationManager::ApplyL (outgoing)
	*/
	(
	CSecurityAssoc *aSa,
	RMBufSendPacket &aPacket,
	RMBufSendInfo &aInfo,
	const TIpAddress &aTunnel
	)
	{
	if (!aTunnel.IsNone())
		{   
		iEngineIPIP.ApplyL(aTunnel, aPacket, aInfo);
		}

	if (aSa == NULL)
		return KErrNone;
	else if (aSa->iType == SADB_SATYPE_AH)
		return iEngineAH.ApplyL(*aSa, aPacket, aInfo);
	else if (aSa->iType == SADB_SATYPE_ESP) {
		TInt result = iEngineESP.ApplyL(*aSa, aPacket, aInfo, iRMBufAllocator);
		if ( result != KErrNone )
			return result;   
		else
			return iEngineNATT.ApplyL(aSa->NatTraversal(), aPacket, aInfo);  /* For NAT Traversal */
		}   
	else
		return KErrGeneral; // Ooooops?
	}

TInt CProtocolKey::ApplyL
	/**
	* @see MAssociationManager::ApplyL (incoming)
	*/
	(
	CSecurityAssoc * &aSa,
	RMBufRecvPacket &aPacket,
	RMBufRecvInfo &aInfo,
	TInt aProtocol,
	TIpAddress &aTunnel
	)
	{
	//
	// if UDP check if NAT traversal capsulation and remove it
	//
	if (aProtocol == STATIC_CAST(TInt, KProtocolInetUdp))
		{
		aProtocol = iEngineNATT.ApplyL(aSa, aPacket, aInfo);
		// There is now diffrence if the incoming UDP packet has been an
		// UDP capsulated ESP packet or not. In both cases the control
		// is returned packet to CProtocolSecpol::ApplyL (sc_prt6.cpp)
		// If the packet was an UDP capsulated ESP packet the UDP header has
		// been removed from packet and pure ESP packet is returned.
		// If packet was an ordinary UDP packet it is returned without
		// any modifications.
		aSa = NULL;
		return aProtocol;
		}
	//
	// IPSEC unwrap, if any
	//
	if (aProtocol == STATIC_CAST(TInt, KProtocolInetAh))
		aProtocol = iEngineAH.ApplyL(aSa, aPacket, aInfo);
	else if (aProtocol == STATIC_CAST(TInt, KProtocolInetEsp))
		aProtocol = iEngineESP.ApplyL(aSa, aPacket, aInfo, iRMBufAllocator);
	else
		aSa = NULL;
	//
	// Tunnel unwrap, if any
	//
	if (
		aProtocol == STATIC_CAST(TInt, KProtocolInetIpip)      // IPv4 header uncovered
		|| aProtocol == STATIC_CAST(TInt, KProtocolInet6Ipip)  // IPv6 header uncovered
		)
		{   
		aInfo.iProtocol = aProtocol;
		aProtocol = iEngineIPIP.ApplyL(aTunnel, aPacket, aInfo);
		}           
	else
		{
		aTunnel.SetAddressNone();
		}
	return aProtocol;
	}
