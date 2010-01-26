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
// This file defines the TQueuedRequestDescription, which allows
// uniquely identifies the location of a queued request. 
// @internalAll
// @prototype
// 
//

#ifndef NETUPSQUEUEDREQUESTDESCRIPTION_H
#define NETUPSQUEUEDREQUESTDESCRIPTION_H

#include <e32def.h>		  								// defines TInt
#include <e32std.h>										// defines ThreadId, TProcessId etc

#include <comms-infras/ss_activities.h>

#include "netupstypes.h"								// defines TRequestId
#include "netupsdatabaseentry.h"						// defines a netups database entry
#include "netupsprocessentry.h"							// defines a netups process entry
#include "netupsthreadentry.h"							// defines a net ups thread entry

namespace NetUps
{

struct TQueuedRequestDescription
	{
	TQueuedRequestDescription(	CDatabaseEntry* aDatabaseEntry,
								CProcessEntry* 	aProcessEntry,
								CThreadEntry*	aThreadEntry,
								ESock::TCommsId* aCommsId, 
								TRequestId		aRequestId) :
								iDatabaseEntry(aDatabaseEntry),
								iProcessEntry(aProcessEntry),
								iThreadEntry(aThreadEntry),
								iCommsId(aCommsId),
								iRequestId(aRequestId)
								{	
								}
	CDatabaseEntry*		iDatabaseEntry;
	CProcessEntry* 		iProcessEntry;
	CThreadEntry*		iThreadEntry;
	ESock::TCommsId* 	iCommsId;
	TRequestId			iRequestId;
	};
	
} // end of namespace NetUps

#endif // NETUPSQUEUEDREQUESTDESCRIPTION_H
