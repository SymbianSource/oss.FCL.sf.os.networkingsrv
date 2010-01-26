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
// Inline Functions file for the IP Address Info SubConnection Parameters
// 
//

/**
 @file
 @publishedPartner
 @released since 9.5
*/



#ifndef IPADDRINFOPARAMS_INL
#define IPADDRINFOPARAMS_INL

CSubConIPAddressInfoParamSet* CSubConIPAddressInfoParamSet::NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType)
	{
	CSubConIPAddressInfoParamSet* obj = NewL();
	CleanupStack::PushL(obj);
	aFamily.AddParameterSetL(obj, aType);
	CleanupStack::Pop(obj);
	return obj;
	}

CSubConIPAddressInfoParamSet* CSubConIPAddressInfoParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(CSubConIPAddressInfoParamSet::EUid, CSubConIPAddressInfoParamSet::ETypeId);
	return static_cast<CSubConIPAddressInfoParamSet*>(CSubConParameterSet::NewL(typeId));
	}

CSubConIPAddressInfoParamSet::CSubConIPAddressInfoParamSet()
	:CSubConGenericParameterSet(), iOpCode(ENone)
	{
	}

CSubConIPAddressInfoParamSet::~CSubConIPAddressInfoParamSet()
	{
	iParams.Close();
	}

void CSubConIPAddressInfoParamSet::AddParamInfo(TSubConIPAddressInfo aParam)
	{
	iParams.Append(aParam);
	}

void CSubConIPAddressInfoParamSet::RemoveParamInfo(TUint aIndex)
	{
	iParams.Remove(aIndex);
	}

TInt CSubConIPAddressInfoParamSet::DeleteParams()
	{
	if(iOpCode == EDelete)
		{
		iParams.Reset();
		iOpCode = ENone;
		
		return KErrNone;
		}
	
	return KErrNotReady;
	}

CSubConIPAddressInfoParamSet::TSubConIPAddressInfo::TSubConIPAddressInfo(TSockAddr &aCliSrcAddr, TSockAddr &aCliDstAddr, TInt aProtocolId, TSubConIPAddressInfo::EState aState)
	: iCliSrcAddr(aCliSrcAddr), iCliDstAddr(aCliDstAddr), iProtocolId(aProtocolId), iState(aState)
	{
	}


TBool CSubConIPAddressInfoParamSet::TSubConIPAddressInfo::Compare(TSubConIPAddressInfo aInfo)
	{
	TBool result = aInfo.iCliSrcAddr == iCliSrcAddr;
	result &= aInfo.iCliDstAddr == iCliDstAddr;
	result &= aInfo.iProtocolId == iProtocolId;
	
	return result;
	}

CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& CSubConIPAddressInfoParamSet::GetParamInfoL(TUint aIndex)
	{
	if(aIndex > iParams.Count())
		{
		User::Leave(KErrOverflow);
		}
	
	return iParams[aIndex];
	}

TUint CSubConIPAddressInfoParamSet::GetParamNum()
	{
	return iParams.Count();
	}

void CSubConIPAddressInfoParamSet::SetOperationCode(CSubConIPAddressInfoParamSet::EOperationCode aOpCode)
	{
	iOpCode = aOpCode;	
	}

CSubConIPAddressInfoParamSet::EOperationCode CSubConIPAddressInfoParamSet::GetOperationCode()
	{
	return iOpCode;
	}

#endif
// IPADDRINFOPARAMS_INL

