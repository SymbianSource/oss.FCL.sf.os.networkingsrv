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
 @file IPCPRFACTORY.H
 @internalComponent
*/

#if !defined(__IPCPRFACTORY_H__)
#define __IPCPRFACTORY_H__

#include <ss_connprov.h> // CConnectionProviderFactoryBase, MCommsFactoryQuery
#include <in_sock.h> // KAfInet
#include <agentdialog.h> // RGenConAgentDialogServer
#include <commsdebugutility.h> // __FLOG_DECLARATION_MEMBER

const TUint KIPConnectionProviderFactoryId = KAfInet;

class XConnectionIPFactoryQuery : public MCommsFactoryQuery
	{
public:
	XConnectionIPFactoryQuery( CConnectionProviderBase* aConnectionProviderBase ) :
		iConnectionProviderBase( aConnectionProviderBase )
		{
		}

protected:
	CConnectionProviderBase* iConnectionProviderBase;

public:
	virtual TMatchResult Match( TFactoryObjectInfo& aConnectionInfo );
	};

class CIPNetworkProviderFactory : public CConnectionProviderFactoryBase
	{
public:
	static CIPNetworkProviderFactory* NewL(TAny* aParentContainer);

protected:   
	CIPNetworkProviderFactory(TUint aFactoryId, CConnectionFactoryContainer& aParentContainer);
	virtual CConnectionProviderBase* DoCreateProviderL();
	virtual MProviderSelector* DoSelectProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* aMessage );
	virtual MProviderSelector* DoSelectNextLayerProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* aMessage );
	virtual void DoEnumerateConnectionsL(RPointerArray<TConnectionInfo>& aConnectionInfoPtrArray);
	};

class TCommIdList;
namespace ESock
	{
	class CConnStart;
	}
namespace  CommsDat
	{
	class CMDBSession;
	class CCDGlobalSettingsRecord;
    class CCDIAPPrioritySelectionPolicyRecord;
	}

class CIPConnectionSelector : public CActive, public MProviderSelector
/** IP-layer connection selector. IP level factory creates
these to intercept and complete the Shim layer selection
it triggers.

@internalComponent
@released Since 9.1
*/
	{
public:
	TInt Select(Meta::SMetaData& aPreferences, const RMessagePtr2* aMessage);
	void SelectComplete(CConnectionProviderBase* aConnProvider, TInt aError);
	void ProgressNotification(TInt aStage, TInt aError);
	void LayerUp(TInt aError);
	void ServiceChangeNotification(TUint32 aId, const TDesC& aType);
	void SubConnectionEvent(CSubConnectionProviderBase* aSubConnNextLayerProvider, const TSubConnectionEvent& aSubConnectionEvent);
	void Detach();
	
	virtual TInt Cancel();
	virtual TInt Cancel(TInt aReason, const RMessage2* aMessage);

	//Only my own factory can create me but the constructor may be public because noone can link against it anyway.
	//My factory could be my friend and the constructor priate but then my factory could mistakenly delete me!
	CIPConnectionSelector(ISelectionNotify& aNotify, CIPNetworkProviderFactory& aFactory);

private:
	virtual ~CIPConnectionSelector(); //Nobody should delete me! Only I can delete myself.

	// From CActive
	virtual void DoCancel();
	virtual void RunL();
	virtual TInt RunError(TInt aError);

    // Helper functions
	static TInt DestroyMyself(TAny* aSelf);
	void SelectL(Meta::SMetaData& aPreferences);
	void SelectLinkLayerL();
	void FillListL(CommsDat::CCDIAPPrioritySelectionPolicyRecord& aPolicy);
    CommsDat::CCDGlobalSettingsRecord* LoadGlobalSettingsRecordLC();
    CommsDat::CCDIAPPrioritySelectionPolicyRecord* LoadPolicyRecordLC(TInt aAccessPoint);

private:
    //ISelectionNotify must be stored by value, cos' it's just a short-lived wrapper class.
    //It doesn't exist as a someone that waits for the completion, but stores refereneces
    //to the one that does.
    ISelectionNotify iNotify;
    CIPNetworkProviderFactory& iFactory;
    MProviderSelector* iNextLayerSelector;
	RGenConAgentDialogServer iDlgServ;
	TUint32 iAPid;
	RMessagePtr2 iSelectMessage;
	CAsyncCallBack iAsyncDestructor;
	CommsDat::CMDBSession* iDbs;
	ESock::CConnStart* iConnStart;
#ifdef SYMBIAN_NETWORKING_UMTSR5
	TSecureId iAppSecureId;
#endif 
	__FLOG_DECLARATION_MEMBER;
	TConnPref* iPrefs;
	};

#endif // __IPCPRFACTORY_H__
