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

#ifndef __MPBUTIL_H__
#define	__MPBUTIL_H__

#include "ETELMM.H"		// for RMobilePhoneBookStore
/**
@internalComponent
*/
const TInt KFieldEntrySize = 8;
const TInt KErrStartOfNewEntry = -1;
const TInt KMaxUint16Val = 0xFFFF;

class CPhoneBookBuffer : public CBase
/**
This class enables the conversion from integer or descriptor data type into 
the TLV (Tag-Length-Value) format and vice versa.
@internalComponent
*/
	{
public:
	IMPORT_C CPhoneBookBuffer();
	IMPORT_C void Set(TDes8* aData);
	IMPORT_C TInt AddNewEntryTag();
	IMPORT_C TInt PutTagAndValue(TUint8 aTagValue, TUint8 aInteger);
	IMPORT_C TInt PutTagAndValue(TUint8 aTagValue, TUint16 aInteger);
	IMPORT_C TInt PutTagAndValue(TUint8 aTagValue, TUint32 aInteger);
	IMPORT_C TInt PutTagAndValue(TUint8 aTagValue, const TDesC8 &aData);
	IMPORT_C TInt PutTagAndValue(TUint8 aTagValue, const TDesC16 &aData);
	IMPORT_C TInt RemovePartialEntry();
	IMPORT_C void StartRead();

	enum TPhBkTagType
		{
		EPhBkTypeNoData,
		EPhBkTypeInt8,
		EPhBkTypeInt16,
		EPhBkTypeInt32,
		EPhBkTypeDes8,
		EPhBkTypeDes16,
		};

	IMPORT_C TInt GetTagAndType(TUint8 &aTagValue, TPhBkTagType &aDataType);
	IMPORT_C TInt GetValue(TUint8 &aInteger);
	IMPORT_C TInt GetValue(TUint16 &aInteger);
	IMPORT_C TInt GetValue(TUint32 &aInteger);
	IMPORT_C TInt GetValue(TPtrC8 &aData);
	IMPORT_C TInt GetValue(TPtrC16 &aData);
	IMPORT_C void SkipValue(TPhBkTagType aDataType);
	IMPORT_C TInt BufferLength();
	IMPORT_C TInt RemainingReadLength();

private:
	TInt AppendInt8(TUint8 aInteger);
	TInt AppendInt16(TUint16 aInteger);
	TInt AppendInt32(TUint32 aInteger);

private:
	TDes8*  iPtr;   // Points to supplied Data buffer
	TPtrC8 iRead;   // Points to populated TLV buffer
	TPtr8 iMonitor; // Used to remove partially populated entries from TLV buffer
	};

#endif
