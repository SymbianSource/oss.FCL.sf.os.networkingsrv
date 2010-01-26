// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file provides the public interface into the Networking UPS Component.
// @internalAll
// @released
// 
//

#ifndef NETUPS_H
#define NETUPS_H

#include <e32def.h>		  								// defines TInt
#include <e32std.h>										// defines ProcessId, ThreadId, Dll
#include <e32des8.h>									// definition for descriptors

#include <ups/upstypes.h> 								// defines TServiceId

#include <elements/nm_address.h> 					 	// defines TNodeId

#include <comms-infras/netupstypes.h>					// defines MPolicyCheckRequestOriginator

#include <comms-infras/commsdebugutility.h> 			// defines the comms debug logging utility

namespace NetUps
{

class CNetUpsImpl;
	
class CNetUps : public CBase
	{
public:
	IMPORT_C static CNetUps* NewL(TInt32 aServiceId);
	virtual ~CNetUps();
	
	IMPORT_C static TBool UpsDisabled(TInt32 aServiceId);

	IMPORT_C void ProcessPolicyCheckRequestL(TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId& aRequestId);
	IMPORT_C TInt CancelRequest(const Messages::TNodeId&  aCallersNodeId, const TRequestId& aRequestId);
	IMPORT_C void IncrementConnectionCountL(const Messages::TNodeId&  aCallersNodeId); 
	IMPORT_C void DecrementConnectionCountL(const Messages::TNodeId&  aCallersNodeId);
	IMPORT_C void DecrementConnectionCountL(const Messages::TNodeId&  aCallersNodeId, TProcessId& aProcessId, TThreadId& aThreadId);
private:
	virtual void ConstructL(TInt32 aServiceId);
	CNetUps(TInt32 aServiceId);
private:
	CNetUps(const CNetUps&);
	void operator=(const CNetUps&);
	bool operator==(const CNetUps&);		
private:
	TInt32 iServiceId;
	CNetUpsImpl* iNetUps;		
	__FLOG_DECLARATION_MEMBER;		
	};

} // end namespace NetUps

#endif // NETUPS_H
