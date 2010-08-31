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
// spdb.h - IPSEC security policy database
// Security Policy Database
//



/**
 @file spdb.h
 @internalComponent
*/
#ifndef __SPDB_H__
#define __SPDB_H__

#include <e32std.h>
#include <networking/crypto.h>	// only for TAlgorithmClass, is this really necessary?
#include "sa_spec.h"
#include "ipaddress.h"

class REndPoints;
class RIpAddress;


#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
 class CSecurityProposalSpec  : public CBase
    {
    public:
    	TUint8 iType;
    	TUint8 iAalg;
    	TUint16 iAalgLen;
    	TUint8 iEalg;	
    	TUint16 iEalgLen;	
    	TUint iLarvalLifetime;	        
    	struct sadb_lifetime iHard;	
    	struct sadb_lifetime iSoft;  	
	// Limits for key lengths (for ACQUIRE only)
	TUint16 iMinAuthBits, iMaxAuthBits;		//< Required length of the authentication key
	TUint16 iMinEncryptBits, iMaxEncryptBits;	//< Required length of the encryption key    	
    };
    
    
//
// class CPropList
//
//
class CPropList : public CArrayFixFlat<CSecurityProposalSpec *>
    {
    public:
         static CPropList* NewL(TInt aGranularity);

         void Construct(TInt aGranularity);

         static CPropList* NewL(CPropList *aSAList);

         void ConstructL(CPropList *aSAList);
    private:

        CPropList(TInt aGranularity);

        CPropList(CPropList *aSAList);
    };
#endif SYMBIAN_IPSEC_VOIP_SUPPORT




class CPolicySpec : public CIpsecReferenceCountObject
	/**
	* Security association template.
	*
	* Specify requirements for a Security Association.
	*
	* Each policy syntax construct "sa name = { parameters }" creates
	* an instance of this.
	*/
	{
	~CPolicySpec();				// private!
public:
    CPolicySpec();

	TSecurityAssocSpec iSpec;		//< The parameters of SA template
	HBufC *iName;					//< The name of the SA template
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	CPropList *iPropList;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
	};

class CPolicyAction : public CIpsecReferenceCountObject
	/**
	* Policy action description.
	*
	* Specify single IPsec action/transformation to do.
	*
	* Each policy syntax element the actions part of the
	* "selector = { actions }" policy syntax generates
	* one instance from this class.
	*/
	{
public:
	~CPolicyAction();
	CPolicySpec *iSpec;			//< Security Association specification
 	CTransportSelector *iTS;	//< The selector applicapble to this action.
 	TUint iOptional:1;			//< ==1, if this item is optional (experimental "feature")
	TUint iIsTunnel:1;			//< ==1, if tunnel address is was set (even if set as "::")
	RIpAddress iTunnel;			//< Use SA in tunnel mode (if specified non-zero)
	};

typedef class RArray<CPolicyAction *> RPolicyActions;

class CSelectorInterface : public CBase
	/**
	* Hold interface information.
	*
	* Maintain mapping between interface name and index.
	*
	* Each policy syntax that uses "if name" selector references
	* an instance of from this class. Only one instance per interface
	* name is used.
	*/
	{
public:
	CSelectorInterface();
	~CSelectorInterface();
	CSelectorInterface *iNext;	//< Next Inteface
	TUint32 iInterfaceIndex;	//< Real Interface Index, loaded at policy load time
	HBufC *iName;				//< Interface Name
	};

class CPolicySelector : public CBase
	/**
	* Selector and Action definition.
	*
	* The IPsec policy consists of an ordered sequence of instances of this
	* object. Each is describes actions to be done, if the selector part
	* matches.
	*/
	{
public:
	CPolicySelector();
	~CPolicySelector();
	CPolicySelector *iNext;				//< The next selector.
	const CSelectorInterface *iInterface;//< Limited to specific interface, if non-NULL
	TUint32 iFilterMask;				//< The filter definition.
	TUint32 iFilterData;				//< The filter definition.
	CTransportSelector *iTS;			//< The transport selectors.
	RPolicyActions iActions;			//< The actions, when selector matches
	};

class CSecurityPolicy : public CBase
	/**
	* Security Policy Database (SPD).
	*
	* The CSecurityPolicy class is a representation of a Security Policy Database (SPD).
	* Each call to static SetPolicy function creates a new SPD from the text string
	* which describes the policy.
	*/
	{
	friend class CProtocolSecpol;
	friend class TParser;
	//
	// Construct an empty policy object
	//
	CSecurityPolicy();
	~CSecurityPolicy();
public:
	static TInt SetPolicy(CSecurityPolicy * &aPolicy, const TDesC &aNewPolicy, TUint &aOffset, REndPoints &aEp);

	//
	// The following methods are really intended to be used only
	// by the Policy parser, not for generic consumption!
	CPolicySpec *FindSpec(const TDesC &aName) const;

	inline TAlgorithmMap *FindAlg(TAlgorithmClass aClass, TInt anAlg) const;
	inline TAlgorithmMap *FindAlg(const TDesC &aLib, const TDesC &anAlg) const;
	inline TAlgorithmMap *NewAlgL(const TDesC &aLib, const TDesC &anAlg) const;
	const CSelectorInterface *LookupInterfaceL(const TDesC &aName);

	CPolicySelector *iSelectors;			//< Policy selectors to match
private:
	RArray<CPolicySpec *> iSpecs;			//< SA specifications
	RArray<RIpAddress> iEndPoints;			//< EP definitions

	CAlgorithmList *iAlgorithms;			//< Algorithm Mapping
	CSelectorInterface *iInterfaces;		//< Interface Names
	};


TAlgorithmMap *CSecurityPolicy::FindAlg(TAlgorithmClass aClass, TInt anAlg) const
	/**
	* Find algorithm by class and number.
	*
	* @param aClass The class (digest or cipher)
	* @param anAlg The number
	* @return Algorithm mapping or NULL.
	*/
	{
	return iAlgorithms->Lookup(aClass, anAlg);
	}

TAlgorithmMap *CSecurityPolicy::FindAlg(const TDesC &aLib, const TDesC &anAlg) const
	/**
	* Find algorithm by name.
	* @param aLib The libary name (or empty)
	* @param anAlg The algorithm name
	* @return Algorithm mapping or NULL.
	*/
	{
	return iAlgorithms->Lookup(aLib, anAlg);
	}

TAlgorithmMap *CSecurityPolicy::NewAlgL(const TDesC &aLib, const TDesC &anAlg) const
	/**
	* Create new algorithm entry.
	*
	* @param aLib The library name
	* @param anAlg The algorithm name
	* @return Algorithm mapping or NULL.
	* @leave if allocation fails.
	*/
	{
	iAlgorithms->AddL(EAlgorithmClass_Cipher, 0, 0, aLib, anAlg);
	return iAlgorithms->Lookup(aLib, anAlg);
	}

#endif
