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
// uscl_sblp.inl
//

#ifndef _USCL_QOS3GPP_SUBCONPARAMS_INL__
#define _USCL_QOS3GPP_SUBCONPARAMS_INL__

// This file supports SBLP testing by simulating 
// class CSubConSBLPR5ExtensionParamSet.

CSubConQosR99ParamSet* CSubConQosR99ParamSet::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
	{
	CSubConQosR99ParamSet* obj = NewL();
	CleanupStack::PushL(obj);
	aFamily.AddExtensionSetL(*obj, aType);
	CleanupStack::Pop(obj);
	return obj;
	}

CSubConQosR99ParamSet* CSubConQosR99ParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(KSubCon3GPPExtParamsFactoryUid, KSubConQosR99ParamsType);
	return static_cast<CSubConQosR99ParamSet*>(CSubConParameterSet::NewL(typeId));
	}

CSubConQosR99ParamSet::CSubConQosR99ParamSet()
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

RPacketQoS::TTrafficClass CSubConQosR99ParamSet::GetTrafficClass() const
	{
	return iTrafficClass;
	}

RPacketQoS::TDeliveryOrder CSubConQosR99ParamSet::GetDeliveryOrder() const
	{
	return iDeliveryOrder;
	}

RPacketQoS::TErroneousSDUDelivery CSubConQosR99ParamSet::GetErroneousSDUDelivery() const
	{
	return iDeliveryOfErroneusSdu;
	}

RPacketQoS::TBitErrorRatio CSubConQosR99ParamSet::GetResidualBitErrorRatio() const
	{
	return iResidualBer;
	}

RPacketQoS::TSDUErrorRatio CSubConQosR99ParamSet::GetSDUErrorRatio() const
	{
	return iErrorRatio;
	}

RPacketQoS::TTrafficHandlingPriority CSubConQosR99ParamSet::GetTrafficHandlingPriority() const
	{
	return iPriority;
	}

TInt CSubConQosR99ParamSet::GetTransferDelay() const
	{
	return iTransferDelay;
	}

TInt CSubConQosR99ParamSet::GetMaxSduSize() const
	{
	return iMaxSduSize;
	}

TInt CSubConQosR99ParamSet::GetMaxBitrateUplink() const
	{
	return iMaxBitrateUplink;
	}

TInt CSubConQosR99ParamSet::GetMaxBitrateDownlink() const
	{
	return iMaxBitrateDownlink;
	}

TInt CSubConQosR99ParamSet::GetGuaBitrateUplink() const
	{
	return iGuaBitrateUplink;
	}

TInt CSubConQosR99ParamSet::GetGuaBitrateDownlink() const
	{
	return iGuaBitrateDownlink;
	}

void CSubConQosR99ParamSet::SetTrafficClass(RPacketQoS::TTrafficClass aTrafficClass)
	{
	iTrafficClass = aTrafficClass;
	}

void CSubConQosR99ParamSet::SetDeliveryOrder(RPacketQoS::TDeliveryOrder aDeliveryOrder)
	{
	iDeliveryOrder = aDeliveryOrder;
	}

void CSubConQosR99ParamSet::SetErroneousSDUDelivery(RPacketQoS::TErroneousSDUDelivery aDeliveryOfErroneusSdu)
	{
	iDeliveryOfErroneusSdu = aDeliveryOfErroneusSdu;
	}

void CSubConQosR99ParamSet::SetResidualBitErrorRatio(RPacketQoS::TBitErrorRatio aResidualBer)
	{
	iResidualBer = aResidualBer;
	}

void CSubConQosR99ParamSet::SetSDUErrorRatio(RPacketQoS::TSDUErrorRatio aErrorRatio)
	{
	iErrorRatio = aErrorRatio;
	}

void CSubConQosR99ParamSet::SetTrafficHandlingPriority(RPacketQoS::TTrafficHandlingPriority aPriority)
	{
	iPriority = aPriority;
	}

void CSubConQosR99ParamSet::SetTransferDelay(TInt aTransferDelay)
	{
	iTransferDelay = aTransferDelay;
	}

void CSubConQosR99ParamSet::SetMaxSduSize(TInt aMaxSduSize)
	{
	iMaxSduSize = aMaxSduSize;
	}

void CSubConQosR99ParamSet::SetMaxBitrateUplink(TInt aMaxBitrateUplink)
	{
	iMaxBitrateUplink = aMaxBitrateUplink;
	}

void CSubConQosR99ParamSet::SetMaxBitrateDownlink(TInt aMaxBitrateDownlink)
	{
	iMaxBitrateDownlink = aMaxBitrateDownlink;
	}

void CSubConQosR99ParamSet::SetGuaBitrateUplink(TInt aGuaBitrateUplink)
	{
	iGuaBitrateUplink = aGuaBitrateUplink;
	}

void CSubConQosR99ParamSet::SetGuaBitrateDownlink(TInt aGuaBitrateDownlink)
	{
	iGuaBitrateDownlink = aGuaBitrateDownlink;
	}


#ifdef SYMBIAN_NETWORKING_UMTSR5  
CSubConIMSExtParamSet* CSubConIMSExtParamSet::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
	{
	CSubConIMSExtParamSet* obj = NewL();
	CleanupStack::PushL(obj);
	aFamily.AddExtensionSetL(*obj, aType);
	CleanupStack::Pop(obj);
	return obj;
	}
	
CSubConIMSExtParamSet* CSubConIMSExtParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(KSubCon3GPPExtParamsFactoryUid, KSubConIMSExtParamsType);
	return static_cast<CSubConIMSExtParamSet*>(CSubConParameterSet::NewL(typeId));
	}
	
CSubConIMSExtParamSet::CSubConIMSExtParamSet()
	: iIMSSignallingIndicator(EFalse) 
	{
	}

TBool CSubConIMSExtParamSet::GetIMSSignallingIndicator() const
	{
	return iIMSSignallingIndicator;
	}
	
void CSubConIMSExtParamSet::SetIMSSignallingIndicator(TBool aIMSSignallingIndicator)
	{
	iIMSSignallingIndicator = aIMSSignallingIndicator;
	}
	
CSubConQosR5ParamSet* CSubConQosR5ParamSet::NewL(CSubConParameterFamily& aFamily,CSubConParameterFamily::TParameterSetType aType)
	{
	CSubConQosR5ParamSet* obj = NewL();
	CleanupStack::PushL(obj);
	aFamily.AddExtensionSetL(*obj, aType);
	CleanupStack::Pop(obj);
	return obj;
	}

CSubConQosR5ParamSet* CSubConQosR5ParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(KSubCon3GPPExtParamsFactoryUid, KSubConQosR5ParamsType);
	return static_cast<CSubConQosR5ParamSet*>(CSubConParameterSet::NewL(typeId));
	}

CSubConQosR5ParamSet::CSubConQosR5ParamSet()
	: iSrcStatsDesc(RPacketQoS::ESourceStatisticsDescriptorUnknown), iSignallingIndicator(EFalse)
	{
	}

TBool CSubConQosR5ParamSet::GetSignallingIndicator() const
	{
	return iSignallingIndicator;
	}

void CSubConQosR5ParamSet::SetSignallingIndicator(TBool aSignallingIndicator)
	{
	iSignallingIndicator = aSignallingIndicator;
	}
	
RPacketQoS::TSourceStatisticsDescriptor CSubConQosR5ParamSet::GetSourceStatisticsDescriptor() const
	{
	return iSrcStatsDesc;
	}
	
void CSubConQosR5ParamSet::SetSourceStatisticsDescriptor(RPacketQoS::TSourceStatisticsDescriptor aSrcStatsDescType)
	{
	iSrcStatsDesc = aSrcStatsDescType;
	}
	
#endif 
// SYMBIAN_NETWORKING_UMTSR5

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
	
CSubConSBLPR5ExtensionParamSet* CSubConSBLPR5ExtensionParamSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(KSubCon3GPPExtParamsFactoryUid, KSubConnSBLPR5ExtensionParamsType);
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
// _USCL_QOS3GPP_SUBCONPARAMS_INL__

