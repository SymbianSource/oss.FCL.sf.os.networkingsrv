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
// IPProto Connection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTOCPR_H
#define SYMBIAN_IPPROTOCPR_H

#include <comms-infras/corecpr.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_datamonitoringprovider.h>

#include "idletimer.h"

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KIPProtoCprTag KESockConnectionTag
_LIT8(KIPProtoCprSubTag, "ipprotocpr");
#endif

namespace IpProtoCpr
    {
    class TStoreProvision;
	class TSendStarted;
    class TDoOpenCloseRoute;
    class TProcessDataClientStatusChange;
    class TAwaitingStart;
    class TCleanupStart;
    class TCheckIfLastControlClientLeaving;
    class TAwaitingGoneDown;
	class TLinkDown;
	class TLinkUp;
	class TStoreAndForwardStateChange;
	class TSendStoppedAndGoneDown;
    }

namespace ESock
{
	class MPlatsecApiExt;
	class MLegacyControlApiExt;
	class ADataMonitoringProvider;
	class ADataMonitoringProtocolReq;
	class TDataMonitoringConnProvisioningInfo;
}

NONSHARABLE_CLASS(CIPProtoConnectionProvider) : public CCoreConnectionProvider,
	public ESock::ADataMonitoringProvider,
	public ESock::MLegacyControlApiExt,
	public ESock::ALegacySubConnectionActiveApiExt,
	public ESock::ALegacyEnumerateSubConnectionsApiExt,
	public ITFHIERARCHY_LINK_5(CIPProtoConnectionProvider,
			CCoreConnectionProvider,
			ESock::ADataMonitoringProtocolReq,
			ESock::MLinkCprApiExt,
			ESock::MLegacyControlApiExt,
			ESock::ALegacySubConnectionActiveApiExt,
			ESock::ALegacyEnumerateSubConnectionsApiExt)
/**
 IPProto connection provider

 @internalTechnology
 @released Since 9.4
 */
    {
friend class CIPProtoConnectionProviderFactory;
friend class IpProtoCpr::TStoreProvision;
friend class IpProtoCpr::TDoOpenCloseRoute;
friend class TEnumerateConnectionsQuery;
friend class IpProtoCpr::TProcessDataClientStatusChange;
friend class IpProtoCpr::TSendStarted;
friend class IpProtoCpr::TAwaitingStart;
friend class IpProtoCpr::TAwaitingGoneDown;
friend class IpProtoCpr::TCleanupStart;
friend class IpProtoCpr::TCheckIfLastControlClientLeaving;
friend class IpProtoCpr::TLinkDown;
friend class IpProtoCpr::TLinkUp;
friend class IpProtoCpr::TStoreAndForwardStateChange;
friend class IpProtoCpr::TSendStoppedAndGoneDown;
friend class CLinkCprExtensionApi;




	public:
    typedef ITFHIERARCHY_LINK_5(CIPProtoConnectionProvider,
    		CCoreConnectionProvider,
    		ESock::ADataMonitoringProtocolReq,
    		ESock::MLinkCprApiExt,
    		ESock::MLegacyControlApiExt,
    		ALegacySubConnectionActiveApiExt,
    		ESock::ALegacyEnumerateSubConnectionsApiExt) TIfStaticFetcherNearestInHierarchy;

public:
	static CIPProtoConnectionProvider* NewL(ESock::CConnectionProviderFactoryBase& aFactory);
	virtual ~CIPProtoConnectionProvider();

public:
	void ReturnInterfacePtrL(ADataMonitoringProtocolReq*& aInterface);
	void ReturnInterfacePtrL(ESock::MLinkCprApiExt*& aInterface);
	void ReturnInterfacePtrL(ESock::MLegacyControlApiExt*& aInterface);
	void ReturnInterfacePtrL(ESock::ALegacySubConnectionActiveApiExt*& aInterface);
	void ReturnInterfacePtrL(ESock::ALegacyEnumerateSubConnectionsApiExt*& aInterface);

	// From MLegacyControlApiExt
	virtual TInt ControlL(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, ESock::MPlatsecApiExt* aPlatsecItf);

	void DataClientIdle();

	// From ALegacyEnumerateSubConnectionsApiExt
	virtual void EnumerateSubConnections(ESock::CLegacyEnumerateSubConnectionsResponder*& aResponder);
	virtual void GetSubConnectionInfo(TSubConnectionInfo &aInfo);
	
    TInt iProvisionError;
    ESock::TDataMonitoringSubConnProvisioningInfo* iSubConnProvisioningInfo;
	
protected:
	enum TTimerType { ETimerUnknown, ETimerShort, ETimerMedium, ETimerLong, ETimerImmediate, ETimerMax };

protected:
    CIPProtoConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
    void ConstructL();

    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
    virtual void Received(MeshMachine::TNodeContextBase& aContext);

private:
	class COneShotTimer : public CTimer
		{
	public:
		static COneShotTimer* NewL(TInt aPriority, CIPProtoConnectionProvider* aOwner);

	protected:
		COneShotTimer(TInt aPriority, CIPProtoConnectionProvider* aOwner);
		void ConstructL();

	protected:
		virtual void RunL();

	private:
		CIPProtoConnectionProvider* iOwner;
		};

private:
	void SetUsageProfile(TInt aProfile);
	void TimerExpired();
	void TimerComplete(TInt aError);
	void SetTimerMode(TTimerType aTimerMode);
	void TimerAfter(TInt aInterval);
	TTimerType DecideTimerMode(TInt aRouteCount);
	void StartNextTick();
	void LinkUp();
	void LinkDown();
	TInt ControlClientsCount();
	void SetTimers(TUint32 aShortTimer, TUint32 aMediumTimer, TUint32 aLongTimer);
	void OpenRoute();
	void CloseRoute();
	void CancelTimer();
	void ResetTimer();
	void DisableTimers();
	void EnableTimers();
	void StopConnection();
	void ForceCheckShortTimerMode();

private:
	COneShotTimer* iTimer;
	TInt iTimerMode;
	TInt iTickThreshold[ETimerMax];
	TInt iExpiredTicks;
	TInt iTotalTimerDrift;
	TTime iDriftCheckTime;
    ESock::TDataMonitoringConnProvisioningInfo iDataMonitoringConnProvisioningInfo;
	TInt iLastControlClientsCount;
	TInt iRouteCount;
	TBool iRouteCountStretchOne;
	TBool iPeriodActivity;
	TInt iTimerDisableCount;
	TTime iStartTime;
	TBool iLinkUp:1;
	TBool iConnectionControlActivity:1;
	TBool iTimerExpired:1;
	TBool iTimerStopped:1;
	TBool iTimerRunning:1;
	TBool iSubConnEventDataSent:1;	// Hack to cope with multiple DataClientStatusChange notifications for 'stopped'
	TBool iNodeLocalExtensionsCreated;
	ESock::RMetaExtensionContainer iNodeLocalExtensions;
    };


#endif //SYMBIAN_IPPROTOCPR_H

