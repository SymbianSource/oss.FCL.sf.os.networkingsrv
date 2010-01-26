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

/**
 
 Gets the original packet length.
 
 @return Length of the original packet 
*/
inline TUint32 TPcapRecord::Length() const
{
	return iPacketLen;
}

/**
Sets the record time.

@param aSec UNIX epoc time
@param aUsec Microseconds past aSec
*/
inline void TPcapRecord::SetTime(TUint32 aSec, TUint32 aUsec)
{
	iSec = aSec;
	iUsec = aUsec;
}

/**
Sets the original packet length.

@param aLength Length of the original packet 
*/
inline void TPcapRecord::SetLength(TUint32 aLength)
{
	iPacketLen = aLength;
}

/**
Copies in the original packet data.

@param aData Reference to original packet data 
*/
inline void TPcapRecord::SetData(const TDesC8& aData)
{
	iData = aData;
}

/**
Sets the libpcap link type for this packet.

@param aLinkType libpcap data link type
*/
inline void TPcapRecord::SetLinkType(TUint32 aLinkType)
{
	iLinkType = aLinkType;
}

/**
Gets the current packet data.
Data().Length() might not be equal to Length() if only a portion of the packet
was captured.

@return Reference to current packet data
*/
inline TPcapRecord::TPcapBuf& TPcapRecord::Data()
{
	return iData;
}

/**
Gets the packet data link type.

@return libpcap data link type
*/
inline TUint32 TPcapRecord::LinkType() const
{
	return iLinkType;
}


/**
Gets the major version number of the libpcap file.

@return Major version number
*/
inline TUint16 CFilePcap::VersionMajor() const
{
	return iVersionMajor;
}

/**
Gets the minor version number of the libpcap file.

@return Minor version number
*/
inline TUint16 CFilePcap::VersionMinor() const
{
	return iVersionMinor;
}

/**
Gets the time zone of the time stamps in the libpcap file.

@return Difference from UTC
*/
inline TUint32 CFilePcap::ThisZone() const
{
	return iThisZone;
}

/**
Gets the number of significant figures in the timestamps in the libpcap file.

@return Number of significant fitures
*/
inline TUint32 CFilePcap::SigFigs() const
{
	return iSigFigs;
}

/**
Gets the maximum length of the packets in the libpcap file.
Packets longer than this have been truncated.

@return Maximum length of packets
*/
inline TUint32 CFilePcap::SnapLen() const	/* max length saved portion of each pkt */
{
	return iSnapLen;
}

/**
Gets the link type of the packets in the libpcap file.

@return libpcap link type code
*/
inline TUint32 CFilePcap::LinkType() const	/* data link type (LINKTYPE_*) */
{
	return iLinkType;
}


