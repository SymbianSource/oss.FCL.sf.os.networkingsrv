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
 @file Script.h
 @internalComponent
*/

#if (!defined __SCRIPT_H__)
#define __SCRIPT_H__

/**
Maximum length for script line
*/
#define MAX_SCRIPT_LINE_LENGTH  200

/**
Maximum length for suite name
*/
#define MAX_LEN_SUITE_NAME		55

class CParseLine;

class CScript : public CBase
/**
Turn on script parse debugging

@internalComponent
*/
{
public:
	static CScript* NewL();
	static CScript* NewL(CParseLine * aParse );
	~CScript();	

	/** read in a script file */
	bool OpenScriptFile(const TFileName &scriptFileName);		

	/** parse and excute script */
	enum TVerdict ExecuteScriptL();						

	void DisplayResults(void);
	void AddResult(enum TVerdict iTestVerdict );
	void AddResult(CScript * subScript );
	void Pause(void);
	TBool BreakOnError(void);
	void Close(void);

	TBool iPauseAtEnd;
	TBool iMultThread;
protected:
	CScript();
	void ConstructL();
	void ConstructL(CParseLine * aParse);
private:
	/** process a line of script */
	void ProcessLineL(const TPtrC8 &narrowline, TInt8 lineNo);	

	// data members
	/** the file system */
	RFs		iTheFs;										

	/** ptr to script file im memory */
	HBufC8 *ipScriptBuffer;								

	/** test path */
	TPath	iTheTestPath;								

	/** line parse object */
	CParseLine * iParse;

	/** current results */
	TInt	iPass;
	TInt	iFail;
	TInt	iInconclusive;
	TInt	iTestSuiteError;
	TInt	iAbort;
	TInt	iTotal;	
	
	TBool iParseLineOwner;
};

#endif /* __SCRIPT_H__ */
