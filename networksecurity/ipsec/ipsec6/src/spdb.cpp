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
// spdb.cpp - IPv6/IPv4 IPsec Security Policy Database (SPD)
// Implementation of the IPsec Security Policy Database (SPD)
//



/**
 @file spdb.cpp
*/
#include <featdiscovery.h>
#include <featureuids.h>
#include <networking/pfkeyv2.h>
#include <networking/ipsecerr.h>
#include <e32std.h>

#include "sa_spec.h"
#include "epdb.h"
#include "spdb.h"

#ifndef OLD_SELECTOR_ORDERING
/** @deprecated
* The old syntax allowed total mixing of "filter" and "selector"
* keywords. The new syntax requires the filter keywords (outbound,
* inbound, merge, final, etc.) to precede any real selectors.
* For backward compatibility, allow old deprecated ordering, if
* OLD_SELECTOR_ORDERING is defined.
*/
#define	OLD_SELECTOR_ORDERING 1
#endif


/**
* Static keyword macro.
*
* Store all keywords as "plain ascii" (narrow, not UNICODE)
*/
#define KEYWORD(s) {(sizeof(s)-1), (const TText8 *)s}

/**
* Keyword number macro.
*
* Generate enum name for the keyword 's'.
*/
#define	KEYENUM(s)	E_ ## s






#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
// CPropList
CPropList::CPropList(TInt aGranularity) :
    CArrayFixFlat<CSecurityProposalSpec *>(aGranularity)
    {}

 CPropList* CPropList::NewL(TInt aGranularity)
    {
    CPropList* self = new (ELeave) CPropList(aGranularity);
    self->Construct(aGranularity);
    return self;
    }

 void CPropList::Construct(TInt /* aGranularity */)
    {}

CPropList::CPropList(CPropList* aSAList) :
    CArrayFixFlat<CSecurityProposalSpec *>(aSAList->Count())
    {}

CPropList* CPropList::NewL(CPropList* aSAList)
    {
    CPropList* self = new (ELeave) CPropList(aSAList);
    CleanupStack::PushL(self);
    self->ConstructL(aSAList);
    CleanupStack::Pop();
    return self;
    }

 void CPropList::ConstructL(CPropList* aSAList)
    {
    TInt count(aSAList->Count());
    }
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

class TKeyword
	/**
	* The keyword definition.
	*/
	{
public:
	TInt iLength;			//< Length of the keyword
	const TText8 *iWord;	//< The keyword
	};

static TInt Lookup(const TKeyword *const aList, const TInt aLength, const TDesC &aToken)
	/**
	* Lookup a keyword from list.
	*
	* @param aList		The list of keyword
	* @param aLength	The number of keywords in the list
	* @param aToken		The keyword to be looked for
	*
	* @return
	*	The index of the matched keyword, or aLength, if none matches.
	*/
	{
	const TInt token_length = aToken.Length();
	for (TInt i = 0; i < aLength; i++)
		{
		const TKeyword &key = aList[i];
		if (key.iLength == token_length)
			{
			// Use own matching loop (instead of Compare), because keywords
			// are stored as 8-bit strings (to make them compact).
			for (TInt k = token_length;;)
				{
				if (--k < 0)
					return i;	// Keyword matched!
				if (key.iWord[k] != aToken[k])
					break;		// No Match!
				}
			}
		}
	return aLength;
	}


typedef enum
	/**
	* Tokens of the IPsec policy syntax.
	*/
	{
	token_string,		//< Any string of non-white space and non-token characters.
	token_question,		//< Question mark: '?'
	token_equal,		//< Equal sign: '='
	token_comma,		//< Comma: ','
	token_brace_left,	//< Left brace: '{'
	token_brace_right,	//< Right brace: '}'
	token_par_left,		//< Left parens: '('
	token_par_right,	//< Right parens: ')'
	token_eof			//< End of policy string.
	} token_type;

class TParser : public TLex
	/**
	* Parser of the policy definition.
	*/
	{
public:
	TParser(CSecurityPolicy *aSP, const TDesC &aPolicy, REndPoints &aEp);
	void ParseL(TUint aStartOffset);
private:
	void ParseEndPointL();
	void SetAddressOrEndPointL(RIpAddress &aAddr, TInt aMask, TInt aError);
	void ParseAddressL(RIpAddress &aAddr, TInt aMask, TInt aError);
	void ParseAddressAndMaskL(RIpAddress &aAddr, RIpAddress& aMask);
	void ParseSecurityBundleL(RPolicyActions &aBundle, CTransportSelector *aTS, CPolicySelector* aPs);//UMA support 
	void ParseSecurityBundleL(RPolicyActions &aBundle, CTransportSelector *aTS);
	void CheckFeatureSupportL(TUid aFeature);
	
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	void ParseAssociationParametersL(CPolicySpec *aSpec);
#else 
	void ParseAssociationParametersL(TSecurityAssocSpec &aSpec);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
	void ParseSelectorL(CPolicySelector *&aPs);
	token_type TransportSelectorL(CTransportSelector *& aTS);
	void ParseAssociationL();
	TAlgorithmMap *ParseAlgorithmReferenceL(TInt anInsert);
	TInt ParseAlgorithmMappingL(TAlgorithmClass aClass);
	token_type NextToken();
	void SkipSpaceAndMark();
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	token_type CheckProposalCloseAndMoreProposals(TInt &aPropBraces);
	CSecurityProposalSpec* CreateProposalL(CPropList& aPropList);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

private:
	TPtrC iToken;			//< The current token.
	CSecurityPolicy *iSp;	//< The result of the parsing operation, The new policy
	REndPoints &iEp;		//< The End Point collection to use for the named endpoints.
	TBool iIPSecGANSupported; //To check whether FF_IPSEC_UMA_SUPPORT_ENABLE is defined and UMA supported
	};

#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
CSecurityProposalSpec* TParser::CreateProposalL(CPropList& aPropList)
	{
	CSecurityProposalSpec* prop = new(ELeave) CSecurityProposalSpec;
	prop->iType = SADB_SATYPE_UNSPEC;	
	aPropList.AppendL(prop);
	return prop;
	}

token_type TParser::CheckProposalCloseAndMoreProposals(TInt &aPropBraces)
	{
	token_type val;
	val=NextToken();
	if (val == token_brace_right )
		{
		if (--aPropBraces != -1)
		return NextToken();
		}
	return val;
	}
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
void TParser::SkipSpaceAndMark()
	/**
	* Skip white space and mark.
	*
	* Skip white space and comments. The '#' character
	* starts a comment that continues to the end of the line.
	* Syntactically, a comment is white space  (and terminates
	* token string.
	*
	* Set the mark on first character that is not white space.
	*/
	{
	TInt comment = 0;
	while (!Eos())
		{
		const TChar ch = Get();
		if (ch == '\n')
			comment = 0;
		else if (comment || ch == '#')
			comment = 1;
		else if (!ch.IsSpace())
			{
			UnGet();
			break;
			}
		}
	Mark();
	}

token_type TParser::NextToken()
	/**
	* Parse over the next token.
	*
	* Skip over white space and return the next token. The
	* parsing point is after the returned token. Set the
	* iToken to reference the returned token as a string.
	*
	* @return The token type
	*/
	{
	token_type val;

	SkipSpaceAndMark();
	if (Eos())
		val = token_eof;
	else
		{
		TChar ch = Get();
		if (ch == '{')
			val = token_brace_left;
		else if (ch == '}')
			val = token_brace_right;
		else if (ch == '(')
			val = token_par_left;
		else if (ch == ')')
			val = token_par_right;
		else if (ch == '=')
			val = token_equal;
		else if (ch == ',')
			val = token_comma;
		else if (ch == '?')
			val = token_question;
		else
			{
			val = token_string;
			while (!Eos())
				{
				ch = Peek();
				if (ch == '{' || ch == '}' ||
					ch == '(' || ch == ')' ||
					ch == '=' || ch == '#' ||
					ch == '?' || ch.IsSpace())
					break;
				Inc();
				}
			}
		}
	iToken.Set(MarkedToken());
	SkipSpaceAndMark();
	return val;
	}

TAlgorithmMap *TParser::ParseAlgorithmReferenceL(TInt aInsert)
	/**
	* Parse algorithm reference
	* @code [ library '.' ] algorithm
	* @endcode
	*
	* @param aInsert Create TAlgorithMap, if not found
	* @return The algorithm map.
	*/
	{
	_LIT(K_null, "null");

	if (NextToken() == token_string)
		{
		TInt dot = iToken.Locate('.');
		const TPtrC lib = dot < 0 ? (TPtrC&)KNullDesC() : iToken.Left(dot);
		TPtrC alg = dot < 0 ? iToken : iToken.Right(iToken.Length()-dot-1);
		if (alg.Compare(K_null) == 0)
			alg.Set(0,0);		// 'null' is a special algorithm name!
		TAlgorithmMap *map = iSp->FindAlg(lib, alg);
		if (map == NULL && aInsert)
			map = iSp->NewAlgL(lib, alg);
		return map;
		}
	// Ás this is syntax error, should probably report something..
	return NULL;
	}


TInt TParser::ParseAlgorithmMappingL(TAlgorithmClass aClass)
	/**
	* Parse algorithm mapping
	* @code ParseAlgorithmReferenceL '(' id ',' bits [',' dummy]* ')'
	* @endcode
	*
	* The library reference is followed by a list of numbers in
	* parentheses. The kernel side only needs one or two (for digest)
	* and rest are ignored (the other numbers may be used by the
	* key management or configuration tools).
	*
	* @param aClass Algorithm type (cipher or digest).
	* @return KErrNone if no errors.
	*/
	{
	TAlgorithmMap *map = ParseAlgorithmReferenceL(1);
	if (map && NextToken() == token_par_left)
		{
		TInt i, n;
		map->iClass = aClass;
		for (i = 0;;++i)
			{
			const TInt error = Val(n);
			if (error != KErrNone)
				return error;
			//
			// Only the first two integers are used by the
			// kernel IPSEC, pass other numbers silently.
			//
			if (i == 0)
				map->iId = n;
			else if (i == 1)
				map->iBits = n;
			
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
			if (map->iId == SADB_AALG_AES_XCBC_MAC)
				{
				map->iClass = EAlgorithmClass_Mac;
				}
#endif	// SYMBIAN_IPSEC_VOIP_SUPPORT			
			const TInt tok = NextToken();
			if (tok == token_par_right)
				break;
			else if (tok != token_comma)
				return KErrGeneral;
			}
		//
		// At least one number (id) must be present
		//
		if (i == 0)
			return KErrGeneral;
		}
	return KErrNone;
	}


void TParser::ParseEndPointL()
	/**
	* Parse Named End Point
	* @code name '=' '{' [ '?' ] ip-address '}'
	* @endcode
	*/
	{
	if (NextToken() != token_string)
		User::Leave(EIpsec_PolicySpecName);	// Endpoint name expeted!
	// Allocate a slot for the new association specification

	const TPtrC name(iToken);

	if (NextToken() != token_equal ||
		NextToken() != token_brace_left)
		User::Leave(EIpsec_PolicySyntaxError);

	TInt opt = 0;
	token_type val = NextToken();
	if (val == token_question)
		{
		opt = 1;
		val = NextToken();
		}
	TIpAddress addr;
	if (val == token_brace_right)
		addr.SetAddressNone();
	else
		{
		 if (val != token_string || addr.SetAddress(iToken) != KErrNone)
			User::Leave(EIpsec_PolicyIpAddressExpected);
		if (NextToken() != token_brace_right)
			User::Leave(EIpsec_PolicyCloseBraceExpected);
		}

	// Add RIpAddress handle to iEndPoints to keep the EP around
	// as long as this policy is loaded (if EP is attached to some
	// other objects, like SA, its life extends beyond the policy
	// life).
	const TInt index = iSp->iEndPoints.Count();
	User::LeaveIfError(iSp->iEndPoints.Append(RIpAddress()));
	User::LeaveIfError(iSp->iEndPoints[index].Open(iEp, name, addr, opt));
	}


#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
void TParser::ParseAssociationParametersL(CPolicySpec *aPolicySpec)
#else
void TParser::ParseAssociationParametersL(TSecurityAssocSpec &aSpec)
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	/**
	* Parse a sequence of association parameters
	* @code [ 'ah' | 'esp' | 'encrypt_alg' number | 'auth_alg' number |
	* 'identity' string | 'identity_remote' string | 'identity_local' string |
	* 'pfs' |
	* 'protocol_specific' | 'local_port_specific' | 'remote_port_specific' |
	* 'proxy_specific' | 'local_specific' | 'remote_specific' |
	* 'src_specific' | 'replay_win_len' number |
	* 'min_auth_bits' number | 'max_auth_bits' number |
	* 'min_encrypt_bits' number | 'max_encrypt_bits' number |
	* 'hard_lifetime_allocations' number | 'hard_lifetime_bytes' number |
	* 'hard_lifetime_addtime' number | 'hard_lifetime_usetime' number |
	* 'soft_lifetime_allocations' number | 'soft_lifetime_bytes' number |
	* 'soft_lifetime_addtime' number | 'soft_lifetime_usetime' number | 'proposal' ]+  '}'
	* @endcode
	*
	* Some parameters are just plain keyword, some have additional numeric
	* or string arguments. The parameter keywords can be in any order, but
	* each keyword can only appear once. At least one 'ah' or 'esp' must be
	* present.
	*
	* @param aSpec The specificantion to fill.
	*/
	{
	static const TKeyword list[] =
		{
		KEYWORD("ah"),
		KEYWORD("esp"),
		KEYWORD("encrypt_alg"),
		KEYWORD("auth_alg"),
		KEYWORD("identity"),
		KEYWORD("identity_remote"),
		KEYWORD("identity_local"),
		KEYWORD("pfs"),
		KEYWORD("protocol_specific"),
		KEYWORD("local_port_specific"),
		KEYWORD("remote_port_specific"),
		KEYWORD("proxy_specific"),	// -- kept for backward compatibility
		KEYWORD("local_specific"),
		KEYWORD("remote_specific"),
		KEYWORD("src_specific"),
		KEYWORD("replay_win_len"),
		KEYWORD("min_auth_bits"),
		KEYWORD("max_auth_bits"),
		KEYWORD("min_encrypt_bits"),
		KEYWORD("max_encrypt_bits"),
		KEYWORD("hard_lifetime_allocations"),
		KEYWORD("hard_lifetime_bytes"),
		KEYWORD("hard_lifetime_addtime"),
		KEYWORD("hard_lifetime_usetime"),
		KEYWORD("soft_lifetime_allocations"),
		KEYWORD("soft_lifetime_bytes"),
		KEYWORD("soft_lifetime_addtime"),
		KEYWORD("soft_lifetime_usetime")
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		,KEYWORD("proposal")
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
		};
	enum	// Order must be exactly the same as above
		{
		KEYENUM(ah),
		KEYENUM(esp),
		KEYENUM(encrypt_alg),
		KEYENUM(auth_alg),
		KEYENUM(identity),
		KEYENUM(identity_remote),
		KEYENUM(identity_local),
		KEYENUM(pfs),
		KEYENUM(protocol_specific),
		KEYENUM(local_port_specific),
		KEYENUM(remote_port_specific),
		KEYENUM(proxy_specific),
		KEYENUM(local_specific),
		KEYENUM(remote_specific),
		KEYENUM(src_specific),
		KEYENUM(replay_win_len),
		KEYENUM(min_auth_bits),
		KEYENUM(max_auth_bits),
		KEYENUM(min_encrypt_bits),
		KEYENUM(max_encrypt_bits),
		KEYENUM(hard_lifetime_allocations),
		KEYENUM(hard_lifetime_bytes),
		KEYENUM(hard_lifetime_addtime),
		KEYENUM(hard_lifetime_usetime),
		KEYENUM(soft_lifetime_allocations),
		KEYENUM(soft_lifetime_bytes),
		KEYENUM(soft_lifetime_addtime),
		KEYENUM(soft_lifetime_usetime),
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		KEYENUM(proposal),
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
		KEYENUM(max_parameters)
		};

	TInt sa_type_defined = 0;
	TInt error = KErrNone;
	token_type val;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	val = NextToken();
	TInt propasalsDefined=0;
	CSecurityProposalSpec* prop = NULL;
	while (val == token_string)
#else
	while ((val = NextToken()) == token_string)
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
		{
		switch (Lookup(list, E_max_parameters, iToken))
			{
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
			case E_proposal: 
				propasalsDefined++;
				prop = CreateProposalL (*aPolicySpec->iPropList);
				if ((val=NextToken()) != token_brace_left)
				{
				User::Leave(EIpsec_PolicySyntaxError);
				}
				break;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
			case E_ah:
				sa_type_defined++;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iType = SADB_SATYPE_AH; 
				if (prop) prop->iType = SADB_SATYPE_AH; 
#else
				aSpec.iType = SADB_SATYPE_AH;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_esp:
				sa_type_defined++;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iType = SADB_SATYPE_ESP;
				if (prop) prop->iType = SADB_SATYPE_ESP;
#else
				aSpec.iType = SADB_SATYPE_ESP;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_encrypt_alg:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop && (error = Val(prop->iEalg, EDecimal)) == KErrNone &&
					iSp->FindAlg(EAlgorithmClass_Cipher, prop->iEalg) == NULL)
					User::Leave(EIpsec_PolicyUnknownEncrypt);
				if(prop)
				aPolicySpec->iSpec.iEalg = prop->iEalg;
#else
				if ((error = Val(aSpec.iEalg, EDecimal)) == KErrNone &&
					iSp->FindAlg(EAlgorithmClass_Cipher, aSpec.iEalg) == NULL)
					User::Leave(EIpsec_PolicyUnknownEncrypt);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_auth_alg:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop && (error = Val(prop->iAalg, EDecimal)) == KErrNone &&
						(iSp->FindAlg(EAlgorithmClass_Digest, prop->iAalg) == NULL)
					&&  (iSp->FindAlg(EAlgorithmClass_Mac, prop->iAalg) == NULL) )
					User::Leave(EIpsec_PolicyUnknownAuth);
				if(prop)
				aPolicySpec->iSpec.iAalg = prop->iAalg ;
#else
				if ((error = Val(aSpec.iAalg, EDecimal)) == KErrNone &&
					iSp->FindAlg(EAlgorithmClass_Digest, aSpec.iAalg) == NULL)
					User::Leave(EIpsec_PolicyUnknownAuth);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_identity:		// temp. backwards compatibility
			case E_identity_remote:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (aPolicySpec->iSpec.iIdentityRemote)
					User::Leave(EIpsec_PolicyIdentityDefined);
				(void)NextToken();	// Advance iToken to the identity value
				aPolicySpec->iSpec.iIdentityRemote = CIdentity::NewL(iToken);
#else
				if (aSpec.iIdentityRemote)
					User::Leave(EIpsec_PolicyIdentityDefined);
				(void)NextToken();	// Advance iToken to the identity value
				aSpec.iIdentityRemote = CIdentity::NewL(iToken);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_identity_local:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (aPolicySpec->iSpec.iIdentityLocal)
					User::Leave(EIpsec_PolicyIdentityDefined);
				(void)NextToken();	// Advance iToken to the identity value
				aPolicySpec->iSpec.iIdentityLocal = CIdentity::NewL(iToken);
#else
				if (aSpec.iIdentityLocal)
					User::Leave(EIpsec_PolicyIdentityDefined);
				(void)NextToken();	// Advance iToken to the identity value
				aSpec.iIdentityLocal = CIdentity::NewL(iToken);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_pfs:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iPfs = 1;
#else
				aSpec.iPfs = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_protocol_specific:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchProtocol = 1;
#else
				aSpec.iMatchProtocol = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_local_port_specific:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchLocalPort = 1;
#else
				aSpec.iMatchLocalPort = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_remote_port_specific:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchRemotePort = 1;
#else
				aSpec.iMatchRemotePort = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_proxy_specific:
				// ** should be deprecated -- msa
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchProxy = 1;
#else
				aSpec.iMatchProxy = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_local_specific:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchLocal = 1;
#else
				aSpec.iMatchLocal = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_remote_specific:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchRemote = 1;
#else
				aSpec.iMatchRemote = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_src_specific:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				aPolicySpec->iSpec.iMatchSrc = 1;
#else
				aSpec.iMatchSrc = 1;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_replay_win_len:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				error = Val(aPolicySpec->iSpec.iReplayWindowLength, EDecimal);
#else
				error = Val(aSpec.iReplayWindowLength, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_min_auth_bits:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				error = Val(aPolicySpec->iSpec.iMinAuthBits, EDecimal);
#else
				error = Val(aSpec.iMinAuthBits, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_max_auth_bits:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop) 
					{
					error = Val(prop->iMaxAuthBits, EDecimal);
					}
#else
				error = Val(aSpec.iMaxAuthBits, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_min_encrypt_bits:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iMinEncryptBits, EDecimal);
					}
#else
				error = Val(aSpec.iMinEncryptBits, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_max_encrypt_bits:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iMaxEncryptBits, EDecimal);
					}
#else
				error = Val(aSpec.iMaxEncryptBits, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_hard_lifetime_allocations:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iHard.sadb_lifetime_allocations , EDecimal);
					}
#else
				error = Val(aSpec.iHard.sadb_lifetime_allocations, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_hard_lifetime_bytes:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iHard.sadb_lifetime_bytes , EDecimal);
					}
#else
				error = Val(aSpec.iHard.sadb_lifetime_bytes, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_hard_lifetime_addtime:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iHard.sadb_lifetime_addtime , EDecimal);
					}
#else
				error = Val(aSpec.iHard.sadb_lifetime_addtime, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_hard_lifetime_usetime:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iHard.sadb_lifetime_usetime , EDecimal);
					}
#else
				error = Val(aSpec.iHard.sadb_lifetime_usetime, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_soft_lifetime_allocations:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iSoft.sadb_lifetime_allocations , EDecimal);
					}
#else
				error = Val(aSpec.iSoft.sadb_lifetime_allocations, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_soft_lifetime_bytes:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iSoft.sadb_lifetime_bytes , EDecimal);
					}
#else
				error = Val(aSpec.iSoft.sadb_lifetime_bytes, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_soft_lifetime_addtime:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iSoft.sadb_lifetime_addtime , EDecimal);
					}
#else
				error = Val(aSpec.iSoft.sadb_lifetime_addtime, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
			case E_soft_lifetime_usetime:
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				if (prop)
					{
					error = Val(prop->iSoft.sadb_lifetime_usetime , EDecimal);
					}
#else
				error = Val(aSpec.iSoft.sadb_lifetime_usetime, EDecimal);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
				break;
				
			default:
				User::Leave(EIpsec_PolicyUnknownSpec);
			}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		val = CheckProposalCloseAndMoreProposals(propasalsDefined);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
		if (error != KErrNone)
			User::Leave(EIpsec_PolicyNumberExpected);
		}
	if (val != token_brace_right)
		User::Leave(EIpsec_PolicyCloseBraceExpected);
	else if (sa_type_defined < 1)
		User::Leave(EIpsec_PolicyNoType);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	else if ((aPolicySpec->iSpec.iType == SADB_SATYPE_AH) && !aPolicySpec->iSpec.iAalg)
		User::Leave(EIpsec_PolicyNoAuthAlgorithm);
	else if ((aPolicySpec->iSpec.iType == SADB_SATYPE_ESP) && !aPolicySpec->iSpec.iEalg)
		User::Leave(EIpsec_PolicyNoEncryptAlgorithm);
#else
	else if (sa_type_defined > 1)
		User::Leave(EIpsec_PolicyTooManyTypes);
	else if ((aSpec.iType == SADB_SATYPE_AH) && !aSpec.iAalg)
		User::Leave(EIpsec_PolicyNoAuthAlgorithm);
	else if ((aSpec.iType == SADB_SATYPE_ESP) && !aSpec.iEalg)
		User::Leave(EIpsec_PolicyNoEncryptAlgorithm);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
	}


void TParser::ParseAssociationL()
	/**
	* Parse security association
	* @code name '=' '{' ParseAssociationParametersL
	* @endcode
	*/
	{
	if (NextToken() != token_string)
		User::Leave(EIpsec_PolicySpecName);
	// Allocate a slot for the new association specification
    CPolicySpec *const ps = new (ELeave) CPolicySpec();
	if (iSp->iSpecs.Append(ps) != KErrNone)
		{
		ps->Close();
		User::Leave(KErrNoMemory);
		}
	ps->iName = HBufC::NewMaxL(iToken.Length());
	*ps->iName = iToken;

	if (NextToken() != token_equal ||
		NextToken() != token_brace_left)
		User::Leave(EIpsec_PolicySyntaxError);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	ParseAssociationParametersL(ps);
#else
	ParseAssociationParametersL(ps->iSpec);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	}

/**
* Parse security actions
* @code [ sa-name '(' [ address ] ')' ]* '}'
* @endcode
*
* Parse a (possibly empty) list of references to security specifications. This will
* be the bundle of security actions for a selector.
*
* @retval aActions The colleted actions
* @param aTS The traffic selector.
* @param aPS the policy selector
* * This is called only when UMA/ GAN is supported.
*/
void TParser::ParseSecurityBundleL(RPolicyActions &aActions, CTransportSelector *aTS, CPolicySelector *aPs)
    {
    
    LOG(Log::Printf(_L("TParser::ParseSecurityBundleL(RPolicyActions &aActions, CTransportSelector *aTS, CPolicySelector *aPs)")));
          
    if(iIPSecGANSupported)
        {
        LOG(Log::Printf(_L("TParser::ParseSecurityBundleL: UMA supported FF_IPSEC_UMA_SUPPORT_ENABLE defined")));
        }
    else
        {
        LOG(Log::Printf(_L("TParser::ParseSecurityBundleL:functionality not suppoted.FF_IPSEC_UMA_SUPPORT_ENABLE not defined")));
        User::Leave(KErrNotSupported);
        }
    
    _LIT(K_tunnel,  "tunnel");

    token_type val;

    TUint opt = 0;
    _LIT(K_Exception,  "UMAException");//UMA exception defined
    for (;;)
        {
        val = NextToken();
        //
        // Experimental addition, allow optional bundle items
        // by prefixing them with '?'...
        //
        if (opt == 0 && val == token_question)
            {
            opt = 1;
            continue;
            }
        else if (val != token_string)
            break;
        // A temporary(?) special kluge: if the keyword is 'tunnel'
        // assume this is a plain tunnel specification, without any
        // relation to the IPSEC. if nobody defined a "tunnel" sa
        // specification. (should probably disallow 'tunnel' as SA
        // spec name, to avoid confusion.. )
        //
        CPolicySpec *spec = iSp->FindSpec(iToken);

        if(spec== NULL && !iToken.Compare(K_Exception))
            {
            LOG(Log::Printf(_L("Found Exception Policy identifier")));
            //FInd Next token... Things looks hacky here. IPsec really need re-designing.
            TInt tokenVal = (TInt)NextToken();
            LOG(Log::Printf(_L("NextToken value is = [%d]"),tokenVal));
            
            TBuf8<32> buf;
            buf.Copy(iToken);
            TLex8 lex(buf);
            TInt scope;
            lex.Val(scope);
            
            //assiging scope to the policy. This will be policy selector with Exception scope being setalong with
            //exception flags
            aPs->iScope_Exception = scope;
            LOG(Log::Printf(_L("TParser::ParseSecurityBundleL, Exception tunnel Scope is = [%d]"),scope));
            while((val = NextToken())!= token_brace_right)
                {
                //do nothing 
                }  //while    
            break;
            }

        // A temporary(?) special kluge: if the keyword is 'tunnel'
        // assume this is a plain tunnel specification, without any
        // relation to the IPSEC. if nobody defined a "tunnel" sa
        // specification. (should probably disallow 'tunnel' as SA
        // spec name, to avoid confusion.. )
        //
        if (spec == NULL && iToken.Compare(K_tunnel) != 0)
            User::Leave(EIpsec_PolicySpecNotFound);

        if (NextToken() != token_par_left)
            User::Leave(EIpsec_PolicyLeftParen);

        CPolicyAction *action = new (ELeave) CPolicyAction;
        if (aActions.Append(action) != KErrNone)
            {
            action->Close();
            User::Leave(KErrNoMemory);
            }
        if ((action->iSpec = spec) != NULL)
            spec->Open();
        // Record the current selector into each action (this is to make it
        // easier to generate the TS list into Acquire message).
        if ((action->iTS = aTS) != NULL)
            aTS->Open();
        action->iOptional = opt;

        if ((val = NextToken()) == token_string)
            {
            SetAddressOrEndPointL(action->iTunnel, 0, EIpsec_PolicyInvalidIpAddress);
            action->iIsTunnel = 1;  // Flag a tunnel.
            val = NextToken();
            }
        if (val != token_par_right)
            User::Leave(EIpsec_PolicyRightParen);
        opt = 0;        // Optional only affects single item at time.
        }
    if (val != token_brace_right)
        User::Leave(EIpsec_PolicyCloseBraceExpected);
    }

/**
* Parse security actions
* @code [ sa-name '(' [ address ] ')' ]* '}'
* @endcode
*
* Parse a (possibly empty) list of references to security specifications. This will
* be the bundle of security actions for a selector.
*
* @retval aActions The colleted actions
* @param aTS The traffic selector.
* This is called in case of No UMA/ GAN support.
*/ 
void TParser::ParseSecurityBundleL(RPolicyActions &aActions, CTransportSelector *aTS)
//#endif
	{
	_LIT(K_tunnel,	"tunnel");

	token_type val;

	TUint opt = 0;

	for (;;)
		{
		val = NextToken();
		//
		// Experimental addition, allow optional bundle items
		// by prefixing them with '?'...
		//
		if (opt == 0 && val == token_question)
			{
			opt = 1;
			continue;
			}
		else if (val != token_string)
			break;
		// A temporary(?) special kluge: if the keyword is 'tunnel'
		// assume this is a plain tunnel specification, without any
		// relation to the IPSEC. if nobody defined a "tunnel" sa
		// specification. (should probably disallow 'tunnel' as SA
		// spec name, to avoid confusion.. )
		//
		CPolicySpec *spec = iSp->FindSpec(iToken);

        // A temporary(?) special kluge: if the keyword is 'tunnel'
        // assume this is a plain tunnel specification, without any
        // relation to the IPSEC. if nobody defined a "tunnel" sa
        // specification. (should probably disallow 'tunnel' as SA
        // spec name, to avoid confusion.. )
        //
		if (spec == NULL && iToken.Compare(K_tunnel) != 0)
			User::Leave(EIpsec_PolicySpecNotFound);

		if (NextToken() != token_par_left)
			User::Leave(EIpsec_PolicyLeftParen);

		CPolicyAction *action = new (ELeave) CPolicyAction;
		if (aActions.Append(action) != KErrNone)
			{
			action->Close();
			User::Leave(KErrNoMemory);
			}
		if ((action->iSpec = spec) != NULL)
			spec->Open();
		// Record the current selector into each action (this is to make it
		// easier to generate the TS list into Acquire message).
		if ((action->iTS = aTS) != NULL)
			aTS->Open();
		action->iOptional = opt;

		if ((val = NextToken()) == token_string)
			{
			SetAddressOrEndPointL(action->iTunnel, 0, EIpsec_PolicyInvalidIpAddress);
			action->iIsTunnel = 1;	// Flag a tunnel.
			val = NextToken();
			}
		if (val != token_par_right)
			User::Leave(EIpsec_PolicyRightParen);
		opt = 0;		// Optional only affects single item at time.
		}
	if (val != token_brace_right)
		User::Leave(EIpsec_PolicyCloseBraceExpected);
	}


void TParser::SetAddressOrEndPointL(RIpAddress &aAddr, TInt aMask, TInt aError)
	/**
	* Initiliaze address from current token.
	*
	* The token can contain direct address literal or a name of an end point.
	* If a name is used, it must exist.
	*
	* @retval aAddr The address to initialize.
	* @param aMask Non-zero, if address is used as mask.
	* @param aError The error to use if end point does not exist (leave)
	*/
	{
	TIpAddress addr;
	if (addr.SetAddress(iToken, aMask) == KErrNone)
		{
		// plain address
		User::LeaveIfError(aAddr.Open(iEp, addr));
		}
	else if (aAddr.Open(iEp, iToken) != KErrNone)
		{
		// Named endpoint not found
		User::Leave(aError);
		}
	}

void TParser::ParseAddressL(RIpAddress &aAddr, TInt aMask, TInt aError)
	/**
	* Parse next token as address.
	* @code address
	* @endcode
	*
	* @retval aAddr The address to initialize
	* @param aMask Non-zero, if address is used as mask.
	* @param aError The error to use, if error
	*/
	{
	if (NextToken() != token_string)
		{
		User::Leave(aError);
		}
	else
		{
		SetAddressOrEndPointL(aAddr, aMask, aError);
		}
	}


void TParser::ParseAddressAndMaskL(RIpAddress &aAddr, RIpAddress& aMask)
	/**
	* Parse two tokens as addres and address mask
	* @code ParseAddressL ParseAddressL
	* @endcode
	*
	* @param aAddr The address to initialize
	* @param aMask The mask to initialize
	*/
	{
	ParseAddressL(aAddr, 0, EIpsec_PolicyIpAddressExpected);
	ParseAddressL(aMask, 1, EIpsec_PolicyIpMaskExpected);
	}

token_type TParser::TransportSelectorL(CTransportSelector *&aTs)
	/**
	* Parse one transport selector.
	* @code [
	* 'remote' ParseAddressAndMaskL | 'local' ParseAddressAndMaskL |
	* 'user_id' | 'protocol' number |
	* 'local_port' number | 'remote_port' number |
	* 'icmp_type' number | 'type' number | 'icmp_code' number ]*
	* @endcode
	*/
	{
	static const TKeyword list[] =
		{
		KEYWORD("remote"),
		KEYWORD("local"),
		KEYWORD("user_id"),
		KEYWORD("protocol"),
		KEYWORD("local_port"),
		KEYWORD("remote_port"),
		KEYWORD("icmp_type"),
		KEYWORD("icmp_code"),
		KEYWORD("type"),
		};

	enum	// Order must be exactly the same as above
		{
		KEYENUM(remote),
		KEYENUM(local),
		KEYENUM(user_id),
		KEYENUM(protocol),
		KEYENUM(local_port),
		KEYENUM(remote_port),
		KEYENUM(icmp_type),
		KEYENUM(icmp_code),
		KEYENUM(type),			// synonym for "icmp_type" (use for MH and similar)

		KEYENUM(max_parameters)
		};
	
	token_type val;
	//
	// By default, a new selector will match everything (mask is all zero)
	//
	RPolicySelectorInfo data;
	RPolicySelectorInfo mask;
	data.FillZ();
	mask.FillZ();

#if OLD_SELECTOR_ORDERING
	// If selector already exists, continue filling it.
	if (aTs != NULL)
		{
		data = aTs->iData;
		mask = aTs->iMask;
		aTs->Close();
		aTs = NULL;
		}
#endif

	do
		{
		TUint8 tmp8;

		switch (Lookup(list, E_max_parameters, iToken))
			{
			case E_remote:
				ParseAddressAndMaskL(data.iRemote, mask.iRemote);
				break;
			case E_local:
				ParseAddressAndMaskL(data.iLocal, mask.iLocal);
				break;
			case E_user_id:
				break;	// Needs to be examined, TIdentity? -- msa
			case E_protocol:
				mask.iProtocol = 0xFF;
				User::LeaveIfError(Val(data.iProtocol, EDecimal));
				break;
			case E_local_port:
				if (mask.iPortLocal)
					User::Leave(EIpsec_PolicySyntaxError);	// Already defined (or type/code)
				mask.iPortLocal = 0xFFFF;
				User::LeaveIfError(Val(data.iPortLocal, EDecimal));
				mask.iFlags |= KTransportSelector_PORTS;
				data.iFlags |= KTransportSelector_PORTS;
				break;
			case E_remote_port:
				if (mask.iPortRemote)
					User::Leave(EIpsec_PolicySyntaxError);	// Already defined
				mask.iPortRemote = ~0;
				User::LeaveIfError(Val(data.iPortRemote, EDecimal));
				mask.iFlags |= KTransportSelector_PORTS;
				data.iFlags |= KTransportSelector_PORTS;
				break;
			case E_icmp_type:
			case E_type:
				if ((mask.iPortLocal | mask.iPortRemote) & 0xFF00)
					User::Leave(EIpsec_PolicySyntaxError);	// Already defined (or port used)
				mask.iPortLocal |= 0xFF00;
				User::LeaveIfError(Val(tmp8, EDecimal));
				data.iPortLocal |= tmp8 << 8;
				// For Type/Code remote port selector is always same as local port value
				mask.iPortRemote = mask.iPortLocal;
				data.iPortRemote = data.iPortLocal;
				mask.iFlags |= KTransportSelector_PORTS;
				data.iFlags &= ~KTransportSelector_PORTS;
				break;
			case E_icmp_code:
				if ((mask.iPortLocal | mask.iPortRemote) & 0x00FF)
					User::Leave(EIpsec_PolicySyntaxError);	// Already defined (or port used)
				mask.iPortLocal |= 0x00FF;
				User::LeaveIfError(Val(tmp8, EDecimal));
				data.iPortLocal |= tmp8;
				// For Type/Code remote port selector is always same as local port value
				mask.iPortRemote = mask.iPortLocal;
				data.iPortRemote = data.iPortLocal;
				mask.iFlags |= KTransportSelector_PORTS;
				data.iFlags &= ~KTransportSelector_PORTS;
				break;
			default:
#if OLD_SELECTOR_ORDERING
				val = token_string;
				goto wrapup;
#else
				User::Leave(EIpsec_PolicyUnknownSelector);
#endif
			}
		}
	while
		((val = NextToken()) == token_string);
#if OLD_SELECTOR_ORDERING
wrapup:
#endif
	aTs = new (ELeave) CTransportSelector(data, mask, NULL);
	return val;
	}

void TParser::ParseSelectorL(CPolicySelector *&aPs)
	/**
	* Parse a policy selector
	* @code [ 'final' | 'merge' | 'outbound' | 'inbound' | 'if' interface ]*
	*	 TransportSelectorL '=' ( '{' ParseSecurityBundleL | 'drop' )
	* @endcode
	*/
	{
	static const TKeyword list[] =
		{
		KEYWORD("final"),
		KEYWORD("merge"),
		KEYWORD("outbound"),
		KEYWORD("inbound"),
		KEYWORD("UMAExceptionTrafficSelector"), //UMA support
		KEYWORD("if"),
		};

	enum	// Order must be exactly the same as above
		{
		KEYENUM(final),
		KEYENUM(merge),
		KEYENUM(outbound),
		KEYENUM(inbound),
		KEYENUM(UMAExceptionTrafficSelector),//exception bits
		KEYENUM(if),

		KEYENUM(max_parameters)
		};

	_LIT(K_drop, "drop");

	token_type val;

    CheckFeatureSupportL(NFeature::KFeatureIdFfIpsecUmaSupportEnable);

	//
	aPs = new (ELeave) CPolicySelector();

	//
	// ...by default the "merge" must be off to match.
	//
	aPs->iFilterMask = KPolicyFilter_MERGE;

	do
		{
#if OLD_SELECTOR_ORDERING
	check_again:
#endif
		switch (Lookup(list, E_max_parameters, iToken))
			{
			case E_final:
				if (aPs->iFilterData & KPolicyFilter_FINAL)
					User::Leave(EIpsec_PolicySyntaxError);	// Duplicate "final" keyword.
				// No need to set "mask", this is a flag only.
				aPs->iFilterData |= KPolicyFilter_FINAL;
				break;
			case E_merge:
				if ((aPs->iFilterMask & KPolicyFilter_MERGE) == 0)
					User::Leave(EIpsec_PolicySyntaxError);	// Duplicate "merge" keyword.
				aPs->iFilterMask &= ~KPolicyFilter_MERGE;	// allow this selector to be merged.
				break;
			case E_outbound:
				if (aPs->iFilterData & KPolicyFilter_SYMMETRIC)
					User::Leave(EIpsec_PolicyInboundOutbound);
				aPs->iFilterData |= KPolicyFilter_OUTBOUND;
				aPs->iFilterMask |= KPolicyFilter_OUTBOUND;
				break;
			case E_inbound:
				if (aPs->iFilterData & KPolicyFilter_SYMMETRIC)
					User::Leave(EIpsec_PolicyInboundOutbound);
				aPs->iFilterData |= KPolicyFilter_INBOUND;
				aPs->iFilterMask |= KPolicyFilter_INBOUND;
				break;
			case E_if:
				if (aPs->iInterface || NextToken() != token_string)
					User::Leave(EIpsec_PolicySyntaxError);	// <-- need own error code?
				aPs->iInterface = iSp->LookupInterfaceL(iToken);
				break;
			case E_UMAExceptionTrafficSelector:
                    if(iIPSecGANSupported)
                        {
                        //UMA support
                        LOG(Log::Printf(_L("TParser::ParseSelectorL Setting Exception selector flag")));
                   //The flags signifies special case for UMA/exception selectors. These selectors
                   //will be present in case when there is no inbound and bypass filter data or selectors
                   //are set. This selector will only allow traffic whose scope match the exception scope 
                        aPs->iFilterData|=KPolicyFilter_Exception;
                        aPs->iFilterMask |= KPolicyFilter_Exception;
                        }
                    else
                        {
                        LOG(Log::Printf(_L("TParser::ParseSelectorL error GAN/ UMA feature is not enabled ")));
                        }
	               break;				
			default:
				val = TransportSelectorL(aPs->iTS);
#if OLD_SELECTOR_ORDERING
				if (Lookup(list, E_max_parameters, iToken) < E_max_parameters)
					goto check_again;
#endif
				goto wrapup;
			}
		}
	while
		((val = NextToken()) == token_string);
wrapup:

	if (val != token_equal)
		User::Leave(EIpsec_PolicySyntaxError);
	if (NextToken() == token_brace_left)
	    {
	  //UMA support RE417-40027
        if(iIPSecGANSupported)
            {
        ParseSecurityBundleL(aPs->iActions, aPs->iTS, aPs);
            }
        else
            {
        ParseSecurityBundleL(aPs->iActions, aPs->iTS);

            }
	    }
	else if (iToken.Compare(K_drop) == 0)
		aPs->iFilterData |= KPolicyFilter_DROP;
	else
		User::Leave(EIpsec_PolicySyntaxError);
	}

#ifdef __VC32__
#pragma warning(disable : 4097) // typedef-name used as synonym for class-name
#endif

TParser::TParser(CSecurityPolicy *aSp, const TDesC &aPolicy, REndPoints &aEp) :
	TLex(aPolicy), iSp(aSp), iEp(aEp)
	{
    
	}
/**
 * To check the feature support
 */
void TParser::CheckFeatureSupportL(TUid aFeature)
    {
    // Check Gan support from feature manager
    iIPSecGANSupported = CFeatureDiscovery::IsFeatureSupportedL(aFeature);
    
    if(iIPSecGANSupported != (TInt)ETrue)
		{
		LOG(Log::Printf(_L("TParser::CheckFeatureSupport Error Checking Feature Support")));
		}
		else
		{
		LOG(Log::Printf(_L("TParser::CheckFeatureSupport %d Feature Supported %d"),aFeature,iIPSecGANSupported));
		}
	}

void TParser::ParseL(TUint aStartOffset)
	/**
	* Parse complete security policy
	* @code (
	* 'sa' ParseAssociationL |
	* 'encrypt' ParseAlgorithmMappingL |
	* 'auth' ParseAlgorithmMappingL |
	* 'ep' ParseEndPointL )*
	* @endcode
	*
	* @param aStartOffset Starting offset of the actual policy
	* @leave error on syntax or other error.
	*/
	{
	static const TKeyword list[] =
		{
		KEYWORD("sa"),
		KEYWORD("encrypt"),
		KEYWORD("auth"),
		KEYWORD("ep"),
		};
	enum
		{
		KEYENUM(sa),
		KEYENUM(encrypt),
		KEYENUM(auth),
		KEYENUM(ep),

		KEYENUM(max_list)
		};

	CPolicySelector **last = &iSp->iSelectors;

	Inc(aStartOffset);
	while (NextToken() == token_string)
		{
		switch (Lookup(list, E_max_list, iToken))
			{
			case E_sa:
				ParseAssociationL();
				break;
			case E_encrypt:
				ParseAlgorithmMappingL(EAlgorithmClass_Cipher);
				break;
			case E_auth:
				ParseAlgorithmMappingL(EAlgorithmClass_Digest);
				break;
			case E_ep:
				ParseEndPointL();
				break;
			default:
				ParseSelectorL(*last);
				if (*last)
					last = &(*last)->iNext;
				break;
			}
		}
	if (!Eos())
		// Parsing didn't detect error, but not all parsed!
		User::Leave(EIpsec_PolicySyntaxError);
	}

	
CSecurityPolicy::CSecurityPolicy()
	/**
	* Constructor.
	*/
	{
	IPSEC_OBJECT_INC;
	}


TInt CSecurityPolicy::SetPolicy(CSecurityPolicy * &aPolicy, const TDesC &aNewPolicy, TUint &aOffset, REndPoints &aEp)
	/**
	* Construct a new binary policy and replace a the current with it.
	*
	* @retval aPolicy	Holder of the current binary policy
	* @param aNewPolicy	The new policy definition (text string)
	* @retval aOffset	The initial start, and final point in parsing
	* @param aEp		The End point collection to store the named endpoints
	*
	* @return KErrNone on success, and some < 0 error code otherwise.
	*/
	{
	delete aPolicy;
	aPolicy = new CSecurityPolicy();
	if (aPolicy == NULL)
		return KErrNoMemory;
	
	TParser parser(aPolicy, aNewPolicy, aEp);
	aPolicy->iAlgorithms = new CAlgorithmList;
	if (aPolicy->iAlgorithms == NULL)
		return KErrNoMemory;
	TRAPD(result, parser.ParseL(aOffset); );
	aOffset = parser.MarkedOffset();
	return result;
	}

CSecurityPolicy::~CSecurityPolicy()
	/**
	* Descructor.
	*/
	{
	// release association descriptions/templates
	for (TInt i = iSpecs.Count(); --i >= 0; )
		{
		CPolicySpec *const s = iSpecs[i];
		if (s)
			s->Close();
		}
	iSpecs.Close();

	// release End Points assocated with this policy
	for (TInt j = iEndPoints.Count(); --j >= 0; )
		{
		iEndPoints[j].Close();
		}
	iEndPoints.Close();

	// release the policy selectors
	CPolicySelector *ps;
	while ((ps = iSelectors) != NULL)
		{
		iSelectors = ps->iNext;
		delete ps;
		}

	// release the named interface entries
	while (iInterfaces)
		{
		CSelectorInterface *tmp = iInterfaces;
		iInterfaces = tmp->iNext;
		delete tmp;
		}

	// ..and algorithms table
	delete iAlgorithms;
	IPSEC_OBJECT_DEC;
	}

CPolicySelector::CPolicySelector()
	/**
	* Constructor
	*/
	{
	IPSEC_OBJECT_INC;
	}

CPolicySelector::~CPolicySelector()
	/**
	* Descructor.
	*/
	{

	// Release policy actions
	for (TInt i = iActions.Count(); --i >= 0; )
		{
		CPolicyAction *const action = iActions[i];
		if (action)
			action->Close();
		}
	iActions.Close();

	// Release transport selector(s)
	if (iTS)
		iTS->Close();
	
	IPSEC_OBJECT_DEC;
	}

//
CSelectorInterface::CSelectorInterface()
	/**
	* Constructor
	*/
	{
	IPSEC_OBJECT_INC;
	}

CSelectorInterface::~CSelectorInterface()
	/**
	* Descructor.
	*/
	{
	delete iName;
	IPSEC_OBJECT_DEC;
	}
	
const CSelectorInterface *CSecurityPolicy::LookupInterfaceL(const TDesC &aName)
	/**
	* Locate interface by name from policy.
	*
	* Return an existing interface object, if found and otherwise,
	* create and initialize new object.
	*
	* @param aName The interface name
	* @return The interface
	* @leave KErrNoMemory, if allocation fails.
	*/
	{
	CSelectorInterface *si;
	//
	// See if interface already exists
	//
	for (si = iInterfaces; si != NULL; si = si->iNext)
		if (aName.Compare(*si->iName) == 0)
			return si;	// Already exists, reuse
	//
	// First time occurrence of this interface name, create a new entry
	//
	si = new (ELeave) CSelectorInterface();
	si->iNext = iInterfaces;
	iInterfaces = si;
	si->iName = HBufC::NewMaxL(aName.Length());
	*si->iName = aName;
	return si;
	}

CPolicySpec *CSecurityPolicy::FindSpec(const TDesC &aName) const
	/**
	* Locate security association specification by name.
	*
	* @param aName
	* @return The specification or NULL, if not found
	*
	*/
	{
	for (TInt i = iSpecs.Count(); i > 0; )
		{
		CPolicySpec *const s = iSpecs[--i];
		if (aName.Compare(*s->iName) == 0)
			return s;
		}
	return NULL;
	}

//

CPolicyAction::~CPolicyAction()
	/**
	* Desctructor.
	*/
	{
	if (iSpec)
		iSpec->Close();
	if (iTS)
		iTS->Close();
	}

//

CPolicySpec::CPolicySpec()
    {
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
    iPropList = CPropList::NewL(1);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
    }

	
CPolicySpec::~CPolicySpec()
	/**
	* Destructor.
	*/
	{
	delete iName;
	//
	// Identity blocks are reference counted, there
	// may still be SA's referring to them
	//
	if (iSpec.iIdentityLocal)
		iSpec.iIdentityLocal->Close();
	if (iSpec.iIdentityRemote)
		iSpec.iIdentityRemote->Close();
	}
