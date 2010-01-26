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
// esock_params.h
// 
//

#ifndef dummynif_H
#define dummynif_H

#include <e32base.h>
#include <e32std.h>	

#include <comms-infras/metadata.h>
#include <es_prot.h>

class TDummyPref : public Meta::SMetaDataECom
	{
public:
	inline static TDummyPref* NewL();
	inline static TDummyPref* NewL(TInt aAP);
	
	inline TDummyPref();
	
	enum
	{
		EUid = 0x08545209,
		ETypeId = 2
	};

	inline void setAP(TInt aAP);
	inline TInt AP();
		
	
protected:
	DATA_VTABLE
	
	TInt iAP;
	};

inline TDummyPref* TDummyPref::NewL()
	{
	Meta::STypeId input = Meta::STypeId::CreateSTypeId(TDummyPref::EUid,TDummyPref::ETypeId);
	return static_cast<TDummyPref*>(Meta::SMetaDataECom::NewInstanceL(input));
	}

inline TDummyPref* TDummyPref::NewL(TInt aAP)
	{
	TDummyPref* self = NewL();
	self->setAP(aAP);
	return self;
	}

inline TDummyPref::TDummyPref()
	{
	}

inline void TDummyPref::setAP(TInt aAP)
	{
	iAP = aAP;
	}

inline TInt TDummyPref::AP()
	{
	return iAP;
	}

class CDummyParamaterFactory : public CBase
/** Connection generic parameter set factory.

@internalComponent
*/
	{
public:
	static SMetaDataECom* NewL(TAny* aConstructionParameters);
	};



#endif