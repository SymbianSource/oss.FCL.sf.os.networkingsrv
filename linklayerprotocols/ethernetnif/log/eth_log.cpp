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
// Ethernet libcap format logging
// 
//

/**
 @file
 @internalComponent 
*/

#include "eth_log.h"

#ifdef TCPDUMP_LOGGING

CEthLog::CEthLog() :
	iUnixTimeBase(KUnixTimeBaseDes)
{
}

CEthLog::~CEthLog()
{
	__FLOG_CLOSE; 
}

CEthLog* CEthLog::NewL()
{
	CEthLog* self = new (ELeave) CEthLog;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); // self
	return self;
}

void CEthLog::ConstructL()
{
	iEthTcpDumpLogFileName = KEthTcpDump;
	__FLOG_OPEN(KEthTcpDumpFirstTag,KEthTcpDump);
}

/**
Dumps a packet in a pcap format (see www.ethereal.com)
For each record the format is:
struct timeval - time packet received
number of octects in packet
number of octects from packet in file - ie. the number that we captured.  For this purpose, this will be all of them.
Byte ordering of the header is little endian
Byte ordering of the packet is network byte order (big endian)
@param aCaptureTime The time that the packet was captured
@param RMBufChain& aBuffer Buffer to dump
*/
void CEthLog::DumpFrame(TTime aCaptureTime, RMBufChain& aPdu)
{
	TBuf8<16> recordHeader; // one TInt64 for time, one TUint32 for observed packet length, one TUint32 for captured packet length
	recordHeader.SetLength(recordHeader.MaxLength());

	TInt err;
	// Build the header for this frame
	TTimeIntervalSeconds captureTimeSecs;
	
	err = aCaptureTime.SecondsFrom(iUnixTimeBase, captureTimeSecs);
	if(err)	// if there's an overflow, then stick all zeros in - not good, but at least we got the data
		captureTimeSecs = 0;
		
	TInt captureTimeMicroSecs;
	captureTimeMicroSecs = I64LOW(aCaptureTime.Int64());
	captureTimeMicroSecs = captureTimeMicroSecs % 1000000; // get microseconds component
	
	recordHeader[0]= static_cast<TUint8>((captureTimeSecs.Int() & 0x000000ff));
	recordHeader[1]= static_cast<TUint8>((captureTimeSecs.Int() & 0x0000ff00) >> 8);
	recordHeader[2]= static_cast<TUint8>((captureTimeSecs.Int() & 0x00ff0000) >> 16);
	recordHeader[3]= static_cast<TUint8>((captureTimeSecs.Int() & 0xff000000) >> 24);
	recordHeader[4]= static_cast<TUint8>((captureTimeMicroSecs & 0x000000ff));
	recordHeader[5]= static_cast<TUint8>((captureTimeMicroSecs & 0x0000ff00) >> 8);
	recordHeader[6]= static_cast<TUint8>((captureTimeMicroSecs & 0x00ff0000) >> 16);
	recordHeader[7]= static_cast<TUint8>((captureTimeMicroSecs & 0xff000000) >> 24);

	TInt32 dataBytes = aPdu.Length();

	recordHeader[8]= static_cast<TUint8>((dataBytes & 0x000000ff));	// first entry shows the length of the packet
	recordHeader[9]= static_cast<TUint8>((dataBytes & 0x0000ff00) >> 8);
	recordHeader[10]= static_cast<TUint8>((dataBytes & 0x00ff0000) >> 16);
	recordHeader[11]= static_cast<TUint8>((dataBytes & 0xff000000) >> 24);

	recordHeader[12]=recordHeader[8];	// second shows how much of it we've written to the file
	recordHeader[13]=recordHeader[9];
	recordHeader[14]=recordHeader[10];
	recordHeader[15]=recordHeader[11];

	// create a temporary buffer to hold the pdu (flogger needs a descriptor, so we have to un-mbuf the data)
	// could put this on the heap, but start by trying it on the stack because we don't have to do memory allocations that way
	// might be worth having a permanent ~1500 byte HBufC on the heap to reuse here
	TBuf8<KDefaultMtuSetting + KEtherLLCHeaderSize> pdu;
	pdu.SetMax();
	aPdu.CopyOut(pdu);
	
	__FLOG_BINARY((recordHeader));
	__FLOG_BINARY((pdu));
}

/**
Dump file header to show that this is a pcap capture file in pcap format
Format is (from winpcap savefile.c):
hdr.magic = TCPDUMP_MAGIC;
hdr.version_major = PCAP_VERSION_MAJOR;
hdr.version_minor = PCAP_VERSION_MINOR;
hdr.thiszone = thiszone;
hdr.snaplen = snaplen;
hdr.sigfigs = 0;
hdr.linktype = linktype;
@see http://www.ethereal.com/lists/ethereal-dev/199909/msg00124.html for more details
*/
void CEthLog::DumpTcpDumpFileHeader()
{
	const TUint headerSize = 24; // header is 24 bytes long

	TBuf8<headerSize> fileHeader;
	fileHeader.SetLength(fileHeader.MaxLength());

	fileHeader[0] = 0xd4;	// first four bytes - pcap magic number, indicates this is a pcap format file
	fileHeader[1] = 0xc3;
	fileHeader[2] = 0xb2;
	fileHeader[3] = 0xa1;

	fileHeader[4] = 0x02;	// two bytes of major version number
	fileHeader[5] = 0x00;

	fileHeader[6] = 0x04;	// two bytes of minor version number
	fileHeader[7] = 0x00;

	// find the offset from UTC of this device
	TLocale locale;
	TInt32 offset = locale.UniversalTimeOffset().Int();

	fileHeader[8] =	static_cast<TUint8>(((offset & 0xff000000) >> 24));		// four bytes of offset from UTC in seconds
	fileHeader[9] = static_cast<TUint8>(((offset & 0x00ff0000) >> 16));
	fileHeader[10] = static_cast<TUint8>(((offset & 0x0000ff00) >> 8));		
	fileHeader[11] = static_cast<TUint8>((offset & 0x000000ff));

	fileHeader[12] = 0x00;		// four bytes of timestamp accuracy (not sure of units) - just set to zero, no-one seems to use them
	fileHeader[13] = 0x00;
	fileHeader[14] = 0x00;		
	fileHeader[15] = 0x00;

	fileHeader[16] = 0xFF;		// four bytes of maximum snapshot length - we're just going set it to something big
	fileHeader[17] = 0xFF;
	fileHeader[18] = 0x00;		
	fileHeader[19] = 0x00;

	fileHeader[20] = 0x01;		// four bytes of packet capture type (0x01 for 10Mb ethernet (and other speeds, I suspect!))
	fileHeader[21] = 0x00;
	fileHeader[22] = 0x00;		
	fileHeader[23] = 0x00;

	__FLOG_BINARY((fileHeader));
}

#endif
