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
 @file SHIMDATATRANSFER.CPP
*/

#include "shimnifmansconn.h"
#include "shimdatatransfer.h"
#include "es_prot.h" //CConnectionProvdBase alias NIFMAN




CConnDataTransferShim::CConnDataTransferShim(CNifManSubConnectionShim& aSubConnectionProviderShim) :
	iSubConnectionProviderShim(aSubConnectionProviderShim)
	{
	}
		
CConnDataTransferShim::~CConnDataTransferShim()
	{
	iUplinkGranularities.Close();
	iDownlinkGranularities.Close();
	}
		

TInt CConnDataTransferShim::DoDataTransferred(TUint& aUplinkVolume, TUint& aDownlinkVolume)
	{
	// Find out the amount of data transferred from the connection provider
	TInt ret = iSubConnectionProviderShim.Provider().DataTransferred(iSubConnectionProviderShim.Id(), aUplinkVolume, aDownlinkVolume);

	// Let the subinterface know in case any subconnections have outstanding data notification requests that might be completed by this
	NotifyDataTransferred(aUplinkVolume, aDownlinkVolume);

	// And return the results to the connection
	return(ret);
	}
	
TInt CConnDataTransferShim::DoDataTransferredCancel()
	{
	// do nothing
	return KErrNone;
	}
	
TInt CConnDataTransferShim::DoDataSentNotificationRequest(TUint aRequestedGranularity, TUint aRequestedNotificationVolume)
	{
	TInt requiredGranularity;

	if(aRequestedGranularity == 0)	// absolute mode
		{
		// Start by finding out how much data has been sent, then calculate granularity
		TUint uplinkDataVolume;
		TUint dummyDataVolume;

		DoDataTransferred( uplinkDataVolume, dummyDataVolume);

		requiredGranularity = aRequestedNotificationVolume - uplinkDataVolume;
		if(requiredGranularity < 0)	// we've already achieved this so notify immediately
			{
			// possible optimisation: pass reference to caller in here and 
            // only notify them if this condition is met
            // essentially we don't know what the granularity was here
			for(TInt i=0; i < iClients.Count(); i++)
				{
				iClients[i]->NotifyDataSent(uplinkDataVolume, 0);
				}
			}
		}
	else	// relative mode
		{
		requiredGranularity = aRequestedGranularity;
		}

	return CalculateNewUplinkGranularity(requiredGranularity);
	}
	
TInt CConnDataTransferShim::DoDataSentNotificationCancel()
	{
	// Do nothing.  Not worth trying to adjust granularity array.
	return KErrNone;
	}
	
TInt CConnDataTransferShim::DoDataReceivedNotificationRequest(TUint aRequestedGranularity, TUint aRequestedNotificationVolume)
	{
	TInt requiredGranularity;

	if(aRequestedGranularity == 0)	// absolute mode
		{
		// Start by finding out how much data has been sent, then calculate granularity
		TUint dummyDataVolume;
		TUint downlinkDataVolume;

		DoDataTransferred( dummyDataVolume, downlinkDataVolume);

		requiredGranularity = aRequestedNotificationVolume - downlinkDataVolume;
		if(requiredGranularity < 0)	// we've already achieved this so notify immediately
			{
			// possible optimisation: pass reference to caller in here 
			// and only notify them if this condition is met
            // essentially we don't know what the granularity was here
			for(TInt i=0; i < iClients.Count(); i++)
				{
				iClients[i]->NotifyDataReceived(downlinkDataVolume, 0);
				}
			}
		}
	else	// relative mode
		{
		requiredGranularity = aRequestedGranularity;
		}

	return CalculateNewDownlinkGranularity(requiredGranularity);
	}
	
TInt CConnDataTransferShim::DoDataReceivedNotificationCancel()
	{
	// Do nothing.  Not worth trying to adjust granularity array.
	return KErrNone;
	}

TInt CConnDataTransferShim::NotifyDataTransferred(TUint aUplinkVolume, TUint aDownlinkVolume)
/**
Called as a side-effect of someone calling DataTransferredRequest(), to allow any absolute volume notifications that may be outstanding to be completed if the required amount of data has been sent/received

@param aUplinkVolume The total volume of data sent on this subconnection
@param aDownlinkVolume The total volume of data received on this subconnection
@return KErrNone, or one of the system-wide error codes
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: New data transferred notification (uplink: %d, downlink: %d)"), 
						 iSubConnectionProviderShim.Id(), aUplinkVolume, aDownlinkVolume));

	for(TInt i=0; i < iClients.Count(); i++)
		{
		iClients[i]->NotifyDataTransferred(aUplinkVolume, aDownlinkVolume);
		}

	return KErrNone;
	}

TInt CConnDataTransferShim::NotifyDataSent(TUint aUplinkVolume)
/**
Notification from connection provider via CInterface that the requested granularity for data sent has been met or exceeded

@note The granularity system is not perfect, as it may be the case that we get notifications for more than "granularity" quantity of data.  
@param aUplinkVolume The total volume of data sent so far on this subconnection
@return KErrNone, or one of the system-wide error codes
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: New data sent notification (uplink: %d)"), 
						 iSubConnectionProviderShim.Id(), aUplinkVolume));

	// Pass notification up to all subconnections, including the current granularity setting in case they are in relative notification mode
	for(TInt i=0; i < iClients.Count(); i++)
		{
		iClients[i]->NotifyDataSent(aUplinkVolume, iCurrentUplinkGranularity);
		}

	// Set the new granularity required of the interface
	return SetNextUplinkGranularity();
	}

TInt CConnDataTransferShim::NotifyDataReceived(TUint aDownlinkVolume)
/**
Notification from connection provider via CInterface that the requested granularity for data received has been met or exceeded

@param aDownlinkVolume The total volume of data received so far on this subconnection
@return KErrNone, or one of the system-wide error codes
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: New data received notification (downlink: %d)"), iSubConnectionProviderShim.Id(), aDownlinkVolume));

	// Pass notification up to all subconnections, including the current granularity setting in case they are in relative notification mode
	for(TInt i=0; i < iClients.Count(); i++)
		{
		iClients[i]->NotifyDataReceived(aDownlinkVolume, iCurrentDownlinkGranularity);
		}

	// Set the new granularity required of the interface
	return SetNextDownlinkGranularity();
	}

TInt CConnDataTransferShim::CalculateNewUplinkGranularity(TUint aRequestedGranularity)
/**
Calculate the required granularity to satisfy client requests
This function calculates the delta between client requests and stores it in an array.

@param aRequestedGranularity The new requested granularity
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Quantise requests to granularity of 1K
*/
	{
	TInt ret = KErrNone;
	TInt requestedGranularity = static_cast<TInt>(aRequestedGranularity);
	
	//@todo In the future, this method could be rewritten to expand the maximum
	//granularity from 2GB to 4GB.  Probably not necessary...
	if(requestedGranularity < 0)	// check that the cast didn't produce an invalid result
		{
		__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: ERROR - calculating new uplink granularity - overflow when casting integer"), 
							 iSubConnectionProviderShim.Id()));
		return(KErrOverflow);
		}
	
	__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: Calculating new uplink granularity..."), 
						 iSubConnectionProviderShim.Id()));

	TUint newGranularity = 0;

	ret = CalculateNewGranularity(requestedGranularity, iCurrentUplinkGranularity, iUplinkGranularities, newGranularity);
	if (ret == KErrNone && newGranularity)
		{
		ret = SetUplinkGranularity(newGranularity);
		}
	return(ret);
	}

TInt CConnDataTransferShim::CalculateNewDownlinkGranularity(TUint aRequestedGranularity)
/**
Calculate the required granularity to satisfy client requests
This function calculates the delta between client requests and stores it in an array.

@param aRequestedGranularity The new requested granularity
@return KErrNone if successful, otherwise one of the system-wide error codes
@todo Quantise requests to granularity of 1K
*/
	{
	TInt ret;
	TInt requestedGranularity = static_cast<TInt>(aRequestedGranularity);

	//@todo In the future, this method could be rewritten to expand the maximum 
	//      granularity from 2GB to 4GB.  Probably not necessary...
	if(requestedGranularity < 0)	// check that the cast didn't produce an invalid result
		{
		__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: ERROR - calculating new downlink granularity - overflow when casting integer"), 
							 iSubConnectionProviderShim.Id()));
		return(KErrOverflow);
		}

	__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("ESock: CConnDataTransferShim[id: %d]: Calculating new downlink granularity..."), 
						 iSubConnectionProviderShim.Id()));

	TUint newGranularity = 0;

	ret = CalculateNewGranularity(requestedGranularity, iCurrentDownlinkGranularity, iDownlinkGranularities, newGranularity);
	if (ret == KErrNone && newGranularity)
		{
		ret = SetDownlinkGranularity(newGranularity);
		}
	return(ret);
	}

TInt CConnDataTransferShim::CalculateNewGranularity(TInt aRequestedGranularity, TUint aCurrentGranularity, RArray<TUint>& aGranularities, TUint& aNewCurrentGranularity)
/**
Given a new granularity, calculate new values for the Current granularity and granularity array.

Helper function used for processing new uplink and downlink granularities.

@param aRequestedGranularity new granularity (in).
@param aCurrentGranularity current granularity in the lower provider (in).
@param aGranularities ordered array of delta granularities which will become the current granularity
after the latter has expired (in/out).
@param aNewCurrentGranularity new value of current granularity to be set in lower provider (out).
@return KErrNone or a system wide error code.
*/
	{
	TInt ret = KErrNone;
	TInt i = 0;
	TInt count = aGranularities.Count();		// granularities in the array

	// The reason we have the three way if statement below is because we have to deal with
	// the fact that there is a "current" granularity (set in the lower provider), and 
	// an array of delta granularities which will become the current granularity in turn:
	//
	//  +---+   +---+---+---+
	//  |   |   |   |   |   |... 
	//  +---+   +---+---+---+
	//            0   1   2
	// Current      Array
	//
	// The complexity comes when a new granularity needs to be notionally inserted either before the
	// current granularity or between the current granularity and the first entry in the array.

	if (count == 0 && aCurrentGranularity == 0)
		{
		// Empty granularity array and no current granularity.
		//
		// - set current granularity to the requested granularity (no entries needed in array)
		aNewCurrentGranularity = aRequestedGranularity;
		}
	else
	if (aRequestedGranularity < aCurrentGranularity)
		{
		// Requested granularity is less than current granularity.  The requested granularity
		// needs to be notionally inserted before the current granularity.

		// add a new entry into the beginning of the array that contains the delta between the requested
		// granularity and current granularity.  This new entry represents the "old" current granularity.
		ret = aGranularities.Insert(aCurrentGranularity - aRequestedGranularity, 0);
		if (ret == KErrNone)
			{
			// the requested granularity becomes the new current granularity
			aNewCurrentGranularity = aRequestedGranularity;
			}
		}
	else
	if (aRequestedGranularity > aCurrentGranularity)
		{
		// Requested granularity is greater than current granularity.  The requested granularity
		// needs to be inserted into the array at the appropriate place.  The current granularity
		// remains unchanged.

		// Take into account the current granularity by subtracting it from requested granularity
		aRequestedGranularity -= aCurrentGranularity;
		
		// Find correct insertion position.  Each entry visited in the array will subtract from the
		// requested granularity, leaving the latter as being delta based.
		for(i=0; i < count ; i++)
			{
			aRequestedGranularity -= aGranularities[i];
			if (aRequestedGranularity <= 0)
				{
				break;
				}
			}
			
		if (aRequestedGranularity < 0)
			{
			// Insertion position found within the array - held in "i".
			aRequestedGranularity += aGranularities[i];
			ret = aGranularities.Insert(aRequestedGranularity, i);
			if (ret == KErrNone)
				{
				// Adjust the next granularity in the array by the Requested granularity
				// that we've just inserted.  "i+1" because of the Insert() above.
				aGranularities[i+1] -= aRequestedGranularity;
				}
			}
		else
		if (aRequestedGranularity > 0)
			{
			// Reached end of the array while searching - insert requested granularity at the end.
			ret = aGranularities.Append(aRequestedGranularity);
			}
		// aRequestedGranularity == 0 is a no-op case (setting zero granularity).
		}
	// (aRequestedGranularity == iCurrentUplinkGranularity) is a no-op case (setting the same
	// granularity as current value).
	return (ret);
	}


TInt CConnDataTransferShim::SetUplinkGranularity(TUint aRequestedGranularity)
/**
Set data sent notification granularity in lower provider.

@param aRequestedGranularity granularity to set.  If zero, cancel data sent notifications.
@return KErrNone or a system wide error code.
*/
	{
	iCurrentUplinkGranularity = aRequestedGranularity;
	if (iCurrentUplinkGranularity)
		{
		return iSubConnectionProviderShim.Provider().SetDataSentNotificationGranularity(iSubConnectionProviderShim.Id(), iCurrentUplinkGranularity);
		}
	else
		{
		return iSubConnectionProviderShim.Provider().DataSentNotificationCancel(iSubConnectionProviderShim.Id());
		}
	}

TInt CConnDataTransferShim::SetNextUplinkGranularity()
/**
Get the next uplink granularity from the array, and send it to the connection provider

@return KErrNone if successful, otherwise one of the system-wide error codes
*/
	{
	if(iUplinkGranularities.Count())
		{
		__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: setting new uplink granularity (%d)"), 
							 iSubConnectionProviderShim.Id(), iCurrentUplinkGranularity));

		// Read the next granularity from the array
		TUint granularity = iUplinkGranularities[0];
		
		// Remove the value read
		iUplinkGranularities.Remove(0);

		return SetUplinkGranularity(granularity);
		}
	else
		{
		__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: cancelling data sent notifications - no values remaining in granularity array"), 
							 iSubConnectionProviderShim.Id()));

		return SetUplinkGranularity(0);		// cancel notifications
		}
	}

TInt CConnDataTransferShim::SetDownlinkGranularity(TUint aRequestedGranularity)
/**
Set data received notification granularity in lower provider.

@param aRequestedGranularity granularity to set.  If zero, cancel data received notifications.
@return KErrNone or a system wide error code.
*/
	{
	iCurrentDownlinkGranularity = aRequestedGranularity;
	if (iCurrentDownlinkGranularity)
		{
		return iSubConnectionProviderShim.Provider().SetDataReceivedNotificationGranularity(iSubConnectionProviderShim.Id(), iCurrentDownlinkGranularity);
		}
	else
		{
		return iSubConnectionProviderShim.Provider().DataReceivedNotificationCancel(iSubConnectionProviderShim.Id());
		}
	}

TInt CConnDataTransferShim::SetNextDownlinkGranularity()
/**
Get the next downlink granularity from the array, and send it to the connection provider

@return KErrNone if successful, otherwise one of the system-wide error codes
*/
	{
	if(iDownlinkGranularities.Count())
		{
		__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("CConnDataTransferShim[id: %d]: setting new downlink granularity (%d)"), 
							 iSubConnectionProviderShim.Id(), iCurrentDownlinkGranularity));
		// Read the next granularity from the array
		TUint granularity = iDownlinkGranularities[0];
		
		// Remove the value read
		iDownlinkGranularities.Remove(0);

		return SetDownlinkGranularity(granularity);
		}
	else
		{
		__CFLOG_VAR((KShimScprTag, KShimScprDataTag, _L8("ESock: CConnDataTransferShim[id: %d]: cancelling data received notifications - no values remaining in granularity array"), 
							 iSubConnectionProviderShim.Id()));

		return SetDownlinkGranularity(0);	// cancel notifications
		}
	}
