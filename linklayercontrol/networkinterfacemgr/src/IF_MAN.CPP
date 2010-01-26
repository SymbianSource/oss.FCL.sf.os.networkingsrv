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
// Interface Manager
// 
//

/**
 @file IF_MAN.CPP
*/

#include "IF_DEF.H"
#include "NI_STD.H"
#include <es_mbman.h>
#include "Ni_Log.h"
#include <agentdialog.h>
#include <c32root.h>
#include <ecom/ecom.h>
#include "cagentdlgproc.h"

/**
@internalComponent
*/
const TInt KAsyncDtorPriority = 10000;

/**
@internalComponent
*/
typedef CNifFactory* (*TNifFactoryNewL)();

/**
@internalComponent
*/
/*typedef CNetworkControllerBase* (*CreateNetConFunctionL)();*/

inline TAny* Tls()
/**
@internalComponent
*/
 { return Dll::Tls(); }

inline TInt SetTls(TAny* aTls) 
/**
@internalComponent
*/
{ return Dll::SetTls(aTls); }


EXPORT_C CNifFactory::CNifFactory()
/**
Constructor
*/
	{

	}

EXPORT_C CNifFactory::~CNifFactory()
/**
Destructor
*/
	{

	__ASSERT_DEBUG(!iLib.Handle(), Panic(ENifManPanic_LibraryNotClosed));
	delete iAsyncDtor;
	}


EXPORT_C TInt CNifFactory::Open()
/**
If somehow something opens object just before it is destructed
@return KErrNone,systemwide error code.
*/
	{
	if(AccessCount()==0 && iAsyncDtor->IsActive())
		iAsyncDtor->Cancel();
	Inc();
	return KErrNone;
	}

EXPORT_C void CNifFactory::Close()
/**
Defer deletion of object to facilitate async closing of library
*/
	{

	Dec();
	if (AccessCount()==0)
		{
		if(iAsyncDtor)
			{
			if(!iAsyncDtor->IsAdded())
				{
				CActiveScheduler::Add(iAsyncDtor);
				}
			iAsyncDtor->CallBack();
			}
		else // Only partially constructed
			ControlledDelete(this);
		}
	}

EXPORT_C TInt CNifFactory::ControlledDelete(TAny* aFactory)
/**
Delete factory
@return systemwide error code
*/
	{
	
	CNifFactory* n = (CNifFactory*)aFactory;
	RLibrary lib;
	lib.SetHandle(n->iLib.Handle());
	n->iLib.SetHandle(0);
	delete n; // deletes async callback object also
	lib.Close();
	return KErrNone;
	}

EXPORT_C void CNifFactory::Cleanup(TAny* aObject)
/**
For use with clean up stack
@param aObject
*/
	{
	
	((CObject*)aObject)->Close();
	}

EXPORT_C void CNifFactory::InitL(RLibrary& aLib, CObjectCon& aCon)
/**
Setup async Destructor & transfer library
@param aLib
@param aCon
*/
	{	
	
	TCallBack c = TCallBack(ControlledDelete, this);
	iAsyncDtor = new (ELeave) CAsyncCallBack(c, KAsyncDtorPriority);
	iAsyncDtor->Deque();	//PERF: we'll re-join the queue JIT
	iLib.SetHandle(aLib.Handle());
	aLib.SetHandle(0); // Handle transferred

	TParse parse;
	User::LeaveIfError(parse.Set(iLib.FileName(),0,0));
	
	TPtrC temp = parse.Name();
	SetNameL(&temp);

	InstallL(); // pure virtual
	aCon.AddL(this);	
	}


//
CNifMan::CNifMan()
	{
	}

CNifMan::~CNifMan()
	{

	TInt i;

	// go through deleting agents
	CObjectCon* con;
	if(iAgents)
		{
		con = iAgents;
		for(i=0 ; i<con->Count() ; ++i)
			delete (*con)[i];
		}

	// As all agents have been deleted there should be no factories left
    // but as their d'tors are async they could still be lying around but
	// access counts should be zero
	// go through deleting factories for real

	delete iAgentDlgProc;

	if(iAgentFactories)
		{
		con = iAgentFactories;
		for(i=0 ; i<con->Count() ; ++i)
			delete (*con)[i];
		}

	if(iContainers)
        {
		delete iContainers;
        }
        
	if(iDefMatch)
        {
		delete iDefMatch;
        }
        
	if(iDefFilename)
	    {
		delete iDefFilename;
	    }
	    
	if(Global()==this)
	    {
	    SetTls(NULL);
	    }

	// Unload the Dialog Server CPM
	RRootServ rootserver;
	if(rootserver.Connect()==KErrNone) 
		{
		TCFModuleName name;
		name.Copy(KCommsDialogServerName);
		TRequestStatus status;
		rootserver.UnloadCpm(status, name, EImmediate);
		User::WaitForRequest(status);
		rootserver.Close();
		}
	
	REComSession::FinalClose();
	
	LOG( NifmanLog::Printf(_L("--------------- Nifman Finished --------------")); )
	}


CAgentDialogProcessor* CNifMan::AgentDialogProcessor()
    {
    return iAgentDlgProc;
    }


void CNifMan::InstallL(const TDesC& /*aArgs*/)
/**
Treat as constructL() 
Unless the bug has been fixed leaving will orphan a library handle
so this is avoided at all costs by ensuring the extension is only ever loaded once
*/
	{

	LOG( NifmanLog::Printf(_L("--------------- Nifman Started ---------------")); )
	LOG_DETAILED( NifmanLog::Printf(_L("Detailed logging enabled")); )

	if(Tls())
		User::Leave(KErrAlreadyExists);

	iContainers = CObjectConIx::NewL();
	iAgents = iContainers->CreateL();
	iAgentFactories = iContainers->CreateL();

	iAgentDlgProc = new(ELeave)CAgentDialogProcessor();

//	StartEsWatchThread();
		
	User::LeaveIfError(SetTls(this));
	}

void CNifMan::Remove()
/**
Let the destructor do it all
*/
	{
	}

EXPORT_C CNifMan* CNifMan::Global()
/**
Get the global nifman
*/
	{
	return (CNifMan*)Tls();
	}

void CNifMan::DoGetFileNameL(const TDesC& aName, TDes& aFilename)
/**
Look in ini file for default agent module name
*/
	{

	CESockIniData* ini=0;
	TPtrC result;
	
	if(!iDefMatch || !iDefFilename)
		{

		ini = CESockIniData::NewL();
	    CleanupStack::PushL(ini);
		if(!iDefMatch)
		    {
		    if(!ini->FindVar(KNifManSection, KNifManDefault, result))
				{
				_LIT(genconnString,"genconn");
			    iDefMatch = genconnString().AllocL();
				}
		    else
			    iDefMatch = result.AllocL();
			}

		if(!iDefFilename)
			{
			if(!ini->FindVar(KNifManAgents, *iDefMatch, result))
				{
				TName name = *iDefMatch;
				name.Append(KAgentExtension);
				iDefFilename = name.AllocL();
				}
			else
				iDefFilename = result.AllocL();
			}
		}

	if(!aName.Length() || !aName.CompareF(*iDefMatch))
		{
		aFilename = *iDefFilename;
		}
	else
		{
		if(!ini)
			{
			ini = CESockIniData::NewL();
	        CleanupStack::PushL(ini);
			}
		if(!ini->FindVar(KNifManAgents, aName, result))
			{
			aFilename=aName;

			if(aFilename.Right(4) != KAgentExtension())
				aFilename.Append(KAgentExtension);
		    }
		else
		    aFilename = result;
		}
	
	if(ini)
		CleanupStack::PopAndDestroy();
	}


CNifFactory* CNifMan::DoFindFactoryL(TUid aUid2, const TDesC& aFilename, CObjectCon& aCon, TBool aCreate)
/**
Load a factory and check the Uid etc
function always opens factory CObject if successful
*/
	{


	TParse parse;
	User::LeaveIfError(parse.Set(aFilename, 0, 0));

	TName dummy1;
	TInt find=0;
	CNifFactory* f=0;

	if(aCon.FindByName(find, parse.Name(), dummy1)!=KErrNone)
		{

		if(!aCreate)
			User::Leave(KErrNotFound);

	    // Else load the module
		TAutoClose<RLibrary> lib;
		User::LeaveIfError(lib.iObj.Load(aFilename));
		lib.PushL();

		// The Uid check
		if(lib.iObj.Type()[1]!=aUid2)
			User::Leave(KErrBadLibraryEntryPoint);

		TNifFactoryNewL libEntry=(TNifFactoryNewL)lib.iObj.Lookup(1);
		if (libEntry==NULL)
			User::Leave(KErrNoMemory);

		f =(*libEntry)(); // Opens CObject
		if (!f)
			User::Leave(KErrBadDriver);

		CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, f));
		f->InitL(lib.iObj, aCon); // Transfers the library object if successful

		// Can pop the library now - auto close will have no effect because handle is null
		CleanupStack::Pop();
		lib.Pop();

		}
	else
		{
		f=(CNifFactory*)aCon.At(find);
		f->Open();
		}
	return f;
	}


TInt CNifMan::NumAgents() const
	{
	return iAgents->Count();
	}
