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
// @file pfqosparser.h
// Header file for PF_QOS PARSER
// @internalTechnology
// @released
//

#ifndef __PFQOSPARSER_H__
#define __PFQOSPARSER_H__

#include <e32std.h>
#include <e32base.h>

#include <networking/pfqoslib.h>

/** @internalTechnology */
const TInt KBufSize = 2048;


/** 
 * Base class for extension policy variables 
 *
 * @internalTechnology 
 */
class TVariableBase
    {
    public:
    IMPORT_C virtual ~TVariableBase();
    inline TInt Type() const;
    inline const TDesC& Name() const;
    protected:
    TInt iType;
    TName iName;
    public:
    TDblQueLink iNext;
    };

/** 
 * Integer variable 
 *
 * @internalTechnology 
 */
class TIntVariable : public TVariableBase
    {
    public:
    IMPORT_C TIntVariable(const TDesC& aName, TInt aValue);
    inline void SetValue(TInt aValue);
    inline TInt Value() const;
    private:
    TInt iValue;
    };

/** 
 * Real variable 
 *
 * @internalTechnology 
 */
class TRealVariable : public TVariableBase
    {
    public:
    IMPORT_C TRealVariable(const TDesC& aName, TReal aValue);
    inline void SetValue(TReal aValue);
    inline TReal Value() const;
    private:
    TReal iValue;
    };


/** 
 * String variable 
 *
 * @internalTechnology 
 */
class TStringVariable : public TVariableBase
    {
    public:
    IMPORT_C TStringVariable(const TDesC& aName, const TDesC& aValue);
    inline void SetValue(TDesC& aValue);
    inline const TDesC& Value() const;
    private:
    TName iValue;
    };


/** @internalTechnology */
typedef TDblQue<TVariableBase> TVariableQueue;
/** @internalTechnology */
typedef TDblQueIter<TVariableBase> TVariableQueueIter;

/** 
 * Extension policy data 
 * 
 * @internalTechnology 
 */
class CExtension : public CBase
    {
    public:
    IMPORT_C static CExtension* NewL();
    IMPORT_C static CExtension* NewL(const TDesC8& aData);
    IMPORT_C ~CExtension();
    IMPORT_C void SetName(const TDesC& aName);
    /*
     *  Adds the integer variable into the iVariables queue.
     *  Exception: SBLP variables (SblpMediaComponentNumber 
     *  and SblpIPFlowNumber) are not checked for duplicates.
     *
     *  Return KErrAlreadyExists if it already found this variable 
     *  in the queue otherwise KErrNone
     */
    IMPORT_C TInt AddIntegerL(const TDesC& aName, TInt aValue);
    /*
     *  Adds the real variable into the iVariables queue.
     *  Exception: SBLP variables (SblpMediaComponentNumber 
     *  and SblpIPFlowNumber) are not checked for duplicates.
     *
     *  Return KErrAlreadyExists if it already found this variable 
     *  in the queue otherwise KErrNone
     */
    IMPORT_C TInt AddRealL(const TDesC& aName, TReal aValue);
    /*
     *  Adds the string variable into the iVariables queue.
     *  Exception: SBLP variable (SblpMediaAuthorizationToken) 
     *  is not checked for duplicates.
     *
     *  Return KErrAlreadyExists if it already found this variable 
     *  in the queue otherwise KErrNone
     */
    IMPORT_C TInt AddStringL(const TDesC& aName, const TDesC& aValue);
    IMPORT_C TVariableBase* FindVariable(const TDesC& aName);
    IMPORT_C TInt Copy(TDes8& aData);
    IMPORT_C TInt CopyL(const TDesC8& aData);
    IMPORT_C TInt CopyL(CExtension& aExtension);
    IMPORT_C const TPtrC8 Data();
    IMPORT_C TInt Length() const;
    inline const TDesC& Name() const;
    inline void SetType(TInt aType);
    inline TInt Type() const;
    inline TVariableQueue& Queue();

    public:
    TDblQueLink iNext;

    private:
    CExtension();
    void ConstructL();
    void InitL();
    void SetIntValueL(TInt aValue, const TDesC& aName);
    void SetReal32ValueL(TReal32 aValue, const TDesC& aName);
    void SetStringValueL(const TDesC& aValue, const TDesC& aName);
    void SetLengthL();
    void Reset();

    private:
    TInt iType;
    TName iName;
    TInt iPos;
    CBufFlat* iBuf;
    TVariableQueue iVariables;
    };


/** 
 * CSelectorBase
 * 
 * @internalTechnology 
 */
class CSelectorBase : public CBase
    {
    public:
    IMPORT_C CSelectorBase(TUint aType);
    IMPORT_C CSelectorBase(/*lint -e(1724) thinks this chould be 'const' */ CSelectorBase& aSel);
    IMPORT_C CSelectorBase(TPfqosBase& aBase, TPfqosSelector& aSel, 
                    TPfqosAddress& aSrc, TPfqosAddress& aDst, TUint aType);
    IMPORT_C virtual ~CSelectorBase();

    TUint16      iDstPortMax;
    TInetAddr    iDst;           // including port selector, if port non-zero
    TInetAddr    iDstMask;       // only address part used, as a mask
    TUint16      iSrcPortMax;
    TInetAddr    iSrc;           // including port selector, if port non-zero
    TInetAddr    iSrcMask;       // only address part used, as a mask
    TUint8       iProtocol;      // used, if non-zero
    TCheckedUid  iUid;           // Uid
    TUint32      iIapId;         // Internet Access Point Identifier
    TUint16      iPolicyOptions;
    TUint        iPriority;      // Priority of the policy (default, 
                                 // application, or override)
    TUint        iType;          // Policy type
    TName        iName;          // Policy name --experimental--
    TUint        iOwner;         // Owner - reserved for QoS framework only!!
    TDblQueLink iNext;
    };

/** @internalTechnology */
typedef TDblQue<CExtension> TExtensionQueue;
/** @internalTechnology */
typedef TDblQueIter<CExtension> TExtensionQueueIter;

/** 
 * Extension policy to be used by additional modules
 * 
 * @internalTechnology 
 */
class CExtensionPolicy : public CSelectorBase
    {
    public:
    IMPORT_C CExtensionPolicy(TPfqosBase& aBase, TPfqosSelector& aSel, 
                    TPfqosAddress& aSrc, TPfqosAddress& aDst, TInt aType);
    IMPORT_C CExtensionPolicy();
    IMPORT_C ~CExtensionPolicy();
    inline TInt Type() const;
    inline void SetType(TInt aType);
    IMPORT_C void AddExtensionL(CExtension& aExtension);
    IMPORT_C void AddExtensionL(const TDesC8& aExtension);
    inline TExtensionQueue& Extensions();
    protected:
    TExtensionQueue iExtensions;
    };


/** @internalTechnology */
enum TTokenType
    {
    ETokenString,
    ETokenEqual,
    ETokenComma,
    ETokenBraceLeft,
    ETokenBraceRight,
    ETokenParLeft,
    ETokenParRight,
    ETokenError,
    ETokenEof
    };

/** @internalTechnology */
typedef TDblQue<CExtensionPolicy> TQoSPolicyQueue;
/** @internalTechnology */
typedef TDblQueIter<CExtensionPolicy> TQoSPolicyQueueIter;

/** 
 * Parser for policy file
 * 
 * @internalTechnology 
 */
class TPolicyParser : public TLex
    {
    public:
    IMPORT_C TPolicyParser(const TDesC &aPolicy);
    IMPORT_C ~TPolicyParser();
    IMPORT_C TInt ParseL();
    inline TExtensionQueue& Extensions();
    inline TQoSPolicyQueue& Policies();

    private:
    TInt ParseIPAddrAndMask(TInetAddr& aAddr, TInetAddr& aMask);
    TInt ParseExtensionSpecL();
    TInt ParseExtensionParams(CExtension& aBuf);
    TInt FindExtensionPolicy(CExtensionPolicy *aSel);
    TInt ParsePolicyL(TInt aPolicyType);
    TTokenType NextToken();
    TTokenType GetStringValue();
    void SkipSpaceAndMark();
    // void Error(const TDesC &aFmt, ...);
    void Error(TRefByValue<const TDesC> aFmt, ...);
    void AddPolicy(CExtensionPolicy& aPolicy) { iPolicies.AddLast(aPolicy); }

    TExtensionQueue iExtensions;
    TQoSPolicyQueue iPolicies;

    public:
    int iLine;
    TBuf<200> iMsg;
    TPtrC iToken;
    };

/** @internalTechnology */
inline TExtensionQueue& TPolicyParser::Extensions()
    { return iExtensions; }

/** @internalTechnology */
inline TQoSPolicyQueue& TPolicyParser::Policies()
    { return iPolicies; }

//
// Inline methods
//
/** @internalTechnology */
inline TInt TVariableBase::Type() const
    { return iType; }

/** @internalTechnology */
inline const TDesC& TVariableBase::Name() const
    { return iName; }

/** @internalTechnology */
inline void TIntVariable::SetValue(TInt aValue)
    { iValue = aValue; }

/** @internalTechnology */
inline TInt TIntVariable::Value() const
    { return iValue; }

/** @internalTechnology */
inline void TRealVariable::SetValue(TReal aValue)
    { iValue = aValue; };

/** @internalTechnology */
inline TReal TRealVariable::Value() const
    { return iValue; };

/** @internalTechnology */
inline void TStringVariable::SetValue(TDesC& aValue)
    { if (iValue.MaxLength() >= aValue.Length()) iValue.Copy(aValue); };

/** @internalTechnology */
inline const TDesC& TStringVariable::Value() const
    { return iValue; }

/** @internalTechnology */
inline const TDesC& CExtension::Name() const
    { return iName; }

/** @internalTechnology */
inline void CExtension::SetType(TInt aType)
    { iType = aType; }

/** @internalTechnology */
inline TInt CExtension::Type() const
    { return iType; }

/** @internalTechnology */
inline TVariableQueue& CExtension::Queue()
    { return iVariables; }

/** @internalTechnology */
inline TInt CExtensionPolicy::Type() const
    { return iType; }

/** @internalTechnology */
inline void CExtensionPolicy::SetType(TInt aType)
    { iType = aType; }

/** @internalTechnology */
inline TExtensionQueue& CExtensionPolicy::Extensions()
    { return iExtensions; }


#endif
