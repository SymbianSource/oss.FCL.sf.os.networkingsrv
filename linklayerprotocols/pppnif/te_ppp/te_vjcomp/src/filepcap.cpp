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
//

#include "filepcap.h"
#include <es_sock.h>

static const TUint32 KPcapMagic = 0xa1b2c3d4;

TPcapRecord::TPcapRecord()
{
}

TPcapRecord::~TPcapRecord()
{
}

CFilePcap::~CFilePcap()
	{
	iFile.Close();
	iFsSession.Close();
	}

CFilePcap* CFilePcap::NewLC()
	{
	// NewLC with two stage construction
	// get new, leave if can't
	CFilePcap* self=new (ELeave) CFilePcap;
	// push onto cleanup stack in case self->ConstructL leaves
	CleanupStack::PushL(self);
	// complete construction with second phase constructor
	self->ConstructL();
	return self;
	}

/**
Make the connection to the file server.

@leave Error if connection can't be established
*/
void CFilePcap::ConstructL()
	{
	User::LeaveIfError(iFsSession.Connect());
	}

/**
Open the file and read the file header.

@return Error code
*/
TInt CFilePcap::Open(const TDesC& aFileName)
	{
	TInt err = iFile.Open(iFsSession, aFileName, EFileShareReadersOnly);
	if (err != KErrNone)
		{
		    return err;
		}

	TBuf8<sizeof(TUint32) * 6> hdr;
	err = iFile.Read(hdr);
	if (err != KErrNone)
		{
		    return err;
		}
	if (hdr.Length() < hdr.MaxLength())
		{
		    return KErrCorrupt;
		}

	if (LittleEndian::Get32(hdr.Ptr() + 0) != KPcapMagic)
		{
		    return KErrCorrupt;
		}

	iVersionMajor = LittleEndian::Get16(hdr.Ptr() + 4);
	iVersionMinor = LittleEndian::Get16(hdr.Ptr() + 6);
	iThisZone = LittleEndian::Get32(hdr.Ptr() + 8);
	iSigFigs = LittleEndian::Get32(hdr.Ptr() + 12);
	iSnapLen = LittleEndian::Get32(hdr.Ptr() + 16);
	iLinkType = LittleEndian::Get32(hdr.Ptr() + 20);
	
	return err;
	}

/**
Rewind to the beginning of the data portion of the file so that the next
call to ReadRecord() returns the first packet.
*/
void CFilePcap::Rewind(void)
	{
	TInt off = 24;
	iFile.Seek(ESeekStart, off);
	}

/**
Read the next record from the file

@param aRec Object to hold returned packet information
@return error value or KErrNone if no error occurs
*/
TInt CFilePcap::ReadRecord(TPcapRecord& aRec)
	{
	TBuf8<sizeof(TUint32) * 4> hdr;
	
	// Read packet header
	TInt err = iFile.Read(hdr);
	if (err != KErrNone)
		{
		return err;
		}
		
	if (hdr.Length() == 0)
	    {
	    aRec.SetLength(0);
	    return 0;
	    }
	    
	if (hdr.Length() < hdr.MaxLength())
		{
		return KErrCorrupt;
		}

	aRec.SetLinkType(iLinkType);
	aRec.SetTime(LittleEndian::Get32(hdr.Ptr() + 0), LittleEndian::Get32(hdr.Ptr() + 4));
	TInt32 len = LittleEndian::Get32(hdr.Ptr() + 8);
	aRec.SetLength(LittleEndian::Get32(hdr.Ptr() + 12));
	
	// Read data portion of packet
	TPcapRecord::TPcapBuf& data = aRec.Data();
	err = iFile.Read(data, len);
	if (err != KErrNone)
		{
		return err;
		}
	if (data.Length() < len)
		{
		return KErrCorrupt;
		}
	return err;
	}

