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

#if !defined(__FILEPCAP_H__)
#define __FILEPCAP_H__

#include <e32base.h>
#include <f32file.h>

enum
	{
		EPcapLinkTypeEthernet = 1,
		EPcapLinkTypePpp = 9,
		EPcapLinkTypeIp = 12
	};
	
/**
Container class holding a single libpcap packet record.
*/
class TPcapRecord
	{
public:
	/** Length of longest packet that can be read */
	enum { KMaxPacketSize = 2048 };
	typedef TBuf8<KMaxPacketSize> TPcapBuf;
	
	TPcapRecord();
	~TPcapRecord();

	// Current record header getters
	inline TUint32 Length() const;
	//TTime Time() const;

	// Current record header putters
	//void SetTime(TTime aTime);
	inline void SetData(const TDesC8& aData);
	inline void SetTime(TUint32 aSec, TUint32 aUsec);
	inline void SetLength(TUint32 aLength);
	inline void SetLinkType(TUint32 aLinkType);

	/** Current record data getter */
	inline TPcapBuf& Data();

	/** Data link type */
	inline TUint32 LinkType() const;

private:
	/** Data link type (LINKTYPE_*) */
	TUint32 iLinkType;

	/** Time stamp - secs */
	TUint32 iSec;
	
	/** Time stamp - microsecs */
	TUint32 iUsec;
	
	/** Total length of the current packet.	Only a portion may have been captured. */
	TUint32 iPacketLen;
	
	/** Captured current packet data */
	TPcapBuf iData;
	};


/**
Class to read libpcap format data files.
*/
class CFilePcap : public CBase
	{
public:
	~CFilePcap();
	static CFilePcap* NewL();
	static CFilePcap* NewLC();
	void ConstructL();
	
	/** Open the file and read the file header */
	TInt Open(const TDesC& aFileName);
	
	/** Rewind to the beginning of the file */
	void Rewind(void);
	
	/** Read the next record from the file */
	TInt ReadRecord(TPcapRecord& aRec);
	
	// File header getters
	inline TUint16 VersionMajor() const;
	inline TUint16 VersionMinor() const;
	inline TUint32 ThisZone() const;	/* gmt to local correction */
	inline TUint32 SigFigs() const;	/* accuracy of timestamps */
	inline TUint32 SnapLen() const;	/* max length saved portion of each pkt */
	inline TUint32 LinkType() const;	/* data link type (LINKTYPE_*) */

	
private:
	// File header
	TUint16 iVersionMajor;
	TUint16 iVersionMinor;
	TUint32 iThisZone;	/* gmt to local correction */
	TUint32 iSigFigs;	/* accuracy of timestamps */
	TUint32 iSnapLen;	/* max length saved portion of each pkt */
	TUint32 iLinkType;	/* data link type (LINKTYPE_*) */

	/** libpcap format file handle */
	RFile iFile;
	
	/** File server session */
	RFs iFsSession;
	
	};

#include "filepcap.inl"

#endif
