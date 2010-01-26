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
//

#include "DHCPConfigListener.h"
#include "DHCPControl.h"
#include <networking/ipeventtypes.h>

CDHCPConfigListener* CDHCPConfigListener::NewL(TName& aInterfaceName, CDHCPControl& aControl)
	{
	CDHCPConfigListener* obj = new (ELeave) CDHCPConfigListener(aControl);
	CleanupStack::PushL(obj);
	obj->ConstructL(aInterfaceName);
	CleanupStack::Pop();
	return obj;
	}


/**
 Constructor.
*/
CDHCPConfigListener::CDHCPConfigListener(CDHCPControl& aControl)
:	iControl(aControl), iEventLinklocal(this, LinklocalAddressSignalHandlerFn)
	{
	/*do nothing*/
	};


void CDHCPConfigListener::ConstructL(TName& aInterfaceName)
	{
	// register for events
	TRAP_IGNORE(SubscribeL(aInterfaceName, IPEvent::ELinklocalAddressKnown, iEventLinklocal));
	}


/**
 Destructor
*/
CDHCPConfigListener::~CDHCPConfigListener()
	{
	if (iNetSubscribe)
		{
		iEventLinklocal.Cancel(*iNetSubscribe);
		}
	};


TBool CDHCPConfigListener::HaveLinkLocal()
	{
	return iLinkLocalAddresses ? ETrue : EFalse;
	}



/**
description.
@return	
@param
@leave <error> description
*/
void CDHCPConfigListener::LinklocalAddressSignalHandlerFn(TAny* aThis, const Meta::SMetaData* /*aData*/)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPConfigListener::LinklocalSignalHandlerFn()")));
	CDHCPConfigListener* inst = reinterpret_cast<CDHCPConfigListener*>(aThis);
// uncomment this if we need to extract the address
//	const IPEvent::CLinklocalAddressKnown* msg = static_cast<const IPEvent::CLinklocalAddressKnown*>(aData);

	inst->iLinkLocalAddresses ++;

	inst->iControl.LinkLocalCreated();
	}
