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
// llmnrnotifyhandler.h - Link-local multicast name resolution 
// notify handler module header
//

#ifndef _LLMNRNOTIFYHANDLER_H_
#define _LLMNRNOTIFYHANDLER_H_

/**
@file llmnrnotifyhandler.h
LLMNR interface monitoring
@internalComponent	Domain Name Resolver
*/
#include "timeout.h"

class CDndLlmnrResponder;
class CDndLlmnrNotifyHandler : public CBase
	/**
	* Handles notifications of interface changes for the LLMNR
	*/
    {
public:
    CDndLlmnrNotifyHandler(CDndLlmnrResponder &aMaster);
    ~CDndLlmnrNotifyHandler();
    void ConstructL();
    void ConfigurationChanged();
    void Timeout(const TTime &aNow);
	RTimeout iTimeout;			//< Hook to the timer service (MTimeoutManager)
private:
    void ScanInterfaces();
	TInt IsLlmnrDisabledL(TUint32 aIap);
    CDndLlmnrResponder &iMaster;
    };

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KDndLlmnrNotifyHandlerTimeoutOffset 4
__ASSERT_COMPILE(KDndLlmnrNotifyHandlerTimeoutOffset == _FOFF(CDndLlmnrNotifyHandler, iTimeout));
#else
#define KDndLlmnrNotifyHandlerTimeoutOffset _FOFF(CDndLlmnrNotifyHandler, iTimeout)
#endif

//
//	CLlmnrNotifyHandlerTimeoutLinkage
//	**************************
//	Glue to bind timeout callback from the timeout manager into Timeout() call
//	on the CDndLlmnrNotifyHandler
//  Copied from CDndResolverTimeoutLinkage
class CLlmnrNotifyHandlerTimeoutLinkage : public TimeoutLinkage<CDndLlmnrNotifyHandler, KDndLlmnrNotifyHandlerTimeoutOffset>
	{
public:
	static void Timeout(RTimeout &aLink, const TTime &aNow, TAny * /*aPtr*/)
		{
		Object(aLink)->Timeout(aNow);
		}
	};
#endif
