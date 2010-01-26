/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file CNetworkControllerBase.h
 @internalTechnology
*/

#if !defined (__CNETWORKCONTROLLERBASE_H__)
#define __CNETWORKCONTROLLERBASE_H__

#include <cdbstore.h>
#include <es_prot.h>

class MNetworkControllerObserver
/**
@internalTechnology
*/
	{
public:
	virtual void SelectComplete(const TDesC& aName) = 0;
	virtual void SelectComplete(TInt aError) = 0;
	virtual void ReconnectComplete(TInt aError) = 0;
	};

#endif // __CNETWORKCONTROLLERBASE_H__

