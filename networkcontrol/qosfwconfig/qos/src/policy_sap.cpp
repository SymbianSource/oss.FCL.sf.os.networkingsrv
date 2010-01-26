// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "policy_sap.h"
#include "qos_prot.h"
#include "qos_channel.h"
#include "policy_mgr.h"

#ifdef QOS_PLATSEC_CAPABILITY
// Qos security policies
_LIT_SECURITY_POLICY_C1(policyNetworkControl, ECapabilityNetworkControl);
#endif


void Panic(TPfqosPanic aPanic)
	{
	_LIT(K_PFQOS, "PFQOS");
	User::Panic(K_PFQOS, aPanic);
	}

CQoSProvider::CQoSProvider(CProtocolQoS& aProtocol) : iProtocol(aProtocol)
	{
	}

CQoSProvider::~CQoSProvider()
	{
	// Release all policies by this SAP
	iProtocol.PolicyMgr()->DoCleanUp(iProtocol, (TUint)this);
	// Remove all references from channels to this SAP
	iProtocol.ChannelMgr()->DetachFromAll(this);
	iRecvQ.Free();	// Release all pending buffers.
	iSAPlink.Deque();
	LOG(Log::Printf(_L("~\tqos SAP[%u] destruction completed"), (TInt)this));
	}
	
#ifdef _LOG
static const TDesC& MsgTypeName(TInt aType)
	{
	_LIT(KTxtReserved,			"EPfqosReserved");
    _LIT(KTxtUpdate,			"EPfqosUpdate");
    _LIT(KTxtAdd,				"EPfqosAdd");
	_LIT(KTxtDelete,			"EPfqosDelete");
	_LIT(KTxtGet,				"EPfqosGet");
	_LIT(KTxtFlush,				"EPfqosFlush");
	_LIT(KTxtDump,				"EPfqosDump");
	_LIT(KTxtEvent,				"EPfqosEvent");
	_LIT(KTxtConfigure,			"EPfqosConfigure");
	_LIT(KTxtReject,			"EPfqosReject");
	_LIT(KTxtJoin,				"EPfqosJoin");
	_LIT(KTxtLeave,				"EPfqosLeave");
	_LIT(KTxtCreateChannel,		"EPfqosCreateChannel");
	_LIT(KTxtOpenExistingChannel,"EPfqosOpenExistingChannel");
    _LIT(KTxtDeleteChannel,		"EPfqosDeleteChannel");
 	_LIT(KTxtConfigChannel,		"EPfqosConfigChannel");
	_LIT(KTxtLoadFile,			"EPfqosLoadFile");
	_LIT(KTxtUnloadFile,		"EPfqosUnloadFile");
	_LIT(KTxtUnknown,			"EPfqos unknown");
	
	switch(aType)
		{
		case EPfqosReserved: return KTxtReserved;
		case EPfqosUpdate: return KTxtUpdate;
		case EPfqosAdd: return KTxtAdd;
		case EPfqosDelete: return KTxtDelete;
		case EPfqosGet: return KTxtGet;
		case EPfqosFlush: return KTxtFlush;
		case EPfqosDump: return KTxtDump;
		case EPfqosEvent: return KTxtEvent;
		case EPfqosConfigure: return KTxtConfigure;
		case EPfqosReject: return KTxtReject;
		case EPfqosJoin: return KTxtJoin;
		case EPfqosLeave: return KTxtLeave;
		case EPfqosCreateChannel: return KTxtCreateChannel;
		case EPfqosOpenExistingChannel: return KTxtOpenExistingChannel;
		case EPfqosDeleteChannel: return KTxtDeleteChannel;
		case EPfqosConfigChannel: return KTxtConfigChannel;
		case EPfqosLoadFile: return KTxtLoadFile;
		case EPfqosUnloadFile: return KTxtUnloadFile;
		default: break;
		}
	return KTxtUnknown;
	}
#endif

TUint CQoSProvider::Write(const TDesC8 &aDesc, TUint /*aOptions*/, TSockAddr* /*aAddr=NULL*/)
	{
	TPfqosMessage msg(aDesc);

	// Note: TPfqosMessage does not check pfqos_msg_errno!
#ifdef _LOG
	const TInt type = msg.iBase.iMsg ? msg.iBase.iMsg->pfqos_msg_type : -1;
	Log::Printf(_L("Write\tqos SAP[%u] %S(%d)"), (TInt)this, &MsgTypeName(type), type); 
#endif

	if (msg.iError == KErrNone && msg.iBase.iMsg && msg.iBase.iMsg->pfqos_msg_errno == 0)
		iProtocol.Exec(msg, this);
	else
		{
		iSocket->Error(msg.iError ? msg.iError : KErrGeneral, MSocketNotify::EErrorSend);
		LOG(Log::Printf(_L("\tqos SAP[%u] Write Message corrupt"), (TInt)this));
		}
	return aDesc.Size();
	}

void CQoSProvider::GetData(TDes8 &aDesc, TUint aOptions, TSockAddr* /*aAddr*/)
	{
	RMBufPacket packet;

	if (!iRecvQ.Remove(packet))
		Panic(EPfqosPanic_NoData);
	packet.Unpack();
	packet.CopyOut(aDesc);
	if (aOptions & KSockReadPeek)
		{
		LOG(Log::Printf(_L("GetData\tqos SAP[%u] length=%d [max=%d] (peek) queued=%d bytes in %d msgs remains"),
				(TInt)this, packet.Length(), aDesc.MaxLength(), iQueuedBytes, iQueuedPackets));
		packet.Pack();
		iRecvQ.Prepend(packet);
		iSocket->NewData(1);
		}
	else
		{
		iQueuedPackets -= 1;
		iQueuedBytes -= packet.Length(); 	// (info->iLength is not maintained!)
		LOG(Log::Printf(_L("GetData\tqos SAP[%u] length=%d [max=%d] queued=%d bytes in %d msgs remains"),
				(TInt)this, packet.Length(), aDesc.MaxLength(), iQueuedBytes, iQueuedPackets));
		packet.Free();
		}
	}

void CQoSProvider::Start()
	{
	}

void CQoSProvider::LocalName(TSockAddr & /*aAddr*/) const
	{
	}

void CQoSProvider::ActiveOpen()
	{
	}

TInt CQoSProvider::PassiveOpen(TUint /*aQueSize*/)
	{
	return 0;
	}

void CQoSProvider::ActiveOpen(const TDesC8& /*aConnectionData*/)
	{
	}

TInt CQoSProvider::PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/)
	{
	return 0;
	}

void CQoSProvider::Shutdown(TCloseType aOption, const TDesC8& /*aDisconnectionData*/)
	{
	Shutdown(aOption);
	}

void CQoSProvider::Shutdown(TCloseType aOption)
	{
	LOG(Log::Printf(_L("Shutdown\tqos SAP[%u] aOption=%d"), (TInt)this, (TInt)aOption));
	switch(aOption)
	{
	case EStopInput:
		iFlags &= ~KProviderKey_ExpectInput;
		iRecvQ.Free();
		iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
		break;

	case EStopOutput:
		// PFQOS does not currently have asynchronous output, all
		// messages are processed as they arrive.
		iSocket->Error(KErrNone,MSocketNotify::EErrorClose); // Complete KErrNone
		break;
	default:
		iFlags &= ~KProviderKey_ExpectInput;
		if (aOption!=EImmediate)
			{
			LOG(Log::Printf(_L("\tqos SAP[%u] aOption!=EImmediate"), (TInt)this));
			iSocket->CanClose();
			}
		}
	}

void CQoSProvider::Ioctl(TUint /*aLevel*/, TUint /*aName*/, TDes8* /*aOption*/)
	{
	}

void CQoSProvider::CancelIoctl(TUint /*aLevel*/, TUint /*aName*/)
	{
	}

TInt CQoSProvider::SetOption(TUint /*aLevel*/, TUint /*aName*/, const TDesC8& /*aOption*/)
	{
	return KErrNotSupported;
	}

TInt CQoSProvider::GetOption(TUint /*aLevel*/, TUint /*aName*/, TDes8& /*aOption*/) const
	{
	return KErrNotSupported;
	}

TInt CQoSProvider::SetLocalName(TSockAddr &/*aAddr*/)
	{
	return KErrNone;
	}

void CQoSProvider::RemName(TSockAddr &/*aAddr*/) const
	{
	}

TInt CQoSProvider::SetRemName(TSockAddr &/*aAddr*/)
	{
	return KErrNone;
	}

void CQoSProvider::AutoBind()
	{
	}

void CQoSProvider::Deliver(TPfqosMessage& aMsg)
	{
	RMBufRecvPacket packet;
	RMBufRecvInfo *info = NULL;

	// Allocate info and fill with dummy values, none of which
	// make any sense really, nobody is using them anyways for
	// these packets...
	//
	TRAPD(err, info = packet.NewInfoL());
	packet.SetInfo(info);
	if (err == KErrNone)
		{
		info->iProtocol = KProtocolQoS;
		info->iLength = 0;
		info->iInterfaceIndex = 0;

#ifdef _LOG
		if(aMsg.iEvent.iExt != NULL)
			{
			switch (aMsg.iEvent.iExt->event_type)
				{
			case KPfqosEventFailure:
				Log::Printf(_L("\tqos SAP[%u] Received Event: KPfqosEventFailure.  Error=%d"), (TInt)this, aMsg.iBase.iMsg->pfqos_msg_errno);
				break;
			case KPfqosEventConfirm:
				Log::Printf(_L("\tqos SAP[%u] Received Event: KPfqosEventConfirm.  Error=%d"), (TInt)this, aMsg.iBase.iMsg->pfqos_msg_errno);
				break;			
			case KPfqosEventAdapt:
				Log::Printf(_L("\tqos SAP[%u] Received Event: KPfqosEventAdapt.  Error=%d"), (TInt)this, aMsg.iBase.iMsg->pfqos_msg_errno);
				break;
			case KPfqosEventJoin:
				Log::Printf(_L("\tqos SAP[%u] Received Event: KPfqosEventJoin.  Error=%d"), (TInt)this, aMsg.iBase.iMsg->pfqos_msg_errno);
				break;
			case KPfqosEventLeave:
				Log::Printf(_L("\tqos SAP[%u] Received Event: KPfqosEventLeave.  Error=%d"), (TInt)this, aMsg.iBase.iMsg->pfqos_msg_errno);
				break;
			default:
				Log::Printf(_L("\tqos SAP[%u] Received Event: OTHERS.  Error=%d"), (TInt)this, aMsg.iBase.iMsg->pfqos_msg_errno);
				break;
				}
			}
#endif
		//?? ByteStreamL does not seem to maintain info->iLength! Just a causion.
		//?? It's not currently used for anything..
		TRAP(err, aMsg.ByteStreamL(packet));
		if (err == KErrNone)
			{
			iQueuedPackets += 1;
			iQueuedBytes += packet.Length();
			LOG(Log::Printf(_L("\tqos SAP[%u] NewData(1) length=%d (deliver) queue=%d bytes in %d msgs"),
				(TInt)this, packet.Length(), iQueuedBytes, iQueuedPackets));
			packet.Pack();
			iRecvQ.Append(packet);
			
			iSocket->NewData(1);
			return;
			}
		}
	packet.Free();
	}

#ifdef QOS_PLATSEC_CAPABILITY
TInt CQoSProvider::SecurityCheck(MProvdSecurityChecker *aSecurityChecker)
	{
	return aSecurityChecker->CheckPolicy(policyNetworkControl, "CQoSProvider");
	}
#endif
