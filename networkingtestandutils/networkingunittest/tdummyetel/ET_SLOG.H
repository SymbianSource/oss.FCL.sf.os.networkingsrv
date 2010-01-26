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

#if !defined(__ET_SLOG_H__)
#define __ET_SLOG_H__

#include <e32base.h>

#if defined (_DEBUG)
#define ETELLOGGER	

#endif
#ifdef _DEBUG

/**
@internalComponent
*/
_LIT(KLogFileName, "setel.txt");
_LIT(KLogFolder,   "etel");
_LIT8(KTimeFormat, "%02d.%02d:%02d:%06d ");
_LIT8(KTextFormat, "%S");

/**
KTextWidth sets up the length of the buffer per line to be shown on the screen
e.g. after 80 chars the next line is used for output
*/
 
/**
@internalComponent
*/
const TInt KTextWidth = 80;				
const TInt KETelLogBufferSize = 256;

/**
@Note writing to the file ../etel/setel.txt
@internalComponent
*/


#include <comms-infras/commsdebugutility.h>

class RETelLogger : public RFileLogger
/**
@internalComponent
*/
	{
public:
	static void Write(const TDesC8& aText);
	static void WriteFormat(TRefByValue<const TDesC8> aFmt,...);
	};

#ifdef  ETELLOGGER
#define LOGTEXT(AAA)	          RETelLogger::Write(AAA)
#define LOGTEXT2(AAA,BBB)	      RETelLogger::WriteFormat(TRefByValue<const TDesC8>(AAA),BBB)
#define LOGTEXT3(AAA,BBB,CCC)	  RETelLogger::WriteFormat(TRefByValue<const TDesC8>(AAA),BBB,CCC)
#else
#define LOGTEXT(AAA)
#define LOGTEXT2(AAA,BBB)
#define LOGTEXT3(AAA,BBB,CCC)
#endif

#define LOGTEXTREL(AAA)
#define LOGTEXTREL2(AAA,BBB)
#define LOGTEXTREL3(AAA,BBB,CCC)

#else	// release version and therefore no logging

#define LOGTEXT(AAA)
#define LOGTEXT2(AAA,BBB)
#define LOGTEXT3(AAA,BBB,CCC)

#ifdef  ETELLOGGER
#define LOGTEXTREL(AAA)	          
#define LOGTEXTREL2(AAA,BBB)	  
#define LOGTEXTREL3(AAA,BBB,CCC)  
#else
#define LOGTEXTREL(AAA)
#define LOGTEXTREL2(AAA,BBB)
#define LOGTEXTREL3(AAA,BBB,CCC)
#endif

#endif  // _DEBUG

#ifdef ETELLOGGER
#define LOGDESTROY()              RETelLogger::Destruct()
#else
#define LOGDESTROY()
#endif


#endif
