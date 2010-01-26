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
// key_msg.cpp - IPv6/IPv4 IPSEC PFKEY messages
// This module contains the main entry points for the
// implementations of the PFKEY messages, currently:
// SADB_GETSPI		CProtocolKey::ExecGetSPI
// SADB_UPDATE		CProtocolKey::ExecUpdate
// SADB_ADD		CProtocolKey::ExecAdd
// SADB_DELETE		CProtocolKey::ExecDelete
// SADB_GET		CProtocolKey::ExecGet
// SADB_ACQUIRE	CProtocolKey::ExecAcquire
// SADB_REGISTER	CProtocolKey::ExecRegister
// SADB_EXPIRE		(only sent to clients by IPsec)
// SADB_FLUSH		CProtocolKey::ExecFlush
// SADB_DUMP		CProtocolKey::ExecDump
// This module belongs to CProtocolKey implementation
// Common to all methods
// Return
// the request was syntactically correct and the
// ultimate success or failur can be checked from
// the reply PFKEY message (to all interested
// listeners).
// the request has some fatal flaws (programming
// errors). No PFKEY reply message is generated and
// the issuing socket write should indicate a failed
// operation to the application.
// The Base sadb_msg_errno field of the reply PFKEY message
// contains the ultimate result of the operation, again using
// the EPOC32 error codes. However, as the field is declared
// as "unsigned char", a positive value is stored in every
// case, that is for example "-KErrAlreadyExists". That is,
// this implementation makes a hidden assumption that KErrNone
// is ZERO, and all other errors that get used have a negative
// value! (the reserved field is now used to store extra 16
// bits of the error).
// Each ExecNNN method gets the same three parameters, even
// though not all need them all
// can be modified (a copy
// of the original and already
// linked to aMsg)
//



/**
 @file key_msg.cpp
 @code
 @endcode
 @li		KErrNone
 @li		!= KErrNone
 @li		TPfkeyMessage &aMsg		- the message to be acted
 @li		struct sadb_msg &aBase	- a working BASE part that
 @li		CProviderKey *aSrc		- the originating SAP
*/

#include "sadb.h"
#include "sa_crypt.h"
#include <networking/pfkeyv2.h>
#include "pfkey.h"
#include "pfkeymsg.h"

static void Update(struct sadb_msg &aBase, CSecurityAssoc &aSa, TPfkeyMessage &aMsg, MAssociationManager &aManager, CIpsecCryptoManager *aCrypto)
	/**
	* Update SA without leaving.
	*
	* Implement a local help function for the SA Update, which is not leaving.
	*
	* @param aBase	The writable copy of the base part of PFKEY message.
	* @param aMsg	The PFKEY message
	* @param aSa	The Security Association to update
	* @param aManager The manager of assocaitions
	* @param aCrypto The crypto library manager
	*/
	{
	TRAPD(ret, ret = aSa.UpdateL(aManager, aMsg, aCrypto));

	// PFKEY official error field is only 8 bits. Normal EPOC errors will usually
	// fit this space, when treated as small positive integers. However, IPSEC and
	// other components may return much bigger numbers. As suggested by Craig Metz,
	// the low order 8 bits of the error code are placed to the official field,
	// and the next 16 bits into the unused portion of the base structure. This
	// may give some breathing space, until the full 32 bit error can be fit into
	// the base structure...
	ret = -ret;				// Turn error number into positive number
	aBase.sadb_msg_errno = (TUint8)ret;
	aBase.sadb_msg_reserved = (TUint16)(ret >> 8);
	
	if (aMsg.iTs.iExt)
		aMsg.iTs.iTS = &aSa.iTS;

	}

CSecurityAssoc *CProtocolKey::FindEgg
	(
	CSecurityAssoc *sa,
	const TPfkeyMessage &aMsg,
	const struct sadb_msg &aBase
	)
	/**
	* Find the "Egg SA".
	*
	* A very local utility function to check if a iHash[i] chain
	* contains a matching "Egg" SA. This is not a general function,
	* ot has very specific preconditions (iDstAddr) etc, This is
	* more like inline macro, just coded to make code shorter.
	*
	* @param sa	The first sa to check
	* @param aMsg PFKEYv2 message to identify the Egg.
	* @param aBase The base part of the PFKEYv2 message.
	* @return NULL, if not found, and SA pointer, if found
	*/
	{
	const TIpAddress &dst = aMsg.iDstAddr.iAddr();

	for ( ; sa != NULL; sa = sa->iNext)
		if (sa->iSPI == 0 &&
			sa->iSendSeq == aBase.sadb_msg_seq &&
			sa->iType == aBase.sadb_msg_satype &&
			sa->iDst() == dst)
			break;	// Found!!
	return sa;
	}

void CProtocolKey::DumpSA
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aDst, CSecurityAssoc *sa)
	/**
	* Dump SA information into requesting socket.
	*
	* Build PFKEY message from the specified SA and deliver (CProtocolKey::Deliver())
	* it to the requesting socket (SAP). The keying information is never returned
	* this way.
	*
	* @param aMsg The PFKEY message to use
	* @param aBase The base part of the message.
	* @param aDst The requesting socket
	* @param sa The Security Assocication.
	*/
	{
	T_sadb_sa xSa
		(
		sa->iSPI,
		sa->iReplayCheck ? KMaxReplayWindowLength : 0,
		sa->iState,
		sa->iAalg,
		sa->iEalg,
		sa->iFlags);
	aMsg.iSa.iExt = &xSa;

	T_sadb_lifetime xCurrent(sa->iCurrent);
	aMsg.iCurrent.iExt = &xCurrent;

	T_sadb_lifetime xHard(SADB_EXT_LIFETIME_HARD, sa->iHard, sa->iCurrent);
	aMsg.iHard.iExt = &xHard;

	T_sadb_lifetime xSoft(SADB_EXT_LIFETIME_SOFT, sa->iSoft, sa->iCurrent);
	aMsg.iSoft.iExt = &xSoft;

	T_sadb_address xDst(SADB_EXT_ADDRESS_DST, sa->iInfo.iProtocol);
	aMsg.iDstAddr.iExt = &xDst;
	aMsg.iDstAddr.iAddr = sa->iDst;
	aMsg.iDstAddr.iPort = sa->iInfo.iPortDst;

	T_sadb_address xSrc(SADB_EXT_ADDRESS_SRC, sa->iInfo.iProtocol);
	if (!sa->iSrc().IsNone())
		{
		aMsg.iSrcAddr.iExt = &xSrc;
		aMsg.iSrcAddr.iAddr = sa->iSrc;
		aMsg.iSrcAddr.iPort = sa->iInfo.iPortSrc;
		}

	TIpAddress xProxyAddr(KInet6AddrNone, sa->TunnelIndex());

	T_sadb_address xProxy(SADB_EXT_ADDRESS_PROXY, sa->iInfo.iProtocol);
	if (!sa->iInfo.iSrc().IsNone())
		{
		aMsg.iProxyAddr.iExt = &xProxy;
		aMsg.iProxyAddr.iAddr = sa->iInfo.iSrc;
		}
	else if (xProxyAddr.iScope)
		{
		aMsg.iProxyAddr.iExt = &xProxy;
		aMsg.iProxyAddr.iAddr.Open(iEndPointCollection, xProxyAddr);
		}

	T_sadb_ident xSrcId(SADB_EXT_IDENTITY_SRC);
	if (sa->iInfo.iSrcIdentity)
		{
		// Build Source Identity Extension for the PFKEY
		aMsg.iSrcIdent.iExt = &xSrcId;
		aMsg.iSrcIdent.iData.Set(sa->iInfo.iSrcIdentity->Identity());
		xSrcId = T_sadb_ident(SADB_EXT_IDENTITY_SRC, aMsg.iSrcIdent.iData.Length());
		xSrcId.sadb_ident_type = sa->iInfo.iSrcIdentity->Type();
		}

	T_sadb_ident xDstId(SADB_EXT_IDENTITY_DST);
	if (sa->iInfo.iDstIdentity)
		{
		// Build Destination Identity Extension for the PFKEY
		aMsg.iDstIdent.iExt = &xDstId;
		aMsg.iDstIdent.iData.Set(sa->iInfo.iDstIdentity->Identity());
		xDstId = T_sadb_ident(SADB_EXT_IDENTITY_DST, aMsg.iDstIdent.iData.Length());
		xDstId.sadb_ident_type = sa->iInfo.iDstIdentity->Type();
		}
	
	T_sadb_ident xSrcEp(SADB_X_EXT_ENDPOINT_SRC);
	if (sa->iSrc.IsNamed())
		{
		// Build Source End Point Extension for the PFKEY
		aMsg.iSrcEndpoint.iExt = &xSrcEp;
		aMsg.iSrcEndpoint.iData.Set(sa->iSrc.Name());
		xSrcEp = T_sadb_ident(SADB_X_EXT_ENDPOINT_SRC, aMsg.iSrcEndpoint.iData.Length());
		}

	T_sadb_ident xDstEp(SADB_X_EXT_ENDPOINT_DST);
	if (sa->iDst.IsNamed())
		{
		// Build Destination End Point Extension for the PFKEY
		aMsg.iDstEndpoint.iExt = &xDstEp;
		aMsg.iDstEndpoint.iData.Set(sa->iDst.Name());
		xDstEp = T_sadb_ident(SADB_X_EXT_ENDPOINT_DST, aMsg.iDstEndpoint.iData.Length());
		}

	T_sadb_ts xTs(sa->iTS.Count());
	if (xTs.sadb_x_ts_numsel)
		{
		aMsg.iTs.iExt = &xTs;
		aMsg.iTs.iTS = &sa->iTS;
		}

	aBase.sadb_msg_satype = sa->iType;
	aBase.sadb_msg_len = aMsg.Length64();
	aDst->Deliver(aMsg);
	LOG(aMsg.LogPrint(_L("---->"), iCrypto->Algorithms()));

	// Can't leave pointers hanging.. clear them
	aMsg.iSa.iExt = NULL;
	aMsg.iCurrent.iExt = NULL;
	aMsg.iHard.iExt = NULL;
	aMsg.iSoft.iExt = NULL;
	aMsg.iSrcAddr.iExt = NULL;
	aMsg.iSrcAddr.iAddr.Close();
	aMsg.iDstAddr.iExt = NULL;
	aMsg.iDstAddr.iAddr.Close();
	aMsg.iProxyAddr.iExt = NULL;
	aMsg.iProxyAddr.iAddr.Close();
	aMsg.iSrcIdent.iExt = NULL;
	aMsg.iDstIdent.iExt = NULL;
	aMsg.iSrcEndpoint.iExt = NULL;
	aMsg.iDstEndpoint.iExt = NULL;
	aMsg.iTs.iExt = NULL;
	}

TInt CProtocolKey::ExecGetSPI
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey * /*aSrc*/)
	/**
	* GetSPI -- allocate next free SPI number for incoming SA
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket
	*/
	{
	// base, address, SPI range
	if (aMsg.iDstAddr.iExt == NULL ||
		aMsg.iSpirange.iExt == NULL ||
		aMsg.iSpirange.iExt->sadb_spirange_max <
			aMsg.iSpirange.iExt->sadb_spirange_min)
		return KErrGeneral;

	const int i = Hash(aMsg.iDstAddr.iAddr(), aBase.sadb_msg_satype);

	TUint32 spi = aMsg.iSpirange.iExt->sadb_spirange_min;
	if (!spi)
		spi++;		// Do not allow a request for SPI=0, this
					// value is used internally to mark "eggs"
					// (SA's acting as ACQUIRE request queue)!


	// Locate an unused SPI value within the requested range
	T_sadb_sa xsa(ByteOrder::Swap32(spi));

	CSecurityAssoc *sa = iHash[i];
	while (sa)
		{
		if (sa->iType == aBase.sadb_msg_satype &&
			sa->iDst() == aMsg.iDstAddr.iAddr() &&
			sa->iSPI == xsa.sadb_sa_spi)
			{
			// The spi is already in use, try next one in range
			if (xsa.sadb_sa_spi < aMsg.iSpirange.iExt->sadb_spirange_max)
				{
				xsa.sadb_sa_spi = ByteOrder::Swap32(++spi);
				sa = iHash[i];	// Restart search (might consider ordering
								// this list by SPI value, perhaps?)
								// *Don't reorder, unless something is
								// also done to support the current
								// ACQUIRE, which uses the current ordering
								// as a "feature" (check KEY_SA.CPP Acquire!)
								// -- msa
				continue;
				}
			break;	// --> Already Exists error!
			}
		else
			sa = sa->iNext;
		}
	if (sa)
		// Requested SPI could not be allocated
		aBase.sadb_msg_errno = -KErrAlreadyExists;
	else
		{
		if (aBase.sadb_msg_pid == 0 &&
			(sa = FindEgg(iHash[i], aMsg, aBase)) != NULL)
			{
			// This GETSPI is a reply to earlier kernel originated ACQUIRE,
			// locate the matching larval (egg) SA from the SAD.
			// (With current implementation this should really not happen,
			// as the Egg is for outgoing direction, and assigning SPI to
			// it at this end is somewhat dubious...
			//
			// However, could this branch be taken when the dst is multicast,
			// localhost or some other own address? Thus, leave code in just
			// in case -- msa
			sa->iSPI = xsa.sadb_sa_spi;
			sa->iSendSeq = 0;
			}
		else
			{
			// Application just requested an SPI value, need to create a larval
			// SA to hold this assigned value.

			// A problem? if application doesn't do update within timelimit, the
			// SA is expired and spi may get allocated to someone else. How to
			// verify that this "old" application doesn't have access to it anymore?
			// Have a SAP stored in larval SA? And, allow UPDATE only from this SAP?
			// Delete larval SA's if the owning SAP dies?
			aMsg.iSa.iExt = &xsa;
			sa = new CSecurityAssoc(aMsg);
			if (sa)
				{
				sa->iNext = iHash[i];
				iHash[i] = sa;
				}
			else
				aBase.sadb_msg_errno = -KErrNoMemory;
			}
		//
		// Add Association Extension to the PFKEY
		// Reply Message (SA(*))
		//
		aMsg.iSa.iExt = &xsa;
		aBase.sadb_msg_len = aMsg.Length64();
		}
	//
	// Report GETSPI to all interested listeners
	//
	Deliver(aMsg);
	return sa ? sa->TimerInit(*this) : KErrNone;
	}


TInt CProtocolKey::ExecUpdate
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey * /*aSrc*/)
	/**
	* Update existing SA.
	*
	* Uses PFKEY extensions:
	* base, SA, (lifetime(HSC)), address(SD), (address(P)), key(AE),
	*		(identify(SD), (sensitivity)
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket
	*/
	{
	if (aMsg.iDstAddr.iExt == NULL ||
		aMsg.iSa.iExt == NULL)
		return KErrGeneral;

	TInt i;
	CSecurityAssoc *sa =
		Lookup(aBase.sadb_msg_satype, aMsg.iSa.iExt->sadb_sa_spi, aMsg.iDstAddr.iAddr(), i);
	if (sa)
		Update(aBase, *sa, aMsg, *this, iCrypto);
	else if (aBase.sadb_msg_pid == 0 &&
			 (sa = FindEgg(iHash[i], aMsg, aBase)) != NULL)
		{
		//
		// This ADD is a reply to earlier kernel originated ACQUIRE,
		// Change the state from larval egg to real LARVAL state
		sa->iSendSeq = 0;
		Update(aBase, *sa, aMsg, *this, iCrypto);
		}
	else
		aBase.sadb_msg_errno = -KErrNotFound;
	//
	// Reply all Listeners
	//
	aMsg.iAuthKey.iExt = NULL;
	aMsg.iEncryptKey.iExt = NULL;
	aBase.sadb_msg_len = aMsg.Length64();
	Deliver(aMsg);

	return sa ? sa->TimerInit(*this) : KErrNone;
	}

TInt CProtocolKey::ExecAdd
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey * /*aSrc*/)
	/**
	* Add mew SA.
	*
	* Uses PFKEY extensions:
	* base, SA, (lifetime(HS)), address(SD), (address(P),) key(AE),
	*		(identity(SD),) (sensitivity)
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket
	*/
	{

	if (aMsg.iDstAddr.iExt == NULL ||
		aMsg.iSa.iExt == NULL ||
		aMsg.iSa.iExt->sadb_sa_state != SADB_SASTATE_MATURE)
		return KErrGeneral;

	CSecurityAssoc *sa;
	TInt i;
	sa = Lookup(aBase.sadb_msg_satype, aMsg.iSa.iExt->sadb_sa_spi, aMsg.iDstAddr.iAddr(), i);
	if (sa)
		aBase.sadb_msg_errno = -KErrAlreadyExists;
	else if (aBase.sadb_msg_pid == 0 &&
			 (sa = FindEgg(iHash[i], aMsg, aBase)) != NULL)
		{
		// This ADD is a reply to earlier kernel originated ACQUIRE,
		// Change the state from larval egg to real LARVAL state
		sa->iSendSeq = 0;
		Update(aBase, *sa, aMsg, *this, iCrypto);
		}
	else
		{
		// No matching SA exist yet, can create a new
		sa = new CSecurityAssoc(aMsg);
		if (sa)
			{
			Update(aBase, *sa, aMsg, *this, iCrypto);
			if(aBase.sadb_msg_errno == 0 && aBase.sadb_msg_reserved == 0 )
				{
				// *NOTE*
				//	Adding always front may have some useful semantics, It will
				//	make outbound traffic to use newest matching SA (see ACQUIRE
				//	processing).
				
				sa->iNext = iHash[i];
				iHash[i] = sa;
				}
			else
				{
				delete (sa);
				sa = NULL;
				}
			}
		else
			aBase.sadb_msg_errno = -KErrNoMemory;
		}
	// Return the message to all listeners
	// (Keying information removed, which changes the length and
	// thus the sadb_msg_len must be recomputed. The byte length
	// *should* already be divisible by 8, no roundup coded!!!)
	aMsg.iAuthKey.iExt = NULL;
	aMsg.iEncryptKey.iExt = NULL;
	aBase.sadb_msg_len = aMsg.Length64();

	Deliver(aMsg);
	return sa ? sa->TimerInit(*this) : KErrNone;
	}

TInt CProtocolKey::ExecDelete
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey * /*aSrc*/, TBool deliverMsg)
	/**
	* Delete an existing SA.
	*
	* Uses PKEFY extensions:
	* base, SA(*), address(SD)
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket
	* @param deliverMsg Value to determine whether or not to deliver the PFKEY 
	*                                   message to the listeners (ETrue by default). 
	*/
	{
	if (aMsg.iDstAddr.iExt == NULL ||
		aMsg.iSa.iExt == NULL)
		return KErrGeneral;

	TInt i;	// not used
	CSecurityAssoc *sa;
	sa = Lookup(aBase.sadb_msg_satype, aMsg.iSa.iExt->sadb_sa_spi, aMsg.iDstAddr.iAddr(), i);
	if (sa)
		Delete(sa);
	else
		aBase.sadb_msg_errno = -KErrNotFound;
	if (deliverMsg)
		Deliver(aMsg);
	return KErrNone;
	}

TInt CProtocolKey::ExecGet
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc)
	/**
	* Get dump of specificic SA,
	*
	* Uses PKEFY extensions:
	* base, SA(*), address(SD)
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket (SAP).
	*/
	{
	if (aMsg.iDstAddr.iExt == NULL ||
		aMsg.iSa.iExt == NULL)
		return KErrGeneral;

	TInt i;
	CSecurityAssoc *sa;
	sa = Lookup(aBase.sadb_msg_satype, aMsg.iSa.iExt->sadb_sa_spi, aMsg.iDstAddr.iAddr(), i);
	if (sa)
		DumpSA(aMsg, aBase, aSrc, sa);
	else
		{
		aBase.sadb_msg_errno = -KErrNotFound;
		aSrc->Deliver(aMsg);
		LOG(aMsg.LogPrint(_L("---->"), iCrypto->Algorithms()));
		}
	return KErrNone;
	}

TInt CProtocolKey::ExecAcquire
	(TPfkeyMessage &aMsg, struct sadb_msg & aBase, CProviderKey* aSrc)
	/**
	* Acquire from the socket.
	*
	* Pass application originated ACQUIRE messages
	* to other application sockets as is.
	* Also used to handle ACQUIRE responses from listeners.
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket (SAP).
	*/
	{
	TInt errNo = -((aBase.sadb_msg_reserved <<8 ) + (aBase.sadb_msg_errno));
	TInt retValue = KErrNone;
	
	/* sa is created to set the error value in the iErrValue memeber variable,
	 * which will be used by the ReadErr() of RSecurityAssociation
	 */
	TInt iTemp;   
	CSecurityAssoc *sa;
        sa = Lookup(aBase.sadb_msg_satype, aMsg.iSa.iExt->sadb_sa_spi, aMsg.iDstAddr.iAddr(), iTemp);
        sa->SetErrorValue(errNo);
        
	if (errNo == KErrNone)
		{
		DeliverRegistered(aMsg);
		}
	else 
		{
		retValue = ExecDelete(aMsg, aBase, aSrc, EFalse);
		}
	return retValue;	
	}

TInt CProtocolKey::ExecRegister
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc)
	/**
	* Register application socket as a key manager.
	*
	* The return message reports the list of currently
	* supported algorithms.
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket (SAP).
	*/
	{
	TUint i = aBase.sadb_msg_satype;
	TUint8 m = (TUint8)(1 << (i % 8));
	i = i / 8;
	aSrc->iRegistered[i] |= m;

	// Prepare the reply message
	//
	// This returns all known algorithms, whether the register
	// was for AH, ESP or any unknown SA type.
	TInt num_auth = 0, num_encrypt = 0;

	// Supported algorithms returns an array of sadb_alg
	// descriptors, first num_auth are authentication, the
	// the last num_encrypt are encryption algorithms.
	// NOTE: There may be some unused/unfilled entries
	// in the middle!!!
	// (may return NULL, if both counts are zero). A NULL
	// return with non-zero count indicates out of memory
	// situation.
	CArrayFixFlat<struct sadb_alg> *algs =
		iCrypto->SupportedAlgorithms(num_auth, num_encrypt);
	if (algs == NULL && (num_auth || num_encrypt))
		return KErrNoMemory;

	T_sadb_supported auth(SADB_EXT_SUPPORTED_AUTH, num_auth);
	T_sadb_supported encrypt(SADB_EXT_SUPPORTED_ENCRYPT, num_encrypt);
	if (num_auth > 0)
		aMsg.iAuthAlgs.Init(&auth, num_auth, algs->Back(0));
	else
		aMsg.iAuthAlgs.iExt = NULL;
	if (num_encrypt > 0)
		aMsg.iEncryptAlgs.Init(&encrypt, num_encrypt, algs->End(0) - num_encrypt);
	else
		aMsg.iEncryptAlgs.iExt = NULL;
	aBase.sadb_msg_len = aMsg.Length64();
	DeliverRegistered(aMsg);
	delete algs;
	return KErrNone;
	}

TInt CProtocolKey::ExecFlush
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey * /*aSrc*/)
	/**
	* Release all security associations of matching the type.
	*
	* The type selections can be: AH, ESP or all.
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket (SAP).
	*/
	{
	for (TInt i = 0; i < HashSize(); ++i)
		{
		for (CSecurityAssoc *p, **pp = &iHash[i]; (p = *pp) != NULL;) 
			if (aBase.sadb_msg_satype == SADB_SATYPE_UNSPEC ||
				aBase.sadb_msg_satype == p->iType)
				{
				*pp = p->iNext;	// Removes SA from the list
				delete p;
				}
			else
				pp = &p->iNext;
		}
	Deliver(aMsg);
	return KErrNone;
	}

TInt CProtocolKey::ExecDump
	(TPfkeyMessage &aMsg, struct sadb_msg &aBase, CProviderKey *aSrc)
	/**
	* Dump all security associations of matching type.
	*
	* Generate one PFKEY message from each selected SA and
	* deliver messages to the requesting socket.
	*
	* The type selections can be: AH, ESP or all.
	*
	* @param aMsg The PFKEY message to process.
	* @param aBase Modifiable copy of the base extension.
	* @param aSrc The originating socket (SAP).
	*/
	{
	TUint8 type = aBase.sadb_msg_satype;

	for (TInt i = 0; i < HashSize(); ++i)
		{
		for (CSecurityAssoc *sa = iHash[i]; sa != NULL; sa = sa->iNext) 
			if (type == SADB_SATYPE_UNSPEC || type == sa->iType)
				{
				// Deliver SA as PFKEY message to aSrc
				// (need a local PFkeyMessage for this)
				//
				// Generate TPfkeyMsg from the SA
				// Seq#?
				DumpSA(aMsg, aBase, aSrc, sa);
				}
		}

	// Return the terminator Message
	aBase.sadb_msg_seq = 0;
	aBase.sadb_msg_pid = 0;
	aBase.sadb_msg_satype = type;
	aBase.sadb_msg_len = aMsg.Length64();
	aSrc->Deliver(aMsg);
	LOG(aMsg.LogPrint(_L("---->"), iCrypto->Algorithms()));
	return KErrNone;
	}


TInt CProtocolKey::Exec(const TDesC8 &aMsg, CProviderKey *aSrc)
	/**
	* Execute PFKEYv2 Messages from the socket interface.
	*
	* Called from CProviderKey::Write().
	*
	* This is the main function that calls the actual message handler
	* according to the message type:
	*
	* @li SADB_GETSPI
	*	ExecGetSPI	allocates a new SPI value from the specified SPI range.
	*	Echo the reply message to all listeners.
	* @li SADB_UPDATE
	*	ExecUpdate	updates a SA.
	*	Echo the reply message to all listeners.
	* @li SADB_ADD
	*	ExecAdd	adds a new SA.
	*	Echo the reply message all listeners.
	* @li SADB_DELETE
	*	ExecDelete	deletes the SA.
	*	Echo the reply message to all listeners.
	* @li SADB_GET
	*	ExecGet	returns the information about the specified SA.
	*	Return the reply only to the requesting socket (the key information is not included).
	* @li SADB_ACQUIRE
	*	ExecAcquire	passes application originated ACQUIRE to other listeners.
	*	Echo the message to all registered listeners. The IPSEC implementation makes no other
	*	processing for them.
	* @li SADB_REGISTER
	*	ExecRegister registers the source (aSrc) socket for receiving the specified
	*	ACQUIRE messages for the specified security association type.
	*	Echo the reply to all registered listeners.
	* @li SADB_FLUSH
	*	ExecFlush deletes all existing security associations from the SAD.
	*	Echo the message to all listeners (done after completed operation).
	* @li SADB_DUMP
	*	ExecDump return the description of every Security Association of the
	*	specified type to the requesting socket (aSrc). Return each association
	*	as a separate PFKEY message.
	*	The last message is an empty message with PID=0 and SEQ=0
	* @li SADB_EXPIRE is not a valid PFKEY request from the socket (it only generated
	*	by the IPsec itself).
	* @li Others
	*	Echo all other messages as is to all registered listeners. IPsec implementation
	*	does not do anything with them.
	*
	* @param aMsg	The PFKEYv2 message
	* @param aSrc	The originating socket.
	* @return KErrNone or error code on fail.
	*/
	{
	TIpAddress src, dst, proxy;		// Working space for the TPfkeyMessage Constructor!
	TPfkeyMessage msg(aMsg, iEndPointCollection);
	LOG(msg.LogPrint(_L("PFKEY"), iCrypto->Algorithms()));

	if (msg.iError != KErrNone)
		{                      
		// We have an exception for scenario involving SADB_ACQUIRE message with sadb_msg_errno != KErrNone. 
		// The TPfkeyMessage's constructor would have initialized the iError value as KErrArgument. 
		// But we need to permit futher processing. For all other scenarios we can return with error.
		if (!msg.iBase.iMsg ||
		msg.iBase.iMsg->sadb_msg_type != SADB_ACQUIRE ||
		msg.iBase.iMsg->sadb_msg_errno == KErrNone)
			{                                          
			return msg.iError;
			}
		}	
	
	if (msg.iBase.iMsg)		// (the BASE part must always be present)
		{
		// Many of the Exec functions need to modify the
		// message content before it is passed on to the
		// listeners. As the BASE part pointed by aMsg is
		// const, create a modifiable copy of it for the
		// Exec use (change msg to point this).
		struct sadb_msg base = *msg.iBase.iMsg;
		msg.iBase.iMsg = &base;
		
		switch (msg.iBase.iMsg->sadb_msg_type)
			{
		case SADB_GETSPI:
			return ExecGetSPI	(msg, base, aSrc);
		case SADB_UPDATE:
			return ExecUpdate	(msg, base, aSrc);
		case SADB_ADD:
			return ExecAdd		(msg, base, aSrc);
		case SADB_DELETE:
			return ExecDelete	(msg, base, aSrc);
		case SADB_GET:
			return ExecGet		(msg, base, aSrc);
		case SADB_ACQUIRE:
			return ExecAcquire	(msg, base, aSrc);
		case SADB_REGISTER:
			return ExecRegister	(msg, base, aSrc);
		case SADB_FLUSH:
			return ExecFlush	(msg, base, aSrc);
		case SADB_DUMP:
			return ExecDump		(msg, base, aSrc);
		default:
			DeliverRegistered(msg);
			return KErrNone;

		case SADB_RESERVED:		// RESERVED is not used!
		case SADB_EXPIRE:		// EXPIRE is not a valid from application!
			return KErrGeneral;	// Invalid message
			}
		}
	// Gets here only if the message is considered invalid at this
	// method (no ExecXXXX called). KErrGeneral is used in a sense
	// of EINVAL here.
	return KErrGeneral;
	}

