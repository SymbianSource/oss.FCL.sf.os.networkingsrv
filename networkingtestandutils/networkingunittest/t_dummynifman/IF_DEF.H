// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Interface Manager
// 
//

/**
 @file IF_DEF.H
 @internalComponent
*/


#if !defined(__IF_DEF_H__)
#define __IF_DEF_H__

#include <e32std.h> // for TTime
#include <comms-infras/nifif.h>
#include <comms-infras/nifagt.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_ini.h>
#include <f32file.h>
#include <connpref.h>
#include <ss_std.h>
#include <comms-infras/dbaccess.h>
#include "CNetworkControllerBase.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif

const TInt KMaxLengthOfTableNameAndColumnName = KCommsDbSvrMaxColumnNameLength * 2 + 1;
const TInt KMessageArrayGranularity = 2;

/**
Do not change this without altering the EMI compatibility layer
to match the new value.
The "whole connection" subconnection and the single subconnection
*/
const TUint KNumberOfEmulatedSubConnections = 2;

class MNifSessionNotify
/**
Interface between the nif/agent controller class (CNifAgentRef) and the
reporting channel (CNifSession)
@internalComponent
@note Removed ProgressNotification() method from MAgentSessionNotify interface and
added it here - agents should not directly call ProgressNotification() on MAgentSessionNotify,
they should call AgentProgress() on MNifAgentNotify
*/
	{
public:
	/**
	Notification from an interface that a new progress stage has been reached

	@param aStage The progress stage that has been reached
	@param aError Any errors that have occured
	*/
	virtual void ProgressNotification(TInt aStage, TInt aError)=0;

	/**
	Notification from an subconnection that a new progress stage has been reached

	@param aSubConnectionUniqueId The subconnection to which this notification refers
	@param aStage The progress stage that has been reached
	@param aError Any errors that have occured
	*/
	virtual void ProgressNotification(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)=0;

	/**
	Notification that the sent bytes threshold for a subconnection has been met or exceeded in the nif

	@param aSubConnectionUniqueId The subconnection to which this notification refers
	@param aUplinkVolume The total number of bytes sent by this subconnection
	*/
	virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume)=0;

	/**
	Notification that the received bytes threshold for a subconnection has been met or exceeded in the nif

	@param aSubConnectionUniqueId The subconnection to which this notification refers
	@param aDownlinkVolume The total number of bytes sent by this subconnection
	*/
	virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume)=0;

	/**
	Notification of an event relating to a subconnection from an agent, nif or internally within Nifman

	*/
	virtual void SubConnectionEvent(const TSubConnectionEvent& aSubConnectionEvent)=0;
	};

class CNifAgentRef;
class CBinderRef : public CActive
/**
@internalComponent
*/
	{
friend class CNifAgentRef;
public:
	virtual void DoCancel();
	virtual void RunL();
	~CBinderRef();
	CBinderRef(MNifIfUser* aNetwork, CNifAgentRef& aAgentRef, TAny* aId);  //lint -sem(CBinderRef::CBinderRef, custodial(t))
	void BindL(CNifIfLink* aInterface);
	void LinkDown(TInt aError, TBool aFatal=ETrue);
	void UnBind(TInt aError, TBool aFatal=ETrue);			 //lint -sem(CBinderRef::UnBind, custodial(t))

public:
	TDblQueLink iLink;
	MNifIfUser* iNetworkLayer;
	CNifIfBase* iLinkLayer;
	CNifAgentRef* iAgentRef;
	TAny* iId;
	};

class CNifMan;
class MNifSessionNotify;
class CNifExtendedManagementCompatibilityLayer;
class CNifConfigurationControl;

class CNifAgentRef : public CObject, public MNifAgentNotify, public MNifIfNotify, public MTimer
/**
@internalComponent
*/
	{
friend class CNifMan;
friend class CBinderRef;

public:
	enum TTimerType { ETimerUnknown = 0, ETimerShort, ETimerMedium, ETimerLong, ETimerImmediate, ETimerMax };

	static CNifAgentRef* NewL(const TDesC& aName, CNifAgentFactory& aFactory);
	virtual ~CNifAgentRef();

	/** Interface virtual */
    virtual void LinkLayerDown(TInt aReason, TAction aAction);
	virtual void LinkLayerUp();
    virtual void NegotiationFailed(CNifIfBase* aIf, TInt aReason);
    virtual TInt Authenticate(TDes& aUsername, TDes& aPassword);
    virtual void CancelAuthenticate();
	virtual TInt GetExcessData(TDes8& aBuffer);
    virtual void IfProgress(TInt aStage, TInt aError);
    virtual void IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual void OpenRoute();
	virtual void CloseRoute();
	virtual TInt Notification(TNifToAgentEventType aEvent, void* aInfo);
	virtual void BinderLayerDown(CNifIfBase* aBinderIf, TInt aReason, TAction aAction);
	virtual TInt PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer);
	virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume);
	virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume);
	virtual void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

	/** Agent virtual */
	virtual void ConnectComplete(TInt aStatus);
	virtual void ReconnectComplete(TInt aStatus);
	virtual void AuthenticateComplete(TInt aStatus);
	virtual void ServiceStarted();
	virtual void ServiceClosed();
	virtual void DisconnectComplete();
	virtual void AgentProgress(TInt aStage, TInt aError);
	virtual void AgentProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo);
	virtual TInt IncomingConnectionReceived();
    virtual TName Name() const;
	virtual void AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

	/** CObject virtual */
	virtual TInt Open();
	virtual void Close();

	/** General function */
	TInt Start(TAgentConnectType aType = EAgentStartDialOut);
	TInt Start(CStoreableOverrideSettings* aOverrides, TAgentConnectType aType = EAgentStartDialOut);
	TInt Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, const RMessagePtr2* aMessage);
	TInt Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption);
	void Stop(TInt aError);
	void Stop(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aError);
	TInt AddNetworkLayer(MNifIfUser* aNetwork, TAny* aId=0);
	void LinkDownAll(TInt aError, TBool aFatal=ETrue);
	void BindFailAll(TInt aError);
	TBool IsAnyNetworkLayerOpen();
	TBool AreAllNetworkLayersOpen();
	void NetworkLayerClosed(MNifIfUser* aNetwork);
	TInt IsNetworkLayerActive();
	void NetworkAdaptorEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

	/** General function for subconnections */
	TInt GetInterfaceType(TConnectionType& aConnectionType);
	TInt EnumerateSubConnections(TUint& aCount);
	TInt GetSubConnectionInfo(TUint aIndex, TDes8& aSubConnectionInfo);
	TInt GetSubConnectionInfo(TDes8& aSubConnectionInfo);
	TInt AllSubConnectionNotificationEnable();
	TInt CancelAllSubConnectionNotification();
	TInt DataTransferred(TSubConnectionUniqueId aSubConnectionUniqueId, TUint& aUplinkVolume, TUint& aDownlinkVolume);
	TInt SetDataSentNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkGranularity);
	TInt DataSentNotificationCancel(TSubConnectionUniqueId aSubConnectionUniqueId);
	TInt SetDataReceivedNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkGranularity);
	TInt DataReceivedNotificationCancel(TSubConnectionUniqueId aSubConnectionUniqueId);


	inline CNifAgentBase& Agent() const;
	inline const TNifProgress& Progress() const;

	void DisableTimers();
	void EnableTimers();

	TBool MatchesIdOrInterface(TAny* aId, CNifIfBase* aIf);

	void AddObserverL(MNifSessionNotify* aObserver);
	void RemoveObserver(MNifSessionNotify* aObserver);

	void SetUsageProfile(TUint aProfile);
	inline void ConnectionControlActivity();

	void RefreshConnectionInfo();
	inline void SetConnectionInfo(const TSoIfConnectionInfo& aInfo);
	inline const TSoIfConnectionInfo& ConnectionInfo() const;
	inline TBool MatchesConnectionInfo(const TSoIfConnectionInfo& aInfo) const;
	TInt GetInterfaceName(TUint aIndex, TDes& aInterfaceName);

	TInt GetTimerThresholds();
	void ResetTimer();
	void StartNextTick();

	/** Timer virtual */
	virtual void TimerComplete(TInt aStatus);

   	void SendIoctlMessageL(const RMessage2& aMessage);
   	void SendCancelIoctl();
   	
//PREQ399_REMOVE
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
    CNifIfBase* GetNif(TInt aIndex);
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS //PREQ399_REMOVE

protected:
	CNifAgentRef(CObject& aFactory);
	explicit CNifAgentRef(TInt aDummy);

	void ConstructL(const TDesC& aName, CNifAgentFactory& aFactory);

	void Progress(TInt aStage, TInt aError);
	void Progress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);


	virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage );
    virtual TInt DoWriteInt( const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage );
    virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage );
    virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage );
    virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage );
    virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue, const RMessagePtr2* aMessage );
	virtual TInt DoReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage );
    virtual TInt DoWriteBool( const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage );




	TBool	iIsSubClassed;

private:
	inline void SetTimerMode(TTimerType aTimerMode);
	TTimerType DecideTimerMode();
	CBinderRef* FindBinder(CNifIfBase* aIf);
	CBinderRef* FindBinder(MNifIfUser* aNetworkLayer);

private:

	enum TState
	    {
		EIdle, EConnecting, EStarted,
		EStopping, EDisconnecting, EReconnecting,
		ELinkUp, EDisconnected
	    };

	TTimerType	iTimerMode;

	/** time marker for timer drift calculations */
	TTime		iDriftCheckTime;

	/** total timer drift observed in microsecs */
	TInt		iTotalTimerDrift;

	TInt		iTickThreshold[ETimerMax];

	TInt  iUserCount;
	TBool iPeriodActivity;
	TBool iConnectionControlActivity;

	TBool iAuthenticating;
    CNifAgentBase* iAgent;           //lint -esym(613, CNifAgentRef::iAgent) // often NULL
	TState iState;
	TInt iOrigReason;
	CNifIfLink* iInterface;          //lint -esym(613, CNifAgentRef::iInterface) // often NULL
	TDblQue<CBinderRef> iBinders;
	TNifProgress iProgress;
	TInt iRouteCount;
	TInt iExpiredTicks;
	TInt iTimerDisableCount;
	TBool iReconnectAfterDisconnect;
	TAgentConnectType iConnectType;

	CNetworkControllerBase* iNetCon;

	RPointerArray<MNifSessionNotify> iSessions;
	TSoIfConnectionInfo iConnInfo;

	TBool iSendAllSubConnectionEvents;	 //< all subconnection notification active?

	CNifExtendedManagementCompatibilityLayer* iNifExtndMngmntCompatibilityLayer;
	MNifIfExtendedManagementInterface* iNifExtndMngmtInterface;			//< we do not own this object - do not delete it
	MNifAgentExtendedManagementInterface* iAgentExtndMngmtInterface;	//< we do not own this object - do not delete it
	CNifConfigurationControl* ipNifConfigurationControl;
	};

class CNifExtendedManagementCompatibilityLayer : public CBase, public MNifIfExtendedManagementInterface, public MNifAgentExtendedManagementInterface
/**
Compatibility layer to provide basic extended management functionality for nifs/agents
that do not support extended management, ie. do not implement the MNif*ExtendedManagementInterface(s)
@internalComponent
*/
	{
public:
	CNifExtendedManagementCompatibilityLayer(MNifIfNotify* aNotifyInterface);
	virtual ~CNifExtendedManagementCompatibilityLayer();

	/** Call from Nifman */
	TInt SetTimeStarted(TTime aTimeStarted);

	/** Notification from network layer (eg. IP) via Nifman */
	TInt DataSent(TUint aUplinkBytes);
	TInt DataReceived(TUint aDownlinkBytes);

	/** MNifIfExtendedManagementInterface */
	TInt Stop(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aReason);
	TInt GetDataTransferred(TSubConnectionUniqueId aSubConnectionUniqueId, TUint& aSentBytes, TUint& aReceivedBytes);
	TInt SetDataSentNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aGranularity);
	TInt CancelDataSentNotification(TSubConnectionUniqueId aSubConnectionUniqueId);
	TInt SetDataReceivedNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aGranularity);
	TInt CancelDataReceivedNotification(TSubConnectionUniqueId aSubConnectionUniqueId);

	/** MNifAgentExtendedManagementInterface */
	TInt GetInterfaceType(TConnectionType& aConnectionType);
	TInt EnumerateSubConnections(TUint& aCount);
	TInt GetSubConnectionInfo(TUint aIndex, TDes8& aSubConnectionInfo);
	TInt GetSubConnectionInfo(TDes8& aSubConnectionInfo);

private:
	TConnectionType iInterfaceType;

	TTime iTimeStarted;

	/** the current volume of data transferred */
	TUint iUplinkDataVolume;

	TUint iDownlinkDataVolume;

	class TSubConnectionNotificationData
	/**
	Store the whole connection notification volumes and the subconnection notification volumes seperately,
	in case clients start making independant requests on the two
	@internalComponent
	@note The results for the whole connection and the subconnection will be identical,
	so this is only allow clients to use the same code with real subconnections and this compatibility layer
	*/
		{
		public:
			TSubConnectionUniqueId iSubConnectionUniqueId;

			/** the last granularity of the notifications, for situations where we need to send
			repeated notifications without the client setting a new granularity */
			TUint iUplinkDataNotificationGranularity;

			TUint iDownlinkDataNotificationGranularity;

			/** the amount of data to send before we send a notification; calculated using
			the granularity variables above */
			TUint iUplinkDataNextNotificationVolume;

			TUint iDownlinkDataNextNotificationVolume;
		};

	/** the notification granularities for the subconnection and the connection */
	TSubConnectionNotificationData iConnectionNotificationsVolumes[KNumberOfEmulatedSubConnections];

	/** need this as we have to emulate some calls to MNifIfNotify from the nif */
	MNifIfNotify* iNotifyInterface;
	};

class CNifMonitor : public CBase
/**
Class used to monitor all interface state changes and propagate
these to registered CNifSession observers
@internalComponent
*/
	{
public:
	static CNifMonitor* NewL();
	virtual ~CNifMonitor();

	void AddSessionL(CNifSession* aSession);
	void RemoveSession(CNifSession* aSession);

	void StatusChanged(CNifAgentRef& aAgent, TConnInterfaceState aState);

protected:
	CNifMonitor();

private:
	/** pointer to CNifSession objects that have registered for interface state change notifications */
	RPointerArray<CNifSession> iSessions;
	};

class CMBufPoolReaper;
class Nif;
class CNifServer;
class CNifMan : public CSocketServExtBase
/**
@internalComponent
*/
	{
friend class Nif;
public:
	CNifMan();
	virtual ~CNifMan();
	virtual void InstallL(const TDesC& aArgs);
	virtual void Remove();

	CNifAgentRef* FindByUniqueNameL(const TDesC& aName) const;
	CNifAgentRef* FindByNameL(const TDesC& aName);
	CNifAgentRef* FindOrCreateL(const TDesC& aName, TBool aCreateNewInstance = EFalse);
	CNifAgentRef* FindByIdOrInterface(TAny* aId, CNifIfBase* aIf);
	CNifAgentRef* FindByConnectionInfo(const TSoIfConnectionInfo& aConnInfo) const;

	TInt NumAgents() const;
	CNifAgentRef* Agent(TInt aPos) const;
	void AddAgentL(CNifAgentRef* aAgent);
	void AgentGoingDown(CNifAgentRef& aAgent);
	void AgentGoingUp(CNifAgentRef& aAgent);

	CNifIfBase* CreateInterfaceL(const TDesC& aName, MNifIfNotify* aNotify);

	static CNifMan* Global();
	void NetworkLayerClosed(MNifIfUser* aNetwork);
	void UpdateIniConfigL();
	static void CheckSSThread();

	inline CNifMonitor& Monitor() { return *iMonitor; };
	inline CNetworkControllerBase* NetworkController() { return iNetworkController; }

private:
	void LoadNetworkControllerL(const TDesC& aName);

private:
	CNifAgentRef* DoFindOrCreateL(const TDesC& aName, TBool aCreate, TBool aCreateNewInstance = EFalse);
	CNifFactory* DoFindFactoryL(TUid aUid2, const TDesC& aFilename, CObjectCon& aCon, TBool aCreate);
	void DoGetFileNameL(const TDesC& aName, TDes& aFilename);
	TInt StartEsWatchThread();

	CObjectCon* iAgents;
	CObjectCon* iAgentFactories;
	CObjectCon* iIfFactories;
	CObjectConIx* iContainers;

	CNifMonitor* iMonitor;

	CNifServer* iServer;
	CMBufPoolReaper* iMbPoolReaper;
	HBufC* iDefMatch;
	HBufC* iDefFilename;
	TInt iCurrentMBufMaxHeap;
	TInt iRequiredMBufMaxHeap;

	RLibrary iNetConLib;
	CNetworkControllerBase* iNetworkController;
    };

class CNifSession : public CConnectionProvdBase, public MNifSessionNotify, public MAgentSessionNotify, public MServiceChangeObserver, public MNetworkControllerObserver
/**
@internalComponent
*/
	{
public:

	static CNifSession* NewL(MConnectionNotify* aConnection);
	virtual ~CNifSession();

	/** CConnectionProvdBase Interface */
	virtual void StartL(TConnStartType aConnectType);
	virtual void StartL(TConnStartType aConnectType, const TConnPref& aPref);
	virtual void AttachToConnectionL(const TDesC8& aConnectionInfo, TBool aMonitorAttach);
	virtual void Stop(TInt aError);
	virtual void ProgressL(TNifProgressBuf& aBuffer) const;
	virtual void LastProgressError(TNifProgressBuf& aBuffer);
	virtual void RequestServiceChangeNotificationL();
	virtual void CancelServiceChangeNotification();
	virtual void GetIntSettingL(const TDesC& aSettingName, TUint32& aValue);
	virtual void GetBoolSettingL(const TDesC& aSettingName, TBool& aValue);
	virtual void GetDes8SettingL(const TDesC& aSettingName, TDes8& aValue);
	virtual void GetDes16SettingL(const TDesC& aSettingName, TDes16& aValue);
	virtual HBufC* GetLongDesSettingLC(const TDesC& aSettingName);
	virtual void EnumerateConnectionsL(RPointerArray<TConnectionInfo>& aConnectionInfoBuffer);
	virtual TBool AllInterfaceNotificationL();
	virtual void ControlL(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, const RMessagePtr2* aMessage);
	virtual void SetUsageProfile(TUint aProfile);

	/** CConnectionProvdBase interface implementation for subconnections */
	virtual void Stop(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aError);
	virtual void ProgressL(TSubConnectionUniqueId aSubConnectionUniqueId, TNifProgressBuf& aBuffer) const;
	virtual TInt EnumerateSubConnections(TUint& aCount);
	virtual TInt GetSubConnectionInfo(TUint aIndex, TDes8& aSubConnectionInfo);
	virtual TInt GetSubConnectionInfo(TDes8& aSubConnectionInfo);
	virtual TInt AllSubConnectionNotificationEnable();
	virtual TInt CancelAllSubConnectionNotification();
	virtual TInt DataTransferred(TSubConnectionUniqueId aSubConnectionUniqueId, TUint& aUplinkVolume, TUint& aDownlinkVolume);
	virtual TInt DataTransferredCancel(TSubConnectionUniqueId aSubConnectionUniqueId);
	virtual TInt SetDataSentNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkGranularity);
	virtual TInt DataSentNotificationCancel(TSubConnectionUniqueId aSubConnectionUniqueId);
	virtual TInt SetDataReceivedNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkGranularity);
	virtual TInt DataReceivedNotificationCancel(TSubConnectionUniqueId aSubConnectionUniqueId);

	/** MNifSessionNotify */
	virtual void ProgressNotification(TInt aStage, TInt aError);
	virtual void ProgressNotification(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume);
	virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume);
	virtual void SubConnectionEvent(const TSubConnectionEvent& aSubConnectionEvent);

	/** MAgentSessionNotify
	ProgressNotification() moved to MNifSessionNotify, see MAgentSessionNotify definition for details */
	virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType);

	/** MNetworkControllerObserver */
	virtual void SelectComplete(const TDesC& aName);
	virtual void SelectComplete(TInt aError);
	virtual void ReconnectComplete(TInt aError);

	void AnInterfaceStateChange(CNifAgentRef& aAgent, TConnInterfaceState aState);
	inline CNifAgentRef* AgentRef();

	inline void SetConnStartType(TConnStartType aType);

	void DetachFromConnection();

   	virtual void SendIoctlMessageL(const RMessage2& aMessage);
   	virtual void SendCancelIoctl();
   	
//PREQ399_REMOVE
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
    virtual CNifIfBase* GetNif(TInt aIndex);
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS //PREQ399_REMOVE

protected:
	CNifSession();
	void CreateL(MConnectionNotify* aConnection);
	void SetProgress(TInt aStage, TInt aError);
	void SetProgress(const TNifProgress& aProgress);
	void SetLastProgressError(TInt aStage, TInt aError);
	void SendProgressUpdate(const TNifProgress& aProgress);

private:
	void GetSelectionInfoL(TSoIfConnectionInfo& aSelectionInfo);
	void PostProgressToConnection(TInt aStage, TInt aError);
	void PostProgressToConnection(TInt aStage, TInt aError, const TDesC8& aInfo);
	inline CNifMan* Global() { return iGlobal; };
	void ProgressNotification(const TNifProgress& aProgress);
	void LoadAgentL(const TDesC& aName = KNullDesC());
	/** connection retry functionality */
	TBool DecideWhetherToRetry(TInt aStage, TInt aError);
	void StartConnectionRetry();
	void ResetConnectionFlags();
	void SetOriginalConnError(TInt aError);
	TBool StartByAgentNameEnabled() const;

	static TInt ProgressCb(TAny* aThisPtr);

private:
	CAsyncCallBack* iProgressCb;
	TProgressQueue iProgressQueue;
	CNifAgentRef* iAgentRef;
	TNifProgress iProgress;
	TNifProgress iLastProgressError;
	TBool iRetryConnection:1;
	TBool iLinkLayersOpen:1;
	TBool iTimersDisabled:1;
    TBool ibRequestServiceChangeNotification:1;
	TConnStartType iConnectType;
	TConnPref iConnectPrefs;
	TInt iConnectionAttempt;
	TInt iLastConnectionError;
	TInt iOriginalConnError;
	CNetworkControllerBase* iNetCon;
	CNifMan* iGlobal;
	TBool iIsMonitoring;

	enum TStartByAgentNameState
		{
		EAgentUninitialised = 0x0,
		EAgentLoaded = 0x1,
		EAgentStarted = 0x2,
		};

	TStartByAgentNameState iStartByAgentNameState;
	TUint iUsageProfile;
	};

class CEsIni : public CESockIniData
/**
@internalComponent
*/
	{
public:
	static CEsIni* NewL();
	};

class RNifConfig : public RSessionBase
/**
@internalComponent
*/
	{
public:
	TInt Open(TInt aSlots=KDefaultNifSlots);
	static TInt CreateNifSession(RSessionBase& aSession, const TVersion& aVersion, TInt aSlots);
	static TInt SendReceive(RSessionBase& aSession, TInt aFunction,TAny *aPtr);
	};

/**
@internalComponent
*/
_LIT(KNifManServer,"NifmanServer");
_LIT(KNifManModule,"Nifman");
_LIT(KAgentExtension,".agt");

_LIT(KNifManSection,"nifman");
_LIT(KNifManDefault,"default");
_LIT(KNifManMaxHeapSize,"maxmbufheap");
_LIT(KNifManAgents,"agents");
_LIT(KNifManInterfaces,"interfaces");
_LIT(KNetConLibraryName, "networkcontroller");
_LIT(KDefaultNetConLibName, "NETCON.DLL");

inline CNifAgentRef* CNifSession::AgentRef()
	{ return iAgentRef; }

inline void CNifSession::SetConnStartType(TConnStartType aType)
	{ iConnectType = aType; }

inline void CNifAgentRef::ConnectionControlActivity()
	{ iConnectionControlActivity = ETrue; };

inline void CNifAgentRef::SetConnectionInfo(const TSoIfConnectionInfo& aConnInfo)
	{ iConnInfo = aConnInfo; }

inline void CNifAgentRef::SetTimerMode(TTimerType aTimerMode)
	{ iTimerMode = aTimerMode; ResetTimer(); };

inline const TSoIfConnectionInfo& CNifAgentRef::ConnectionInfo() const
	{ return iConnInfo; }

inline TBool CNifAgentRef::MatchesConnectionInfo(const TSoIfConnectionInfo& aConnInfo) const
	{ return iConnInfo.operator==(aConnInfo); };

inline CNifAgentBase& CNifAgentRef::Agent() const
	{ return *iAgent; }

inline const TNifProgress& CNifAgentRef::Progress() const
	{ return iProgress; }

#endif
