// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// hostname.cpp - hostnames of the local node
//

#include "hostname.h"
#include "inet6log.h"

#if defined(TCPIP6_USE_COMMDB)
#	include <commdb.h>
#else
#	include <commsdattypesv1_1.h>
	using namespace CommsDat;
#endif

class CLocalHostName : public CBase
	/**
	* Stores a local hostname.
	*
	* The hostnames are stored in separate objects and reference counted.
	* The same hostname can be mapped to multiple network ids.
	*
	*/
	{
	/**
	* Constructor.
	* Private, only used from CLocalHostName::New method, due to somewhat
	* dubious tricks with TLitC class (to store the name).
	*/
	CLocalHostName(const THostName &aName, CLocalHostName *aNext) : iNext(aNext)
		{
		// See _LIT and TLitC for this initializer!!!
		iName.iTypeLength = aName.Length();
		TPtr((TText *)&iName.iBuf[0], aName.Length()).Copy(aName);
		}

public:
	static CLocalHostName *New(const THostName &aName, CLocalHostName *aNext);
	CLocalHostName *iNext;	//< Link to the next name.
	TInt iRefs;				//< The reference count (value 0 implies already one reference).
	TLitC<1> iName;			//< Host name (<1> is *FAKE*!)
	//
	// This is followed by the actual name characters.
	// CHostName class *CANNOT* be subclassed!!!!
	};


CLocalHostName *CLocalHostName::New(const THostName &aName, CLocalHostName *aNext)
	{
	// note: TLitC<1> causes extra allocation of sizeof(TInt) [depends on align
	// settings]. Could try to adjust the extra requested size down by that amount,
	// but it would result more complex code... -- msa
	return new (aName.Size()) CLocalHostName(aName, aNext);
	}

class TLocalHostMap
	{
public:
	TLocalHostMap(TUint32 aId, const CLocalHostName *aName) : iId(aId), iName(aName) {}
	const TUint32 iId;
	const CLocalHostName *iName;
	};

THostNames::~THostNames()
	{
	Cleanup();
	}



void THostNames::Cleanup()
	{
	// note: this does not need to worry about reference counts
	// in hostnames, because everything is cleaned.
	delete iMap;
	iMap = NULL;

	while (iNameList != NULL)
		{
		CLocalHostName *e = iNameList;
		iNameList = e->iNext;
		delete e;
		}
	}

void THostNames::ReferenceRemoved(const CLocalHostName *aName)
	{
	for (CLocalHostName **h = &iNameList, *p; (p = *h) != NULL; h = &p->iNext)
		{
		if (p == aName)
			{
			if (--p->iRefs < 0)
				{
				*h = p->iNext;
				delete p;
				}
			return;
			}
		}
	}


TInt THostNames::Map(TUint32 aId, const TDesC &aName)
	{
	//
	// Locate or create a hostname entry
	//
	TInt err = KErrNoMemory;

	CLocalHostName *name = iNameList;
	for (;;name = name->iNext)
		{
		if (name == NULL)
			{
			name = CLocalHostName::New(aName, iNameList);
			if (name == NULL)
				return KErrNoMemory;
			iNameList = name;
			// The initial iRefs = 0 is ok.
			break;
			}
		else if (aName.Compare(name->iName) == 0)
			{
			// Additional reference to an existing name.
			name->iRefs++;
			break;
			}
		}

	if (iMap != NULL || (iMap = new CArrayFixFlat<TLocalHostMap>(4)) != NULL)
		{
		//
		// Remove old mapping, if any
		//
		for (TInt i = iMap->Count(); i > 0; )
			{
			TLocalHostMap &m = iMap->At(--i);
			if (m.iId == aId)
				{
				// Id already present, just change the name reference (if changed).
				if (m.iName != name)
					{
					LOG(Log::Printf(_L("\tlocalhost unmapped: <%u, %S(%d)>"), m.iId, &m.iName->iName, m.iName->iRefs));
					ReferenceRemoved(m.iName);
					m.iName = name;
					}
				LOG(Log::Printf(_L("\tlocalhost mapped: <%u, %S(%d)>"), m.iId, &m.iName->iName, m.iName->iRefs));
				return KErrNone;
				}
			}
		// Id was not recorded before this, create a new entry.
		TRAP(err, iMap->AppendL(TLocalHostMap(aId, name)));
		if (err == KErrNone)
			{
			LOG(Log::Printf(_L("\tlocalhost mapped: <%u, %S(%d)>"), aId, &name->iName, name->iRefs));
			return KErrNone;
			}
		}
	ReferenceRemoved(name);
	return err;
	}


void THostNames::Unmap(TUint32 aId)
	{
	//
	// Remove old mapping, if any
	//
	if (iMap)
		{
		for (TInt i = iMap->Count(); i > 0; )
			{
			const TLocalHostMap &m = iMap->At(--i);
			if (m.iId == aId)
				{
				LOG(Log::Printf(_L("\tlocalhost unmapped: <%u, %S(%d)>"), m.iId, &m.iName->iName, m.iName->iRefs));
				ReferenceRemoved(m.iName);
				iMap->Delete(i);
				break;	// There can be only one matching id!
				}
			}
		iMap->Compress();
		}
	}

const TDesC &THostNames::Find(TUint32 aId) const
	{
	if (iMap)
		{
		for (TInt i = iMap->Count(); i > 0; )
			{
			const TLocalHostMap &m = iMap->At(--i);
			if (m.iId == aId)
				return m.iName->iName;
			}
		}
	return KNullDesC;
	}

TInt THostNames::Reset(const TDesC &aLocalHost)
	{
	Cleanup();

	// Define the return, when network id is not specified.

	Map(0, aLocalHost);

	// ----------------------------------------------
	// Insert here the code to scan the commdb and,
	// for each pair <nid, name>, call Map(id, name).
	// ----------------------------------------------
	TRAPD(err, GetCommDbDataL(0));

	return err;
	}
	
TInt THostNames::Refresh(TUint32 aId)
	{
	TRAPD(err, GetCommDbDataL(aId));
	return err;
	}


void THostNames::GetCommDbDataL(TUint32 aId)
	{
#if defined(TCPIP6_USE_COMMDB)
#	ifdef HOST_NAME
	CCommsDatabase* db = CCommsDatabase::NewL();
	CleanupStack::PushL(db);

	CCommsDbTableView* view = db->OpenTableLC(TPtrC(NETWORK));
	for (TInt ret = view->GotoFirstRecord(); ret == KErrNone; ret = view->GotoNextRecord())
		{
		THostName name;
		TUint32 nid = 0;
		TRAP(ret,
			view->ReadTextL(TPtrC(HOST_NAME), name);
			view->ReadUintL(TPtrC(COMMDB_ID), nid));
		if (ret == KErrNone)
			Map(nid, name);
		}
	CleanupStack::PopAndDestroy(2);
#endif
#else
	// . (not tested)
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession *dB = CMDBSession::NewLC(KCDVersion1_2);
#else
	CMDBSession *dB = CMDBSession::NewLC(KCDVersion1_1);
#endif


	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	dB->SetAttributeMask( ECDHidden | ECDPrivate );

	CMDBRecordSet<CCDNetworkRecord> *networkTable;
	networkTable = new (ELeave) CMDBRecordSet<CCDNetworkRecord>(KCDTIdNetworkRecord);
	CleanupStack::PushL(networkTable);
	networkTable->LoadL(*dB);
	const TInt totalRecords = networkTable->iRecords.Count();
	for (TInt i = 0; i < totalRecords; ++i)
		{
		const TUint32 nid = (*networkTable)[i]->RecordId();
		if (aId == 0 || nid == aId)
			Map(nid, (*networkTable)[i]->iHostName);	
		}
	CleanupStack::PopAndDestroy(2);		
#endif
	}
