// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @publishedPartner
 @released since 9.5
*/

#include "mbmsparams.h"
#include <e32std.h>
#include <comms-infras/ss_log.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <comms-infras/cs_connservparams.h>
#include <comms-infras/metatypearray.h>

using namespace ConnectionServ;
using namespace Meta;

#ifdef __CFLOG_ACTIVE
#define KMBMSTierMgrTag KESockConnServTag
_LIT8(KConnservMBMSParamsTag, "MBMSParams");
#endif

START_ATTRIBUTE_TABLE( XMBMSServiceParameterSet,KConnectionServMBMSParamImplUid, KMBMSParameterSetType )
	REGISTER_ATTRIBUTE( XMBMSServiceParameterSet,iServiceInfo,TMeta<TMBMSChannelInfoV1> )
	REGISTER_ATTRIBUTE( XMBMSServiceParameterSet,iServiceMode, TMetaNumber )
	REGISTER_ATTRIBUTE( XMBMSServiceParameterSet,iAvailabilityStatus, TMetaNumber )
END_ATTRIBUTE_TABLE()

START_ATTRIBUTE_TABLE( XMBMSServiceQuerySet,KConnectionServMBMSParamImplUid, KMBMSQuerySetType )
	REGISTER_ATTRIBUTE( XMBMSServiceQuerySet,iQueryType,TMeta<TQueryType> )
	REGISTER_ATTRIBUTE( XMBMSServiceQuerySet,iBearerAvailability, TMetaNumber )
	REGISTER_ATTRIBUTE( XMBMSServiceQuerySet,iCurrentCount, TMetaNumber )
	REGISTER_ATTRIBUTE( XMBMSServiceQuerySet,iMaxCount, TMetaNumber )
END_ATTRIBUTE_TABLE()

// Define the interface UIDs
const TImplementationProxy ImplementationTable[] =
	{
	IMPLEMENTATION_PROXY_ENTRY(KConnectionServMBMSParamImplUid, CConnectionServMBMSParamsFactory::NewL),
	IMPLEMENTATION_PROXY_ENTRY(KSubConChannelParamsImplUid, CSubConChannelParamsFactory::NewL),
	IMPLEMENTATION_PROXY_ENTRY(KSubConMBMSExtParamsImplUid, CSubConMBMSExtParamsFactory::NewL),
	};

/**
ECOM Implementation Factories
*/

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

    return ImplementationTable;
    }

/**
Plugin Implementation
*/
XConnectionServParameterSet* CConnectionServMBMSParamsFactory::NewL(TAny* aConstructionParameters)
	{
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);
	XConnectionServParameterSet* retval = NULL;

	switch (type)
		{
		case KMBMSParameterSetType:
			retval = new (ELeave) XMBMSServiceParameterSet;
		break;

		case KMBMSQuerySetType:
			retval = new (ELeave) XMBMSServiceQuerySet;
		break;

		default:
			User::Leave(KErrNotFound);
		}

	return retval;
	}

START_ATTRIBUTE_TABLE( CSubConMBMSExtensionParamSet, KSubConMBMSExtParamsImplUid, KSubConMBMSSessionExtParamsType )
	REGISTER_ATTRIBUTE(CSubConMBMSExtensionParamSet, iSessionIds, TMetaArray<TUint>)
	REGISTER_ATTRIBUTE(CSubConMBMSExtensionParamSet, iServiceMode, TMeta<TMbmsServiceMode>)
	REGISTER_ATTRIBUTE(CSubConMBMSExtensionParamSet, iOperationType, TMeta<CSubConMBMSExtensionParamSet::TOperationType>)
END_ATTRIBUTE_TABLE()
/**
Plugin Implementation
*/
CSubConExtensionParameterSet* CSubConMBMSExtParamsFactory::NewL(TAny* aConstructionParameters)
	{
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);
	switch (type)
		{
	case (KSubConMBMSSessionExtParamsType):
 		return new(ELeave) CSubConMBMSExtensionParamSet;
 		
	default:
		User::Leave(KErrNotFound);
		}
	return NULL;
	}


START_ATTRIBUTE_TABLE( CSubConChannelParamSet, KSubConChannelParamsImplUid, KSubConChannelParamsType )
	REGISTER_ATTRIBUTE(CSubConChannelParamSet, iServiceInfo, TMeta<GenericScprParameters::TChannel >)
END_ATTRIBUTE_TABLE()
/**
Plugin Implementation
*/
CSubConGenericParameterSet*  CSubConChannelParamsFactory::NewL(TAny* aConstructionParameters)
	{
	
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);
     switch (type)
        {
     case (KSubConChannelParamsType):
         return new (ELeave) CSubConChannelParamSet;
         
        }
       return NULL; 
	}


/*virtual*/XMBMSServiceParameterSet::~XMBMSServiceParameterSet()
/**
   Standard destructor.
 */
	{
	}

/*virtual*/CSubConChannelParamSet::~CSubConChannelParamSet()
/**
   Standard destructor.
 */
	{
	}

/*virtual*/CSubConMBMSExtensionParamSet::~CSubConMBMSExtensionParamSet()
	{
	iSessionIds.Close();
	}

/*virtual*/XMBMSServiceQuerySet::~XMBMSServiceQuerySet()
/**
   Standard destructor.
 */
	{
	}	
