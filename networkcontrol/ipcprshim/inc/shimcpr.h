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
* This temporary shim layer contanis the interfaces defined by MConnectionNotify, CConnectionProvdBase and CInterface.
* However only the connection-related part of CInterface appears here, the rest is in CNifManSubConnectionShim.
* 
*
*/



/**
 @file SHIMCPR.H
 @internalComponent
*/

#if !defined(__SS_SHIMCPR_H__)
#define __SS_SHIMCPR_H__

#include <ss_connprov.h>		// for CConnectionProviderFactoryBase
#include <cflog.h>
#include <es_prot.h>			// for MConnectionNotify and CConnectionProvdBase
#include <comms-infras/ss_log.h>				// for KESockConnectionTag

#define KShimCprTag KESockConnectionTag
_LIT8(KShimCprSubTag, "shimcpr");	// logging tag


class CConnectionProvdBase;
class CNifManSubConnectionShim;
class CConnectionProviderFactoryShim;
class CConnectionSettingsShim;
class CConnection;
class MProviderSelector;
class CSubConnectionLinkShimClient;
class CConnectionSelectorShim;
NONSHARABLE_CLASS(CConnectionProviderShim) : public CConnectionProviderBase, MConnectionNotify
	{
  	friend class CConnectionProviderFactoryShim;
  	friend class CConnectionSelectorShim; //to get/set the aggregarted CConnectionProvdBase


public:
	CConnectionProvdBase& Provider()
		{
		ASSERT(iProvider);
		return *iProvider;
		}
	CSubConnectionLinkShimClient* QuerySubSessions(TInt32 aTypeId, const CConnection* aConnection);

#ifdef SYMBIAN_NETWORKING_UMTSR5
	// The public function which will set the pointer to the factiory
	void SetFactoryIfactory(CConnectionProviderFactoryShim *aFactoryShim);
#endif // SYMBIAN_NETWORKING_UMTSR5	
      
protected:
	virtual ~CConnectionProviderShim();
	

#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	CConnectionProviderShim(CConnectionProviderFactoryBase& aFactory)
	   : CConnectionProviderBase(aFactory), iSubConnectionType(KInvalidFactoryId)
	    {
	    __CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tCConnectionProviderShim()"),
			this));
	    }    
#else
	CConnectionProviderShim(CConnectionProviderFactoryBase& aFactory)
	   : CConnectionProviderBase(aFactory)
	    {
	    __CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tCConnectionProviderShim()"),
			this));
	    }	
#endif
// SYMBIAN_NETWORKING_3GPPDEFAULTQOS

	    
    void ConstructL();
    static CConnectionProviderShim* NewL(CConnectionProviderFactoryBase& aFactory);

	void ConnectionJoiningL( const CConnection& aConnection );
	void ConnectionLeaving( const CConnection& aConnection );
	
	
	      
protected:
	// Define methods from the interface specified by MConnectionNotify
	virtual void SelectionComplete(TInt aError, const TDesC8& aSelectionInfo);
	virtual void ConnectionError(TInt aError);
	virtual void LinkLayerOpen(TInt aError);
	virtual void LinkLayerClosed(TInt aError);
	virtual void ProgressNotification(TInt aStage, TInt aError);
	virtual void ProgressNotification(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType);
	virtual void InterfaceStateChangeNotification(TDesC8& aInfo);
	virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume);
	virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume);
	virtual void SubConnectionEvent(const TSubConnectionEvent& aSubConnectionEvent);
	
	virtual TBool ShouldIDeleteNow();
	
protected:
	//CConnectionProviderBase virtuals
	virtual void DoDataClientJoiningL(MConnectionDataClient& aDataClient);
	virtual void DoDataClientLeaving(MConnectionDataClient& aDataClient);
	virtual void DoControlClientJoiningL(MConnectionControlClient& aControlClient);
	virtual void DoControlClientLeaving(MConnectionControlClient& aControlClient);
   
	virtual void DoStartL(Meta::SMetaData& aParams, const RMessagePtr2* aMessage);	
	virtual TInt DoStop(TInt aError, const RMessagePtr2* aMessage);

	virtual TInt DoCanDoSubConnection(RSubConnection::TSubConnType aSubConnType) const;

	virtual CConnectionProviderBase* DoNextLayer() const ;
	virtual void DoJoinNextLayerL(CConnectionProviderBase* aNextLayer);

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

    CConnectionProvdBase* GetNifSession();
    void InitialiseL(CConnectionProvdBase* aStarterSession);
    void SetBlockProgressesL(CConnectionProvdBase* aStarterSession);
    void AddRefL(CConnectionSelectorShim* aSelector);
    void ReleaseRef(CConnectionSelectorShim* aSelector);    
    
private:
	CNifManSubConnectionShim* FindSubConnection(TSubConnectionUniqueId aId);
	void ReleaseRef();

private:
	CConnectionProvdBase *iProvider; // Enable access to 'old' NIFMAN layer
	CConnectionSettingsShim* iConnectionSettings;
	TUint iUsageProfile;
	RPointerArray<const CConnection> iConnections;
	// TInt iRefCount; //to keep a connection up while selection is going on
	RPointerArray<CNifManSubConnectionShim> iNifManSubConnections;
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
    TInt iSubConnectionType;
#endif // SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	RPointerArray<CConnectionSelectorShim> iSelectors;
	TBool iHasAnyControlClientJoined:1;
	TBool iIsAlreadyUninitialised:1;
#ifdef SYMBIAN_NETWORKING_UMTSR5
	// The handle to the factory object that creates the CConnectionProviderShim
	CConnectionProviderFactoryShim *iFactoryShim;
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5


};

#endif
// __SS_SHIMCPR_H__
