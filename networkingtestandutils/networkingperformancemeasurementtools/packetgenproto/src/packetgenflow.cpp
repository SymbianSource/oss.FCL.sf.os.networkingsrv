// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//  Implements the CPacketGenFlow class. This class creates an interface
//  to the IPv4 protocol when it is required.
//

#include "packetgenflow.h"
#include <comms-infras/ss_activities.h>
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/netperftrace.h>
#else
#include "netperftrace.h"
#endif
#include <comms-infras/ss_logext.h>
#include <nifmbuf.h>
#include <hal.h>

#ifdef _DEBUG
_LIT8(KNifSubDirTest, "PacketGen"); 
#endif

// for panics
_LIT(KPanicCategory, "PacketGenProtocol");
enum
	{
	KPanic_DestroyReceivedBeforeUnbind = 2001
	};


_LIT8(KIp4ProtocolName, "ip");


using namespace ESock;

//
// class CPacketGenFlow  //
//


CPacketGenFlow::CPacketGenFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
/**
 * Constructor.
 *
 * @param aFactory Reference to the factory which created this object.
 * @param aTheLogger The logging object, ownership is passed to this object
 */
	:ESock::CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf)
	{
 	RDebug::Print(_L("Created PacketGenFlow."));
	LOG_NODE_CREATE(KNifSubDirTest, CPacketGenFlow);
	}

CPacketGenFlow::~CPacketGenFlow()
	{
	iBinders.ResetAndDestroy();
	}



//
// Methods from CSubConnectionFlowBase:  //
//

ESock::MFlowBinderControl* CPacketGenFlow::DoGetBinderControlL()
	{
	return this;
	}


// Messages::ANode
void CPacketGenFlow::ReceivedL(
	const Messages::TRuntimeCtxId& aSender,
	const Messages::TNodeId& aRecipient,
	Messages::TSignatureBase& aMessage
	)
/**
Method called on incoming SCPR messages

@param aCFMessage message base
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
						User::Panic(KPanicCategory,KPanic_DestroyReceivedBeforeUnbind);
						}
					
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

ESock::MLowerControl* CPacketGenFlow::GetControlL(const TDesC8& aProtocol)
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


ESock::MLowerDataSender* CPacketGenFlow::BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl)
	{
	CPacketGenBinder* binder = FindOrCreateBinderL(aProtocol);
	ASSERT(binder);
	binder->BindToUpperL( *aReceiver, *aControl );

	iSubConnectionProvider.RNodeInterface::PostMessage(
		Id(),
		TCFControlProvider::TActive().CRef()
		);

	return binder;
	}

void CPacketGenFlow::Unbind( ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl)
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

ESock::CSubConnectionFlowBase* CPacketGenFlow::Flow()
	{
	return this;
	}


//
// Own methods //
//

CPacketGenBinder* CPacketGenFlow::FindOrCreateBinderL(const TDesC8& aProtocol)
	{
	if (aProtocol.Compare(KIp4ProtocolName))
		{
		User::Leave(KErrNotSupported);
		}

	for (TInt i=0;i<iBinders.Count();++i)
		{
		if(iBinders[i]->ProtocolName() == aProtocol)
			{
			return iBinders[i];
			}
		}
	// not found.. create it.
	CPacketGenBinder* newBinder = CPacketGenBinder::NewL(aProtocol);
	CleanupStack::PushL(newBinder);
	iBinders.AppendL(newBinder);
	CleanupStack::Pop(newBinder);
	return newBinder;
	}
	
    

	

//##################################################################################################
	
//
// class CPacketGenBinder  //
//


//
// My constructor //
//

CPacketGenBinder::CPacketGenBinder(const TDesC8& aProtocolName):
	CActive(CActive::EPriorityStandard),
	iLowerControl(NULL),
	iLowerDataSender(NULL),
	iUpperControl(NULL),
	iUpperDataReceiver(NULL),
	iProtocolName(aProtocolName),
	iGenerating(EFalse)
	{}

CPacketGenBinder* CPacketGenBinder::NewL(const TDesC8& aProtocolName)
	{
	CPacketGenBinder* inst = new(ELeave) CPacketGenBinder(aProtocolName);
	CleanupStack::PushL(inst);
	inst->ConstructL();
	CleanupStack::Pop(inst);
 	RDebug::Print(_L("Created PacketGenBinder."));
	return inst;
	}

void CPacketGenBinder::ConstructL()
	{
	}

/*virtual*/
CPacketGenBinder::~CPacketGenBinder()
	{
// rjl if required: add vectors for multiple senders
	iProperty.Close();
	iPacketTemplate.Free();
	}

//
// Methods from CActive (for pubsub control: //
//
/*virtual*/ void CPacketGenBinder::DoCancel()
	{
	}

/*virtual*/ void CPacketGenBinder::RunL()
	{
	// pub/sub control interface not yet implemented. copy if necessary from
	//  the delay meter binder
	}

//
// Methods from ESock::MLowerControl: //
//

TInt CPacketGenBinder::GetName(TDes& aName)
	{
	TBuf16<10> tmp;
	tmp.Copy(ProtocolName());
	aName.Format(_L("packetgen[%S][0x%08x]"), &tmp, this);
	
	return KErrNone;
	}

TInt CPacketGenBinder::BlockFlow(TBlockOption aOption)
	{
	if (iLowerControl==NULL)
		{
		return KErrNotReady;
		}
	return iLowerControl->BlockFlow(aOption) ;
	}

TInt CPacketGenBinder::GetConfig(TBinderConfig& aConfig)
	{
	if (iLowerControl==NULL)
		{
		return KErrNotReady;
		}
	return iLowerControl->GetConfig(aConfig) ;
	}

TInt CPacketGenBinder::Control(TUint aLevel, TUint aName, TDes8& aOption)
	{
	// Pass it on..
	if (iLowerControl==NULL)
		{
		return KErrNotReady;
		}
	return iLowerControl->Control(aLevel,aName,aOption);
	}
	

//
// Methods from ESock::MLowerDataSender: //
//

ESock::MLowerDataSender::TSendResult CPacketGenBinder::Send(RMBufChain& aData)
	{
	if (iLowerControl==NULL)
		{
		return ESendBlocked; // returning this obliges us to send an unblock later..
								// so perhaps it'd be better to just swallow the packet?
		}

	if ( ! iGenerating )
		{
		RMBufChain payloadBufChain;
		TInt protocol,port,statedLength;
		if ( ExtractIPv4ProtocolAndPort(aData, /*returns*/payloadBufChain,protocol,port,statedLength))
			{
			if(protocol == 17 && port == 5001)	// are we interested?
				{
				NETPERF_TRACE_1(_L("Packet generation trigger packet detected. Starting packet generation."));

				if (payloadBufChain.Length() < statedLength)
					{
					// weird. we can't do anything with this.
					ASSERT(1); // so we can set a breakpoint
					NETPERF_TRACE_3(_L("CDelayMeterBinder::Send: stated size of packet (%d) doesn't match up with observed size (%d). Discarding packet.")
							, statedLength, payloadBufChain.Length());
					}
				else
					{
					iGenerating=ETrue;

					// take a copy of the packet now
					iPacketTemplate.CopyPackedL(aData);
				    
					// set up timers
					HAL::Get(HALData::EFastCounterFrequency,iFcf);
					HAL::Get(HALData::EFastCounterCountsUp,iFcUp);
					iFinishFc = iFcf; // need 2 steps here to escape 32 bit limit..
					iFinishFc *= 30 ; //  30 seconds -	rjl TODO: make this configurable
					iFcCount = 0;
					iLatestFc = iFcUp?User::FastCounter():(~User::FastCounter());
					
				    // pass first packet through
					if(iLowerDataSender->Send(aData) == ESendBlocked)
						{
						return ESendBlocked;
						}
						
					return GeneratePackets(); // will return ESendBlocked if it was flowed off.
					}
				}
			}
		}

	return iLowerDataSender->Send(aData);
	}


//
// Methods from ESock::MUpperControl: //
//

/*virtual*/
void CPacketGenBinder::StartSending()
	{
	ESock::MLowerDataSender::TSendResult sendResult = ESendAccepted;
	if(iGenerating)
		{
		sendResult = GeneratePackets();
		}
	
	if (iUpperControl && sendResult == ESendAccepted)
		{
		iUpperControl->StartSending();
		}
	else
		{
		ASSERT(1); // so a breakpoint can be set if necessary
		}
	}

/*virtual*/
void CPacketGenBinder::Error(TInt anError)
	{
	if (iUpperControl)
		{
		iUpperControl->Error(anError);
		}
	else
		{
		ASSERT(1); // so a breakpoint can be set if necessary
		}
	}


//
// Methods from ESock::MUpperDataReceiver: //
//

/*virtual*/
void CPacketGenBinder::Process(RMBufChain& aData)
	{
	// EXAMPLE NOTE:
	//  This is where your protocol will do its work on incoming data.
	
	if (iUpperDataReceiver == NULL)
		{
		// Why is the guy below still sending data to me when I'm not bound above?
		//   Try to ignore it
		ASSERT(1); // so a breakpoint can be set if necessary
		return;
		}
	iUpperDataReceiver->Process(aData);
	}


//
// and my own methods.. //
//

// called by layer above calling my flow's BindL
void CPacketGenBinder::BindToUpperL(MUpperDataReceiver& aUpperDataReceiver, MUpperControl& aUpperControl)
	{
	if(iUpperDataReceiver || iUpperControl) {User::Leave(KErrInUse);}
	iUpperDataReceiver=&aUpperDataReceiver;
	iUpperControl=&aUpperControl;
	}

// called by layer above calling my flow's Unbind. Returns ETrue if unbind happened here, EFalse otherwise
TBool CPacketGenBinder::UnbindFromUpper(MUpperDataReceiver& aUpperDataReceiver, MUpperControl& aUpperControl)
	{
	if(&aUpperDataReceiver == iUpperDataReceiver && &aUpperControl == iUpperControl)
		{
		iUpperDataReceiver=0;
		iUpperControl=0;
		return ETrue;
		}
	return EFalse;
	}

// called by my flow receiving a BinderRequest
void CPacketGenBinder::BindToLowerFlowL(ESock::MFlowBinderControl& aLowerBinderControl)
	{
	if(iLowerControl || iLowerDataSender)
		{
		User::Leave(KErrInUse);
		}
	iLowerControl = aLowerBinderControl.GetControlL(ProtocolName());
	iLowerDataSender = aLowerBinderControl.BindL(ProtocolName(), this, this);
	}

const TDesC8& CPacketGenBinder::ProtocolName() const
	{
	return iProtocolName;
	}


TBool CPacketGenBinder::ExtractIPv4ProtocolAndPort(RMBufChain& aData,
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


ESock::MLowerDataSender::TSendResult CPacketGenBinder::GeneratePackets()
	{
	while(1)
		{
		// do exit maths first in case we should have already finished..
		TUint32 thisFc = iFcUp?User::FastCounter():(~User::FastCounter());
		TUint32 fcDiff = thisFc - iLatestFc;
		iFcCount += fcDiff;
		iLatestFc = thisFc;

		if(iFcCount>iFinishFc)
			{
			break;
			}
		
		for(TInt i=0;i<100;++i)
			{
			RMBufPacketBase aToSend;
			aToSend.CopyPackedL(iPacketTemplate);
			//iPacketTemplate.Copy(aData);
			ESock::MLowerDataSender::TSendResult sendResult = iLowerDataSender->Send(aToSend);
			if(sendResult == ESendBlocked)
				{
				return ESendBlocked;
				}
			}
		}
	NETPERF_TRACE_1(_L("Packet generation finished."));
	iGenerating=EFalse;
	iPacketTemplate.Free();
	return ESendAccepted;
	}


void CPacketGenBinder::IncrementSequenceNumberAndUpdateChecksum(RMBufChain& /*aPayloadBufChain*/,TInt /*aStampOffset*/,TUint8* /*aChecksumPtr*/)
	{
	// not implemented yet. for now the receiver will have to just count packets below its IP stack
	//  (lest the duplicate packets are disgarded)
	}

	
