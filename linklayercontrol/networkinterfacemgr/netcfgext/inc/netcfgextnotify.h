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
// netcfgextnotify.cpp
// 
//

/**
 @file
 @brief MNifIfNotify implementation for network config extensions
 @internalTechnology
*/

#ifndef SYMBIAN_NETCFGEXTNOTIFY_H
#define SYMBIAN_NETCFGEXTNOTIFY_H

#include <comms-infras/nifif.h>
#include <comms-infras/ss_subconnprov.h>

#include <comms-infras/nifprvar_internal.h>

class CNetCfgExtNotify : public CBase, public MNifIfNotify
	{
public:
	IMPORT_C static CNetCfgExtNotify* NewL(ESock::CSubConnectionProviderBase* aScpr);
	
	/**
	   MNifIfNotify interface
	   Only IfProgress, DoReadDes and DoReadInt supported.
	*/
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
	void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

	TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage);
	TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage);
	TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage);
	TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage);
	TInt DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage);
private:
	CNetCfgExtNotify(ESock::CSubConnectionProviderBase* aScpr);
	
	ESock::CSubConnectionProviderBase* iScpr;
	};

#endif // SYMBIAN_NETCFGEXTNOTIFY_H

