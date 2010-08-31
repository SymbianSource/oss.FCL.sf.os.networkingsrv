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
//

/**
 @file
 @internalComponent
*/


#if !defined(__SHIMNIFMANSCONN_H__)
#define __SHIMNIFMANSCONN_H__

#include <comms-infras/ss_log.h>
#include "shimdatatransfer.h"
#include "shimclient.h"
#include "shimcpr.h" //access to the CConnectionProviderShim::Provider() fn

//const TInt KSubConnectionProviderShimImplementationUid = 0x10207106;

#define KShimScprTag KESockSubConnectionTag
_LIT8(KShimScprSubTag, "shimscpr");
_LIT8(KShimScprDataTag, "shimscprData");
_LIT8(KShimScprClientTag, "shimscprClient");

//Former CInterface CSubConnection related up-calls so that CConnectionProviderShim
//doesn't have to link against the sub-connection shim
class CConnection;
class MSubInterfaceShim : public MConnDataTransferNotify
	{
public:
	virtual TInt ProgressNotification(TInt aStage, TInt aError, const TDesC8& aInfo) = 0;
	virtual TSubConnectionUniqueId Id() = 0;
	virtual void SetSubConnectionUniqueId( TSubConnectionUniqueId aSubConnectionUniqueId ) = 0;
	virtual void ConnectionJoiningL(const CConnection& aConnection) = 0;
	virtual void ConnectionLeaving(const CConnection& aConnection) = 0;
	};

//class CSubConnectionProviderFactoryShim;
class CSubConnectionLinkShimClient;

NONSHARABLE_CLASS(CNifManSubConnectionShim) : public CBase, public MConnectionDataClient, public MSubInterfaceShim
/**
 @internalComponent
 */
	{

public:
	CNifManSubConnectionShim(CConnectionProviderShim& aProviderShim);
	~CNifManSubConnectionShim();

	// MSubInterfaceShim impl
	virtual TInt ProgressNotification(TInt aStage, TInt aError, const TDesC8& aInfo);
	virtual TInt NotifyDataTransferred(TUint aUplinkVolume, TUint aDownlinkVolume);
	virtual TInt NotifyDataSent(TUint aUplinkVolume, TUint aCurrentGranularity);
	virtual TInt NotifyDataReceived(TUint aDownlinkVolume, TUint aCurrentGranularity);
	virtual TSubConnectionUniqueId Id();
	virtual void SetSubConnectionUniqueId( TSubConnectionUniqueId aSubConnectionUniqueId );
	virtual void ConnectionJoiningL(const CConnection& aConnection);
	virtual void ConnectionLeaving(const CConnection& aConnection);

	CConnectionProvdBase& Provider()
		{
		return static_cast<CConnectionProviderShim*>(iConnectionProvider)->Provider();
		}
	CConnDataTransferShim* DataTransferShim()
		{
		return iConnDataTransferShim;
		}
	CSubConnectionLinkShimClient* ShimClient( TInt aIndex )
		{
		return aIndex < iShimClients.Count() ? iShimClients[aIndex] : NULL;
		}
	void DeleteAsync();	
protected:
	//MConnectionDataClient
	virtual void ConnectionGoingDown(CConnectionProviderBase& aConnProvider);
	virtual void ConnectionError(TInt aStage, TInt aError);
	virtual void Notify(TNotify aNotifyType,  CConnectionProviderBase* aConnProvider, TInt aError, const CConNotificationEvent* aConNotificationEvent);
      virtual void AttachToNext(CSubConnectionProviderBase* aSubConnProvider);


	TInt FindClient(const CConnection& aConnection);
      CConnDataTransfer& CreateDataTransferL();

protected:
	RPointerArray<CSubConnectionLinkShimClient> iShimClients;
	TSubConnectionUniqueId iSubConnectionsUniqueId;
	CConnDataTransferShim* iConnDataTransferShim;
	CConnectionProviderBase* iConnectionProvider;
	CAsyncCallBack iAsyncDestructor;
private:
	static TInt AsyncDestructorCb(TAny* aInstance);
	};

NONSHARABLE_CLASS(CSubConnectionLinkShimClient) : public CBase, public MConnDataTransferNotify, public MShimControlClient
/**
 Handle subconnection-related asynchronous client requests that may not complete immediately, it effectively
 represents the old CSubConnection
 @internalComponent
 */
	{
public:
	CSubConnectionLinkShimClient(const CConnection& aConnection, CNifManSubConnectionShim& aSubConnectionShim);
	~CSubConnectionLinkShimClient();

	//MConnDataTransferNotify interface towards CConnDataTransfer
	virtual TInt NotifyDataTransferred(const TUint aUplinkVolume, const TUint aDownlinkVolume);
	virtual TInt NotifyDataSent(TUint aUplinkVolume, TUint aCurrentGranularity);
	virtual TInt NotifyDataReceived(TUint aDownlinkVolume, TUint aCurrentGranularity);
	
	void ProgressNotification(TInt aStage, TInt aError, const TDesC8& aInfo);

	//MShimControlClient interface towards ESOCK
	//former CConnection::StopSubConnectionL(const RMessage2& aMessage);
	virtual TBool StopL(const RMessage2& aMessage);
	// Former calls from CConnection::GetSubConnectionInfo
	virtual TInt GetSubConnectionInfo(const RMessage2& aMessage);
	//	Former Calls from RConnection via CSubConnection
	virtual TInt GetCurrentProgress(TNifProgress& aProgress);
	virtual TBool DataTransferredL(const RMessage2& aMessage);
	virtual TBool DataTransferredCancel(const RMessage2& aMessage);
	virtual TBool RequestSubConnectionProgressNotificationL(const RMessage2& aMessage);
	virtual TBool CancelSubConnectionProgressNotification(const RMessage2& aMessage);
	virtual TBool DataSentNotificationRequestL(const RMessage2& aMessage);
	virtual TBool DataSentNotificationCancel(const RMessage2& aMessage);
	virtual TBool DataReceivedNotificationRequestL(const RMessage2& aMessage);
	virtual TBool DataReceivedNotificationCancel(const RMessage2& aMessage);
	virtual TBool IsSubConnectionActiveRequestL(const RMessage2& aMessage);
	virtual TBool IsSubConnectionActiveCancel(const RMessage2& aMessage);
	TBool Match(const CConnection& aConnection) const;
	TSubConnectionUniqueId Id();
	virtual TInt ReturnCode() const;

	void SetReturnCode(TInt aErr)
		{
		iReturnCode = aErr;
		}

	/**
	Callback from activity timer
	*/
	void CheckSubConnectionActivity();

	NONSHARABLE_CLASS(CActivityTimer) : public CTimer
	/**
	@internalComponent
	*/
		{
	public:
		static CActivityTimer* NewL(CSubConnectionLinkShimClient* aOwner, TInt aPriority);
		virtual inline ~CActivityTimer() {};
		void RunL();
	private:
		inline CActivityTimer(CSubConnectionLinkShimClient* aOwner, TInt aPriority);
	private:
		CSubConnectionLinkShimClient* iOwner;
		};


private:
	const CConnection& iConnection;
	CNifManSubConnectionShim& iSubConnectionShim;
	
	TUint iUplinkDataVolume;
	TUint iDownlinkDataVolume;
	TUint iUplinkDataNotificationVolume;
	TUint iDownlinkDataNotificationVolume;
	TInt iRemainingUplinkGranularity;
	TInt iRemainingDownlinkGranularity;

	TBool iDataSentNotificationsInAbsoluteMode;
	TBool iDataReceivedNotificationsInAbsoluteMode;

	TNifProgress iCurrentProgress;
	TInt iLastProgressToClient;
	TInt iClientRequestedProgress;
	CActivityTimer* iActivityTimer;
	TInt iRequestedClientTimerPeriod;
	TBool iClientBelievesSubConnectionActive;
	TUint iPreviousUplinkDataVolume;
	TUint iPreviousDownlinkDataVolume;

	TBool iOutstandingProgressNotification:1;
	TBool iOutstandingDataSentNotification:1;
	TBool iOutstandingDataReceivedNotification:1;
	TBool iOutstandingSubConnectionActivity:1;

	RMessage2 iOutstandingProgressNotificationMessage;
	RMessage2 iOutstandingDataSentNotificationMessage;
	RMessage2 iOutstandingDataReceivedNotificationMessage;
	RMessage2 iOutstandingSubConnectionActivityMessage;
	
	TInt iReturnCode;
	};

inline CSubConnectionLinkShimClient::CActivityTimer::CActivityTimer(CSubConnectionLinkShimClient* aOwner, TInt aPriority) 
: CTimer(aPriority), iOwner(aOwner)
/**
Set up the activity timer for subconnection activity

@param aOwner, The owing subconnection
@param aPriority, The priority of the active object
*/
	{ CActiveScheduler::Add(this); };
	
#endif
// __SHIMNIFMANSCONN_H__
