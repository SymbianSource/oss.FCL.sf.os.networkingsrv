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
// Support for TCP/IP stack calling NIFMAN via MNifIfNotify pointer in CNIfIfBase.
// 
//

/**
 @file notify.cpp
*/

#include "notify.h"
#include "panic.h"
#include "idletimer.h"
#include <comms-infras/ss_datamonitoringprovider.h>

using namespace ESock;

//
// CIPShimNotify methods //
//

CIPShimNotify::CIPShimNotify(CIPShimIfBase* aIntf) : iIntf(aIntf)
	{
	}

CIPShimNotify* CIPShimNotify::NewL(CIPShimIfBase* aIntf)
	{
	return new (ELeave) CIPShimNotify(aIntf);
	}

void CIPShimNotify::OpenRoute()
	{ 
   	iIntf->ProtcolIntf()->OpenRoute();
	}
	
void CIPShimNotify::CloseRoute()
	{	       	
   	iIntf->ProtcolIntf()->CloseRoute();
	}
	
TInt CIPShimNotify::PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer)
	{
	ASSERT(iPacketActivity);
	ASSERT(iConnectionVolumesPtr);
	ASSERT(iSubConnectionVolumesPtr);
			
	*iPacketActivity = aResetTimer;

	switch(aDirection)
		{
	case EIncoming:
		/******************************
		 * NOTE: shared memory write
		 ******************************/
		iConnectionVolumesPtr->iReceivedBytes += aBytes;
		
		if(iConnectionVolumesPtr->iReceivedBytes < aBytes)
			{
			iIntf->Flow().PostConnDataReceivedThresholdReached(KMaxTUint32);
			}
		else 
			{
			if(iConnectionThresholdsPtr->iReceivedThreshold && iConnectionVolumesPtr->iReceivedBytes >= iConnectionThresholdsPtr->iReceivedThreshold)
				{
				iConnectionThresholdsPtr->iReceivedThreshold = 0;
				iIntf->Flow().PostConnDataReceivedThresholdReached(iConnectionVolumesPtr->iReceivedBytes);
				}
			}
			
		/******************************
		 * NOTE: shared memory write
		 ******************************/
		iSubConnectionVolumesPtr->iReceivedBytes += aBytes;
		
		if(iSubConnectionVolumesPtr->iReceivedBytes < aBytes)
			{
			iIntf->Flow().PostSubConnDataReceivedThresholdReached(KMaxTUint32);
			}
		else
			{
			if(iSubConnectionThresholdsPtr->iReceivedThreshold && iSubConnectionVolumesPtr->iReceivedBytes >= iSubConnectionThresholdsPtr->iReceivedThreshold)
				{
				iSubConnectionThresholdsPtr->iReceivedThreshold = 0;
				iIntf->Flow().PostSubConnDataReceivedThresholdReached(iSubConnectionVolumesPtr->iReceivedBytes);
				}				
			}			
		break;
		
	case EOutgoing:
		/******************************
		 * NOTE: shared memory write
		 ******************************/
		iConnectionVolumesPtr->iSentBytes += aBytes;
		
		if(iConnectionVolumesPtr->iSentBytes < aBytes)
			{
			iIntf->Flow().PostConnDataSentThresholdReached(KMaxTUint32);
			}
		else 
			{
			if(iConnectionThresholdsPtr->iSentThreshold && iConnectionVolumesPtr->iSentBytes >= iConnectionThresholdsPtr->iSentThreshold)
				{
				iConnectionThresholdsPtr->iSentThreshold = 0;
				iIntf->Flow().PostConnDataSentThresholdReached(iConnectionVolumesPtr->iSentBytes);
				}
			}
			
		/******************************
		 * NOTE: shared memory write
		 ******************************/
		iSubConnectionVolumesPtr->iSentBytes += aBytes;
		
		if(iSubConnectionVolumesPtr->iSentBytes < aBytes)
			{
			iIntf->Flow().PostSubConnDataSentThresholdReached(KMaxTUint32);
			}
		else
			{
			if(iSubConnectionThresholdsPtr->iSentThreshold && iSubConnectionVolumesPtr->iSentBytes >= iSubConnectionThresholdsPtr->iSentThreshold)
				{
				iSubConnectionThresholdsPtr->iSentThreshold = 0;
				iIntf->Flow().PostSubConnDataSentThresholdReached(iSubConnectionVolumesPtr->iSentBytes);
				}				
			}			
		break;
		
	default:
		break;
		
		} // switch(aDirection)

	return KErrNone;
	}

void CIPShimNotify::SetPacketActivityFlag(volatile TBool* aPacketActivity)
	{
	ASSERT(aPacketActivity);
	iPacketActivity = aPacketActivity;
	}

void CIPShimNotify::SetDataVolumePtrs(TDataVolumes* aConnectionVolumesPtr, TDataVolumes* aSubConnectionVolumesPtr)
	{
	ASSERT(aConnectionVolumesPtr);
	ASSERT(aSubConnectionVolumesPtr);
	
	iConnectionVolumesPtr = aConnectionVolumesPtr;
	iSubConnectionVolumesPtr = aSubConnectionVolumesPtr;
	}
	
void CIPShimNotify::SetNotificationThresholdPtrs(TNotificationThresholds* aConnectionThresholdsPtr, TNotificationThresholds* aSubConnectionThresholdsPtr)
	{
	ASSERT(aConnectionThresholdsPtr);
	ASSERT(aSubConnectionThresholdsPtr);
	
	iConnectionThresholdsPtr = aConnectionThresholdsPtr;
	iSubConnectionThresholdsPtr = aSubConnectionThresholdsPtr;
	}


//
// Methods stubbed out to panic
//

void CIPShimNotify::LinkLayerDown(TInt /*aReason*/, TAction /*aAction*/)
	{
	Panic(EBadNotifyCall);
	}
	
void CIPShimNotify::LinkLayerUp()
	{
	Panic(EBadNotifyCall);
	}
	
void CIPShimNotify::NegotiationFailed(CNifIfBase* /*aIf*/, TInt /*aReason*/)
	{
	Panic(EBadNotifyCall);
	}
	
TInt CIPShimNotify::Authenticate(TDes& /*aUsername*/, TDes& /*aPassword*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
void CIPShimNotify::CancelAuthenticate()
	{
	Panic(EBadNotifyCall);
	}
	
TInt CIPShimNotify::GetExcessData(TDes8& /*aBuffer*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
void CIPShimNotify::IfProgress(TInt /*aStage*/, TInt /*aError*/)
	{
	Panic(EBadNotifyCall);
	}
	
void CIPShimNotify::IfProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
	{
	Panic(EBadNotifyCall);
	}
	
TInt CIPShimNotify::Notification(TNifToAgentEventType /*aEvent*/, void * /*aInfo*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
void CIPShimNotify::BinderLayerDown(CNifIfBase* /*aBinderIf*/, TInt /*aReason*/, TAction /*aAction*/)
	{
	Panic(EBadNotifyCall);
	}
		
void CIPShimNotify::NotifyDataSent(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aUplinkVolume*/)
	{
	Panic(EBadNotifyCall);
	}
	
void CIPShimNotify::NotifyDataReceived(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aDownlinkVolume*/)
	{
	Panic(EBadNotifyCall);
	}
	
void CIPShimNotify::NifEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
	{
	Panic(EBadNotifyCall);
	}
	
TInt CIPShimNotify::DoReadInt(const TDesC& /*aField*/, TUint32& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoWriteInt(const TDesC& /*aField*/, TUint32 /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoReadDes(const TDesC& /*aField*/, TDes8& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoReadDes(const TDesC& /*aField*/, TDes16& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoWriteDes(const TDesC& /*aField*/, const TDesC8& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoWriteDes(const TDesC& /*aField*/, const TDesC16& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoReadBool(const TDesC& /*aField*/, TBool& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
TInt CIPShimNotify::DoWriteBool(const TDesC& /*aField*/, TBool /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	Panic(EBadNotifyCall);
	return KErrNotSupported;
	}
	
