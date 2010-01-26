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
// apibase.h - Definitions for exporting APIs from TCP/IP stack to protocol hooks
// Definitions for exporting APIs from TCP/IP stack to protocol hooks
//



/**
 @file apibase.h
 @publishedPartner
 @released
*/

#ifndef __APIBASE_H__
#define __APIBASE_H__

#include "inet6err.h"

/**
* This interface class provides capability of introducing new interfaces for existing classes
* in TCP/IP stack to be used by protocol hooks.
* With this class, protocol hooks can query whether an interface is
* supported and then use the interface. Backwards compatibility of revised interfaces is allowed by
* versioning the interfaces. This enables independent developement of protocol hooks
* and the TCP/IP stack while maintaining the compatibility towards older implementations.
* This should be used as base class for all interface classes that are visible outside
* the TCP/IP stack, or that should be prepared to export an API at some time to allow making
* compatible modifications later on.
*
* There should be a constant definition of form KApiVer_<name> for each API that is exported that
* identifies the version number of current API.
*
* @publishedPartner
* @released
*/
class MInetBase
{
public:
	/**
	* Returns a pointer to object that implements the requested API. If the the class that
	* processes this call does not recognize the API name, it leaves with KErrNotFound.
	* New versions
	* of the same API should maintain binary backwards compatibility. That is, the caller
	* requests a minimal accepted version, and if the implemented API is of the same version or
	* higher, an object should be returned. If the implemented version is smaller than what
	* requested, implementation of GetApiL leaves with KErrInetUnsupportedApi.
	* If API is changed in a way that does not maintain backwards
	* compatibility, a new API name should be allocated for it.
	*
	* Note: Callers should not use this method directly, but are strongly recommended to use
	* IMPORT_API_L macro, which checks for type safety. Additionally, there is IMPORT_API_VER_L
	* macro which can be used to explicitly request a known version and to return the actual
	* version implemented.
	*
	* <em> For example: </em>
	* @verbatim
	* MEventService *es = IMPORT_API_L(NetworkService()->Interfacer(), MEventService);
	* @endverbatim
	*
	* There should be MEventService header somewhere visible to the caller.
	*
	* @param aApiName API name that needs to be known by the implementator
	* @param aVersion Requested minimum version. Later versions are also accepted by the caller.
	*		  Overwritten by the actual version of the API when function returns,
	*		  if matching API was found.
	*
	* @return Pointer to the object that implements the API. The method leaves with error if the
	*	  instance could not be returned.
	*
	* @exception KErrInetUnsupportedApi Name of the API is not supported by the implementing class.
	* @exception KErrInetUnsupportedApiVersion The given API is implemented, but the version is
	*		  incompatible, i.e. older than requested.
	*/
	virtual void *GetApiL(const TDesC8& /*aApiName*/, TUint* /*aVersion*/)
	      { User::Leave(KErrInetUnsupportedApi); return NULL; }

	/**
	* This is similar to GetApiL with returned version, but this function does not return the
	* actual version implemented, so it can be used with constant parameters also.
	*/
	inline void *GetApiL(const TDesC8& aApiName, TUint aVersion)
	      { return GetApiL(aApiName, &aVersion); }
};


/**
* \def IMPORT_API_VER_L(base,api,version)
* Returns the pointer to object that implements requested API. \a base is the class from which
* the API is requested. \a api is the name of the virtual class that defines the API.
* \a version is the minimum version that is requested. Later versions are required to be backwards
* compatible, so the returned version number can be larger. See MInetBase description for more info.
*
* Note that IMPORT_API_VER_L does User::Leave, if the requested API was not available.
*/
#define IMPORT_API_VER_L(base,api,version) (api *)base->GetApiL(_L8(#api), version)

/**
* \def IMPORT_API_L
* Returns the pointer to object that implements requested API. Version is not given but it is
* determined by looking for constant definition KApiVer_<name> in the header files that tells
* the current version of the API.
* \a base is the class from which the API is requested.
* \a api is the name of the virtual class that defines the API. 
*/
#define IMPORT_API_L(base,api) (api *)base->GetApiL(_L8(#api), KApiVer_##api);


/**
* This is a helper class for checking the API type safety for objects that provide an API.
* The static class should not be used directly, but instead through EXPORT_API_L macro.
*
* @publishedPartner
* @released
*/
template<class T> class ApiChecker
{
public:
	/**
	* Returns an instance of the given API (template class T), if the current version is
	* equal or above the version that was requested. New versions of the API
	* are required to be (binary) backwards compatible. If version does not match, leaves with
	* KErrInetUnsupportedApiVersion.
	*
	* @param aCurVersion  Current version of the API implementation
	* @param aVersion     Version requested by the caller
	* @param aInstance    API implementation that is returned if it is of valid version.
	*/
	static T* CheckVersionL(TUint aCurVersion, TUint *aVersion, T *aInstance);
};

template<class T> T* ApiChecker<T>::CheckVersionL(TUint aCurVersion,
						  TUint *aVersion, T *aInstance)
{
	if (*aVersion > aCurVersion)
	  {
	    *aVersion = aCurVersion;
	    User::Leave(KErrInetUnsupportedApiVersion);
	  }
	
	*aVersion = aCurVersion;
	return aInstance;
}


/**
* \def EXPORT_API_L(api,instance,version)
* To be used in MInetBase::GetApiL implementation for checking if an API and its version matches
* the GetApiL request. Returns a pointer to API instance if the API matches.
* Leaves with KErrInetUnsupportedApiVersion if the requested version is not supported,
* i.e. its later than what is implemented.
*
* \a api is the class defining the API that is checked for. \a instance is the pointer that is
* returned if API matches.
* \a version is the minimum API version that is required. The actual version of API is
* returned as output
*
* <em> Example of a GetApiL implementation: </em>
* @verbatim
* void *CIp6Manager::GetApiL(TDesC8 aApiName, TUint* aVersion)
*	{
*
*	if (aApiName == _L8("MEventService"))
*	    return EXPORT_API_L(MEventService, iEventManager, aVersion);
*
*	if (aApiName == _L8("MNetworkInfo"))
*	    return EXPORT_API_L(MNetworkInfo, this, aVersion);
*
*	User::Leave(KErrNotFound);
*	return NULL;
*	}
* @endverbatim
*/
#define EXPORT_API_L(api,instance,version) \
	ApiChecker<api>::CheckVersionL(KApiVer_##api, version, instance)

#endif // __APIBASE_H__
