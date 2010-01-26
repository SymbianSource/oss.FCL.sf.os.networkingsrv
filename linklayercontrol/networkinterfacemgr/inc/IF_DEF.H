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
 @file IF_DEF.H
 @internalComponent
*/


#if !defined(__IF_DEF_H__)
#define __IF_DEF_H__

#include <e32std.h> // for TTime
#include <comms-infras/nifif.h>
#include <comms-infras/nifagt.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_ini.h>
#include <f32file.h>
#include <connpref.h>
#include <ss_std.h>
#include <comms-infras/dbaccess.h>
#include <es_prot_internal.h>

const TInt KMaxLengthOfTableNameAndColumnName = KCommsDbSvrMaxColumnNameLength * 2 + 1;
const TInt KMessageArrayGranularity = 2;


class CMBufPoolReaper;
class Nif;
class CAgentAdapter;
class CAgentDialogProcessor;

NONSHARABLE_CLASS(CNifMan) : public CSocketServExtBase
/**
@internalComponent
*/
	{
friend class Nif;
friend class CAgentAdapter;

public:
	CNifMan();
	virtual ~CNifMan();
	virtual void InstallL(const TDesC& aArgs);
	virtual void Remove();

	CAgentDialogProcessor* AgentDialogProcessor();    
    
	TInt NumAgents() const;
	CNifIfBase* CreateInterfaceL(const TDesC& aName, MNifIfNotify* aNotify);

	IMPORT_C static CNifMan* Global();
	void NetworkLayerClosed(MNifIfUser* aNetwork);
	void UpdateIniConfigL();

private:
	CNifAgentRef* DoFindOrCreateL(const TDesC& aName, TBool aCreate, TBool aCreateNewInstance = EFalse);
	CNifFactory* DoFindFactoryL(TUid aUid2, const TDesC& aFilename, CObjectCon& aCon, TBool aCreate);
	void DoGetFileNameL(const TDesC& aName, TDes& aFilename);
	TInt StartEsWatchThread();

	CObjectCon* iAgents;
	CObjectCon* iAgentFactories;
	CObjectConIx* iContainers;

	CAgentDialogProcessor* iAgentDlgProc;
	HBufC* iDefMatch;
	HBufC* iDefFilename;
	TInt iCurrentMBufMaxHeap;
	TInt iRequiredMBufMaxHeap;
    };


_LIT(KNifManModule,"Nifman");
_LIT(KAgentExtension,".agt");

_LIT(KNifManSection,"nifman");
_LIT(KNifManDefault,"default");
_LIT(KNifManMaxHeapSize,"maxmbufheap");
_LIT(KNifManAgents,"agents");
_LIT(KNifManInterfaces,"interfaces");
_LIT(KNetConLibraryName, "networkcontroller");
_LIT(KDefaultNetConLibName, "NETCON.DLL");

#endif
