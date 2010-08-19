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
// sa_spec.h - IPv6/IPv4 IPSEC security associations
// This file collects minimal definitions that need to be exported
// from the Security Associations Database into Security Policy
// database
//



/**
 @internalComponent
*/
#ifndef __SA_SPEC_H__
#define __SA_SPEC_H__

#include <ip6_hook.h>
#include "ipaddress.h"
#include <networking/pfkeyv2.h>
#include <networking/crypto.h>	// only for TAlgorithmClass, is this really necessary?
#include "epdb.h"
#include "ipseclog.h"

// Selector applies to inbound packets
const TUint KPolicyFilter_INBOUND = 0x1;
// Selector applies to outbound packets
const TUint KPolicyFilter_OUTBOUND = 0x2;
// Selector applies to both inbound and outbound packets
const TUint KPolicyFilter_SYMMETRIC =
	KPolicyFilter_INBOUND|KPolicyFilter_OUTBOUND;

// Indicate tunnel mode for Acquire and Verify (not really a filter).
const TUint KPolicyFilter_TUNNEL = 0x4;

// Selector can be merged with another matched selector.
const TUint KPolicyFilter_MERGE = 0x8;

// Negate selector -- drop if match occurs.
const TUint KPolicyFilter_DROP = 0x10;
// Final selector, do not look for "merge" selectors after this.
const TUint KPolicyFilter_FINAL = 0x20;

//Exception flag for policy filter. This flag will allow only exception scope traffic and drop rest
//special usecase for UMA traffic.
const TUint KPolicyFilter_Exception = 0x40;

/**
* Ports is present in selector.
* The value indicates whether the content is actual port (1) or a packet
* type/code (0) from ICMP, MH or other similar protocols.
*/
const TUint KTransportSelector_PORTS = 0x1;

// Object tracking in the windows environment to
// help detecting possible memory leaks.
#if __WINS__ && _DEBUG
	extern int IPSEC_OBJECT_COUNT;
	#define	IPSEC_OBJECT_INC	++IPSEC_OBJECT_COUNT
	#define	IPSEC_OBJECT_DEC	--IPSEC_OBJECT_COUNT
	#define	IPSEC_OBJECT_TRACKING 1
// ..when desperate, use the following ..
//	#define	IPSEC_OBJECT_INC	do { Log::Printf(_L("INC[%u] %d"), (TInt)this, ++IPSEC_OBJECT_COUNT); } while (0)
//	#define	IPSEC_OBJECT_DEC	do { Log::Printf(_L("DEC[%u] %d"), (TInt)this, --IPSEC_OBJECT_COUNT); } while (0)
#else
	#define	IPSEC_OBJECT_INC
	#define	IPSEC_OBJECT_DEC
	#undef	IPSEC_OBJECT_TRACKING
#endif


class CIpsecReferenceCountObject : public CBase
	/**
	* The base class for all IPSEC reference count objects.
	*
	* Many IPSEC objects are implemented as "reference counted objects"
	* which automaticly delete self, when the last reference is removed.
	*
	* The contruction of the object is counted as one reference.
	* The Open() and Close() methods are not supposed to be overridden,
	* and are not defined "virtual". This base class attempts to be
	* as light as possible
	*
	* The destructor is virtual and private. The reference counted
	* objects are never deleted from outside, only from the Close()
	* method.
	*/
	{
public:
#ifdef IPSEC_OBJECT_TRACKING
	// Non-default constructor only needed for debugging
	CIpsecReferenceCountObject() 
		{
		IPSEC_OBJECT_INC;
		}
#endif
	inline void Open();
	inline TInt IsShared() const;
	void Close();
protected:
	virtual ~CIpsecReferenceCountObject()
		{
#ifdef IPSEC_OBJECT_TRACKING
		// Non-empty destructor only needed for debugging
		IPSEC_OBJECT_DEC;
#endif
		}
	TInt iRefs;
	};

void CIpsecReferenceCountObject::Open()
	/**
	* Increment reference count.
	*
	* Records an additional reference to the object.
	*/
	{
	++iRefs;
	}

TInt CIpsecReferenceCountObject::IsShared() const
	/**
	* Return the current reference count.
	*
	* The reference count works as implicit flag to
	* test whether object has more than one reference
	* to it (is shared).
	*
	* @li = 0, only one reference exist (not shared)
	* @li > 0, other references exist (object is shared)
	*
	* @return The reference count
	*/
	{
	return iRefs;
	}

class RPolicySelectorInfo
	/**
	* The policy selector information. This defines the IPSEC selector data layout,
	* as is used as basic component in the transport selectors. This is also the
	* basic information block, which is extracted from a packet (or flow).
	*/
	{
public:
	inline void FillZ();

	TUint8 iFlags;				//< Transport Selector flags (KTransportSelector_*)				
	TUint8 iProtocol;			//< IP Protocol code (0..255)
	TUint8 iReserved1;			//< Reverved for future use.(now for alignment only)
	TUint8 iReserved2;			//< Resevved for future use (now for alignment only)
	//
	TUint16 iPortRemote;		//< Remote port
	TUint16 iPortLocal;			//< Local port
	//
	RIpAddress iRemote;			//< Remote Address with scope id
	RIpAddress iLocal;			//< Local Address with scope id
	};


void RPolicySelectorInfo::FillZ()
	// Fill all fields with ZERO (including possible compiler generated padding)
	{
	TUint32 *const p = (TUint32 *)this;
	p[0] = p[1] = 0;	// iFlags, iProtocol, reserveds, iPortRemote, iPortLocal
	}


class TPolicyFilterInfo
	/**
	* The policy control and filter information.
	*
	* This defines the basic data for choosing the
	* selectors which are tested for transport selectors.
	* The filter can be based on
	*
	* - packet direction: inboud or outbound
	* - interface index
	*
	* This includes some other action controlling
	* flags, which affect the actions when the
	* selector matches.
	*
	* - merge
	* - final
	* - drop
	*
	*/
	{
public:
	TUint32 iFlags;		//< The KPolicyFilter_* flags.
	TUint32 iIndex;		//< The Interface Index.
	};

class CTransportSelector : public CIpsecReferenceCountObject
	/**
	* The Transport Selector.
	*
	* A transport selector is a list of basic selectors or'ed together.
	* The Transport Selector matches, if any of the basic selectors in
	* the list match.
	*
	* The basic transport selector is a reference counted object. The contruction
	* counts as the first reference and must be matched with corresponding
	* Close(). Record additional references with Open().
	*
	* The contruction automaticly increments the next Or'ed selector. Once
	* constructed, the selector is immutable. Aside from the reference count
	* house keeping, the content of the selector is constant.
	*/
	{
public:
	CTransportSelector(const RPolicySelectorInfo &aData, const RPolicySelectorInfo &aMask, CTransportSelector *const aOr);
	TInt Match(const RPolicySelectorInfo &aKey) const;
private:
	~CTransportSelector();
	TInt iRefs;				 				//< The reference count.
public:

	// immutable after construction
	const RPolicySelectorInfo iData;		//< The selector data (the values to check)
	const RPolicySelectorInfo iMask;		//< The selector mask (what values to check)
	CTransportSelector *const iOr;			//< Next alternative or NULL.
	};

//
// Mapping of low level types in pfkeyv2.h into more semantic names
// (This is to avoid a need to look many places in case pfkeyv2 changes)
//
typedef uint32_t TLifetimeAllocations;
typedef uint64_t TLifetimeBytes;
typedef uint64_t TLifetimeSeconds;


/**
// The default life time in seconds for larval SA's.
// Larvar SA is created by GETSPI (may also be used as a default for
// iLarvalLifetime in TSecurityAssocSpec).
*/
const TInt KLifetime_LARVAL_DEFAULT = 90;

class CIdentity : public CIpsecReferenceCountObject
	/**
	* A container for the Identity string.
	*/
	{
public:

	// Create and construct a new Identity block
	static CIdentity *NewL(const TDesC &aIdentity);
	static CIdentity *NewL(const TDesC8 &aIdentity, TUint16 aType);
	inline TUint16 Type() const	{return iType; }
	inline TInt Match(const CIdentity &aOther) const;
private:
	// Construct and destruct are private, triggered internally.
	CIdentity(TUint32 aLength) : iTypeLength(aLength) {};
	~CIdentity() {}

	TUint16 iType;						//< Type of the identity string (PREFIX, FQDN, USERFQDN)

	// //
	// *WARNING* *WARNING* *WARNING*
	// What now follows, is the TLitC8 structure.
	// The extra space is allocated only, if iTypeLength
	// is non-zero.
	// //
	// Why this TLitC8 "hack" instead of traditional
	// C construct with a "length" member and "fake buf[1]"?
	//
	// As far as layout, this is exactly the same. The TLitC8
	// "hack" only forces a Symbian specific layout. When a
	// descriptor is needed, it doesn't need to be constructed,
	// it's already existing and just returning a reference
	// to iTypeLength as TLitC8 is sufficient.
	// //
	const TUint iTypeLength;			//< TypeLength of TLitC8
public:
	inline const TDesC8 &Identity() const
		/**
		* Return the Identity string.
		*
		* @return The identity
		*/
		{
		return ((TLitC8<1> &)iTypeLength)();
		}
	};
	
TInt CIdentity::Match(const CIdentity &aOther) const
	/*
	* Return ETrue, if identities match.
	*
	* @param aOther The other identity
	* @return result of comparison.
	*/
	{
	return Identity() == aOther.Identity();
	}


// TLifetime, a help structure

class TLifetime
	{
public:
	TLifetime(const struct sadb_lifetime &aLifetime);
	static void Freeze(TTime &aTime, const TTime &aNow);
	TLifetime();
	// For current, these will count items used so far. For Hard and
	// Soft these will contain the limit values for the current
	// counts.
	// study: present unspecified limit with 0 or max value?
	TLifetimeAllocations iAllocations;	// Connections limit
	TLifetimeBytes iBytes;				// Transmitted bytes limit
	//
	// For Current, these will record the creation and first use times.
	// For Hard and Soft, these will record the expiration times (e.g.
	// simple comparison with the current time can be used to test for
	// expiration, and for returning CURRENT values to application, use
	// the SecondsFrom method with current.
	//
	TTime iAddtime;						// Lifetime limit from creation
	TTime iUsetime;						// Lifetime limit from first use
	};


class TSecurityAssocSpec
	/**
	* Security Association template.
	*
	* The TSecurityAssocSpec is a template for a Security Association.
	* This information and the information extracted from the packet is used
	* to locate a matching Security Association.
	*/
	{
public:
	TUint8 iType;				//< Security Association type (AH or ESP)
	TUint8 iAalg;				//< Authentication algorithm number
	TUint8 iEalg;				//< Encryption algorithm number
	TUint8 iReplayWindowLength;	//< Use Replay Window
	TUint iPfs:1;				//< "Perfect Forward Secresy" (PFS)

	/**
	* The SA is local address specicic.
	*
	* When set, the SA's is bound to a specific local
	* address. If not set, the SA can be used with any of
	* the currently valid own addresses.
	*
	* Note: The member name "iMatchSrc" is misleading.
	*/
	TUint iMatchSrc:1;

	// MatchProxy retained for backward compatibility
	TUint iMatchProxy:1;		//< (PFP) (deprecated) incoming == iMatchRemote, outgoing == iMatchLocal

	// The PFP (Populate From Packet) flags
	TUint iMatchLocal:1;		//< (PFP) Specific to local address of the packet
	TUint iMatchRemote:1;		//< (PFP) Specific to remote address of the packet
	TUint iMatchProtocol:1;		//< (PFP) Specific to protocol of the packet
	TUint iMatchLocalPort:1;	//< (PFP) Specific to local port of the packet
	TUint iMatchRemotePort:1;	//< (PFP) Specific to remtoe port of the packet

	// Identity references
	CIdentity *iIdentityLocal;	//< The local Identity
	CIdentity *iIdentityRemote;	//< The remote Identity

	// Limits for key lengths (for ACQUIRE only)
	TUint16 iMinAuthBits, iMaxAuthBits;			//< Required length of the authentication key
	TUint16 iMinEncryptBits, iMaxEncryptBits;	//< Required length of the encryption key

	/**
	* Max time for the Key Managers to handle ACQUIRE request.
	*
	* iLarvalLifetime specifies the maximum time to wait, after
	* an ACQUIRE request originating from this template is sent
	* to the key manager(s). This time should be long enough to
	* allow key manager to complete the negotiation for an
	* association.
	*
	* If not specified (=0), the default is #KLifetime_LARVAL_DEFAULT
	*/
	TUint iLarvalLifetime;

	// Required lifetimes
	struct sadb_lifetime iHard;	//< Hard Lifetime requirement (copied into ACQUIRE)
	struct sadb_lifetime iSoft;	//< Soft Lifetime requirement (copied into ACQUIRE)
	};

class TAlgorithmMap
	/**
	* Map symbolic algorithm name with algorithm number.
	*
	* The symbolic name of the algorithm identifies the implementation. That is, the name
	* given here is matched against the names advertised by the installed cryptograpchic
	* libaries. Each installed cryptographic library provides a list of supported algorithm
	* as names. This name is local concept.
	*
	* The algorithm number is the externally visible identification, which
	* is used by the key management negotiations (in IKE etc), and appears in PFKEY messages.
	*/
	{
public:
	TAlgorithmMap(TAlgorithmClass aClass, TInt anId, TInt aBits, const TDesC &aLibrary, const TDesC &aAlgorithm);
	TAlgorithmClass iClass;		//< Algorithm Class (digest, cipher, ...)
	TInt iId;					//< IPsec algorithm id number
	TInt iBits;					//< Actual # of bits to be used (digest only)

	/**
	* Name of the library instance.
	* If iLibrary is empty,
	* then the first matching algorithm from any of the
	* installed libraries is used.
	*/
	TProtocolName iLibrary;
	/**
	* Name of the algorithm.
	* If iAlgorithm is empty, then
	* this map entry describes a NULL algorithm. No libraries
	* are searched.
	*/
	TAlgorithmName iAlgorithm;
	};

class CAlgorithmList : public CArrayFixFlat<TAlgorithmMap>
	/**
	* List of potentially available algorithtms.
	*
	* CAlgorithmList is an array of TAlgorithmMap entries and provides methods
	* for finding a specific map and adding new mapping. The use of this information
	* is documented in more detail in connection of the Cryptographic library.
	*
	* The algorithm list defines all potentially available algorithms. The actual
	* set of available algorithms depends on currently installed crypto libraries.
	*/
	{
public:
	CAlgorithmList();
	void AddL(TAlgorithmClass aClass, TInt anId, TInt aBits, const TDesC &aLibrary, const TDesC &anAlg);
	TAlgorithmMap *Lookup(TAlgorithmClass aClass, TInt anAlg) const;
	TAlgorithmMap *Lookup(const TDesC &aLibrary, const TDesC &anAlg) const;
	};



class CSecurityAssoc;
class REndPoints;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
class CPropList;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT


class MAssociationManager
	/**
	* The Security Association Database (SDB) interface as seen by Security Policy.
	*/
	{
public:
	virtual void Open() = 0;
	virtual void Close() = 0;

	/**
	* Acquire a new Security Association.
	*
	* SECPOL calls this when it needs a Security Association for a flow.
	*
	* If a matching Security Association already exists, it is returned.
	* Otherwise this generates a PFKEY ACQUIRE message for each registered
	* key management application and the function returns without SA.
	*
	* When not found, this creates a "larval egg SA" that will match any
	* future request with same parameters. This prevents generating multiple
	* ACQUIRE messages for the same security association.
	*
	* @retval aSa	located SA
	* @param aSpec	SA requirements
	* @param aPropList The list of (possibly multiple) proposals pertaining to the aSa
	* @param aTS	The traffic selector.
	* @param aSrc	the source address (of SA)
	* @param aDst	the destination address (of SA)
	* @param aInfo	the selector information
	* @param aTunnel True, when association is used in tunnel mode.
	* @returns
	* @li	KErrNone        SA found and returned.
	* @li	KRequestPending SA found (or created), but in LARVAL state
	* @li	KErrDied        the new larval SA expired (some weird problem)
	* @li	KErrNotFound    creating SA failed (parameters error? memory?)
	*/
	virtual TInt Acquire(
		CSecurityAssoc * &aSa,
		const TSecurityAssocSpec &aSpec,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		const CPropList *aPropList,
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
		const CTransportSelector *aTS,
		const RIpAddress &aSrc,
		const RIpAddress &aDst,
		const RPolicySelectorInfo &aInfo,
		TBool aTunnel) = 0;

	/**
	* Verify Security Association.
	*
	* SECPOL calls this to verify that the applied SA matches the
	* policy specification.
	*
	* @param aSa	the SA to be verified
	* @param aSpec	the required SA features
	* @param aSrc	the source address from the packet
	* @param aDst	the destination address of the packet
	* @param aInfo	the selector information
	*
	* @returns
	*	@li	KErrNone, when all is OK
	*	@li	error < 0, when something doesn't match
	*/
	virtual TInt Verify(
		const CSecurityAssoc *aSa,
		const TSecurityAssocSpec &aSpec,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		const CPropList *aPropList,
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT	
		const RIpAddress &aSrc,
		const RIpAddress &aDst,
		const RPolicySelectorInfo &aInfo) = 0;
		
	
	/**
	* Outgoing packet transformation for IPSEC
	*
	* SECPOL calls this once for each packet for each IPsec required
	* IPsec transform (ESP or AH).
	*
	* First, applies the tunnel transform, if present.
	* After this, applies the IPSEC transform specified by the Security
	* Association if present.
	* Having a NULL SA parameter allows this to be used as a plain
	* tunnel wrapper.
	*
	* @param aSa		The SA to be applied to the packet (or NULL)
	* @param aPacket	The outgoing packet
	* @param aInfo		The info block associated with the packet
	* @param aTunnel	The outer tunnel destination (request tunneling if specified)
	*
	* @returns
	* @li KErrNone, if transformation successfully done
	* @li KErrGeneral, otherwise (some error condition occurred)
	*/
	virtual TInt ApplyL(
		CSecurityAssoc* aSa,
		RMBufSendPacket &aPacket,
		RMBufSendInfo &aInfo,
		const TIpAddress &aTunnel) = 0;

	/**
	* Incoming packet transformation for IPSEC (one layer).
	*
	* SECPOL calls this for each incoming packet when the next
	* procotol indicates IPsec header (AH or ESP).
	*
	* Decode IPSEC layer from the received packet and return the
	* the applied Security Association and the optional tunnel
	* address.
	*
	* @retval aSa		Returns the SA that was used by this transformation (if any)
	* @param aPacket	The incoming packet
	* @param aInfo		The info block associated with the packet
	* @param aProtocol	The protocol (either AH or ESP, and maybe UDP for NAT traversal)
	* @retval aTunnel	Returns outer source address, if detunneling was done. Otherwise
	*					just unspecified address.
	* @returns
	* @li	< 0, transform failed with error
	* @li	>= 0, transform succesfull, the next protocol id after unwrap.
	*
	* @exception leave	transform failed with an error
	*
	* If the input aProtocol is not ESP, AH or IP-in-IP, this function does
	* nothing and just returns the aProtocol and packet unchanged!!
	*/
	virtual TInt ApplyL(
		CSecurityAssoc* &aSa,
		RMBufRecvPacket &aPacket,
		RMBufRecvInfo &aInfo,
		TInt aProtocol,
		TIpAddress &aTunnel) = 0;

	/**
	* Returns the maximum overhead caused by this SA/Tunnel combination
	* for an outbound packet.
	*
	* @param aSa		the association (can be null)
	* @param aTunnel	request IPSEC tunneling, if address is specified
	*
	* @return	the header overhead caused by the transformation
	*/
	virtual TInt Overhead(const CSecurityAssoc *const aSa, const TIpAddress &aTunnel) const = 0;

	/**
	* Unconditionally remove all references to the SA and destroy the object,
	*
	* The Security Association must be deleted by this function, the ~CSecurityAssociation()
	* destructor must not be invoked from outside this function.
	*
	* Remove the association from the hash table (iHash) and terminate
	* the pending timer (if any).
	*
	* @param aSa	The SA (NULL also allowed for NOP)
	*/
	virtual void Delete(CSecurityAssoc *aSa) = 0;

	/**
	* Activate a timeout callback for SA.
	*
	* CSecurityAssociation calls this to set a timer for self when the SA
	* has time based lifetime. Unless cancelled, timeout expiration calls the
	* CSecurityAssoc::TimerExpired after aDelta seconds has passed.
	*
	* @param aSa The affected SA
	* @param aDelta The timeout
	*/
	virtual void TimerOn(CSecurityAssoc &aSa, TInt aDelta) = 0;

	/**
	* Generate Expired message.
	* Called by CSecurityAssocition, when it detects that lifetime has expired (hard)
	* or is about to expired (soft). Generate an Expired message and deliver it to all
	* interested parties.
	*
	* @param aSa	The association
	* @param aType	Expiration type (SADB_EXT_LIFETIME_SOFT, SADB_EXT_LIFETIME_HARD)
	* @param aLifetime Expired lifetime
	*/
	virtual void Expired(const CSecurityAssoc &aSa, TInt aType, const TLifetime &aLifetime) = 0;

	/**
	* Deliver Algorithm map from policy to Security Association Database (SAD).
	*
	* If successful, the SAD takes ownership of the table and sets aList to NULL.
	* If not succesful, the ownership of the table remains
	* with the caller (aList is not changed).
	*
	* @param aList	The algorithm List.
	*/
	virtual void SetAlgorithms(CAlgorithmList*& aList) = 0;

	/**
	* Find an SA matching the parameters.
	*
	* If SA cannot be located with the given destination address, the search
	* is repeated with no destination address.
	*
	* This function exists ONLY for locating the INCOMING SA for a packet,
	* which has AH or ESP header..
	*
	* @param aType		the Association Type (AH or ESP)
	* @param aSPI		the SPI number
	* @param aDst		the destination address (never NONE)
	* @returns
	* @li non NULL,   pointer to CSecurityAssociation, if found
	* @li NULL,       if the requested association does not exist
	*/
	virtual CSecurityAssoc *Lookup(TUint8 aType, TUint32 aSPI, const TIpAddress &aDst) const = 0;

	/**
	* Return the named EndPoint collection.
	*
	* Security Associtiations can be bound to named end points. The same EP's can be
	* referenced in the security policy. To allow this, the Security Policy (SPD) and
	* Security Association Databases (SAD) must use a shared "name space" for the
	* end points.
	*
	* The end point collection is owned by SAD, and the SPD needs to find a reference
	* to the same instance using this function.
	*
	* @return The end point collection.
	*/
	virtual REndPoints &EndPointCollection() = 0;
	};


class RSecurityAssociation;

/**
* Security Association callback.
*/
typedef void (*SecurityAssociationCallback)(RSecurityAssociation &aAssoc);

class RSecurityAssociation : public RCircularList
	/**
	* Security Association handle.
	*
	* The handle contains a reference to a security association. The circular
	* list links together handles, which reference the same association. The
	* "head" of the list is in the security association.
	*
	* The callback function is called when the state of the association changes
	* in any way.
	*/
	{
	friend class CSecurityAssoc;
	friend class CProtocolKey;
public:

	TInt Status() const;
	void Init(SecurityAssociationCallback aCallback);
	void None();
	void Reset(CSecurityAssoc *aSa);

	inline CSecurityAssoc *Association() const
		{
		return iAssociation;
		}
	inline TInt ReadErr()const { return iErrorVal; }
	inline void SetError(TInt aError) { iErrorVal=aError; }
private:
	CSecurityAssoc *iAssociation;			//< The security association.
	SecurityAssociationCallback iCallback;	//< The callback function.
	TInt iErrorVal;				            // Stores the error value returned by KMD server
	};


#endif
