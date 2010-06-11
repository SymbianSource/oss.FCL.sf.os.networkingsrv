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
 @file SHIMCPRFACTORY.H
 @internalComponent
*/

#if !defined(__SHIMCPRFACTORY_H__)
#define __SHIMCPRFACTORY_H__

#include <ss_connprov.h>		// for CConnectionProviderFactoryBase
#include <cflog.h>

__CFLOG_STMT(_LIT8(KLogSubSysESOCK, "ESOCK");) // subsystem name
	

#ifdef SYMBIAN_NETWORKING_UMTSR5   

// The class CConnectionProviderFactoryShim implements this interface as part of the 635 IAP locking mechanism. Since
// each RConn::Start(), will instantiate its own selector, its then become the resposiblity of the factory to store the
// status related to the IAP locking. This interface has the functions called by the selectors with the status of IAP lockied
// and the IAP number to be locked

class MIAPLockInfo
{
public:
	virtual void SetIAPLockStatus(TBool aLockStatus, TInt aLockedIAP)=0;	
	virtual void GetIAPLockStatus(TBool &aLockStatus, TInt &aLockedIAP)=0;
};	

#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5	
   
class CSubConnectioProviderLinkShim;

const TUint KShimConnectionProviderFactoryId = 0x10207104; //the same as CSubConnectionProviderFactoryShim

class CSubConnectionFactoryContainer;
NONSHARABLE_CLASS(CConnectionProviderFactoryShim) : public CConnectionProviderFactoryBase, public MConnectionNotify
#ifdef SYMBIAN_NETWORKING_UMTSR5
													, public MIAPLockInfo
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5													
	{
public:
	static CConnectionProviderFactoryShim* NewL(TAny* aConstructionParameters);
   	~CConnectionProviderFactoryShim();


protected:
	void ConstructL();
	CConnectionProviderFactoryShim(TUint aFactoryId, CConnectionFactoryContainer& aParentContainer);

	virtual CConnectionProviderBase* DoCreateProviderL();
	virtual MProviderSelector* DoSelectProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* aMessage );
	virtual MProviderSelector* DoSelectNextLayerProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* aMessage );	

	virtual void DoEnumerateConnectionsL(RPointerArray<TConnectionInfo>& aConnectionInfoPtrArray);
	
	//CCommsFactoryBase
	virtual TInt DoReceiveMessage( NetMessages::CMessage& aNetMessage );

	//MConnectionNotify interface to catch the global events
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
#ifdef SYMBIAN_NETWORKING_UMTSR5 
public:
  
	// Interface MIAPLockInfo functions to store the status of IAP locking
	virtual void SetIAPLockStatus(TBool aLockStatus, TInt aLockedIAP);	
	virtual void GetIAPLockStatus (TBool &aLockStatus, TInt &aLockedIAP);
	
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5	
	

protected:
	CConnectionProvdBase* iNifmanSession;
#ifdef SYMBIAN_NETWORKING_UMTSR5	
	
	// This variable will tell the status whether the IAP is locked or not
	TBool iIsIAPLocked;
	// This variable will store the IAP number that is being locked.
	TInt  iLockedIAP;
	
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5
	};

#endif // __SHIMCPRFACTORY_H__
