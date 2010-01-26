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
// This file provides the internal interface into the CNetUpsComponent.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSIMPL_H
#define NETUPSIMPL_H

#include <e32def.h>  									// definition for TUint
#include <e32base.h> 									// definition for CBase
#include <e32std.h>										// definition for TProcessId, TThreadId
#include <e32des8.h>									// definition for descriptors

#include <comms-infras/commsdebugutility.h> 			// defines the comms debug logging utility
#include <comms-infras/ss_activities.h>
#include <comms-infras/ss_nodemessages.h> 				// definition access point config extension

#include <ups/upsclient.h>								// definition for Ups Session

#include "netupstypes.h"					// definition for MPolicyCheckRequestOriginator
#include "netupsstatedef.h"								// definition for different net ups session states
//#include "netupsqueuedrequestdescription.h"				// definition for the queued request description

namespace NetUps
{
class CNetUps;
class CDatabaseEntry;
class CProcessEntry;
class CThreadEntry;
class CUpsStateMachine;	

NONSHARABLE_CLASS(CNetUpsImpl) : public CBase
	{
public:
	enum TLifeTimeMode
		{
		EProcessLifeTimeMode,
		ENetworkLifeTimeMode
		};
		
	enum TSessionMode
		{
		ENonSessionMode,
		ESessionMode
		};	
public:		
	enum TConnectionAdjustment
		{
		EIncrement = 0,
		EDecrement = 1
		};
public:
	// Construction / Destruction  Methods
	static CNetUpsImpl* NewL();
	~CNetUpsImpl();
	
	// Reference count methods
	void IncrementRefCount(void);
	void DecrementRefCount(void);
	TInt32 RefCount();

	// Configure Life Setting, used by TLS
	void SetLifeTimeMode(TLifeTimeMode aLifeTimeMode);	
	TLifeTimeMode LifeTimeMode();

	// Methods to be called by client
	void ProcessPolicyCheckRequestL(const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId& aRequestId);
	TInt CancelRequest(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, const TRequestId& aRequestId);
	void IncrementConnectionCountL(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId); 
	void DecrementConnectionCountL(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId);
	void DecrementConnectionCountL(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, TProcessId& aProcessId, TThreadId& aThreadId);
	// utility methods
	static TSessionMode MapInternalStateToSessionMode(TNetUpsState aNetUpsState);
	static void DetermineUpsDecision(TNetUpsState aNetUpsState, TNetUpsDecision&);
	TRequestId	AllocateRequestId();
private:
	// Construction / Destruction methods
	CNetUpsImpl();
	void ConstructL();
	CNetUpsImpl(const CNetUps&);
	void operator=(const CNetUps&);
	bool operator==(const CNetUps&);		
	
	// Utility Methods		
	CUpsStateMachine&  IdentifyStateMachineL(TInt32 aServiceId);	
	void IdentifyDatabaseEntryL(const TPolicyCheckRequestData& aPolicyCheckRequestData, CDatabaseEntry*& aDatabaseEntry, CProcessEntry*& aProcessEntry, CThreadEntry*& aThreadEntry); 
	TInt  IdentifyDatabaseEntry(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, CThreadEntry*& aThreadEntry);
	TInt  IdentifyDatabaseEntry(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, CDatabaseEntry*& aDatabaseEntry, CProcessEntry*& aProcessEntry, CThreadEntry*& aThreadEntry); 
	TInt  IdentifyDatabaseEntry(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, TProcessId& aProcessId, TThreadId& aThreadId, CDatabaseEntry*& aDatabaseEntry, CProcessEntry*& aProcessEntry, CThreadEntry*& aThreadEntry);
	UserPromptService::RUpsSession& UpsSession();	
private:
	TLifeTimeMode							 iLifeTimeMode;
	TInt32 									 iRefCount;
	RPointerArray<CUpsStateMachine> 		 iStateMachine;	// one state machine per service id	
	RPointerArray<CDatabaseEntry> 			 iDatabaseEntry;// one database entry per service id
	UserPromptService::RUpsSession		iUpsSession;
	TRequestId							iRequestId;			
	__FLOG_DECLARATION_MEMBER;		
	};

} // end of namespace NetUps

#endif // NETUPSIMPL_H
