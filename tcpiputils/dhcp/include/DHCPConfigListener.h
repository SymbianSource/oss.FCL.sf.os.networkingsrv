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
// The Listeners header file.
// Listens to the connection state to perform specific actions
// based on its status.
// 
//

/**
 @file DHCPConfigListener.h
*/

#ifndef __DHCPCONFIGLISTENER_H__
#define __DHCPCONFIGLISTENER_H__


#include "DHCPStates.h"
#include <comms-infras/netsignalevent.h>


class CDHCPControl;

/**
 * Base class for Listeners
 * 
 * @internalTechnology
 */
class CDHCPConfigListener : public CBase, public SDhcpSignal
	{
public:
	static CDHCPConfigListener* NewL(TName& aInterfaceName, CDHCPControl& aControl);
	virtual ~CDHCPConfigListener();
	
	static void LinklocalAddressSignalHandlerFn(TAny* aThis, const Meta::SMetaData* aData);
	TBool HaveLinkLocal();

private:
	CDHCPConfigListener(CDHCPControl& aControl);
	void ConstructL(TName& aInterfaceName);
	CDHCPControl &iControl;
	NetSubscribe::TEvent iEventLinklocal;
	TInt iLinkLocalAddresses;
	};
	
#endif // __DHCPCONFIGLISTENER_H__
