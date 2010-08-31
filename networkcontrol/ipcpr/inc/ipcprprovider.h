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
* This class is an example implementation of a bearer-mobile network session layer. It is intended as a guide only and does
* not employ any advanced bearer selection algorithms.
* This is part of an ECOM plug-in
* 
*
*/



/**
 @file IPCPRPROVIDER.H
 @internalComponent
*/

#if !defined(__SS_IPCPRPROVIDER_H__)
#define __SS_IPCPRPROVIDER_H__

#include "ipcprfactory.h"	// for CIPConnectionSelector
#include <es_sock.h>
#include <es_prot.h>
#include <ss_connprov.h>	// for CConnectionProviderBase and MConnectionControlClient
#include <comms-infras/ss_log.h>			// for KESockConnectionTag

#ifdef SYMBIAN_NETWORKING_UMTSR5	

#include "MAppIdInfo.h"

#endif

#define KIpcprTag KESockConnectionTag
_LIT8(KIpcprSubTag, "ipcpr");		// logging tag


	

class CConnectionSettings;
/**
 @internalComponent
 @released Since 9.1
 */
#ifdef SYMBIAN_NETWORKING_UMTSR5
NONSHARABLE_CLASS(CIPNetworkConnectionProvider) : public CConnectionProviderBase, public MConnectionControlClient, 
												  public MConnectionAppIdInfo
	{
#else // SYMBIAN_NETWORKING_UMTSR5	

NONSHARABLE_CLASS(CIPNetworkConnectionProvider) : public CConnectionProviderBase, public MConnectionControlClient
	{

#endif // SYMBIAN_NETWORKING_UMTSR5	

public:
	static CIPNetworkConnectionProvider* NewL(CConnectionProviderFactoryBase& aFactory);
	
#ifdef SYMBIAN_NETWORKING_UMTSR5	
	// Interface from MConnectionAppIdInfo
	virtual TUint32 GetAppSecureId(); 

	void SetAppSecurId(TUint32 aSecureId);
#endif // SYMBIAN_NETWORKING_UMTSR5	
   
protected:
	CIPNetworkConnectionProvider(CConnectionProviderFactoryBase& aFactory) :
		CConnectionProviderBase(aFactory)
			{
      	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider [this=%08x]:\tCIPNetworkConnectionProvider [MConnectionControlClient=%08x]"),
      	   this, (MConnectionControlClient*)this));
			}
	~CIPNetworkConnectionProvider();
	
	/////////////////////////////////////////////////////////////////////////////
	// from MConnectionControlClient
	virtual void ConnectionGoingDown(CConnectionProviderBase& aConnProvider);
	virtual void ProgressNotification(TInt aStage, TInt aError);
	virtual void ConnectionError(TInt aStage,  TInt aError);
	virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType);
	virtual void SubConnectionEvent(CSubConnectionProviderBase* aSubConnNextLayerProvider, const TSubConnectionEvent& aSubConnectionEvent);
	virtual void LayerUp(TInt aError);
	virtual TCtlType CtlType() const;

	////////////////////////////////////////////////////////////////////////////
	// from CConnectionProviderBase
#ifdef SYMBIAN_NETWORKING_UMTSR5		
	virtual TAny* DoFetchInterfaceInstanceL( const STypeId& aTid );
#endif	
	
	virtual void DoDataClientJoiningL(MConnectionDataClient& aDataClient);
	virtual void DoDataClientLeaving(MConnectionDataClient& aDataClient);
	virtual void DoControlClientJoiningL(MConnectionControlClient& aControlClient);
	virtual void DoControlClientLeaving(MConnectionControlClient& aControlClient);

	virtual void DoStartL(Meta::SMetaData& aParams, const RMessagePtr2* aMessage);	
	virtual TInt DoStop(TInt aError, const RMessagePtr2* aMessage);   

	virtual void DoProgressL(Meta::SMetaData& aBuffer) const;
	virtual void DoLastProgressError(Meta::SMetaData& aBuffer);
	virtual void DoRequestServiceChangeNotificationL();
	virtual void DoCancelServiceChangeNotification();
	virtual void DoControlL(TUint aOptionLevel, TUint aOptionName, Meta::SMetaData& aOption, const RMessagePtr2* aMessage);
	virtual TInt DoAllSubConnectionNotificationEnable();
	virtual TInt DoCancelAllSubConnectionNotification();
	virtual void DoSendIoctlMessageL(const RMessage2& aMessage);
	virtual void DoSendCancelIoctl();   
	virtual TInt DoEnumerateSubConnectionsL(TUint& aCount);
	virtual TUint DoEnumerateClientsL(HBufC8*& aClientInfoBuffer, TEnumClients aClientType);
	virtual void DoConnectionControlActivityL( TControlActivity aControlActivity, const Meta::SMetaData* aData, const RMessagePtr2* aMessage );

	virtual CConnectionSettings& DoSettingsAccessL();

	virtual TInt DoCanDoSubConnection(RSubConnection::TSubConnType aSubConnType) const;

	virtual CConnectionProviderBase* DoNextLayer() const;
	virtual void DoJoinNextLayerL(CConnectionProviderBase* aNextLayer);

protected:
	void UpdateUsageProfile(MConnectionControlClient* aControlClient);

private:
	CConnectionProviderBase* iShimCpr;
	TMetaDes8* iMDes;
#ifdef SYMBIAN_NETWORKING_UMTSR5	
	// Added as per the requirements of PREQ 635 to block sockets
	TUint32		iAppSecureId;
#endif // SYMBIAN_NETWORKING_UMTSR5	
	};

#endif // __SS_IPCPRPROVIDER_H__
