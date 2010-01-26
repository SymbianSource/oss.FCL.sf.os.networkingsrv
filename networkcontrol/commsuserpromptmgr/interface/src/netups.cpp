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
// This file provides the implementation for CNetUps's methods
// @internalAll
// @released
// 
//

#include <e32std.h> 			// defines DLL TLS
#include <e32err.h> 			// defines standard error codes
#include <e32const.h>			// defines KMaxTInt32

#include "netupsassert.h" 		// defines NetUps Asserts
#include "netupsserviceid.h"	// defines the supported service ids.


#include "netups.h"				// defines public NetUps interface
#include "netupsimpl.h"			// defines the NetUps implementation
#include "netupstls.h"			// defines structure used to store Netups components in TLS

using namespace Messages;

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

EXPORT_C CNetUps* CNetUps::NewL(TInt32 aServiceId)
/** Creates an instance of the Net UPS. 
@return A pointer to an instance of the Net UPS if successful, otherwise leaves with a system error code.
 */ 
	{
	CNetUps* self = new (ELeave) CNetUps(aServiceId);

#ifdef NETUPS_TEST_HARNESS
	__UHEAP_MARK;
#endif
	
	CleanupStack::PushL(self);
	self->ConstructL(aServiceId);
	CleanupStack::Pop(self);
	
	return self;
	}

void CNetUps::ConstructL(TInt32 aServiceId)
	{
	if (aServiceId != EIpServiceId)
		{
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent, _L("CNetUps::ConstructL aServiceId = %d"), aServiceId);
		User::Leave(KErrNotSupported);
		}

	CNetUpsTls* netUpsTls = static_cast<CNetUpsTls*>(Dll::Tls());

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_2(_L("CNetUps %08x:\tConstructL(), aServiceId = %d"), this, aServiceId);		

	if (netUpsTls == NULL)
		{
		netUpsTls = CNetUpsTls::NewL(aServiceId);

		CleanupStack::PushL(netUpsTls);
		Dll::SetTls( static_cast<TAny*>(const_cast<CNetUpsTls*>(netUpsTls)) );
		CleanupStack::Pop(netUpsTls);
		}
			
	iNetUps = netUpsTls->GetNetUpsImpl();

	if (iNetUps->RefCount() == KMaxTInt32)
		{
		User::Leave(KErrOverflow);
		}
	else
		{
		iNetUps->IncrementRefCount();				
		}	

	}

CNetUps::CNetUps(TInt32 aServiceId) : iServiceId(aServiceId)
	{
	}

CNetUps::~CNetUps()
/** Deletes this instance of the Net UPS. 
 */
	{
	__FLOG_1(_L("CNetUps %08x:\t~CNetUps()"), this);		

	if (iNetUps) // OOM tests require this, investigate.
		{
		iNetUps->DecrementRefCount();

		if (iNetUps->RefCount() <= 0)
			{
			CNetUpsTls* netUpsTls = static_cast<CNetUpsTls*>(Dll::Tls());
			delete netUpsTls;
			Dll::SetTls( static_cast<TAny*>(NULL) );
			}			
		}
		
	__FLOG_CLOSE;	
#ifdef NETUPS_TEST_HARNESS
	__UHEAP_MARKEND;	
#endif
	}	

EXPORT_C TBool CNetUps::UpsDisabled(TInt32 aServiceId)
/** Determines whether the NetUps Component is disabled or enabled for a given Service ID.
 
@param aServiceId The service ID which specialises this query.
*/
	{
	__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent, _L("CNetUps::UpsDisabled(), aServiceId = %d"), aServiceId);		
	
	TBool rc = ETrue;
	
	if (aServiceId != EIpServiceId)
		{
		__FLOG_STATIC0(KNetUpsSubsys, KNetUpsComponent, _L("CNetUps::UpsDisabled() Service id not supported"));
		rc = EFalse;
		}
	else
		{
		CNetUps* netUps = NULL;
		TRAPD(err, netUps = CNetUps::NewL(aServiceId);)
		if (err == KErrNone)
			{
			// coverity[leave_without_push]
			delete netUps;
			rc = EFalse;		
			}				
		}
	
	return rc;
	}

EXPORT_C void CNetUps::ProcessPolicyCheckRequestL(TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId& aRequestId)

/** Processes an asynchronous UPS policy check request; when the request completes the CNetUps calls
MPolicyCheckRequestOriginator::ProcessPolicyCheckResponse to notify the originator of the response.

@param aPolicyCheckRequestData A structure which encapsulates the attributes associated with a policy check request.
@param aRequestId			   The NetUps updates this variable with an identifier which uniquely identifies this policy check request.

 */
	{
	__FLOG_7(_L("CNetUps %08x:\tProcessPolicyCheckRequestL(): process id = %d, thread id = %d, service id = %d, platsec result = %d, commsId = %08x, originator  = %08x"),
			this, aPolicyCheckRequestData.iProcessId.Id(), aPolicyCheckRequestData.iThreadId.Id(), (TInt32) aPolicyCheckRequestData.iServiceId, aPolicyCheckRequestData.iPlatSecCheckResult, &aPolicyCheckRequestData.iCommsId, &aPolicyCheckRequestData.iPolicyCheckRequestOriginator);

	aPolicyCheckRequestData.SetServiceId(iServiceId);
	iNetUps->ProcessPolicyCheckRequestL(aPolicyCheckRequestData, aRequestId);				
	}

EXPORT_C TInt CNetUps::CancelRequest(const TNodeId& aCallersNodeId, const TRequestId& aRequestId)
/** Cancels an outstanding asynchronous UPS Policy Check Request.
 * 
@param aCallersNodeId   A structure which uniquely identifies the node which posted the original request.
@param aRequestId This variable uniquelyidentifies the policy check request which is to be cancelled.
 */
	{
	__FLOG_3(_L("CNetUps %08x:\tCancelRequest(), aCallersNodeId = %08x, iServiceId = %d"), this, &aCallersNodeId, iServiceId);		

	return iNetUps->CancelRequest(iServiceId, aCallersNodeId, aRequestId);
	}


EXPORT_C void CNetUps::IncrementConnectionCountL(const TNodeId& aCallersNodeId)
/** Increments the connection count associated with a particular thread.
Typically the connection count is incremented by the NetUps as soon as the request is authorised to
avoid race conditions which may result in an inaccurate connection count.

@param aCallersNodeId      The CommsId associated with the caller of this method.    	   
 */
	{
	__FLOG_3(_L("CNetUps %08x:\tIncrementConnectionCountL(), aCallersNodeId = %08x, iServiceId = %d"), this, &aCallersNodeId, iServiceId);		

	iNetUps->IncrementConnectionCountL(iServiceId, aCallersNodeId);
	}

EXPORT_C void CNetUps::DecrementConnectionCountL(const TNodeId& aCallersNodeId)
/** Decrements the connection count associated with a particular thread.
This method should always be called when a connection is deleted.

@param aCallersNodeId      The CommsId associated with the caller of this method.    	   
 */
	{
	__FLOG_3(_L("CNetUps %08x:\tDecrementConnectionCountL(), aCallersNodeId = %08x, iServiceId = %d"), this, &aCallersNodeId, iServiceId);		

	iNetUps->DecrementConnectionCountL(iServiceId, aCallersNodeId);
	}

EXPORT_C void CNetUps::DecrementConnectionCountL(const Messages::TNodeId&  aCallersNodeId, TProcessId& aProcessId, TThreadId& aThreadId)
	{
	__FLOG_5(_L("CNetUps %08x:\tDecrementConnectionCount1L(), aCallersNodeId = %08x, iServiceId = %d, process id = %d, thread id = %d,"), this, &aCallersNodeId, iServiceId, aProcessId.Id(), aThreadId.Id());		

	iNetUps->DecrementConnectionCountL(iServiceId, aCallersNodeId, aProcessId, aThreadId);
	}

} // end of namespace

