// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file et_slog.cpp Common file logging utility for ETel Server
*/

#include <e32std.h>
#include <f32file.h>
#include "ET_SLOG.H"

#if defined(ETELLOGGER)

//
// RETelLogger class definitions
//
void RETelLogger::Write(const TDesC8& aText)
	{
#if defined (SERIAL_FLOG)

	TBuf8<KEtelLogBufferSize> buf;
	TTime now;
	now.HomeTime();
	TDateTime dateTime;
	dateTime = now.DateTime();
	buf.Format(KTimeFormat,dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	buf.AppendFormat(KTextFormatSerial,&aText);
	TBuf<KEtelLogBufferSize> buf2;
	buf2.Copy(buf);
	RDebug::Print(buf2);

#else
	RFileLogger logger;
	TInt ret=logger.Connect();
	if (ret==KErrNone)
		{
		logger.SetDateAndTime(EFalse,EFalse);
		logger.CreateLog(KLogFolder, KLogFileName,EFileLoggingModeAppend);		
		TBuf8<KETelLogBufferSize> buf;
		TTime now;
		now.HomeTime();
		TDateTime dateTime;
		dateTime = now.DateTime();
		buf.Format(KTimeFormat,dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
		buf.AppendFormat(KTextFormat,&aText);

		logger.Write(buf);
		}

	logger.Close();
#endif
	}
 
void RETelLogger::WriteFormat(TRefByValue<const TDesC8> aFmt,...)
	{
    VA_LIST list;
    VA_START(list,aFmt);

#if defined (SERIAL_FLOG)
	TBuf8<KEtelLogBufferSize> buf;
	TTime now;
	now.HomeTime();
	TDateTime dateTime;
	dateTime = now.DateTime();
	buf.Format(KTimeFormat,dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	buf.AppendFormatList(aFmt, list );
	TBuf<KEtelLogBufferSize> buf2;
	buf2.Copy(buf);
	RDebug::Print(buf2);
#else
	TBuf8<2*KETelLogBufferSize> buf;
	buf.SetMax();
	buf.FillZ();
	TTime now;
	now.HomeTime();
	TDateTime dateTime;
	dateTime = now.DateTime();
	buf.Format(KTimeFormat,dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	buf.AppendFormatList(aFmt, list );

	RFileLogger logger;
	TInt ret=logger.Connect();
	if (ret==KErrNone)
		{
		logger.SetDateAndTime(EFalse,EFalse);
		logger.CreateLog(KLogFolder, KLogFileName,EFileLoggingModeAppend);
		TInt k=buf.Length();
		TInt l = 0;
		do
			{
			TBuf8<KTextWidth> myBuf;
			myBuf.Copy(&buf[l], (l<=(k-KTextWidth))?KTextWidth:(k-l));
			logger.Write(myBuf);
			l+=KTextWidth;
			}
		while(l < k);
		}

	logger.Close();
	#endif
	}

#endif // #if defined(ETELLOGGER)
