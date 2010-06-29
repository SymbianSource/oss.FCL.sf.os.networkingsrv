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
* Header file for the IP SubConnection Provider and its basic version
* 
*
*/



/**
 @file ipscpr.h
*/

#ifndef __IPSCPR_H__
#define __IPSCPR_H__

#include <e32base.h>
#include <e32std.h>
#include <ss_subconnprov.h>
#include "deft_scpr.h"
#include <eintsock.h>
#include <in_sock.h>
#include <es_sock.h>


#ifndef BASIC_IPSCPR
#include "asyncwriter.h"
#include <qoslib.h>
#include <ip_subconparams.h>

#ifdef SYMBIAN_NETWORKING_UMTSR5
#include <umtsextn.h>
#include <imsextn.h>
#endif
#endif


/**
Connection Provider Id - Protocol Family

@internalComponent

@released Since v9.0
*/
const TUint KIPConnectionProviderFactoryId = KAfInet;

/**
ECOM Implementation Id for Connection Provider

@internalComponent

@released Since v9.0
*/
#ifdef BASIC_IPSCPR
const TInt KSubConnectionProviderImplementationUid = { 0x102752C5 };
#else
const TInt KSubConnectionProviderImplementationUid = { 0x10204308 };
#endif


class CConnectionProviderBase;
class CIpSubConnectionProviderFactory : public CSubConnectionProviderFactoryBase
/**
Factory that is used to create instances of IP Connection Providers

@internalComponent

@released Since v9.0
*/
	{
public:
	static CIpSubConnectionProviderFactory* NewL(TAny* aConstructionParameters);
	virtual ~CIpSubConnectionProviderFactory();

	CIpSubConnectionProviderFactory(TUint aFactoryId, CSubConnectionFactoryContainer& aParentContainer);

	// methods to be overriden for CSubConnectionProviderFactory
	virtual CSubConnectionProviderBase* DoCreateProviderL(CConnectionProviderBase& aConnProvider, RSubConnection::TSubConnType aType);
	};


#ifndef BASIC_IPSCPR

class CQoSMsgWriter;
class CQoSMsgReader;
class TPfqosMessage;
class CSubConGenEventParamsGranted;
class CSubConQosGenericParamSet;
class CSubConQosIPLinkR99ParamSet;
class CSubConSBLPR5ExtensionParamSet;
class CSblpParameters;
class TQoSParameters;
class TUmtsQoSParameters;


#ifdef SYMBIAN_NETWORKING_UMTSR5  
class TUmtsR5QoSParameters;
class TImsParameter;
class CSubConQosR5ParamSet;
class CSubConIMSExtParamSet;
#endif 
// SYMBIAN_NETWORKING_UMTSR5 


class CIpSubConnectionProvider : public CEmptySubConnectionProvider
/**
Defines the IP Connection Provider.  Class provides a mapping from ESock Subconnection
function calls to QoS.PRT messages.

@internalComponent

@released Since v9.0
*/
	{
private:
    enum 
        {
        KParameterRelInvalid = 0,
        KParameterRelGeneric = 1,
        KParameterRel4Rel99 = 4,
        KParameterRel5 = 5,
        };

public:
	// Construction
	static CIpSubConnectionProvider* NewL(CIpSubConnectionProviderFactory& aFactory, CConnectionProviderBase& aConnProvider);

protected:
	// Construction
	CIpSubConnectionProvider(CIpSubConnectionProviderFactory& aFactory, CConnectionProviderBase& aConnProvider);

	// Methods to be overriden be derived subconnection provider
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
	virtual TAny* FetchInterfaceInstanceL(CSubConnectionProviderBase& aProvider, const STypeId& aTid);
	virtual void ConnectionGoingDown(CConnectionProviderBase& aConnProvider);
	virtual void Notify(TNotify aNotifyType,  CConnectionProviderBase* aConnProvider, TInt aError, const CConNotificationEvent* aConNotificationEvent);
    virtual void AttachToNext(CSubConnectionProviderBase* aSubConnProvider);

	// Don't allow clients to invoke the destructor.
	// (Only the CCommsFactoryBase should do this)
	virtual ~CIpSubConnectionProvider();

	void ConstructL();

public:
	// Messages from PRT
	void ProcessPRTMsg(TPfqosMessage& aMsg);
	void ProcessPRTError(TPfqosMessage& aMsg, TInt aError);
#ifdef _DEBUG
	void ProcessPRTError(TInt aMsgType, TInt aError);
#endif

private:
	// Messages to PRT
	void SendOpenExistingL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendCreateL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendCloseL();
	void SendJoinL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendLeaveL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendSetQoSL();

	// Internal Processing
	void ProcessPRTEventL(TPfqosMessage& aMsg);
	void ProcessPRTReplyL(TPfqosMessage& aMsg);

	void ConvertParametersFromESockL(CSubConParameterBundle& aParameterBundle);
	void ConvertParametersFromQOSL(TPfqosMessage& aMsg, CSubConGenEventParamsGranted* aEvent);

	TInt DetermineClient(const TPfqosMessage& aMsg, MSubConnectionDataClient*& aDataClient);
	void MapGenericParamsFromESockToPRTL(const CSubConQosGenericParamSet& generic) const;
	void MapGenericParamsFromPRTToESockL(CSubConQosGenericParamSet& generic) const;
	void MapExtensionParamsFromESockToPRTL(const CSubConQosIPLinkR99ParamSet& extension, TUmtsQoSParameters& params);
	
	void ResetPrtExtensions();
	
	
#ifdef SYMBIAN_NETWORKING_UMTSR5  
	void MapExtensionParamsFromESockToPRTL(const CSubConQosR5ParamSet& aExtension, TUmtsR5QoSParameters& aParams);
	void MapExtensionParamsFromESockToPRTL(const CSubConImsExtParamSet& aExtension, TImsParameter& aParams);
    
    CSubConExtensionParameterSet* MapFromUmtsR5ExtensionL (const CUmtsR5QoSPolicy* aPolicy);
    CSubConExtensionParameterSet* MapFromImsExtensionL (const CImsPolicy* aPolicy);
#endif 
// SYMBIAN_NETWORKING_UMTSR5 


	void MapExtensionParamsFromESockToPRTL(const CSubConSBLPR5ExtensionParamSet& extension, CSblpParameters& params) const;
	
	void ConvertTQoSIntoCQoSParamsL(const TQoSParameters& aParameter);
	void ConvertCQoSIntoTQoSParamsL(TQoSParameters& aParameters) const;

private:
	/** Internal Socket */
	RInternalSocket iSocket;
	
	/** PRT message writer */
	CQoSMsgWriter* iWriter;

	/** PRT message reader */
	CQoSMsgReader* iReader;

	/** QoS channel Id */
	TInt iChannelId;

	/** Process UID */
	TUidType iUid;

	/** QoS Parameters */
    CQoSParameters* iPrtParameters;

	/** QoS Extensions */
    TQoSExtensionQueue iPrtExtensions;

	/** QoS Parameters Set */
	TBool iParametersSet;
	TInt iParameterRelease;
	
	/**Class for doing async writes */
	CAsyncWriter* iAsyncWriter;
	};

#endif  // BASIC_IPSCPR
#endif  // __IPSCPR_H__

