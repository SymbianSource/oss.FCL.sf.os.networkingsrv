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
// Tunnel Common Code and Data
// This file contains code and data that the Tunnel CFProtocol and MCPr need to share,
// for example TTunnelAddressMsgSig.
// 
//

/**
 @file
 @internalComponent
*/

#include "TunnelAgentHandler.h"
#include "tunnelProvision.h"
#include <comms-infras/ss_thread.h>
#include <ss_glob.h>
#include <in_sock.h>

using namespace ESock;
using namespace Elements;
using namespace Messages;

enum
    {
    ETunnelSetAddressSig = 1,
    };

//
// Attribute table for TTunnelAddressMsgSig
//

START_ATTRIBUTE_TABLE(TSigTunnelAddressParams, TTunnelAgentMessage::ERealmId, ETunnelSetAddressSig)
  REGISTER_ATTRIBUTE(TSigTunnelAddressParams, iAddress, TMeta<TInetAddr>)
  REGISTER_ATTRIBUTE(TSigTunnelAddressParams, iNameSer1, TMeta<TInetAddr>)
  REGISTER_ATTRIBUTE(TSigTunnelAddressParams, iNameSer2, TMeta<TInetAddr>)
  REGISTER_ATTRIBUTE(TSigTunnelAddressParams, iIsUpdate, TMetaNumber)
END_ATTRIBUTE_TABLE_BASE( TSignatureBase, ESignatureBase )

DEFINE_MVIP_CTR( TSigTunnelAddressParams )

const TImplementationProxy tunnelMessageImplTable[] =
/**
Virtual constructor table for Tunnel messages.
*/
	{
	MVIP_CTR_ENTRY(ETunnelSetAddressSig, TSigTunnelAddressParams)
	};

void TTunnelMessages::RegisterL()
/**
Register the virtual constructor table
*/
	{
	TlsGlobals::Get().RegisterInterfaceL(TUid::Uid(TTunnelAgentMessage::ERealmId), sizeof(tunnelMessageImplTable) / sizeof(tunnelMessageImplTable[0]), tunnelMessageImplTable);
	}

void TTunnelMessages::DeRegister()
/**
DeRegister the virtual constructor table
*/
	{
	TlsGlobals::Get().DeregisterInterface(TUid::Uid(TTunnelProvision::EUid));
	}


