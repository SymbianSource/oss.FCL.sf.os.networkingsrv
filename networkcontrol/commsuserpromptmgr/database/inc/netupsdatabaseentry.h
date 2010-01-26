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
// upsdatabaseentry.h
// This file provides the specification for the database entry.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSDATABASEENTRY_H
#define NETUPSDATABASEENTRY_H

#include <e32base.h>								// defines CBase

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
class CProcessEntry;
NONSHARABLE_CLASS(CDatabaseEntry) : public CBase
	{
public:
	static CDatabaseEntry* NewL(TInt32 aServiceId);
	virtual ~CDatabaseEntry();

	TInt32 ServiceId();
	RPointerArray<CProcessEntry>& ProcessEntry(void);
protected: // class may be specialised by NetUps clients if required
	CDatabaseEntry(TInt32 aServiceId);
private:
	void ConstructL();
protected:
	RPointerArray<CProcessEntry> 	iProcessEntry;
	TInt32							iServiceId;
private:
	__FLOG_DECLARATION_MEMBER;		
	};
} // end of namespace

#endif // NETUPSDATABASEENTRY_H
