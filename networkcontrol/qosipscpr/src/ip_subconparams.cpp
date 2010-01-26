// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation file for the IP SubConnection Parameters
// 
//

/**
 @file ip_subconparams.h
*/


#include <e32std.h>
#include <e32test.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <ip_subconparams.h>
#include <comms-infras/metatypearray.h>

#ifndef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
// need to ensure the original class name and TUid of the factory class
// is used for implementions in this component
#undef KSubConIPParamsUid
#undef KSubConQosIPLinkR99ParamsType
#undef CSubConQosIPLinkR99ParamSet
#endif

START_ATTRIBUTE_TABLE( CSubConQosIPLinkR99ParamSet, KSubConIPParamsUid, KSubConQosIPLinkR99ParamsType )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iTrafficClass, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iDeliveryOrder, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iDeliveryOfErroneusSdu, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iResidualBer, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iErrorRatio, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iPriority, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iTransferDelay, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iMaxSduSize, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iMaxBitrateUplink, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iMaxBitrateDownlink, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iGuaBitrateUplink, TMetaNumber )
	REGISTER_ATTRIBUTE( CSubConQosIPLinkR99ParamSet, iGuaBitrateDownlink, TMetaNumber )
END_ATTRIBUTE_TABLE()

START_ATTRIBUTE_TABLE( CSubConSBLPR5ExtensionParamSet, KSubConIPParamsUid, KSubConnSBLPR5ExtensionParamsType )
	REGISTER_ATTRIBUTE( CSubConSBLPR5ExtensionParamSet, iAuthToken, TMeta<TAuthToken>)
	REGISTER_ATTRIBUTE( CSubConSBLPR5ExtensionParamSet, iFlowIds, TMetaArray<RFlowIdentifiers>)
END_ATTRIBUTE_TABLE()

CSubConExtensionParameterSet* CSubConIPExtensionParamsFactory::NewL(TAny* aConstructionParameters)
	{
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);
	switch (type)
		{
	case (KSubConQosIPLinkR99ParamsType):
		return new (ELeave) CSubConQosIPLinkR99ParamSet;
		// break;
	case (KSubConnSBLPR5ExtensionParamsType):
		return new (ELeave) CSubConSBLPR5ExtensionParamSet;
		// break;
	default:
		User::Leave(KErrNotFound);
		}
	return NULL;
	}

