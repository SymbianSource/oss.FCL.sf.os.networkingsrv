// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @internalTechnology
*/

#ifndef TE_PUNYCODECONVERTERTESTWRAPPER_H
#define TE_PUNYCODECONVERTERTESTWRAPPER_H


#include <test/datawrapper.h>

/**
Forward declaration
*/ 
class RHostResolver;
class RSocketServ;

/**
Class implements the CDataWrapper base class and provides the commands used by the scripts file
*/
class CPunycodeConverterTestWrapper : public CDataWrapper
	{
public:
	CPunycodeConverterTestWrapper();
	~CPunycodeConverterTestWrapper();
	
	static	CPunycodeConverterTestWrapper*	NewL();
	//This function is not used currently
	virtual TAny*	GetObject() { return this; }
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	
protected:
	void ConstructL();
	
private:

	void DoResolveName(const TDesC& aSection);
	void DoResolveAddress(const TDesC& aSection);
	void DoPunyCodeToIdn(const TDesC& aSection);
	void DoIdnToPunyCode(const TDesC& aSection);
	void DoCapTest(const TDesC& aSection);
	void DoSurrogatePair(const TDesC& aSection);
	void DoEnableIdnSupport();
	void DoDisableIdnSupport();
	void DoOpenResolver();
	void DoCloseResolver();
	
private:
	TBuf<128> iNextTestCaseInput;
	RHostResolver* iHostResolver;
	TInt iHostResolverError;
	RSocketServ* iSockServ;
	TInt iSockServError;
	};

#endif //TE_PUNYCODECONVERTERTESTWRAPPER_H
