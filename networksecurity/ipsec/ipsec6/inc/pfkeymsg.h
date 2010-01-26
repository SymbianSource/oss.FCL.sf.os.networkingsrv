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
// pfkeymsg.h - IPv6/IPv4 IPSEC PFKEY message handling utilities
// PF_KEY v2 message transformation between the socket stream and
// internal representation.
//



/**
 @internalComponent
*/
#ifndef __PFKEYMSG_H__
#define __PFKEYMSG_H__

#include <es_mbuf.h>
#include "ipaddress.h"
#include <networking/pfkeyv2.h>
#include "pfkeyext.h"
#include "sa_spec.h"

// Descriptor representing a single ZERO byte (octet)
const TLitC8<1> KZeroByte = {1, {0}};

//
// Map the basic PFKEY V2 structures into Classes with
// constructors for initialized content.
class T_sadb_msg : public sadb_msg
	{
public:
	T_sadb_msg(TUint8 aMsgType, TUint8 aSaType = 0, TUint32 aSeq = 0);
	};

class T_sadb_sa : public sadb_sa
	{
public:
	T_sadb_sa(TUint32 aSPI = 0, TUint8 aWindow = 0, TUint8 aState = SADB_SASTATE_LARVAL,
		TUint8 aAalg = 0, TUint8 aEalg = 0, TUint32 aFlags = 0);
	};


//	T_sadb_lifetime
class T_sadb_lifetime : public sadb_lifetime
	{
public:
	T_sadb_lifetime(const TLifetime &aLt);
	T_sadb_lifetime(TUint8 aType, const TLifetime &aLt, const TLifetime &aRef);
	};


class T_sadb_address : public sadb_address
	{
public:
	T_sadb_address(TUint8 aType, TUint8 aProto = 0, TUint8 aPrefix = 0);
	};

class T_sadb_key : public sadb_key
	{
public:
	T_sadb_key(TUint8 aType, TInt aKeyBytes = 0, TInt aKeyBits = -1);
	};

class T_sadb_supported : public sadb_supported
	{
public:
	T_sadb_supported(TUint8 aType, TInt aNum = 0);
	};

class T_sadb_ident : public sadb_ident
	{
public:
	T_sadb_ident(TUint8 aType, TInt aLength = 0);
	};

class T_sadb_sens : public sadb_sens
	{
public:
	// A dummy
	T_sadb_sens();
	};

class T_sadb_prop : public sadb_prop
	{
public:
	T_sadb_prop(TUint8 aReplay, TInt aNum);
	};

class T_sadb_selector : public sadb_x_selector
	{
public:
	TInetAddr iSrc;
	TInetAddr iDst;
	};

class T_sadb_ts : public sadb_x_ts
	{
public:
	T_sadb_ts(TInt aNum);
	};

//
// Remapping of the bytestream PF_KEY into structures
//

class TPfkeyBase
	{
public:
	const struct sadb_msg *iMsg;
	TPfkeyBase() {iMsg = 0;}
	TUint Length() const {return iMsg ? sizeof(*iMsg) : 0;}
	TInt ByteStream(RMBufChain &aPacket, TInt aTotal) const;
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};

class TPfkeyAssociation
	{
public:
	const struct sadb_sa *iExt;
	TPfkeyAssociation() {iExt = 0;}
	TUint Length() const {return iExt ? sizeof(*iExt) : 0;}
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel, const CAlgorithmList *aAlgorithms) const;		// Only available in DEBUG mode
	};

class TPfkeyLifetime
	{
public:
	const struct sadb_lifetime *iExt;
	TPfkeyLifetime() {iExt = 0;}
	TUint Length() const {return iExt ? sizeof(*iExt) : 0;}
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};

class TPfkeyIdentity
	{
public:
	const struct sadb_ident *iExt;
	TPtrC8 iData;
	TPfkeyIdentity() {iExt = 0;}
	TUint Length() const
		{return iExt ? ((sizeof(*iExt) + iData.Length() + 1 + 7) / 8) * 8 : 0; }
	TInt LoadFromStream(const TInt aLength, const TUint8 *aPtr);
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC8 &aLabel) const;		// Only available in DEBUG mode
	};

class TPfkeyAddress
	{
public:
	const struct sadb_address *iExt;
	RIpAddress iAddr;
	TUint16 iPort;
	TPfkeyAddress() {iExt = 0; }
	TUint Length() const
		{return iExt ? ((sizeof(*iExt) + sizeof(TInetAddr) + 7) / 8) * 8 : 0; }
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	TInt LoadFromStream(const TInt aLength, const TUint8 *aPtr, REndPoints &aEp);
	TInt BindToEndPoint(TPfkeyIdentity &aIdentity, REndPoints &aEp);
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};

class TPfkeyKey
	{
public:
	const struct sadb_key *iExt;
	TPtrC8 iData;
	TPfkeyKey() {iExt = 0;}
	TUint Length() const
		{return iExt ? ((sizeof(*iExt) + iData.Length() + 7) / 8) * 8 : 0; }
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};


class TPfkeySensitivity
	{
public:
	const struct sadb_sens *iExt;
	TPtrC8 iSensBitmap;
	TPtrC8 iIntegBitmap;
	TPfkeySensitivity() {iExt = 0;}
	TUint Length() const
		{return iExt ? ((sizeof(*iExt) + iSensBitmap.Length() + iIntegBitmap.Length() + 7) / 8) * 8 : 0; }
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};

class TPfkeyProposal
	{
public:
	const struct sadb_prop *iExt;
	const struct sadb_comb *iComb;
	TInt iNumComb;
	TPfkeyProposal() {iExt = 0; iComb = 0; iNumComb = 0;}
	TUint Length() const
		{return iExt ? ((sizeof(*iExt) + iNumComb * sizeof(*iComb) + 7) / 8) * 8 : 0;}
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel, const CAlgorithmList *aAlgorithms) const;		// Only available in DEBUG mode
	};

class TPfkeySupported
	{
public:
	const struct sadb_supported *iExt;
	const struct sadb_alg *iAlg;
	TInt iNumAlg;
	TPfkeySupported() {iExt = 0; iAlg = 0; iNumAlg = 0;}
	TUint Length() const
		{return iExt ? ((sizeof(*iExt) + iNumAlg * sizeof(*iAlg) + 7) / 8) * 8 : 0;}
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void Init(struct sadb_supported *aExt, TInt aNumAlg, struct sadb_alg *aAlg);
	void LogPrint(const TDesC &aLabel, const CAlgorithmList *aAlgorithms) const;		// Only available in DEBUG mode
	};

class TPfkeySpirange
	{
public:
	const struct sadb_spirange *iExt;
	TPfkeySpirange() {iExt = 0;}
	TUint Length() const
		{return iExt  ? sizeof(*iExt) : 0;}
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};

class RTrafficSelectorSet;
class TPfkeyTs
	{
public:
	const struct sadb_x_ts *iExt;
	TPfkeyTs() {iExt = 0; iTS = NULL;}
	const RTrafficSelectorSet *iTS;
	TUint Length() const
		{
		return (iExt && iTS) ? ((sizeof(*iExt) + iExt->sadb_x_ts_numsel * sizeof(T_sadb_selector) + 7) / 8) * 8 : 0;
		}
	const T_sadb_selector &Selector(TInt aIndex) const
		{
		return ((T_sadb_selector *)((TUint8 *)iExt + sizeof(*iExt)))[aIndex];
		}
	TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
	void LogPrint(const TDesC &aLabel) const;		// Only available in DEBUG mode
	};

class TPFkeyPrivExt
	{
public:
	const struct sadb_gen_ext *iExt;
	TPtrC8 iData;
	TPFkeyPrivExt() {iExt = 0;}
	};

//
// Internal presentation of the PF_KEY message
//
class TPfkeyMessage
	{
public:
	// Length64()
	//	Returns the length of the stream representation
	//	of this message in 8 byte blocks (e.g. multiply
	//  this by 8 to get the length in octets.
	TUint16 Length64() const;

	// ByteStream(aPacket)
	//	Append a byte stream presentation of this message into
	//	a RMBufChain (aPacket)
	void ByteStreamL(RMBufChain &aPacket) const;
	//
	//
	// Construct internal presentation from the PFKEY bytestream message
	TPfkeyMessage(const TDesC8& aMsg, REndPoints &aEp);
	TPfkeyMessage() {}
	void LogPrint(const TDesC &aLabel, const CAlgorithmList *aAlgorithms) const;		// Only available in DEBUG mode

	TInt iError;	// == KErrNone, if message format valid.
	TPfkeyBase iBase;
	TPfkeyAssociation iSa;
	TPfkeyLifetime iCurrent;
	TPfkeyLifetime iHard;
	TPfkeyLifetime iSoft;
	TPfkeyAddress iSrcAddr;
	TPfkeyAddress iDstAddr;
	TPfkeyAddress iProxyAddr;
	TPfkeyKey iAuthKey;
	TPfkeyKey iEncryptKey;
	TPfkeyIdentity iSrcIdent;
	TPfkeyIdentity iDstIdent;
	TPfkeyIdentity iSrcEndpoint;
	TPfkeyIdentity iDstEndpoint;
	TPfkeySensitivity iSensitivity;
	TPfkeyProposal iProposal;
	TPfkeySupported iAuthAlgs;
	TPfkeySupported iEncryptAlgs;
	TPfkeySpirange iSpirange;
	TPfkeyTs iTs;

	TPFkeyPrivExt  iPrivateExtension;  // For ESP UDP encapsulation     
	};
#endif
