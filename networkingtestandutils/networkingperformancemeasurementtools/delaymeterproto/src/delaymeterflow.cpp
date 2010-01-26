// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Protocol used by netperfte to insert timestamps on incoming packets,
// and record delays of outgoing packets which it reports back to netperfte
// 
//

/**
 @file
 @internalTechnology
*/

#include "e32base.h"
#include "delaymeterflow.h"
#include "delaymeterprotofactory.h"
#include <comms-infras/ss_activities.h>
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/delaymeter.h>
#include <networking/netperftrace.h>
#else
#include "netperftrace.h"
#include "delaymeter.h"
#endif
#include <comms-infras/ss_logext.h>
#include <es_mbman.h>

#ifdef _DEBUG
_LIT8(KNifSubDirTest, "DelayMeter");
#endif

// for panics
_LIT(KPanicCategory, "DelayMeterProtocol");
enum
	{
	KPanic_DestroyReceivedBeforeUnbind = 2001
	};


_LIT8(KIp4ProtocolName, "ip");


using namespace ESock;
using namespace DelayMeter;

//
// class CDelayMeterFlow  //
//


CDelayMeterFlow::CDelayMeterFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
/**
 * Constructor.
 *
 * @param aFactory Reference to the factory which created this object.
 * @param aTheLogger The logging object, ownership is passed to this object
 */
	:ESock::CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf)
	{
	LOG_NODE_CREATE(KNifSubDirTest, CDelayMeterFlow);
	}

CDelayMeterFlow::~CDelayMeterFlow()
	{
	iBinders.ResetAndDestroy();
	}



//
// Methods from CSubConnectionFlowBase:  //
//

ESock::MFlowBinderControl* CDelayMeterFlow::DoGetBinderControlL()
	{
	return this;
	}

// Messages::ANode
void CDelayMeterFlow::ReceivedL(
	const Messages::TRuntimeCtxId& aSender,
	const Messages::TNodeId& aRecipient,
	Messages::TSignatureBase& aMessage
	)
/**
Method called on incoming SCPR messages

@param aSender Sending Node
@param aMessage message base
@param aMessage The message
*/
    {
    CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);

	// Dispatch the message locally
	if (ESock::TCFDataClient::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{

			case ESock::TCFDataClient::TStart::EId :
				{
				iSubConnectionProvider.RNodeInterface::PostMessage(
					Id(),
					ESock::TCFDataClient::TStarted().CRef()
					);
				}
				break;

			case ESock::TCFDataClient::TStop::EId :
				{
				TInt i;
				for (i=iBinders.Count()-1;i>=0;--i)
					{
					delete iBinders[i];
					iBinders.Remove(i);
					}
				iSubConnectionProvider.PostMessage(Id(), TCFDataClient::TStopped(KErrNone).CRef());
				}
				break;

			case ESock::TCFDataClient::TProvisionConfig::EId :
				{
				ESock::TCFDataClient::TProvisionConfig& aMess = Messages::message_cast<ESock::TCFDataClient::TProvisionConfig>(aMessage);
				iAccessPointConfig.Close();
				iAccessPointConfig.Open(aMess.iConfig);
				}
				break;

			case ESock::TCFDataClient::TBindTo::EId :
				{
				ESock::TCFDataClient::TBindTo& bindToReq = Messages::message_cast<ESock::TCFDataClient::TBindTo>(aMessage);
				if (bindToReq.iNodeId == Messages::TNodeId::NullId())
					{
					User::Leave(KErrNotSupported);
					}

				const Messages::TNodeId& commsId = bindToReq.iNodeId;
				CSubConnectionFlowBase* lowerFlow = Messages::mnode_cast<CSubConnectionFlowBase>(&commsId.Node());

				MFlowBinderControl* lowerBinderControl = lowerFlow->GetBinderControlL();
				ASSERT(lowerBinderControl);

				TInt i;
				for (i=0;i<iBinders.Count();++i)
					{
					// binder for each protocol will request binder for same protocol from lower binder controller using this fn.
					iBinders[i]->BindToLowerFlowL(*lowerBinderControl);
					}
				ASSERT(i); // there should be some binders!

				Messages::RClientInterface::OpenPostMessageClose(
					Id(),
					aSender,
					ESock::TCFDataClient::TBindToComplete().CRef()
					);
				}
				break;

			default:
				ASSERT(EFalse);
			}
		}
	else if (Messages::TEChild::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
			case Messages::TEChild::TDestroy::EId :
				{
				TInt i;
				for (i=0;i<iBinders.Count();++i)
					{
					// ensure all binders unbound
					if (iBinders[i]->InUse())
						{
						// WHO IS CALLING DESTROY ON ME BEFORE UNBINDING? They need a punch.
						NETPERF_TRACE_1(_L("something is sending TDestroy to CDelayMeterFlow before unbinding."));
						User::Panic(KPanicCategory,KPanic_DestroyReceivedBeforeUnbind);
						}
						
					// EXAMPLE CODE: cancel requests here if necessary...
					//iBinders[i]->Cancel();
					
					}
				if (i==iBinders.Count()) // all unbound
					{
					DeleteThisFlow();
					}
				}
				break;

			default:
				ASSERT(EFalse);
			}
		}
	// realm != TCFMessage::ERealmId
	else
		{
		ASSERT(EFalse);
		}
	}

//
// Methods from MFlowBinderControl:  //
//

ESock::MLowerControl* CDelayMeterFlow::GetControlL(const TDesC8& aProtocol)
/**
Create and return an MLowerControl instance of the specified binder type.

Called from upper layer during binding procedure.

@param aProtocol Protocol type of the binder
@return MLowerControl instance of the protocol type
*/
	{
	ESock::MLowerControl* lowerControl = FindOrCreateBinderL(aProtocol);
	ASSERT(lowerControl);
	return lowerControl;
	}


ESock::MLowerDataSender* CDelayMeterFlow::BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl)
	{
	CDelayMeterBinder* binder = FindOrCreateBinderL(aProtocol);
	ASSERT(binder);
	binder->BindToUpperL( *aReceiver, *aControl );

	iSubConnectionProvider.RNodeInterface::PostMessage(
		Id(),
		TCFControlProvider::TActive().CRef()
		);

	return binder;
	}

void CDelayMeterFlow::Unbind( ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl)
	{
	ASSERT(aReceiver);
	ASSERT(aControl);
	TInt i;
	TInt numberUnbound=0;
	for (i=0;i<iBinders.Count();++i)
		{
		numberUnbound += (iBinders[i]->UnbindFromUpper(*aReceiver,*aControl) ? 1 : 0);
		}
	ASSERT(i); // there should be some binders!
	ASSERT(numberUnbound<=1); // only 1 unbind should have happened

	iSubConnectionProvider.RNodeInterface::PostMessage(
		Id(),
		TCFControlProvider::TIdle().CRef()
		);
	}

ESock::CSubConnectionFlowBase* CDelayMeterFlow::Flow()
	{
	return this;
	}


//
// Own methods //
//

CDelayMeterBinder* CDelayMeterFlow::FindOrCreateBinderL(const TDesC8& aProtocol)
	{
	if (aProtocol.Compare(KIp4ProtocolName))
		{  // only work with IPv4
		User::Leave(KErrNotSupported);
		}

	for (TInt i=0;i<iBinders.Count();++i)
		{
		if (iBinders[i]->ProtocolName() == aProtocol)
			{
			return iBinders[i];
			}
		}
	// not found.. create it.
	CDelayMeterBinder* newBinder = CDelayMeterBinder::NewL();
	CleanupStack::PushL(newBinder);
	iBinders.AppendL(newBinder);
	CleanupStack::Pop(newBinder);
	return newBinder;
	}




//##################################################################################################

//
// class CDelayMeterBinder  //
//


//
// My constructor //
//

CDelayMeterBinder::CDelayMeterBinder():
	CActive(CActive::EPriorityStandard),
	iLowerControl(NULL),
	iLowerDataSender(NULL),
	iUpperControl(NULL),
	iUpperDataReceiver(NULL)
	{
	}


CDelayMeterBinder* CDelayMeterBinder::NewL()
	{
	CDelayMeterBinder* inst = new(ELeave) CDelayMeterBinder;
	CleanupStack::PushL(inst);
	inst->ConstructL();
	CleanupStack::Pop(inst);
	return inst;
	}

void CDelayMeterBinder::ConstructL()
	{
	CActiveScheduler::Add(this);

	DelayMeter::DefinePubSubKeysL();

	// watch for incoming commands
	User::LeaveIfError(iProperty.Attach(TUid::Uid(KDelayMeterControlLevel), KCommandToDelayMeter));

	iProperty.Subscribe(iStatus);
	SetActive();
	}

/*virtual*/
CDelayMeterBinder::~CDelayMeterBinder()
	{
	iSendRecorders.ResetAndDestroy();
	iReceiveTimestampers.ResetAndDestroy();
	DelayMeter::UndefinePubSubKeys();
	}


//
// Methods from CActive (for pubsub control: //
//

/*virtual*/ void CDelayMeterBinder::DoCancel()
	{
	iProperty.Cancel();
	}

/*virtual*/ void CDelayMeterBinder::RunL()
	{
	// get command number from pub/sub key 0
	TInt name=0;
	iProperty.Get(name);

	// get passed in buffer from pub/sub key KDataTransfer
	HBufC8* buffer = HBufC8::New(RProperty::KMaxLargePropertySize);
	User::LeaveIfNull(buffer);
	CleanupStack::PushL(buffer);
	TPtr8 ptr = buffer->Des();
	RProperty::Get(TUid::Uid(KDelayMeterControlLevel), KDataTransfer, ptr);

	// do the actual call
	TInt ret = Control(KDelayMeterControlLevel,name,ptr);

	// set the results buffer back to pub/sub key KDataTransfer
	RProperty::Set(TUid::Uid(KDelayMeterControlLevel), KDataTransfer, ptr);
	CleanupStack::PopAndDestroy(buffer);

	// signal finished by writing return code to subkey KResponseFromDelayMeter. tear down the scheduler if we can't,
	//   hopefully the deadlock will be noticed by the size of the explosion.
	User::LeaveIfError(RProperty::Set(TUid::Uid(KDelayMeterControlLevel), KResponseFromDelayMeter, ret));

	iProperty.Subscribe(iStatus);
	SetActive();
	}

//
// Methods from ESock::MLowerControl: //
//

TInt CDelayMeterBinder::GetName(TDes& aName)
	{
	TBuf16<10> tmp;
	tmp.Copy(ProtocolName());
	aName.Format(_L("delaymeter[%S][0x%08x]"), &tmp, this);

	return KErrNone;
	}

TInt CDelayMeterBinder::BlockFlow(TBlockOption aOption)
	{
	if (iLowerControl==NULL)
		{
		return KErrNotReady;
		}
	return iLowerControl->BlockFlow(aOption) ;
	}

TInt CDelayMeterBinder::GetConfig(TBinderConfig& aConfig)
	{
	if (iLowerControl==NULL)
		{
		return KErrNotReady;
		}
	return iLowerControl->GetConfig(aConfig) ;
	}


TInt CDelayMeterBinder::Control(TUint aLevel, TUint aName, TDes8& aOption)
	{
	if (aLevel == KDelayMeterControlLevel)
		{
		if (aName <= KDelayMeterControl_NumberOfControlsPerInstance)
			{
			// prefs always passed in. on Start for initialisation,
			//   on GetResults, for identification of which stamper/recorder to get results from
			if (aOption.Length() < sizeof(TDelayMeterPrefs) )
				{
				return KErrCorrupt;
				}
			TUint8* ptr = const_cast<TUint8*>(aOption.Ptr());
			TDelayMeterPrefs* prefs = reinterpret_cast<TDelayMeterPrefs*> (ptr);

			TRAPD(result,ControlL(aName,aOption,*prefs));

			return result;
			}
		else
			{
			// so it's for some instance below, so remove this "layer"...
			aName -= KDelayMeterControl_NumberOfControlsPerInstance;
			}
		}

	// Pass it on..
	if (iLowerControl==NULL)
		{
		return KErrNotReady;
		}
	return iLowerControl->Control(aLevel,aName,aOption);
	}


void CDelayMeterBinder::ControlL(TUint aName, TDes8& aOption, TDelayMeterPrefs& prefs)
	{
	switch(aName)
		{
		case KDelayMeterControl_StartSendRecorder:
			StartSendRecorderL(aOption,prefs);
			break;

		case KDelayMeterControl_GetSendRecorderResults:
			GetSendRecorderResultsL(aOption,prefs);
			break;

		case KDelayMeterControl_StartReceiveTimestamper:
			StartReceiveTimestamperL(aOption,prefs);
			break;

		case KDelayMeterControl_GetReceiveTimestamperResults:
			GetReceiveTimestamperResultsL(aOption,prefs);
			break;
		}
	}


void CDelayMeterBinder::StartSendRecorderL(TDes8& aOption, TDelayMeterPrefs& prefs)
	{
	if (aOption.Length() != sizeof(TDelayMeterPrefs) )
		{
		User::Leave(KErrCorrupt);
		}

	if (FindSendRecorder(prefs.iProtocol, prefs.iDestinationPort))
		{
		User::Leave(KErrInUse);
		}

	CSendRecorder* newRecorder = new (ELeave) CSendRecorder;
	CleanupStack::PushL(newRecorder);
	newRecorder->SetupL(prefs);
	iSendRecorders.AppendL(newRecorder);
	CleanupStack::Pop(newRecorder);
	}

void CDelayMeterBinder::GetSendRecorderResultsL(TDes8& aOption, TDelayMeterPrefs& prefs)
	{
	CSendRecorder* recorder = FindSendRecorder(prefs.iProtocol,prefs.iDestinationPort, /*remove entry when found:*/ETrue);
	if (recorder == NULL)
		{
		User::Leave(KErrNotReady);
		}
	CleanupStack::PushL(recorder); // I own this til its destruction

	TInt nResults=0;
	TDelaySampleSet* results=NULL;
	recorder->GetResultsL(nResults,results);
	TInt resultsLength=(nResults * sizeof(TDelaySampleSet));

	if (aOption.Length() < resultsLength )
		{
		User::Leave(KErrOverflow);
		}

	TUint8* results_output = const_cast<TUint8*>(aOption.Ptr());
	Mem::Copy(results_output,(TUint8*)(results),resultsLength);
	aOption.SetLength(resultsLength);

	CleanupStack::PopAndDestroy(recorder);
	}

void CDelayMeterBinder::StartReceiveTimestamperL(TDes8& aOption, TDelayMeterPrefs& prefs)
	{
	if (aOption.Length() != sizeof(TDelayMeterPrefs) )
		{
		User::Leave(KErrCorrupt);
		}

	if (FindReceiveTimestamper(prefs.iProtocol, prefs.iDestinationPort))
		{
		User::Leave(KErrInUse);
		}

	CReceiveTimestamper* newStamper = new (ELeave) CReceiveTimestamper;
	CleanupStack::PushL(newStamper);
	newStamper->SetupL(prefs);
	iReceiveTimestampers.AppendL(newStamper);
	CleanupStack::Pop(newStamper);
	}

void CDelayMeterBinder::GetReceiveTimestamperResultsL(TDes8& aOption, TDelayMeterPrefs& prefs)
	{
	CReceiveTimestamper* stamper = FindReceiveTimestamper(prefs.iProtocol,prefs.iDestinationPort, /*remove entry when found:*/ETrue);
	if (stamper == NULL)
		{
		User::Leave(KErrNotReady);
		}
	CleanupStack::PushL(stamper); // I own this til its destruction

	TInt nResults=0;
	TDelaySampleSet* results=NULL;
	stamper->GetResultsL(nResults,results);
	TInt resultsLength=(nResults * sizeof(TDelaySampleSet));

	if (aOption.Length() < resultsLength )
		{
		User::Leave(KErrOverflow);
		}

	TUint8* results_output = const_cast<TUint8*>(aOption.Ptr());
	Mem::Copy(results_output,(TUint8*)(results),resultsLength);

	CleanupStack::PopAndDestroy(stamper);
	}

//
// Methods from ESock::MLowerDataSender: //
//

ESock::MLowerDataSender::TSendResult CDelayMeterBinder::Send(RMBufChain& aData)
	{
	if (iSendRecorders.Count())
		{
		RMBufChain payloadBufChain;
		TInt protocol,port,statedLength;
		if ( ExtractIPv4ProtocolAndPort(aData, /*returns*/payloadBufChain,protocol,port,statedLength))
			{
			// are we interested?
			CSendRecorder* stamper = FindSendRecorder(protocol,port);
			if (stamper)
				{
				if (payloadBufChain.Length() < statedLength)
					{
					// weird. we can't do anything with this.
					ASSERT(1); // so we can set a breakpoint
					NETPERF_TRACE_3(_L("CDelayMeterBinder::Send: stated size of packet (%d) doesn't match up with observed size (%d). Discarding packet.")
							, statedLength, payloadBufChain.Length());
					}
				else
					{
					stamper->RecordDelay(payloadBufChain,statedLength);
					}
				}
			}
		}

	if (iLowerControl==NULL)
		{
		return ESendBlocked; // returning this obliges us to send an unblock later..
								// so perhaps it'd be better to just swallow the packet?
		}
	return iLowerDataSender->Send(aData);
	}


//
// Methods from ESock::MUpperControl: //
//

/*virtual*/
void CDelayMeterBinder::StartSending()
	{
	if (iUpperControl)
		{
		iUpperControl->StartSending();
		}
	else
		{
		ASSERT(1); // to allow setting a breakpoint
		NETPERF_TRACE_1(_L("CDelayMeterBinder::StartSending: upper control not yet known!"));
		}
	}

/*virtual*/
void CDelayMeterBinder::Error(TInt anError)
	{
	if (iUpperControl)
		{
		iUpperControl->Error(anError);
		}
	else
		{
		ASSERT(1); // to set a breakpoint
		NETPERF_TRACE_1(_L("CDelayMeterBinder::Error: upper control not yet known!"));
		}
	}


//
// Methods from ESock::MUpperDataReceiver: //
//



/*virtual*/
void CDelayMeterBinder::Process(RMBufChain& aData)
	{
	if (iUpperDataReceiver == NULL)
		{
		// Why is the guy below still sending data to me when I'm not bound above?
		//   Try to ignore it
		ASSERT(1); // so a breakpoint can be set if necessary
		NETPERF_TRACE_1(_L("CDelayMeterBinder::Process: incoming traffic discarded as upper data receiver not (or no longer) set"));
		return;
		}
	if (iReceiveTimestampers.Count())
		{
		RMBufChain payloadBufChain;
		TInt protocol,port,statedLength;
		if ( ExtractIPv4ProtocolAndPort(aData, /*returns*/payloadBufChain,protocol,port,statedLength))
			{
			// are we interested?
			CReceiveTimestamper* stamper = FindReceiveTimestamper(protocol,port);
			if (stamper)
				{
				//	TInt statedLength = udpStatedLength + 20 /*ip header*/ ;
				if (payloadBufChain.Length() < statedLength)
					{
					// weird. we can't do anything with this.
					ASSERT(1); // so we can set a breakpoint
					NETPERF_TRACE_3(_L("CDelayMeterBinder::Process: stated size of packet (%d) doesn't match up with observed size (%d). Discarding packet.")
							, statedLength, payloadBufChain.Length());
					}
				else
					{
					stamper->Timestamp(payloadBufChain,statedLength);
					}
				}
			else
				{
				NETPERF_TRACE_1(_L("CDelayMeterBinder::Process: incoming traffic not stamped as no matching stamper found"));
				}
			}
		}
	else
		{
		NETPERF_TRACE_1(_L("CDelayMeterBinder::Process: incoming traffic not stamped as no stampers are configured"));
		}


	iUpperDataReceiver->Process(aData);
	}


//
// and my own methods.. //
//

// called by layer above calling my flow's BindL
void CDelayMeterBinder::BindToUpperL(MUpperDataReceiver& aUpperDataReceiver, MUpperControl& aUpperControl)
	{
	if (iUpperDataReceiver || iUpperControl) {User::Leave(KErrInUse);}
	iUpperDataReceiver=&aUpperDataReceiver;
	iUpperControl=&aUpperControl;
	}

// called by layer above calling my flow's Unbind. Returns ETrue if unbind happened here, EFalse otherwise
TBool CDelayMeterBinder::UnbindFromUpper(MUpperDataReceiver& aUpperDataReceiver, MUpperControl& aUpperControl)
	{
	if (&aUpperDataReceiver == iUpperDataReceiver && &aUpperControl == iUpperControl)
		{
		iUpperDataReceiver=0;
		iUpperControl=0;
		return ETrue;
		}
	return EFalse;
	}

// called by my flow receiving a BinderRequest
void CDelayMeterBinder::BindToLowerFlowL(ESock::MFlowBinderControl& aLowerBinderControl)
	{
	if (iLowerControl || iLowerDataSender)
		{
		User::Leave(KErrInUse);
		}
	iLowerControl = aLowerBinderControl.GetControlL(ProtocolName());
	iLowerDataSender = aLowerBinderControl.BindL(ProtocolName(), this, this);
	}

const TDesC8& CDelayMeterBinder::ProtocolName() const
	{
	return KIp4ProtocolName;
	}


CSendRecorder* CDelayMeterBinder::FindSendRecorder(TInt aProtocol, TInt aPortNumber, TBool aRemoveFoundEntry)
	{
	for (TInt i=0;i<iSendRecorders.Count();++i)
		{
		CSendRecorder* recorder = iSendRecorders[i];
		if (recorder->iPrefs.iDestinationPort == aPortNumber &&
		   recorder->iPrefs.iProtocol == aProtocol )
			{
			if (aRemoveFoundEntry)
				{
				iSendRecorders.Remove(i);
				}
			return recorder;
			}
		}
	return NULL;
	}

CReceiveTimestamper* CDelayMeterBinder::FindReceiveTimestamper(TInt aProtocol, TInt aPortNumber, TBool aRemoveFoundEntry)
	{
	for (TInt i=0;i<iReceiveTimestampers.Count();++i)
		{
		CReceiveTimestamper* sender = iReceiveTimestampers[i];
		if (sender->iPrefs.iDestinationPort == aPortNumber &&
		   sender->iPrefs.iProtocol == aProtocol )
			{
			if (aRemoveFoundEntry)
				{
				iReceiveTimestampers.Remove(i);
				}
			return sender;
			}
		}
	return NULL;
	}

TBool CDelayMeterBinder::ExtractIPv4ProtocolAndPort(RMBufChain& aData,
											/*returns: */ RMBufChain& aPayload, TInt& aProtocol, TInt& aPort, TInt& aLength)
	{
	RMBuf* mbuf = aData.First();
	ASSERT(mbuf);
	ASSERT(mbuf->Type()==EMBufHeader);
	mbuf=mbuf->Next();
	ASSERT(mbuf);
	ASSERT(mbuf->Type()==EMBufData);  // this should be smart enough..
	aPayload = mbuf;

	if (aPayload.Align(38) != 38) return EFalse; // cover whole IP+(UDP/TCP ports) header, and
							// TCP header up to checksum (which covers UDP checksum too)

	TUint8* payload = mbuf->Ptr();

	//check IP version (4) and IP header length(5*4))
	if (payload[0] != (TInt8)0x45) return EFalse;
	//check that there is no IP fragmentation
	if ((payload[6] & (~0x40)) != (TInt8)0  || payload[7] != (TInt8)0) return EFalse;

	//extract protocol, port, length..
	aProtocol = payload[9];
	aPort = (payload[22]<<8) + payload[23];
	aLength = (payload[2]<<8) + payload[3];
	return ETrue;
	}



