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

#ifndef __MBMSPARAMS_INL__
#define __MBMSPARAMS_INL__

#include <e32debug.h>

// implementation, to be copied to .inl file

namespace ConnectionServ
{  
TMBMSInfo* TMBMSChannelInfoV1::AddrPtr() 
/** 
    Gets a pointer to the buffer TChannel.
	
	@return TMBMSInfo*
*/
    {
    return reinterpret_cast<TMBMSInfo*>(UserPtr());
    } 

TMBMSChannelInfoV1::TMBMSChannelInfoV1()
/**
   Standard constructor.
*/
	{
	SetUserLength(sizeof(TMBMSInfo));
	}
	
void TMBMSChannelInfoV1::SetTmgi(const TTmgi& aTmgi)
/** 
    Setter method to set TTmgi
	
	@param aTmgi a reference to the existing TTmgi. 
*/
	{
	AddrPtr()->iTmgi = aTmgi;
	}

const TTmgi& TMBMSChannelInfoV1::GetTmgi() 
/** 
    Getter method to get TTmgi.
	
	@return TTmgi&.
*/
	{
	return AddrPtr()->iTmgi;
	}
	
void TMBMSChannelInfoV1::SetScope(const TMbmsScope aMbmsScope)
/** 
    Setter method to set MbmsScope
	
	@param aTmgiScope a reference to the existing TTmgiScope. 
*/
	{
	AddrPtr()->iMbmsScope = aMbmsScope;
	}
const TMbmsScope TMBMSChannelInfoV1::GetScope() 
/** 
    Getter method to get TTmgiScope.
	
	@return TMbmsScope&.
*/
	{
	return AddrPtr()->iMbmsScope;
	}		
const TInetAddr& TMBMSChannelInfoV1::GetInetAddress() 
/** 
    Getter method to retrieve IP address.This function is implemented for later use to
	set multicast address.
	
	@return TInetAddr&.
*/
	{
	return AddrPtr()->iInetAddr;
	}
	
void TMBMSChannelInfoV1::SetInetAddress(const TInetAddr& aInetAddr)
/** 
    Setter method to set IP address.This function is implemented for later use.
	
	@param aInetAddr a reference to the existing TInetAddr. 
*/
	{
	AddrPtr()->iInetAddr = aInetAddr;
	}
	
TMbmsServicePriority TMBMSChannelInfoV1::GetServicePriority() 
/** 
    Getter method to retrieve service priority.
	
	@return TMbmsServicePriority 
*/
 	{
 	return AddrPtr()->iServicePriority;
 	}
 	
void TMBMSChannelInfoV1::SetServicePriority(const TMbmsServicePriority aServicePriority)
/** 
    Setter method to set service priority.
	
	@param aServicePriority of type TMbmsServicePriority
*/
	{
	AddrPtr()->iServicePriority = aServicePriority;
	}


XMBMSServiceParameterSet::XMBMSServiceParameterSet()
/**
   Standard constructor.
 */
	{
	}
TMbmsServiceMode XMBMSServiceParameterSet::GetServiceMode() const
/** 
    Getter method to get MBMS service mode.
	
	@return TMbmsServiceMode 
*/
    {
    return iServiceMode;
    }
void XMBMSServiceParameterSet::SetServiceMode(const TMbmsServiceMode  aServiceMode)
/** 
    Setter method to set MBMS service mode.
	
	@param aServiceMode of type TMbmsServiceMode
*/
	{
	iServiceMode = aServiceMode;
	}
TMbmsAvailabilityStatus XMBMSServiceParameterSet::GetMBMSServiceAvailability() const
/** 
    Getter method to get MBMS service availability
		
	@return TMbmsAvailabilityStatus. 
*/
	{
    return iAvailabilityStatus;
	}
void XMBMSServiceParameterSet::SetMBMSServiceAvailability(const TMbmsAvailabilityStatus aAvailabilityStatus)
/** 
    Setter method to set MBMS service availability
	
	@param aAvailabilityStatus of type TMbmsAvailabilityStatus.
*/	
	{
	iAvailabilityStatus = aAvailabilityStatus;
	}
 TMBMSChannelInfoV1* XMBMSServiceParameterSet::GetChannelInfo() 
/** 
    Getter method to get MBMS service information.
		
	@return TMBMSChannelInfoV1& 
*/
	{
	return &iServiceInfo;
	}

void XMBMSServiceQuerySet::SetQueryType(const TQueryType aQueryType)
/** 
    Setter method to set MBMS query type.
	
	@param aQueryType of type TQueryType.
*/
	{
	iQueryType = aQueryType;
	}
XMBMSServiceQuerySet::TQueryType XMBMSServiceQuerySet::GetQueryType() const
/** 
    Getter method to get MBMS query type.
	
	@return TQueryType 
*/
	{
	return iQueryType;
	}

//Checks for MBMS Bearer Availability
TMbmsNetworkServiceStatus XMBMSServiceQuerySet::GetMBMSBearerAvailability() const
/** 
    Getter method to get MBMS bearer availability.
	
	@return TMbmsNetworkServiceStatus.
*/
	{
	return iBearerAvailability;
	}
	
void XMBMSServiceQuerySet::SetMBMSBearerAvailability(const TMbmsNetworkServiceStatus aBearerAvailability)
/** 
    Setter method to set MBMS bearer availability.
	
	@param aBearerAvailability of type TMbmsNetworkServiceStatus.
*/
	{
	iBearerAvailability = aBearerAvailability;
	}
	
TUint XMBMSServiceQuerySet::GetListCount() const
/** 
    Getter method to get number of entries from the service or monitor list.
	
	@return TUint.
*/
	{
	return iCurrentCount;
	}
void XMBMSServiceQuerySet::SetListCount(const TUint aCurrentCount)
/** 
    Setter method to set number of entries in service or monitor list.
	
	@param aCurrentCount of type TUint. 
*/
	{
	iCurrentCount = aCurrentCount;
	}
TUint XMBMSServiceQuerySet::GetListMaxCount() const
/** 
    Getter method to get the maximum entries that a service or monitor list contain.
	
	@return TUint.
*/
	{
	return iMaxCount;
	}

void XMBMSServiceQuerySet::SetListMaxCount(const TUint aMaxCount)
/** 
    Setter method to set the maximum entries that a service or monitor list contain.
	
	@param aMaxCount of type TUint.
*/
	{
	iMaxCount = aMaxCount;
	}

CSubConChannelParamSet *CSubConChannelParamSet ::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
/**
@param      aFamily the sub-connection parameter family to set.
@param      aType The type of the set from TParameterSetType (ERequested)
@return     pointer to the created object
*/
    {
    CSubConChannelParamSet * obj = NewL();
    CleanupStack::PushL(obj);
    aFamily.SetGenericSetL(*obj, aType);
    CleanupStack::Pop(obj);
    return obj;
    }

CSubConChannelParamSet *CSubConChannelParamSet ::NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType)
/**
@param      aFamily the sub-connection parameter family to set.
@param      aType The type of the set from TParameterSetType (ERequested)
@return     pointer to the created object
*/
    {
    CSubConChannelParamSet * obj = NewL();
    CleanupStack::PushL(obj);
    aFamily.AddParameterSetL(obj, aType);
    CleanupStack::Pop(obj);
    return obj;
    }

CSubConChannelParamSet* CSubConChannelParamSet::NewL()
/**
@return     pointer to the created object
*/
    {
    STypeId typeId = STypeId::CreateSTypeId(KSubConChannelParamsImplUid, KSubConChannelParamsType );
    return static_cast<CSubConChannelParamSet*>(CSubConParameterSet::NewL(typeId));
    }


CSubConChannelParamSet::CSubConChannelParamSet()
    {
    }
 


TAny* CSubConChannelParamSet::GetChannelInfo() 
/** Gets the Channel information
@return aServiceInfo value of the channel
*/
    {
    return iServiceInfo.UserPtr();
    }



//Mbms Session Extension Parameters Functions

CSubConMBMSExtensionParamSet * CSubConMBMSExtensionParamSet ::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
/**
@param      aFamily the sub-connection parameter family to set.
@param      aType The type of the set from TParameterSetType (ERequested)
@return     pointer to the created object
*/
    {
    CSubConMBMSExtensionParamSet * obj = NewL();
    CleanupStack::PushL(obj);
    aFamily.AddExtensionSetL(*obj, aType);
    CleanupStack::Pop(obj);
    return obj;
    }

CSubConMBMSExtensionParamSet * CSubConMBMSExtensionParamSet ::NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType)
/**
@param      aFamily the sub-connection parameter family to set.
@param      aType The type of the set from TParameterSetType (ERequested)
@return     pointer to the created object
*/
    {
    CSubConMBMSExtensionParamSet * obj = NewL();
    CleanupStack::PushL(obj);
    aFamily.AddParameterSetL(obj, aType);
    CleanupStack::Pop(obj);
    return obj;
    }


CSubConMBMSExtensionParamSet* CSubConMBMSExtensionParamSet::NewL()
/**
@return     pointer to the created object
*/
    {
    STypeId typeId = STypeId::CreateSTypeId(KSubConMBMSExtParamsImplUid, KSubConMBMSSessionExtParamsType );
    return static_cast<CSubConMBMSExtensionParamSet*>(CSubConParameterSet::NewL(typeId));
    }

//===========================
// Implementation Extension class
CSubConMBMSExtensionParamSet::CSubConMBMSExtensionParamSet()
	: CSubConExtensionParameterSet()
    {
    }



void CSubConMBMSExtensionParamSet::SetSessionId(const TUint aSessionId)
/**         adds a Session Id to the MBMS Service
@param      aFlowId the flow indentifier to be added.
*/
	{
	this->iSessionIds.Append(aSessionId);
	}


TInt CSubConMBMSExtensionParamSet::GetSessionId(const TUint aIndex)
/**
@param      aIndex the index of the flow identifier that will be returned.
@return     the Mbms Session Id at the index given by aIndex.
*/
	{
	return this->iSessionIds[aIndex];
	}


TInt CSubConMBMSExtensionParamSet::GetSessionCount() const
/**

@return the number of SessionIds in the MBMS service
*/
	{
	return this->iSessionIds.Count();
	}


void CSubConMBMSExtensionParamSet::SetServiceMode(const TMbmsServiceMode aServiceMode)
/** Sets the MBMS Service Mode

@param aServiceMode value of the service
*/
	{
	iServiceMode=aServiceMode;
	}

TMbmsServiceMode CSubConMBMSExtensionParamSet::GetServiceMode() const
/**
@return the MBMS service mode.
*/
	{
	return iServiceMode;
	}


CSubConMBMSExtensionParamSet::TOperationType CSubConMBMSExtensionParamSet::GetOperationType () const
/**
@return the MBMS Extension Set class operation type
*/
	{
	return iOperationType;
	}

void CSubConMBMSExtensionParamSet::SetOperationType(CSubConMBMSExtensionParamSet::TOperationType aOperationType)
/** Sets the MBMS Extension Set Operation Type
@param aOperationType value 
*/
	{
	iOperationType = aOperationType;
	}

} // namespace ConnectionServ

#endif	// __MBMSPARAMS_INL__
