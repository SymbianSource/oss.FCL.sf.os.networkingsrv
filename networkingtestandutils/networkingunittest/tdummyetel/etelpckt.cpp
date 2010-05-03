// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Multimode Packet Data API
// GPRS, CDMAOne, CDMA2000
// 
//

#include "Et_clsvr.h"
#include "ETELEXT.H"

// Packet header files
#include "pcktptr.h"
#include "pcktcs.h"

// Used by ETel to instruct TSY to create a name for the newly opened object
_LIT(KETelNewContextName, "::");

/*GLDEF_C TInt E32Dll(TDllReason)
//
// DLL entry point
//
	{
	return KErrNone;
	}
*/
/***********************************************************************************/
//
// RPacketContext
//
/***********************************************************************************/

EXPORT_C RPacketContext::RPacketContext()
	:iEtelPacketContextPtrHolder(NULL)
	{
	}

EXPORT_C void RPacketContext::ConstructL()
/**
 * This function creates an instance of CPacketContextPtrHolder & assigns the iEtelPacketContextPtrHolder
 * pointer to it. 
 */
	{
	__ASSERT_ALWAYS(iEtelPacketContextPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iEtelPacketContextPtrHolder = CPacketContextPtrHolder::NewL(CEtelPacketPtrHolder::EMaxNumPacketContextPtrSlots);
	}

EXPORT_C void RPacketContext::Destruct()
/**
 * This function deletes & NULLs the iEtelPacketContextPtrHolder pointer.
 */
	{
	delete iEtelPacketContextPtrHolder;
	iEtelPacketContextPtrHolder = NULL;
	}

EXPORT_C TInt RPacketContext::OpenNewContext(RPacketService& aPacketNetwork,TDes& aContextName)
/**
 * This function may be called by the client application.
 * It creates a new 'context' (ie: RPacketContext)
 *
 * @param aPacketNetwork : An RPacketContext may only be opened from an existing RPacketService subsession. A client application must
 * therefore pass a reference to their previously instantiated RPacketService object.
 * 
 * @param aContextName : When the new context is created, the TSY will assign it a unique name & pass this name back to the client
 * using this parameter. The client can use this name to uniquely identify the particular context.
 */
	{
	RSessionBase* session=&aPacketNetwork.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle)); // client has no existing session with ETel

	TRAPD(ret,ConstructL());
	if (ret)
		{
		return ret;
		}
	TInt subSessionHandle=aPacketNetwork.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle)); // client has no existing sub-session!
	TPtrC name(KETelNewContextName); // necessary so that server knows to ask TSY for new name
	TIpcArgs args;
	args.Set(0,&name);
	args.Set(1,&aContextName);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RPacketContext::OpenExistingContext(RPacketService& aPacketNetwork, const TDesC& aContextName)
/**
 * This function may be called by the client application.
 * It opens a handle on an existing RPacketContext object identified by a name. KErrNotFound is returned if the object does 
 * not exist
 *
 * @param aPacketNetwork : This identifies to which RPacketService the particular context to be opened belongs. All existing
 * contexts must have an RPacketService parent.
 *
 * @param aContextName : This uniquely identifies to ETel which existing RPacketContext object the client wants to open. This name was 
 * previously assigned by the TSY when the specified RPacketContext was created using the RPacketContext::OpenNewContext() method.
 */
	{
	RSessionBase* session=&aPacketNetwork.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aPacketNetwork.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aContextName.Length()!=0,PanicClient(KErrBadName));
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,CONST_CAST(TDesC*,&aContextName)));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenByNameFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RPacketContext::Close()
/**
 * This function may be called by the client application.
 * 
 * It closes the client's current sub-session with ETel. Any outstanding requests the client may 
 * have with ETel (notifications for example) will be automatically destroyed by ETel.
 */
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RPacketContext::SetConfig(TRequestStatus& aStatus, const TDesC8& aConfig) const
/**
 * This function may be called by the client application.
 * 
 * It passes to the TSY all parameters necessary to configure the particular context.
 * It is an asynchronous function.
 *
 * @param aStatus : a reference to a TRequestStatus, enabling the client to make this function call
 * asynchronously. 
 *
 * @param aConfig : this is a reference to a TContextConfigGPRS or TContextConfigCDMA class packaged
 * inside a TPckg<> class. The TContextConfigGPRS/CDMA classes contain all the necessary parameters
 * required to configure a context on a GPRS/CDMA network. In the case of this function call, the value
 * for each parameter will be input by the client and probably obtained from CommDB. 
 */
	{
	Set(EPacketContextSetConfig, aStatus, aConfig);
	}

EXPORT_C void RPacketContext::GetConfig(TRequestStatus& aStatus, TDes8& aConfig) const
/**
 * This function may be called by the client application.
 *
 * It requests the configuration of the current context.
 * It is an asynchronous function.
 *
 * @param aStatus : this is a reference to a TRequestStatus.
 *
 * @param aConfig : this is a reference to a TContextConfigGPRS or TContextConfigCDMA class packaged
 * inside a TPckg<> class. The TContextConfigGPRS/CDMA classes contain all the necessary parameters
 * required to configure a context on a GPRS/CDMA network. In the case of this function call, the value
 * for each parameter will be input by the TSY.
 */
	{
	Get(EPacketContextGetConfig, aStatus, aConfig);
	}

EXPORT_C void RPacketContext::NotifyConfigChanged(TRequestStatus& aStatus, TDes8& aConfig) const
/**
 * This function may be called by the client application.
 * 
 * It notifies a client whenever a change in the current context is detected and passes back the
 * updated configuration. It should be noted that once the notification completes, the client must 
 * then re-post the notification if they wish to continue receiving the notification.
 *
 * @param aStatus : this is a reference to a TRequestStatus.
 *
 * @param aConfig : this is a reference to a TContextConfigGPRS or TContextConfigCDMA class packaged
 * inside a TPckg<> class. The TContextConfigGPRS/CDMA classes contain all the necessary parameters
 * required to configure a context on a GPRS/CDMA network. In the case of this function call, the value
 * for each parameter will be input by the TSY.
 */
	{
	Get(EPacketContextNotifyConfigChanged, aStatus, aConfig);
	}

EXPORT_C void RPacketContext::Activate(TRequestStatus& aStatus) const
/**
 * This function may be called by the client application.
 *
 * It activates a context that has been previously configured using the RPacketContext::SetConfig() method.
 * Once this function completes, the context will remain in an 'Activating' state until PPP has been successfully
 * negotiated between the phone & the packet network gateway. Once PPP is negotiated, the context
 * can be considered to be 'Activated'.
 *
 * It is an asynchronous function.
 *
 * @param aStatus : this is a reference to a TRequestStatus.
 */
	{
	Blank(EPacketContextActivate,aStatus);
	}

EXPORT_C void RPacketContext::Deactivate(TRequestStatus& aStatus) const
/**
 * This function may be called by the client application.
 *
 * It deactivates a context previously configured on the phone. 
 *
 * It is an asynchronous function.
 *
 * @param aStatus : this is a reference to a TRequestStatus.
 */
	{
	Blank(EPacketContextDeactivate, aStatus);
	}

EXPORT_C void RPacketContext::Delete(TRequestStatus& aStatus) const
/**
 * This function may be called by the client application.
 *
 * It deletes a context previously configured on the phone. Note that although the context is deleted
 * from the TSY (and/or ME), the client's RPacketContext subsession remains. To remove this, a client
 * must call RPacketContext::Close().
 *
 * It is an asynchronous function.
 *
 * @param aStatus : this is a reference to a TRequestStatus.
 */
	{
	Blank(EPacketContextDelete, aStatus);
	}

EXPORT_C void RPacketContext::LoanCommPort(TRequestStatus& aStatus, RCall::TCommPort& aDataPort) const
/**
 * This function may be called by the client application.
 *
 * It provides a client with details of the comm port to be used for sending/receiving user data. 
 * This comm port is used by Network Interfaces, NIFs, (for example: PPP.NIF) to send & receive data.
 * In a 2-box device this port would generally be the same as that used by the TSY to setup/control the connection.
 * In a 1-box device this port would generally be different to the port used by the TSY to setup/control the connection.
 * It is an asynchronous function.
 *
 * @param aStatus : this is a reference to a TRequestStatus. 
 * @param aDataPort : this is a reference to a TCommPort class containing information on the port, including
 * its name and the associated CSY used to access it.
 */
	{
	__ASSERT_ALWAYS(iEtelPacketContextPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketContextPtrHolder->Set(CEtelPacketPtrHolder::ESlotDataPort,aDataPort);

	Get(EPacketContextLoanCommPort,aStatus,ptr1);
	}

EXPORT_C void RPacketContext::RecoverCommPort(TRequestStatus& aStatus) const
/**
 * This function may be called by the client application.
 * It is an asynchronous function.
 *
 * It allows a client, who previously called RPacketContext::LoanCommPort, to inform ETel & hence the 
 * TSY that it has finished with the comm port.
 *
 * @param aStatus : this is a reference to a TRequestStatus. 
 */
	{
	Blank(EPacketContextRecoverCommPort, aStatus);
	}

EXPORT_C TInt RPacketContext::GetStatus(TContextStatus& aContextStatus) const
/**
 * This function may be called by the client application.
 * It is a synchronous function.
 *
 * It allows a client to get the current status of the context, as defined by the enum TContextStatus.
 *
 * @param aContextStatus : a reference to a enum value defined by TContextStatus.  
 */
	{
	TPckg<TContextStatus> ptr1(aContextStatus);
	return Get(EPacketContextGetStatus, ptr1);
	}

EXPORT_C void RPacketContext::NotifyStatusChange(TRequestStatus& aStatus,TContextStatus& aContextStatus)
/**
 * This function may be called by the client application.
 * It is an asynchronous function.
 *
 * It allows a client to receive a notification whenever a change in the status of the context is detected.
 * The new status is also passed back to the client. It should be noted that once the notification completes, 
 * the client must then re-post the notification if they wish to continue receiving further status change notifications.
 * 
 * @param aStatus : this is reference to a TRequestStatus.
 * @param aContextStatus : this is a reference to the new enum value defined by TContextStatus.
 */
	{
	__ASSERT_ALWAYS(iEtelPacketContextPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketContextPtrHolder->Set(CEtelPacketPtrHolder::ESlotContextStatus,aContextStatus);

	Get(EPacketContextNotifyStatusChange,aStatus,ptr1);
	}	

EXPORT_C TInt RPacketContext::GetProfileName(TName& aQoSProfile) const
/**
 * This function may be called by the client application.
 * It is an asynchronous function.
 *
 * It allows a client to retrieve the name of the QoS profile (RPacketQoS) associated with the
 * context (RPacketContext). If no QoS profile has been defined by the client, the name returned 
 * will be be NULL
 *
 * @param aQoSProfile : this is a reference to the name of the existing qos profile. If no profile
 * has been defined then this name will be NULL.
 */
	{
	TPckg<TName> ptr1(aQoSProfile);
	return Get(EPacketContextGetProfileName,ptr1);
	}
EXPORT_C TInt RPacketContext::GetDataVolumeTransferred(TDataVolume& aVolume) const
/** 
 * This function may be called by the client application.
 * It is a synchronous function.
 *
 * It allows a client to get details on the current amount of data transmitted & received over the airlink
 * since the context was activated.
 * 
 * @param aVolume : this is a reference to TDataVolume. 
 */
	{
	TPckg<TDataVolume> ptr1(aVolume);
	return Get(EPacketContextGetDataVolumeTransferred, ptr1);
	}

EXPORT_C void RPacketContext::NotifyDataTransferred(TRequestStatus& aStatus, TDataVolume& aVolume, TUint aRcvdGranularity,TUint aSentGranularity) const
/** 
 * This function may be called by the client application.
 * It is an asynchronous function.
 *
 * It allows a client to be notified of a change in the volume of data transmitted/received over the airlink since the 
 * context was activated. The new volume is also passed back to the client. It should be noted that once the 
 * notification completes, the client must then re-post the notification if they wish to continue 
 * receiving further notifications.
 *
 * The notification will only complete whenever the change in volume increases by a client-specified amount (granularity).
 * The default granularity for each direction (rx/tx) is 4KB. 
 * 
 * @param aStatus : this is a reference to TRequestStatus.
 * @param aVolume : this is a reference to TDataVolume. 
 * @param aRcvdGranularity : this is a reference to a TUint value defining the granularity for the volume of received data
 * @param aSentGranularity : this is a reference to a TUint value defining the granularity for the volume of transmitted data
 */
	{
	__ASSERT_ALWAYS(iEtelPacketContextPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iEtelPacketContextPtrHolder->iNotifyDataTransferRequest.iRcvdGranularity=aRcvdGranularity;
	iEtelPacketContextPtrHolder->iNotifyDataTransferRequest.iSentGranularity=aSentGranularity;

	TPtr8& ptr1=iEtelPacketContextPtrHolder->Set(CEtelPacketPtrHolder::ESlotDataVolume,aVolume);
	TPtr8& ptr2=iEtelPacketContextPtrHolder->Set(CEtelPacketPtrHolder::ESlotGranularity,iEtelPacketContextPtrHolder->iNotifyDataTransferRequest);

	Get(EPacketContextNotifyDataTransferred, aStatus, ptr1, ptr2);
	}

EXPORT_C void RPacketContext::GetConnectionSpeed(TRequestStatus& aStatus, TUint& aRate) const
/**
 * This function may be called by the client application.
 * It is a synchronous function.
 *
 * Allows a client to get the current connection speed. 
 *
 * @param aRate : reference to a TUint value that is the current connection bandwidth in bits per second.
 */
	{
	__ASSERT_ALWAYS(iEtelPacketContextPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	TPtr8& ptr1 = iEtelPacketContextPtrHolder->Set(CEtelPacketPtrHolder::ESlotGetSpeed, aRate);
	Get(EPacketContextGetConnectionSpeed, aStatus, ptr1);
	}

EXPORT_C void RPacketContext::NotifyConnectionSpeedChange(TRequestStatus& aStatus, TUint& aRate) const
/**
 * This function may be called by the client application.
 * It is an asynchronous function.
 *
 * Allows a client to be notified whenever the current connection speed available changes. The new value of connection 
 * speed availability is also passed back. 
 *
 * @param aStatus : reference to a TReqStatus.
 * @param aRate : reference to a TUint value that is the current connection bandwidth in bits per second.
 */
	{
	__ASSERT_ALWAYS(iEtelPacketContextPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketContextPtrHolder->Set(CEtelPacketPtrHolder::ESlotNotifySpeed, aRate);

	Get(EPacketContextNotifyConnectionSpeedChange, aStatus, ptr1);
	}

EXPORT_C TInt RPacketContext::GetLastErrorCause(TInt& aError) const
/**
 * This function may be called by the client.
 *
 * This function will retrieve the most recent error to have occurred, passed back in the parameter aError. 
 * This may be used, for instance, to retrieve which error caused PPP to close down prematurely. 
 *
 * @param aError : this is the last error received/generated by the TSY
 */
	{
	TPckg<TInt> ptr1(aError);
	return Get(EPacketContextGetLastErrorCause, ptr1);
	}

EXPORT_C RPacketContext::TContextConfigGPRS::TContextConfigGPRS()
: TPacketDataConfigBase()
/**
 * Constructor for TContextConfigGPRS class
 *
 * This class contains member data compatible with parameters required for configuring a context
 * on the GPRS packet network. 
 *
 * The iExtensionId parameter, set automatically by ETel, is used by the TSY when unpacking this class from 
 * a TPckg<> to indicate it is to be unpacked as a TContextConfigGPRS class.
 *
 */
	{
	iExtensionId = KConfigGPRS;
	
	iAccessPointName.Zero();
	iAnonymousAccessReqd = ENotRequired;
	iPdpAddress.Zero();
	iPdpCompression = 0;
	iPdpType = EPdpTypeIPv4;
	};

/***********************************************************************************/
//
// RPacketService
//
/***********************************************************************************/

EXPORT_C RPacketService::RPacketService()
	:iEtelPacketPtrHolder(NULL) 
/**
 * Constructor for RPacketService class
 */
	{
	}

EXPORT_C void RPacketService::ConstructL()
/**
 * ConstructL for the RPacketService class
 *
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iEtelPacketPtrHolder = CPacketPtrHolder::NewL(CEtelPacketPtrHolder::EMaxNumPacketPtrSlots);
	}

EXPORT_C void RPacketService::Destruct()
/**
 * Destruct() for the RPacketService class
 *
 */
	{
	delete iEtelPacketPtrHolder;
	iEtelPacketPtrHolder = NULL;
	}	

EXPORT_C TInt RPacketService::Open(RPhone& aPhone)
/**
 * This function may be called by the client.
 *
 * It allows a client to create an RPacketService subsession from an existing RPhone session
 *
 * @param aPhone : a reference to an existing RPhone session the client has previously created.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=NULL,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;

	TIpcArgs args;
	args.Set(0,&KPacketName);
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RPacketService::Close()
/**
 * This function may be called by the client.
 *
 * It allows a client to close its RPacketService subsession
 */
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RPacketService::NotifyContextAdded(TRequestStatus& aStatus, TDes& aContextId) const
/**
 * This function may be called by the client.
 *
 * It allows a client to be notified whenever a new context (RPacketContext) is configured on the phone,
 * by a client using RPacketContext::SetConfig(). It should be noted that once the notification completes, 
 * the client must then re-post the notification if they wish to continue receiving further notifications.
 *
 * @param aStatus : this is a reference to a TRequestStatus
 * @param aContextId : this is the name of the newly added context. The name is assigned by the TSY &
 * uniquely identifies the particular context.
 */
	{
	Get(EPacketNotifyContextAdded,aStatus,aContextId);
	}

EXPORT_C void RPacketService::Attach(TRequestStatus& aStatus) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * This function will cause the phone to attempt an attach to the packet network, if it is not already attached. 
 * If it is, the error KErrAlreadyExists should be returned. If the phone cannot attach to the network for some reason, 
 * the error KErrPacketNetworkFailure should be returned.
 *
 * This may not often need to be used, as there is a separate setting to tell the phone either to attach at start-up, or 
 * to attach when a context is created. It is intended that this function only be utilised if the user has an option 
 * to choose exactly when to attach and detach.
 *
 * This function is asynchronous since attachment may take anything up to one minute in the worst case scenario.
 *
 * @param aStatus : this is a reference to a TRequestStatus
 */
	{
	Blank(EPacketAttach,aStatus);
	}

EXPORT_C void RPacketService::Detach(TRequestStatus& aStatus) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 * 
 * This function will cause the phone to attempt a detach from the packet network, if it is not already detached. 
 * If it is, the error KErrAlreadyExists should be returned. If at least one context is active the error KErrInUse 
 * should be returned.
 * 
 * @param aStatus : this is a reference to a TRequestStatus
 */
	{
	Blank(EPacketDetach, aStatus);
	}

EXPORT_C TInt RPacketService::GetStatus(TStatus& aPacketStatus) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function that returns the status of the current packet service as defined
 * by the enum TStatus.
 *
 * These states include : 
 * EStatusUnattached	- Not attached to the packet network
 * EStatusAttached		- Attached to the packet network but without an active context
 * EStatusActive		- Attached to the packet network with an active context
 * EStatusSuspended		- Attached to the packet network but with a suspended context
 *
 * @param aPacketStatus : parameter whose value is one of the above
 */
	{
	TPckg<TStatus> ptr1(aPacketStatus);
	return Get(EPacketGetStatus, ptr1);
	}

EXPORT_C void RPacketService::NotifyStatusChange(TRequestStatus& aStatus,TStatus& aPacketStatus) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * It allows a client to be notified of a change in the status of the connection to the packet service.
 * The new status is passed back to the client via aPacketStatus parameter.
 * 
 * These states include : 
 * EStatusUnattached - Not attached to the packet network
 * EStatusAttached - Attached to the packet network but without an active context
 * EStatusActive - Attached to the packet network with an active context
 * EStatusSuspended - Attached to the packet network but with a suspended context
 *
 * @param aStatus : reference to a TRequestStatus
 * @param aPacketStatus : parameter whose value is one of the above
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotPacketStatus,aPacketStatus);

	Get(EPacketNotifyStatusChange,aStatus,ptr1);
	}

EXPORT_C void RPacketService::NotifyContextActivationRequested(TRequestStatus& aStatus, TDes8& aContextParameters) const
/**
 * This function may be called by the client.
 * 
 * This function will complete when the phone receives a request from the packet network to activate a PDP context. 
 * Depending on the type of network, this request may contain the PDP type requested, the PDP address for this context,
 * and possibly the Access Point Name (APN) of the gateway to connect to.
 * 
 * @param aStatus : a reference to a TRequestStatus
 * @param aContextParameters : the relevant parameters as defined above, contained inside the approporiate TContextConfigGPRS or 
 * TContextConfigCDMA context config class, packaged inside a TPckg<> and passed back to the client as a descriptor reference.
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	Get(EPacketNotifyContextActivationRequested, aStatus, aContextParameters);
	}

EXPORT_C void RPacketService::RejectActivationRequest(TRequestStatus& aStatus) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * This function will cause the phone to reject the network's request for a PDP context activation.
 * Note, there is no corresponding "accept" message for the phone to send. 
 * To accept, the phone should simply begin to activate a PDP context with the appropriate IP address 
 * before the network's timer expires. On GPRS, this timer is between 5 and 8 seconds, but may 
 * effectively be between around 25 and 40 seconds as the specifications state that the activation 
 * request should be re-sent up to five times.
 *
 * @param aStatus : a reference to a TRequestStatus
 */
	{
	Blank(EPacketRejectActivationRequest, aStatus);
	}

EXPORT_C void RPacketService::EnumerateContexts(TRequestStatus& aStatus, TInt& aCount, TInt& aMaxAllowed) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * This function will retrieve both the number of opened contexts in the TSY, the aCount 
 * parameter containing this value on completion of the function, and the maximum possible number of 
 * contexts, passed back in the aMaxAllowed parameter. Note that this is not the maximum number of 
 * simultaneously active contexts, but the total number allowed. (The maximum possible number of simultaneously
 * active contexts is currently limited to 1) 
 * 
 * Clients are thus able to determine whether they can open a new context or not. If the actual value of 
 * aMaxAllowed is not available to the TSY its value should be set to -1.
 *
 * @param aStatus : a reference to a TRequestStatus
 * @param aCount: the number of opened contexts existing in the TSY.
 * @param aMaxAllowed : the maximum number of opened context that are allowed in the TSY.
 */
	{
	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotEnumerateCount,aCount);
	TPtr8& ptr2=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotEnumerateMaxCount,aMaxAllowed);
	Get(EPacketEnumerateContexts, aStatus, ptr1, ptr2);
	}

EXPORT_C void RPacketService::GetContextInfo(TRequestStatus& aStatus,TInt aIndex,TContextInfo& aInfo) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * This function will retrieve information about a particular context, specified by the aIndex parameter. 
 * It is usual to use this function in conjunction with the EnumerateContexts() function, which returns 
 * the current number of opened contexts in 'aCount'. 
 *
 * @param aStatus : a reference to a TRequestStatus
 *
 * @param aIndex : The valid range for aIndex will be between 0 and (n-1) where n is the number 
 * returned in aCount.  If an invalid index is specified the function will return KErrArgument.
 *
 * @param aInfo : This contains the context information inside the TContextInfo struct. This struct contains
 * member data to identify the name of the context (TName iName) & the current status of the context
 * (RPacketContext::TContextStatus iStatus)
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iEtelPacketPtrHolder->iGetContextInfoIndex = aIndex;
	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotContextInfoIndex,iEtelPacketPtrHolder->iGetContextInfoIndex);
	TPtr8& ptr2=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotContextInfo,aInfo);

	Get(EPacketGetContextInfo,aStatus, ptr1, ptr2);
	}

EXPORT_C void RPacketService::GetNtwkRegStatus(TRequestStatus& aStatus, TRegistrationStatus& aRegistrationStatus) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * This function allows the client to retrieve the current registration status of the phone on the packet network as
 * defined by the enum TRegistrationStatus.
 * 
 * @param aStatus : a reference to a TRequestStatus
 *
 * @param aRegistrationStatus : this is the current registration status of the phone on the packet network.
 * It can be one of the following values
 * ENotRegisteredNotSearching - The phone is not registered & not searching for a network. A network may or may not be available.
 * ERegisteredOnHomeNetwork - The phone is registered on the home network.
 * ENotRegisteredSearching - The phone is not registered but is searching for a network.
 * ERegistrationDenied - The phone has been denied registration to the network.
 * EUnknown - The registration status is unknown.
 * ERegisteredRoaming - The phone is registered on a network which is not the home network.
 * ENotRegisteredButAvailable - The phone is not registered and not searching for a network. However a network is available.
 * ENotRegisteredAndNotAvailable - The phone is not registered and not searching for a network. There is no available network.
 *
 */
	{
	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotGetNtwkReg,aRegistrationStatus);
	Get(EPacketGetNtwkRegStatus, aStatus, ptr1);
	}

EXPORT_C void RPacketService::NotifyChangeOfNtwkRegStatus(TRequestStatus& aStatus,TRegistrationStatus& aRegistrationStatus) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * Allows a client to be notified whenever a change in the status of the packet network registration is detected.
 *
 * @param aStatus : reference to a TRequestStatus
 * @param aRegistrationStatus : new value of the packet network registration status.
 * It can be one of the following values
 * ENotRegisteredNotSearching - The phone is not registered & not searching for a network. A network may or may not be available.
 * ERegisteredOnHomeNetwork - The phone is registered on the home network.
 * ENotRegisteredSearching - The phone is not registered but is searching for a network.
 * ERegistrationDenied - The phone has been denied registration to the network.
 * EUnknown - The registration status is unknown.
 * ERegisteredRoaming - The phone is registered on a network which is not the home network.
 * ENotRegisteredButAvailable - The phone is not registered and not searching for a network. However a network is available.
 * ENotRegisteredAndNotAvailable - The phone is not registered and not searching for a network. There is no available network.
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotRegStatus,aRegistrationStatus);

	Get(EPacketNotifyChangeOfNtwkRegStatus,aStatus,ptr1);
	}

EXPORT_C void RPacketService::GetMSClass(TRequestStatus& aStatus, TMSClass& aCurrentClass, TMSClass& aMaxClass) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * This function will retrieve both the current class and the highest class of the mobile station. 
 * The parameters aCurrentClass and aMaxClass may take one of the following values which are ordered in 
 * terms of class from highest to lowest:
 *	EMSClassDualMode :	Allows simultaneous attach on packet network & simultaneous active calls/contexts. This is the highest class possible.
 *	EMSClassSuspensionRequired : Allows simultaneous attach on packet network but not simultaneous traffic. An active GPRS/CDMA context can be suspended to allow a circuit-switched call.
 *	EMSClassAlternateMode :	Does not allow simultaneous attach, but may swap betwen packet and circuit switched.
 *	EMSClassPacketSwitchedOnly	: This limits the device to packet switched functionality only.
 *	EMSClassCircuitSwitchedOnly	: This limits the device to circuit Switched functionality only. This is the lowest class possible.
 *	EMSClassUnknown	: The class of the mobile station is unknown.
 *
 * @param aCurrentClass : current class of the phone. 
 * @param aMaxClass : highest possible class the phone can emulate. 
 *
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotCurrentMsClass,aCurrentClass);
	TPtr8& ptr2=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotMaxMsClass,aMaxClass);

	Get(EPacketGetMSClass, aStatus, ptr1, ptr2);
	}

EXPORT_C void RPacketService::SetMSClass(TRequestStatus& aStatus, TMSClass aClass) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * This function will set the current packet class of the mobile station; this should never be higher than the maximum
 * packet class of the mobile station. If the requested class is higher than the maximum class supported by the phone, 
 * the error code KErrTooBig should be returned. The parameter aClass may take one of the values defined in Table 3-5.
 * 
 * It is expected that this function will be used to force a ClassSuspensionRequired mobile into a Class C mode of 
 * operation, either packet or circuit-switched only. This is useful for instance in a situation where the user is billed 
 * for incoming circuit-switched calls (a practice of U.S. network operators) and does not wish to be available for 
 * incoming calls for long periods of time but does want to keep the packet side of the phone always on and always 
 * connected.
 *
 * @param aClass : the class to which the client wishes to set the phone
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iEtelPacketPtrHolder->iMSClass = aClass;
	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotSetMsClass,iEtelPacketPtrHolder->iMSClass);

	Set(EPacketSetMSClass,aStatus, ptr1);
	}

EXPORT_C void RPacketService::NotifyMSClassChange(TRequestStatus& aStatus, TMSClass& aNewClass) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * 
 *
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotNtfMsClass,aNewClass);

	Get(EPacketNotifyMSClassChange,aStatus,ptr1);
	}

EXPORT_C TInt RPacketService::GetStaticCaps(TUint& aCaps, RPacketContext::TProtocolType aPdpType) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * 
 *
 */
	{
	TPckg<TUint> ptr1(aCaps);
	TPckg<RPacketContext::TProtocolType> ptr2(aPdpType);
	return Get(EPacketGetStaticCaps, ptr1, ptr2);
	}

EXPORT_C TInt RPacketService::GetDynamicCaps(TDynamicCapsFlags& aCaps) const
	{
	TPckg<TDynamicCapsFlags> ptr1(aCaps);
	return Get(EPacketGetDynamicCaps, ptr1);
	}

EXPORT_C void RPacketService::NotifyDynamicCapsChange(TRequestStatus& aStatus, TDynamicCapsFlags& aCaps) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * 
 *
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotDynamicCaps,aCaps);

	Get(EPacketNotifyDynamicCapsChange,aStatus,ptr1);
	}

EXPORT_C void RPacketService::SetPreferredBearer(TRequestStatus& aStatus, TPreferredBearer aBearer) const
/**
 * This function may be called by the client.
 *
 * It is an asynchronous function.
 *
 * 
 *
 */
	{
	__ASSERT_ALWAYS(iEtelPacketPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iEtelPacketPtrHolder->iPrefBearer = aBearer;
	TPtr8& ptr1=iEtelPacketPtrHolder->Set(CEtelPacketPtrHolder::ESlotSetPrefBearer,iEtelPacketPtrHolder->iPrefBearer);

	Set(EPacketSetPrefBearer,aStatus, ptr1);
	}


EXPORT_C TInt RPacketService::GetPreferredBearer(TPreferredBearer& aBearer) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * 
 *
 */
	{
	TPckg<TPreferredBearer> ptr1(aBearer);
	return Get(EPacketGetPrefBearer, ptr1);
	}


EXPORT_C TInt RPacketService::SetAttachMode(TAttachMode aMode) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * 
 *
 */
	{
	TPckg<TAttachMode> ptr1(aMode);
	return Set(EPacketSetAttachMode, ptr1);
	}

EXPORT_C TInt RPacketService::GetAttachMode(TAttachMode& aMode) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * 
 *
 */
	{
	TPckg<TAttachMode> ptr1(aMode);
	return Get(EPacketGetAttachMode, ptr1);
	}

EXPORT_C TInt RPacketService::SetDefaultContextParams(const TDesC8& aPckg) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * 
 *
 */
	{
	return Set(EPacketSetDefaultContextParams, aPckg);
	}

EXPORT_C TInt RPacketService::GetDefaultContextParams(TDes8& aPckg) const
/**
 * This function may be called by the client.
 *
 * It is a synchronous function.
 *
 * 
 *
 */
	{
	return Get(EPacketGetDefaultContextParams, aPckg);
	}


TPacketBase::TPacketBase()
	{}

EXPORT_C TInt TPacketBase::ExtensionId() const
/**
	Returns the value of iExtensionId for the associated class.
	e.g. iExtensionId=KETelExtPcktV2 for all the V2 classes.

	@return The protected data member iExtensionId, which specifies the API version number.
*/
	{
	return iExtensionId;
	}

	
	
EXPORT_C RPacketContext::TProtocolConfigOptionV2::TProtocolConfigOptionV2()
: TPacketBase(), iChallenge(0), iResponse(0), iId(0), iDnsAddresses(),
  iMiscBuffer(0)
/**
	Constructor for TProtocolConfigOptionV2 class

	This class will hold authentication data encapsulated in TAuthInfo as well as further data that 
	may be required for CHAP protocol authentication, such a challenge and response
*/
	{
	iExtensionId = KETelExtPcktV2;

	//
	// Initialise the TAuthInfo structure...
	//
	iAuthInfo.iProtocol = EProtocolNone;
	iAuthInfo.iUsername.Zero();
	iAuthInfo.iPassword.Zero();
	};
	
	
EXPORT_C RPacketContext::TDnsInfoV2::TDnsInfoV2()
: TPacketBase(), iPrimaryDns(0), iSecondaryDns(0)
/**
	Standard constructor since version 2 of the API.
	
	@see KETelExtPcktV2
*/
	{
	iExtensionId = KETelExtPcktV2;
	}
