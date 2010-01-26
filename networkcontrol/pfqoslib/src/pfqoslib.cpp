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

#include "pfqoslib.h"
#include "pfqosparser.h"
#include "pfqosliblog.h"
//
// DLL entry point
//
GLDEF_C TInt E32Dll()
	{
	return(KErrNone);
	}


EXPORT_C T_pfqos_msg::T_pfqos_msg(TUint8 aMsgType, TUint32 aSeq)
	{
	pfqos_msg_version = KPfqosMsgV1;
	pfqos_msg_type = aMsgType;
	pfqos_msg_errno = 0;
	pfqos_msg_options = 0;
	pfqos_msg_len = ((sizeof(*this) + 7) / 8);
	pfqos_msg_reserved = 0;
	pfqos_msg_seq = aSeq;
	pfqos_msg_pid = 0;
	}


EXPORT_C T_pfqos_selector::T_pfqos_selector(CSelectorBase* aSelector)
	{
	pfqos_selector_len = ((sizeof(*this) + 7) / 8);
	pfqos_ext_type = EPfqosExtSelector;
	protocol = aSelector->iProtocol;
	uid1 = aSelector->iUid.UidType()[0].iUid;
	uid2 = aSelector->iUid.UidType()[1].iUid;
	uid3 = aSelector->iUid.UidType()[2].iUid;
	iap_id = aSelector->iIapId;
	policy_type = aSelector->iType;
	priority = aSelector->iPriority;
	reserved = 0;
	TPtr8 ptr((TUint8*)name, 0, KMaxName);
	if (ptr.MaxLength() >= aSelector->iName.Length())
		{
		ptr.Copy(aSelector->iName);
		}
	ptr.ZeroTerminate();
	}

EXPORT_C T_pfqos_address::T_pfqos_address(TUint aType, TUint16 aPortMax)
	{
	pfqos_address_len = ((sizeof(*this) + sizeof(TInetAddr) + 
						  sizeof(TInetAddr) + 7) / 8);
	pfqos_ext_type = (TUint8)aType;
	reserved = 0;
	pfqos_port_max = aPortMax;
	}


EXPORT_C T_pfqos_module::T_pfqos_module(TUint aProtocolId, TUint32 aFlags, 
					const TDesC& aName, const TDesC& aPath, TInt aDataLen)
	{
	pfqos_modulespec_len = (TUint16)((sizeof(*this) + aDataLen + 7) / 8);
	pfqos_ext_type = EPfqosExtModulespec;
	protocol_id = aProtocolId;
	flags = aFlags;
	TPtr8 namePtr((TUint8*)name, 0, KMaxName);
	if (namePtr.MaxLength() >= aName.Length())
		{
		namePtr.Copy(aName);
		}
	namePtr.ZeroTerminate();
	TPtr8 pathPtr((TUint8*)path, 0, KMaxFileName);
	if (pathPtr.MaxLength() >= aPath.Length())
		{
		pathPtr.Copy(aPath);
		}
	pathPtr.ZeroTerminate();
	reserved = 0;
	}
 
//lint -e{1928}
EXPORT_C T_pfqos_flowspec::T_pfqos_flowspec(const TQoSParameters& aParameters)
	{
	pfqos_flowspec_len = ((sizeof(*this) + 7) / 8);

	pfqos_ext_type = EPfqosExtFlowspec;

	uplink_bandwidth = aParameters.GetUplinkBandwidth();
	uplink_maximum_burst_size = aParameters.GetUpLinkMaximumBurstSize();
	uplink_maximum_packet_size = aParameters.GetUpLinkMaximumPacketSize();
	uplink_average_packet_size = aParameters.GetUpLinkAveragePacketSize();
	uplink_delay = aParameters.GetUpLinkDelay();
	uplink_priority = static_cast< TUint16 >(aParameters.GetUpLinkPriority());
	
	downlink_bandwidth = aParameters.GetDownlinkBandwidth();
	downlink_maximum_burst_size = aParameters.GetDownLinkMaximumBurstSize();
	downlink_maximum_packet_size = aParameters.GetDownLinkMaximumPacketSize();
	downlink_average_packet_size = aParameters.GetDownLinkAveragePacketSize();
	downlink_delay = aParameters.GetDownLinkDelay();
	downlink_priority = static_cast< TUint16 >(aParameters.GetDownLinkPriority());
	
	// name
	name.FillZ();
	if (name.MaxLength() >= aParameters.GetName().Length())
		{
		name.Copy(aParameters.GetName());
		}
	name.ZeroTerminate();

	flags = aParameters.Flags();
	reserved = 0;

	LOG(Log::Printf(_L("1 pfqos_flowspec_len -- [%d]"),pfqos_flowspec_len));
	}


EXPORT_C T_pfqos_event::T_pfqos_event(TUint8 aType, TUint16 aEventType, 
									  TUint16 aValue)
	{
	pfqos_event_len = ((sizeof(*this) + 7) / 8);
	pfqos_ext_type = aType;
	event_type = aEventType;
	event_value = aValue;
	}

EXPORT_C T_pfqos_channel::T_pfqos_channel(TUint32 aChannelId)
	{
	pfqos_channel_len = ((sizeof(*this) + 7) / 8);
	pfqos_ext_type = EPfqosExtChannel;
	channel_id= aChannelId;
	};

EXPORT_C T_pfqos_configure::T_pfqos_configure(TUint16 aProtocolId)
	{
	pfqos_configure_len = ((sizeof(*this) + 7) / 8);
	pfqos_ext_type = EPfqosExtConfigure;
	protocol_id = aProtocolId;
	reserved = 0;
	}


//
// Internal representation of PfqosMessage
//
EXPORT_C TPfqosBase::TPfqosBase() : iMsg(0)
	{
	}

EXPORT_C TUint TPfqosBase::Length() const
	{
	return iMsg ? sizeof(*iMsg) : 0;
	}

EXPORT_C TInt TPfqosBase::ByteStream(RMBufChain &aPacket, TInt aTotal) const
	{
	if (iMsg)
		{
		struct pfqos_msg base = *iMsg;
		base.pfqos_msg_len = (TUint16)(aTotal / 8);
		aPacket.CopyIn(TPtrC8((TUint8 *)&base, sizeof(base)), 0);
		return Length();
		}
	else
		{
		return 0;
		}
	}


EXPORT_C TPfqosSelector::TPfqosSelector() : iExt(0)
	{
	}

EXPORT_C TUint TPfqosSelector::Length() const
	{
	return iExt ? sizeof(*iExt) : 0;
	}

EXPORT_C TInt TPfqosSelector::ByteStream(RMBufChain &aPacket, TInt aOffset) 
	const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}

EXPORT_C TInt TPfqosSelector::GetUid(TCheckedUid& aUid)
	{
	if (!iExt)
		{
		return KErrNotReady;
		}
	TUidType uid_type(TUid::Uid(iExt->uid1), TUid::Uid(iExt->uid2), 
					  TUid::Uid(iExt->uid3));
	aUid.Set(uid_type);
	return KErrNone;
	};

EXPORT_C TPfqosAddress::TPfqosAddress() : iExt(0), iAddr(0), iPrefix(0)
	{
	}

EXPORT_C TUint TPfqosAddress::Length() const
	{
	return iExt ? ((sizeof(*iExt) + sizeof(TInetAddr) + 
		sizeof(TInetAddr) + 7) / 8) * 8 : 0;
	}

EXPORT_C TInt TPfqosAddress::ByteStream(RMBufChain &aPacket, TInt aOffset) 
	const
	{
	if (iExt && iAddr && iPrefix)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(TPtrC8((TUint8 *)iAddr, sizeof(*iAddr)), aOffset + 
			sizeof(*iExt));
		aPacket.CopyIn(TPtrC8((TUint8 *)iPrefix, sizeof(*iPrefix)), aOffset + 
			sizeof(*iExt) + sizeof(*iAddr));
		aOffset += Length();
		}
	return aOffset;
	}



EXPORT_C TPfqosModule::TPfqosModule() : iExt(0), iData(0,0)
	{
	}

EXPORT_C TUint TPfqosModule::Length() const
	{
	return iExt ? (sizeof(*iExt)+iData.Length()) : iData.Length();
	}

EXPORT_C TInt TPfqosModule::ByteStream(RMBufChain &aPacket, TInt aOffset) 
	const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += sizeof(*iExt);
		}
	if (iData.Length() > 0)
		{
		aPacket.CopyIn(iData, aOffset);
		aOffset += iData.Length();
		}
	return aOffset;
	}



EXPORT_C TPfqosFlowSpec::TPfqosFlowSpec() : iExt(0)
	{
	}

EXPORT_C TUint TPfqosFlowSpec::Length() const
	{
	return iExt ? sizeof(*iExt) : 0;
	}

EXPORT_C TInt TPfqosFlowSpec::ByteStream(RMBufChain &aPacket, TInt aOffset) 
	const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}



EXPORT_C TPfqosEvent::TPfqosEvent() : iExt(0)
	{
	}

EXPORT_C TUint TPfqosEvent::Length() const
	{
	return iExt ? sizeof(*iExt) : 0;
	}

EXPORT_C TInt TPfqosEvent::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}


EXPORT_C TPfqosChannel::TPfqosChannel() : iExt(0)
	{
	}

EXPORT_C TUint TPfqosChannel::Length() const
	{
	return iExt ? sizeof(*iExt) : 0;
	}

EXPORT_C TInt TPfqosChannel::ByteStream(RMBufChain &aPacket, TInt aOffset) 
	const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}


EXPORT_C TPfqosConfigure::TPfqosConfigure() : iExt(0)
	{
	}

EXPORT_C TUint TPfqosConfigure::Length() const
	{
	return iExt ? sizeof(*iExt) : 0;
	}

EXPORT_C TInt TPfqosConfigure::ByteStream(RMBufChain &aPacket, TInt aOffset) 
	const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}

EXPORT_C TPfqosConfigFile::TPfqosConfigFile() : iExt(0)
	{
	}

EXPORT_C TUint TPfqosConfigFile::Length() const
	{
	return iExt ? sizeof(*iExt) : 0;
	}

EXPORT_C TInt TPfqosConfigFile::ByteStream(RMBufChain &aPacket, TInt aOffset)
	const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}


EXPORT_C CPfqosPolicyData::CPfqosPolicyData(TInt aType) : iBufPtr(0,0)
	{
	iType = aType;
	iBuf = NULL;
	}


EXPORT_C CPfqosPolicyData::~CPfqosPolicyData()
	{
	delete iBuf;
	}



EXPORT_C void CPfqosPolicyData::ConstructL()
	{
	
	}


EXPORT_C CPfqosPolicyData* CPfqosPolicyData::NewL(TInt aType)
	{

	CPfqosPolicyData* pfqosPolicyData = new (ELeave) CPfqosPolicyData(aType);
	CleanupStack::PushL(pfqosPolicyData);
	pfqosPolicyData->ConstructL();
	CleanupStack::Pop();
	return pfqosPolicyData;
	
	}


EXPORT_C CPfqosPolicyData* CPfqosPolicyData::NewL(TInt aType, 
	const TUint8 *aBuf,TInt aLength)
	{
	CPfqosPolicyData* pfqosPolicyData = new (ELeave) CPfqosPolicyData(aType);
	CleanupStack::PushL(pfqosPolicyData);
	pfqosPolicyData->ConstructL();
	pfqosPolicyData->CopyL(aBuf, aLength);
	CleanupStack::Pop();
	return pfqosPolicyData;
	}


EXPORT_C TUint CPfqosPolicyData::Length() const
	{
	return iBufPtr.Length();
	}

EXPORT_C TInt CPfqosPolicyData::Size() const
	{
	return iBufPtr.MaxLength();
	}

EXPORT_C void CPfqosPolicyData::CopyL(const TUint8 *aBuf,TInt aLength)
	{
	if (iBuf)
		{
		delete iBuf;
		iBuf=NULL;
		}

	iBuf = HBufC8::NewL(aLength);
	TPtr8 tmp(iBuf->Des());
	iBufPtr.Set(tmp);
	iBufPtr.Copy(aBuf, aLength);
	}

EXPORT_C TDesC8& CPfqosPolicyData::Data()
	{
	return iBufPtr;
	}

EXPORT_C TInt CPfqosPolicyData::Type()
	{
	return iType;
	}

EXPORT_C TInt CPfqosPolicyData::ByteStream(RMBufChain &aPacket, TInt aOffset)
	const
	{
	if (iBufPtr.Length())
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iBufPtr.Ptr(), iBufPtr.Length()), 
			aOffset);
		aOffset += Length();
		}
	return aOffset;
	}


EXPORT_C TPfqosMessage::TPfqosMessage() 
	: iError(0),
	  iNumModules(0),
	  iModuleList(_FOFF(TPfqosModule, iNext)),
	  iNumExtensions(0), 
	  iExtensions(_FOFF(CPfqosPolicyData, iNext))
	{
	}

EXPORT_C void TPfqosMessage::Add(TPfqosModule& aModule)
	{
	iNumModules++;
	iModuleList.AddLast(aModule);
	}

EXPORT_C void TPfqosMessage::AddExtension(CPfqosPolicyData& aExtension)
	{
	iNumExtensions++;
	iExtensions.AddLast(aExtension);
	}

//
// Internal representation of PF_QOS message.
//
EXPORT_C TPfqosMessage::TPfqosMessage(const TDesC8& aMsg)
	: iError(KErrArgument),
	  iNumModules(0),
	  iModuleList(_FOFF(TPfqosModule, iNext)),
	  iNumExtensions(0),
	  iExtensions(_FOFF(CPfqosPolicyData, iNext))
	{
	const TUint8 *p = aMsg.Ptr();
	TInt length = aMsg.Length();

	if (length < (TInt)sizeof(pfqos_msg))
		{
		LOG(Log::Printf(_L("length < (TInt)sizeof(pfqos_msg) -- [%d,%d]"),
			length,(TInt)sizeof(pfqos_msg)));

		return;		// EMSGSIZE (impossible message size)
		}
	//
	// Base Message Header
	//lint -e{826}	Typecast below is ok.
	iBase.iMsg = (struct pfqos_msg *)p;
	if (iBase.iMsg->pfqos_msg_version != KPfqosMsgV1)
		{
		LOG(Log::Printf(_L("iBase.iMsg->pfqos_msg_version != KPfqosMsgV1 -- [%d,%d]"),
			iBase.iMsg->pfqos_msg_version,KPfqosMsgV1));

		return;		// EINVAL
		}
	if (iBase.iMsg->pfqos_msg_len * 8 != length)
		{
		LOG(Log::Printf(_L("iBase.iMsg->pfqos_msg_len * 8 != length -- [%d,%d]"),
			iBase.iMsg->pfqos_msg_len * 8,length));
		return;		// EMSGSIZE (incorrect message length)
		}
	if (iBase.iMsg->pfqos_msg_reserved)
		{
		LOG(Log::Printf(_L("iBase.iMsg->pfqos_msg_reserved -- [%d]"),
			iBase.iMsg->pfqos_msg_reserved));
		return;		// EINVAL (unused parts must be zeroed)
		}
	p += sizeof(struct pfqos_msg);
	length -= sizeof(struct pfqos_msg);

	LOG(Log::Printf(_L("**** length -- [%d]"),length));
	LOG(Log::HexDump(_S("**** p"), _S(":  "), p, 8 ));

	//
	// Extension headers
	// Some general rules:
	// - only one instance of an extension type is valid
	//
	while (length > 0)
		{
		//lint -e{826}	Typecast below is ok.
		struct pfqos_ext *ext = (struct pfqos_ext *)p;
		int ext_len = ext->pfqos_ext_len;

		if (ext_len < 1)
			{
			LOG(Log::Printf(_L("ext_len < 1 -- [%d]"),ext_len));

			return;		// EINVAL (bad message format)
			}
		ext_len *= 8;

		if (ext_len > length)
			{
			LOG(Log::Printf(_L("ext_len > length -- [%d, %d]"),ext_len, length));
			LOG(Log::Printf(_L("calculated sizeof(pfqos_msg): %d"), 
				sizeof(struct pfqos_msg)));
			LOG(Log::HexDump(_S("pfqos_msg "), _S("pfqos_msg "), aMsg.Ptr(), 
				sizeof(struct pfqos_msg)));
			LOG(Log::HexDump(_S("p"), _S(":  "), p, 8 ));

			return;		// EINVAL
			}
		switch (ext->pfqos_ext_type)
			{
		case EPfqosExtReserved:
			{
			LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtReserved")));
			return;		// EINVAL (bad mesage format)
			}

		case EPfqosExtSrcAddress:
			if (iSrcAddr.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtSrcAddress -1")));
			
				return;
				}
			if (ext_len != sizeof(struct pfqos_address) + sizeof(TInetAddr) + 
				sizeof(TInetAddr))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtSrcAddress -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iSrcAddr.iExt = (struct pfqos_address *)p;
			//lint -e{826}	Typecast below is ok.
			iSrcAddr.iAddr = (TInetAddr *)(p + sizeof(struct pfqos_address));
			//lint -e{826}	Typecast below is ok.
			iSrcAddr.iPrefix = (TInetAddr *)(p + sizeof(struct pfqos_address) 
				+ sizeof(TInetAddr));
			break;

		case EPfqosExtDstAddress:
			if (iDstAddr.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtDstAddress -1")));
				return;
				}
			if (ext_len != sizeof(struct pfqos_address) + sizeof(TInetAddr) + 
				sizeof(TInetAddr))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtDstAddress -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iDstAddr.iExt = (struct pfqos_address *)p;
			//lint -e{826}	Typecast below is ok.
			iDstAddr.iAddr = (TInetAddr *)(p + sizeof(struct pfqos_address));
			//lint -e{826}	Typecast below is ok.
			iDstAddr.iPrefix = (TInetAddr *)(p + sizeof(struct pfqos_address) 
				+ sizeof(TInetAddr));
			break;

		case EPfqosExtModulespec:
			{
			TPfqosModule *module = new TPfqosModule();
			//lint -e{774}	new can return NULL!
			if (module == NULL)
				{
				iError = KErrNoMemory;
				LOG(Log::Printf(_L("TPfqosMessage::TPfqosMessage() iError -- [%d] -2"), iError));
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtModulespec")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			module->iExt = (struct pfqos_modulespec *)p;
			if (ext_len - sizeof(struct pfqos_modulespec) > 0)
				{
				module->iData.Set((TUint8 *)(p+sizeof(struct 
					 pfqos_modulespec)), 
					(ext_len - sizeof(struct pfqos_modulespec)));
				}
			//lint --e{429} Lint does not understand the Add semantics below!
			Add(*module);
			}
			break;

		case EPfqosExtSelector:
			if (iSelector.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtSelector -1")));
				return;
				}
			if (ext_len != sizeof(pfqos_selector))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtSelector -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iSelector.iExt = (struct pfqos_selector *)p;
			break;

		case EPfqosExtFlowspec:
			if (iFlowSpec.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtFlowspec -1")));
				return;
				}
			if (ext_len != sizeof(pfqos_flowspec))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtFlowspec -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iFlowSpec.iExt = (struct pfqos_flowspec *)p;
			break;

		case EPfqosExtEvent:
			if (iEvent.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtEvent -1")));
				return;
				}
			if (ext_len != sizeof(pfqos_event))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtEvent -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iEvent.iExt = (struct pfqos_event *)p;
			break;

		case EPfqosExtChannel:
			if (iChannel.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtChannel -1")));
				return;
				}
			if (ext_len != sizeof(pfqos_channel))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtChannel -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iChannel.iExt = (struct pfqos_channel *)p;
			break;

		case EPfqosExtExtension:
			{
			//lint -e{826}	Typecast below is ok.
			pfqos_extension *extensionType = (pfqos_extension *) 
				(p+sizeof(pfqos_configure));
			TPtr8 ptr((TUint8*)ext, ext_len, ext_len);
			TRAPD(err, AddExtensionL(ptr, 
				extensionType->pfqos_extension_type));
			if (err != KErrNone)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtExtension -2")));
				return;
				}
			}
			break;

		case EPfqosExtConfigFile:
			if (iConfigFile.iExt)
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtConfigFile -1")));
				return;
				}
			if (ext_len != sizeof(pfqos_config_file))
				{
				LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtConfigFile -2")));
				return;
				}
			//lint -e{826}	Typecast below is ok.
			iConfigFile.iExt = (struct pfqos_config_file *)p;
			break;

		default:
			// Unknown extensions must be ignored, not an error!
			break;
			}
		p += ext_len;
		length -= ext_len;
		}
	if (length != 0)
		{
		LOG(Log::Printf(_L("ext->pfqos_ext_type : EPfqosExtConfigFile -2")));
		return;
		}

#ifdef _LOG
	// Message unpacked successfully

	Log::Printf(_L("Following message unpacked successfully -- [%d]"),
		iBase.iMsg->pfqos_msg_type);
		
	switch(iBase.iMsg->pfqos_msg_type)
		{
	case 0: 
		Log::Printf(_L("EPfqosReserved"));
		break;
	case 1: 
		Log::Printf(_L("EPfqosUpdate"));
		break;			
	case 2: 
		Log::Printf(_L("EPfqosAdd"));
		break;
	case 3: 
		Log::Printf(_L("EPfqosDelete"));
		break;			
	case 4: 
		Log::Printf(_L("EPfqosGet"));
		break;
	case 5: 
		Log::Printf(_L("EPfqosFlush"));
		break;			
	case 6: 
		Log::Printf(_L("EPfqosDump"));
		break;
	case 7: 
		Log::Printf(_L("EPfqosEvent"));
		break;			
	case 8: 
		Log::Printf(_L("EPfqosConfigure"));
		break;
	case 9: 
		Log::Printf(_L("EPfqosReject"));
		break;			
	case 10: 
		Log::Printf(_L("EPfqosJoin"));
		break;
	case 11: 
		Log::Printf(_L("EPfqosLeave"));
		break;			
	case 12: 
		Log::Printf(_L("EPfqosCreateChannel"));
		break;
	case 13: 
		Log::Printf(_L("EPfqosOpenExistingChannel"));
		break;			
	case 14: 
		Log::Printf(_L("EPfqosDeleteChannel"));
		break;
	case 15: 
		Log::Printf(_L("EPfqosConfigChannel"));
		break;			
	case 16: 
		Log::Printf(_L("EPfqosLoadFile"));
		break;
	case 17: 
		Log::Printf(_L("EPfqosUnloadFile"));
		break;
	default:
		Log::Printf(_L("pfqos_msg_type = %d"), (TInt)iBase.iMsg->pfqos_msg_type);
		break;			
		}
#endif		

	iError = KErrNone;
	}


EXPORT_C TUint16 TPfqosMessage::Length64()
	{
	return (TUint16)((
			 iBase.Length() +
			 iEvent.Length() +
			 iChannel.Length() +
			 iSelector.Length() +
			 iSrcAddr.Length() +
			 iDstAddr.Length() +
			 iFlowSpec.Length() +
			 ModuleLength() + 
			 iConfigFile.Length() +
			 ExtensionLength())/8);
	}


EXPORT_C TUint TPfqosMessage::ExtensionLength()
	{
	TSglQueIter<CPfqosPolicyData> iter(iExtensions);
	CPfqosPolicyData *item;
	TUint length=0;

	while ((item = iter++) != NULL)
		{
		length += item->Length();
		}
	return length;
	}


EXPORT_C TUint TPfqosMessage::ModuleLength()
	{
	TSglQueIter<TPfqosModule> iter(iModuleList);
	TPfqosModule* item;
	TUint length=0;

	while ((item = iter++) != NULL)
		{
		length += item->Length();
		}
	return length;
	}


EXPORT_C void TPfqosMessage::ByteStreamL(RMBufChain &aPacket)
	{
	TInt totlen = Length64() * 8;

	// (Any possible previous content is lost)
	aPacket.AppendL(totlen);
	TInt offset = iBase.ByteStream(aPacket, totlen);

	// Always make sure that all possible extension fields in are included!
	offset = iSelector.ByteStream(aPacket, offset);
	offset = iSrcAddr.ByteStream(aPacket, offset);
	offset = iDstAddr.ByteStream(aPacket, offset);
	offset = iFlowSpec.ByteStream(aPacket, offset);
	offset = iEvent.ByteStream(aPacket, offset);
	offset = iChannel.ByteStream(aPacket, offset);
	offset = iConfigFile.ByteStream(aPacket, offset);

	TSglQueIter<TPfqosModule> iterM(iModuleList);
	TPfqosModule *itemM;

	while ((itemM = iterM++) != NULL)
		{
		offset = itemM->ByteStream(aPacket, offset);
		}

	TSglQueIter<CPfqosPolicyData> iter(iExtensions);
	CPfqosPolicyData *item;

	while ((item = iter++) != NULL)
		{
		offset = item->ByteStream(aPacket, offset);
		}
	}


EXPORT_C TPfqosMessage::~TPfqosMessage()
	{
	TSglQueIter<TPfqosModule> iter(iModuleList);
	TPfqosModule *item;

	//
	// Delete modulelist
	//
	while ((item = iter++) != NULL)
		{
		iModuleList.Remove(*item);
		delete item;
		}

	//
	// Delete extension list
	//
	TSglQueIter<CPfqosPolicyData> i(iExtensions);
	CPfqosPolicyData *ext;

	while ((ext = i++) != NULL)
		{
		iExtensions.Remove(*ext);
		delete ext;
		}
	}

EXPORT_C void TPfqosMessage::AddModuleL(T_pfqos_module& aModule, 
	const TDesC8& aConfigData)
	{
	TPfqosModule* item = new (ELeave) TPfqosModule();
	item->iExt = &aModule;
	item->iData.Set(aConfigData);
	//lint --e{429} Lint does not understand the Add semantics below!
	Add(*item);
	}

EXPORT_C void TPfqosMessage::AddExtensionL(const TDesC8& aExtension, 
	TInt aType)
	{
	CPfqosPolicyData* data = CPfqosPolicyData::NewL(aType,aExtension.Ptr(), 
		aExtension.Length());
	AddExtension(*data);
	}


EXPORT_C TInt TPfqosMessage::SetQoSParameters(TQoSParameters& aParameters) 
	const
	{
	if (!iFlowSpec.iExt)
		{
		return KErrNotFound;
		}

	// uplink
	aParameters.SetUpLinkDelay(iFlowSpec.iExt->uplink_delay);
	aParameters.SetUpLinkMaximumPacketSize(
		iFlowSpec.iExt->uplink_maximum_packet_size);
	aParameters.SetUpLinkAveragePacketSize(
		iFlowSpec.iExt->uplink_average_packet_size);
	aParameters.SetUpLinkPriority(iFlowSpec.iExt->uplink_priority);
	aParameters.SetUpLinkMaximumBurstSize(
		iFlowSpec.iExt->uplink_maximum_burst_size);
	aParameters.SetUplinkBandwidth(iFlowSpec.iExt->uplink_bandwidth);

	// downlink
	aParameters.SetDownLinkDelay(iFlowSpec.iExt->downlink_delay);
	aParameters.SetDownLinkMaximumPacketSize(
		iFlowSpec.iExt->downlink_maximum_packet_size);
	aParameters.SetDownLinkAveragePacketSize(
		iFlowSpec.iExt->downlink_average_packet_size);
	aParameters.SetDownLinkPriority(iFlowSpec.iExt->downlink_priority);
	aParameters.SetDownLinkMaximumBurstSize(
		iFlowSpec.iExt->downlink_maximum_burst_size);
	aParameters.SetDownlinkBandwidth(iFlowSpec.iExt->downlink_bandwidth);

	// flags
	aParameters.SetFlags(iFlowSpec.iExt->flags);

	// names
	aParameters.SetName(iFlowSpec.iExt->name);

	return KErrNone;
	}


// Remove all policy data (flowspec, modules, extensions, event, configure)
EXPORT_C void TPfqosMessage::RemovePolicyData()
	{
	TSglQueIter<TPfqosModule> iter(iModuleList);
	TPfqosModule *item;

	//
	// Delete modulelist
	//
	while ((item = iter++) != NULL)
		{
		iModuleList.Remove(*item);
		delete item;
		}
	iNumModules = 0;

	//
	// Delete extension list
	//
	TSglQueIter<CPfqosPolicyData> i(iExtensions);
	CPfqosPolicyData *ext;

	while ((ext = i++) != NULL)
		{
		iExtensions.Remove(*ext);
		delete ext;
		}
	iNumExtensions = 0;

	iFlowSpec.iExt = NULL;
	iEvent.iExt = NULL;
	iChannel.iExt = NULL;
	iConfigure.iExt = NULL;
	}

