// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is a pep header file
//

#ifndef _PEPMANAGER_
#define _PEPMANAGER_

#include <e32base.h>
#include <es_mbuf.h>
#include <es_ini.h>

// Should be dynamic, based on ini-file
#define KMaximumNumberOfPEPs 11

_LIT(PEP_INI_DATA,					"pep.ini");				// the name of the ini-file

_LIT(PEP_INI_PROXIES,				"proxies");             // [proxies]
_LIT(PEP_PROXY_LIST,				"proxylist");           // proxylist= 1,2,3,4,5


class CPEP;
class CPEPManager;
class CPacketInterface;
class MNifContextNotify;

class CPEPManager 
{
public:
	IMPORT_C static CPEPManager* NewL();
	IMPORT_C ~CPEPManager();
	IMPORT_C CPacketInterface* NewPacketInterfaceL(MNifContextNotify* aContext,TDes& aPepId);
	
	void FreePeP(TUint8 aPepId);
protected:
	CPEPManager();
	void InitL();	
	TBool InitProxies();
	CPEP* GetPeP(TUint8 aPepId);
		
private:	
	CPEP *iPEPTable[KMaximumNumberOfPEPs];
	TBool iPEPUsedTable[KMaximumNumberOfPEPs];
};



#endif
