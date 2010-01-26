// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __TESTMGR_H__
#define __TESTMGR_H__
/**
*	Main  controller class (the engine)
*/
#include "common.h"
#include <c32comm.h>
#include "NIFAGT.H"
#include "NIFIF.H"
#include "NIFMAN.H"
#include <nifutl.h>

class CDummyNifAgentRef;
class CDummyProtocol;
class CTestMgr : public CActive, public MTimer
	{
public:	
		enum TEvent
		{	
			EAgentLoaded,   
			ENifLoaded
		}; 
		CTestMgr();
		virtual ~CTestMgr();
		static CTestMgr* NewL();
		static CTestMgr* NewLC();
		void StartLoadingAgentL();
		void StartLoadingNifL(const TDesC& aInterfaceName, const TDesC& aBaseName);
		void Notify(TEvent aEvent);
		virtual void TimerComplete(TInt aStatus);
		TInt Test1L(void);
		TInt Test2L(void);
		TInt Test3L(void);
		TInt Test4L(void);
		TInt Test5L(void);

private:
		enum TState
		{
			EAgentsLoading,
			EAgentConnecting,
			EBothLoaded,
			ERunTestEntry,
			AllTestRun,
		};
		
		void ConstructL();
		void RunL();
		void DoCancel();
		
		TEvent				iEvent;		
		TState				iCurrentState;
		CObjectConIx*		iContainers;
		CObjectCon*			iAgentFactories;
		CObjectCon*			iIfFactories;
		CDummyNifAgentRef*  ipDummyAgtRef;
		CDummyProtocol*		ipDummyPrt; 	

		CNifAgentFactory*	iAgentFactory;
		CNifIfLink*		iInterface;
		CNifIfBase*		iInterfaceBound;
		
	};

#endif //__TESTMGR_H__
