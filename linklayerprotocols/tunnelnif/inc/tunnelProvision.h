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
// Definitions of Tunnel Provisioning classes.
// 
//

/**
 @file 
 @internalComponent
*/

#if (!defined TUNNELPROVISION_H__)
#define TUNNELPROVISION_H__

#include <e32cmn.h>
#include <comms-infras/metadata.h>
#include <in_iface.h>

namespace ESock
	{
	class CCommsDatIapView;
	}
	
struct TTunnelInfo
/**
Provisioning class embedded within the main TTunnelProvision class
*/
	{
public:
	// Rationale: iIfName is effectively TBuf<32> and not an RBuf because Tunnel interface
	// names are typically 26-28 characters.
	TInterfaceName iIfName;
	TUint32 iLocalIap;
	TUint32 iLocalNetwork;
	};

class TTunnelProvision : public Meta::SMetaData
/**
Structure passed to CFProtocol in a ProvisionConfig() message.
*/
	{
public:
	enum
		{
		EUid = 0x10281E02,		// same as Tunnel Flow UID (arbitrary convention only)
		ETypeId = 1 
		};

public:
	void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);

public:
	TTunnelInfo iInfo;

	// No serialisation takes place in CFProtocol - we only cast an SMetaData* to
	// TTunnelProvision* hence DATA_VTABLE is enough.
	DATA_VTABLE
	};

class TTunnelMessages
/**
Methods used to register the Tunnel message virtual constructors
*/
	{
public:
	static void RegisterL();
	static void DeRegister();
	};

#endif // TUNNELPROVISION_H__
