// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// NETMCPR.INL
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_NETMCPR_INL
#define SYMBIAN_NETMCPR_INL

#ifdef SYMBIAN_NETWORKING_UPS

//
// Inline methods
//

TBool CUpsNetworkMetaConnectionProvider::ProviderStatusDown() const
	{ return iProviderStatusDown; };

void CUpsNetworkMetaConnectionProvider::SetProviderStatusDown(TBool aStatus)
	{ iProviderStatusDown = aStatus; };

TBool CUpsNetworkMetaConnectionProvider::UpsDisabled() const
	{ return iUpsDisabled; };

void CUpsNetworkMetaConnectionProvider::SetUpsDisabled(TBool aUpsDisabled)
	{ iUpsDisabled = aUpsDisabled; };
	
NetUps::CNetUps* CUpsNetworkMetaConnectionProvider::NetUps()
	{ return iNetUps; }

void CUpsNetworkMetaConnectionProvider::SetNetUps(NetUps::CNetUps* aNetUps)
	{ iNetUps = aNetUps; }

const TDesC& CUpsNetworkMetaConnectionProvider::ApName() const				// retrieve Access Point name
	{ return iApName; }

void CUpsNetworkMetaConnectionProvider::SetApNameL(const TDesC& aApName)	// store Access Point name
	{
	FreeApName();
	iApName.CreateL(aApName);
	}

void CUpsNetworkMetaConnectionProvider::FreeApName()						// free Access Point name storage
	{
	iApName.Close();
	}

TBool CUpsNetworkMetaConnectionProvider::UpsControlClientPresent()
	{
	return iUpsControlClientPresent; 
	}

void CUpsNetworkMetaConnectionProvider::SetUpsControlClientPresent()
	{
	iUpsControlClientPresent = ETrue;
	}

#endif //SYMBIAN_NETWORKING_UPS

#endif //SYMBIAN_NETMCPR_INL
