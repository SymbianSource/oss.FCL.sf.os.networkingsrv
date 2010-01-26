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
* Support for TCP/IP calls directly to MNifIfNotify methods
* 
*
*/



/**
 @file notify.h
*/


#if !defined(IPSHIMFLOW_NOTIFY_H_INCLUDED_)
#define IPSHIMFLOW_NOTIFY_H_INCLUDED_

#include <e32base.h>
#include <comms-infras/nifif.h>
#include "nif.h"

namespace ESock
{
struct TDataVolumes;
struct TNotificationThresholds;
}

class CIPShimIfBase;
class TPacketActivity;

class CIPShimNotify : public CBase, public MNifIfNotify
/**
Class used to support TCP/IP stack MNifIfNotify calls to NIFMAN via CNifIfBase.

Only the OpenRoute()/CloseRoute()/PacketActivity() methods are actually called by the
TCP/IP stack and are supported.  All other MNifIfNotify methods just panic.

@internalComponent
*/
	{
public:
	static CIPShimNotify* NewL(CIPShimIfBase* aIntf);

	// from MNifIfNotify
	// (the only methods supported are: OpenRoute(), CloseRoute(), PacketActivity())
	void LinkLayerDown(TInt aReason, TAction aAction);
	void LinkLayerUp();
	void NegotiationFailed(CNifIfBase* aIf, TInt aReason);
	TInt Authenticate(TDes& aUsername, TDes& aPassword);
	void CancelAuthenticate();
	TInt GetExcessData(TDes8& aBuffer);
	void IfProgress(TInt aStage, TInt aError);
	void IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	void OpenRoute();
	void CloseRoute();
	TInt Notification(TNifToAgentEventType aEvent, void * aInfo = NULL);
	void BinderLayerDown(CNifIfBase* aBinderIf, TInt aReason, TAction aAction);
	TInt PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer);
	void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume);
	void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume);
	void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource);
	TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage);
	TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage);
	TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage);
	TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage);
	
	// Idle timer
	void SetPacketActivityFlag(volatile TBool* aPacketActivity);
	
	// Data monitoring
	void SetDataVolumePtrs(ESock::TDataVolumes* aConnectionVolumesPtr, ESock::TDataVolumes* aSubConnectionVolumesPtr);
	void SetNotificationThresholdPtrs(ESock::TNotificationThresholds* aConnectionThresholdsPtr, ESock::TNotificationThresholds* aSubonnectionThresholdsPtr); 
	
private:	
	CIPShimNotify(CIPShimIfBase* aIntf);

private:
	CIPShimIfBase* iIntf;
	ESock::TDataVolumes* iConnectionVolumesPtr;
	ESock::TDataVolumes* iSubConnectionVolumesPtr;
	ESock::TNotificationThresholds* iConnectionThresholdsPtr;
	ESock::TNotificationThresholds* iSubConnectionThresholdsPtr;
	volatile TBool* iPacketActivity;
	TInt iRouteCount;	
	};
	
#endif
