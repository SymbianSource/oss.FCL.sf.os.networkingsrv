// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// spdb.cpp - security policy database module
// "Runtime" methods of "CSecurityPolicy" class
//

#include <networking/pfkeyv2.h>
#include "spdb.h"

//
//  CPolicySelector
//
CPolicySelector::CPolicySelector()
    {}

EXPORT_C CPolicySelector*
CPolicySelector::NewL()
    {
    CPolicySelector* self = new (ELeave) CPolicySelector();
    self->Construct();
    return self;
    }

EXPORT_C void
CPolicySelector::Construct()
    {
    iIcmpType = -1;           // used, if != -1
    iType = -1;               // used, if != -1
    iIcmpCode = -1;           // used, if != -1
    iGlobalSelector = EFalse; // the global flag defaults to false
    iIsFinal = EFalse;
    iIsMerge = EFalse;
    iDirection = KPolicySelector_SYMMETRIC;
    iDropAction = EFalse;
    iProtocol = 0;

    // Init addresses to undefined    
    iLocal.SetFamily(KAFUnspec);
    iLocalMask.SetFamily(KAFUnspec);
    iRemote.SetFamily(KAFUnspec);
    iRemoteMask.SetFamily(KAFUnspec);
    
    // Clear interface name
    iInterface.Zero();
    
    // Init Endpoint names to NULL
    iLocSelEpName = NULL;
    iLocMaskEpName = NULL;
    iRemSelEpName = NULL;
    iRemMaskEpName = NULL;
    
    iSequenceNumber = 0;
    iCompWord = 0;
    }

EXPORT_C CPolicySelector*
CPolicySelector::NewL(CPolicySelector* aPS)
    {
    CPolicySelector* self = new (ELeave) CPolicySelector();
    CleanupStack::PushL(self);
    self->ConstructL(aPS);
    CleanupStack::Pop();
    return self;
    }

EXPORT_C void
CPolicySelector::ConstructL(CPolicySelector* aPS)
    {
    iDirection = aPS->iDirection;
    iRemote = aPS->iRemote;         // including port selector, if port non-zero
    iRemoteMask = aPS->iRemoteMask; // only address part used, as a mask
    iLocal = aPS->iLocal;           // including port selector, if port non-zero
    iLocalMask = aPS->iLocalMask;   // only address part used, as a mask
    iProtocol = aPS->iProtocol;     // used, if non-zero
    iIcmpType = aPS->iIcmpType;     // used, if != -1
    iType = aPS->iType;             // used, if != -1
    iIcmpCode = aPS->iIcmpCode;     // used, if != -1
    iGlobalSelector = aPS->iGlobalSelector;
    iIsFinal = aPS->iIsFinal;
    iIsMerge = aPS->iIsMerge;
    iDropAction = aPS->iDropAction;
    iInterface = aPS->iInterface;
    
    iSequenceNumber = aPS->iSequenceNumber;
    iCompWord = aPS->iSequenceNumber;

    // Copy EndPoint names    
    if (aPS->iLocSelEpName && aPS->iLocSelEpName->Length())
        {
        iLocSelEpName = HBufC8::NewL(aPS->iLocSelEpName->Length());
        *iLocSelEpName = *aPS->iLocSelEpName;
        }
    if (aPS->iLocMaskEpName && aPS->iLocMaskEpName->Length())
        {
        iLocMaskEpName = HBufC8::NewL(aPS->iLocMaskEpName->Length());
        *iLocMaskEpName = *aPS->iLocMaskEpName;
        }
    if (aPS->iRemSelEpName && aPS->iRemSelEpName->Length())
        {
        iRemSelEpName = HBufC8::NewL(aPS->iRemSelEpName->Length());
        *iRemSelEpName = *aPS->iRemSelEpName;
        }
    if (aPS->iRemMaskEpName && aPS->iRemMaskEpName->Length())
        {
        iRemMaskEpName = HBufC8::NewL(aPS->iRemMaskEpName->Length());
        *iRemMaskEpName = *aPS->iRemMaskEpName;
        }

    TSecpolBundleIter iterl(aPS->iBundle);
    CSecpolBundleItem* itemL(NULL);
    CSecpolBundleItem* newItemL(NULL);
    while ((itemL = iterl++) != NULL)
        {
        newItemL = new (ELeave) CSecpolBundleItem;
        
        // Points to the same SA
        newItemL->iSpec = itemL->iSpec;      
        
        // No need to fill iNext. Is filled when adding
        newItemL->iTunnel = itemL->iTunnel;  
        iBundle.AddLast(*newItemL);
        }

    iNext = aPS->iNext;
    }

CSecpolBundleItem*
CPolicySelector::FindBundleL(TInt aIndex)
    {
    TSecpolBundleIter iterL(iBundle);
    CSecpolBundleItem* itemL(NULL);
    CSecpolBundleItem* newItemL(NULL);
    TInt i = 0;
    while (((itemL = iterL++) != NULL) && (i < aIndex))
        {
        i++;
        }

    // The element exists. We create a copy
    if ((i == aIndex) && (itemL != NULL))    
        {
        newItemL = new (ELeave) CSecpolBundleItem;
        
        // Need a copy to have separed bundle lists
        newItemL->iSpec = itemL->iSpec;      
        
        // No need to fill iNext. Is filled when adding
        newItemL->iTunnel = itemL->iTunnel;  
        }

    return newItemL;
    }

CSecpolBundleItem::~CSecpolBundleItem()
    {
    delete iTunnelEpName;
    }

EXPORT_C
CPolicySelector::~CPolicySelector()
    {
    TSecpolBundleIter iterl(iBundle);
    CSecpolBundleItem* itemL(NULL);
    while ((itemL = iterl++) != NULL)
        {
        iBundle.Remove(*itemL);
        delete itemL;
        }

    delete iRemSelEpName;
    delete iRemMaskEpName;
    delete iLocMaskEpName;
    delete iLocSelEpName;
    }

// Match a selector against the packet information
//
// Returns 0, if selector does not match the information
//         1, if selector matches the information
TInt
CPolicySelector::Match(
    const TInetAddr &aSrc,         // The src address, and port if known
    const TInetAddr &aDst,         // The dst address, and port if known
    TInt aProtocol,                // Transport protocol, if > 0 (known)
    TInt aIcmpType,                // ICMP Type, if ICMP (-1 otherwise)
    TInt aIcmpCode,                // ICMP Code, if ICMP (-1 otherwise)
    TInt aType) const              // Type code, (-1 if not used)
    {
    return (aDst.Match(iRemote, iRemoteMask)
            && aSrc.Match(iLocal, iLocalMask)
            && (iProtocol == 0 || iProtocol == aProtocol)
            && (iRemote.Port() == 0 || iRemote.Port() == aDst.Port())
            && (iLocal.Port() == 0 || iLocal.Port() == aDst.Port())
            && (iIcmpType == -1 || iIcmpType == aIcmpType)
            && (iIcmpCode == -1 || iIcmpCode == aIcmpCode)
            && (iType == -1 || iType == aType));
    }

// Checks if this policy is using this SA
EXPORT_C TBool
CPolicySelector::UseSA(CPolicySpec* aSA, TInetAddr* tunnel)
    {
    TSecpolBundleIter iterBundle(iBundle);
    CSecpolBundleItem* itemBundle(NULL);

    while ((itemBundle = iterBundle++) != NULL)
        {
        if (itemBundle->iSpec)
            {
            // SA used in a Policy. 2 SA cannot have the same name
            if (!(itemBundle->iSpec->iName->Compare(*aSA->iName)))
                {
                if (tunnel)
                    {
                    *tunnel = itemBundle->iTunnel;
                    }
                return ETrue;
                }
            }
        }

    return EFalse;  // This selector doesn't use the SA
    }

//
// CSAPairList: Translation table used when copying a policy
CSAPairList::~CSAPairList()
    {
    TSAPairNode* next(NULL);
    TSAPairNode* node(iList);
    while (node)
        {
        next = node->iNext;
        delete node;
        node = next;
        }
    }

//
// Add at the begining to make it faster
void
CSAPairList::AddL(CPolicySpec* aOldSA, CPolicySpec* aNewSA)
    {
    TSAPairNode* node = new (ELeave) TSAPairNode;
    node->iOldSA = aOldSA;
    node->iNewSA = aNewSA;
    node->iNext = iList;
    iList = node;
    }

CPolicySpec*
CSAPairList::Translate(CPolicySpec* aOldSA)
    {
    TSAPairNode* node = iList;
    while (node)
        {
        if (node->iOldSA == aOldSA)
            return node->iNewSA;
        node = node->iNext;
        }

    // Not found
    return(NULL);
    }

EXPORT_C
CSecurityPolicy::CSecurityPolicy()
    {}

EXPORT_C void
CSecurityPolicy::ConstructL()
    {
    iSpecs = CSAList::NewL(1);
    iSelectors = CSelectorList::NewL(1);
    }

// Creates a Security Policy from an existing one
EXPORT_C void
CSecurityPolicy::ConstructL(CSecurityPolicy* aSecPol)
    {
    // Creates a new SA List with new SA nodes!!!
    if (aSecPol->SAList()->Count() > 0)
        {
        iSpecs = CSAList::NewL(aSecPol->SAList());
        }
    else
        {
        iSpecs = CSAList::NewL(1);
        }

    // The selector bundles use references to the old SA nodes
    // so if we copy only the content we'll have invalid references.
    // Therefore, we need a translations tables from old nodes to new
    // ones to pass it to the selector constructor
    if (aSecPol->SelectorList()->Count() > 0)
        {
        CSAPairList* table = CreateTranslationTableL(aSecPol->SAList(), iSpecs);
        CleanupStack::PushL(table);
        iSelectors = CSelectorList::NewL(aSecPol->SelectorList(), table);

        // The table is not needed anymore
        CleanupStack::PopAndDestroy();
        }
    else
        {
        iSelectors = CSelectorList::NewL(1);
        }
    }

CSAPairList*
CSecurityPolicy::CreateTranslationTableL(
    CSAList* aOldSAList,
    CSAList* aNewSAList)
    {
    CSAPairList* table = new (ELeave) CSAPairList;
    CleanupStack::PushL(table);
    TInt count(aOldSAList->Count());
    for (TInt i = 0; i < count; i++)
        {
        table->AddL(aOldSAList->At(i), aNewSAList->At(i));
        }
    CleanupStack::Pop();
    return (table);
    }

CSecurityPolicy::~CSecurityPolicy()
    {
    if (iSpecs)
        {
        // Deletes all the elems in the list
        TInt count(iSpecs->Count());
        for (TInt i = 0; i < count; i++) 
            {
            delete iSpecs->At(i);
            }
        delete iSpecs;
        iSpecs = NULL;
        }

    if (iSelectors)
        {
        // Deletes all the elems in the list
        TInt count(iSelectors->Count());
        for (TInt i = 0; i < count; i++) 
            {
            delete iSelectors->At(i);
            }
        delete iSelectors;
        iSelectors = NULL;
        }
    }

TSecpolBundle*
CSecurityPolicy::FindBundle(
    TUint aDirection,              // Direction flag
    const TInetAddr& aSrc,         // Source Address (and optionally port)
    const TInetAddr& aDst,         // Destination Address (and optionally port)
    TInt aProtocol,                // Transport protocol (if > 0)
    TInt aIcmpType,                // (if != -1)
    TInt aIcmpCode,                // (if != -1)
    TInt aType)                    // (if != -1)
    {
    TInt count(iSelectors->Count());
    for (TInt i = 0; i < count; i++)
        {
        CPolicySelector* selector = iSelectors->At(i);
        if ((selector->iDirection & aDirection)
            && selector->Match(aSrc, 
                               aDst, 
                               aProtocol, 
                               aIcmpType, 
                               aIcmpCode, 
                               aType))
            {
            return (&selector->iBundle);
            }
        }

    return (NULL);
    }

TInt
CSecurityPolicy::SearchForEPNameL(TDesC& aTokenString)
    {
    TInt err(KErrNotFound);
    HBufC8* name = HBufC8::NewL(aTokenString.Length());
    name->Des().Copy(aTokenString);

    // Iterate through the policy specification list
    TInt count(iSpecs->Count());
    for (TInt i = 0; i < count; i++)
        {
        CPolicySpec* polSpec = iSpecs->At(i);
        // Check if spesification type is EndPoint and name for it exists
        if (polSpec->iSpectype == EPolSpecEP && polSpec->iName)
            {
            // Compare EndPoint name against given name
            if (name->Des().Compare(polSpec->iName->Des()) == 0)
                {
                // Match found so set return value to success
                err = KErrNone;
                break;
                }
            }
        }

    // Free memory allocated for name and then return
    delete name;
    return (err);
    }

EXPORT_C CPolicySpec*
CSecurityPolicy::FindSpec(const TDesC8& aName)
    {
    CPolicySpec* spec(NULL);
    TInt count(iSpecs->Count());
    for (TInt i = 0; i < count; i++)
        {
        spec = iSpecs->At(i);
        if ((*spec->iName).Compare(aName) == 0)
            {
            return spec;
            }
        }
    return (NULL);
    }

//
//  CPolicySpec
//
CPolicySpec::CPolicySpec()
    {
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT      
    iPropList = CPropList::NewL(1);
#endif
    }

EXPORT_C CPolicySpec*
CPolicySpec::NewL()
    {
    CPolicySpec* self = new (ELeave) CPolicySpec();
    self->Construct();
    return self;
    }

EXPORT_C void
CPolicySpec::Construct()
    {
    iEpSpec.iIsOptional = EFalse;
    iEpSpec.iEpAddr = NULL;
    }

EXPORT_C CPolicySpec*
CPolicySpec::NewL(TDesC& aName, TPolicySpecType iSpectype)
    {
    CPolicySpec* self = new (ELeave) CPolicySpec();
    CleanupStack::PushL(self);
    self->ConstructL(aName, iSpectype);
    CleanupStack::Pop();
    return self;
    }

EXPORT_C void
CPolicySpec::ConstructL(TDesC& aName, TPolicySpecType aSpectype)
    {
    iName = HBufC8::NewL(aName.Length());
    iName->Des().Copy(aName);
    iSpectype = aSpectype;
    }

// Used to initialize with an existing CPolicySpec
EXPORT_C CPolicySpec*
CPolicySpec::NewL(CPolicySpec* aPolSpec)
    {
    CPolicySpec* self = new (ELeave) CPolicySpec();
    CleanupStack::PushL(self);
    self->ConstructL(aPolSpec);
    CleanupStack::Pop();
    return self;
    }

EXPORT_C void
CPolicySpec::ConstructL(CPolicySpec* aPolSpec)
    {
    // Always bigger than 0
    iName = HBufC8::NewL(aPolSpec->iName->Length()); 
    *iName = *aPolSpec->iName;

    if (aPolSpec->iRemoteIdentity)
        {
        iRemoteIdentity = HBufC8::NewL(aPolSpec->iRemoteIdentity->Length());
        *iRemoteIdentity = *aPolSpec->iRemoteIdentity;
        }

    if (aPolSpec->iLocalIdentity)
        {
        iLocalIdentity = HBufC8::NewL(aPolSpec->iLocalIdentity->Length());
        *iLocalIdentity = *aPolSpec->iLocalIdentity;
        }
    iSpec = aPolSpec->iSpec;
    // Even the queue position is cloned    
    iNext = aPolSpec->iNext;  
    }

EXPORT_C
CPolicySpec::~CPolicySpec()
    {
    delete iName;
    delete iRemoteIdentity;
    delete iLocalIdentity;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT      
      if (iPropList)
        {
        // Deletes all the elems in the list
        TInt count(iPropList->Count());
        for (TInt i = 0; i < count; i++) 
            {
            delete iPropList->At(i);
            }
        delete iPropList;
        iPropList = NULL;       
        }
#endif       
    }
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT  
// CPropList
CPropList::CPropList(TInt aGranularity) :
    CArrayFixFlat<CSecurityProposalSpec *>(aGranularity)
    {}

CPropList*
CPropList::NewL(TInt aGranularity)
    {
    CPropList* self = new (ELeave) CPropList(aGranularity);
    self->Construct(aGranularity);
    return self;
    }

void
CPropList::Construct(TInt /* aGranularity */)
    {}

CPropList::CPropList(CPropList* aSAList) :
    CArrayFixFlat<CSecurityProposalSpec *>(aSAList->Count())
    {}

CPropList*
CPropList::NewL(CPropList* aSAList)
    {
    CPropList* self = new (ELeave) CPropList(aSAList);
    CleanupStack::PushL(self);
    self->ConstructL(aSAList);
    CleanupStack::Pop();
    return self;
    }

void
CPropList::ConstructL(CPropList* aSAList)
    {
    TInt count(aSAList->Count());
 
    }
#endif    
// CSAList
CSAList::CSAList(TInt aGranularity) :
    CArrayFixFlat<CPolicySpec *>(aGranularity)
    {}

EXPORT_C CSAList*
CSAList::NewL(TInt aGranularity)
    {
    CSAList* self = new (ELeave) CSAList(aGranularity);
    self->Construct(aGranularity);
    return self;
    }

EXPORT_C void
CSAList::Construct(TInt /* aGranularity */)
    {}

CSAList::CSAList(CSAList* aSAList) :
    CArrayFixFlat<CPolicySpec *>(aSAList->Count())
    {}

EXPORT_C CSAList*
CSAList::NewL(CSAList* aSAList)
    {
    CSAList* self = new (ELeave) CSAList(aSAList);
    CleanupStack::PushL(self);
    self->ConstructL(aSAList);
    CleanupStack::Pop();
    return self;
    }

EXPORT_C void
CSAList::ConstructL(CSAList* aSAList)
    {
    TInt count(aSAList->Count());
    for (TInt i = 0; i < count; i++)
        {
        CPolicySpec* policy = CPolicySpec::NewL(aSAList->At(i));
        CleanupStack::PushL(policy);
        AppendL(policy);
        CleanupStack::Pop();
        }
    }

//
// CSelectorList
//
CSelectorList::CSelectorList(TInt aGranularity) :
    CArrayFixFlat<CPolicySelector *>(aGranularity)
    {}

EXPORT_C CSelectorList*
CSelectorList::NewL(TInt aGranularity)
    {
    CSelectorList* self = new (ELeave) CSelectorList(aGranularity);
    self->Construct(aGranularity);
    return self;
    }

EXPORT_C void
CSelectorList::Construct(TInt /*aGranularity*/)
    {}

CSelectorList::CSelectorList(CSelectorList* aSelList,
                             CSAPairList* /* aTable */) :
    CArrayFixFlat<CPolicySelector *>(aSelList->Count())
    {}

EXPORT_C CSelectorList*
CSelectorList::NewL(CSelectorList* aSelList,
                    CSAPairList* aTable)
    {
    CSelectorList* self = new (ELeave) CSelectorList(aSelList, aTable);
    CleanupStack::PushL(self);
    self->ConstructL(aSelList, aTable);
    CleanupStack::Pop();
    return self;
    }

EXPORT_C void
CSelectorList::ConstructL(CSelectorList* aSelList,
                          CSAPairList* aTable)
    {
    TInt count(aSelList->Count());
    for (TInt i = 0; i < count; i++)
        {
        CPolicySelector* selector = CPolicySelector::NewL(aSelList->At(i));
        // Bundle translation
        TSecpolBundleIter iterL(selector->iBundle);
        CSecpolBundleItem* itemL(NULL);
        while (((itemL = iterL++) != NULL))
            {
            if (itemL->iSpec)
                {
                itemL->iSpec = aTable->Translate(itemL->iSpec);
                }
            }

        // Specs in the Bundle translated
        CleanupStack::PushL(selector);
        AppendL(selector);
        CleanupStack::Pop();
        }
    }

