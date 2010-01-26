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
// To avoid using stale refernces these keys should only be used in synchronous methods. 
// @internalAll
// @prototype
// 
//

#ifndef NETUPSKEYS_H
#define NETUPSKEYS_H

namespace Messages
{
class TNodeId;	
}

namespace NetUps
{

class CDatabaseEntry;
class CProcessEntry;
class CThreadEntry;

struct TProcessKey
	{
	TProcessKey(CDatabaseEntry& 	aDatabaseEntry	,
				CProcessEntry& 	aProcessEntry) 	:
				iDatabaseEntry(aDatabaseEntry),
				iProcessEntry (aProcessEntry)
				{	
				};
				
	CDatabaseEntry&	iDatabaseEntry;
	CProcessEntry& 	iProcessEntry;
	};

struct TThreadKey : public TProcessKey
	{
	TThreadKey(	CDatabaseEntry& 		aDatabaseEntry,
				CProcessEntry& 			aProcessEntry,
	 			CThreadEntry&			aThreadEntry) :
				TProcessKey(aDatabaseEntry, aProcessEntry),
				iThreadEntry(aThreadEntry)
					{	
					}
	CThreadEntry& 	iThreadEntry;
	};

struct TCommsIdKey : public TThreadKey
	{
	TCommsIdKey(CDatabaseEntry& 			aDatabaseEntry,
				CProcessEntry& 				aProcessEntry,
	 			CThreadEntry&				aThreadEntry,
				const Messages::TNodeId& 		aCallersNodeId) :
				TThreadKey(aDatabaseEntry, 	aProcessEntry, aThreadEntry),
				iCommsId(aCallersNodeId)
					{	
					}					
	const Messages::TNodeId& 	iCommsId;
	};
	
} // end of namespace NetUps

#endif // NETUPSKEYS_H
