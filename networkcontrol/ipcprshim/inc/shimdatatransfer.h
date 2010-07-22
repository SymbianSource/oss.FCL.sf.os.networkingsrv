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
*
*/



/**
 @file SHIMDATATRANSFER.H
 @internalComponent
*/

#if !defined(__SHIMDATATRANSFER_H__)
#define __SHIMDATATRANSFER_H__

#include <e32def.h>
#include <e32base.h>
#include "ss_datatransfer.h"

class CConnectionProvdBase;
class CNifManSubConnectionShim;
class CConnDataTransferShim : public CConnDataTransfer
	{
public:
	CConnDataTransferShim(CNifManSubConnectionShim& aSubConnectionProviderShim);
	~CConnDataTransferShim();
	
public:
	//up-calls from the CNifManSubConnectionShim
	TInt NotifyDataTransferred(TUint aUplinkVolume, TUint aDownlinkVolume);
	TInt NotifyDataSent(TUint aUplinkVolume);
	TInt NotifyDataReceived(TUint aDownlinkVolume);

protected:
	//down-calls clients requests
	virtual TInt DoDataTransferred(TUint& aUplinkVolume, TUint& aDownlinkVolume);
	virtual TInt DoDataTransferredCancel();
	virtual TInt DoDataSentNotificationRequest(TUint aRequestedGranularity, TUint aRequestedNotificationVolume);
	virtual TInt DoDataSentNotificationCancel();
	virtual TInt DoDataReceivedNotificationRequest(TUint aRequestedGranularity, TUint aRequestedNotificationVolume);
	virtual TInt DoDataReceivedNotificationCancel();

protected:
	TInt CalculateNewUplinkGranularity(TUint aRequestedGranularity);
	TInt CalculateNewDownlinkGranularity(TUint aRequestedGranularity);

	TInt SetNextUplinkGranularity();
	TInt SetNextDownlinkGranularity();

	TInt SetUplinkGranularity(TUint aRequestedGranularity);
	TInt SetDownlinkGranularity(TUint aRequestedGranularity);

protected:
	static TInt CalculateNewGranularity(TInt aRequestedGranularity, TUint aCurrentGranularity, RArray<TUint>& aGranularities, TUint& aNewGranularity);

private:
	CNifManSubConnectionShim& iSubConnectionProviderShim;
	
	RArray<TUint> iUplinkGranularities;		// ordered lists of deltas for the granularities
	RArray<TUint> iDownlinkGranularities;

	TUint iCurrentUplinkGranularity;		// the aggregate notification granularity calculated from all the CConnection requests
	TUint iCurrentDownlinkGranularity;
	};
	
#endif	// __SHIMDATATRANSFER_H__
