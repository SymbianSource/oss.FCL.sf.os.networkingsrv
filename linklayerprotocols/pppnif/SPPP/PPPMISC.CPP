// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <f32file.h>
#include "PPPBASE.H"

//
// PPP Link Interface Support
//

MPppRecvr::MPppRecvr(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId)
	{
// Implementation moved to MPppRecvr::Init(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId).
	Init(aPppLcp, aPhase, aPppId);
	}


MPppRecvr::MPppRecvr()
	{
	}

void MPppRecvr::Init(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId)
	{
// Copied here from MPppRecvr::MPppRecvr(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId)

	iPppRecvrListLink.iNext = &iPppRecvrListLink;
	iPppRecvrListLink.iPrev = &iPppRecvrListLink;
	iPppId = aPppId;
	iActivePhase = aPhase;
	iPppLcp = aPppLcp;
	iPppAbortCode = KErrNone;
	// We dont auto-register LCP as LCP wont be ready
	// to accept the registration at this stage
//	if (aPppId!=KPppIdUnknown && aPppId!=KPppIdLcp)
//		Register();	
	}

MPppRecvr::~MPppRecvr()
	{
	iPppRecvrListLink.Deque();
	}

EXPORT_C void MPppRecvr::FlowOn()
	{
	}

void MPppRecvr::LowerLayerUp()
	{
	}

void MPppRecvr::LowerLayerDown(TInt)
	{
	}

void MPppRecvr::Deque()
	{
	iPppRecvrListLink.Deque();
	iPppRecvrListLink.iNext = &iPppRecvrListLink;
	iPppRecvrListLink.iPrev = &iPppRecvrListLink;
	}

void MPppRecvr::Register(TUint aPppId)
	{
	if (aPppId!=KPppIdAsIs)
		SetId(aPppId);
	iPppLcp->RegisterRecvr(this);
	}

void MPppRecvr::Reregister(TUint aPppId, TPppPhase aPhase)
	{
	if (aPppId!=KPppIdAsIs)
		SetId(aPppId);
	SetPhase(aPhase);
	iPppLcp->ReregisterRecvr(this);
	}

void MPppRecvr::Deregister()
	{
	iPppLcp->DeregisterRecvr(this);
	}


typedef CNifFactory* (*TPppFactoryNewL)();
CNifFactory* MPppRecvr::FindPppFactoryL(const TDesC& aFilename, TUid aUid2, CObjectCon& aCon)
//
// Basically this is all the stuff required to load a DLL appart from the
// Factory->CreatMe call
//
	{
	CNifFactory* Factory=NULL;
	TParse parse;
	User::LeaveIfError(parse.Set(aFilename, 0, 0));

	TName dummy1;
	TInt find=0;

	if(aCon.FindByName(find, parse.Name(), dummy1)!=KErrNone)
		{

	    // Else load the module
		TAutoClose<RLibrary> lib;
		User::LeaveIfError(lib.iObj.Load(aFilename));
		lib.PushL();

		// The Uid check
		if(lib.iObj.Type()[1]!=aUid2)
			User::Leave(KErrBadLibraryEntryPoint);

		TPppFactoryNewL libEntry=(TPppFactoryNewL)lib.iObj.Lookup(1);
		if (libEntry==NULL)
			User::Leave(KErrNoMemory);

		Factory =(*libEntry)(); // Opens CObject
		if (!Factory)
			User::Leave(KErrBadDriver);

		CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));
		Factory->InitL(lib.iObj, aCon); // Transfers the library object if successful

		// Can pop the library now - auto close will have no effect because handle is null
		CleanupStack::Pop();
		lib.Pop();

		}
	else
		{
		Factory=(CNifFactory*)aCon.At(find);
		Factory->Open();
		}
		return Factory;
	}

