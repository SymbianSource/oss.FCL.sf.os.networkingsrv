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

#ifndef TE_NAPTUNITTESTTESTWRAPPER_H
#define TE_NAPTUNITTESTTESTWRAPPER_H


#include <test/datawrapper.h>

/**
Forward declaration
*/ 
class RConnection;
class RSocketServ;
class RSocket;
class TInetAddr;

const TInt KNoOfErrors = 0x20;

/**
Class implements the CDataWrapper base class and provides the commands used by the scripts file
*/
class CNaptUnitTestTestWrapper : public CDataWrapper
	{
public:
	CNaptUnitTestTestWrapper();
	~CNaptUnitTestTestWrapper();
	
	static	CNaptUnitTestTestWrapper*	NewL();
	//This function is not used currently
	virtual TAny*	GetObject() { return this; }
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	
private:
	void DoConfigureNapt();
	void DoBringUpIfs(const TTEFSectionName& aSection);
	void DoCloseIfs();
	void DoConfigureNaptCustom(const TTEFSectionName& aSection);
	void DoOpenSockets();
	void DoCloseSockets();
	void DoGetOpt(const TTEFSectionName& aSection);
	void ConstructL();
	void DoOpenNaptSocket();
	void DoConnectSocketServ();
	void DoCloseSocketServ();
	void DoOpenPublicSocket();
	void DoConfigureNaptWithInvalidBuffer();
	void DoGetOptWithInvalidBuffer();
	TInt ReadIniFile(const TTEFSectionName& aSection);
	TInt GetInterfaceAddress(RSocket* aSock, const TDesC& aFName, TUint32& aAddr);
	TInt GetInterfaceIndex(RSocket* aSock,const TDesC& aFName, TUint32& aIndex);
	void ReportError();
	
private:
	enum
	{
		KSockServError = 0x1,
		KPublicConnError = 0x2,
		KPrivateConnError = 0x4,
		KPublicSocketError = 0x8,
		KPrivateSocketError = 0x10,
		KIniReadError = 0x20,
	};

	RSocketServ* iSockServ;
	TInt iErrorCode;
	TInt iErrorBit;

	RConnection* iPublicConn;
	RConnection* iPrivateConn;
	RSocket* iPrivateSocket; // NAPT socket
	RSocket* iPublicSocket;

	//Napt config params
	TInt iPublicIap;
	TInt iPrivateIap;
	TInt iUplinkAccess;
	
	TInetAddr* iPrivateIp;
	
	};

#endif //TE_NAPTUNITTESTTESTWRAPPER_H
