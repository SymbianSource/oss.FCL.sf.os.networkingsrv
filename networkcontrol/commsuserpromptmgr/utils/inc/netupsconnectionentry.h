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
// This file defines the CConnectionEntry a structure which
// specifies the number of connections associated with a CommsId. 
// @internalAll
// @prototype
// 
//

#ifndef NETUPSCONNECTIONENTRY_H
#define NETUPSCONNECTIONENTRY_H

#include <e32def.h>		  								// defines TInt

#include <comms-infras/ss_activities.h>

#include <comms-infras/commsdebugutility.h> 			// defines the comms debug logging utility

#include "netupstypes.h"								// defines TRequestId

namespace NetUps
{

class CConnectionEntry : public CBase
	{
public:
	static CConnectionEntry* NewL(const Messages::TNodeId& aCommsId, TInt32 aCount);
	~CConnectionEntry();
	const Messages::TNodeId& CommsId() { return iCommsId; }
	const TInt32 Count() 	  { return iCount; }
	void IncrementCount();
	void DecrementCount();
private:
	void ConstructL();
	CConnectionEntry(const Messages::TNodeId& aCommsId, TInt32 aCount) : iCommsId(aCommsId), iCount(aCount) {}
private:		
	const Messages::TNodeId iCommsId;
	TInt32					iCount;

	__FLOG_DECLARATION_MEMBER;			
	};
	
} // end of namespace NetUps

#endif // NETUPSCONNECTIONENTRY_H
