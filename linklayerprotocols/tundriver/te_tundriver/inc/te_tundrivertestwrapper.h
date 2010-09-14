// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @file te_tundrivertestwrapper.h
 @internalTechnology
*/

#ifndef TE_TUNDRIVERTESTWRAPPER_H
#define TE_TUNDRIVERTESTWRAPPER_H


#include <test/datawrapper.h>
#include <e32base.h>
#include <e32std.h>
#include <e32cons.h>            // Console
#include <es_sock.h>
#include <in_sock.h>
#include <nifman.h>
#include <commdbconnpref.h>
#include <metadatabase.h> 
#include "te_TcpSslTestEngine.h"

_LIT(KDummy3rdPartyApp,"Dummy3rdpartyApp.exe");
_LIT(KArg,"NULL");

/**
Forward declaration
*/ 

class RSocketServ;

/**
Class implements the CDataWrapper base class and provides the commands used by the scripts file
*/
class CTunDriverTestWrapper : public CDataWrapper
	{
public:
	CTunDriverTestWrapper();
	~CTunDriverTestWrapper();
	
	static	CTunDriverTestWrapper*	NewL();
	//This function is not used currently
	virtual TAny*	GetObject() { return this; }
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	
protected:
	void ConstructL();
		
private:
    TInt StartD3PApp();
    void StopD3PApp();
    void StartConn();
    void DoRConnTest1_1();
    void DoRConnTest1_2();
    void DoRConnTest1_3(const TDesC& aSection);
    void DoRConnTest1_4(const TDesC& aSection);
    void DoRConnTest1_5(const TDesC& aSection);
    void DoRConnTest1_6(const TDesC& aSection);
    void DoRConnTest1_7(const TDesC& aSection);
    void DoRConnTest1_8(const TDesC& aSection);
    void DoRConnTest1_9(const TDesC& aSection);
    void DoRConnTest1_10(const TDesC& aSection);
    void DoRConnTest1_11();
    void DoRConnTest1_12();
    void DoRConnTest1_13(const TDesC& aSection);
    void DoRConnTest1_14(const TDesC& aSection);
    void DoRConnTest1_15();
    void DoRConnTest1_16(const TDesC& aSection);
    void DoRConnTest1_17(const TDesC& aSection);
    void DoRConnTest1_20();
    void DoRConnTest1_21();
    void DoRConnTest1_22();    
    void DoRConnTest1_23();
    void DoRConnTest1_24();
    void DoRConnTest1_25();
    void DoRConnTest1_26();
    void DoRConnTest1_27();
    void DoRConnTest1_28();
    void DoRConnTest1_29();
    void DoRConnTest1_30();
    void DoRConnTest1_31();
	void DoRConnTest1_32();
	void DoRConnTest1_33();
    TBool FindInterface(TPckgBuf<TSoInet6InterfaceInfo> &info);

private:
	TBuf<128>          iNextTestCaseInput;
		
    TRequestStatus     iStatus;
    TInt               iVTunError;
    TUint              iInitNumOfInterfaces, iCurNumOfInterfaces;  
    TInetAddr          iDestAddr, iRealDestAddr;
    TInt               iDestPort, iRealDestPort; 
    RSocketServ        iSocketServ, iSocketServ1, iSocketServ2;
    RSocket            iSocket, iSocket1, iSocket2;
    RConnection        iConnection, iConnection1, iConnection2;
    
    RConnection        *pConn, *pConn1, *pConn2;
    RProcess           p;
    TBuf8<128>         iSendBuf, iRecvBuf;
    
    TCommDbConnPref    iCommDbPref, iCommDbPref1, iCommDbPref2;
    
	};
	

#endif //TE_TUNDRIVERTESTWRAPPER_H
