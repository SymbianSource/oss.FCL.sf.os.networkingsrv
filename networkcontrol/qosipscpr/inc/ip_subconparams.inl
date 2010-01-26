/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Inline Functions file for the IP SubConnection Parameters
* 
*
*/



/**
 @file ip_subconparams.inl
*/


#ifndef __IP_SUBCONPARAMS_INL__
#define __IP_SUBCONPARAMS_INL__

#ifndef SYMBIAN_NETWORKING_3GPPDEFAULTQOS

CSubConQosIPLinkR99ParamSet* CSubConQosIPLinkR99ParamSet::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
	{
	CSubConQosIPLinkR99ParamSet* obj = NewL();
	CleanupStack::PushL(obj);
	aFamily.AddExtensionSetL(*obj, aType);
	CleanupStack::Pop(obj);
	return obj;
	}

CSubConQosIPLinkR99ParamSet* CSubConQosIPLinkR99ParamSet::NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType)
	{
	CSubConQosIPLinkR99ParamSet* obj = NewL();
	CleanupStack::PushL(obj);
	aFamily.AddParameterSetL(obj, aType);
	CleanupStack::Pop(obj);
	return obj;
	}

CSubConQosIPLinkR99ParamSet* CSubConQosIPLinkR99ParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(KSubConIPParamsUid, KSubConQosIPLinkR99ParamsType);
	return static_cast<CSubConQosIPLinkR99ParamSet*>(CSubConParameterSet::NewL(typeId));
	}

CSubConQosIPLinkR99ParamSet::CSubConQosIPLinkR99ParamSet()
	: CSubConExtensionParameterSet(), 
	iTrafficClass(RPacketQoS::ETrafficClassUnspecified),
	iDeliveryOrder(RPacketQoS::EDeliveryOrderUnspecified),
	iDeliveryOfErroneusSdu(RPacketQoS::EErroneousSDUDeliveryUnspecified),
	iResidualBer(RPacketQoS::EBERUnspecified),
	iErrorRatio(RPacketQoS::ESDUErrorRatioUnspecified),
	iPriority(RPacketQoS::ETrafficPriorityUnspecified),
	iTransferDelay(0),
	iMaxSduSize(0),
	iMaxBitrateUplink(0),
	iMaxBitrateDownlink(0),
	iGuaBitrateUplink(0),
	iGuaBitrateDownlink(0)
	{
	}

RPacketQoS::TTrafficClass CSubConQosIPLinkR99ParamSet::GetTrafficClass() const
	{
	return iTrafficClass;
	}

RPacketQoS::TDeliveryOrder CSubConQosIPLinkR99ParamSet::GetDeliveryOrder() const
	{
	return iDeliveryOrder;
	}

RPacketQoS::TErroneousSDUDelivery CSubConQosIPLinkR99ParamSet::GetErroneousSDUDelivery() const
	{
	return iDeliveryOfErroneusSdu;
	}

RPacketQoS::TBitErrorRatio CSubConQosIPLinkR99ParamSet::GetResidualBitErrorRatio() const
	{
	return iResidualBer;
	}

RPacketQoS::TSDUErrorRatio CSubConQosIPLinkR99ParamSet::GetSDUErrorRatio() const
	{
	return iErrorRatio;
	}

RPacketQoS::TTrafficHandlingPriority CSubConQosIPLinkR99ParamSet::GetTrafficHandlingPriority() const
	{
	return iPriority;
	}

TInt CSubConQosIPLinkR99ParamSet::GetTransferDelay() const
	{
	return iTransferDelay;
	}

TInt CSubConQosIPLinkR99ParamSet::GetMaxSduSize() const
	{
	return iMaxSduSize;
	}

TInt CSubConQosIPLinkR99ParamSet::GetMaxBitrateUplink() const
	{
	return iMaxBitrateUplink;
	}

TInt CSubConQosIPLinkR99ParamSet::GetMaxBitrateDownlink() const
	{
	return iMaxBitrateDownlink;
	}

TInt CSubConQosIPLinkR99ParamSet::GetGuaBitrateUplink() const
	{
	return iGuaBitrateUplink;
	}

TInt CSubConQosIPLinkR99ParamSet::GetGuaBitrateDownlink() const
	{
	return iGuaBitrateDownlink;
	}

void CSubConQosIPLinkR99ParamSet::SetTrafficClass(RPacketQoS::TTrafficClass aTrafficClass)
	{
	iTrafficClass = aTrafficClass;
	}

void CSubConQosIPLinkR99ParamSet::SetDeliveryOrder(RPacketQoS::TDeliveryOrder aDeliveryOrder)
	{
	iDeliveryOrder = aDeliveryOrder;
	}

void CSubConQosIPLinkR99ParamSet::SetErroneousSDUDelivery(RPacketQoS::TErroneousSDUDelivery aDeliveryOfErroneusSdu)
	{
	iDeliveryOfErroneusSdu = aDeliveryOfErroneusSdu;
	}

void CSubConQosIPLinkR99ParamSet::SetResidualBitErrorRatio(RPacketQoS::TBitErrorRatio aResidualBer)
	{
	iResidualBer = aResidualBer;
	}

void CSubConQosIPLinkR99ParamSet::SetSDUErrorRatio(RPacketQoS::TSDUErrorRatio aErrorRatio)
	{
	iErrorRatio = aErrorRatio;
	}

void CSubConQosIPLinkR99ParamSet::SetTrafficHandlingPriority(RPacketQoS::TTrafficHandlingPriority aPriority)
	{
	iPriority = aPriority;
	}

void CSubConQosIPLinkR99ParamSet::SetTransferDelay(TInt aTransferDelay)
	{
	iTransferDelay = aTransferDelay;
	}

void CSubConQosIPLinkR99ParamSet::SetMaxSduSize(TInt aMaxSduSize)
	{
	iMaxSduSize = aMaxSduSize;
	}

void CSubConQosIPLinkR99ParamSet::SetMaxBitrateUplink(TInt aMaxBitrateUplink)
	{
	iMaxBitrateUplink = aMaxBitrateUplink;
	}

void CSubConQosIPLinkR99ParamSet::SetMaxBitrateDownlink(TInt aMaxBitrateDownlink)
	{
	iMaxBitrateDownlink = aMaxBitrateDownlink;
	}

void CSubConQosIPLinkR99ParamSet::SetGuaBitrateUplink(TInt aGuaBitrateUplink)
	{
	iGuaBitrateUplink = aGuaBitrateUplink;
	}

void CSubConQosIPLinkR99ParamSet::SetGuaBitrateDownlink(TInt aGuaBitrateDownlink)
	{
	iGuaBitrateDownlink = aGuaBitrateDownlink;
	}


TUint16 TFlowId::GetMediaComponentNumber() const
	{
	return iMediaComponentNumber;
	}

TUint16 TFlowId::GetIPFlowNumber() const
	{
	return iIPFlowNumber;	
	}

void TFlowId::SetMediaComponentNumber(TUint16 aMediaComponentNumber)
	{
	iMediaComponentNumber = aMediaComponentNumber;
	}

void TFlowId::SetIPFlowNumber(TUint16 aIPFlowNumber)
	{
	iIPFlowNumber = aIPFlowNumber;
	}


//===========================
// Implementation Extension class
CSubConSBLPR5ExtensionParamSet::CSubConSBLPR5ExtensionParamSet()
	: CSubConExtensionParameterSet()
	{
	} 

CSubConSBLPR5ExtensionParamSet::~CSubConSBLPR5ExtensionParamSet()
	{
	iFlowIds.Close();
	}

CSubConSBLPR5ExtensionParamSet* CSubConSBLPR5ExtensionParamSet::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
	{
	CSubConSBLPR5ExtensionParamSet* sblpExtn = NewL();
	CleanupStack::PushL(sblpExtn);
	aFamily.AddExtensionSetL(*sblpExtn, aType);
	CleanupStack::Pop(sblpExtn);
	return sblpExtn;
	}

CSubConSBLPR5ExtensionParamSet* CSubConSBLPR5ExtensionParamSet::NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType)
	{
	CSubConSBLPR5ExtensionParamSet* sblpExtn = NewL();
	CleanupStack::PushL(sblpExtn);
	aFamily.AddParameterSetL(sblpExtn, aType);
	CleanupStack::Pop(sblpExtn);
	return sblpExtn;
	}
CSubConSBLPR5ExtensionParamSet* CSubConSBLPR5ExtensionParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(KSubConIPParamsUid, KSubConnSBLPR5ExtensionParamsType);
	return static_cast<CSubConSBLPR5ExtensionParamSet*>(CSubConParameterSet::NewL(typeId));
	}
	
const TAuthToken& CSubConSBLPR5ExtensionParamSet::GetMAT() const
	{
	return iAuthToken;
	}

void CSubConSBLPR5ExtensionParamSet::SetMAT(const TAuthToken& aAuthToken)
	{
	iAuthToken = aAuthToken;
	}

TInt CSubConSBLPR5ExtensionParamSet::GetNumberOfFlowIds() const
	{
	return iFlowIds.Count();
	}

const TFlowId& CSubConSBLPR5ExtensionParamSet::GetFlowIdAt(TInt aIndex) const
	{
	return iFlowIds[aIndex];
	}

void CSubConSBLPR5ExtensionParamSet::AddFlowIdL(const TFlowId & aFlowId)
	{
	iFlowIds.AppendL(aFlowId);
	}
#endif
// SYMBIAN_NETWORKING_3GPPDEFAULTQOS

#endif
// __IP_SUBCONPARAMS_INL__

