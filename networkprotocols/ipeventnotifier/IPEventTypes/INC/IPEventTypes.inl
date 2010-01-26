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
//

/**
 @file ipeventtypes.inl
 @publishedPartner
 @released
*/


#ifndef __IPEVENTTYPES_INL__
#define __IPEVENTTYPES_INL__


inline TBool CMFlagReceived::GetMFlag() const
	{
	return iMFlag;
	}


inline void CMFlagReceived::SetMFlag(TBool aNewVal)
	{
	iMFlag = aNewVal;
	}

inline TBool CMFlagReceived::GetOFlag() const
	{
	return iOFlag;
	}


inline void CMFlagReceived::SetOFlag(TBool aNewVal)
	{
	iOFlag = aNewVal;
	}

inline const TInetAddr & CIPReady::GetIPAddress() const
	{
	return iIPAddress;
	}


inline void CIPReady::SetIPAddress(const TInetAddr& aAddr)
	{
	iIPAddress = aAddr;
	}


inline TBool CIPReady::GetAddressValid() const
	{
	return iAddressValid;
	}


inline void CIPReady::SetAddressValid(TBool aAddrValid)
	{
	iAddressValid = aAddrValid;
	}


inline const TInetAddr& CLinklocalAddressKnown::GetIPAddress() const
	{
	return iIPAddress;
	}


inline void CLinklocalAddressKnown::SetIPAddress(const TInetAddr& aAddr)
	{
	iIPAddress = aAddr;
	}


#endif	// __IPEVENTTYPES_INL_

