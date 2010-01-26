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
//

#include "pfqosparser.h"
#include "qosvariables.h"
#include "pfqosliblog.h"

_LIT(Ksyntax_error, "Syntax error");

EXPORT_C TVariableBase::~TVariableBase()
	{
	}


EXPORT_C TIntVariable::TIntVariable(const TDesC& aName, TInt aValue) 
	: iValue(aValue)
	{
	//??This makes no check against overflow -- either check or use TName&
	//??instead of of TDesC& in parameter list.
	iName.Copy(aName);
	iType = KPfqosTypeInteger;
	}

EXPORT_C TRealVariable::TRealVariable(const TDesC& aName, TReal aValue) 
	: iValue(aValue)
	{
	//??This makes no check against overflow -- either check or use TName&
	//??instead of of TDesC& in parameter list.
	iName.Copy(aName);
	iType = KPfqosTypeReal;
	}

EXPORT_C TStringVariable::TStringVariable(const TDesC& aName, 
										  const TDesC& aValue)
	{
	//??This makes no check against overflow -- either check or use TName&
	//??instead of of TDesC& in parameter list.
	iName.Copy(aName);
	//??This makes no check against overflow -- either check or use TName&
	//??instead of of TDesC& in parameter list.
	iValue.Copy(aValue);
	iType = KPfqosTypeString;
	}


//
// CSelectorBase
//
EXPORT_C CSelectorBase::CSelectorBase(TUint aType) 
	: iType(aType)
	{
	iProtocol = 0;
	iPolicyOptions = 0;
	TUidType uid_type(TUid::Uid(0), TUid::Uid(0), TUid::Uid(0));
	iUid.Set(uid_type);
	iIapId = 0;
	iSrcPortMax = 0;
	iDstPortMax = 0;
	iDst.SetAddress(KInet6AddrNone);
	iSrc.SetAddress(KInet6AddrNone);
	iSrcMask.SetAddress(KInet6AddrNone);
	iDstMask.SetAddress(KInet6AddrNone);
	iName.FillZ();
	iOwner = 0;
	}

//lint -e{1538}	It's safe to copy CBase (I think...?)
EXPORT_C CSelectorBase::CSelectorBase(CSelectorBase& aSel)
	{
	iDstPortMax  = aSel.iDstPortMax;
	iDst = aSel.iDst;
	iDstMask = aSel.iDstMask;
	iSrcPortMax = aSel.iSrcPortMax;
	iSrc = aSel.iSrc;
	iSrcMask = aSel.iSrcMask;
	iProtocol = aSel.iProtocol;
	iUid.Set(aSel.iUid.UidType());
	iIapId = aSel.iIapId;
	iPolicyOptions = aSel.iPolicyOptions;
	iPriority = aSel.iPriority;
	iType = aSel.iType;
	iName.Copy(aSel.iName);
	iOwner = 0;
	}


EXPORT_C CSelectorBase::CSelectorBase(TPfqosBase& aBase, 
									  TPfqosSelector& aSel, 
									  TPfqosAddress& aSrc, 
									  TPfqosAddress& aDst, 
									  TUint aType) : iType(aType)
	{
	iProtocol = (TUint8)aSel.iExt->protocol;
	TCheckedUid uid;
	aSel.GetUid(uid);
	iUid.Set(uid.UidType());
	iIapId = aSel.iExt->iap_id;
	iPolicyOptions = aBase.iMsg->pfqos_msg_options;
	iPriority = aSel.iExt->priority;
	TPtrC8 tmp((TUint8*)aSel.iExt->name);
	iName.Copy(tmp);

	if (aSrc.iAddr != NULL)
		{
		iSrc = *aSrc.iAddr;
		iSrcMask = *aSrc.iPrefix;

		if (aSrc.iExt->pfqos_port_max > iSrc.Port() && 
			aSrc.iExt->pfqos_port_max < 65535)
			{
			iSrcPortMax = aSrc.iExt->pfqos_port_max;
			}
		else
			{
			iSrcPortMax = (TUint16)iSrc.Port();
			}
		}
	else
		{
		iSrc.SetAddress(KInet6AddrNone);
		iSrcMask.SetAddress(KInet6AddrNone);
		}

	if (aDst.iAddr != NULL)
		{
		iDst = *aDst.iAddr;
		iDstMask = *aDst.iPrefix;

		if (aDst.iExt->pfqos_port_max > iDst.Port() && 
			aDst.iExt->pfqos_port_max < 65535)
			{
			iDstPortMax = aDst.iExt->pfqos_port_max;
			}
		else
			{
			iDstPortMax = (TUint16)iDst.Port();
			}
		}
	else
		{
		iDst.SetAddress(KInet6AddrNone);
		iDstMask.SetAddress(KInet6AddrNone);
		}
	iOwner = 0;
	}

EXPORT_C CSelectorBase::~CSelectorBase()
	{
	}


//
// CExtensionPolicy
//
EXPORT_C CExtensionPolicy::CExtensionPolicy(TPfqosBase& aBase, 
											TPfqosSelector& aSel, 
											TPfqosAddress& aSrc, 
											TPfqosAddress& aDst, 
											TInt aType) 
	: CSelectorBase(aBase, aSel, aSrc, aDst, aType)
	{
	iExtensions.SetOffset(_FOFF(CExtension, iNext));
	};

EXPORT_C CExtensionPolicy::CExtensionPolicy() 
	: CSelectorBase(EPfqosExtensionPolicy)
	{
	iExtensions.SetOffset(_FOFF(CExtension, iNext));
	iType = EPfqosExtensionPolicy;
	}

EXPORT_C CExtensionPolicy::~CExtensionPolicy()
	{
	TExtensionQueueIter iter(iExtensions);
	CExtension* ext;
	while ((ext = iter++) != NULL)
		{
		ext->iNext.Deque();
		delete ext;
		}
	}

EXPORT_C void CExtensionPolicy::AddExtensionL(CExtension& aExtension)
	{
	CExtension* ext = CExtension::NewL();
	/* Dynamically allocated memory node has been assigned to the list and managed through the list.
	So CleanupStack::PopAndDestroy() will deallocate that memory. But, Coverity has misinterpreted it an issue.*/
    // coverity [SYMBIAN.CLEANUP STACK]
	// coverity [leave_without_push]
	aExtension.CopyL(*ext);
	iExtensions.AddLast(*ext);
	}


EXPORT_C void CExtensionPolicy::AddExtensionL(const TDesC8& aExtension)
	{
	CExtension* ext = CExtension::NewL(aExtension);
	iExtensions.AddLast(*ext);
	}


//
// QoS Policy db parser
//
EXPORT_C TPolicyParser::TPolicyParser(const TDesC &aPolicy) : iLine(0)
	{
	Assign(aPolicy);
	iExtensions.SetOffset(_FOFF(CExtension, iNext));
	iPolicies.SetOffset(_FOFF(CExtensionPolicy, iNext));
	}

EXPORT_C TPolicyParser::~TPolicyParser()
	{
	TExtensionQueueIter iter(iExtensions);
	CExtension *ext;
	while ((ext = iter++) != NULL)
		{
		ext->iNext.Deque();
		delete ext;
		}
	
	TQoSPolicyQueueIter policy_iter(iPolicies);
	CExtensionPolicy* policy;
	while ((policy = policy_iter++) != NULL)
		{
		policy->iNext.Deque();
		delete policy;
		}
	}


EXPORT_C TInt TPolicyParser::ParseL()
	{
	LOG(Log::Printf(_L("ParseL called...")));
	TInt error = KErrNone;
	iLine = 1;

	while (!Eos())
		{
		if (NextToken() == ETokenString)
			{
			_LIT(Kmodulespec, "modulespec");
			_LIT(Kflowspec, "flowspec");
			_LIT(Kextension_spec, "extension_spec");
			_LIT(Kmodule_policy, "module_policy");
			_LIT(Kflowspec_policy, "flowspec_policy");
			_LIT(Kextension_policy, "extension_policy");

			_LIT(KUnknown, "Unknown token '%S'");

			if (iToken.Compare(Kmodulespec) == 0)
				{
				error = ParseExtensionSpecL();
				}
			else if (iToken.Compare(Kflowspec) == 0)
				{
				error = ParseExtensionSpecL();
				}
			else if (iToken.Compare(Kextension_spec) == 0)
				{
				error = ParseExtensionSpecL();
				}
			else if (iToken.Compare(Kmodule_policy) == 0)
				{
				error = ParsePolicyL(EPfqosModulespecPolicy);
				}
			else if (iToken.Compare(Kflowspec_policy) == 0)
				{
				error = ParsePolicyL(EPfqosFlowspecPolicy); 
				}
			else if (iToken.Compare(Kextension_policy) == 0)
				{
				error = ParsePolicyL(EPfqosExtensionPolicy);
				}
			else
				{
				Error(KUnknown, &iToken);
				error = KErrGeneral;
				}
			}
		}
	return error;
	}



// Dummy error routines for now
void TPolicyParser::Error(TRefByValue<const TDesC> aFmt, ...)
	{
        //coverity[var_decl];
	VA_LIST list;
	VA_START(list, aFmt);
	
	_LIT(KatLine, " at line ");
         //coverity[uninit_use_in_call];
	iMsg.FormatList(aFmt, list);
	iMsg += KatLine;
	//lint -e{747} int -> long long
	iMsg.AppendNum(iLine);
	}


//
// Skip white space and mark, including comments!
//
void TPolicyParser::SkipSpaceAndMark()
	{
	TChar ch;
	TInt comment = 0;

	while (!Eos())
		{
		ch = Get();
		//lint -e{961} Missing else is OK
		if (ch =='\n')
			{
			iLine++;
			comment = 0;
			}
		else if (comment || ch == '#')
			{
			comment = 1;
			}
		else if (!ch.IsSpace())
			{
			UnGet();
			break;
			}
		}
	Mark();
	}


//
//
TTokenType TPolicyParser::NextToken()
	{
	TChar ch;
	TTokenType val;

	SkipSpaceAndMark();
	if (Eos())
		{
		val = ETokenEof;
		}
	else
		{
		ch = Get();
		if (ch == '{')
			{
			val = ETokenBraceLeft;
			}
		else if (ch == '}')
			{
			val = ETokenBraceRight;
			}
		else if (ch == '(')
			{
			val = ETokenParLeft;
			}
		else if (ch == ')')
			{
			val = ETokenParRight;
			}
		else if (ch == '=')
			{
			val = ETokenEqual;
			}
		else if (ch == ',')
			{
			val = ETokenComma;
			}
		else
			{
			val = ETokenString;
			while (!Eos())
				{
				ch = Peek();
				if (ch == '{' || ch == '}' ||
					ch == '(' || ch == ')' ||
					ch == '=' || ch == '#' || ch.IsSpace())
					{
					break;
					}
				Inc();
				}
			}
		}
	iToken.Set(MarkedToken());
	SkipSpaceAndMark();
	return val;
	}


TTokenType TPolicyParser::GetStringValue()
	{
	TChar ch('\0');
	TTokenType val;

	SkipSpaceAndMark();
	if (Eos())
		{
		val = ETokenEof;
		}
	else
		{
		TBool quoted = EFalse;
		ch = Get();
		if (ch == '"')
			{
			quoted = ETrue;
			Mark();
			}
		if (ch == '{')
			{
			val = ETokenBraceLeft;
			}
		else if (ch == '}')
			{
			val = ETokenBraceRight;
			}
		else if (ch == '(')
			{
			val = ETokenParLeft;
			}
		else if (ch == ')')
			{
			val = ETokenParRight;
			}
		else if (ch == '=')
			{
			val = ETokenEqual;
			}
		else if (ch == ',')
			{
			val = ETokenComma;
			}
		else
			{
			val = ETokenString;
			while (!Eos())
				{
				ch = Peek();
				if (ch == '"' && quoted)
					{
					quoted = EFalse;
					}
				if ((ch == '{' || ch == '}' ||
					 ch == '(' || ch == ')' ||
					 ch == '=' || ch == '#' || 
					 ch == '"' || ch.IsSpace()) && !quoted)
					{
					break;
					}
				Inc();
				}
			}
		}
	iToken.Set(MarkedToken());
	if (ch == '"')
		{
		Inc();
		}
	SkipSpaceAndMark();
	return val;
	}

//
// Extension spec
//
TInt TPolicyParser::ParseExtensionSpecL()
	{
	if (NextToken() != ETokenString)
		{
		Error(Ksyntax_error);
		}

	CExtension* spec = CExtension::NewL();
	iExtensions.AddLast(*spec);
	spec->SetName(iToken);

	if (NextToken() != ETokenEqual || NextToken() != ETokenBraceLeft)
		{
		Error(Ksyntax_error);
		}
	else 
		{
		if (ParseExtensionParams(*spec))
			{
			Error(Ksyntax_error);
			}
		else
			{
			return KErrNone;
			}
		}
	spec->iNext.Deque();
	delete spec;
	return KErrGeneral;
	}

//
// Parse extension parameters
//
TInt TPolicyParser::ParseExtensionParams(CExtension& aBuf)
	{
	LOG(Log::Printf(_L("ParseL called...")));
	TInt error = KErrNone;
	TTokenType val;

	while ((val = NextToken()) == ETokenString && error == KErrNone)
		{
		_LIT(KINTEGER, "INTEGER");
		_LIT(KREAL, "REAL");
		_LIT(KSTRING, "STRING");

		if (iToken.Compare(KINTEGER) == 0)
			{
			if (NextToken() != ETokenString)
				{
				return KErrGeneral;
				}
		
			// parameter name
			TName parameterName;
			parameterName.FillZ();
			parameterName = iToken;
		
			if (NextToken() != ETokenEqual)
				{
				return KErrGeneral;
				}
		
			TInt value;
			error = Val(value);
			if (error == KErrNone)
				{
				TRAP(error, aBuf.AddIntegerL(parameterName,value));
				}
		
			// Set the extension type here
			_LIT(KDescExtensionType, "extension_type");
			if (parameterName.Compare(KDescExtensionType) == 0)
				{
				aBuf.SetType(value);
				}
			}
		else if (iToken.Compare(KREAL) == 0)
			{
			if (NextToken() != ETokenString)
				{
				return KErrGeneral;
				}
		
			// parameter name
			TName parameterName;
			parameterName.FillZ();
			parameterName = iToken;
		
			if (NextToken() != ETokenEqual)
				{
				return KErrGeneral;
				}
		
			TReal value;
			error = Val(value);
			if (error == KErrNone)
				{
				TRAP(error, aBuf.AddRealL(parameterName,value));
				}
			}
		else if (iToken.Compare(KSTRING) == 0)
			{
			if ((val = NextToken()) != ETokenString)
				{
				return KErrGeneral;
				}
		
			// parameter name
			TName parameterName;
			parameterName.FillZ();
			parameterName = iToken;
		
			if (NextToken() != ETokenEqual)
				{
				return KErrGeneral;
				}
		
			if ((val = GetStringValue()) == ETokenString)
				{
				TName value = iToken;
				TRAP(error, aBuf.AddStringL(parameterName, value));
				}
			else
				{
				error = KErrGeneral;
				break;
				}
			}
		else
			{
			_LIT(KUnknownType, "Unknown variable type");
			Error(KUnknownType);
			return KErrGeneral;
			}
		}

	if (error != KErrNone)
		{
		_LIT(KInvalidValue, "invalid value");
		Error(KInvalidValue);
		return error;
		}

	if (val != ETokenBraceRight)
		{
		_LIT(KBraceNotFound, "right brace not found");
		Error(KBraceNotFound);
		return KErrGeneral;
		}

	return KErrNone;
	}


//
// Find extension policy data for a selector.
//
TInt TPolicyParser::FindExtensionPolicy(CExtensionPolicy *aSel)
	{
	TExtensionQueueIter iter(iExtensions);
	CExtension *policy;

	// Find extension data
	while ((policy = iter++) != NULL)
		{
		if (policy->Name().Compare(iToken)==0)
			{
			break;
			}
		}
	if (policy == NULL)
		{
		_LIT(KModule, "module not defined");
		Error(KModule);
		return KErrGeneral;
		}

	//
	// Set Data
	//
	TRAPD(err, aSel->AddExtensionL(*policy));
	return err;
	}




TInt TPolicyParser::ParseIPAddrAndMask(TInetAddr& aAddr, TInetAddr& aMask)
	{
	TInt error;

	if (NextToken() != ETokenString)
		{
		_LIT(KNoIpAddress, "ip address not found");
		Error(KNoIpAddress);
		return KErrGeneral;
		}

	error = aAddr.Input(iToken);

	if (error != 0)
		{
		_LIT(KInvalidIp, "invalid ip address ");
		Error(KInvalidIp);
		return error;
		}
	if (NextToken() != ETokenString)
		{
		_LIT(KNoMask, "address mask not found");
		Error(KNoMask);
		return KErrGeneral;
		}

	error = aMask.Input(iToken);

	if (error != 0)
		{
		_LIT(KInvalidMask, "invalid address mask ");
		Error(KInvalidMask);
		return error;
		}
	return KErrNone;
	}



TInt TPolicyParser::ParsePolicyL(TInt aPolicyType)
	{
	CSelectorBase* policy;
	TInt error = KErrNone;
	TTokenType val;
	TUint port;
	
	switch (aPolicyType)
		{
		case EPfqosFlowspecPolicy:
			{
			LOG(Log::Printf(_L("ParsePolicy FlowspecPolicy")));
			policy = new (ELeave) CExtensionPolicy();
			policy->iType = EPfqosFlowspecPolicy;
			break;
			}
		
		case EPfqosModulespecPolicy:
			{
			LOG(Log::Printf(_L("ParsePolicy ModulespecPolicy")));
			policy = new (ELeave) CExtensionPolicy();
			policy->iType = EPfqosModulespecPolicy;
			break;
			}
		
		default:
			{
			LOG(Log::Printf(_L("ParsePolicy ExtensionPolicy")));
			policy = new (ELeave) CExtensionPolicy();
			break;
			}
		}
	policy->iPriority = EPfqosDefaultPriority;

	if (NextToken() != ETokenString)
		{
		Error(Ksyntax_error);
		}

	TInt32 uid1=0;
	TInt32 uid2=0;
	TInt32 uid3=0;

	do
		{
		_LIT(Kdst, "dst");
		_LIT(Kremote, "remote");
		_LIT(Ksrc, "src");
		_LIT(Klocal, "local");
		_LIT(Kuser_id, "user_id");
		_LIT(Kprotocol, "protocol");
		_LIT(Ksrc_port, "src_port");
		_LIT(Klocal_port, "local_port");
		_LIT(Ksrc_port_max, "src_port_max");
		_LIT(Klocal_port_max, "local_port_max");
		_LIT(Kdst_port, "dst_port");
		_LIT(Kremote_port, "remote_port");
		_LIT(Kdst_port_max, "dst_port_max");
		_LIT(Kremote_port_max, "remote_port_max");
		_LIT(Kiap_id, "iap_id");
		_LIT(Kuid1, "uid1");
		_LIT(Kuid2, "uid2");
		_LIT(Kuid3, "uid3");
		_LIT(Kpriority, "priority");
		_LIT(Kname, "name");
	
		if ((iToken.Compare(Kdst)	== 0) || 
			(iToken.Compare(Kremote) == 0))
			{
			error = ParseIPAddrAndMask(policy->iDst, policy->iDstMask);
			}
		else if ((iToken.Compare(Ksrc)   == 0) || 
				 (iToken.Compare(Klocal) == 0))
			{
			error = ParseIPAddrAndMask(policy->iSrc, policy->iSrcMask);
			}
		else if (iToken.Compare(Kuser_id) == 0)
			{
			;	// Needs to be examined, TIdentity? -- msa
			}
		else if (iToken.Compare(Kprotocol) == 0)
			{
			error = Val(port);
			policy->iProtocol = (TUint8)port;
			}
		else if ((iToken.Compare(Ksrc_port)   == 0) || 
				 (iToken.Compare(Klocal_port) == 0))
			{
			error = Val(port);
			policy->iSrc.SetPort(port);
			if (policy->iSrcPortMax == 0)
				{
				policy->iSrcPortMax = (TUint16)port;
				}
			}
		else if ((iToken.Compare(Ksrc_port_max)   == 0) || 
				 (iToken.Compare(Klocal_port_max) == 0))
			{
			error = Val(port);
			policy->iSrcPortMax = (TUint16)port;
			}
		else if ((iToken.Compare(Kdst_port)	== 0) ||
				 (iToken.Compare(Kremote_port) == 0))
			{
			error = Val(port);
			policy->iDst.SetPort(port);
			if (policy->iDstPortMax == 0)
				{
				policy->iDstPortMax = (TUint16)port;
				}
			}
		else if ((iToken.Compare(Kdst_port_max)	== 0) || 
				 (iToken.Compare(Kremote_port_max) == 0))
			{
			error = Val(port);
			policy->iDstPortMax = (TUint16)port;
			}
		else if (iToken.Compare(Kiap_id) == 0)
			{
			error = Val(port);
			policy->iIapId = (TUint8)port;
			}
		else if (iToken.Compare(Kuid1) == 0)
			{
			error = Val(port);
			uid1 = port;
			}
		else if (iToken.Compare(Kuid2) == 0)
			{
			error = Val(port);
			uid2 = port;
			}
		else if (iToken.Compare(Kuid3) == 0)
			{
			error = Val(port);
			uid3 = port;
			}
		else if (iToken.Compare(Kpriority) == 0)
			{
			error = Val(port);
			policy->iPriority = port;
			}
		else if (iToken.Compare(Kname) == 0)
			{
			if (NextToken() != ETokenString)
				{
				delete policy;
				return KErrGeneral;
				}
			policy->iName = iToken;
			}
		else
			{
			_LIT(Kinvalid_keyword, "invalid keyword ");
			Error(Kinvalid_keyword);
			delete policy;
			return KErrGeneral;
			}
		
		if (error != KErrNone)
			{
			_LIT(KErrorFormat, "Error = %d");
			Error(KErrorFormat, error);
			delete policy;
			return error;
			}
		}
	while ((val = NextToken()) == ETokenString);

	TUidType uid_type(TUid::Uid(uid1), TUid::Uid(uid2), TUid::Uid(uid3));
	policy->iUid.Set(uid_type);

	// Update this to support multiple extension blocks
	if (val != ETokenEqual || NextToken() != ETokenBraceLeft)
		{
		Error(Ksyntax_error);
		error = KErrGeneral;
		delete policy;
		return error;
		}
	else
		{
		while ((val = NextToken()) == ETokenString && error == KErrNone)
			{
			switch (aPolicyType)
				{
				case EPfqosFlowspecPolicy:
					{
					error = FindExtensionPolicy((CExtensionPolicy*)policy);
					break;
					}
				
				case EPfqosModulespecPolicy:
					{
					error = FindExtensionPolicy((CExtensionPolicy*)policy);
					break;
					}
				
				default:
					{
					error = FindExtensionPolicy((CExtensionPolicy*)policy);
					break;
					}
				}
			}
		}

	if (val != ETokenBraceRight || error != KErrNone)
		{
		if (error == KErrNone)
			{
			error = KErrGeneral;
			}
		delete policy;
		}
	else
		{
		AddPolicy(*(CExtensionPolicy*)policy);
		}

	return error;
	}


EXPORT_C CExtension* CExtension::NewL()
	{
	CExtension* extension = new (ELeave) CExtension();
	CleanupStack::PushL(extension);
	extension->ConstructL();
	CleanupStack::Pop();
	return extension;
	}

EXPORT_C CExtension* CExtension::NewL(const TDesC8& aData)
	{
	CExtension* extension = new (ELeave) CExtension();
	CleanupStack::PushL(extension);
	extension->ConstructL();
	extension->CopyL(aData);
	CleanupStack::Pop();
	return extension;
	}


CExtension::CExtension()
	{
	iVariables.SetOffset(_FOFF(TVariableBase, iNext));
	iBuf = NULL;
	}


EXPORT_C CExtension::~CExtension()
	{
	TVariableQueueIter iter(iVariables);
	TVariableBase* var;
	while ((var = iter++) != NULL)
		{
		var->iNext.Deque();
		delete var;
		}
	delete iBuf;
	}

void CExtension::ConstructL()
	{
	iBuf = CBufFlat::NewL(KBufSize);
	}

EXPORT_C void CExtension::SetName(const TDesC& aName)
	{
	iName = aName;
	}


EXPORT_C TVariableBase* CExtension::FindVariable(const TDesC& aName)
	{
	TVariableQueueIter iter(iVariables);
	TVariableBase* var;

	while ((var = iter++) != NULL)
		{
		if (!aName.Compare(var->Name()))
			{
			return var;
			}
		}
	return NULL;
	}


EXPORT_C TInt CExtension::AddIntegerL(const TDesC& aName, TInt aValue)
	{
	TVariableBase *tmp = FindVariable(aName);

	/* 
	 *  Don't check for the existing variable names, 
	 *  if it is an SBLP variable.
	 */
	if (!(aName.Compare(KDescSblpMediaComponentNumber) == 0 || 
		  aName.Compare(KDescSblpIPFlowNumber)		 == 0 ))
		{
		if (tmp)
			{
			return KErrAlreadyExists;
			}
		}

	TIntVariable *var = new (ELeave) TIntVariable(aName, aValue);
	//lint --e{429} Lint doesn't understand AddLast semantics.
	iVariables.AddLast(*var);
	return KErrNone;
	}

EXPORT_C TInt CExtension::AddRealL(const TDesC& aName, TReal aValue)
	{
	TVariableBase *tmp = FindVariable(aName);
	if (tmp)
		{
		return KErrAlreadyExists;
		}
	TRealVariable *var = new (ELeave) TRealVariable(aName, aValue);
	//lint --e{429} Lint doesn't understand AddLast semantics.
	iVariables.AddLast(*var);
	return KErrNone;
	}

EXPORT_C TInt CExtension::AddStringL(const TDesC& aName, const TDesC& aValue)
	{
	TVariableBase *tmp = FindVariable(aName);

	/* 
	 *  Don't check for the existing variable names, 
	 *  if it is an SBLP variable.
	 */
	if (!(aName.Compare(KDescSblpMediaAuthorizationToken) == 0))
		{
		if (tmp)
			{
			return KErrAlreadyExists;
			}
		}
		
	TStringVariable *var = new (ELeave) TStringVariable(aName, aValue);
	//lint --e{429} Lint doesn't understand AddLast semantics.
	iVariables.AddLast(*var);
	return KErrNone;
	}

EXPORT_C TInt CExtension::Copy(TDes8& aData)
	{
	TVariableQueueIter iter(iVariables);
	TVariableBase* var;

	TRAPD(err, InitL());
	while ((var = iter++) != NULL && err == KErrNone)
		{
		switch (var->Type())
			{
			case KPfqosTypeInteger:
				{
				TIntVariable* intVar = (TIntVariable*) var;
				TRAP(err, SetIntValueL(intVar->Value(), intVar->Name()));
				}
				break;
		
			case KPfqosTypeString:
				{
				TStringVariable* stringVar = (TStringVariable*) var;
				TRAP(err, SetStringValueL(stringVar->Value(), 
										  stringVar->Name()));
				}
				break;
		
			default:
				break;
			}
		}

	if (err != KErrNone)
		{
		return err;
		}

	TRAP(err, SetLengthL());
	if (err != KErrNone)
		{
		return err;
		}

	if (aData.MaxLength() < iPos)
		{
		return KErrNoMemory;
		}
	iBuf->Read(0, aData, iPos);
	return KErrNone;
	}


EXPORT_C TInt CExtension::CopyL(const TDesC8& aData)
	//??This is leaving function with status return?
	//??Noone seems to check the return value, so
	//??all errors are now changed to leaves.
	{
	Reset();

	const TUint8 *p = aData.Ptr();
	TInt length = aData.Length();
	//lint -e{826} typecast OK
	struct pfqos_configure *ext = (struct pfqos_configure *) p;

	if (length < (TInt)sizeof(pfqos_configure))
		{
		User::Leave(KErrGeneral);		// EMSGSIZE (impossible message size)
		}

	if (ext->pfqos_configure_len * 8 != length)
		{
		User::Leave(KErrGeneral);		// EMSGSIZE (incorrect message length)
		}

	if (ext->pfqos_ext_type == EPfqosExtExtension)
		{
		p += sizeof(struct pfqos_configure);
		//lint -e{826} typecast OK
		pfqos_extension *extensionType = (pfqos_extension *) p;

		SetType(extensionType->pfqos_extension_type);
		TPtrC8 extensionName((TUint8*) extensionType->extension_name);
		if (iName.MaxLength() > extensionName.Length())
			{
			// Need to have room for zero termination!
			iName.Copy(extensionName);
			}
		iName.ZeroTerminate();

		//lint -e{826} odd typecasts are OK
		while (length > 0)
			{
			struct pfqos_configblock *block = (struct pfqos_configblock *)
											  (p + sizeof(pfqos_extension));
			int block_len = block->len;

			if (block_len < 1)
				{
				break;
				}
			block_len *= 8;

			if (block_len > length)
				{
				break;
				}
			//lint -e{961} Missing final 'else' is OK
			if (block->type == KPfqosTypeInteger)
				{
				TPtrC8 tmp((TUint8 *)&block->id[0]);
				struct pfqos_configblock_int *block_int = 
					(struct pfqos_configblock_int*) block;

				TName aName;
				aName.Copy(tmp);
				AddIntegerL(aName, block_int->value);
				}
			else if (block->type == KPfqosTypeReal)
				{
				TPtrC8 tmp((TUint8 *)&block->id[0]);
				struct pfqos_configblock_real *block_real = 
					(struct pfqos_configblock_real*) block;

				TName name;
				name.Copy(tmp);
				//lint -e{747} float -> double
				AddRealL(name, block_real->value);
				}
			else if (block->type == KPfqosTypeString)
				{
				TPtrC8 tmp((TUint8 *)&block->id[0]);
				TPtrC8 value((TUint8 *)(((TUint8*)block)+ 
					sizeof(struct pfqos_configblock)));

				TName name, val;

				if (tmp.Length() > name.MaxLength() ||
					value.Length() > val.MaxLength())
					User::Leave(KErrArgument);

				name.Copy(tmp);
				val.Copy(value);
				AddStringL(name, val);
				}
			p += block_len;
			length -= block_len;
			}
		}

	return KErrNone;
	}

EXPORT_C TInt CExtension::CopyL(CExtension& aExtension)
	{
	TVariableQueueIter iter(iVariables);
	TVariableBase* var;

	aExtension.SetName(iName);
	aExtension.SetType(iType);

	while ((var = iter++) != NULL)
		{
		switch (var->Type())
			{
			case KPfqosTypeInteger:
				{
				TIntVariable* intVar = (TIntVariable*) var;
				aExtension.AddIntegerL(intVar->Name(), intVar->Value());
				}
				break;
		
			case KPfqosTypeReal:
				{
				TRealVariable* realVar = (TRealVariable*) var;
				aExtension.AddRealL(realVar->Name(), realVar->Value());
				}
				break;
		
			case KPfqosTypeString:
				{
				TStringVariable* stringVar = (TStringVariable*) var;
				aExtension.AddStringL(stringVar->Name(), stringVar->Value());
				}
				break;
		
			default:
				break;
			}
		}
	return KErrNone;
	}

EXPORT_C const TPtrC8 CExtension::Data()
	{
	if (!iBuf)
		{
		return TPtr8(0,0);
		}

	TVariableQueueIter iter(iVariables);
	TVariableBase* var;

	TRAPD(err, InitL());
	while ((var = iter++) != NULL && err == KErrNone)
		{
		switch (var->Type())
			{
			case KPfqosTypeInteger:
				{
				TIntVariable* intVar = (TIntVariable*) var;
				TRAP(err, SetIntValueL(intVar->Value(), intVar->Name()));
				}
				break;

			case KPfqosTypeString:
				{
				TStringVariable* stringVar = (TStringVariable*) var;
				TRAP(err, SetStringValueL(stringVar->Value(), 
										  stringVar->Name()));
				}
				break;

			default:
				break;
			}
		}

	if (err != KErrNone)
		{
		return TPtr8(0,0);
		}

	TRAP(err, SetLengthL());
	if (err != KErrNone)
		{
		return TPtr8(0,0);
		}

	return iBuf->Ptr(0);
	}

EXPORT_C TInt CExtension::Length() const
	{
	if (iBuf)
		{
		return iBuf->Capacity();
		}
	return 0;
	}

void CExtension::Reset()
	{
	TVariableQueueIter iter(iVariables);
	TVariableBase* var;
	while ((var = iter++) != NULL)
		{
		var->iNext.Deque();
		delete var;
		}
	iBuf->Reset();
	iPos=0;
	}

void CExtension::InitL()
	{
	iBuf->Reset();
	iPos=0;

	pfqos_configure header;
	header.pfqos_configure_len = 0;
	header.pfqos_ext_type = EPfqosExtExtension;
	header.reserved = 0;
	header.protocol_id = 0;
	iBuf->InsertL(iPos, &header, sizeof(header));
	iPos += sizeof(header);

	pfqos_extension extensionType;
	extensionType.pfqos_ext_len = 0;
	extensionType.pfqos_ext_type = EPfqosExtExtension;
	extensionType.pfqos_extension_type = iType;
	TPtr8 name((TUint8*)extensionType.extension_name, 0, KMaxName);
	if (name.MaxLength() > iName.Length())
		{
		// Need to leave room for the zero termination!
		name.Copy(iName);
		}
	name.ZeroTerminate();
	iBuf->InsertL(iPos, &extensionType, sizeof(extensionType));
	iPos += sizeof(extensionType);
	}


void CExtension::SetIntValueL(TInt aValue, const TDesC& aName)
	{
	pfqos_configblock_int data;
	TPtr8 ptr((TUint8 *)&data, sizeof(pfqos_configblock_int), 
							   sizeof(pfqos_configblock_int));
	data.len = sizeof(pfqos_configblock_int)/8;
	data.padding = data.reserved = 0;
	data.type = KPfqosTypeInteger;
	data.value = aValue;
	TPtr8 tmpPtr((TUint8*)&data.id[0], 0, KMaxName);
	if (tmpPtr.MaxLength() > aName.Length())
		{
		// Need to leave room for the zero termination!
		tmpPtr.Copy(aName);
		}
	tmpPtr.ZeroTerminate();
	iBuf->InsertL(iPos, ptr);
	iPos += ptr.Length();
	}


void CExtension::SetReal32ValueL(TReal32 aValue, const TDesC& aName)
	{
	pfqos_configblock_real data;
	TPtr8 ptr((TUint8 *)&data, sizeof(pfqos_configblock_real), 
							   sizeof(pfqos_configblock_real));
	data.len = sizeof(pfqos_configblock_real)/8;
	data.padding = data.reserved = 0;
	data.type = KPfqosTypeReal;
	data.value = aValue;
	TPtr8 tmpPtr((TUint8*)data.id, 0, KMaxName);
	if (tmpPtr.MaxLength() > aName.Length())
		{
		// Need to leave room for the zero termination!
		tmpPtr.Copy(aName);
		}
	tmpPtr.ZeroTerminate();
	iBuf->InsertL(iPos, ptr);
	iPos += ptr.Length();
	}


void CExtension::SetStringValueL(const TDesC& aValue, const TDesC& aName)
	{
	pfqos_configblock data;
	TPtr8 ptr((TUint8 *)&data, sizeof(pfqos_configblock), 
							   sizeof(pfqos_configblock));
	data.len = (TUint16)((sizeof(pfqos_configblock) + aValue.Length() + 7)/8);
	data.reserved = 0;
	data.type = KPfqosTypeString;
	TPtr8 tmpPtr((TUint8*) data.id, 0, KMaxName);
	if (tmpPtr.MaxLength() > aName.Length())
		{
		// Need to leave room for the zero termination!
		tmpPtr.Copy(aName);
		}
	tmpPtr.ZeroTerminate();
	iBuf->InsertL(iPos, ptr);
	iPos += ptr.Length();

#ifdef _UNICODE
	HBufC8 *tmp = HBufC8::NewL(aValue.Length());
	tmp->Des().Copy(aValue);
	iBuf->InsertL(iPos, *tmp);
	delete tmp;
#else
	iBuf->InsertL(iPos, aValue);
#endif

	iPos += aValue.Length();
	}


void CExtension::SetLengthL()
	{
	TUint16 length16 = (TUint16)((iPos +7) / 8);
	TPtrC8 len = TPtrC8((TUint8 *)&length16, sizeof(length16));
	iBuf->Write(_FOFF(struct pfqos_configure, pfqos_configure_len), len);

	TInt pad = (length16 * 8 - iPos);
	TUint8 dummy=0;
	TPtrC8 dummy2 = TPtrC8((TUint8 *)&dummy, sizeof(dummy));
	for (TInt i = 0; i < pad; i++)
		{
		iBuf->InsertL(iPos, dummy2);
		iPos++;
		}
	}

