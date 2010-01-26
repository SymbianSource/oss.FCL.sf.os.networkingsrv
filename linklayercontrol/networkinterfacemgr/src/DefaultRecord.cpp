// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#include <nd_err.h>
#include "DbAccess.h"
#include "AgentPanic.h"
#include "NetConError.h"

//
// CDefaultRecordAccess defintions
//

CDefaultRecordAccess::CDefaultRecordAccess()
/**
default constructor
*/
	{}

CDefaultRecordAccess::CDefaultRecordAccess(const TDesC& aName)
	: iName(aName)
/**
constructor
*/
	{}

CDefaultRecordAccess::~CDefaultRecordAccess()
/**
destructor
*/
	{
	delete iTable;
	}

void CDefaultRecordAccess::Close()
/**
Close the table, but don't reset the ID, 
so the next time we open a new record, we can work 
out if it is a different one or not.
*/
	{
	CloseTable();
	iOverridden=EFalse;
	}

void CDefaultRecordAccess::CloseTable()
	{
	delete iTable;
	iTable = NULL;
	}

TBool CDefaultRecordAccess::OpenRecordL(CCommsDatabase* aDb, TUint32 aId)
/** 
Close the view if there is one already open. If this has been overriden then use 
the original ID given to us, if not, then use the new one. Open the view on the 
table and position to the correct record.
@return if the record ID has changed, to notify client.
*/
	{
	__ASSERT_DEBUG(iName.Length() > 0, AgentPanic(Agent::EUnknownTableName));

	CloseTable();

	TBool ret=ETrue;
	if (iId==aId || iOverridden)	// if the ID's happen to be the same or the record is already overridden
		ret=EFalse;

	if (!iOverridden)
		iId=aId;

	if (iId == 0)
	    User::Leave(KErrNetConDatabaseDefaultUndefined);

	CCommsDbTableView* table = aDb->OpenViewMatchingUintLC(iName,TPtrC(COMMDB_ID),iId);

	User::LeaveIfError(table->GotoFirstRecord());

	CleanupStack::Pop(); // table
	iTable = table;

	return ret;
	}

TBool CDefaultRecordAccess::OpenRecordL(CCommsDatabase* aDb, TUint32 aId, const TDesC& aTableName)
/**
Open the record, from the table with aName.  If the name of the table, 
or the ID of the record have changed, then return ETrue to signify 
a change in service.
*/
	{

	TBool ret1=EFalse;
	if (iName!=aTableName)	// table has changed, so service certainly has!
		ret1=ETrue;

	iName = aTableName;

	TBool ret2=OpenRecordL(aDb,aId);
	
	return (ret1 || ret2); 
	}

TBool CDefaultRecordAccess::SetOverridden(TBool aOverridden, TUint32 aId)
/**
Set whether this is an overridden record and the ID to use when we open the table.
Return, if this is an override, if the ID has changed.  If it is not a override, this will
be checked when the record is opened.  If this is not an override them ignore the ID as
it will be given again when the record is opened.
*/
	{

	iOverridden=aOverridden;
	TBool ret=EFalse;
	if (iOverridden && iId!=aId)
		{
		ret=ETrue;
		iId=aId;
		}

	return ret;
	}

