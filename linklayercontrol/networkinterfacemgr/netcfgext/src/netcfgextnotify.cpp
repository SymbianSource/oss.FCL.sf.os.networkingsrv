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
//

#include <comms-infras/ss_mmnode.h>
#include "netcfgextprov.h"
#include "netcfgextnotify.h"
#include <commsdattypeinfov1_1.h>

using namespace Elements;
using namespace Messages;
using namespace ESock;

_LIT(KIAPId, "IAP\\Id");
_LIT(KIAPNetwork, "IAP\\IAPNetwork");

EXPORT_C CNetCfgExtNotify* CNetCfgExtNotify::NewL(CSubConnectionProviderBase* aScpr)
	{
	return new(ELeave) CNetCfgExtNotify(aScpr);
	}

CNetCfgExtNotify::CNetCfgExtNotify(CSubConnectionProviderBase* aScpr)
	: iScpr(aScpr)
	{
	}

CNetCfgExtNotify::~CNetCfgExtNotify()
    {
    iScpr = NULL;
    }
void CNetCfgExtNotify::IfProgress(TInt aStage, TInt aError)
	{
	TStateChange change(aStage, aError);
	if(iScpr)
	    {
	RClientInterface::OpenPostMessageClose(iScpr->Id(), iScpr->Id(),
		TCFMessage::TStateChange(change).CRef());
	    }
	}

TInt CNetCfgExtNotify::DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* /*aMessage*/)
	{
	const ESock::RMetaExtensionContainerC& apc = iScpr->AccessPointConfig();
	
	const CNetCfgExtProvision* provision = static_cast<const CNetCfgExtProvision*>(apc.FindExtension(
	        STypeId::CreateSTypeId(CNetCfgExtProvision::EUid, CNetCfgExtProvision::ETypeId)));
	if (!provision)
		{
		return KErrArgument;
		}

	if (aField == KIAPId)
		{
		aValue = provision->Iap();
		return KErrNone;
		}
	else if (aField == KIAPNetwork)
		{
		aValue = provision->NetworkId();
		return KErrNone;
		}
	return KErrNotSupported;
	}

TInt CNetCfgExtNotify::DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage)
	{
	TBuf<KCommsDbSvrMaxFieldLength> buf;
	TInt ret = DoReadDes(aField, buf, aMessage);
	aValue.Copy(buf);
	return ret;
	}

TInt CNetCfgExtNotify::DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* /*aMessage*/)
	{
	const ESock::RMetaExtensionContainerC& apc = iScpr->AccessPointConfig();
	
	const CNetCfgExtProvision* provision = static_cast<const CNetCfgExtProvision*>(apc.FindExtension(STypeId::CreateSTypeId(CNetCfgExtProvision::EUid, CNetCfgExtProvision::ETypeId)));
	if (!provision)
		{
		return KErrArgument;
		}
	
	if (aField == TPtrC(KCDTypeNameConfigDaemonName))
		{
		aValue.Copy(provision->ConfigDaemonName());
		return KErrNone;
		}
	if (aField == TPtrC(KCDTypeNameConfigDaemonManagerName))
		{
		aValue.Copy(provision->ConfigDaemonManagerName());
		return KErrNone;
		}
	
	return KErrNotSupported;
	}


/*
  Unsupported MNifIfNotify methods
*/
void CNetCfgExtNotify::LinkLayerDown(TInt /*aReason*/, TAction /*aAction*/)
	{
	// Unsupported
	}

void CNetCfgExtNotify::LinkLayerUp()
	{
	// Unsupported
	}

void CNetCfgExtNotify::NegotiationFailed(CNifIfBase* /*aIf*/, TInt /*aReason*/)
	{
	// Unsupported
	}

TInt CNetCfgExtNotify::Authenticate(TDes& /*aUsername*/, TDes& /*aPassword*/)
	{
	return KErrNotSupported;
	}

void CNetCfgExtNotify::CancelAuthenticate()
	{
	// Unsupported
	}


TInt CNetCfgExtNotify::GetExcessData(TDes8& /*aBuffer*/)
	{
	return KErrNotSupported;
	}
	
void CNetCfgExtNotify::OpenRoute()
	{
	// Unsupported
	}

void CNetCfgExtNotify::CloseRoute()
	{
	// Unsupported
	}


void CNetCfgExtNotify::IfProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
	{
	// Unsupported
	}

TInt CNetCfgExtNotify::Notification(TNifToAgentEventType /*aEvent*/, void* /*aInfo*/)
	{
	return KErrNotSupported;
	}

void CNetCfgExtNotify::BinderLayerDown(CNifIfBase* /*aBinderIf*/, TInt /*aReason*/, TAction /*aAction*/)
	{
	// Unsupported
	}

TInt CNetCfgExtNotify::PacketActivity(TDataTransferDirection /*aDirection*/, TUint /*aBytes*/, TBool /*aResetTimer*/)
	{
	return KErrNotSupported;
	}

void CNetCfgExtNotify::NotifyDataSent(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aUplinkVolume*/)
	{
	// Unsupported
	}

void CNetCfgExtNotify::NotifyDataReceived(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aDownlinkVolume*/)
	{
	// Unsupported
	}

void CNetCfgExtNotify::NifEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource=0*/)
	{
	// Unsupported
	}



TInt CNetCfgExtNotify::DoWriteInt(const TDesC& /*aField*/, TUint32 /*aValue*/,const RMessagePtr2* /*aMessage*/)
	{
	return KErrNotSupported;
	}


TInt CNetCfgExtNotify::DoWriteDes(const TDesC& /*aField*/, const TDesC8& /*aValue*/, const RMessagePtr2* /*aMessage*/)
	{
	return KErrNotSupported;
	}

TInt CNetCfgExtNotify::DoWriteDes(const TDesC& /*aField*/, const TDesC16& /*aValue*/,const RMessagePtr2* /*aMessage*/)
	{
	return KErrNotSupported;
	}

TInt CNetCfgExtNotify::DoReadBool(const TDesC& /*aField*/, TBool& /*aValue*/,const RMessagePtr2* /*aMessage*/)
	{
	return KErrNotSupported;
	}

TInt CNetCfgExtNotify::DoWriteBool(const TDesC& /*aField*/, TBool /*aValue*/,const RMessagePtr2* /*aMessage*/)
	{
	return KErrNotSupported;
	}


