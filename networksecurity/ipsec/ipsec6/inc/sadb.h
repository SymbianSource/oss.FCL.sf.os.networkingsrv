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
// sadb.h - IPv6/IPv4 security association database
//



/**
 @internalComponent
*/
#ifndef __SADB_H__
#define __SADB_H__

#include <e32std.h>
#include <timeout.h>
#include <networking/pfkeyv2.h>
#include "sa_spec.h"
#include "pfkeymsg.h"
#include "natt_eng.h"
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#define MAX_PROPOSALS_PER_SA 8
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
class CIpsecCryptoManager;
class CAuthenticationBase;
class CEncryptionBase;

class TExtendedSequenceNumber
	/**
	* The 64-bit sequence number.
	*/
	{
public:
	inline TUint32 operator++ ();
	inline int operator! () const;
	inline TExtendedSequenceNumber & operator= (TUint32);
	inline operator TUint32() const;
	inline void SetHigh(TUint32);
	inline TUint32 High() const;
private:
	TUint32 iLow;
	TUint32 iHigh;
	};

TUint32 TExtendedSequenceNumber::operator++ ()
	{
	if (++iLow == 0)
		++iHigh;
	return iLow;
	}
	
int TExtendedSequenceNumber::operator! () const
	{
	return (iLow == 0) && (iHigh == 0);
	}
	
TExtendedSequenceNumber &TExtendedSequenceNumber::operator= (TUint32 aVal)
	{
	iLow = aVal;
	return *this;
	}
	
TExtendedSequenceNumber::operator TUint32() const
	{
	return iLow;
	}
	
void TExtendedSequenceNumber::SetHigh(TUint32 aVal)
	{
	iHigh = aVal;
	}
	
TUint32 TExtendedSequenceNumber::High() const
	{
	return iHigh;
	}

class RTrafficSelector
	/**
	* Internal Traffic Selector
	*/
	{
public:
	TBool operator<=(const RTrafficSelector &aSel) const;

	RIpAddress iSrc;			//< Source Address
	RIpAddress iDst;			//< Destination Address
	TUint16 iPortSrc;			//< Source port [0..65535]	
	TUint16 iPortDst;			//< Destination port [0..65535]
	TUint8 iProtocol;			//< Protocol [0..255]
	};
	
class RTrafficSelectorSet : public RArray<RTrafficSelector>
	/**
	* A collection of Traffic Selectors
	*/
	{
public:
	~RTrafficSelectorSet() { Reset();}
	void Close() { Reset(); RArray<RTrafficSelector>::Close(); }
	void Reset();
	};

class RAssociationInfo : public RTrafficSelector
	/**
	* Security association info
	*
	* This contains the assocation specific selector information.
	*
	* The fields of RTrafficSelector implement the PFP (Populate
	* From Packet) feature, but with "reinterpeeted" member names
	*/
	{
public:
	CIdentity *iSrcIdentity;	//< Source Identity Pointer
	CIdentity *iDstIdentity;	//< Destination Identity Pointer
	};


class CSecurityAssoc : public CBase
	/**
	* Security Association (SA).
	*
	* This CSecurityAssoc class implements the Security Association Database (SAD)
	* for functionality that deals with individual, and isolated SA instances
	* (the MAssociationManager = CProtocolKey, takes care of the managing of
	* the collection of these SA instances).
	*/
	{
	friend class CProtocolKey;
	friend class TIpsecAH;
	friend class TIpsecESP;
	friend class TAcquireMessage;
	friend class TExpireMessage;
	friend class CSecurityAssocTimeoutLinkage;
public:
	inline TInt State() const { return iState; }
	inline TUint32 SPI() const { return iSPI; }
	inline CNatTraversal* NatTraversal() const { return iNatTraversal; }	
	TInt UpdateL(MAssociationManager &aManager, const TPfkeyMessage &aMsg, CIpsecCryptoManager *aCryptoLib);
	inline TUint32 TunnelIndex() const { return iTunnelIndex; }
	void Attach(RSecurityAssociation &aHandle) { iHandles.Attach(aHandle); }
private:
	CSecurityAssoc(const TPfkeyMessage &aMsg);
	CSecurityAssoc(const TSecurityAssocSpec &aSpec,
		const RIpAddress &aSrc,
		const RIpAddress &aDst,
		const RAssociationInfo &aInfo);
	void Cleanup();
	// *NOTE* Open/Close are still private methods internal
	// to the class and it's friends! Not for general use!
	inline void Open() { ++iRefs; }
	// Close returns KErrDied, if instance is deleted and
	// KErrNone otherwise!
	TInt Close();
	~CSecurityAssoc();
	void SetErrorValue(TInt errValue)	{ iErrValue = errValue; }
	int ReplayCheck(TUint32 aSeq);
	void ReplayUpdate(TUint32 aSeq);
	TInt MatchSpec(const TSecurityAssocSpec &aSpec,
#ifdef 	SYMBIAN_IPSEC_VOIP_SUPPORT	
     const CPropList *aPropList,
#endif //		

		const RIpAddress &aSrc,
		const RIpAddress &aDst,
		const RAssociationInfo &aInfo,
		const RTrafficSelector &aPkt) const;
	//
	//
	//
	TInt TimerExpired(MAssociationManager &aManager, const TTime &aNow);
	TInt CountExpired(MAssociationManager &aManager);
	TInt TimerInit(MAssociationManager &aManager);
	TInt MarkUsed(MAssociationManager &aManager);

	// Security Association State
	TUint8 iType;				//< SA Type (AH or ESP)
	TUint8 iState;				//< State of the SA (LARVAL, MATURE, DYING, DEAD)
	TUint iUsed:1;				//< =1, after first use of this SA.
	TUint iTestInWindow:1;		//< (Replay Check) iTestSeq is within window
	TUint iReplayCheck:1;		//< (Replay Check) =1, if replay check is enabled

	TUint32 iSPI;				//< Assigned SPI number (network byteorder!)
	TUint32 iFlags;				//< Security Association Flags
	CSecurityAssoc *iNext;		//< Links SAs with same hash code together.
	RCircularList iHandles;		//z Attached handles
	TInt DoCallbacks();
	/**
	* The reference count.
	* A very short term reference count to be used when
	* calling methods whose side effect may be the destruction
	* of this SA. The CSecurityAssoc destructor is run, when
	* this count goes negative! Initial value is ZERO!
	* (see Open()/Close() methods)
	*/
	TInt iRefs;

	// Security Association end points
	// SAs must be unique by triple (iType, iSPI, iDst)
	RIpAddress iDst;			//< Dst Address of the SA
	RIpAddress iSrc;			//< Src Address of the SA

	// Additional Negotiated Information
	RAssociationInfo iInfo;
public:
	RTrafficSelectorSet iTS;	
private:
	CNatTraversal *iNatTraversal;	//< NULL or NAT Traversal state
	TUint32 iTunnelIndex;			//< Associated VPN interface (set through a special proxy address).

	/**
	* Current Lifetime tracking.
	*
	* @li iCurrent.iAddtime
	*	the universal time of SA creation
	* @li iCurrent.iUsetime
	*	NullTTime(), before first use and the corresponding
	*	Hard/iSoft values contain the lifetime in seconds.
	*	the universal time of first SA use, iHard/iSoft
	*/
	TLifetime iCurrent;
	TLifetime iHard;			//< Hard lifetime limits
	TLifetime iSoft;			//< Soft lifetime limits

	TInt iErrValue; 			//< Error code from KMD Server

	// Encryption and authentication section
	TUint8 iAalg;				//< Authentication algorithm number
	TUint8 iEalg;				//< Encryption algorithm number
	HBufC8 *iIV;				//< A temp buffer for iv
	CAuthenticationBase *iAeng;	//< Authentication engine instance
	CEncryptionBase *iEeng;		//< Encryption engine instance
	

	// Running information
	TExtendedSequenceNumber iTestSeq;	//< The sequence nr currently being tested.
	TExtendedSequenceNumber iRecvSeq;	//< The sequence nr in previous incoming header
	TExtendedSequenceNumber iSendSeq;	//< The sequence nr in previous outgoing header
	TUint32 iBitmap[4];			//< The bitmap for anti-replay service (now 128 bits max)

	static inline TInt BitmapWord(TUint32 aSeq)
		// Compute the index to bitmap [0..4]
		{ return (aSeq >> 5) & 0x3; }
	static inline TInt BitmapBit(TUint32 aSeq)
		// Compute the bit numer [0..31] within bitmap element
		{return aSeq & 0x1f; }
public:
	RTimeout iTimeout;			//< Lifetime timer Queue
	};

// * This value must be a power of 2 (2**n)
const TUint32 KMaxReplayWindowLength = 4 * 32;

#endif
