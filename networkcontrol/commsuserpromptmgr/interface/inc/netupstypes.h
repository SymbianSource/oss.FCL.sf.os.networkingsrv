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
// This file provides the public type definitions used by the Networking UPS Component.
// @internalAll
// @released
// 
//

#ifndef NETUPSTYPES_H
#define NETUPSTYPES_H

#include <e32def.h>		  							// defines TInt
#include <e32std.h>									// defines ThreadId, TProcessId etc

#include <elements/nm_address.h> 					// defines TNodeId
#include <comms-infras/ss_upsaccesspointconfigext.h> // defines TDestinationNameType

namespace NetUps
{
enum TNetUpsDecision
	{
	EYes,
	ENo,
	ESessionYes,
	ESessionNo
	};

typedef TUint32 TRequestId;

class MPolicyCheckRequestOriginator
	{
public:
	virtual TInt ProcessPolicyCheckResponse(TNetUpsDecision aNetUpsDecision, const Messages::TNodeId& aCommsId) = 0; 
	virtual TInt ProcessPolicyCheckResponse(TInt aError, const Messages::TNodeId& aCommsId) = 0; 
	};	

struct TPolicyCheckRequestData
	{
	enum TDefaultServiceId
		{
		EUndefined = 0
		};
		
	TPolicyCheckRequestData(const TProcessId& 												aProcessId,
	 						const TThreadId& 												aThreadId,	
							TBool 		   													aPlatSecCheckResult,
							const TDesC& 													aDestinationName,
							const TDesC& 													aAccessPointName,
							const Messages::TNodeId& 										aCommsId,
							MPolicyCheckRequestOriginator&									aPolicyCheckRequestOriginator) :
							iProcessId(aProcessId),
							iThreadId(aThreadId),
							iPlatSecCheckResult(aPlatSecCheckResult),
							iDestinationName(aDestinationName),
							iAccessPointName(aAccessPointName),
							iCommsId(aCommsId),
							iPolicyCheckRequestOriginator(aPolicyCheckRequestOriginator)
							{
							iServiceId = EUndefined;	
							}

	// The Service Id parameter is set by the NetUps, not the client.
	void SetServiceId(TInt32& aServiceId) { iServiceId = aServiceId;}
	TInt32 ServiceId() { return iServiceId; }
	
	const TProcessId& 										iProcessId;
	const TThreadId& 										iThreadId;
		  TInt32 			  								iServiceId;
	const TBool 		   									iPlatSecCheckResult;
	const TDesC& 											iDestinationName;
	const TDesC& 											iAccessPointName;
	const Messages::TNodeId& 								iCommsId;
	MPolicyCheckRequestOriginator&							iPolicyCheckRequestOriginator;							
	};


} // end of namespace NetUps

#endif // NETUPSTYPES_H
