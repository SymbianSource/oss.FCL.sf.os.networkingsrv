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
// pfkey.h - IPv6/IPv4 IPSEC KEY protocol family
//



/**
 @internalComponent
*/
#ifndef __PFKEY_H__
#define __PFKEY_H__

#include <es_prot.h>
#include "ipsec.h"
#include "sadb.h"
#include "epdb.h"

#include "ah_eng.h"
#include "esp_eng.h"
#include "ipip_eng.h"
#include "natt_eng.h"

class CProviderKey;

//
//  CProtocolKey
//
class CProtocolKey : public CProtocolBase, public MAssociationManager
	/**
	* The PFKEY protocol implementation.
	*
	* PFKEY protocol manages the PFKEY sockets and maintains the Security
	* Association Database (SAD).
	*
	* PFKEY protocol provides MAssociationManger API, which defines the
	* services for the SECPOL protocol and SAD.
	*/
	{
public:
	CProtocolKey();
	CProtocolKey& operator=(const CProtocolKey&);
	virtual ~CProtocolKey();
	virtual CServProviderBase *NewSAPL(TUint aProtocol);
	virtual void InitL(TDesC& aTag);
	virtual void StartL();
	virtual void BindToL(CProtocolBase *protocol);
	virtual void BindL(CProtocolBase *aProtocol, TUint id);
	virtual void Identify(TServerProtocolDesc *) const;

	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8& aOption, CProtocolBase* aSourceProtocol=NULL);
	virtual TInt SetOption(TUint aLevel, TUint aName,const TDesC8& aOption, CProtocolBase* aSourceProtocol=NULL);
		
	virtual void Open();
	virtual void Close();

	// Methods to be used from the SECPOL
	TInt Acquire(
		CSecurityAssoc * &aSA,
		const TSecurityAssocSpec &aSpec,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT		
		const CPropList *aPropList, 
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
		const CTransportSelector *aTS,
		const RIpAddress &aSrc,
		const RIpAddress &aDst,
		const RPolicySelectorInfo &aInfo,
		TBool aTunnel);

#ifdef	SYMBIAN_IPSEC_VOIP_SUPPORT	
		TInt Verify(	const CSecurityAssoc *aSA,	const TSecurityAssocSpec &aSpec,
		const CPropList *aPropList,
		const RIpAddress &aSrc,	const RIpAddress &aDst,	const RPolicySelectorInfo &aInfo);
#else
		TInt Verify(
		const CSecurityAssoc *aSA,
		const TSecurityAssocSpec &aSpec,
		const RIpAddress &aSrc,
		const RIpAddress &aDst,
		const RPolicySelectorInfo &aInfo);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
				
	TInt ApplyL(	// Outbound
		CSecurityAssoc *aSa,
		RMBufSendPacket &aPacket,
		RMBufSendInfo &info,
		const TIpAddress &aTunnel);
	TInt ApplyL(	// Inbound
		CSecurityAssoc * &aSa,
		RMBufRecvPacket &aPacket,
		RMBufRecvInfo &info,
		TInt aProtocol,
		TIpAddress &aTunnel);
	TInt Overhead(const CSecurityAssoc *const aSa, const TIpAddress &aTunnel) const;
	void SetAlgorithms(CAlgorithmList*& aList);

	// Methods to be used from the CSecurityAssoc
	inline void TimerOn(CSecurityAssoc &aSa, TInt aDelta);
	void Expired(const CSecurityAssoc &aSa, TInt aType, const TLifetime &aLifetime);

	// Generic
	void Delete(CSecurityAssoc *aSa);
	CSecurityAssoc *Lookup(TUint8 aType, TUint32 aSPI, const TIpAddress &aDst) const;
	REndPoints &EndPointCollection() { return iEndPointCollection; }

	// PFKEYv2 main entry (called by CProviderKey
	TInt Exec(const TDesC8 &aMsg, CProviderKey *aSrc = NULL);
private:
	virtual CSecurityAssoc *Lookup(TUint8 aType, TUint32 aSPI, const TIpAddress &aDst, TInt &aHash) const;

	static CSecurityAssoc *FindEgg(CSecurityAssoc *sa,  const TPfkeyMessage &aMsg, const struct sadb_msg &aBase);

	// These ExecNNN methods are only used in key_msg.cpp (not intended for general use)
	TInt ExecGetSPI(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecUpdate(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecAdd(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecDelete(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc, TBool deliverMsg=ETrue);
	TInt ExecGet(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecAcquire(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecRegister(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecFlush(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	TInt ExecDump(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc);
	void DumpSA(TPfkeyMessage &aKey, struct sadb_msg &aBase, CProviderKey *aDst, CSecurityAssoc *sa);
	//
	void Deliver(const TPfkeyMessage &aMsg);
	void DeliverRegistered(const TPfkeyMessage &aMsg);

	inline TInt HashSize() const;
	inline TInt Hash(const TIp6Addr &addr, TUint8 type) const;

	/**
	* The Security Association DataBase (SAD).
	*
	* The Collection of Security Associations
	* hashed by *remote address*. The size of the
	* hash is automatically controlled by the size
	* of this array. Feel free to place any other
	* magic constant (prime!) here, the code will adjust)
	*/
	CSecurityAssoc *iHash[111];

	MTimeoutManager *iTimer;		//< Timing services.
	TUint32 iSequenceNumber;		//< Current sequence number for the kernel originated PF_KEY msgs
	TDblQue<CProviderKey> iSAPlist;	//< Housekeeping of attached sockets (= SAP's)
	TIpsecAH iEngineAH;				//< IPsec Authentication engine
	TIpsecESP iEngineESP;			//< IPsec Encryption engine
	TIpsecIPIP iEngineIPIP;			//< IPsec IP-in-IP tunneling engine
	TIpsecNATT iEngineNATT;			//< IPsec NAT Traversal engine  
	CIpsecCryptoManager *iCrypto;	//< IPsec Crypto Library Manager
	REndPoints iEndPointCollection;	//< The named end point collection.
	RArray<RIpAddress> iEndPoints;	//< The SetOpt EP definitions
	RMBufAllocator iRMBufAllocator;   //< RMBufAllocator used in encryption/decryption operation
	};
	


void CProtocolKey::TimerOn(CSecurityAssoc &aSa, TInt aDelta)
	/**
	* Activate a timeout call on SA.
	*
	* @param aSa The security association
	* @param aDelta The delay in seconds.
	*/
	{
	iTimer->Set(aSa.iTimeout, aDelta);
	}

TInt CProtocolKey::HashSize() const
	/**
	* Return number of entries in the hash array.
	*/
	{
	return sizeof(iHash) / sizeof(iHash[0]);
	}
	
TInt CProtocolKey::Hash(const TIp6Addr &addr, TUint8 type) const
	/**
	* Compute hash value from IPv6 address and assocation type.
	*
	* Hash computes the hash value from IPv6 address (128 bits) and
	* association type code, return a pointer to the list of Security
	* Associations, which include all associations with this remote host
	* (the list may include associations with other hosts that map to
	* the same hash value!)
	*
	* @param addr The IPv6 address
	* @param type The association type (AH or ESP)
	* @return Index into hash table (iHash).
	*/
	{
	const TUint32 tmp =
				addr.u.iAddr32[0] ^
				addr.u.iAddr32[1] ^
				addr.u.iAddr32[2] ^
				addr.u.iAddr32[3];
	return ((tmp >> 16) ^ tmp ^ type) % HashSize();
	}


//
// PF_KEY Socket Provider Base
//

/**
* Max value for Security Association type.
*
* The maximum allowed value (for this implementation) for Security
* association type (AH, ESP, etc.) value. The pfkey2.h value
* SADB_SATYPE_MAX is not used. This allows compiled code to work even
* if some future PFKEY adds more types. The limit is now taken from
* the size of the sadb_msg_satype field (= uint8_t). Only needed in
* building a bitmap for a REGISTERED listener.
*/
const TUint KProviderKey_SATYPE_MAX = 255;

class CProviderKey: public CProviderIpsecBase
	/**
	* The PFKEY socket provider (SAP).
	*/
	{
public:
	CProviderKey(CProtocolKey& aProtocol);
	void Start();
	TUint Write(const TDesC8 &aDesc,TUint options, TSockAddr* aAddr=NULL);

	TInt SetOption(TUint level,TUint name, const TDesC8 &anOption);
	TInt GetOption(TUint level,TUint name,TDes8 &anOption) const;
public:
	// For CProtocolKey only
	void Deliver(const TPfkeyMessage &aMsg);
	TUint8 iRegistered[(KProviderKey_SATYPE_MAX+7)/8];	//< Registered to listen protocols (now only AH and ESP).
protected:
	CProtocolKey& iProtocol;		//< PFKEY protocol object
	};

#endif
