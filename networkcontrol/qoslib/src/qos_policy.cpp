// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qoslib.h"
#include "qoslib_glob.h"
#include "pfqos_stream.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

/**
Default constructor. 

RQoSPolicy::Open() must always be called before any other 
methods in the RQoSPolicy can be called. 

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
*/
EXPORT_C RQoSPolicy::RQoSPolicy() : iPolicy(NULL)
	{
	}

/**
Destructor. 
 
Closes any open policy. 

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
*/
EXPORT_C RQoSPolicy::~RQoSPolicy()
	{
	if (iPolicy)
		iPolicy->Close();
	}

/**
This must always be called before any other function can be used. 
 
It specifies the selector for a QoS policy. 
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aSelector Selector for the QoS policy.
@return KErrNone, if successful; otherwise, an error code if the QoS channel 
cannot be opened. 
*/
EXPORT_C TInt RQoSPolicy::Open(const TQoSSelector& aSelector)
	{
	if (iPolicy)
		return KErrAlreadyExists;
	CQoSMan* manager = NULL;
	TRAPD(err,
		manager = CQoSMan::NewL();
		// There is manager->Open() in above NewL.
		// How and when is that cancelled, if leave happens below? --msa
		iPolicy = manager->OpenQoSPolicyL(aSelector)
		);
	return err;
	}

/**
Sets the QoS parameters for the QoS policy. 
 
A CQoSAddEvent event is received asynchronously to indicate the success or 
failure of the request. 
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aPolicy QoS parameters.
@return KErrNone, if successful; otherwise, an error code. 
*/
EXPORT_C TInt RQoSPolicy::SetQoS(CQoSParameters& aPolicy)
	{
	if (!iPolicy)
		return KErrNotReady;
	TRAPD(err, iPolicy->SetQoSL(aPolicy));
	return err;
	}

/**
Gets the QoS policy from QoS policy database. 

A CQoSGetEvent event is received asynchronously to indicate the success or 
failure of the request.
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return KErrNone, if successful; otherwise, an error code. 
*/
EXPORT_C TInt RQoSPolicy::GetQoS()
	{
	if (!iPolicy)
		return KErrNotReady;
	TRAPD(err, iPolicy->GetQoSL());
	return err;
	}

/**
Deletes the QoS policy.
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return KErrNone, if successful; otherwise, an error code (e.g. if 
RQoSPolicy::Open() was not called).
*/
EXPORT_C TInt RQoSPolicy::Close()
	{
	if (!iPolicy)
		return KErrNotReady;
	TRAPD(err, iPolicy->DeleteL());
	if (err == KErrNone)
		{
		iPolicy->Close();
		iPolicy = NULL;
		}
	return err;
	}

/**
Registers an event observer to catch QoS events.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aObserver Event observer.
@param aMask An event mask. An application can specify a set of QoS events 
that it wants to receive. By default all events are notified to the 
application. See TQoSEvent enumerations.
@return KErrNone, if successful; otherwise, an error code. 
*/
EXPORT_C TInt RQoSPolicy::NotifyEvent(MQoSObserver& aObserver, TUint aMask)
	{
	if (!iPolicy)
		return KErrNotReady;
	return iPolicy->NotifyEvent(aObserver, aMask);
	}

/**
Deregisters an event observer to catch QoS events. 
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aObserver Event observer.
@return KErrNone if successful, otherwise an error code. 
*/
EXPORT_C TInt RQoSPolicy::CancelNotifyEvent(MQoSObserver& aObserver)
	{
	if (!iPolicy)
		return KErrNotReady;
	return iPolicy->CancelNotifyEvent(aObserver);
	}

/**
Loads a QoS policy file into the QoS policy database. 
 
A TQoSEvent event (EQoSEventLoadPolicyFile) is received asynchronously to 
indicate the success or failure of the request. 
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aName Name of the QoS policy file to be loaded.
@return KErrNone, if successful; otherwise, an error code. 
*/
EXPORT_C TInt RQoSPolicy::LoadPolicyFile(const TDesC& aName)
	{
	if (!iPolicy)
		return KErrNotReady;
	TRAPD(err, iPolicy->LoadFileL(aName));
	return err;
	}

/**
Unloads a QoS policy file from the QoS policy database. 
 
A TQoSEvent event (EQoSEventUnloadPolicyFile) is received asynchronously to 
indicate the success or failure of the request. 
 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aName Name of the QoS policy file to be unloaded.
@return KErrNone, if successful; otherwise, an error code. 
*/
EXPORT_C TInt RQoSPolicy::UnloadPolicyFile(const TDesC& aName)
	{
	if (!iPolicy)
		return KErrNotReady;
	TRAPD(err, iPolicy->UnloadFileL(aName));
	return err;
	}


//
CQoSRequestBase::~CQoSRequestBase()
	{
	iManager->ClearPendingRequest(this);
	iManager->Close();
	}

void CQoSRequestBase::ParseExtensions(TPfqosMessage& aMsg, CQoSParameters& aPolicy)
	{
	aMsg.SetQoSParameters(aPolicy.iQoS);
	TSglQueIter<CPfqosPolicyData> iter(aMsg.iExtensions);
	CPfqosPolicyData *data;

	while ((data = iter++) != NULL)
		{
		TInt extensionType;
		TInt ret = GetExtensionType(data->Data(), extensionType);
		if (ret == KErrNone)
			{
			CExtensionBase *ext = aPolicy.FindExtension(extensionType);
			if (ext)
				ext->ParseMessage(data->Data());
			}
		}
	}

TInt CQoSRequestBase::GetExtensionType(const TDesC8& aData, TInt& aType)
	{
	const TUint8 *p = aData.Ptr();
	TInt length = aData.Length();
	//lint -e{826} thinks this cast is suspicious -- is not!
	struct pfqos_configure *ext = (struct pfqos_configure *) p;

	if (length < (TInt)sizeof(pfqos_configure))
		return KErrGeneral;	 // EMSGSIZE (impossible message size)

	if (ext->pfqos_configure_len * 8 != length)
		return KErrGeneral;	 // EMSGSIZE (incorrect message length)

	if (ext->pfqos_ext_type != EPfqosExtExtension)
		return KErrGeneral;

	p += sizeof(struct pfqos_configure);
	//lint -e{826} thinks this cast is suspicious -- is not!
	pfqos_extension *extensionType = (pfqos_extension *) p;
	aType = extensionType->pfqos_extension_type;
	return KErrNone;
	}

TInt CQoSRequestBase::NotifyEvent(MQoSObserver& aObserver, TUint aMask)
	{
	if (aMask & ~KQoSEventAll)
		return KErrNotSupported;
	iEventMask = aMask;
	iObserver = &aObserver;
	return KErrNone;
	}

TInt CQoSRequestBase::CancelNotifyEvent(MQoSObserver& aObserver)
	{
	if (iObserver != &aObserver)
		return KErrNotFound;
	iObserver = NULL;
	return KErrNone;
	}


//
CPolicy* CPolicy::NewL(CQoSMan* aManager, const TQoSSelector& aSelector)
	{
	CPolicy* policy = new (ELeave) CPolicy(aManager, aSelector);
	return policy;
	}

CPolicy::CPolicy(CQoSMan* aManager, const TQoSSelector& aSelector)
	{
	iManager = aManager;
	iSelector = aSelector;
	iPending = ENone;
	iPolicyCreated = EFalse;
	}

CPolicy::~CPolicy()
	{
	iManager->RemoveQoSPolicy(this);
	}

void CPolicy::Close()
	{
	delete this;
	}

TBool CPolicy::Match(const TQoSSelector& aSelector)
	{
	return (iSelector == aSelector);
	}

void CPolicy::SetQoSL(CQoSParameters& aPolicy)
	{
	if (iPending != ENone)
		User::Leave(KErrInUse);
	iPolicy.CopyL(aPolicy);
	CRequest* request = CRequest::NewL(this, KQoSDefaultBufSize);
	if (iPolicyCreated)
		{
		request->iMsg->Init(EPfqosUpdate);
		iPending = EPendingUpdate;
		}
	else
		{
		request->iMsg->Init(EPfqosAdd);
		iPending = EPendingAdd;
		}
	request->iMsg->AddSelector((TUint8)iSelector.Protocol(), 
							   iManager->Uid().UidType(), 
							   EPfqosFlowspecPolicy, 
							   iSelector.IapId(), 
							   EPfqosApplicationPriority, 
							   TPtr(0,0));
	request->iMsg->AddSrcAddress(iSelector.GetSrc(), 
								 iSelector.GetSrcMask(), 
								 (TUint16)iSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(iSelector.GetDst(), 
								 iSelector.GetDstMask(), 
								 (TUint16)iSelector.MaxPortDst()); 
	request->iMsg->AddQoSParameters(iPolicy.iQoS);
	
	TQoSExtensionQueueIter iter(iPolicy.Extensions());
	CExtensionBase *extension;
	while ((extension=iter++) != NULL)
		request->iMsg->AddExtensionPolicy(extension->Data());
	iManager->Send(request);

	}

void CPolicy::GetQoSL()
	{

	if (iPending != ENone)
		User::Leave(KErrInUse);
	CRequest* request = CRequest::NewL(this, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosGet);
	iPending = EPendingGet;
	request->iMsg->AddSelector((TUint8)iSelector.Protocol(), 
							   iManager->Uid().UidType(), 
							   EPfqosFlowspecPolicy, 
							   iSelector.IapId(), 
							   EPfqosApplicationPriority, 
							   TPtr(0,0));
	request->iMsg->AddSrcAddress(iSelector.GetSrc(), 
								 iSelector.GetSrcMask(), 
								 (TUint16)iSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(iSelector.GetDst(), 
								 iSelector.GetDstMask(), 
								 (TUint16)iSelector.MaxPortDst()); 
	
	TQoSExtensionQueueIter iter(iPolicy.Extensions());
	CExtensionBase *extension;
	while ((extension=iter++) != NULL)
		request->iMsg->AddExtensionPolicy(extension->Data());
	iManager->Send(request);

	}

void CPolicy::DeleteL()
	{
	CRequest* request = CRequest::NewL(this,KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosDelete);
	iPending = EPendingDelete;
	request->iMsg->AddSelector((TUint8)iSelector.Protocol(), 
							   iManager->Uid().UidType(), 
							   EPfqosFlowspecPolicy, 
							   iSelector.IapId(), 
							   EPfqosApplicationPriority, 
							   TPtr(0,0));
	request->iMsg->AddSrcAddress(iSelector.GetSrc(), 
								 iSelector.GetSrcMask(), 
								 (TUint16)iSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(iSelector.GetDst(), 
								 iSelector.GetDstMask(), 
								 (TUint16)iSelector.MaxPortDst()); 
	
	TQoSExtensionQueueIter iter(iPolicy.Extensions());
	CExtensionBase *item;
	while ((item = iter++) != NULL)
		request->iMsg->AddExtensionHeader((TUint16)item->Type());

	iManager->Send(request);
	}

void CPolicy::ProcessReply(TPfqosMessage& aMsg)
	{
	TInt aErrorCode = aMsg.iBase.iMsg->pfqos_msg_errno;



	TPendingStatus status = iPending;

	if (aErrorCode)
		{
		if (status == EPendingLoadFile || status == EPendingUnloadFile)
			{
			iPending = ENone;
#if _UNICODE
			TPtrC8 tmp((TUint8*)aMsg.iConfigFile.iExt->filename);
			TFileName filename;
			filename.Copy(tmp);
#else
			TPtrC8 filename((TUint8*)aMsg.iConfigFile.iExt->filename);
#endif
			TQoSEvent event_type;
			if (status == EPendingLoadFile)
				event_type = EQoSEventLoadPolicyFile;
			else
				event_type = EQoSEventUnloadPolicyFile;
			CQoSLoadEvent event(event_type, aErrorCode, filename);
			if (iObserver && iEventMask & EQoSEventLoadPolicyFile)
				iObserver->Event(event);
			}
		else
			{
			NotifyError(aErrorCode);
			}
		}
	else
		{
		iPending = ENone;

		switch (status)
			{
		case EPendingAdd:
			{
			CQoSParameters policy;
			ParseExtensions(aMsg, policy);
			CQoSAddEvent event(&policy, aErrorCode);
			if (iObserver && iEventMask & EQoSEventAddPolicy)
				iObserver->Event(event);
			iPolicyCreated = ETrue;
			}
			break;

		case EPendingUpdate:
			{
			CQoSParameters policy;
			ParseExtensions(aMsg, policy);
			CQoSAddEvent event(&policy, aErrorCode);
			if (iObserver && iEventMask & EQoSEventAddPolicy)
				iObserver->Event(event);
			}
			break;

		case EPendingDelete:
			{
			iPending = ENone;
			CQoSDeleteEvent event(aErrorCode);
			if (iObserver && iEventMask & EQoSEventDeletePolicy)
				iObserver->Event(event);
			}
			break;

		case EPendingGet:
			{
			CQoSParameters policy;


			ParseExtensions(aMsg, policy);


			CQoSGetEvent event(&policy, aErrorCode);


			if (iObserver && iEventMask & EQoSEventGetPolicy)
				iObserver->Event(event);
			}
			break;

		case EPendingLoadFile:
		case EPendingUnloadFile:
			{
#if _UNICODE
			TPtrC8 tmp((TUint8*)aMsg.iConfigFile.iExt->filename);
			TFileName filename;
			filename.Copy(tmp);
#else
			TPtrC8 filename((TUint8*)aMsg.iConfigFile.iExt->filename);
#endif
			TQoSEvent event_type;
			if (status == EPendingLoadFile)
				event_type = EQoSEventLoadPolicyFile;
			else
				event_type = EQoSEventUnloadPolicyFile;
			CQoSLoadEvent event(event_type, KErrNone, filename);
			if (iObserver && iEventMask & EQoSEventLoadPolicyFile)
				iObserver->Event(event);
			}
			break;

		default:
			break;
			}
		}
	}

TBool CPolicy::MatchReply(const TPfqosMessage& aMsg, TUint8 aMsgType)
	{
	if (((iPending == EPendingAdd && aMsgType == EPfqosAdd) ||
	 (iPending == EPendingUpdate && aMsgType == EPfqosUpdate) ||
	 (iPending == EPendingGet && aMsgType == EPfqosGet) ||
	 (iPending == EPendingDelete && aMsgType == EPfqosDelete) ||
	 (iPending == EPendingLoadFile && aMsgType == EPfqosLoadFile) ||
	 (iPending == EPendingUnloadFile && aMsgType == EPfqosUnloadFile)) &&
	(iSelector.GetDst().Match(*aMsg.iDstAddr.iAddr)) &&
	(iSelector.GetSrc().Match(*aMsg.iSrcAddr.iAddr)) &&
	(iSelector.Protocol() == aMsg.iSelector.iExt->protocol) &&
	(iSelector.GetDst().Port() == aMsg.iDstAddr.iAddr->Port()) &&
	(iSelector.GetSrc().Port() == aMsg.iSrcAddr.iAddr->Port()) &&
	(iSelector.MaxPortDst() == aMsg.iDstAddr.iExt->pfqos_port_max) &&
	(iSelector.MaxPortSrc() == aMsg.iSrcAddr.iExt->pfqos_port_max))
		return ETrue;

	return EFalse;
	}

void CPolicy::ProcessEvent(TPfqosMessage& aMsg)
	{
	if ((iSelector.GetDst().Match(*aMsg.iDstAddr.iAddr)) &&
	(iSelector.GetSrc().Match(*aMsg.iSrcAddr.iAddr)) &&
	(iSelector.Protocol() == aMsg.iSelector.iExt->protocol) &&
	(iSelector.GetDst().Port() == aMsg.iDstAddr.iAddr->Port()) &&
	(iSelector.GetSrc().Port() == aMsg.iSrcAddr.iAddr->Port()) &&
	(iSelector.MaxPortDst() == aMsg.iDstAddr.iExt->pfqos_port_max) &&
	(iSelector.MaxPortSrc() == aMsg.iSrcAddr.iExt->pfqos_port_max))
		{
		iCapabilities = aMsg.iEvent.iExt->event_value;

		switch (aMsg.iEvent.iExt->event_type)
			{
		case KPfqosEventFailure:
			if (iObserver && iEventMask & EQoSEventFailure)
				{
				ParseExtensions(aMsg, iPolicy);
				CQoSFailureEvent event(iPolicy, aMsg.iBase.iMsg->pfqos_msg_errno);
				iObserver->Event(event);
				}
			break;

		case KPfqosEventConfirm:
			if (iObserver && iEventMask & EQoSEventConfirm)
				{
				ParseExtensions(aMsg, iPolicy);
				CQoSConfirmEvent event(iPolicy);
				iObserver->Event(event);
		  		}
			break;

		case KPfqosEventAdapt:
			if (iObserver && iEventMask & EQoSEventAdapt)
				{
				ParseExtensions(aMsg, iPolicy);
				CQoSAdaptEvent event(iPolicy, aMsg.iBase.iMsg->pfqos_msg_errno);
				iObserver->Event(event);
				}
			break;

		default:
			return;
			}
		}
	}

void CPolicy::NotifyError(TInt aReason)
	{
	TPendingStatus status = iPending;
	iPending = ENone;

	if (aReason)
		{
		switch (status)
			{
		case EPendingAdd:
		case EPendingUpdate:
			{
			CQoSAddEvent event(&iPolicy, aReason);
			if (iObserver && iEventMask & EQoSEventAddPolicy)
				iObserver->Event(event);
			}
			break;

		case EPendingGet:
			{
			CQoSGetEvent event(NULL, aReason);
			if (iObserver && iEventMask & EQoSEventGetPolicy)
				iObserver->Event(event);
			}
			break;

		case EPendingDelete:
			{
			CQoSDeleteEvent event(aReason);
			if (iObserver && iEventMask & EQoSEventDeletePolicy)
				iObserver->Event(event);
			}
			break;

		default:
			break;
			}
		}
	}


void CPolicy::LoadFileL(const TDesC& aName)
	{
	if (iPending != ENone)
		User::Leave(KErrInUse);
	if (aName.Length() > KMaxFileName)
		User::Leave(KErrArgument);
	CRequest* request = CRequest::NewL(this, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosLoadFile);
	iPending = EPendingLoadFile;
	request->iMsg->AddSelector((TUint8)iSelector.Protocol(), 
							   iManager->Uid().UidType(), 
							   EPfqosFlowspecPolicy, 
							   iSelector.IapId(), 
							   EPfqosApplicationPriority, 
							   TPtr(0,0));
	request->iMsg->AddSrcAddress(iSelector.GetSrc(), 
								 iSelector.GetSrcMask(), 
								 (TUint16)iSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(iSelector.GetDst(), 
								 iSelector.GetDstMask(), 
								 (TUint16)iSelector.MaxPortDst());
	request->iMsg->AddConfigFile(aName);
	iManager->Send(request);
	}

void CPolicy::UnloadFileL(const TDesC& aName)
	{
	if (iPending != ENone)
		User::Leave(KErrInUse);
	if (aName.Length() > KMaxFileName)
		User::Leave(KErrArgument);
	CRequest* request = CRequest::NewL(this, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosUnloadFile);
	iPending = EPendingUnloadFile;
	request->iMsg->AddSelector((TUint8)iSelector.Protocol(), 
							   iManager->Uid().UidType(), 
							   EPfqosFlowspecPolicy, 
							   iSelector.IapId(), 
							   EPfqosApplicationPriority, 
							   TPtr(0,0));
	request->iMsg->AddSrcAddress(iSelector.GetSrc(), 
								 iSelector.GetSrcMask(), 
								 (TUint16)iSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(iSelector.GetDst(), 
								 iSelector.GetDstMask(), 
								 (TUint16)iSelector.MaxPortDst());
	request->iMsg->AddConfigFile(aName);
	iManager->Send(request);
	}
