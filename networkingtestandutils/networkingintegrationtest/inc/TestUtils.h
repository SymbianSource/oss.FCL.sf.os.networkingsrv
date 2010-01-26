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
 @file TestUtils.h
 @internalComponent
*/

class CTestUtils
/**
This contains CTestUtils 
Test suites utility fuuctions

@internalComponent
*/
{
public:

	IMPORT_C  static CTestUtils * NewL(CLog * aLogSystem );

	/** public interface to run test utils. */
	IMPORT_C  void RunUtils( const TDesC& Text  );

private:
	/** second phase constructor */
	void Construct(CLog * aLogSystem );

	void RunUtilsL( const TDesC& Text  );

	void ChangeDirL (const TDesC& aDirname ); 
	void MakedirL (const TDesC&) ;
	void CopyFileL (const TDesC& anOld,const TDesC& aNew); 
	void DeleteFileL (const TDesC& aFile); 
	void MakeReadWriteL(const TDesC& aFile);

	/** pointer to Logging object which handles File and Console logging */
	CLog * iLogSystem;


};
