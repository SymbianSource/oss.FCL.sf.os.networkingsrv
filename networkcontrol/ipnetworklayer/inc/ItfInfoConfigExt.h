/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file for the Default SubConnection Flow
* 
*
*/



/**
 @file itfinfoConfigExt.h
*/

#ifndef ITFINFOCONFIGEXT_H_INCLUDED_
#define ITFINFOCONFIGEXT_H_INCLUDED_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/metadata.h>
#include <es_enum.h>
#include <nifman.h>
#include <in_iface.h>
#include <elements/rworkerlock.h>

const TUint KIpProtoConfigExtUid = 0x102732E7;

enum TIpProtoConfigExtTypes
	{
	EItfInfoConfigExt = 0,
	EInterfaceName,
	};

class TItfInfoConfigExt : public Meta::SMetaData
/** 

@internalTechnology
@released Since 9.4
*/
	{
public:
    enum {ETypeId = EItfInfoConfigExt, EUid = KIpProtoConfigExtUid, EMaxProtocolListSize = 50 };
        
	explicit TItfInfoConfigExt(const TConnectionInfo& aConnectionInfo)
	:	iConnectionInfo(aConnectionInfo)
		{
		}

	DATA_VTABLE
	TConnectionInfo iConnectionInfo;
	TBuf8<EMaxProtocolListSize> iProtocolList;
	};

class XInterfaceNames : public Meta::SMetaData
	{
public:
	static XInterfaceNames* NewL();

	~XInterfaceNames();

	TInt InterfaceName(TUint aIndex, TInterfaceName& aInterfaceName);
	void AddInterfaceNameL(const TAny* aOwner, const TInterfaceName& aInterfaceName);
	void RemoveInterfaceName(const TAny* aOwner);

private:
	XInterfaceNames() { }
	void ConstructL();

private:
    struct TNameAndOwner
        {
        TNameAndOwner(const TAny* aOwner, const TInterfaceName& aInterfaceName)
        :iOwner(aOwner),
         iName(aInterfaceName)
        {}
         
        const TAny*    iOwner;
        const TInterfaceName iName;
        };
	RArray<TNameAndOwner> iInterfaceNames;
	RWorkerLock iLock;

public:
	DATA_VTABLE
	};

#endif


