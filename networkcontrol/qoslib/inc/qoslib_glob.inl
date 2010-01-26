// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef __QOSLIB_GLOB_INL__
#define __QOSLIB_GLOB_INL__

inline CQoSParameters& CChannel::GetPolicy()
	{ return iPolicy; }

inline TUint CChannel::Capabilities() const
	{ return iCapabilities; }

inline TInt CChannel::ChannelId() const
	{ return iChannelId; };

inline RSocket& CQoSMan::Socket()
	{ return iSocket; }

inline const TCheckedUid& CQoSMan::Uid() const
	{ return iUid; }

inline void CQoSMan::Open()
	{ iRefCount++; }

inline void QoSManGlobals::Set(CQoSMan* aGlobals)
	{ Dll::SetTls(aGlobals); }

inline CQoSMan* QoSManGlobals::Get()
	{ CQoSMan* manager=(CQoSMan *)Dll::Tls(); return manager; }

#endif
