/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* CNifIfBase and CProtocolBase shim layer functionality
* 
*
*/



/**
 @file nif.cpp
*/

#if !defined(NIF6_H_INCLUDED)
#define NIF6_H_INCLUDED
#include <e32base.h>
#include <comms-infras/ss_protflow.h>
#include "nif.h"
#include <comms-infras/nifif.h>
#include <comms-infras/es_protbinder.h>

class CIPShimIncoming;
class CIPShimSubConnectionFlow;
class CIPShimIfBase6 : public CIPShimIfBase
	{
/**
Class that represents an IP6 NIF to the TCP/IP stack.  

@internalComponent
*/
public:
	static CIPShimIfBase6* NewL(const TDesC8& aProtocolName);
	
	// IP4/IP6 specific required derivations
	virtual void GetConfigFirstTime();

protected:
	CIPShimIfBase6(const TDesC8& aProtocolName);

	// IP4/IP6 specific required derivations
	virtual TInt ServiceInfoControl(TDes8& aOption, TUint aName);
	virtual TInt ServiceConfigControl(TDes8& aOption);

private:
	TBinderConfig6	iConfig6;		// stored address information from BinderReady() upcall	
	};

#endif // NIF6_H_INCLUDED
