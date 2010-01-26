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

#include "rasdef.h"

_LIT(RAS_DB_FILE,"Ras.ini");
_LIT(RAS_DB_SECTION,"RAS");

CRasDatabase::CRasDatabase(CNtRas* aNtRas)
	: iNtRas(aNtRas)
	{
	__DECLARE_NAME(_S("CRasDatabase"));
	}

CRasDatabase::~CRasDatabase()
	{
	delete iIniFile;
	}

CRasDatabase* CRasDatabase::NewL(CNtRas* aNtRas)
	{
	
	CRasDatabase* p = new (ELeave) CRasDatabase(aNtRas);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop();
	return p;
	}

void CRasDatabase::ConstructL()
	{
	iIniFile = CESockIniData::NewL(RAS_DB_FILE);
	}

TInt CRasDatabase::ReadField(const TDesC& aField, TUint32& aValue)
	{

	TInt temp;
	if(!aField.CompareF(NIF_IFSERVERMODE))
		{
		if(!iIniFile->FindVar(RAS_DB_SECTION, aField, temp))
			temp=0;
		aValue=(TUint32)temp;
		return KErrNone;
		}
	else if(!aField.CompareF(RAS_YESTOCONNECT))
		{
		if(!iIniFile->FindVar(RAS_DB_SECTION, aField, temp))
			temp=0;
		aValue=(TUint32)temp;
		return KErrNone;
		}
	else if(!aField.CompareF(RAS_YESTORECONNECT))
		{
		if(!iIniFile->FindVar(RAS_DB_SECTION, aField, temp))
			temp=0;
		aValue=(TUint32)temp;
		return KErrNone;
		}

	if(!iIniFile->FindVar(RAS_DB_SECTION, aField, temp))
		{
		return KErrNotFound;
		}
	else
		{
		aValue=(TUint32)temp;
		return KErrNone;
		}
	}

TInt CRasDatabase::ReadField(const TDesC& aField, TDes8& aValue)
	{
	TPtrC des;
	if (!iIniFile->FindVar(RAS_DB_SECTION, aField, des))
		return KErrNotFound;

	if (des.Length()>aValue.MaxLength())
		return KErrTooBig;

#ifndef UNICODE
	aValue = des;
#else
	TInt i;
	TInt len;
	TUint8* Dest;
	TUint16* Source;
	len = des.Length();
	Source = CONST_CAST(TUint16*, des.Ptr());
	Dest = CONST_CAST(TUint8*, aValue.Ptr());
	for(i=0;i<len;i++)
		{
		*(Dest+i) = (TUint8)(*(Source+i) & 0xFF);
		}

	aValue.SetLength(len);
#endif
	return KErrNone;
	}

