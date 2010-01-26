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

#include "PPPLOG.H"

#if defined (ESOCK_LOGGING_ACTIVE)

#include "PPPHDLC.H"
#include "ncpip.h"
#include "ncpip6.h"
#include "VJ.H"
#include "PPPCCP.H"

#include "PPPCHAP.H"

static const TUint KDnsPort = 53;

_LIT(KPppLogFolder,"Ppp");
_LIT(KPppLogFile,"Ppp.txt");	
_LIT8(KTcpDumpFirstTag,"TcpDump");	
_LIT8(KPppTcpDump,"TcpDump.log");	

_LIT(KEndOfLine,"\n");
_LIT(KStateString1,"%s State -> %s");
_LIT(KStateString2,"%s State %s -> %s");
_LIT(KPhaseString1,"PPP Phase -> %s");
_LIT(KPhaseString2,"PPP Phase %s -> %s");
_LIT(KCodeTextString,"	%s [0x%02x]");
_LIT(KCodeTextWithoutSpacesString,"	%s [0x%02x]");
_LIT(KIdentifierLengthString,"	Id = 0x%02x, Len = %d");
_LIT(KLengthString,"	Length = %d");
_LIT(KLengthString2,"	    Length = 0x%02x");
_LIT(KTCPLengthString,"	    Length = %d, Hdr len = %d");
_LIT(KUDPLengthPortString,"	    Length = %d, Src port = %d, Dst port = %d");
_LIT(KBytesRemainingString,"        %d byte(s) remaining");
_LIT(KLcpCodeString,"	Code = %s [0x%02x]");
_LIT(KCodeString,"	Code = 0x%02x");
_LIT(KCodeString2,"	Code = 0x%04x");
_LIT(KProtocolString,"	Protocol = 0x%04x");
_LIT(KNumberString,"	Number = 0x%08x");
_LIT(KSecondsString,"	Seconds = %d");
_LIT(KSizeString,"	    Size = %d");
_LIT(KMapString,"	    Map = 0x%08x");
_LIT(KProtocolTextString,"	    Protocol = %s [0x%04x]");
_LIT(KLcpOptFcsTypeString,"	    Types =%s%s%s");
_LIT(KMaximumBytesString,"	    Maximum bytes = %d");
_LIT(KUserNameString,"	Username = \"%S\"");
_LIT(KPasswordString,"	Password = \"%S\"");
_LIT(KMessageString,"	Message = \"%S\"");
_LIT(KDelayString,"	    Delay=%d\n");
_LIT(KNameString,"	Name = \"%S\"");
_LIT(KChangeMaskString,"	Change Mask = 0x%x: ");
_LIT(KChecksumString,"	Checksum = 0x%x");
//_LIT(KChecksumString2,"	Checksum = 0x%04x");
_LIT(KChecksumString3,"	    Chksum = 0x%04x (0x%04x) !!!");
_LIT(KConnectionString,"	    Connection = 0x%x");
_LIT(KHdrLengthString,"	Length = %d, Hdr len = %d");
//_LIT(KTOSTTLChksumString,"        TOS = 0x%02x, TTL = %d, Chksum = 0x%04x");
_LIT(KIDFragmentString,"	Id = 0x%04x, Fragment = %d %s%s%s");
_LIT(KSrcDstAddrString,"	Src = %d.%d.%d.%d, Dst = %d.%d.%d.%d");
_LIT(KIpv6SrcAddress,"    Src = %x:%x:%x:%x:%x:%x:%x:%x");
_LIT(KIpv6DstAddress,"    Dst = %x:%x:%x:%x:%x:%x:%x:%x");
_LIT(KIpv6IfIdent,"            EUI64 = %x:%x:%x:%x");
_LIT(KIpv6Class,"    Class = %d");
_LIT(KIpv6FlowLabel,"    FlowLabel = %d");
_LIT(KIpv6PayloadLen,"    Payload = %d words");
_LIT(KIpv6NextHeadType,"    Next Header type is [%d]");
_LIT(KIpv6HopLimit,"    Hop Limit is %d");
_LIT(KIpv6UnknownHeadType,"    Unknown next header: [%d]");
_LIT(KConnectionNoString,"	Connection number [0x%02x]");
_LIT(KRemoteAddrString,"	    Remote Address = %d.%d.%d.%d");
_LIT(KOurAddrString,"	    Our Address = %d.%d.%d.%d");
_LIT(KMaxSlotString,"	      Max Slot Id = %d");
_LIT(KCompSlotString,"	      Comp Slot Id = %d");
_LIT(KAddrString,"	    Address = %d.%d.%d.%d");
_LIT(KPortString,"	    Src port = %d, Dst port = %d");
_LIT(KWindowUrgentString,"	    Window = %d, Urgent = %d");
_LIT(KSeqAckString,"	    Seq = 0x%08x, Ack = 0x%08x");
_LIT(KFlagsString,"	    Flags = 0x%04x (%s%s%s%s%s%s%s%s)");

#define EIGHT_SPACE_MARGIN			_S("        ")
#define FOURTEEN_SPACE_MARGIN		_S("              ")


/** Record types in the pppdump log file */
enum
	{
	EpppDumpLogRecSentData = 1,
	EpppDumpLogRecRcvData = 2,
	EpppDumpLogRecRcvDelim = 4,
	EpppDumpLogRecTimeStepLong = 5,
	EpppDumpLogRecTimeStepShort = 6,
	EpppDumpLogRecResetTime = 7
	};

//
// PppLog
//

CPppLog::CPppLog()
	{}

CPppLog::~CPppLog()
	{
	__FLOG_CLOSE;
	delete iLogFileName;
	}

CPppLog* CPppLog::NewL()
	{
	CPppLog* self = new (ELeave) CPppLog;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); // self
	return self;
	}

void CPppLog::ConstructL()
	{
	TFileName defFileName(KPppLogFile);
	iTcpDumpLogFileName = KPppTcpDump;
	iLogFileName = defFileName.AllocL();
	iTimeOrigin.UniversalTime();
	__FLOG_OPEN(KTcpDumpFirstTag,KPppTcpDump);
	}

void CPppLog::SetLogFileNameL(const TDesC& aName)
	{
	if(aName.Length() == 0)
		//Nothing to do, keep logging to ppp.txt
		return;
	delete iLogFileName;

	// Create ppp log file name
	TFileName logFileName(aName);
	const TChar KColonChar(':');
	TInt colonPos = logFileName.Locate(KColonChar);
	_LIT(KHyphenChar, "-");
	if (colonPos != KErrNotFound)              
		{
		logFileName.Replace(colonPos, 2, KHyphenChar);
		}
	_LIT(KLogFileExtension, ".txt");
	logFileName.Append(KLogFileExtension);
	logFileName.LowerCase();
	iLogFileName = logFileName.AllocL();

	//Set tcpdump log file name
	iTcpDumpLogFileName = _L8("");
	iTcpDumpLogFileName.Append(aName);
	colonPos = iTcpDumpLogFileName.Locate(KColonChar);
	_LIT8(KHyphenChar8, "-");
	if (colonPos != KErrNotFound)
		{
		iTcpDumpLogFileName.Replace(colonPos, 2, KHyphenChar8);
		}
	_LIT8(KTcpDumpLogFileExtension, ".log");
	iTcpDumpLogFileName.Append(KTcpDumpLogFileExtension);
	iTcpDumpLogFileName.LowerCase();

	__FLOG_SET_TAGS(KTcpDumpFirstTag,iTcpDumpLogFileName);
	}

void CPppLog::Write(const TDesC& aDes)
/**
Writes aDes to the log.

@param aDes String descriptor
*/
	{
#ifdef __FLOG_ACTIVE
	RFileLogger::Write(KPppLogFolder(), *iLogFileName, EFileLoggingModeAppend, aDes);
#else
	// Preventing unused variable warnings.
	(void)aDes;
#endif //__FLOG_ACTIVE
	}

void CPppLog::Printf(const TRefByValue<const TDesC> aFmt,...)
/**
Writes a multiple argument list to the log, trapping and ignoring any leave.

@param aFmt printf-style format string
*/
	{
#ifdef __FLOG_ACTIVE
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KPppLogFolder(), *iLogFileName, EFileLoggingModeAppend, aFmt, list);
	VA_END(list);
#else
	// Preventing unused variable warnings.
	(void)aFmt;
#endif //__FLOG_ACTIVE
	}

//First attempt below was following the libcap 
//packet capture format.  This format was originally used to facilitate
//running a tcptrace stream analysis on the captured data, but further
//investigation revealed that raw HDLC frames are not decoded by libpcap
//programs (even type 50 libpcap data files).  Improvements to Ethereal also
//allowed its tcptrace to be run using pppdump format capture files,
//making libpcap format files pretty well unnecessary.
//void PppLog::LibcapDumpFileHeader()
/*
Dumps file header in a format compatible with Libcap.
Unfortunately, Ethereal cannot cope with HDLC packet including
0x7e flag char and 0x7d escape char.
Format is:
@code
struct FileHeader {
	TUint32 magic; 
	TUint16 version_major;
	TUint16 version_minor;
	TUint32 thiszone;	// gmt to local correction
	TUint32 sigfigs;	// accuracy of timestamps
	TUint32 snaplen;	// max length saved portion of each pkt
	TUint32 linktype;	// data link type (LINKTYPE_*) 
						9 PPP, 50 PPP HDLC, 12 Raw IP
};
@endcode
Note LINKTYPE specified in libpcap/bpf/net/bpf.h (see www.tcpdump.org)
byte ordering is little endian.
*/
//	{
//	TBuf8<sizeof(TUint32)*5+sizeof(TUint16)*2> fileHeader;
//	*((TUint32*) &(fileHeader.Ptr()[0])) = 0xa1b2c3d4;
//	*((TUint16*) &(fileHeader.Ptr()[4])) = 0x02; 
//	*((TUint16*) &(fileHeader.Ptr()[6])) = 0x04; 
//	*((TUint32*) &(fileHeader.Ptr()[8])) = 0x00;
//	*((TUint32*) &(fileHeader.Ptr()[12])) = 0x00;
//	*((TUint32*) &(fileHeader.Ptr()[16])) = 0xffff;
//	*((TUint32*) &(fileHeader.Ptr()[20])) = 50; 
//	fileHeader.SetLength(fileHeader.MaxLength());
//	RFileLogger::Write(KPppLogFolder(),KPppTcpDump(),EFileLoggingModeOverwriteRaw,fileHeader);
//	}

//void PppLog::LibcapDump(TPppHdlcBuf& aBuffer)
/*
Dumps a packet in a format compatible with Libcap.
For each record the format is:
@code
struct record 
{
	TUint32 sec;		// time stamp - secs
	TUint32 usec;		// time stamp - microsecs
	TUint32 captureLen;	// length packet captured
	TUint32 packetLen;	// total length of packet
};
@endcode
Byte ordering of the header is little endian
Byte ordering of the packet is network byte order (big endian)

@param aBuffer Buffer to dump
*/
//	{
	//
	//Unfortunately ethereal gets confused by the 0x7e separator
	//So we need to strip it from the buffer before logging it
//	TInt startOffset = 0;
//	TInt endOffset =0;
//	if(aBuffer[0] == 0x7e)
//		startOffset = 1;
//	if(aBuffer[aBuffer.Length()-1] == 0x7e)
//		endOffset = 1;
//	if(startOffset && aBuffer.Length() ==1)
//		return;
//	TPppHdlcBuf dumpBuf;
//	dumpBuf.Copy(aBuffer.Ptr()+startOffset,aBuffer.Length()-startOffset-endOffset);
//
//	TBuf8<sizeof(TUint32)*4> recordHeader;
//	*((TUint32*) &(recordHeader.Ptr()[0])) = 0x01;
//	*((TUint32*) &(recordHeader.Ptr()[4])) = 0x0;
//	*((TUint32*) &(recordHeader.Ptr()[8])) = dumpBuf.Length();
//	*((TUint32*) &(recordHeader.Ptr()[12])) = dumpBuf.Length();
//	recordHeader.SetLength(recordHeader.MaxLength());
//	RFileLogger::Write(KPppLogFolder(),KPppTcpDump(),EFileLoggingModeAppendRaw,recordHeader);
//
//	RFileLogger::Write(KPppLogFolder(),KPppTcpDump(),EFileLoggingModeAppendRaw,dumpBuf);
//	}


//Second attempt wih pppdump packet dump format
void CPppLog::PppDumpFileHeader()
/**
Dumps file header in a format compatible with pppdump.
As a file header just drop a 0x07|t3|t2|t1|t0 record followed by a
0x05|t3|t2|t1|t0 record, that seems common practice.
Since this "header" is actually a regular record, it can be
appended to an existing file.
*/
	{
	_LIT(Ktime_tOrigin,"19700000:000000.000000");
	TTime time_t_Origin(Ktime_tOrigin);
	TTimeIntervalSeconds secs;
	TTime timeNow;
	timeNow.UniversalTime();
	timeNow.SecondsFrom(time_t_Origin,secs);
	TBuf8<10> fileHeader;
	fileHeader.SetLength(fileHeader.MaxLength());
	
	fileHeader[0] = EpppDumpLogRecResetTime;
	BigEndian::Put32(&fileHeader[1], secs.Int());

	fileHeader[5] = EpppDumpLogRecTimeStepLong;
	BigEndian::Put32(&fileHeader[6], 0);
	
	__FLOG_BINARY(fileHeader);
	}

void CPppLog::PppDumpFrame(TInt aDirection, TUint32 aTimeStep, const TDesC8& aBuffer)
/**
Dumps a packet in a pppdump format (see www.ethereal.com)
For each record the format is:
@code
0x07|t3|t2|t1|t0	Reset time    t = time_t (UNIX: secs since 01/01/1970)
0x06|t0			Time step (short) - ts = time step (tenths)
0x05|t3|t2|t1|t0	Time step (short) - ts = time step (tenths)
0x04			Receive deliminator (not seen in practice)
0x02|n1|n0		Received data	- n = number of bytes following
0x01|n1|n0		Sent data		- n = number of bytes following
@endcode
Byte ordering of the header is little endian
Byte ordering of the packet is network byte order (big endian)

@param aDirection 01:in/02:out (as documented by pppdump packet trace format)
@param aTimeStep Delta time from last dumped frame
@param aBuffer Buffer to dump
*/
	{
		TBuf8<2+6> recordHeader;
		int i=0;
		if (aTimeStep > 0xff)
			{
			// 32-bit differential time record
			recordHeader.SetLength(8);
			recordHeader[i++] = EpppDumpLogRecTimeStepLong;
			BigEndian::Put32(&recordHeader[i], aTimeStep);
			i += 4;
			}
		else if (aTimeStep > 0)
			{
			// 8-bit differential time record
			recordHeader.SetLength(5);
			recordHeader[i++] = EpppDumpLogRecTimeStepShort;
			recordHeader[i++] = (TUint8) aTimeStep;
			}
		else
			{
			recordHeader.SetLength(3);
			}
		recordHeader[i++] = (TUint8) aDirection;
		BigEndian::Put16(&recordHeader[i], (TUint16)aBuffer.Length());
		i += 2;
		__FLOG_BINARY(recordHeader);
		__FLOG_BINARY(aBuffer);
	}

void CPppLog::DumpFrameFileHeader(TInt aLogFormat, TInt aLogType)
/**
Specifies the file format to use for the frame log and writes the file header.

@param aLogFormat Network log file format; EpppDumpLogFormat for pppdump
@param aLogType EpppLogLinkFormat to log all frames,
 ErawIpLogLinkFormat to just log IP frames
*/
    {
    iLogFormat = aLogFormat;
    iLogType = aLogType;
    if (iLogFormat == EpppDumpLogFormat)
        {
        PppDumpFileHeader();
        }
    // else unknown format

	Printf(_L("Note: the format of PPP log has changed\n"));
	Printf(_L("The level of logging is now specified in the file: \\system\\data\\ppp.ini\n"));
	Printf(_L("In the section [log], the entry: level= n specifies the level of logging\n"));
	Printf(_L("n=0 :\tFinite State Machine transition are logged\n"));
	Printf(_L("n=1 :\tReserved for future use\n"));
	Printf(_L("n=2 :\tReserved for future use\n"));
	Printf(_L("n=3 :\tReserved for future use\n"));
	Printf(_L("n=4 :\tLog parsed PPP packets *SLOW* (superseded by TcpDump)\n"));
	Printf(_L("n=5 :\tHexDump of PPP packets *VERY SLOW* (superseded by TcpDump)\n"));
	Printf(_L("\tA new binary log is generated with the TcpDump tag, and after being\n"));
	Printf(_L("\textracted with splitlog.bat, it can be used as an input file for Ethereal.\n"));
	Printf(_L("Ethereal is a general purpose packet analyser that can parse a wide variety of protocols.\n"));
	Printf(_L("\tIt is freeware and can be fetched from: http://www.ethereal.com\n"));
    
    iLogHeaderDone=ETrue;
    }

void CPppLog::DumpFrame(TInt aDirection, const TDesC8& aBuffer)
/**
Dumps frame in the configured format.

@param aDirection EpppDirectionSend or EpppDirectionReceive
(as documented by pppdump packet trace format)
@param aBuffer Buffer to dump
*/
	{
	if(iLogFormat==EpppDumpLogFormat)
	{
		if(!iLogHeaderDone)
			{
			PppDumpFileHeader();
			iLogHeaderDone=ETrue;
			}
		//Find absolute time now
		TUint32 timeStep;
		TTime timeNow;
		timeNow.UniversalTime();
		TTimeIntervalMicroSeconds us= timeNow.MicroSecondsFrom(iTimeOrigin);
		//Round to the number of 1/10 secs within the hour
		timeStep = I64LOW(us.Int64());
		timeStep /= 100000;		
		if(timeStep)
			iTimeOrigin=timeNow;
		PppDumpFrame(aDirection,timeStep,aBuffer);
	}
	// else unknown log format
    }
    	
void CPppLog::HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth)
	{

	TBuf<0x100> buf;
	TInt i = 0;
	const TText* p = aHeader;
	while (aLen>0)
		{
		TInt n = aLen>aWidth ? aWidth : aLen;
		if (p!=NULL)
			{
			_LIT(string1,"%s%04x : ");
			buf.AppendFormat(string1, p, i);
			}
		TInt j;
		_LIT(string2,"%02x ");
		for (j=0; j<n; j++)
			buf.AppendFormat(string2, aPtr[i+j]);
		_LIT(string3,"   ");
		while (j++<KHexDumpWidth)
			buf.Append(string3);
		_LIT(string4," ");
		buf.Append(string4);
		_LIT(string5,"%c");
		for (j=0; j<n; j++)
			buf.AppendFormat(string5, aPtr[i+j]<32 || aPtr[i+j]>126 ? '.' : aPtr[i+j]);
		buf.Append(KEndOfLine);
		Write(buf);
		buf.SetLength(0);
		aLen -= n;
		i += n;
		p = aMargin;
		}
	}

void CPppLog::Dump(RMBufChain& aPacket, TInt aChannel)
	{
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	TUint prot = TPppAddr::Cast(info->iDstAddr).GetProtocol();

	HBufC8* hbuf = 0;
	TRAPD(err, hbuf = HBufC8::NewL(aPacket.Length()+1));
	if(err != KErrNone)
		{
		_LIT(string0,"CPppLog::Dump error = %d");
		Printf(string0, err);
		}
	else	
		{
		TPtr8 buf = hbuf->Des();
		buf.SetMax();
		aPacket.CopyOut(buf, aPacket.First()->Length());
		
		if (aChannel==KPppHdlcSendChannel)
			{
			_LIT(string1,"PPP Send %d bytes");
			Printf(string1, info->iLength);
			}
		else if (aChannel==KPppHdlcRecvChannel)
			{
			_LIT(string2,"PPP Recv %d bytes");
			Printf(string2, info->iLength);
			}
		else
			{
			_LIT(string3,"PPP %d bytes");
			Printf(string3, info->iLength);
			}

		_LIT(string4,"    %s [0x%04x] ");
		Printf(string4, ProtocolToText(prot), prot);
		
		TPtrC8 des(buf);
		switch (prot)
			{
		case KPppIdLcp:
			DumpLcp(des);
			break;
		case KPppIdPap:
			DumpPap(des);
			break;
		case KPppIdChap:
			DumpChap(des);
			break;
		case KPppIdIpcp:
			DumpIpcp(des);
			break;
		case KPppIdIp6cp:
			DumpIp6cp(des);
			break;
		case KPppIdIp:
			DumpIp(des);
			break;
		case KPppIdIp6:
			DumpIp6(des);
			break;
		case KPppIdVjCompTcp:
			DumpVjCompTcp(des);
			break;
		case KPppIdVjUncompTcp:
			DumpVjUncompTcp(des);
			break;
		case KPppIdMsCbcp:
			DumpCbcp(des);
			break;
		case KPppIdCcp:
			DumpCcp(des);
			break;
		case KPppIdCompressed:
			break;
		default:
			Printf(_L("Raw Bytes:"));
			DumpBytes(_S(""), des.Ptr(), des.Length());
			Write(KEndOfLine);
			break;
			}
		}
	delete hbuf; 
	}

void CPppLog::DumpState(const TText* aFsm, TPppFsmState aOldState, TPppFsmState aNewState)
	{

	if (aNewState==aOldState)
		Printf(KStateString1, aFsm, StateToText(aNewState));
	else
		Printf(KStateString2, aFsm, StateToText(aOldState), StateToText(aNewState));
	}

void CPppLog::DumpPhase(TPppPhase aOldPhase, TPppPhase aNewPhase)
	{

	if (aNewPhase==aOldPhase)
		Printf(KPhaseString1, PhaseToText(aNewPhase));
	else
		Printf(KPhaseString2, PhaseToText(aOldPhase), PhaseToText(aNewPhase));
	}

// Private functions

void CPppLog::DumpChapType(const TUint8* aPtr)
	{

	switch (*aPtr)
		{
	case 5:
			{
			_LIT(string1,"	    Algorithm=MD5");
			Write(string1);
			}
		break;

	case 0x80:
			{
			_LIT(string2,"	    Algorithm=MS-CHAP-V1");
			Write(string2);
			}
		break;

	case 0x81:
			{
			_LIT(string2,"	    Algorithm=MS-CHAP-V2");
			Write(string2);
			}
		break;

	default:
			{
			_LIT(string3,"	    Algorithm=Unknown");
			Write(string3);
			}
		break;
		}
	}

void CPppLog::DumpBytes(const TText* aMargin, const TUint8* aPtr, TInt aLen)
	{

	HexDump(NULL,aMargin,aPtr,aLen,8);
	}


TInt CPppLog::DumpLcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	if(len>aDes.Length())
		{
		Printf(_L("WARNING: Length reported in header was more than actual frame length"));
		return len;
		}
	else if (len<aDes.Length())
		Printf(_L("WARNING: Length reported in header was less than actual frame length!"));

	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);
	
	Printf(KCodeTextString, LcpCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			TInt tmp = 0;
			while (n>0 && aDes.Length()>0)
				{
				tmp = DumpLcpOption(aDes);
				if (tmp == 0)	//Check for an error in the DumpLcpOption (0 is interpreted as an error)
					{
					Printf(_L("WARNING: Length reported in LCP Option was incorrect! Dumping data instead"));
					DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
					break;
					}
				n=n-tmp;
				}	

			if (n>0)
				Printf(KBytesRemainingString, n);
			}

		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = *ptr;
			Printf(KCodeString, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);		
			Printf(KProtocolString, val);
			}
		break;
	case KPppLcpEchoRequest:
			{
			TUint val = BigEndian::Get32(ptr);	
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpEchoReply:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpDiscardRequest:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpIdentification:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpTimeRemaining:
			{
			TUint val = BigEndian::Get32(ptr);
			ptr+=4;
			Printf(KNumberString, val);
			val = BigEndian::Get32(ptr);
			Printf(KSecondsString, val);
			}
		break;
	default:
		break;
		}

	return len;
	}

TInt CPppLog::DumpLcpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	if(len > aDes.Length())
		{
		Printf(_L("WARNING: Length reported in option header was more than actual frame length"));
		return 0;	//Return an incorrect length to flag this error.
		}

	if(len < 2)
		{
		Printf(_L("WARNING: Length reported in option header was less than a correct frame length"));
		return 0;	//Return an incorrect length. (the minimum correct length of an option is 2)
		}
	
	Printf(KLcpCodeString, LcpOptToText(opt), opt);
	Printf(KLengthString, len);
	
	switch (opt)
		{
	case KPppLcpOptMaxRecvUnit:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KSizeString, val);
			}
		break;
	case KPppLcpOptEscapeCharMap:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KMapString, val);
			}
		break;
	case KPppLcpOptAuthenticationProtocol:
			{
			TUint val = BigEndian::Get16(ptr);

			Printf(KProtocolTextString, ProtocolToText(val), val);
			if (val != KPppIdChap)
				{
				if (len>4)
					DumpBytes(EIGHT_SPACE_MARGIN, ptr+2, len-4);
				}
			else
				{
				if(len>4)
					DumpChapType(ptr+2);
				}
			}
		break;
	case KPppLcpOptQualityProtocol:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolTextString, ProtocolToText(val), val);
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+2, len-4);
			}
		break;
	case KPppLcpOptMagicNumber:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			}
		break;
	case KPppLcpOptProtocolCompress:
	case KPppLcpOptAddrCtrlCompress:
		break;
	case KPppLcpOptFcsType:
			{
			TUint val = *ptr++;
			Printf(KLcpOptFcsTypeString, (val&KPppHdlcFcs0Flag)?_S(" None"):_S(""), (val&KPppHdlcFcs16Flag)?_S(" 16 bit"):_S(""), (val&KPppHdlcFcs32Flag)?_S(" 32 bit"):_S(""));
			}
		break;
	case KPppLcpOptPadding:
			{
			TUint val = *ptr++;
			Printf(KMaximumBytesString, val);
			}
		break;
	case KPppLcpOptCallback:
			{
			TUint val = *ptr++;
			Printf(KCodeTextString, CallbackOpToText(val), val);
			if (len>3)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+3, len-3);
			}
		break;
	case KPppLcpOptCompoundFrames:
		break;
	default:
		break;
		}
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

TInt CPppLog::DumpPap(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;

	Printf(KCodeTextString, PapCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	if (code==1)
		{
		TInt n = *ptr++;
		TPtrC8 v;
		v.Set(ptr, n);    ptr += n;
		TBuf16<KLogBufferSize> temp;
		temp.Copy(v.Left(Min(v.Length(),KLogBufferSize)));
		Printf(KUserNameString, &temp);
		n = *ptr++;
		v.Set(ptr, n);    ptr += n;
		temp.Copy(v.Left(Min(v.Length(),KLogBufferSize)));
		Printf(KPasswordString, &temp);
		}
	else if (code==2 || code==3)
		{
		TInt n = *ptr++;
		TPtrC8 v;
		v.Set(ptr, n);    ptr += n;
		TBuf16<KLogBufferSize> temp;
		temp.Copy(v.Left(Min(v.Length(),KLogBufferSize)));
		Printf(KMessageString, &temp);
		}

	aDes.Set(aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

TInt CPppLog::DumpCbcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	TInt length = len;
	ptr += 2;

	Printf(KCodeTextString, CbcpCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);

	//
	// OK we can be offered many options at once, so 
	// loop for the length and decode them.
	//
	len -= 4; // Subtract the stuff we know about

	TUint8	type;
	TUint	typeLen;
	while (len)
		{
		// Yes I know we can have a permanent loop but I just don't care
		type = *ptr++;
		switch(type)
			{
		case 1:
				{
				_LIT(string1,"        No CallBack");
				Write(string1);
				}
			break;
		case 2:
				{
				_LIT(string2,"        Callback to a user specified No.");
				Write(string2);
				}
			break;
		case 3:
				{
				_LIT(string3,"        Callback to a pre specified No.");
				Write(string3);
				}
			break;
		case 4:
				{
				_LIT(string4,"        Callback to one of a list of numbers");
				Write(string4);
				}
			break;
		default:
				{
				_LIT(string5,"        Unknown Callback type");
				Write(string5);
				}
			break;
		}

		if (len == 1)
			len = 0;

		typeLen = *ptr++;
			Printf(KLengthString,typeLen);

		if (typeLen > 2)
		 	Printf(KDelayString,*ptr++);

		if (typeLen > 3)
			{
			//
			// Dump the Address Type field
			//
			if (*ptr++ == 1)
				{
				_LIT(string6,"            PSTN/ISDN");
				Write(string6);
				}
			else
				{
				_LIT(string7,"            Other");
				Write(string7);
				}
			}

		//
		// Dump the NUL-terminated ASCII string
		//
 		DumpBytes(EIGHT_SPACE_MARGIN, ptr, (typeLen-4));
		len -= typeLen;
		}

	aDes.Set(aDes.Ptr()+length, aDes.Length()-length);
	return length;
	}

TInt Advance(TPtrC8& aPacket, const TUint8* aPos)
	{
	aPacket.Set(aPos, aPacket.Length() - (aPacket.Ptr() - aPos));
	return aPacket.Ptr() - aPos;
	}

TInt CPppLog::DumpChap(TPtrC8& aPacket)
	{
	if (aPacket.Length() < KPppChapCodeFieldSize)
		return 0;

	const TUint8* ptr = aPacket.Ptr();
	TInt packLength = aPacket.Length();
	TUint8 code = *ptr;

	Printf(KCodeTextWithoutSpacesString,ChapCodeToText(code), code);
	ptr += KPppChapCodeFieldSize;

	if (packLength < KPppChapCodeFieldSize +
				KPppChapIdFieldSize +
				KPppChapLengthFieldSize)
		return Advance(aPacket, ptr);

	TUint8 id = *ptr;
	ptr += KPppChapIdFieldSize;
	TUint16 length = BigEndian::Get16(ptr);
	ptr += KPppChapLengthFieldSize;

	Printf(KIdentifierLengthString, id, length);

	if (packLength > length)
		packLength = length;
	
	if (code == KPppChapChallengeCode || code == KPppChapResponseCode)
		{
		if (packLength < KPppChapCodeFieldSize +
				KPppChapIdFieldSize +
				KPppChapLengthFieldSize +
				KPppChapValueSizeFieldSize +
				KPppChapMinValueSize)
			return Advance(aPacket, ptr);

		TUint8 valueSize = *ptr;
		if (valueSize < KPppChapMinValueSize)
			return Advance(aPacket, ptr);
		ptr += KPppChapValueSizeFieldSize;
		if (valueSize > packLength -
					KPppChapCodeFieldSize -
					KPppChapIdFieldSize -
					KPppChapLengthFieldSize -
					KPppChapValueSizeFieldSize)
			return Advance(aPacket, ptr);
		DumpBytes(EIGHT_SPACE_MARGIN, ptr, valueSize);
		ptr += valueSize;

// length will contain the length of the CHAP Name field next
		ASSERT(packLength <= length);
		length = static_cast<TUint16>(packLength -
				KPppChapCodeFieldSize -
				KPppChapIdFieldSize -
				KPppChapLengthFieldSize -
				KPppChapValueSizeFieldSize -
				valueSize);
		if (length < KPppChapMinNameSize)
			return Advance(aPacket, ptr);
		
		if (length > KLogBufferSize)
			length = KLogBufferSize;

		TPtrC8 name(ptr, length);

		HBufC16* uniName=NULL;
		TRAPD(err, uniName = HBufC16::NewL(length));
		if (err==KErrNone)
			{
			// Convert the string to a 16 bit descriptor
			uniName->Des().Copy(name);
			Printf(KNameString, uniName);
			delete uniName;
			}
		ptr += length;
		}
	else if (code == KPppChapSuccessCode || code == KPppChapFailureCode)
		{
// length will contain the length of the message next
		ASSERT(packLength <= length);
		length = static_cast<TUint16>(packLength -
					KPppChapCodeFieldSize -
					KPppChapIdFieldSize -
					KPppChapLengthFieldSize);
		if (length == 0)
			return Advance(aPacket, ptr);

		TPtrC8 msg(ptr, length);

		HBufC16* uniMsg=NULL;
		TRAPD(err, uniMsg = HBufC16::NewL(length));
		if (err==KErrNone)
			{
			// Convert the string to a 16 bit descriptor
			uniMsg->Des().Copy(msg);
			Printf(KMessageString, uniMsg);
			delete uniMsg;
			}
		ptr += length;
		}

	return Advance(aPacket, ptr);
	}


TInt CPppLog::DumpCcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);

	Printf(KCodeTextWithoutSpacesString,FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			TInt tmp = 0;
			while (n>0 && aDes.Length()>0)
				{
				tmp = DumpCcpOption(aDes);
				if (tmp == 0) 	//Check for an error in the DumpCcpOption (0 is interpreted as an error)
					{
					Printf(_L("WARNING: Length reported in CCP Option was incorrect! Dumping data instead"));
					DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
					break;
					}
				n=n-tmp;
				}	

			if (n>0)
				Printf(KBytesRemainingString, n);
			}
		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KCodeString, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolString, val);
			}
		break;
	default:
		break;
		}
	return len;
	}

TInt CPppLog::DumpCcpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	if(len > aDes.Length())
		{
		Printf(_L("WARNING: Length reported in option header was more than actual frame length"));
		return 0;	//Return an incorrect length to flag this error.
		}

	if(len < 2)
		{
		Printf(_L("WARNING: Length reported in option header was less than a correct frame length"));
		return 0;	//Return an incorrect length. (the minimum correct length of an option is 2)
		}

	Printf(KCodeTextString, CcpCodeToText(opt), opt);
	Printf(KLengthString, len);

	DumpBytes(FOURTEEN_SPACE_MARGIN, ptr, len-2);
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

const TText* CPppLog::CcpCodeToText(TUint aValue)
	{

	// CCP Codes
	switch (aValue)
		{
	case KPppCcpOptOui:
		return _S("OUI");
	case KPppCcpOptPred1:
		return _S("Predictor type1");
	case KPppCcpOptPred2:
		return _S("Predictor type2");
	case KPppCcpOptPuddle:
		return _S("Puddle");
	case KPppCcpOptHP:
		return _S("HP PPC");
	case KPppCcpOptStac:
		return _S("Stac LZS");
	case KPppCcpOptMsoft:
		return _S("Microsoft PPC");
	case KPppCcpOptGandalf:
		return _S("Gandalf");
	case KPppCcpOptV42bis:
		return _S("V42bis");
	case KPppCcpOptBSDLzw:
		return _S("BSD LZW");
	case KPppCcpOptMagnalink:
		return _S("Magnalink (or bad deflate)");
	case KPppCcpOptDeflate:
		return _S("Deflate");
	case KPppCcpOptReserved:
		return _S("Reserved");
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		return _S("unassigned");
	default:
		return _S("Unknown");
		}
	}

TInt CPppLog::DumpVjCompTcp(TPtrC8& aDes)
/**
Dump a Van Jacobson compressed TCP/IP packet.
@see DumpVjUncompTcp
*/
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint16	change = *ptr++;
	TUint8	urgent=0;
	TInt16	window=0;
	TUint16	ack=0;
	TUint16	sequence=0;
	TBuf<50> changeMaskBuf;
	TUint8	connection=0;

	changeMaskBuf.Append(KChangeMaskString);

	if (change & KVjCompMaskConn)
		{	
		_LIT(string7,"C");
		changeMaskBuf.Append(string7);
		connection = *ptr++;
		}

	TUint16 checksum = BigEndian::Get16(ptr);
	ptr += 2;

	if (change & KVjCompMaskPush)
		{
		_LIT(string8,"P");
		changeMaskBuf.Append(string8);
		}

	/*
	*	Don't reorder SpecialI && SpecialD, they are like this
	*	as SpecialD is the SWAU bits and SpecialI is SWU
	*
	*/

	if ((change & KVjCompMaskSpecials) == KVjCompMaskSpecialD)
		{
		_LIT(string1,"	Special D");
		Write(string1);
		}
	else if ((change & KVjCompMaskSpecials) == KVjCompMaskSpecialI)
		{
		_LIT(string2,"	Special I");
		Write(string2);
		}
	else
		{
		if (change & KVjCompMaskUrgent)
			{
			_LIT(string3,"U");
			changeMaskBuf.Append(string3);
			urgent = *ptr++;
			}

		if (change & KVjCompMaskWindow )
			{
			window = (TInt16)DecodeSignedDelta(ptr);
			_LIT(string4,"W");
			changeMaskBuf.Append(string4);
			}

		if (change & KVjCompMaskAck )
			{
			ack = DecodeDelta(ptr);
			_LIT(string5,"A");
			changeMaskBuf.Append(string5);
			}

		if (change & KVjCompMaskSeq)
			{	
			sequence = DecodeDelta(ptr);
			_LIT(string6,"S");
			changeMaskBuf.Append(string6);
			}
		}


	TUint16	ipId=0;
	if (change & KVjCompMaskIp)
		{	
		ipId = DecodeDelta(ptr);
		_LIT(string9,"I");
		changeMaskBuf.Append(string9);
		}

	Printf(TRefByValue<const TDesC>(changeMaskBuf), change);

	Printf(KChecksumString, checksum);

	if (change & KVjCompMaskConn)
		{
		Printf(KConnectionString,connection);
		}

	if	(urgent)
		{
		_LIT(string10,"	Urgent Delta = 0x%x");
		Printf(string10,urgent);
		}

	if (window)
		{
		_LIT(string11,"	Window Delta = %d");
		Printf(string11,window);
		}

	if (ack)
		{
		_LIT(string12,"	Ack Delta = 0x%x");
		Printf(string12,ack);
		}

	if (sequence)
		{
		_LIT(string13,"	Sequence Delta = 0x%x");
		Printf(string13,sequence);
		}

	if (ipId)
		{
		_LIT(string14,"	IPId = 0x%x");
		Printf(string14,ipId);
		}

	return 1;
	}

TUint16 CPppLog::DecodeDelta(TUint8*& aPtr)
	{

	TUint16 wordValue;
	TUint8	byteValue = *aPtr++;
	if (byteValue != 0)	
		return (TUint16)byteValue;
	else
		{
		/*
		*	Zero is an extender
		*/
		wordValue = BigEndian::Get16(aPtr);
		aPtr += 2;
		return wordValue;
		}
	}

TInt16 CPppLog::DecodeSignedDelta(TUint8*& aPtr)
	{

	TInt16 wordValue;
	TUint8 byteValue = *aPtr++;
	if (byteValue != 0)	
		return (TInt16)byteValue;
	else
		{
		/*
		*	Zero is an extender
		*/
		wordValue = BigEndian::Get16(aPtr);
		aPtr += 2;
		return wordValue;
		}
	}

TInt CPppLog::DumpVjUncompTcp(TPtrC8& aDes)
/**
Dump a Van Jacobson uncompressed TCP/IP packet.
@see DumpIp is almost identical
@see DumpVjCompTcp
*/
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 c = *ptr++;
//	TUint ver = c >> 4;
	TUint hlen = (c & 0xf) << 2;
//	TUint8 tos = *ptr;
	ptr++;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 id = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 frag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TBool zf = (frag & 0x8000);
	TBool df = (frag & 0x4000);
	TBool mf = (frag & 0x2000);
	frag = (TUint16)((frag & 0x1fff)<<3);
//	TUint8 ttl = *ptr;
	ptr++;
	TUint8 proto = *ptr++;
//	TUint16 chksum = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint32 srca = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 dsta = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
//	TBool opts = (hlen>20);

	Printf(KHdrLengthString, len, hlen);
	Printf(KSrcDstAddrString, (srca&0xff000000)>>24,(srca&0x00ff0000)>>16,(srca&0x0000ff00)>>8,(srca&0x000000ff),
							  (dsta&0xff000000)>>24,(dsta&0x00ff0000)>>16,(dsta&0x0000ff00)>>8,(dsta&0x000000ff));
	Printf(KIDFragmentString, id, frag, df?_S("<DF>"):_S(""), mf?_S("<MF>"):_S(""), zf?_S("<Z>"):_S(""));
//	Printf(KTOSTTLChksumString, tos, ttl, chksum);
	Printf(KConnectionNoString, proto);
	
	if (hlen>20)
		ptr += (hlen-20);
	
	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set(ptr, tlen);

	Printf(KCodeTextString, IpProtocolToText(6), 6);
	return n+DumpTcp(aDes, srca, dsta, tlen);
	}

TInt CPppLog::DumpIp6cp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);

	Printf(KCodeTextWithoutSpacesString, FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			while (n>0 && aDes.Length()>0)
				n -= DumpIp6cpOption(aDes);
			if (n>0)
				Printf(KBytesRemainingString, n);
			}
		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KCodeString2, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolString, val);
			}
		break;
	default:
		break;
		}
	return len;
	}

TInt CPppLog::DumpIpcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);

	Printf(KCodeTextWithoutSpacesString, FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			TInt tmp = 0;
			while (n>0 && aDes.Length()>0)
				{
				tmp = DumpIpcpOption(aDes);
				if (tmp == 0)  	//Check for an error in the DumpIpcpOption (0 is interpreted as an error)
					{
					Printf(_L("WARNING: Length reported in IPCP Option was incorrect! Dumping data instead"));
					DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
					break;
					}
				n=n-tmp;
				}	

			if (n>0)
				Printf(KBytesRemainingString, n);
			}
		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KCodeString2, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolString, val);
			}
		break;
	default:
		break;
		}
	return len;
	}

TInt CPppLog::DumpIpcpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	if(len > aDes.Length())
		{
		Printf(_L("WARNING: Length reported in option header was more than actual frame length"));
		return 0;	//Return an incorrect length to flag this error.
		}

	if(len < 2)
		{
		Printf(_L("WARNING: Length reported in option header was less than a correct frame length"));
		return 0;	//Return an incorrect length. (the minimum correct length of an option is 2)
		}

	Printf(KCodeTextString, IpcpCodeToText(opt), opt);
	Printf(KLengthString2, len);
	
	TUint32 val;

	switch (opt)
		{
	case KPppIpcpOptIpAddresses:
		val = BigEndian::Get32(ptr);
		Printf(KRemoteAddrString,(val&0xff000000)>>24,(val&0x00ff0000)>>16,(val&0x0000ff00)>>8,(val&0x000000ff));
		val = BigEndian::Get32(ptr+4);
		Printf(KOurAddrString,(val&0xff000000)>>24,(val&0x00ff0000)>>16,(val&0x0000ff00)>>8,(val&0x000000ff));
		break;
	case KPppIpcpOptIpCompressionProtocol:
			{
			TUint prot = BigEndian::Get16(ptr);
			ptr += 2;
			Printf(KProtocolTextString, ProtocolToText(prot), prot);
			if (len>4)
				{
				switch (prot)
					{
				case KPppIdVjCompTcp:
					Printf(KMaxSlotString, *ptr++);
					Printf(KCompSlotString, *ptr++);
					break;
				default:
					DumpBytes(FOURTEEN_SPACE_MARGIN, ptr+4, len-4);
					break;
					}
				}
			}
		break;
	case KPppIpcpOptIpAddress:
	case KPppIpcpOptPrimaryDnsAddress:
	case KPppIpcpOptSecondaryDnsAddress:
	case KPppIpcpOptPrimaryNbnsAddress:
	case KPppIpcpOptSecondaryNbnsAddress:
			{
			val = BigEndian::Get32(ptr);
			Printf(KAddrString,(val&0xff000000)>>24,(val&0x00ff0000)>>16,(val&0x0000ff00)>>8,(val&0x000000ff));
			}
		break;
	default:
		break;
		}
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}


TInt CPppLog::DumpIp6cpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	Printf(KCodeTextString, Ip6cpCodeToText(opt), opt);
	Printf(KLengthString2, len);


	switch (opt)
		{
	case KPppIp6cpOptInterfaceIdentifier:
		{
		TUint16 s1 = BigEndian::Get16( ptr );
		ptr+=2;
		TUint16 s2 = BigEndian::Get16( ptr );
		ptr+=2;
		TUint16 s3 = BigEndian::Get16( ptr );
		ptr+=2;
		TUint16 s4 = BigEndian::Get16( ptr );
		ptr+=2;
		Printf(KIpv6IfIdent, s1,s2,s3,s4);
		break;
		}
	case KPppIp6cpOptCompressionProtocol:
		{
		TUint16 prot = BigEndian::Get16(ptr);
		ptr += 2;
		Printf(KProtocolTextString, ProtocolToText(prot), prot);
		if (len>4)
			{
			switch (prot)
				{
			case KPppIdVjCompTcp:
				Printf(KMaxSlotString, *ptr++);
				Printf(KCompSlotString, *ptr++);
				break;
			default:
				DumpBytes(FOURTEEN_SPACE_MARGIN, ptr+4, len-4);
				break;
				}
			}
		break;
		}
	default:
		break;
		}
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

const TText* CPppLog::ProtocolToText(TUint aValue)
	{

	// Protocols
	switch (aValue)
		{
	case KPppIdLcp:
		return _S("LCP");
	case KPppIdPap:
		return _S("PAP");
	case KPppIdChap:
		return _S("CHAP");
	case KPppIdMsCbcp:
		return _S("MSCBCP");
	case KPppIdIpcp:
		return _S("IPCP");
	case KPppIdIp6cp:
		return _S("IP6CP");
	case KPppIdIp:
		return _S("IP");
	case KPppIdIp6:
		return _S("IPv6");
	case KPppIdVjCompTcp:
		return _S("VJ Comp tcp");
	case KPppIdVjUncompTcp:
		return _S("VJ Uncomp tcp");
	case KPppIdCcp:
		return _S("CCP");
	case KPppIdCompressed:
		return _S("PPP Compressed");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::StateToText(TPppFsmState aState)
	{

	// States
	switch (aState)
		{
	case EPppFsmInitial:
		return _S("INITIAL");
	case EPppFsmStarting:
		return _S("STARTING");
	case EPppFsmClosed:
		return _S("CLOSED");
	case EPppFsmStopped:
		return _S("STOPPED");
	case EPppFsmStopping:
		return _S("STOPPING");
	case EPppFsmClosing:
		return _S("CLOSING");
	case EPppFsmReqSent:
		return _S("REQUEST SENT");
	case EPppFsmAckRecvd:
		return _S("ACK RECEIVED");
	case EPppFsmAckSent:
		return _S("ACK SENT");
	case EPppFsmOpened:
		return _S("OPENED");
	default:	// Should never happen in theory!
		return _S("UNKNOWN");
		}
	}

const TText* CPppLog::PhaseToText(TPppPhase aPhase)
	{

	// Phases
	switch (aPhase)
		{
	case EPppPhaseTerminate:
		return _S("TERMINATE");
	case EPppPhaseEstablish:
		return _S("ESTABLISH");
	case EPppPhaseEarlyCallback:
		return _S("EARLY CALLBACK");
	case EPppPhaseAuthenticate:
		return _S("AUTHENTICATE");
	case EPppPhaseLateCallback:
		return _S("LATE CALLBACK");
	case EPppPhaseNetwork:
		return _S("NETWORK");
	default:	// Should never happen in theory!
		return _S("UNKNOWN");
		}
	}

const TText* CPppLog::LcpCodeToText(TUint aValue)
	{

	// LCP Codes
	switch (aValue)
		{
	case KPppLcpEchoRequest:
		return _S("Echo request");
	case KPppLcpEchoReply:
		return _S("Echo reply");
	case KPppLcpDiscardRequest:
		return _S("Discard request");
	case KPppLcpIdentification:
		return _S("Identification");
	case KPppLcpTimeRemaining:
		return _S("Time remaining");
	default:
		return FsmCodeToText(aValue);
		}
	}

const TText* CPppLog::FsmCodeToText(TUint aValue)
	{

	// LCP Codes
	switch (aValue)
		{
	case KPppLcpConfigRequest:
		return _S("Config Request");
	case KPppLcpConfigAck:
		return _S("Config ACK");
	case KPppLcpConfigNak:
		return _S("Config NAK");
	case KPppLcpConfigReject:
		return _S("Config Reject");
	case KPppLcpTerminateRequest:
		return _S("Terminate request");
	case KPppLcpTerminateAck:
		return _S("Terminate ACK");
	case KPppLcpCodeReject:
		return _S("Code Reject");
	case KPppLcpProtocolReject:
		return _S("Protocol Reject");
	case KPppCcpResetReq:
		// Only used by CCP, but this is the easiest place to put it
		return _S("Reset Request");
	case KPppCcpResetAck:
		// Only used by CCP, but this is the easiest place to put it
		//__DEBUGGER();
		return _S("Reset Ack");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::LcpOptToText(TUint aValue)
	{

	// LCP Options
	switch (aValue)
		{
	case KPppLcpOptMaxRecvUnit:
		return _S("Max receive size");
	case KPppLcpOptEscapeCharMap:
		return _S("Escape char map");
	case KPppLcpOptAuthenticationProtocol:
		return _S("Authentication protocol");
	case KPppLcpOptQualityProtocol:
		return _S("Quality protocol");
	case KPppLcpOptMagicNumber:
		return _S("Magic number");
	case KPppLcpOptProtocolCompress:
		return _S("Protocol field compression");
	case KPppLcpOptAddrCtrlCompress:
		return _S("Addr & Ctrl field compression");
	case KPppLcpOptFcsType:
		return _S("Fcs Type");
	case KPppLcpOptPadding:
		return _S("Padding");
	case KPppLcpOptCallback:
		return _S("Callback protocol");
	case KPppLcpOptCompoundFrames:
		return _S("Compound frames");
	case KPppLcpOptMRRU:
 		return _S("MRRU");
 	case KPppLcpOptMultiLinkEndPointDescriminator:
 		return _S("Multi-link End-Point Descriminator");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::CallbackOpToText(TUint aValue)
	{

	// CB Codes
	switch (aValue)
		{
	case 0:
		return _S("Location preset");
	case 1:
		return _S("Dialing string");
	case 2:
		return _S("Location identifier");
	case 3:
		return _S("E.164 number");
	case 4:
		return _S("Distinguished name");
	case 5:
		return _S("E.165 number");
	case 6:
		return _S("MS CBCP");
	default:
		return _S("Unknown operation");
		}
	}

const TText* CPppLog::PapCodeToText(TUint aValue)
	{

	// PAP Codes
	switch (aValue)
		{
	case 1:
		return _S("Authenticate Request");
	case 2:
		return _S("Authenticate ACK");
	case 3:
		return _S("Authenticate NAK");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::CbcpCodeToText(TUint aValue)
	{

	// CBCP Codes
	switch (aValue)
		{
	case 1:
		return _S("Callback Request");
	case 2:
		return _S("Callback Response");
	case 3:
		return _S("Callback Ack");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::ChapCodeToText(TUint aValue)
	{

	// CHAP Codes
	switch (aValue)
		{
	case 1:
		return _S("Challenge");
	case 2:
		return _S("Response");
	case 3:
		return _S("Success");
	case 4:
		return _S("Failure");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::IpProtocolToText(TUint aValue)
	{

	// IP Protocols
	switch (aValue)
		{
	case 1:
		return _S("ICMP");
	case 58:
		return _S("ICMPv6");
	case 17:
		return _S("UDP");
	case 6:
		return _S("TCP");

	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::IpcpCodeToText(TUint aValue)
	{

	// IPCP Codes
	switch (aValue)
		{
	case KPppIpcpOptIpAddresses:
		return _S("IP Addresses");
	case KPppIpcpOptIpCompressionProtocol:
		return _S("IP Compression Protocol");
	case KPppIpcpOptIpAddress:
		return _S("IP Address");
	case KPppIpcpOptPrimaryDnsAddress:
		return _S("Primary DNS Address");
	case KPppIpcpOptSecondaryDnsAddress:
		return _S("Secondary DNS Address");
	case KPppIpcpOptPrimaryNbnsAddress:
		return _S("Primary NBNS Address");
	case KPppIpcpOptSecondaryNbnsAddress:
		return _S("Secondary NBNS Address");
	default:
		return _S("Unknown");
		}
	}

const TText* CPppLog::Ip6cpCodeToText(TUint aValue)
	{
	// IP6CP Codes
	switch (aValue)
		{
	case KPppIp6cpOptInterfaceIdentifier:
		return _S("Interface Identifier");
	case KPppIp6cpOptCompressionProtocol:
		return _S("Compression Protocol");
	default:
		return _S("Unknown");
		}
	}

TInt CPppLog::DumpIp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 c = *ptr++;
//	TUint ver = c >> 4;
	TUint hlen = (c & 0xf) << 2;
//	TUint8 tos = *ptr;
	ptr++;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 id = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 frag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TBool zf = (frag & 0x8000);
	TBool df = (frag & 0x4000);
	TBool mf = (frag & 0x2000);
	frag = (TUint16)((frag & 0x1fff)<<3);
//	TUint8 ttl = *ptr;
	ptr++;
	TUint8 proto = *ptr++;
//	TUint16 chksum = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint32 srca = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 dsta = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
//	TBool opts = (hlen>20);

	Printf(KHdrLengthString, len, hlen);
	Printf(KSrcDstAddrString,(srca&0xff000000)>>24,(srca&0x00ff0000)>>16,(srca&0x0000ff00)>>8,(srca&0x000000ff),
							 (dsta&0xff000000)>>24,(dsta&0x00ff0000)>>16,(dsta&0x0000ff00)>>8,(dsta&0x000000ff));
	Printf(KIDFragmentString, id, frag, df?_S("<DF>"):_S(""), mf?_S("<MF>"):_S(""), zf?_S("<Z>"):_S(""));
//	Printf(KTOSTTLChksumString, tos, ttl, chksum);
	Printf(KCodeTextString, IpProtocolToText(proto), proto);
	
	if (hlen>20)
		ptr += (hlen-20);
	
	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set(ptr, tlen);

	if (tlen > 0)
		{
		switch (proto)
			{
		case 1:
			return n+DumpIcmp(aDes, tlen);
		case 6:
			return n+DumpTcp(aDes, srca, dsta, tlen);
		case 17:
			return n+DumpUdp(aDes, srca, dsta, tlen);
		default:
			break;
			}
		}
	return n;
	}

TInt CPppLog::DumpIp6(TPtrC8& aDes)
	{
	// This dumps the main IPv6 Header, no support for option headers
	// IPv6 Headers are FIXED length, but are chainable...
	TUint16* ptr = (TUint16*)aDes.Ptr();
	
	// First 32 bits contain version(4), class(8), Flow Label(20)
	
	TUint32 first = ByteOrder::Swap32((TUint32) *ptr);
	//TUint8 ver = (TUint8)((first >> 28) & 0xf);
	TUint8 cls = (TUint8)((first >> 20) & 0xff);
	TUint32 flowLab = (first & 0xfffff);
	ptr+=2;

	// Second 32 bits contain payload length (16), next header type (8), hop limit (8)
	TUint16 payLoadLen = ByteOrder::Swap16( *ptr++ );
	TUint16 next = ByteOrder::Swap16( *ptr++ ); 
	TUint8 nextHead = (TUint8)((next >> 8) & 0xff);
	TUint8 hopLimit = (TUint8)(next & 0xff);
	
	// Source address (128 bits)
	TUint16 s1 = ByteOrder::Swap16( *ptr++ );
	TUint16 s2 = ByteOrder::Swap16( *ptr++ );
	TUint16 s3 = ByteOrder::Swap16( *ptr++ );
	TUint16 s4 = ByteOrder::Swap16( *ptr++ );
	TUint16 s5 = ByteOrder::Swap16( *ptr++ );
	TUint16 s6 = ByteOrder::Swap16( *ptr++ );
	TUint16 s7 = ByteOrder::Swap16( *ptr++ );
	TUint16 s8 = ByteOrder::Swap16( *ptr++ );
	
	// Destination address (128 bits)
	TUint16 a1 = ByteOrder::Swap16( *ptr++ );
	TUint16 a2 = ByteOrder::Swap16( *ptr++ );
	TUint16 a3 = ByteOrder::Swap16( *ptr++ );
	TUint16 a4 = ByteOrder::Swap16( *ptr++ );
	TUint16 a5 = ByteOrder::Swap16( *ptr++ );
	TUint16 a6 = ByteOrder::Swap16( *ptr++ );
	TUint16 a7 = ByteOrder::Swap16( *ptr++ );
	TUint16 a8 = ByteOrder::Swap16( *ptr++ );
	
	//Printf(_L("    Version is %d"),ver);	// Should be 6 for IPv6 !!
	Printf(KIpv6Class, cls);
	Printf(KIpv6FlowLabel, flowLab);
	Printf(KIpv6PayloadLen, payLoadLen);
	Printf(KIpv6NextHeadType, nextHead);
	Printf(KIpv6HopLimit, hopLimit);
	Printf(KIpv6SrcAddress, s1,s2,s3,s4,s5,s6,s7,s8);
	Printf(KIpv6DstAddress, a1,a2,a3,a4,a5,a6,a7,a8);
	
	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set( (TUint8*) ptr, tlen);

	switch (nextHead)
		{
		case 6:		// TCP Packet
			Printf(_L("        TCP Data"));
			//DumpTcp(aDes);
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 17:	// UDP Packet
			Printf(_L("        UDP Data"));
			DumpUdp(aDes,s8,a8,tlen);
			break;
		case 43:	// Routing Header
			Printf(_L("        Routing Header"));
			//DumpRouter
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 44:	// Fragment Header
			Printf(_L("        Packet Fragment"));
			//DumpFragment
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 50:	// ESP Payload
			Printf(_L("        ESP Payload"));
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 58:	// ICMPv6
			Printf(_L("        ICMPv6"));
			DumpIcmp(aDes,tlen);
			break;
		case 59:	// No Next Header
			Printf(_L("        NO NEXT HEADER"));
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		default:
			Printf(KIpv6UnknownHeadType, nextHead);
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
		}

	return n;	
	}

TInt CPppLog::DumpTcp(TPtrC8& aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength)
	{
	
	TInt n = Min(aLength, aDes.Length());
	TInt len = n;
	
	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 osum0 = ptr[16];
	ptr[16] = 0;
	TUint8 osum1 = ptr[17];
	ptr[17] = 0;

	TUint32 sum = 0;
	sum += (aSrcA >> 16);
	sum += (aSrcA & 0xffff);
	sum += (aDstA >> 16);
	sum += (aDstA & 0xffff);
	sum += 6;
	sum += n;
	while (n>1)
		{
		sum += (ptr[0]<<8);
		sum += (ptr[1]);
		ptr += 2;
		n -= 2;
		}
	if (n>0)
		sum += (ptr[0]<<8);
	while (sum>0xffff)
		{
		sum = (sum & 0xffff) + (sum>>16);
		}
	sum = ~sum & 0xffff;
	ptr = (TUint8*)aDes.Ptr();
	ptr[16] = osum0;
	ptr[17] = osum1;
	
	TUint srcp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint dstp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint32 seqnum = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 acknum = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;

    TUint16 hlenflag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	
	TUint hlen = (hlenflag>>12)<<2;
    TUint flag = (hlenflag & 0x0fff);
    TBool cwrf = (hlenflag & 0x0080); 
    TBool ecef = (hlenflag & 0x0040); 
	TBool urgf = (hlenflag & 0x0020);
	TBool ackf = (hlenflag & 0x0010);
	TBool pshf = (hlenflag & 0x0008);
	TBool rstf = (hlenflag & 0x0004);
	TBool synf = (hlenflag & 0x0002);
	TBool finf = (hlenflag & 0x0001);
    ptr+=2;
	TUint window = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	TUint chksum = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	TUint urgptr = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	
	Printf(KTCPLengthString, len, hlen);
	Printf(KPortString, srcp, dstp);
//	Printf(KDestPortString, dstp);
//	Printf(KTCPHeaderLengthString, hlen);
	Printf(KSeqAckString, seqnum, acknum);
	Printf(KFlagsString, flag, cwrf?_S(" <CWR>"):_S(""),ecef?_S(" <ECE>"):_S(""),urgf?_S(" <URG>"):_S(""), ackf?_S(" <ACK>"):_S(""), pshf?_S(" <PSH>"):_S(""),
		                      rstf?_S(" <RST>"):_S(""), synf?_S(" <SYN>"):_S(""), finf?_S(" <FIN>"):_S(""));
	Printf(KWindowUrgentString, window, urgptr);
	if (chksum != sum)
		Printf(KChecksumString3, chksum, sum);

	if (hlen>20)
		{
		_LIT(string2,"	TCP Options %d bytes");
		Printf(string2, hlen-20);
		TInt h, i, opt, optlen=0;
		h = hlen-20;
		for (i=0; i<h; i+=optlen)
			{
			opt = ptr[i];
			if (opt == 0) // KTcpOptEol
				break;
			if (opt == 1) // KTcpOptNop
				optlen = 1;
			else
				{
				if (i+1 >= h)
					break;
				optlen = ptr[i+1];
				if (optlen < 2)
					optlen = 2;
				}

			switch (opt)
				{
			case 1:
					{
					_LIT(string3,"	    NOP");
					Write(string3);
					}
				break;
			case 2:
					{
					_LIT(string4,"	    Max Seg Size = %d");
					Printf(string4, BigEndian::Get16(ptr+i+2));
					}
				break;
			default:
					{
					_LIT(string5,"	    Unknown [0x%02x]");
					Printf(string5, opt);
					}
				break;
				}
			}
		}
	
	ptr += (hlen-20);
	TInt n1 = (TInt)aDes.Ptr()-(TInt)ptr;
	aDes.Set(ptr, aDes.Length()-n1);
	return n1;
	}

TInt CPppLog::DumpIcmp(TPtrC8& aDes, TInt aLength)
	{

	if (aLength < 2)
		return 0;

	const TUint8* ptr = aDes.Ptr();

	_LIT(string1,"	    Type = %d, Code = %d");
	Printf(string1, *ptr, *(ptr+1));
	HexDump(FOURTEEN_SPACE_MARGIN, FOURTEEN_SPACE_MARGIN, ptr, aLength);
	return 0;
	}

TInt CPppLog::DumpUdp(TPtrC8& aDes, TUint32 /* aSrcA */, TUint32 /*aDstA */, TInt aLength)
	{
	if (aLength < 6)
		return 0;

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint srcp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint dstp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;

	ptr += 2;						// skip checksum
	Printf(KUDPLengthPortString, len, srcp, dstp);
	//Printf(KPortString, srcp, dstp);

	if (srcp == KDnsPort || dstp == KDnsPort)
		{
		UpdatePtr(aDes, ptr);
		return DumpDns(aDes);
		}

	// aDes.Set(remptr, remlen);
	return 0;
	}

TInt CPppLog::UpdatePtr(TPtrC8& aDes, TUint8* ptr)
	{
	TInt n = (TInt)ptr - (TInt)aDes.Ptr();
	TInt tlen = aDes.Length() - n;
	aDes.Set(ptr, tlen);
	return tlen;
	}

//
// Dump DNS request/response
//

TInt CPppLog::DumpDns(TPtrC8& aDes)
	{
	if (aDes.Length()< 12)
		return 0;

	TUint8* ptr = (TUint8*)aDes.Ptr();

	_LIT(KDnsHdrString, "\t\tDNS Id = 0x%02x, Flags = 0x%02x");
	_LIT(KDnsQuestionString, "\t\tQuestions = %d");
	_LIT(KDnsAnswerString, "\t\tAnswers = %d");
	_LIT(KDnsAuthAddString, "\t\tAuthorities = %d, Additionals = %d");

	TUint Id = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;
	TUint Flags = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;

	Printf(KDnsHdrString, Id, Flags);

	TUint nques = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;
	TUint nans = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;
	TUint nauth = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;
	TUint nadd = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;

	UpdatePtr(aDes, ptr);

	// Only deal with one query for now due to difficulty of testing

	if (nques > 0)
		{
		Printf(KDnsQuestionString, nques);
		if (DumpDnsQuery(aDes) < 0)
			return 0;
		}

	if (nques > 1)
		return 0;

	if (nans > 0)
		{
		Printf(KDnsAnswerString, nans);
		while (nans-- > 0)
			if (DumpDnsAnswer(aDes) < 0)
				return 0;
		}

	if (nauth > 0 || nadd > 0)
		Printf(KDnsAuthAddString, nauth, nadd);

	// Write(KEndOfLine);
	
	// aDes.Set(remptr, remlen);
	return 0;
	}

TInt CPppLog::DumpDnsQuery(TPtrC8& aDes)
	{
	if (DumpDnsNameAndType(aDes) < 0 || aDes.Length() < 2)
		return -1;

	UpdatePtr(aDes, (TUint8*) aDes.Ptr() + 2);	// skip over class

	return 0;
	}

TInt CPppLog::DumpDnsAnswer(TPtrC8& aDes)
	{
	_LIT(KDnsAnswerResource, "\t\t    Resource = %d.%d.%d.%d");

	if (DumpDnsNameAndType(aDes) < 0 || aDes.Length() < 8)
		return -1;

	// above check guarantees at least Class, TTL and Resource Data Length
	// fields are present

	TUint8* ptr = (TUint8*)aDes.Ptr();

	ptr += 6;						// skip over class and ttl

	TUint resourceLen = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;

	if ((TUint) aDes.Length() < 8 + resourceLen)
		return -1;

	if (resourceLen == 4)
		{
		// IPv4 address
		TUint addr = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
		Printf(KDnsAnswerResource, (addr&0xff000000)>>24,(addr&0x00ff0000)>>16,(addr&0x0000ff00)>>8,(addr&0x000000ff));
		}
		
	ptr += resourceLen;

	UpdatePtr(aDes, ptr);

	return 0;
	}

TInt CPppLog::DumpDnsNameAndType(TPtrC8& aDes)
	{
	TBuf<120>buf;
	TUint8* ptr = (TUint8*)aDes.Ptr();
	TInt length = aDes.Length();
	TUint count;

	_LIT(KDnsNameString, "\t\t  Name = ");
	_LIT(KDnsDot, ".");
	_LIT(KDnsNameCompString, "<%d>");
	_LIT(KDnsType, ", Type = %d");

	const TInt KDnsNameCompStringMaxLen = 7;
	buf.Append(KDnsNameString);

	if (length < 1)
		return -1;

	count = *ptr++;
	length--;

	while (count > 0)
		{

		// Not dealing with compression yet

		if (count & 0xc0)
			{
			if (length > 0)
				{
				count = ((count & ~0xc0) << 8) | *ptr++;
				length--;
				if (buf.Length() + KDnsNameCompStringMaxLen >= buf.MaxLength())
					return -1;
				else
					{
					buf.AppendFormat(KDnsNameCompString, count);
					break;
					}
				}
			else
				return -1;
			}

		if (count > 63)
			return -1;

		if ((TUint)length < count)
			return -1;
		length -= count;

		while (count-- > 0)
			{
			if (buf.Length() >= buf.MaxLength())
				return -1;
			buf.Append(*ptr);
			ptr++;
			}

		if (length > 0)
			{
			count = *ptr++;
			length--;
			if (count > 0)
				{
				if (buf.Length() >= buf.MaxLength())
					return -1;
				buf.Append(KDnsDot);
				}
			}
		}

	TUint type = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;
	buf.AppendFormat(KDnsType, type);

	Write(buf);

	UpdatePtr(aDes, ptr);

	return 0;
	}

#endif //ESOCK_LOGGING_ACTIVE
