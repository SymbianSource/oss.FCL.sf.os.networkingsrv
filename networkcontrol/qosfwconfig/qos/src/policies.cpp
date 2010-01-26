// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "policies.h"
#include "qoslog.h"

#ifdef _LOG
#include "qos_prot.h"	// for the TLogAddress class!

// Dump selector
void DumpSelector(const CSelectorBase& aSel)
	{
	TLogAddress dst(aSel.iDst);
	TLogAddress dmsk(aSel.iDstMask);
	TLogAddress src(aSel.iSrc);
	TLogAddress smsk(aSel.iSrcMask);
	Log::Printf(_L("\t\tsel(%d): src=%S - %d %S dst=%S - %d %S prot=%d name='%S' iap=%d uid(%d %d %d) pri=%d, owner=%d"),
			aSel.iType,
			&src, (TInt)aSel.iSrcPortMax, &smsk,
			&dst, (TInt)aSel.iDstPortMax, &dmsk,
			(TInt)aSel.iProtocol, &aSel.iName,
			aSel.iIapId,
			aSel.iUid.UidType()[0].iUid, aSel.iUid.UidType()[1].iUid, aSel.iUid.UidType()[2].iUid,
			aSel.iPriority, aSel.iOwner);
	}
#endif

//
// CPolicySelector
//
CPolicySelector::CPolicySelector(TPfqosMessage& aMsg) :
  CSelectorBase(aMsg.iBase, aMsg.iSelector, aMsg.iSrcAddr, aMsg.iDstAddr, 
				EPfqosFlowspecPolicy)
	{
	iType = EPfqosFlowspecPolicy;
	LOG(Log::Printf(_L("new\tqos selector[%u] EPfqosFlowspecPolicy(%d) size=%d"), (TInt)this, iType, sizeof(CPolicySelector)));
	}

CPolicySelector::CPolicySelector(CSelectorBase& aSel) 
	: CSelectorBase(aSel)
	{
	iType = EPfqosFlowspecPolicy;
	LOG(Log::Printf(_L("new\tqos selector[%u] EPfqosFlowspecPolicy(%d) size=%d"), (TInt)this, iType, sizeof(CPolicySelector)));
	}

CPolicySelector::~CPolicySelector()
	{
	LOG(Log::Printf(_L("~\tqos selector[%u] EPfqosFlowspecPolicy(%d) -- done"), (TInt)this, iType));
	}

void CPolicySelector::SetQoSParameters(const TPfqosFlowSpec& aSpec)
	{
	if (!aSpec.iExt)
		{
		return;
		}

	// Uplink
	iQoS.SetUpLinkDelay(aSpec.iExt->uplink_delay);
	iQoS.SetUpLinkMaximumPacketSize(aSpec.iExt->uplink_maximum_packet_size);
	iQoS.SetUpLinkAveragePacketSize(aSpec.iExt->uplink_average_packet_size);
	iQoS.SetUpLinkPriority(aSpec.iExt->uplink_priority);
	iQoS.SetUpLinkMaximumBurstSize(aSpec.iExt->uplink_maximum_burst_size);
	iQoS.SetUplinkBandwidth(aSpec.iExt->uplink_bandwidth);
	iQoS.SetName(aSpec.iExt->name);

	// Downlink
	iQoS.SetDownLinkDelay(aSpec.iExt->downlink_delay);
	iQoS.SetDownLinkMaximumPacketSize(
		aSpec.iExt->downlink_maximum_packet_size);
	iQoS.SetDownLinkAveragePacketSize(
		aSpec.iExt->downlink_average_packet_size);
	iQoS.SetDownLinkPriority(aSpec.iExt->downlink_priority);
	iQoS.SetDownLinkMaximumBurstSize(aSpec.iExt->downlink_maximum_burst_size);
	iQoS.SetDownlinkBandwidth(aSpec.iExt->downlink_bandwidth);

	// Flags
	iQoS.SetFlags(aSpec.iExt->flags);
	}


//
// CModuleSelector
//
CModuleSelector::CModuleSelector(TPfqosMessage& aMsg) 
	: CSelectorBase(aMsg.iBase, aMsg.iSelector, aMsg.iSrcAddr, aMsg.iDstAddr, 
					EPfqosModulespecPolicy), 
	  iModules(_FOFF(CModuleSpec, iDLink))
	{
	iType = EPfqosModulespecPolicy;
	LOG(Log::Printf(_L("new\tqos selector[%u] EPfqosModulespecPolicy(%d) size=%d"), (TInt)this, iType, sizeof(CPolicySelector)));
	}


CModuleSelector::CModuleSelector(CSelectorBase& aSel) 
	: CSelectorBase(aSel), iModules(_FOFF(CModuleSpec, iDLink))
	{
	iType = EPfqosModulespecPolicy;
	LOG(Log::Printf(_L("new\tqos selector[%u] EPfqosModulespecPolicy(%d) size=%d"), (TInt)this, iType, sizeof(CPolicySelector)));
	}


CModuleSelector::~CModuleSelector()
	{
	LOG(Log::Printf(_L("~\tqos selector[%u] EPfqosModulespecPolicy(%d) - destruct start"), (TInt)this, iType));
	TDblQueIter<CModuleSpec> iter(iModules);
	CModuleSpec* sel;

	while ((sel = iter++) != NULL)
		{
		sel->iDLink.Deque();
		delete sel;
		}
	LOG(Log::Printf(_L("~\tqos selector[%u] EPfqosModulespecPolicy(%d) - destruct complete"), (TInt)this, iType));
	}

void CModuleSelector::AddModuleSpec(CModuleSpec& aModule)
	{
	iModules.AddLast(aModule);
	}

void CModuleSelector::RemoveModuleSpec(CModuleSpec* aModule)
	{
	aModule->iDLink.Deque();
	delete aModule;
	}

CModuleSpec* CModuleSelector::FindModuleSpec(TUint aProtocolId)
	{
	TDblQueIter<CModuleSpec> iter(iModules);
	CModuleSpec* sel;

	while ((sel = iter++) != NULL)
		{
		if (sel->ProtocolId() == aProtocolId)
			{
			return sel;
			}
		}
	return NULL;
	}

//
// CModuleSpec
//
CModuleSpec* CModuleSpec::NewL(const TDesC& aFileName, 
							   const TDesC& aModuleName)
	{
	CModuleSpec* moduleSpec = new (ELeave) CModuleSpec();
	CleanupStack::PushL(moduleSpec);
	moduleSpec->ContructL(aFileName, aModuleName);
	CleanupStack::Pop();
	return moduleSpec;
	}

CModuleSpec* CModuleSpec::NewL(const TPfqosModule& aModule)
	{
	CModuleSpec* moduleSpec = new (ELeave) CModuleSpec();
	CleanupStack::PushL(moduleSpec);
	moduleSpec->ContructL(aModule);
	CleanupStack::Pop();
	return moduleSpec;
	}

void CModuleSpec::ContructL(const TPfqosModule& aModule)
	{
	iExtension = CExtension::NewL();
	iExtension->CopyL(aModule.iData);
	TPtrC8 fileName((TUint8*)aModule.iExt->path);
	iFileName.Copy(fileName);
	TPtrC8 moduleName((TUint8*)aModule.iExt->name);
	iName.Copy(moduleName);
	iProtocolId = aModule.iExt->protocol_id;
	iFlags = aModule.iExt->flags;
	}

void CModuleSpec::ContructL(const TDesC& aFileName, const TDesC& aModuleName)
	{
	iExtension = CExtension::NewL();
	iFileName.Copy(aFileName);
	iName.Copy(aModuleName);
	iProtocolId = 0;
	iFlags = 0;
	}

CModuleSpec::CModuleSpec()
	{
	LOG(Log::Printf(_L("new\tqos ModuleSpec[%u] size=%d"), (TInt)this, sizeof(CModuleSpec)));
	iProtocolId = 0;
	iFlags = 0;
	iFileName.FillZ();
	iName.FillZ();
	}


CModuleSpec::~CModuleSpec()
	{
	delete iExtension;
	LOG(Log::Printf(_L("~\tqos ModuleSpec[%u] deleted"), (TInt)this));
	}
