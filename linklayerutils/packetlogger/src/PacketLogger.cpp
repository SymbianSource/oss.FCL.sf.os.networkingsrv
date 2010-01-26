// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements CPacketLogger.
// 
//

/**
 @file
*/


#include <es_sock.h>
#include <hal.h>
#include "PacketLogger.h"

static const TInt KInternalBufferSize = 4096;

/** Constants for tcpdump log*/
static const TUint32 KTcpDumpFileHeaderMagic = 0xa1b2c3d4;
static const TUint16 KTcpDumpVersionMajor = 2;
static const TUint16 KTcpDumpVersionMinor = 4;

/** Constants for pppdump log */
//static const TInt8 KPppDumpRecSentData = 1;  ENABLE IF NEEDED
//static const TInt8 KPppDumpRecRcvData = 2;   ENABLE IF NEEDED
//static const TInt8 KPppDumpRecRcvDelim = 4;  ENABLE IF NEEDED
static const TInt8 KPppDumpRecTimeStepLong = 5;
static const TInt8 KPppDumpRecTimeStepShort = 6;
static const TInt8 KPppDumpRecResetTime = 7;

/**
 * Factory method for CPacketLogger.
 * 
 * @param aTag The tag name for the log
 * @param aFileName The filename to log to.
 * @param aDumpType The type of dump (one of TDumpType values)
 * @param aLinkType Used by tcpdump. One of the types defined in
 * libpcap/bpf/net/bpf.h
 * In case of ETcpDump this is *TUint32
 * which represents the linktype as defined by TcpDump format.
 * In case of EPppDump it is *TUint8 which represents the direction.
 * @return Ownership of a new CPacketLogger.
 */
EXPORT_C CPacketLogger* CPacketLogger::NewL(const TDesC8& aTag, const TDesC8& aFileName, const TDumpType aDumpType, const TInt aLinkType)
	{
	CPacketLogger* self = new(ELeave)CPacketLogger;
	CleanupStack::PushL(self);
	self->ConstructL(aTag, aFileName, aDumpType, aLinkType);
	CleanupStack::Pop(self);
	return self;
	}
	
/**
 * 2nd-phase construction. Creates the libpcap file. Adds the 
 * file header based on the dump type.
 * 
 * @param aTag The tag name for the log
 * @param aFileName The filename to log to.
 * @param aDumpType The type of dump (one of TDumpType values)
 * @param aLinkType Used by tcpdump. (Valid types are specified 
 * in libpcap/bpf/net/bpf.h)
 */
#ifdef __FLOG_ACTIVE
void CPacketLogger::ConstructL(const TDesC8& aTag, const TDesC8& aFileName, const TDumpType aDumpType, const TInt aLinkType)
#else
void CPacketLogger::ConstructL(const TDesC8&, const TDesC8&, const TDumpType aDumpType, const TInt aLinkType)
#endif
	{
	TInt tickPeriod;
	iPacketCounter = 0;
	iDumpType = aDumpType;
	//initialize the time related data
	HAL::Get(HALData::ESystemTickPeriod, tickPeriod);
	iTickPeriod = tickPeriod;
	iTimeLastPacket = User::TickCount();
	iTimeLastPacket *= iTickPeriod;
	//initialize intermal buffer
	iHBuf = HBufC8::NewMaxL(KInternalBufferSize);
	//write the header
	if(aDumpType == ETcpDump)
		{
		__FLOG_OPEN(aTag, aFileName);
		WriteTcpDumpHeader(aLinkType);
		}
	else if(aDumpType == EPppDump)
		{
		__FLOG_OPEN(aTag, aFileName);
		WritePppDumpHeader();
		}
	}
	
/**
 * Constructor
 */
CPacketLogger::CPacketLogger()
 	{
	}

/**
 * Destructor
 */
EXPORT_C CPacketLogger::~CPacketLogger()
	{
	delete iHBuf;
	iHBuf = NULL;
	
	if(iDumpType == ETcpDump || iDumpType == EPppDump)
		{
		__FLOG_CLOSE;
		}
	}

/**
 * Adds a raw ip packet to the dump file based on the dump format.
 * @param aPacket Packet as RMBufChain
 * @param aDirection Direction of packet. Meaningful only in case of PPP Dump
 */
EXPORT_C void CPacketLogger::WritePacket(const RMBufChain& aPacket, const TUint8 aDirection)
	{
	TInt packetLength = aPacket.Length() - aPacket.First()->Length();

	if (packetLength > KInternalBufferSize)
		{
		_LIT8(ERROR_STR,"Packet after packet %d was larger than 4K. It is skipped.");
		__FLOG_1(ERROR_STR, iPacketCounter);
		//Packet is too big for the internal buffer. Don't dump it.
		return;
		}
		
	TPtr8 packetPtr = iHBuf->Des();
	packetPtr.SetLength(packetLength);
	aPacket.CopyOut(packetPtr, aPacket.First()->Length());
	
	WritePacket(packetPtr, aDirection);
	}

/**
 * Adds a text log to the log file.
 * @param aText text to be added
 */
EXPORT_C void CPacketLogger::WriteText(const TDesC8& aText)
	{
	__FLOG_0(aText);
	}

/**
 * Adds a raw ip packet to the dump file based on the dump format.
 * @param aPacket Packet as buffer
 * @param aDirection Direction of packet. Meaningful only in case of PPP Dump
 */
EXPORT_C void CPacketLogger::WritePacket(const TDesC8& aPacket, const TUint8 aDirection)
	{
	iPacketCounter++;
	if(iDumpType == ETcpDump)
		{
		TcpDumpPacket(aPacket);
		}
	else if(iDumpType == EPppDump)
		{
		PppDumpPacket(aPacket, aDirection);
		}
	}
	
/**
 * Dumps file header in a format compatible with Libcap.
 * Libcap compatible files can be manipulated via
 * utilities such as Mergecap (http://www.ethereal.com/docs/man-pages/mergecap.1.html)
 * Ethereal (http://www.ethereal.com/docs/man-pages/ethereal.1.html)
 * etc. More information can be found from www.ethereal.com
 * tcpdump file header format is as follows
 * @code
 * struct TFileHeader 
 * 	{
 * 	TUint32 magic; 
 * 	TUint16 version_major;
 * 	TUint16 version_minor;
 * 	TInt32  thiszone;	// gmt to local correction
 * 	TUint32 sigfigs;	// accuracy of timestamps
 * 	TUint32 snaplen;	// max length saved portion of each pkt
 * 	TUint32 linktype;	// data link type (LINKTYPE specified in libpcap/bpf/net/bpf.h) 
 * 						// 12 corresponds to raw IP
 * 	};
 * @endcode
 * @param aLinkType Link type for the packet (One of the types specified in
 * libpcap/bpf/net/bpf.h)
 */
void CPacketLogger::WriteTcpDumpHeader(const TInt aLinkType)
	{
	//construct the file header
  	TBuf8<sizeof(TUint32)*5+sizeof(TUint16)*2> fileHeader;
  	fileHeader.SetLength(fileHeader.MaxLength());
  	
	BigEndian::Put32(&fileHeader[0], KTcpDumpFileHeaderMagic);
	BigEndian::Put16(&fileHeader[4], KTcpDumpVersionMajor);
	BigEndian::Put16(&fileHeader[6], KTcpDumpVersionMinor);
	BigEndian::Put32(&fileHeader[8], 0); //gmt to local correction
	BigEndian::Put32(&fileHeader[12], 0); //accuracy of timestamps
	BigEndian::Put32(&fileHeader[16], 0xffff); //max length saved portion of each packet
	BigEndian::Put32(&fileHeader[20], static_cast<TUint32>(aLinkType));
	
	//write the file header
	__FLOG_BINARY(fileHeader);
	}

/**
 * Adds a raw ip packet to the dump file based on tcpdump format.
 * @param aPacket TCP packet to dump
 * tcpdump packet header format is as follows
 * @code
 * struct TTimeval 
 * 	{
 * 	TUint32 tv_sec;     // seconds
 * 	TUint32 tv_usec;    // microseconds
 * 	};
 * struct TPacketHeader 
 * 	{
 * 	TTimeval ts;		// time stamp
 * 	TUint32  caplen;	// length of portion present
 * 	TUint32  len;		// length of this packet (off wire)
 * 	};
 * @endcode
 */
void CPacketLogger::TcpDumpPacket(const TDesC8& aPacket)
	{
	TInt32 nsecs;
	TInt32 secs;
	TInt64 curTime = User::TickCount();
	curTime *= iTickPeriod;
	
	//seconds portion of offset
	TInt64 timeInSeconds = curTime / 1000000;
	//microsends portion of offset
	TInt64 timeInNSeconds;
	I64DIVMOD(curTime, 1000000, timeInNSeconds);
	
	secs = I64INT(timeInSeconds);
	nsecs = I64INT(timeInNSeconds);
	iTimeLastPacket = curTime;
	
	//construct the packet header
  	TBuf8<sizeof(TUint32)*4> packetHeader;
  	packetHeader.SetLength(packetHeader.MaxLength());

	BigEndian::Put32(&packetHeader[0], secs); //seconds for time stamp
	BigEndian::Put32(&packetHeader[4], nsecs); //microseconds for time stamp
	BigEndian::Put32(&packetHeader[8], aPacket.Length()); //length of portion present
	BigEndian::Put32(&packetHeader[12], aPacket.Length()); //length of this packet
  	
	//dump the packet header
	__FLOG_BINARY(packetHeader);

	//dump the packet
	__FLOG_BINARY(aPacket);
	}

/**
 * Dumps file header in a format compatible with pppdump.
 * As a file header just drop a 0x07|t3|t2|t1|t0 record followed by a
 * 0x05|t3|t2|t1|t0 record, that seems common practice.
 * Since this "header" is actually a regular record, it can be
 * appended to an existing file.
 */
void CPacketLogger::WritePppDumpHeader()
	{
	_LIT(Ktime_tOrigin,"19700000:000000.000000");
	TTime time_t_Origin(Ktime_tOrigin);
	TTimeIntervalSeconds secs;
	TTime timeNow;
	timeNow.UniversalTime();
	timeNow.SecondsFrom(time_t_Origin,secs);
	
	TBuf8<10> fileHeader;
	fileHeader.SetLength(fileHeader.MaxLength());
	
	fileHeader[0] = KPppDumpRecResetTime;
	BigEndian::Put32(&fileHeader[1], secs.Int());

	fileHeader[5] = KPppDumpRecTimeStepLong;
	BigEndian::Put32(&fileHeader[6], 0);
	
	//dump the file header
	__FLOG_BINARY(fileHeader);
	}

/**
 * Dumps a packet in a pppdump format (see www.ethereal.com)
 * @param aPacket Raw IP packet to dump
 * For each record the format is:
 * @code
 * 0x07|t3|t2|t1|t0	Reset time    t = time_t (UNIX: secs since 01/01/1970)
 * 0x06|t0			Time step (short) - ts = time step (tenths)
 * 0x05|t3|t2|t1|t0	Time step (short) - ts = time step (tenths)
 * 0x04			Receive deliminator (not seen in practice)
 * 0x02|n1|n0		Received data	- n = number of bytes following
 * 0x01|n1|n0		Sent data		- n = number of bytes following
 * @endcode
 * Byte ordering of the header is little endian
 * Byte ordering of the packet is network byte order (big endian)
 * 
 * @param aPacket PPP packet as buffer
 * @param aDirection Direction of packet.
 */
void CPacketLogger::PppDumpPacket(const TDesC8& aPacket, const TUint8 aDirection)
	{
	TInt64 curTime = User::TickCount();
	curTime *= iTickPeriod;
	//offset in microseconds
	TInt64 timeOffset = curTime - iTimeLastPacket;
	//seconds portion of offset
	TInt64 timeInTenthOfSeconds = timeOffset / 100000;
	TUint32 timeStep = I64INT(timeInTenthOfSeconds);
	
	if(timeStep)
		{
		iTimeLastPacket = curTime;
		}
	
	TBuf8<2+6> recordHeader;
	int i=0;
	if (timeStep > 0xff)
		{
		// 32-bit differential time record
		recordHeader.SetLength(8);
		recordHeader[i++] = KPppDumpRecTimeStepLong;
		BigEndian::Put32(&recordHeader[i], timeStep);
		i += 4;
		}
	else if (timeStep > 0)
		{
		// 8-bit differential time record
		recordHeader.SetLength(5);
		recordHeader[i++] = KPppDumpRecTimeStepShort;
		recordHeader[i++] = (TUint8) timeStep;
		}
	else
		{
		recordHeader.SetLength(3);
		}
	recordHeader[i++] = aDirection;
	BigEndian::Put16(&recordHeader[i], (TUint16)aPacket.Length());
	
	//dump the packet header
	__FLOG_BINARY(recordHeader);
	
	//dump the packet
	__FLOG_BINARY(aPacket);
	}
