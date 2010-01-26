/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Networking packet logging utility
* 
*
*/



/**
 @file
*/

#ifndef __PACKETLOGGER_H__
#define __PACKETLOGGER_H__

//this file defines the __FLOG_ACTIVE flag
#include <comms-infras/commsdebugutility.h>

#if defined __FLOG_ACTIVE

#include <e32base.h>
#include <es_mbuf.h>
class RFileLogger;

/**
 * Class containing methods for dumping ip
 * packets of different formats.
 * The output is written to the log file 
 * produced by comms debug utility.
 * The supported output formats are listed with the 
 * TDumpType enum.
 * This class cannot be derived from.
 * This class should be active only if __FLOG_ACTIVE
 * flag is defined. To ensure that use the macros
 * defined below. A sample usage would be:
 * @code
 *   __PACKETLOG_DECLARATION_MEMBER
 *   __PACKETLOG_NEWL(aTag, aFileName, aDumpType, aLinkType);
 *   ...
 *   __PACKETLOG_WRITE_PACKET(aPacket, aDirection);
 *   ...
 *   __PACKETLOG_DELETE;
 * @endcode
 * If the __FLOG_ACTIVE flag is not set, the macros map
 * to nothing.
 */
class CPacketLogger : public CBase
	{
public:
	/** Output dump formats supported by this class */
	enum TDumpType
		{
		/** Produced dump will be in tcp dump format */
		ETcpDump,
		/** Produced dump will be in ppp dump format */
		EPppDump
		};

public:
	IMPORT_C static CPacketLogger* NewL(const TDesC8& aTag, const TDesC8& aFileName, const TDumpType aDumpType, const TInt aLinkType);
	IMPORT_C ~CPacketLogger();
	IMPORT_C void WritePacket(const RMBufChain& aPacket, const TUint8 aDirection);
	IMPORT_C void WritePacket(const TDesC8& aPacket, const TUint8 aDirection);
	IMPORT_C void WriteText(const TDesC8& aText);

private:
	void ConstructL(const TDesC8& aTag, const TDesC8& aFileName, const TDumpType aDumpType, const TInt aLinkType);
	CPacketLogger();

private:
	void WriteTcpDumpHeader(const TInt aLinkType);
	void TcpDumpPacket(const TDesC8& aPacket);
	
	void WritePppDumpHeader();
	void PppDumpPacket(const TDesC8& aPacket, const TUint8 aDirection);

private:
	/** Specifies the output format for the dumps */
	TDumpType iDumpType;
	/** Marks the start time when the last packet was received for dumping */
	TInt64 iTimeLastPacket;
	/** The time between system ticks, in microseconds. */
	TInt64 iTickPeriod;
	/** Counter for the number of packets successfully dumped */
	TUint32 iPacketCounter;
	/** Internal buffer to optimize memory allocation for WritePacket(const RMBufChain& aPacket, const TUint8 aDirection)*/
	HBufC8* iHBuf;
	/** Used for directing output to comms debug utility */
	__FLOG_DECLARATION_MEMBER;
	};

#define __PACKETLOG_DECLARATION_MEMBER	   CPacketLogger* __packetLogger__

#define __PACKETLOG_NEWL(aTag, aFileName, aDumpType, aDumpFormat) __packetLogger__ = CPacketLogger::NewL(aTag, aFileName, aDumpType, aDumpFormat)

#define __PACKETLOG_DELETE  delete __packetLogger__; __packetLogger__ = NULL

#define __PACKETLOG_WRITE_PACKET(aPacket, aDirection) __packetLogger__->WritePacket(aPacket, aDirection)

#define __PACKETLOG_LOG(aText) __packetLogger__->WriteText(aText)

#else //__FLOG_ACTIVE

#define __PACKETLOG_DECLARATION_MEMBER   TInt32 __noLogger__

#define __PACKETLOG_NEWL(aTag, aFileName, aDumpType, aDumpFormat)

#define __PACKETLOG_DELETE

#define __PACKETLOG_WRITE_PACKET(aPacket, aDirection)

#define __PACKETLOG_LOG(aText) __packetLogger__->WriteText(aText)

#endif //__FLOG_ACTIVE

#endif // __PACKETLOGGER_H__
