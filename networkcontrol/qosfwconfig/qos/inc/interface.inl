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
//



/**
 @internalComponent
*/
#ifndef __QOS_INTERFACE_INL__
#define __QOS_INTERFACE_INL__

inline RModule* CInterface::TrafficControl()
	{ return iTrafficControl; };

inline void CInterface::SetTrafficControl(RModule* aModule)
	{ iTrafficControl = aModule; };
	
inline TInt CInterface::TrafficControlStatus() const
	{ return iTrafficControlStatus; }


inline CProtocolQoS& CInterface::Protocol()
	{ return iProtocol; };

inline const TDesC& CInterface::Name() const
	{ return iName; };

inline const TDesC& CInterface::InterfaceName() const
	{ return iInterfaceName; };

inline TUint32 CInterface::IapId() const
	{ return iIapId; };

inline CNifIfBase* CInterface::Nif()
	{ return iNif; };

#endif

