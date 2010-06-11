/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header file for the default SubConnection Provider
* 
*
*/



/**
 @file deft_scpr.h
*/

#ifndef __DEFT_CPR_H__
#define __DEFT_CPR_H__

#include <e32base.h>
#include <e32std.h>
#include <ss_subconnprov.h>
#include <ss_connprov.h>
#include "ipscprlog.h"


class CConnDataTransfer;
class CEmptySubConnectionProvider : public CSubConnectionProviderBase, public MConnectionEnumerateClients, public MConnectionDataClient
/**
Defines the IP Connection Provider.  Class provides a mapping from ESock Subconnection
function calls to QoS.PRT messages.

@internalComponent

@released Since v9.0
*/
	{
protected:
	// Construction
	CEmptySubConnectionProvider(CSubConnectionProviderFactoryBase& aFactory, CConnectionProviderBase& aConnProvider) :
		CSubConnectionProviderBase(aFactory, aConnProvider)
			{
			__IPCPRLOG(IpCprLog::Printf(_L("CEmptySubConnectionProvider [this=%08x]:\tCEmptySubConnectionProvider() [MConnectionDataClient=%08x]"),
			   this, (MConnectionDataClient*)this));
			}
			
	~CEmptySubConnectionProvider();
	
	virtual MConnectionDataClient* DoSelfConnectionDataClient();

	//MConnectionEnumerateClients
	virtual void EnumerateClientsL(TUint& aCount, TDes8& aDes, CConnectionProviderBase::TEnumClients aClientType);

	//MConnectionDataClient
	virtual void ConnectionError(TInt aStage, TInt aError);

	virtual void DoControlClientJoiningL(MSubConnectionControlClient& aControlClient);
	virtual void DoControlClientLeaving(MSubConnectionControlClient& aControlClient);
	};
	

/**
Defines the default IP Connection Provider. 

@internalComponent

@released Since v9.0
*/
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
class CDefaultSubConnectionProvider : public CEmptySubConnectionProvider, public MSubConnectionControlClient
#else
class CDefaultSubConnectionProvider : public CEmptySubConnectionProvider
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	{
public:

	// Construction
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	CDefaultSubConnectionProvider(CSubConnectionProviderFactoryBase& aFactory, CConnectionProviderBase& aConnProvider, RSubConnection::TSubConnType aType)
	   : CEmptySubConnectionProvider(aFactory, aConnProvider), iSubConnType(aType)
#else
	CDefaultSubConnectionProvider(CSubConnectionProviderFactoryBase& aFactory, CConnectionProviderBase& aConnProvider)
	   : CEmptySubConnectionProvider(aFactory, aConnProvider)
#endif
			{
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
			__IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider [this=%08x]:\tCDefaultSubConnectionProvider() [MSubConnectionControlClient=%08x] [MConnectionDataClient=%08x]"),
			   this, (MSubConnectionControlClient*)this, (MConnectionDataClient*)this));
#else
			__IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider [this=%08x]:\tCDefaultSubConnectionProvider() [MConnectionDataClient=%08x]"),
			   this, (MConnectionDataClient*)this));
#endif			   
			}
			
	~CDefaultSubConnectionProvider();

#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	//-=========================================================
	// MSubConnectionControlClient methods
	//-=========================================================
	/**	Override this to inform the sub-connection client (e.g. socket, host resolver etc.) that 
		the sub-connection is going down
	
	@param aSubConnProvider the sub-connection provider going down */
	virtual void SubConnectionGoingDown(CSubConnectionProviderBase& aSubConnProvider);

	/**	Override this to notify the control client of a sub-connection event, eg. layer up etc.
	
	@param aSubConnProvider The provider which the event was generated on
	@param aNotifyType The type of event
	@param aError Error code, if any
	@param aEvent The notification object containing specialized information */
	virtual void SubConnectionEvent(CSubConnectionProviderBase& aSubConnProvider, MConnectionDataClient::TNotify aNotifyType, TInt aError, const CSubConNotificationEvent* aEvent);


	virtual void LayerUp(CSubConnectionProviderBase& aSubConnProvider, TInt aError);
	virtual void IncomingConnection(CSubConnectionProviderBase* aSubConnProvider, CSubConParameterBundle* aParameterBundle, TInt aError);
#endif
// SYMBIAN_NETWORKING_3GPPDEFAULTQOS

protected:
	//-=========================================================
	// CSubConnectionProviderBase methods
	//-=========================================================
	// Methods to be overriden be derived subconnection provider
	virtual void DoControlClientJoiningL(MSubConnectionControlClient& aControlClient);	//Fix for DEF096132
	virtual void DoDataClientJoiningL(MSubConnectionDataClient& aDataClient);
	virtual void DoDataClientLeaving(MSubConnectionDataClient& aDataClient);
	virtual void DoSourceAddressUpdate(MSubConnectionDataClient& aDataClient, const TSockAddr& aSource);
	virtual void DoDestinationAddressUpdate(MSubConnectionDataClient& aDataClient, const TSockAddr& aDestination);
	virtual void DoDataClientRouted(MSubConnectionDataClient& aDataClient, const TSockAddr& aSource, const TSockAddr& aDestination, const TDesC8& aConnectionInfo);
	virtual void DoParametersAboutToBeSetL(CSubConParameterBundle& aParameterBundle);
	virtual TInt DoControl(TUint aOptionLevel, TUint aOptionName, TDes8& aOption);

   virtual void DoStartL();
	virtual void DoStop();
	virtual CSubConnectionProviderBase* DoNextLayer();
	virtual CConnDataTransfer& DoDataTransferL();

	//MConnectionDataClient
	virtual TAny* FetchInterfaceInstanceL(CConnectionProviderBase& aProvider, const STypeId& aTid);
	virtual void ConnectionGoingDown(CConnectionProviderBase& aConnProvider);
	virtual void Notify(TNotify aNotifyType,  CConnectionProviderBase* aConnProvider, TInt aError, const CConNotificationEvent* aConNotificationEvent);
   virtual void AttachToNext(CSubConnectionProviderBase* aSubConnProvider);
  
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
   virtual void DoControlClientLeaving(MSubConnectionControlClient& aControlClient);
private:
    const RSubConnection::TSubConnType iSubConnType;
#endif


#ifdef SYMBIAN_NETWORKING_UMTSR5
    TUint32 iAppId;
	CSubConnectionProviderBase * iSubConNextLayer;
#endif
	};
#endif
// __DEFT_CPR_H__

