/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* This file contains the class definitions for IPsec security policy
*
*/



/**
 @file spdb.h
 @released
 @internalTechnology
*/

#ifndef _SPDB_H
#define _SPDB_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDE FILES
//////////////////////////////////////////////////////////////////////////////

#include <in_sock.h>
#include "sa_spec.h"

//////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
//////////////////////////////////////////////////////////////////////////////

class CPolicySelector;

//////////////////////////////////////////////////////////////////////////////
// ENUMERATIONS
//////////////////////////////////////////////////////////////////////////////

enum TPolicySpecType 
    { 
    EPolSpecSA,
    EPolSpecEP
    };

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

const TUint KPolicySelector_INBOUND     = 0x1;

const TUint KPolicySelector_OUTBOUND    = 0x2;

const TUint KPolicySelector_SYMMETRIC   = 
    (KPolicySelector_INBOUND | KPolicySelector_OUTBOUND);
    
const TUint KPolicySelector_INTERFACE   = 0x4;

//////////////////////////////////////////////////////////////////////////////
// CLASS DECLARATIONS
//////////////////////////////////////////////////////////////////////////////
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT  
//////////////////////////////////////////////////////////////////////////////
// class CPropList
//////////////////////////////////////////////////////////////////////////////
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
#endif    
//////////////////////////////////////////////////////////////////////////////
// class CPolicySpec
//  Specify requirements for a single Security Association. The same 
//  requirements can be shared by multiple policy bundles (policy actions).
//////////////////////////////////////////////////////////////////////////////
//
    
class CPolicySpec : public CBase
    {
    friend class CSecurityPolicy;

    public:
        HBufC8 *iName;

        // Actual storage of the Identity strings. If present, the 
        // TSecurityAssocSpec will only have constant reference to this !
        HBufC8 *iRemoteIdentity;
        HBufC8 *iLocalIdentity;
        
        IMPORT_C static CPolicySpec* NewL();

        IMPORT_C void Construct();

        IMPORT_C static CPolicySpec* NewL(TDesC &aName, 
                                          TPolicySpecType iSpectype = EPolSpecSA);

        IMPORT_C void ConstructL(TDesC &aName, 
                                 TPolicySpecType aSpectype = EPolSpecSA);

        // Used to initialize with an existing CPolicySpec
        IMPORT_C static CPolicySpec* NewL(CPolicySpec *aPolSpec);

        IMPORT_C void ConstructL(CPolicySpec *aPolSpec);
        
        IMPORT_C ~CPolicySpec();
        
        TSecurityAssocSpec iSpec;

        TEpSpec iEpSpec;

        TPolicySpecType iSpectype;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT  
		CPropList* iPropList;
#endif
    private:
        CPolicySpec();
 
        TSglQueLink iNext;
    };


//////////////////////////////////////////////////////////////////////////////
// class CSecpolBundleItem
//////////////////////////////////////////////////////////////////////////////
//  
class CSecpolBundleItem : public CBase
    {
    friend class CSecurityPolicy;
    friend class CPolicySelector;

    public:
    
        ~CSecpolBundleItem();
    
        // Security Association specification
        CPolicySpec *iSpec;
        
        // Tunnel IP address     
        TInetAddr iTunnel;
              
        // Tunnel Endpoint name
        HBufC8* iTunnelEpName;
        
        TSglQueLink iNext;
        
    };

//////////////////////////////////////////////////////////////////////////////
// class TSecpolBundle
//////////////////////////////////////////////////////////////////////////////
//  
class TSecpolBundle : public TSglQue<CSecpolBundleItem>
    {
    public:
        TSecpolBundle(): 

        TSglQue<CSecpolBundleItem>(_FOFF(CSecpolBundleItem, iNext)) {};
    };

typedef class TSglQueIter<CSecpolBundleItem> TSecpolBundleIter;

//////////////////////////////////////////////////////////////////////////////
// class CSAList
//////////////////////////////////////////////////////////////////////////////
//
class CSAList : public CArrayFixFlat<CPolicySpec *>
    {
    public:
        IMPORT_C static CSAList* NewL(TInt aGranularity);

        IMPORT_C void Construct(TInt aGranularity);

        IMPORT_C static CSAList* NewL(CSAList *aSAList);

        IMPORT_C void ConstructL(CSAList *aSAList);
    private:

        CSAList(TInt aGranularity);

        CSAList(CSAList *aSAList);
    };

//////////////////////////////////////////////////////////////////////////////
// struct TSAPairNode
//////////////////////////////////////////////////////////////////////////////
//
struct TSAPairNode
    {
    CPolicySpec *iOldSA;
    CPolicySpec *iNewSA;
    TSAPairNode *iNext;
    };

//////////////////////////////////////////////////////////////////////////////
// class CSAPairList
//  Translation table used when copying a policy
//////////////////////////////////////////////////////////////////////////////
//
class CSAPairList : public CBase
    {
    public:
        ~CSAPairList();

        void AddL(CPolicySpec *aOldSA, CPolicySpec *aNewSA);

        CPolicySpec *Translate(CPolicySpec *aOldSA);
    private:

        TSAPairNode *iList;
    };

//////////////////////////////////////////////////////////////////////////////
// class CSelectorList
//////////////////////////////////////////////////////////////////////////////
//
class CSelectorList : public CArrayFixFlat<CPolicySelector *>
    {
    public:
        IMPORT_C static CSelectorList* NewL(TInt aGranularity);

        IMPORT_C void Construct(TInt aGranularity);

        IMPORT_C static CSelectorList* NewL(CSelectorList* CPolicySelector, 
                                            CSAPairList* aTable);
                                            
        IMPORT_C void ConstructL(CSelectorList *CPolicySelector,
                                 CSAPairList *aTable);
                                 
    private:
        CSelectorList(TInt aGranularity);
        
        CSelectorList(CSelectorList *CPolicySelector, CSAPairList *aTable);
    };

//////////////////////////////////////////////////////////////////////////////
// class CPolicySelector
//  Map the selectors from an IP packet/connection into a bundle
//////////////////////////////////////////////////////////////////////////////
//
class CPolicySelector : public CBase
    {
    friend class CSecurityPolicy;
    friend class TParser;
    public:
        IMPORT_C static CPolicySelector* NewL();
        IMPORT_C void Construct();
        IMPORT_C static CPolicySelector* NewL(CPolicySelector *aPS);
        IMPORT_C void ConstructL(CPolicySelector *aPS);
        
        IMPORT_C ~CPolicySelector();
        
        TInt Match(const TInetAddr& aSrc, 
                   const TInetAddr& aDst,
                   TInt aProtocol, 
                   TInt aIcmpType, 
                   TInt aIcmpCode, 
                   TInt aType) const;
        
        // Checks if this policy is using this SA
        IMPORT_C TBool UseSA(CPolicySpec* aSA,
                             TInetAddr* tunnel = NULL);
        
        // Return a Copy of the aIndex element of the SA bundle
        CSecpolBundleItem* FindBundleL(TInt aIndex);
        
    private:
        CPolicySelector();
        
    public:
        
        //
        // If the addr is to match any address, the mask must all zeroes
        // If the addr is to match as is, the mask must be all ones
        // If the selector is to match a transport connection, the
        // port numbers are stored as non-zero in iDst and iSrc,
        // and iProtocol is non-zero.
        //
        TUint iDirection;
        TInetAddr iRemote;      // including port selector, if port non-zero
        TInetAddr iRemoteMask;  // only address part used, as a mask
        TInetAddr iLocal;       // including port selector, if port non-zero
        TInetAddr iLocalMask;   // only address part used, as a mask
        HBufC8* iRemSelEpName;  //rem endpoint name
        HBufC8* iRemMaskEpName; //rem mask ep name
        HBufC8* iLocSelEpName;  //loc endpoint name
        HBufC8* iLocMaskEpName; //loc mask endpoint name
        TInt iProtocol;         // used, if non-zero
        TInt iIcmpType;         // used, if != -1
        TInt iIcmpCode;         // used, if != -1
        TInt iType;				// used, if != -1
        TBool iDropAction;       // ETrue - drop action
        TInt iSequenceNumber;   // Selector's writing sequence
        TUint32 iCompWord;      // Selector's sorting properties, see
        TBool iGlobalSelector;  //flag specifying if this is a global selector
        TBool iIsFinal;
        TBool iIsMerge;
        TSecpolBundle iBundle;
        TSglQueLink iNext;
        TBuf<20> iInterface; ///< Limited to specific interface, if non-NULL
    };

//////////////////////////////////////////////////////////////////////////////
// class CSecurityPolicy
//  Map the selectors from an IP packet/connection into a bundle
//////////////////////////////////////////////////////////////////////////////
//
class CSecurityPolicy : public CBase
    {
    friend class CProtocolSecpol;
    public:
        IMPORT_C CSecurityPolicy();
        IMPORT_C void ConstructL(CSecurityPolicy *aSecPol);
        IMPORT_C void ConstructL();
        ~CSecurityPolicy();

        IMPORT_C CPolicySpec *FindSpec(const TDesC8 &aName);

        TSecpolBundle *FindBundle(TUint aDirection, 
                                  const TInetAddr &aSrc, 
                                  const TInetAddr &aDst,
                                  TInt aProtocol,
                                  TInt aIcmpType,
                                  TInt aIcmpCode,
                                  TInt aType);

        TInt SearchForEPNameL(TDesC& aTokenString);   

        //
        // The following methods are really intended to be used only
        // by the Policy parser, not for generic consumption!
        //
        void Add(CPolicySpec *aSpec) 
            {
            iSpecs->AppendL(aSpec);
            }

        void Add(CPolicySelector *aSelector) 
            {
            iSelectors->AppendL(aSelector);
            }
        
        inline CSAList *SAList()
            {
            return iSpecs;
            }

        inline void SetSAList(CSAList *aSAList) 
            {
            delete iSpecs; 
            iSpecs = aSAList;
            }

        inline CSelectorList *SelectorList()
            {
            return iSelectors;
            }

        inline void SetSelectorList(CSelectorList *aSelList)
            {
            delete iSelectors; 
            iSelectors = aSelList;
            }
            
    private:

        CSAPairList* CreateTranslationTableL(CSAList* aOldSAList, 
                                             CSAList* aNewSAList);
        
    private:
        
        CSAList* iSpecs;

        CSelectorList* iSelectors;
    };
    
#endif
