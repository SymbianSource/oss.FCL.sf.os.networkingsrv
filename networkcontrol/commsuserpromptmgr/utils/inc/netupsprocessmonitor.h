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
// upsdataprocessmonitor.h
// This file provides the interface specification for active objects
// which monitors process life times
// @internalAll
// @prototype
// 
//


#ifndef NETUPSPROCESSMONITOR_H
#define NETUPSPROCESSMONITOR_H

#include <e32base.h>				// defines CActive
#include <e32std.h>					// defines RProcess

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps 
{
class CProcessEntry;
class CDatabaseEntry;
	
NONSHARABLE_CLASS(CProcessMonitor) : public CActive
	{
public:
	static CProcessMonitor* NewL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry);
	~CProcessMonitor();
	void Start();
private:
	void ConstructL();
	CProcessMonitor(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry);

	void RunL();
	void DoCancel();
	void RunErrorL();
private:
	RProcess 		iProcess;
	CDatabaseEntry& iDatabaseEntry;
	CProcessEntry&	iProcessEntry;

	__FLOG_DECLARATION_MEMBER;		
	};		

} // end of namespace

#endif // NETUPSPROCESSMONITOR_H

