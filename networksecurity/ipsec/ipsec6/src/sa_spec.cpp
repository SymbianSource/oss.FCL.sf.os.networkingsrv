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
// sa_spec.cpp - IPv6/IPv4 IPSEC security associations
//

#include "sa_spec.h"
#include "sadb.h"


TInt RSecurityAssociation::Status() const
	/**
	* Return state of Security assocation.
	*
	* If handle does not have an attached SA, the status is SADB_SASTATE_DEAD.
	*
	* @return The state 
	*/
	{
	return iAssociation ? iAssociation->State() : SADB_SASTATE_DEAD;
	}

void RSecurityAssociation::Reset(CSecurityAssoc *aSa)
	/**
	* Set new SA to the handle.
	*
	* Detach previous SA, if any present.
	*
	* @param aSa The new SA (or NULL)
	*/
	{
	if (iAssociation != aSa)
		{
		Detach();
		iAssociation = aSa;
		if (aSa)
			aSa->Attach(*this);
		}
	}

void RSecurityAssociation::Init(SecurityAssociationCallback aCallback)
	/**
	* Initialize handle.
	*
	* @param aCallback The callback function.
	*/
	{
	iPrev = this;
	iNext = this;
	iAssociation = NULL;
	iCallback = aCallback;
	}

void RSecurityAssociation::None()
	/**
	* Detach handle from SA.
	*/
	{
	Detach();
	iAssociation = NULL;
	}

void CIpsecReferenceCountObject::Close()
	/**
	* Decrement reference count and destroy object after last reference.
	*/
	{
	if (--iRefs < 0)
		delete this;
	}

CTransportSelector::CTransportSelector(const RPolicySelectorInfo &aData, const RPolicySelectorInfo &aMask, CTransportSelector *const aOr)
	: iData(aData), iMask(aMask), iOr(aOr)
	/**
	* Contructor for the basic Transport Selector.
	*
	* Construct the basic selector with initial reference. If the next
	* alternate selecter is present, it's reference count is also
	* incremented.
	*
	* @param aData	The selector data (the values to check)
	* @param aMask	The selector mask (which values to check)
	* @param aOr	The next alternate selector
	*/
	{
	if (iOr)
		iOr->Open();
	}

CTransportSelector::~CTransportSelector()
	/**
	* Destructor for the basic Transport Selector.
	*
	* Closes the reference to alternate selector, if
	* present.
	*/
	{
	if (iOr)
		iOr->Close();
	}
	
TInt CTransportSelector::Match(const RPolicySelectorInfo &aKey) const
	/**
	* Match the packet data against a Transport Selector.
	*
	* The Match returns true, if this or any of the alternate
	* selectors match the packet data.
	*
	* @param aKey The packet data to match against.
	* @return Result: 0 = no match, 1 = match.
	*/
	{
	TUint32 const *const key = (TUint32 *)&aKey;
	const CTransportSelector *ts = this;
	do
		{
		TUint32 const *const ptr = (TUint32 *)&ts->iData;
		TUint32 const *const msk = (TUint32 *)&ts->iMask;
		if (
			((key[0] ^ ptr[0]) & msk[0]) == 0 &&
			((key[1] ^ ptr[1]) & msk[1]) == 0 &&
			aKey.iRemote().IsEqMask(ts->iData.iRemote(), ts->iMask.iRemote()) &&
			aKey.iLocal().IsEqMask(ts->iData.iLocal(), ts->iMask.iLocal()))
			return 1;	// A selector matched!
		} while ((ts = ts->iOr) != NULL);

	// None of the basic selectors matched.
	return 0;
	}

CIdentity *CIdentity::NewL(const TDesC8 &aIdentity, TUint16 aType)
	/**
	* Create a new identity object.
	*
	* @param aIdentity The identity string.
	* @param aType The identity type
	*
	* @return The Identity object.
	*/
	{
	const TUint extra = aIdentity.Length();
	CIdentity *ident = new (extra) CIdentity(extra);
	if (ident == NULL)
		User::Leave(KErrNoMemory);
	ident->iType = aType;
	// See _LIT and TLitC for this initializer!!!
	TPtr8(&((TLitC8<1> &)ident->iTypeLength).iBuf[0], extra).Copy(aIdentity);
	return ident;
	}

CIdentity *CIdentity::NewL(const TDesC &aIdentity)
	/**
	* Create a new identity object.
	*
	* Based on content of the identity string, makes a rough
	* guess about the identity type:
	*
	* @param aIdentify The identity string.
	* @return The Identity object.
	*/
	{
	const TUint extra = aIdentity.Length();
 	CIdentity *ident = new (extra) CIdentity(extra);
	if (ident == NULL)
		User::Leave(KErrNoMemory);
	// See _LIT and TLitC for this initializer!!!
	TPtr8(&((TLitC8<1> &)ident->iTypeLength).iBuf[0], extra).Copy(aIdentity);

	// Just make an ad-hoc guess about the type of identity
	// (in real life, the syntax of policy file should indicate
	// the type instead). These guesses are bound to stab you in
	// back someday...
	TInetAddr tmp;		// ..just for address testing..
	if (aIdentity.Locate('@') >= 0)
		ident->iType = SADB_IDENTTYPE_USERFQDN;
	else if (aIdentity.Locate('/') >= 0 || tmp.Input(aIdentity) == KErrNone)
		// Use prefix for true prefix notation *and* for a plain IP address.
		ident->iType = SADB_IDENTTYPE_PREFIX;
	else
		ident->iType = SADB_IDENTTYPE_FQDN;
	return ident;
	}


CAlgorithmList::CAlgorithmList()
	: CArrayFixFlat<TAlgorithmMap>(10)
	/**
	* Constructor.
	*/
	{
	}

TAlgorithmMap::TAlgorithmMap(TAlgorithmClass aClass, TInt anId, TInt aBits, const TDesC &aLibrary, const TDesC &anAlgorithm)
	: iClass(aClass), iId(anId), iBits(aBits), iLibrary(aLibrary), iAlgorithm(anAlgorithm)
	/**
	* Constructor.
	*
	* @param aClass The class (cipher or digest)
	* @param anId	The algorithm number
	* @param aBits	The number of bits (for digest)
	* @param aLibrary The library name (or null string)
	* @param anAlgorithm The algorithm name (or null string)
	*/
	{
	}

void CAlgorithmList::AddL(TAlgorithmClass aClass, TInt anId, TInt aBits, const TDesC &aLibrary, const TDesC &anAlgorithm)
	/**
	* Add or redefine an algorithm mapping.
	*
	* First, try to find a matching entry by algorithm class and number.
	*
	* If found, then replace TAlgorithmMap content with the new mapping.
	*
	* If not found, then create a new mapping and add it to the algorithm list.
	*
	* @param aClass The algorithm class (Cipher, Digest, ...)
	* @param anId The algorithm number
	* @param aBits Number of bits (for digest only)
	* @param aLibrary The library name
	* @param anAlgorithm The algorithm name
	* @leave if adding new entry fails.
	*/
	{
	TAlgorithmMap map(aClass, anId, aBits, aLibrary, anAlgorithm);
	TAlgorithmMap *m = Lookup(aClass, anId);
	if (m)
		*m = map;
	else
		AppendL(map);
#if 0
	LOG(_LIT(KEncr, "ENCR"); _LIT(KAuth, "AUTH"););
	LOG(Log::Printf(_L("\tAlgorithmMap %S %d (bits=%d) maps to %S.%S"),
		map.iClass == EAlgorithmClass_Digest ? &KAuth() : &KEncr(),
		map.iId, map.iBits,
		&map.iLibrary, &map.iAlgorithm));
#endif
	}

TAlgorithmMap *CAlgorithmList::Lookup(TAlgorithmClass aClass, TInt anAlg) const
	/**
	* Lookup TAlgorithmMap by algorithm number.
	*
	* @param aClass The Algorithm class (Cipher or Digest)
	* @param anAlg	The Algorithm number.
	*
	* @return Algorithm map, or NULL if not found.
	*/
	{
	const TInt n = Count();
	for (TInt i = 0; i < n; i++)
		{
		const TAlgorithmMap& map = operator[](i);

		if (map.iClass == aClass && map.iId == anAlg)
			return (TAlgorithmMap *)&map;	// ...cast "const" away...
		}
	return NULL;
	}

TAlgorithmMap *CAlgorithmList::Lookup(const TDesC &aLibrary, const TDesC &anAlg) const
	/**
	* Lookup TAlgorithmMap by implementation name.
	*
	* Find TAlgorithmMap by name. If the library name is empty (Length() ==0),
	* search compares only algorithm name. Otherwise, both library and algorithm
	* must match.
	*
	* @param aLibrary The name of the crypto library
	* @param anAlg The Algorithm name
	*
	* @return Algorithm map, or NULL if not found.
	*/
	{
	const TInt n = Count();
	for (TInt i = 0; i < n; i++)
		{
		const TAlgorithmMap& map = operator[](i);

		if ((aLibrary.Length() == 0 || aLibrary == map.iLibrary) && anAlg == map.iAlgorithm)
			return (TAlgorithmMap *)&map;	// ...cast "const" away...
		}
	return NULL;
	}
