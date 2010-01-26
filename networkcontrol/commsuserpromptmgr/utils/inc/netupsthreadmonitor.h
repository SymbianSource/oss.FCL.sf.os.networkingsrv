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
// upsdatathreadmonitor.h
// This file provides the interface specification for active objects
// which monitors thread life times
// @internalAll
// @prototype
// 
//


#ifndef NETUPSTHREADMONITOR_H
#define NETUPSTHREADMONITOR_H

#include <e32base.h>	// defines CActive
#include <e32std.h>		// defines RThread

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps 
{
class	CThreadEntry;
class 	CProcessEntry;
class	CDatabaseEntry;

NONSHARABLE_CLASS(CThreadMonitor) : public CActive
	{
public:
		static CThreadMonitor* NewL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry& aThreadEntry);
		~CThreadMonitor();
		void Start();
		const RThread& Thread() const;
private:
		void ConstructL();
		CThreadMonitor(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry& aThreadEntry);

		void RunL();
		void DoCancel();
		void RunErrorL();		
private:
		RThread 		iThread;
		CDatabaseEntry& iDatabaseEntry;
		CProcessEntry&	iProcessEntry;
		CThreadEntry&	iThreadEntry;
		
	__FLOG_DECLARATION_MEMBER;				
	};		
} // end of namespace netups

#endif // NETUPSTHREADMONITOR_H
