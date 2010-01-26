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

#ifndef __DUMPER_H__
#define __DUMPER_H__

#include <e32base.h>
#include <e32std.h>
#include <f32file.h>

const TInt KDumpFileHeaderMaxSizeAllowed			= 24;//the header of tcpdump file is 24 bytes long
const TInt KDumpFileRecordHeaderMaxSizeAllowed		= sizeof(TUint32)*4;//16 bytes
const TUint32 KTcpDumpMagicNumber					= 0xa1b2c3d4 ;
const TUint16 KMajorVersion							= 0x02;
const TUint16 KMinorVersion							= 0x04;
const TUint32 KThisZone								= 0x00;
const TUint32 KSigfgs								= 0x00;
const TUint32 KSnapLen								= 0xffff;
const TUint32 KLinkType								= 12;

class CPktLogger : public CBase
/**
Creates a binary file that could be read by EtherReal.
All the packets are then logged in this binary file.
File Name : iphookslog.tcpdump
Folder : C:\logs\HookLogs 

The binary file generated follows the libpcap format:

		File Header (24 bytes)

		Record Header(16 bytes) # 1
		Packet Payload

		Record Header(16 bytes)# 2
		Packet Payload

		Record Header(16 bytes)# 3
		Packet Payload

		Record Header(16 bytes)# n
		Packet Payload
*/
	{
public:
	CPktLogger();
	~CPktLogger();

	void ConstructL();
	static CPktLogger* NewL();
	static CPktLogger* NewLC();

	/**
	Create the file header for the tcpdump file.
	The file header consists of the following fields:

	·	Magic number (32-bits)
	·	Major version (16-bits)
	·	Minor version (16-bits)
	·	Time zone offset field (32-bits) 
	·	Time stamp accuracy field (32-bits)
	·	Snapshot length (32-bits)
	·	Link layer type (32-bits)

	Magic number is written in the little endian format (since Symbian runs on little 
	endian ARM processors). It contains the value 0x a1b2c3d4. 

	The major version contains the value 2.

	The minor version contains the value 4.

	The time zone offset and time stamp accuracy fields contain the value zero 
	(since it is not used by the ethereal).

	The snapshot length field should be the maximum number of bytes per packet 
	that will be captured. In the example code, since we are capturing the entire packet 
	length, it is set to be 0xFFFF (65535). 

	The link-layer type contains the value 12. 12 signifies that we are capturing the 
	raw IP packets. 
	*/
	void DumpFileHeader();

	/**
	Dump the record packet header.
	Packet Header is 4*4bytes = 16bytes long. It contains following info:

	(I) a time stamp that consists consisting of two things:
		1. a UNIX-format time-in-seconds when the packet was captured, i.e. the number 
		of seconds since January 1, 1970, 00:00:00 GMT (that's GMT, *NOT* local time!)
		2. the number of microseconds since that second when the packet was captured

	(II) a 32-bit value giving the number of bytes of packet data that were captured

	(III) a 32-bit value giving the actual length of the packet, in bytes (which may 
		be greater than the previous number, if you're not saving the entire packet).

	All those numbers must be in the same byte order as the numbers in the file header.
	*/
	void DumpLibPCapRecordHeader( TUint32 aPktLenCaptured, TUint32 aActualPktLen );

	/**
	After the file header, the actual frames are dumped in binary format. Thus, each 
	frame consists of a frame header followed by the raw bytes of the frame.
	*/
	void DumpLibPCapRecordData( const TDesC8& aPacket );

private:
	TTime	iTimeOrigin; //used for logging only

	RFs iFileSvrSession;
	RFile iFileObj;
	};


#endif
