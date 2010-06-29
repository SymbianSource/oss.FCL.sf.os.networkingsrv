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
 @file CONNECTIONSELECTORSHIM.H
 @internalComponent
*/

#if !defined(__CONNECTIONSELECTORSHIM_H__)
#define __CONNECTIONSELECTORSHIM_H__

#include <ss_fact.h>
#include <ss_select.h>
#include <ss_connprov.h>
#include <es_prot.h>
#ifdef SYMBIAN_NETWORKING_UMTSR5
#include <comms-infras/nifif.h>
#include <comms-infras/dbaccess.h>
#include <commdbconnpref.h>
#include "shimcprfactory.h" 
#endif //SYMBIAN_NETWORKING_UMTSR5

class CConnectionProviderShim;
class CConnectionSelectorShim : public CBase, public MProviderSelector, public MConnectionNotify
/**
@internalTechnology
@released Since 9.1
*/
	{	
	friend class CConnectionProviderFactoryShim;
	
public:	

#ifdef SYMBIAN_NETWORKING_UMTSR5
	
	// This function will be called by the ShimCprFactory to set its interface for the information
	// pertaining to the IAP lock status
	void  SetFactoryIface(MIAPLockInfo * aFactoryIface);
	
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5
	
protected:
	CConnectionSelectorShim(ISelectionNotify& aNotify);
	virtual ~CConnectionSelectorShim();	
    void DeleteAsync();
	
public:
    TInt Select(Meta::SMetaData& aPreferences, const RMessagePtr2* aMessage);
      
    // From MConnectionNotify
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

	//MProviderSelector
	virtual TInt Cancel(TInt aReason, const RMessage2* aMessage);
	virtual TInt Cancel();
	void SetProviderNull();
private:
	static TInt AsyncDestructorCb(TAny* aInstance);
	void CreateProviderL();
	void HandleSelectionL();
#ifdef SYMBIAN_NETWORKING_UMTSR5
	void MaybeLockIapL();
#endif	
	TBool IsConnectionStoppingL();
	
private:
    CConnectionProvdBase* iSelectorSession;
    ISelectionNotify iNotify;
    CAsyncCallBack iAsyncDestructor;
    RBuf8 iSelectionInfo;
    CConnectionProviderShim* iConnProvider;
    TInt iError;
#ifdef SYMBIAN_NETWORKING_UMTSR5
    TBool iSetProviderNull;
    TBool iIsLinkLayerOpen;
#else
    TBool iSetProviderNull:1;
    TBool iIsLinkLayerOpen:1;
#endif  
	// New Variables added as required by 635 to achieve IAP locking
#ifdef SYMBIAN_NETWORKING_UMTSR5
	// The Connection Info, IAP , etc.
	TSoIfConnectionInfo iConnectionInfo;
	// The Secure ID of the current Application
    TSecureId 			iAppSecureId;
    // Interface to the Factor which Stores and get the IAP Lock Status
    MIAPLockInfo*		iFactoryIface;
#endif 

	};


#endif	// __CONNECTIONSELECTORSHIM_H__

