/**
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* IPSec policy parser main module 
*
*/



/**
 @file ipsecpolparser.h
 @released
 @internalTechnology
*/

#ifndef __IPSECPOLPARSER_H
#define __IPSECPOLPARSER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDE FILES
//////////////////////////////////////////////////////////////////////////////

#include <f32file.h>
#include <in_sock.h>

#include "spdb.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

#define FIRST_SEC_PARSER_VERSION    1
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#define SEC_PARSER_VERSION          4
#else
#define SEC_PARSER_VERSION          3
#endif

#define MAX_EALG_VALUE              255
#define MAX_INFO_SIZE               1024
#define PIECE_AVG_LENGTH            2048

#define PFKEY_INI_OUTBOUND          8
#define PFKEY_INI_INBOUND           4
#define KErrKeyParser               50

const TInt KPolicyBufferSizeIncrement = 6000;

//////////////////////////////////////////////////////////////////////////////
// ENUMERATIONS
//////////////////////////////////////////////////////////////////////////////

typedef enum
{
    token_string,
    token_equal,
    token_comma,
    token_brace_left,
    token_brace_right,
    token_par_left,
    token_par_right,
    token_error,
    token_eof
}
token_type;

//////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
//////////////////////////////////////////////////////////////////////////////

class CIpSecurityPiece;

//////////////////////////////////////////////////////////////////////////////
// CLASS DECLARATIONS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// class TPolicyParser
//  This parser is utilized by IPsec Policy Manager to convert a given policy
//  from string format into binary object format and vice versa.
//////////////////////////////////////////////////////////////////////////////
//
class TPolicyParser : public TLex
    {
public:
    IMPORT_C TPolicyParser(const TDesC& aPolicy);

    IMPORT_C TInt ParseL(CIpSecurityPiece* aPieceData);

    IMPORT_C static TInt Write(CSecurityPolicy* aSp,
                               HBufC8*& aPolBfr,
                               TBool aSortingOrder = EFalse);

    IMPORT_C static TInt BufferAppend(HBufC8*& aPolBfr,
                                      const TDesC8& aText);

private:
    static TInt WriteSAs(CSAList* aSAList,
                         HBufC8*& aPolBfr);

    static void TextSA(CPolicySpec* aSA, TDes8& aBuf);

    static TInt WriteSelectors(CSelectorList* aSelList,
                               HBufC8*& aPolBfr,
                               TBool aSortingOrder = EFalse);

    static TInt WriteSelectorsInSortingOrder(CSelectorList* aSelList,
            HBufC8*& aPolBfr,
            TBool aSortingOrder = ETrue);

    static void TextSel(CPolicySelector* aSel,
                        TDes8& aBuf,
                        TBool aSortingOrder = EFalse);

    TInt parse_ip_addr_and_maskL(TInetAddr& addr,
                                 TInetAddr& mask,
                                 HBufC8*& aSelEpName,
                                 HBufC8*& aMaskEpName,
                                 CSecurityPolicy* aSecPol = NULL);

    TInt parse_sa_spec_listL(TSecpolBundle& aBundle,
                             CSecurityPolicy* aSp);

    TInt parse_sa_spec_paramsL(CPolicySpec& aSpec);

    TInt parse_conn2saL(CSecurityPolicy* aSp);

    TInt parse_sa_specL(CSecurityPolicy* aSp);

    TInt parse_ep_specL(CSecurityPolicy* aSp);

    TInt parse_ep_spec_paramsL(CPolicySpec& aSpec);

    token_type NextToken();

    void SkipSpaceAndMark();

    void Error(TRefByValue <const TDesC> aFmt, ...);

#ifdef  SYMBIAN_IPSEC_VOIP_SUPPORT    
    TInt validateProposals(CPropList& aPropList);
    CSecurityProposalSpec* CreateProposalL(CPropList& aPropList);
#endif    
    
public:

    int iLine;

    TBuf<200> iMsg;

    TPtrC iToken;
    };

//////////////////////////////////////////////////////////////////////////////
// class CKeysData
//  This object contains IPsec key information.
//////////////////////////////////////////////////////////////////////////////
//
class CKeysData : public CBase
    {
public:
    IMPORT_C CKeysData();
    IMPORT_C CKeysData(CKeysData* aKey);

public:
    TUint8 sa_type;
    TInt spi;
    TUint8 encr_alg;
    TUint8 auth_alg;
    TInt direction;
    TInt lifetime_bytes;
    TInt lifetime_sec;
    TInetAddr src_addr;
    TInetAddr dst_addr;
    TUint8 protocol;
    TBuf8<256> auth_key;
    TBuf8<256> encr_key;
    };

//////////////////////////////////////////////////////////////////////////////
// class CKeysDataArray
//  Container for storing IPsec keys.
//////////////////////////////////////////////////////////////////////////////
//
class CKeysDataArray : public CArrayFixFlat<CKeysData *>
    {
public:
    IMPORT_C static CKeysDataArray* NewL(TInt aGranularity);
    IMPORT_C static CKeysDataArray* NewL(CKeysDataArray* aData);

    IMPORT_C void Construct(TInt aGranularity);

    IMPORT_C void ConstructL(CKeysDataArray* aData);

    IMPORT_C ~CKeysDataArray();

    IMPORT_C void CopyL(CKeysDataArray* aData);

    IMPORT_C void Empty();

private:

    CKeysDataArray(TInt aGranularity);

    CKeysDataArray(CKeysDataArray* aData);
    };

//////////////////////////////////////////////////////////////////////////////
// class TKeyParser
//  Parses IPsec key information from string format into binary format
//  and vice versa.
//////////////////////////////////////////////////////////////////////////////
//
class TKeyParser : public TLex
    {
public:
    IMPORT_C TKeyParser(const TDesC& aStr);

    IMPORT_C TInt ParseL(CKeysDataArray* aKeys);

    IMPORT_C static TInt Write(CKeysDataArray* aKeys, RFile& aFile);

private:
    static void TextPFKey(CKeysData* aKey, TDes8& aElem);

    void NextToken();

    TPtrC8 DeHex(const TDesC& aStr);

    int SkipSpaceAndMark();

    static TUint8 HexVal(TUint8 aChar);

    TPtrC iToken;

    // Non-zero, if next token is first in line
    TInt iFirst;

    TBuf8<256> iHex;
    };

//////////////////////////////////////////////////////////////////////////////
// class CIpSecurityPiece
//  Contains all IPsec policy information.
//////////////////////////////////////////////////////////////////////////////
//
class CIpSecurityPiece : public CBase
    {
public:

    // Factory methods for object creation
    IMPORT_C void ConstructL(TInt aSize = 64);
    IMPORT_C ~CIpSecurityPiece();

    //
    IMPORT_C void SetInfoL(const TDesC& aDes);

    inline HBufC* Info() { return iInfo;}

    inline CSecurityPolicy* Policies() { return iPolicies;}

    inline void SetPolicies(CSecurityPolicy* aPolicy) {iPolicies = aPolicy;}

    inline CKeysDataArray* Keys() { return iKeys;}

    // Buffer for last error text
    TBuf<200> iErrorInfo;

private:

    // Buffer for IPsec policy information text
    HBufC* iInfo;

    // IPsec Policy data
    CSecurityPolicy* iPolicies;

    // IPsec keys data
    CKeysDataArray* iKeys;
    };

//////////////////////////////////////////////////////////////////////////////
// class TIpSecParser
//  Parser for IPsec policies.
//////////////////////////////////////////////////////////////////////////////
//
class TIpSecParser : public TLex
    {
public:
    IMPORT_C TIpSecParser(const TDesC& aDes);

    IMPORT_C TInt ParseL(CIpSecurityPiece* aPiece_data);

    IMPORT_C static TInt Write(CIpSecurityPiece* aPiece_data,
                               HBufC8*& aPolBfr);

    IMPORT_C TInt ParseAndIgnoreIKEL(CIpSecurityPiece* aPiece_data);

private:
    TBool CheckVersion();

    void ParseInfoL(CIpSecurityPiece* aPiece_data);

    TInt ParsePoliciesL(CIpSecurityPiece* aPieceData);

    TInt ParseKeysL(CKeysDataArray* aKeys);

    void NextTag();

    TInt DoParseL(CIpSecurityPiece* aPiece_data, TBool aIncludeIKE);

    static TInt WriteVersion(HBufC8*& aPolBfr);

    static TInt WriteInfo(CIpSecurityPiece* aPiece_data,
                          HBufC8*& aPolBfr);

    static TInt WritePolicies(CIpSecurityPiece* aPiece_data,
                              HBufC8*& aPolBfr);

    static TInt WriteKeys(CIpSecurityPiece* aPiece_data,
                          RFile& aFile);

private:

    // IPsec policy version info
    TInt iVersion;
    };

#endif
