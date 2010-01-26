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
// MPBUTIL.CPP
// 
//

#include "ETELMM.H"		// for RMobilePhoneBookStore
#include "mpbutil.h"

const TUint8 KTagPadZeroValue = 0x00;

EXPORT_C CPhoneBookBuffer::CPhoneBookBuffer() : iMonitor(NULL,0,0)
	{
	}

EXPORT_C void CPhoneBookBuffer::Set(TDes8* aBuf)
/*
 * This method sets iPtr to point to supplied data buffer.
 */
	{
	iPtr = aBuf;
	}

EXPORT_C TInt CPhoneBookBuffer::AddNewEntryTag()
/**
 * This method adds the new-entry tag to the buffer and sets iMonitor to point to the
 * beginning of each new entry.
 */
	{
	// First check whether there is enough space to fit new entry tag
	if( (iPtr->MaxLength() - iPtr->Length()) > 1 )
		{
		AppendInt8(RMobilePhoneBookStore::ETagPBNewEntry);
		const TInt len = iPtr->Length();
		//iMonitor.Set(&iPtr[len-1], iPtr->MaxLength() - len, iPtr->MaxLength() - len); 
		iMonitor.Set(&(*iPtr)[len-1], iPtr->MaxLength() - len, iPtr->MaxLength() - len);
		return KErrNone;
		}
	else
		{
		return KErrOverflow;
		}
	}

EXPORT_C TInt CPhoneBookBuffer::PutTagAndValue(TUint8 aTagValue, TUint8 aInteger)
/**
 * This overloaded method adds a tag and an integer value to the buffer.
 */
	{
	// First need to ensure that aInteger can fit into the supplied buffer
	if((iPtr->MaxLength()-iPtr->Length()) > 2)
		{
		AppendInt8(aTagValue);
		AppendInt8(aInteger);
		return KErrNone;
		}
	else
		{
		return KErrOverflow;
		}
	}

EXPORT_C TInt CPhoneBookBuffer::PutTagAndValue(TUint8 aTagValue, TUint16 aInteger)
/**
 * This overloaded method adds a tag and an integer value to the buffer.
 */
	{
	// First need to ensure that aInteger can fit into the supplied buffer
	if((iPtr->MaxLength()-iPtr->Length()) > 3)
		{
		AppendInt8(aTagValue);
		AppendInt16(aInteger);
		return KErrNone;
		}
	else
		{
		return KErrOverflow;
		}
	}

EXPORT_C TInt CPhoneBookBuffer::PutTagAndValue(TUint8 aTagValue, TUint32 aInteger)
/**
 * This overloaded method adds a tag and an integer value to the buffer.
 */
	{
	// First need to ensure that aInteger can fit into the supplied buffer
	if((iPtr->MaxLength()-iPtr->Length()) > 5)
		{
		AppendInt8(aTagValue);
		AppendInt32(aInteger);
		return KErrNone;
		}
	else
		{
		return KErrOverflow;
		}
	}

EXPORT_C TInt CPhoneBookBuffer::PutTagAndValue(TUint8 aTagValue, const TDesC16 &aData)
/**
* This method adds a tag and a 16-bit descriptor into the buffer.
  */
	{
	// First need to ensure that aData can fit into the supplied buffer
	// Need to use Size() because this is Unicode data and we want to know number of bytes not number of characters
	// Also allow for the 1-byte tag and up to 3 zero padding bytes
	if((aData.Size()+3 <= KMaxUint16Val) && ((iPtr->MaxLength()-iPtr->Length()) > (aData.Size()+4)))
		{
		// Make sure buffer is word aligned after adding 3 bytes for tag and length
		switch(iPtr->Size()%4)
			{
		case 0:
			AppendInt8(KTagPadZeroValue);
			break;
		case 1:
			break;
		case 2:
			AppendInt8(KTagPadZeroValue);
			AppendInt8(KTagPadZeroValue);
			AppendInt8(KTagPadZeroValue);
			break;
		case 3:
			AppendInt8(KTagPadZeroValue);
			AppendInt8(KTagPadZeroValue);
			break;
		default:
			break;
			}

		AppendInt8(aTagValue);
		TUint16 len=(TUint16) aData.Size();
		iPtr->Append((const TUint8*)&len,2);
		TUint8* dataPtr=reinterpret_cast<TUint8*>(const_cast<TUint16*>(aData.Ptr()));
		iPtr->Append(dataPtr,len);
		return KErrNone;
		}
	else 
		{
		return KErrOverflow;
		}	
	}

EXPORT_C TInt CPhoneBookBuffer::PutTagAndValue(TUint8 aTagValue, const TDesC8 &aData)
/**
 * This method adds a tag and a 8-bit descriptor into the buffer.
 */
	{
	// First need to ensure that aData can fit into the supplied buffer
	if((aData.Size() <= KMaxUint16Val) && ((iPtr->MaxLength()-iPtr->Length()) > (aData.Size()+1)))
		{
		AppendInt8(aTagValue);
		TUint16 len=(TUint16) aData.Length();
		iPtr->Append((const TUint8*)&len,2);
		iPtr->Append(aData);
		return KErrNone;
		}
	else 
		{
		return KErrOverflow;
		}	
	}

EXPORT_C TInt CPhoneBookBuffer::RemovePartialEntry()
/*
 * This method removes partially populated entry from the TLV buffer. Note that
 * this method can also be used to remove a complete entry from the buffer.
 */
	{
	iPtr->SetLength(iPtr->MaxLength() - iMonitor.Length());
	iMonitor.FillZ();
	return KErrNone;
	}

EXPORT_C void CPhoneBookBuffer::StartRead()
/*
 * This method sets iRead to point to populated TLV buffer.
 */
	{
	iRead.Set(*iPtr);
	}

EXPORT_C TInt CPhoneBookBuffer::GetTagAndType(TUint8 &aTagValue, TPhBkTagType &aDataType)
/**
 * This method reads the next tag value and sets the data type (i.e. TInt8, TInt16, TInt32 
 * or Des).
 */
	{
	// Check we've not reached end of buffer - if we have then return
	TInt length = iRead.Length();
	
	if (length<=0)
		return KErrNotFound;

	// Extract all padding zero bytes until tag is found
	TInt i=0;
	do
		{
		aTagValue=iRead[i++];
		}
	while ((aTagValue==KTagPadZeroValue) && i<length);

	if (i < length)
		iRead.Set(iRead.Mid(i));
	else
		return KErrNotFound;

	switch(aTagValue) // set tag type according to the tag value
		{
		case RMobilePhoneBookStore::ETagPBNewEntry:
			aDataType = EPhBkTypeNoData;
			break;

		case RMobilePhoneBookStore::ETagPBTonNpi:
			aDataType = EPhBkTypeInt8;
			break;

		case RMobilePhoneBookStore::ETagPBAdnIndex: 
			aDataType = EPhBkTypeInt16; 
			break;

		case RMobilePhoneBookStore::ETagPBText:
		case RMobilePhoneBookStore::ETagPBNumber:
			aDataType = EPhBkTypeDes16; 
			break;

		default:
			return KErrNotFound;
		}
	return KErrNone;
	}

EXPORT_C TInt CPhoneBookBuffer::GetValue(TUint16 &aInteger)
/**
 * This method reads the TUint16 integer value pointed to by the read pointer.
 * The read pointer is then incremented passed the read values.
 */
	{
	aInteger=(TUint16)((iRead[1]<<8)+iRead[0]);		// Dependant upon endianess
	iRead.Set(iRead.Mid(2));
	return KErrNone;
	}

EXPORT_C TInt CPhoneBookBuffer::GetValue(TUint8 &aInteger)
/**
 * This method reads the TUint16 integer value pointed to by the read pointer.
 * The read pointer is then incremented passed the read values.
 */
	{
	aInteger=(TUint8)(iRead[0]);		
	iRead.Set(iRead.Mid(1));
	return KErrNone;
	}

EXPORT_C TInt CPhoneBookBuffer::GetValue(TUint32 &aInteger)
/**
 * This method reads the TUint16 integer value pointed to by the read pointer.
 * The read pointer is then incremented passed the read values.
 */
	{
	TUint16 aMSW(0), aLSW(0);
	GetValue(aLSW);
	GetValue(aMSW);
	aInteger=(TUint32)((aMSW<<16)+aLSW);		// Dependant upon endianess
	return KErrNone;
	}

EXPORT_C TInt CPhoneBookBuffer::GetValue(TPtrC8 &aData)
/**
 * This method reads the 8-bit descriptor pointed to by the read pointer.
 * The read pointer is then incremented passed the read value.
 */
	{
	TUint16 len;
	len=(TUint16)((iRead[1]<<8)+iRead[0]);		// Dependant upon endianess
	aData.Set(iRead.Mid(2,len));
	iRead.Set(iRead.Mid(len+2));
	return KErrNone;
	}

EXPORT_C TInt CPhoneBookBuffer::GetValue(TPtrC16 &aData)
/**
 * This method reads the 16-bit descriptor pointed to by the read pointer.
 * The read pointer is then incremented passed the read value.
 */
	{
	TUint16 size=(TUint16)((iRead[1]<<8)+iRead[0]);		// Dependant upon endianess
	TUint16 len=(TUint16)(size/2);

	iRead.Set(iRead.Mid(2));

	TUint16* dataPtr=reinterpret_cast<TUint16*>(const_cast<TUint8*>(iRead.Ptr()));

	aData.Set(dataPtr,len);
	
	iRead.Set(iRead.Mid(size));
	return KErrNone;
	}

EXPORT_C TInt CPhoneBookBuffer::BufferLength()
/**
 * This method gets the populated buffer length.
 */
	{
	return iPtr->Length();
	}

EXPORT_C TInt CPhoneBookBuffer::RemainingReadLength()
/**
 * This method gets length of the remaining data to read
 */
	{
	return iRead.Length();
	}

EXPORT_C void CPhoneBookBuffer::SkipValue(TPhBkTagType aDataType)
	{
	TUint16 size=0;
	switch (aDataType)
		{
	case EPhBkTypeInt8:
		iRead.Set(iRead.Mid(1));
		break;
	case EPhBkTypeInt16:
		iRead.Set(iRead.Mid(2));
		break;
	case EPhBkTypeInt32:
		iRead.Set(iRead.Mid(4));
		break;
	case EPhBkTypeDes8:
	case EPhBkTypeDes16:
		size=(TUint16)((iRead[1]<<8)+iRead[0]);		// Dependant upon endianess
		iRead.Set(iRead.Mid(2+size));
		break;
	default:
		// EPhBkTypeNoData caught here - no data to skip, so do nothing
		break;
		}
	}

TInt CPhoneBookBuffer::AppendInt8(TUint8 aInteger)
	{
	const TUint8* intAddr=(const TUint8*)&aInteger;
	iPtr->Append(intAddr,1);
	return KErrNone;
	}

TInt CPhoneBookBuffer::AppendInt16(TUint16 aInteger)
	{
	const TUint8* intAddr=(const TUint8*)&aInteger;
	iPtr->Append(intAddr,2);
	return KErrNone;
	}

TInt CPhoneBookBuffer::AppendInt32(TUint32 aInteger)
	{
	const TUint8* intAddr=(const TUint8*)&aInteger;
	iPtr->Append(intAddr,4);
	return KErrNone;
	}
