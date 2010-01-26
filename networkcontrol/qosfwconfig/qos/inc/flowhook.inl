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
#ifndef __FLOWHOOK_INL__
#define __FLOWHOOK_INL__

inline void CFlowHook::SetUid(const TUidType& aUid)
	{ iUid.Set(aUid); };

inline const TCheckedUid& CFlowHook::Uid() const
	{ return iUid; };

inline TQoSPolicy& CFlowHook::Policies()
	{ return iQoSPolicies; };

inline CInterface* CFlowHook::Interface()
	{ return iInterface; }

inline CFlowContext& CFlowHook::Context()
	{ return iContext; }

inline CProtocolQoS& CFlowHook::Protocol()
	{ return iProtocol; }

inline const TQoSParameters& CFlowHook::QoSParameters() const
	{ return iQoS; }

inline void CFlowHook::SetQoSChannel(CQoSConn* aChannel)
	{ iChannel = aChannel; }

inline CQoSConn* CFlowHook::Channel()
	{ return iChannel; }

inline TBool CFlowHook::ChannelJoined()
	{ return iChannelJoined == 1; }

inline void CFlowHook::SetChannelJoined(TBool aJoined)
	{ iChannelJoined = aJoined;	}
#endif
