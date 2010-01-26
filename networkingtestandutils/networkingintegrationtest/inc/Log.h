/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file Log.h
*/

#include <f32file.h>

#if (!defined __LOG_H__)
#define __LOG_H__

/**	
max length of log file line
@internalComponent
*/
#define	MAX_LOG_LINE_LENGTH		256

/**	
max length of source file name
@internalComponent
*/
#define	MAX_LOG_FILENAME_LENGTH	200

/**	
severity level
@internalComponent
*/
enum TSeverity
{
	ESevrErr  = 1,
	ESevrWarn = 2,
	ESevrInfo = 4,
	ESevrAll  = 7
};

/**
The test result.
@internalAll
*/
enum TVerdict
{
	/** The test has passed.
	*/
	EPass =0,
		
	/** The test has failed.
	*/
	EFail,				

	/** The test system was unable to run the test, due to an error in another part of software. 
	*/
	EInconclusive,		

	/** The test system was unable to run the test, due to an internal error in the test system. 
	*/
	ETestSuiteError,	

	/** The test was aborted (possibly  by the test operator).
	*/
	EAbort				
};

class TIntegrationTestLog16Overflow :public TDes16Overflow
/**
Unicode overflow handler
@internalComponent
*/
	{
public:
	/** TDes16Overflow pure virtual */
	virtual void Overflow(TDes16& aDes);
	};

class CFileLogger 
/**
File Logger client interface that Controls the flogger server.
This class is responsible for providing all functions clients 
required of the flogger system.
This member is internal and not intended for use.	
@internalComponent
*/
{
public:
	IMPORT_C void CreateLog(const TDesC& aDir, const TDesC& aName);
	IMPORT_C TInt Connect( void );
	IMPORT_C void CloseLog( void );
	IMPORT_C void WriteFormat( TRefByValue<const TDesC16> format, ...  );

private:
	// handle
	RFile iLogfile;
	RFs	  iFs;
	TBool iEnabled;
};

class CLog  : public CBase
/**	
Class CLog used for all file and console log data
@internalComponent
*/
{
public:
	IMPORT_C static CLog * NewL(CConsoleBase * console);
	IMPORT_C void Construct( CConsoleBase * console );
	IMPORT_C ~CLog();

	/** open close log file */
	IMPORT_C void OpenLogFileL(const TFileName &scriptFileName);
	IMPORT_C void CloseLogFile();

	/** write to log output in Printf style */
	IMPORT_C void Log( TRefByValue<const TDesC16> format, ... );

	/** write to log output in Printf style */
	IMPORT_C void Log( TInt aSeverity, TRefByValue<const TDesC16> format, ... );

	/** write to log output in Printf style */
	IMPORT_C void Log( TRefByValue<const TDesC16> format, VA_LIST aList );

	IMPORT_C void LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt, VA_LIST aList);

	IMPORT_C void LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt, ...);

	IMPORT_C void LogResult( TVerdict ver, TRefByValue<const TDesC16> format, ... );

	/** add some blank lins to log output */
	IMPORT_C void LogBlankLine( TInt number =1 );
	
	/** convert a EPOC error to text */
	IMPORT_C static TPtrC EpocErrorToText(TInt aError);

	/** convert a test verdict to text */
	IMPORT_C const TText* TestResultText( enum TVerdict TestVerdict );

	/** get severity */
	IMPORT_C void SetSeverity( TInt aSeverity );

	/** set severity */
	IMPORT_C TInt Severity(); 

	/** set severity */				  
	IMPORT_C void SetPutSrcInfo( TBool aPutSrcInfo );

	
	inline void SetHtmlLogMode(TBool aArg)
	/** 
	set Html log mode 
	*/
	{ iHtmlLogMode = aArg;}; 

	
	inline TBool HtmlLogMode( )
	/** 
	get Html log mode 
	*/
	{return iHtmlLogMode;};

protected:
	/** log systems pointer to the current console */
	CConsoleBase * iConsole;

	/** File logging system */
	CFileLogger iFileLogger;

	/** Severity level */
	TInt iSeverity;

	/** Do we need to put information about source file & #line?
	Default is yes. */
	TBool iPutSrcInfo;

	/** Do we need to produce HTML log file?
	Default is yes. */
	TBool iHtmlLogMode;
};

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF1(p1)							LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF2(p1, p2)						LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF3(p1, p2, p3)					LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF4(p1, p2, p3, p4)				LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF5(p1, p2, p3, p4, p5)			LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4), (p5)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF6(p1, p2, p3, p4, p5, p6)		LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4), (p5), (p6)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define INFO_PRINTF7(p1, p2, p3, p4, p5, p6, p7)	LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4), (p5), (p6), (p7)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF1(p1)							LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF2(p1, p2)						LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1), (p2)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF3(p1, p2, p3)					LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1), (p2), (p3)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF4(p1, p2, p3, p4)				LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1), (p2), (p3), (p4)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF5(p1, p2, p3, p4, p5)			LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1), (p2), (p3), (p4), (p5)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF6(p1, p2, p3, p4, p5, p6)		LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1), (p2), (p3), (p4), (p5), (p6)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define WARN_PRINTF7(p1, p2, p3, p4, p5, p6, p7)	LogExtra(((TText8*)__FILE__), __LINE__, ESevrWarn, (p1), (p2), (p3), (p4), (p5), (p6), (p7)) 


/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF1(p1)								LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF2(p1, p2)							LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF3(p1, p2, p3)						LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3)) ;

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF4(p1, p2, p3, p4)					LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3), (p4)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF5(p1, p2, p3, p4, p5)				LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3), (p4), (p5)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF6(p1, p2, p3, p4, p5, p6)			LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3), (p4), (p5), (p6)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define ERR_PRINTF7(p1, p2, p3, p4, p5, p6, p7)		LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, (p1), (p2), (p3), (p4), (p5), (p6), (p7)) 


/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define TEST_START(p1)								LogExtra((TText8*)__FILE__, __LINE__, ESevrInfo, (_L("======Start test %S")), (p1)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define TEST_NEXT(p1)								LogExtra((TText8*)__FILE__, __LINE__, ESevrInfo, _L("Next test %S"), (p1)) 

/**	
@note To use this macro you have to define in your class
method LogExtra();
@internalComponent
*/
#define TEST_END()									LogExtra((TText8*)__FILE__, __LINE__, ESevrInfo, _L("======End test =====") ) 

#endif /* __LOG_H__ */
