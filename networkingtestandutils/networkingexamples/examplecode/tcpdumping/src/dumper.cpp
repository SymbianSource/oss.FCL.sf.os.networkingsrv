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

#include <e32base.h>
#include <es_sock.h> // for BigEndian
#include "dumper.h"

_LIT(KUnixTimeBaseDes, "19700101:");
//1st phase of construction
CPktLogger::CPktLogger():iTimeOrigin(KUnixTimeBaseDes)
	{
	}

void CPktLogger::ConstructL()
/**
2nd stage construction of packet logger.

Leaves with an error if the logging file could not be created.
*/
	{
	User::LeaveIfError(iFileSvrSession.Connect());

	_LIT(KHookDumpFile, "C:\\logs\\HookLogs\\iphookslog.tcpdump");		
	TInt err = iFileObj.Open( iFileSvrSession, KHookDumpFile, EFileWrite|EFileShareAny );

	if ( err != KErrNone )
		{
		err = iFileObj.Create( iFileSvrSession, KHookDumpFile, EFileWrite|EFileShareAny );    
		if( err != KErrNone ) 
			{
			// We aren't going to do any logging, so close down the File Server session.
			iFileSvrSession.Close();
			User::Leave(err);
			}
		}
	DumpFileHeader();
	}

CPktLogger::~CPktLogger()
	{
	iFileObj.Close();
	iFileSvrSession.Close();
	}

CPktLogger* CPktLogger::NewL()
	{
	CPktLogger* loggerObj = new( ELeave ) CPktLogger;
	CleanupStack::PushL( loggerObj ); //A might-leave operation is to be followed - so give a chance to the system to do the cleanup for you.
	loggerObj->ConstructL();
	CleanupStack::Pop(); //loggerObj
	return loggerObj;
	}

CPktLogger* CPktLogger::NewLC()
	{
	CPktLogger* loggerObj = new( ELeave ) CPktLogger;
	CleanupStack::PushL( loggerObj ); //A might-leave operation is to be followed - so give a chance to the system to do the cleanup for you.
	loggerObj->ConstructL();
	return loggerObj;
	}

void CPktLogger::DumpFileHeader()
	{

	TUint8 buffer[ KDumpFileHeaderMaxSizeAllowed ];
	TUint8* ptr = 0;

	//Magic number = 4 bytes
	ptr = (TUint8*)&KTcpDumpMagicNumber;
	buffer[ 0 ] = ptr[ 0 ] ;
	buffer[ 1 ] = ptr[ 1 ] ;
	buffer[ 2 ] = ptr[ 2 ] ;
	buffer[ 3 ] = ptr[ 3 ] ;

	//Major version = 2 bytes
	ptr = (TUint8*)&KMajorVersion;
	buffer[ 4 ] = ptr[ 0 ] ;
	buffer[ 5 ] = ptr[ 1 ] ;

	//Minor version = 2 bytes
	ptr = (TUint8*)&KMinorVersion;
	buffer[ 6 ] = ptr[ 0 ] ;
	buffer[ 7 ] = ptr[ 1 ] ;

	//Time Zone = 4 bytes
	ptr = (TUint8*)&KThisZone;
	buffer[ 8 ] = ptr[ 0 ] ;
	buffer[ 9 ] = ptr[ 1 ] ;
	buffer[ 10 ] = ptr[ 2 ] ;
	buffer[ 11 ] = ptr[ 3 ] ;

	//File Length/ sigfigs??  = 4 bytes
	ptr = (TUint8*)&KSigfgs;
	buffer[ 12 ] = ptr[ 0 ] ;
	buffer[ 13 ] = ptr[ 1 ] ;
	buffer[ 14 ] = ptr[ 2 ] ;
	buffer[ 15 ] = ptr[ 3 ] ;

	//snaplen  = 4 bytes
	ptr = (TUint8*)&KSnapLen;
	buffer[ 16 ] = ptr[ 0 ] ;
	buffer[ 17 ] = ptr[ 1 ] ;
	buffer[ 18 ] = ptr[ 2 ] ;
	buffer[ 19 ] = ptr[ 3 ] ;

	// linktype = 4 bytes
	ptr = (TUint8*)&KLinkType;
	buffer[ 20 ] = ptr[ 0 ] ;
	buffer[ 21 ] = ptr[ 1 ] ;
	buffer[ 22 ] = ptr[ 2 ] ;
	buffer[ 23 ] = ptr[ 3 ] ;

	TPtr8 fileHdr( buffer, KDumpFileHeaderMaxSizeAllowed, KDumpFileHeaderMaxSizeAllowed );
	
	iFileObj.Write( fileHdr );
	}

void CPktLogger::DumpLibPCapRecordHeader( TUint32 aPktLenCaptured, TUint32 aActualPktLen )
	{

	union tag
		{
		TUint8 pktHdrBuffer[ KDumpFileRecordHeaderMaxSizeAllowed ];
		TUint32 iDummyByte; // To force alignment on 4-bytes boundary - imp on arm but not on x86!
		} buffer;

	TUint8* ptr = NULL;

	TTime timeNow;
	timeNow.UniversalTime();
	TTimeIntervalSeconds sec;
	timeNow.SecondsFrom(iTimeOrigin, sec);
	LittleEndian::Put32(&buffer.pktHdrBuffer[ 0 ], sec.Int());

	TUint captureTimeMicroSecs;
	captureTimeMicroSecs = I64LOW(timeNow.Int64());
	captureTimeMicroSecs = captureTimeMicroSecs % 1000000; // get microseconds component
	LittleEndian::Put32(&buffer.pktHdrBuffer[ 4 ], captureTimeMicroSecs);

	//a 32-bit value giving the number of bytes of packet data that were captured;
	ptr = ( ( TUint8* )( TAny* )&aPktLenCaptured );
	buffer.pktHdrBuffer[ 8 ]  = ptr[ 0 ];
	buffer.pktHdrBuffer[ 9 ]  = ptr[ 1 ];
	buffer.pktHdrBuffer[ 10 ] = ptr[ 2 ];
	buffer.pktHdrBuffer[ 11 ] = ptr[ 3 ];

	//a 32-bit value giving the actual length of the packet, in bytes
	//(which may be greater than the previous number, if you're not
	//saving the entire packet).
	ptr = ( ( TUint8* )( TAny* )&aActualPktLen ) ;
	buffer.pktHdrBuffer[ 12 ] = ptr[ 0 ];
	buffer.pktHdrBuffer[ 13 ] = ptr[ 1 ];
	buffer.pktHdrBuffer[ 14 ] = ptr[ 2 ];
	buffer.pktHdrBuffer[ 15 ] = ptr[ 3 ];

	TPtrC8 recordHdr(buffer.pktHdrBuffer, KDumpFileRecordHeaderMaxSizeAllowed);
	
	iFileObj.Write( recordHdr );
	return;
	}

void CPktLogger::DumpLibPCapRecordData( const TDesC8& aPacket )
	{
	iFileObj.Write( aPacket );
	}
