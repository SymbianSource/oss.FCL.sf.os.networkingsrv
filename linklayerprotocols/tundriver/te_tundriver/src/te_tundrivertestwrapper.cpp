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
// Contains the implementation of all the commands, which is used by the script file.
//

/**
 @file te_tundrivertestwrapper.h
 @internalTechnology
 */

#include "te_tundrivertestwrapper.h"
#include <e32base.h>
#include <utf.h>
#include <in_sock.h>
#include <es_sock.h>

#include <metadatabase.h> 
#include <commsdattypesv1_1.h> 
#include <commsdat.h>
#include <commsdattypesv1_1_partner.h>
#include <commsdattypeinfov1_1_internal.h>
#include <commdb.h>
#include <commdbconnpref.h>

//Virtual Interface 

// Commands

_LIT(KRConnTest1_1,         "RConnTest1_1");
_LIT(KRConnTest1_2,         "RConnTest1_2");
_LIT(KRConnTest1_3,         "RConnTest1_3");
_LIT(KRConnTest1_4,         "RConnTest1_4");
_LIT(KRConnTest1_5,         "RConnTest1_5");
_LIT(KRConnTest1_6,         "RConnTest1_6");
_LIT(KRConnTest1_7,         "RConnTest1_7");
_LIT(KRConnTest1_8,         "RConnTest1_8");
_LIT(KRConnTest1_9,         "RConnTest1_9");
_LIT(KRConnTest1_10,        "RConnTest1_10");
_LIT(KRConnTest1_11,        "RConnTest1_11");
_LIT(KRConnTest1_12,        "RConnTest1_12");
_LIT(KRConnTest1_13,        "RConnTest1_13");
_LIT(KRConnTest1_14,        "RConnTest1_14");
_LIT(KRConnTest1_15,        "RConnTest1_15");
_LIT(KRConnTest1_16,        "RConnTest1_16");
_LIT(KRConnTest1_17,        "RConnTest1_17");
_LIT(KRConnTest1_18,        "RConnTest1_18");
_LIT(KRConnTest1_19,        "RConnTest1_19");
_LIT(KRConnTest1_20,        "RConnTest1_20");
_LIT(KRConnTest1_21,        "RConnTest1_21");
_LIT(KRConnTest1_22,        "RConnTest1_22");
_LIT(KRConnTest1_23,        "RConnTest1_23");
_LIT(KRConnTest1_24,        "RConnTest1_24");
_LIT(KRConnTest1_25,        "RConnTest1_25");
_LIT(KRConnTest1_26,        "RConnTest1_26");
_LIT(KRConnTest1_27,        "RConnTest1_27");
_LIT(KRConnTest1_28,        "RConnTest1_28");
_LIT(KRConnTest1_29,        "RConnTest1_29");
_LIT(KRConnTest1_30,        "RConnTest1_30");
_LIT(KRConnTest1_31,        "RConnTest1_31");
_LIT(KRConnTest1_32,        "RConnTest1_32");
_LIT(KRConnTest1_33,        "RConnTest1_33");

// Config file
_LIT(KNameDefault, 	    	"default");
_LIT(KIpAddr,               "IpAddr");
_LIT(KGatewayAddr,          "GatewayAddr");
_LIT(KDestAddr, 		    "DestAddr");
_LIT(KLocalAddr,            "LocalAddr");
_LIT(KLocalPort,            "LocalPort");
_LIT(KDestPort,             "DestPort");
_LIT(KTcpDestPort,          "TcpDestPort");
_LIT(KRealDestAddr,         "RealDestAddr");
_LIT(KRealDestPort,         "RealDestPort");

//Test code
_LIT(KCommand, 			    "aCommand = %S");
_LIT(KSection, 			    "aSection = %S");

//Test code error
_LIT(KInetInputFail,         "TInetAddr Input failed with error: %d");

#define TUNDRIVER_IAP   14
#define REAL_IAP    9
#define LOCAL_PORT 7777

// public
//LOCAL_D CConsoleBase* console; 


/**
Constructor.

@internalTechnology
 */
CTunDriverTestWrapper::CTunDriverTestWrapper()
    {
    }

/**
Destructor.

@internalTechnology
 */
CTunDriverTestWrapper::~CTunDriverTestWrapper()
    {
    }

/**
Function to instantiate TestWrapper.
@return Returns constructed TestWrapper instance pointer
@internalTechnology
 */
CTunDriverTestWrapper* CTunDriverTestWrapper::NewL()
    {
    CTunDriverTestWrapper*	ret = new (ELeave) CTunDriverTestWrapper();
    CleanupStack::PushL(ret);
    ret->ConstructL();
    CleanupStack::Pop(ret);
    return ret;	
    }

/**
Second level constructor, constructs TestWrapper instance.
@internalTechnology
 */
void CTunDriverTestWrapper::ConstructL()
    {
    iInitNumOfInterfaces    = 0;
    iCurNumOfInterfaces     = 0;
    iDestPort               = 0;

    }

/**
Function to map the input command to respective function.

@return - True Upon successfull command to Function name mapping otherwise False
@param aCommand Function name has to be called
@param aSection INI file paramenter section name
@param aAsyncErrorIndex Error index
@see Refer the script file COMMAND section.

@internalTechnology
 */
TBool CTunDriverTestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
    {
    TBool ret = ETrue;

    // Print out the parameters for debugging
    INFO_PRINTF2( KCommand, &aCommand );
    INFO_PRINTF2( KSection, &aSection );
    //INFO_PRINTF2( KAsyncErrorIndex, aAsyncErrorIndex );

    //RConnection Testcases
    if(KRConnTest1_1() == aCommand)
        {
        DoRConnTest1_1();
        }
    else if(KRConnTest1_2() == aCommand)
        {
        DoRConnTest1_2();
        }
    else if(KRConnTest1_3() == aCommand)
        {
        DoRConnTest1_3(aSection);
        }
    else if(KRConnTest1_4() == aCommand)
        {
        DoRConnTest1_4(aSection);
        }
    else if(KRConnTest1_5() == aCommand)
        {
        DoRConnTest1_5(aSection);
        }
    else if(KRConnTest1_6() == aCommand)
        {
        DoRConnTest1_6(aSection);
        }
    else if(KRConnTest1_7() == aCommand)
        {
        DoRConnTest1_7(aSection);
        }
    else if(KRConnTest1_8() == aCommand)
        {
        DoRConnTest1_8(aSection);
        }
    else if(KRConnTest1_9() == aCommand)
        {
        DoRConnTest1_9(aSection);
        }
    else if(KRConnTest1_10() == aCommand)
        {
        DoRConnTest1_10(aSection);
        }
    else if(KRConnTest1_11() == aCommand)
        {
        DoRConnTest1_11();
        }
    else if(KRConnTest1_12() == aCommand)
        {
        DoRConnTest1_12();
        }
    else if(KRConnTest1_13() == aCommand)
        {
        DoRConnTest1_13(aSection);
        }
    else if(KRConnTest1_14() == aCommand)
        {
        DoRConnTest1_14(aSection);
        }
    else if(KRConnTest1_15() == aCommand)
        {
        DoRConnTest1_15();
        }
    else if(KRConnTest1_16() == aCommand)
        {
        DoRConnTest1_16(aSection);
        }
    else if(KRConnTest1_17() == aCommand)
        {
        DoRConnTest1_17(aSection);
        }
    else if(KRConnTest1_18() == aCommand)
        {
        
        }
    else if(KRConnTest1_19() == aCommand)
        {
    
        }
    else if(KRConnTest1_20() == aCommand)
        {
        DoRConnTest1_20();
        }
    else if(KRConnTest1_21() == aCommand)
        {
        DoRConnTest1_21();
        }
    else if(KRConnTest1_22() == aCommand)
        {
        DoRConnTest1_22();
        }
    else if(KRConnTest1_23() == aCommand)
        {
        DoRConnTest1_23();
        }
    else if(KRConnTest1_24() == aCommand)
        {
        DoRConnTest1_24();
        }
    else if(KRConnTest1_25() == aCommand)
        {
        DoRConnTest1_25();
        }
    else if(KRConnTest1_26() == aCommand)
        {
        DoRConnTest1_26();
        }
    else if(KRConnTest1_27() == aCommand)
        {
        DoRConnTest1_27();
        }
    else if(KRConnTest1_28() == aCommand)
        {
        DoRConnTest1_28();
        }
    else if(KRConnTest1_29() == aCommand)
        {
        DoRConnTest1_29();
        }
    else if(KRConnTest1_30() == aCommand)
        {
        DoRConnTest1_30();
        }
	else if(KRConnTest1_31() == aCommand)
        {
        DoRConnTest1_31();
        }
	else if(KRConnTest1_32() == aCommand)
        {
        DoRConnTest1_32();
        }
    else if(KRConnTest1_33() == aCommand)
        {
        DoRConnTest1_33();
        }	
    else
        {
        ret = EFalse;
        User::LeaveIfError(KErrNone); // just to suppress LeaveScan warning
        }
    return ret;
    }

/**
Function to open Virtual Socket.
@internalTechnology
 */

TBool CTunDriverTestWrapper::FindInterface(TPckgBuf<TSoInet6InterfaceInfo> &info)
    {
    TBool success = FALSE;
    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());

        return success;
        }

    if((iStatus = iConnection1.Open(iSocketServ)) == KErrNone )
        INFO_PRINTF1(_L("Connection1 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection1: %d"), iStatus.Int());
        SetError(iStatus.Int());

        return success;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection1.Start(iCommDbPref, iStatus);


    //open socket on interface. 
    if((iStatus = iSocket1.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection1)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));

    User::LeaveIfError( iSocket1.SetOpt( KSoInetEnumInterfaces, KSolInetIfCtrl ) );

    TProtocolDesc in;
    User::LeaveIfError( iSocket1.Info( in ) );
    TInt res = iSocket1.GetOpt( KSoInetNextInterface, KSolInetIfCtrl, info );
    if (res != KErrNone)
        {
        User::Leave( res );
        }
    
    TPckgBuf<TSoInet6InterfaceInfo> next;


    while (res == KErrNone)
        {
        res = iSocket1.GetOpt( KSoInetNextInterface, KSolInetIfCtrl, next );
        if (res == KErrNone)
            info = next;
        //Compare the string with interface name
        if (info().iName.Mid(1,4).Compare(_L("TunDriver")))
            //if (info().iName == KIfName)
            {
            success = TRUE;
            break;

            }
        else
            {
            continue;
            }
        }

    return success;


    }

TInt CTunDriverTestWrapper::StartD3PApp()
    {
    TInt err(0);
    if ((err = p.Create(KDummy3rdPartyApp,KArg)) == KErrNone)
        {
        p.Resume();
        }
    else
        {
        ERR_PRINTF2(_L("Error Opening Dummy3rdPartyApp: %d"), iVTunError);
        SetError(err);
        return(err);        
        }
    //Wait for Dummy3rdPartyApp to be up and running!
    User::After(10000000);   //10 Sec
    return(err);
    }

void CTunDriverTestWrapper::StopD3PApp()
    {
    p.Kill(NULL);   //Kill Dummy3rdPartyApp.       
    }

//Gurpreet: RConnection Testcases

void CTunDriverTestWrapper::DoRConnTest1_1()
    {
    INFO_PRINTF1(_L("********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_1             "));
    INFO_PRINTF1(_L("********************************************"));

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iVTunError = iConnection.Start(iCommDbPref);
    //User::WaitForRequest(iStatus);
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iVTunError);

    if(iVTunError!= KErrNone)
        {
        SetError(iVTunError);
        }
    iConnection.Close();
    iSocketServ.Close();    
    }

void CTunDriverTestWrapper::DoRConnTest1_2()
    {
    INFO_PRINTF1(_L("********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_2             "));
    INFO_PRINTF1(_L("********************************************"));

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    iConnection.Close();
    iSocketServ.Close();    
    }

void CTunDriverTestWrapper::DoRConnTest1_3(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_3             "));
    INFO_PRINTF1(_L("********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection.Start(iCommDbPref, iStatus);
		User::WaitForRequest(iStatus);
		if (iStatus.Int() != KErrNone)
			{
			ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
			SetError(iStatus.Int());
			StopD3PApp();
			return;
			}
	INFO_PRINTF1(_L("Connection started!\n "));

    //open socket on interface. 
    if((iStatus = iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Nokia sues Apple!";

    // Get destination address from config file   
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    iDestAddr.SetPort(iDestPort);

    //bind local port to socket
    err = iSocket.SetLocalPort(7776);

    //send data to server
    iSocket.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent"));
    else
	   {
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
  	    }

    iRecvBuf.Zero();
    iSocket.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed"));  
        SetError(iStatus.Int());
        }
    iSocket.Close();
    iSocketServ.Close();
    
    StopD3PApp();
    
    }

void CTunDriverTestWrapper::DoRConnTest1_4(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_4             "));
    INFO_PRINTF1(_L("********************************************"));

		//start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    if (iStatus.Int() != KErrNone)
		{
		ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iSocketServ.Close();
		StopD3PApp();
		return;
		}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    //open socket on interface. 

    if((iStatus = iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Nokia sues Apple!";

    // Get destination address from config file  
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    iDestAddr.SetPort(iDestPort);

    //bind local port to socket
    err = iSocket.SetLocalPort(LOCAL_PORT);

    //send data to server
    iSocket.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent"));
    else
	   {
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
  	    }

    iRecvBuf.Zero();
    iSocket.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed"));
        SetError(iStatus.Int());
        }
    iSocket.Close();
    iConnection.Close();
    iSocketServ.Close();    

    StopD3PApp();
    
    }

void CTunDriverTestWrapper::DoRConnTest1_5(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_5             "));
    INFO_PRINTF1(_L("********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }
    if((iStatus = iConnection.Open(iSocketServ)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    if (iStatus.Int() != KErrNone)
		{
		ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iSocketServ.Close();
		StopD3PApp();
		return;
		}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    //open socket on interface. 
    if((iStatus = iSocket1.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket1 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket1: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    if((iStatus = iSocket2.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket2 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket2: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    // Get destination address from config file 
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket1.Close();
        iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iSocket1.Close();
        iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    iDestAddr.SetPort(iDestPort);

    //bind local port to socket
    err = iSocket1.SetLocalPort(LOCAL_PORT);
    if(err != KErrNone)
    	{
        ERR_PRINTF2(_L("Error Binding Socket1: %d"), err);    
        SetError(err);
        iSocket1.Close();
        iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
      	}
    //bind local port to socket
    err = iSocket2.SetLocalPort(LOCAL_PORT + 1);
    if(err != KErrNone)
    	{
        ERR_PRINTF2(_L("Error Binding Socket2: %d"), err);
        SetError(err);
        iSocket1.Close();
        iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
      	}

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Nokia sues Apple!";//"Sending Data through Socket1!";

    //send data to server
    iSocket1.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);

    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru socket1"));
    else
    	{
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
        iSocket1.Close();
        iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
      	}

    iRecvBuf.Zero();
    iSocket1.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket1 : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
    	{
        ERR_PRINTF1(_L("\nData receive failed on Socket1"));  
        SetError(iStatus.Int());
        iSocket1.Close();
        iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
      	}

    iSendBuf.Zero();
    iSendBuf = (TText8*)" Socket2:Nokia sues Apple!";
    iSocket2.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);

    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru socket2"));
    else
	    {
		ERR_PRINTF1(_L("\nData sending failed"));
	    SetError(iStatus.Int());
	    iSocket1.Close();
	    iSocket2.Close();
	    iConnection.Close();
	    iSocketServ.Close();
	    StopD3PApp();
	    }

    iRecvBuf.Zero();
    iSocket2.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    temp.Zero();
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket2: "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket2"));
        SetError(iStatus.Int());
        }
    iSocket1.Close();
    iSocket2.Close();
    iConnection.Close();
    iSocketServ.Close();
    
    StopD3PApp();
    
    }

void CTunDriverTestWrapper::DoRConnTest1_6(const TDesC& aSection)
    {

    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_6              "));
    INFO_PRINTF1(_L("*********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }
    if((iStatus = iConnection1.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection1 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    if((iStatus = iConnection2.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection2 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection1.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
   	
    if(iStatus.Int()!=KErrNone)
	   	{
	   	ERR_PRINTF2(_L("Error starting connection1: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iSocketServ.Close();
		StopD3PApp();
		return;
	    }
	    INFO_PRINTF2(_L("Starting Connection1 %d.\n"), iStatus.Int());
        	

    iConnection2.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    if(iStatus.Int()!=KErrNone)
	    {
	    ERR_PRINTF2(_L("Error starting connection2: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iConnection1.Close();
		iSocketServ.Close();
		StopD3PApp();
		return;
	    }

    INFO_PRINTF2(_L("Starting Connection2 %d.\n"), iStatus.Int());

    //open socket on interface. 
    if((iStatus = iSocket1.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection1)) == KErrNone )
        INFO_PRINTF1(_L("Socket1 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket1: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    if((iStatus = iSocket2.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection2)) == KErrNone )
        INFO_PRINTF1(_L("Socket2 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket2: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iSocket1.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Socket1: Nokia sues Apple!";

    // Get destination address from config file    
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    iDestAddr.SetPort(iDestPort);
    //bind local port to socket
    err = iSocket1.SetLocalPort(LOCAL_PORT);
    if(err != KErrNone)
		{
	    ERR_PRINTF1(_L("Error in setting local port to socket1"));
	    SetError(err);
	    iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ.Close();
	    StopD3PApp();
	    return;
	    }

    err = iSocket2.SetLocalPort(LOCAL_PORT+1);
    if(err != KErrNone)
	    {
		ERR_PRINTF1(_L("Error in setting local port to socket1"));
	    SetError(err);
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ.Close();
	    StopD3PApp();
	    return;
	    }
	    //send data to server
    iSocket1.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
	  if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent for socket1"));
    else
	    {
	    ERR_PRINTF1(_L("\nData sending failed for socket1"));
		SetError(iStatus.Int());
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ.Close();
	    StopD3PApp();
	    return;
	    }


    iSendBuf.Zero();
    iSendBuf = (TText8*)"Socket2: Nokia sues Apple!";
    iSocket2.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent for socket2"));
    else
	    {
	    ERR_PRINTF1(_L("\nData sending failed for socket2"));
		SetError(iStatus.Int());
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ.Close();
	    StopD3PApp();
      	return;
     	}

    iRecvBuf.Zero();
    iSocket1.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket1 : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
	    {
  	    ERR_PRINTF1(_L("\nData receive failed on Socket1"));
		SetError(iStatus.Int());
		iSocket1.Close();
	    iSocket2.Close();
  	    iConnection1.Close();
    	iConnection2.Close();
      	iSocketServ.Close();
	    StopD3PApp();
  	    return;
    	}


    iSendBuf.Zero();
    iSendBuf = (TText8*)"Sending Data through Socket2!";
    iSocket2.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru socket2"));
    else
	    {
        ERR_PRINTF1(_L("\nData sending failed through socket2"));
		SetError(iStatus.Int());
		iSocket1.Close();
	    iSocket2.Close();
  	    iConnection1.Close();
    	iConnection2.Close();
      	iSocketServ.Close();
	    StopD3PApp();
  	    return;
    	}


    iRecvBuf.Zero();
    iSocket2.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    temp.Zero();
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket2: "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket2"));
        SetError(iStatus.Int());
        }
    iSocket1.Close();
    iSocket2.Close();
    iConnection1.Close();
    iConnection2.Close();
    iSocketServ.Close();
    StopD3PApp();
    }

//Sockets sending concurrently over same interface but from different socket servers
//(Connection explicitly created)
void CTunDriverTestWrapper::DoRConnTest1_7(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_7              "));
    INFO_PRINTF1(_L("*********************************************"));

	  //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ1.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server1 Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iSocketServ2.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server2 Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ1.Close();
        StopD3PApp();
        return;
        }
    if((iStatus = iConnection1.Open(iSocketServ1 /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection1 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    if((iStatus = iConnection2.Open(iSocketServ2 /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection2 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection1.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
	if(iStatus.Int()!=KErrNone)
	    {
	    ERR_PRINTF2(_L("Error starting connection1: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iSocketServ1.Close();
		iSocketServ2.Close();
		StopD3PApp();
		return;
	    }
    INFO_PRINTF2(_L("Starting Connection1 %d.\n"), iStatus.Int());

    iConnection2.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
	if(iStatus.Int()!=KErrNone)
		{
	    ERR_PRINTF2(_L("Error starting connection2: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iConnection1.Close();
		iSocketServ1.Close();
		iSocketServ2.Close();
		StopD3PApp();
		return;
	    }
    INFO_PRINTF2(_L("Starting Connection2 %d.\n"), iStatus.Int());

    //Enumerate the number of interfaces
    iConnection1.EnumerateConnections(iInitNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces: %d!\n "), iInitNumOfInterfaces);

    //open socket on interface. 
    if((iStatus = iSocket1.Open(iSocketServ1, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection1)) == KErrNone )
        INFO_PRINTF1(_L("Socket1 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket1: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }
    if((iStatus = iSocket2.Open(iSocketServ2, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection2)) == KErrNone )
        INFO_PRINTF1(_L("Socket2 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket2: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iSocket1.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Socket1: Nokia sues Apple!";

    // Get destination address from config file   
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();        
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestPort"));   
        SetError(KErrUnknown);
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }    
    iDestAddr.SetPort(iDestPort);
    //bind local port to socket
    err = iSocket1.SetLocalPort(LOCAL_PORT);

    if(err != KErrNone)
		{
	    ERR_PRINTF1(_L("Error in setting local port to socket1"));
	    SetError(err);
	    iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ1.Close();
	    iSocketServ2.Close();
	    StopD3PApp();
	    return;
	    }

    err = iSocket2.SetLocalPort(LOCAL_PORT+1);
    if(err != KErrNone)
		{
		ERR_PRINTF1(_L("Error in setting local port to socket1"));
	    SetError(err);
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ1.Close();
	    iSocketServ2.Close();
	    StopD3PApp();
	    return;
	    }
    //send data to server
    iSocket1.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
		User::WaitForRequest(iStatus);
	  if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent for socket1"));
    else
	    {
	    ERR_PRINTF1(_L("\nData sending failed for socket1"));
		SetError(iStatus.Int());
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ1.Close();
	    iSocketServ2.Close();
	    StopD3PApp();
	    return;
	    }
    iRecvBuf.Zero();
    iSocket1.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket1 : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
    	{
        ERR_PRINTF1(_L("\nData receive failed on Socket1"));
        SetError(iStatus.Int());
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
      	}

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Socket2: Nokia sues Apple!";
    iSocket2.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent for socket2"));
    else
    	{
        ERR_PRINTF1(_L("\nData sending failed for socket2"));
        SetError(iStatus.Int());
        iSocket1.Close();
        iSocket2.Close();
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;        
      	}


    iRecvBuf.Zero();
    User::After(30000);
    iSocket2.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    temp.Zero();
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket2: "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket2"));
        SetError(iStatus.Int());
        }

    iSocket1.Close();
    iSocket2.Close();
    iConnection1.Close();
    iConnection2.Close();
    iSocketServ1.Close();      
    iSocketServ2.Close();
    StopD3PApp();
    }


//Sockets sending from within different socket servers over different interfaces
//(Connection explicitly created)
void CTunDriverTestWrapper::DoRConnTest1_8(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_8             "));
    INFO_PRINTF1(_L("*********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ1.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server1 Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    if((iStatus = iSocketServ2.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server2 Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }
    if((iStatus = iConnection1.Open(iSocketServ1)) == KErrNone )
        INFO_PRINTF1(_L("Connection1 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    if((iStatus = iConnection2.Open(iSocketServ2)) == KErrNone )
        INFO_PRINTF1(_L("Connection2 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iCommDbPref1.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref1.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref1.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection1.Start(iCommDbPref1, iStatus);
    User::WaitForRequest(iStatus);
	if(iStatus.Int()!=KErrNone)
		{
	    ERR_PRINTF2(_L("Error starting connection1: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iSocketServ1.Close();
		iSocketServ2.Close();
		StopD3PApp();
		return;
	    }
    INFO_PRINTF2(_L("Starting Connection1 %d.\n"), iStatus.Int());

    iCommDbPref2.SetIapId(REAL_IAP);             // Ethernet Interface
    iCommDbPref2.SetBearerSet(KCommDbBearerLAN);
    iCommDbPref2.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection2.Start(iCommDbPref2, iStatus);
    User::WaitForRequest(iStatus);
	if(iStatus.Int()!=KErrNone)
		{
		ERR_PRINTF2(_L("Error starting connection2: %d"), iStatus.Int());	
		SetError(iStatus.Int());
		iSocketServ1.Close();
		iSocketServ2.Close();
		StopD3PApp();
		return;
	    }

    INFO_PRINTF2(_L("Starting Connection2 %d.\n"), iStatus.Int());

    //open socket on interface. 
    if((iStatus = iSocket1.Open(iSocketServ1, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection1)) == KErrNone )
        INFO_PRINTF1(_L("Socket1 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket1: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }
    if((iStatus = iSocket2.Open(iSocketServ2, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection2)) == KErrNone )
        INFO_PRINTF1(_L("Socket1 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket2: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iSocket1.Close();
				iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Socket1: Nokia sues Apple!";

    // Get destination address from config file   
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
		iSocket1.Close();
		iSocket2.Close();
		iConnection1.Close();
        iConnection2.Close();
        iSocketServ1.Close();
        iSocketServ2.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    iDestAddr.SetPort(iDestPort);
    //bind local port to socket
    err = iSocket1.SetLocalPort(LOCAL_PORT);
    if(err != KErrNone)
	    {
		ERR_PRINTF1(_L("Error in setting local port to socket1"));
	    SetError(err);
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ1.Close();
	    iSocketServ2.Close();
	    StopD3PApp();
	    return;
	    }

    //read Real interface ip addr and port from config file
    returnValue = GetStringFromConfig(aSection, KRealDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading RealDestAddr"));   
        SetError(KErrUnknown);
		iSocket1.Close();
	    iSocket2.Close();
	    iConnection1.Close();
	    iConnection2.Close();
	    iSocketServ1.Close();
	    iSocketServ2.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    err = iRealDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
				iSocket1.Close();
	      iSocket2.Close();
	      iConnection1.Close();
	      iConnection2.Close();
	      iSocketServ1.Close();
	      iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KRealDestPort, iRealDestPort);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading RealDestAddr"));   
        SetError(KErrUnknown);
				iSocket1.Close();
	      iSocket2.Close();
	      iConnection1.Close();
	      iConnection2.Close();
	      iSocketServ1.Close();
	      iSocketServ2.Close();
        StopD3PApp();
        return;
        }
    iRealDestAddr.SetPort(iRealDestPort);
    //bind local port to socket
    //err = iSocket2.SetLocalPort(LOCAL_PORT);
    //send data to server
    iSocket1.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru virtual interface"));
    else
    		{
        ERR_PRINTF1(_L("\nData sending failed through virtual interface"));
				SetError(iStatus.Int());
				iSocket1.Close();
	      iSocket2.Close();
	      iConnection1.Close();
	      iConnection2.Close();
	      iSocketServ1.Close();
	      iSocketServ2.Close();
        StopD3PApp();
        return;
      	}

    iRecvBuf.Zero();
    iSocket1.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket1 : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket1"));
        SetError(iStatus.Int());
				iSocket1.Close();
	      iSocket2.Close();
	      iConnection1.Close();
	      iConnection2.Close();
	      iSocketServ1.Close();
	      iSocketServ2.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Socket2: Nokia sues Apple!";
    iSocket2.SendTo(iSendBuf, iRealDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru real interface"));
    else
    		{
        ERR_PRINTF1(_L("\nData sending failed through real interface"));
        SetError(iStatus.Int());
				iSocket1.Close();
	      iSocket2.Close();
	      iConnection1.Close();
	      iConnection2.Close();
	      iSocketServ1.Close();
	      iSocketServ2.Close();
        StopD3PApp();
        return;

				}
    iRecvBuf.Zero();
    iSocket2.RecvFrom(iRecvBuf, iRealDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    temp.Zero();
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket2: "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket2"));
        SetError(iStatus.Int());
        }

    iSocket1.Close();
    iSocket2.Close();
    iConnection1.Close();
    iConnection2.Close();
    iSocketServ1.Close();      
    iSocketServ2.Close();
    StopD3PApp();
    }

void CTunDriverTestWrapper::DoRConnTest1_9(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
        INFO_PRINTF1(_L("           Test: DoRConnTest1_9             "));
        INFO_PRINTF1(_L("********************************************"));
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
    CMDBSession *cmdbSession = CMDBSession::NewL(KCDVersion1_2);
#else
    CMDBSession *cmdbSession = CMDBSession::NewL(KCDVersion1_1);
#endif
    CleanupStack::PushL(cmdbSession); 

    CCDIAPRecord *iapRecord = (CCDIAPRecord*)CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord);
    CleanupStack::PushL(iapRecord);

    iapRecord->SetRecordId(14);

    iapRecord->LoadL(*cmdbSession);

    //iapRecord->iServiceType;
    TUint recid = iapRecord->iService;
    CCDLANServiceRecord *lanRecord = (CCDLANServiceRecord*)CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord);
    CleanupStack::PushL(lanRecord);

    lanRecord->SetRecordId(recid);

    lanRecord->LoadL(*cmdbSession);

    // Get gateway address from config file  
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KGatewayAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        return;
        }

    // Create address
    TInetAddr gwAddr;
    TInt err = gwAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        return;
        }
    TPtrC displayIpAddr(lanRecord->iIpAddr);
    //INFO_PRINTF2(_L("lanRecord ipAddr is %S"), &displayIpAddr); 

    TPtrC ipGateway(_L("10.1.1.100"));
    //TPtrC ipGateway(ptrToReadFromConfig);
    lanRecord->iIpGateway.SetMaxLengthL(ipGateway.Length());
    lanRecord->iIpGateway = ipGateway;


    // Get gateway address from config file         
    returnValue = GetStringFromConfig(aSection, KIpAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        return;
        }
    //TPtrC ipAddr(_L("10.1.1.100"));
    TPtrC ipAddr(ptrToReadFromConfig);
    lanRecord->iIpAddr.SetMaxLengthL(ipAddr.Length());
    lanRecord->iIpAddr = ipAddr;

    TRAPD(modifyErr, lanRecord->ModifyL(*cmdbSession));
    INFO_PRINTF2(_L("lanRecord ModifyL result was %d.  Should be 0"), modifyErr);

    TPtrC displayIpAddr2(lanRecord->iIpAddr);
    INFO_PRINTF2(_L("lanRecord ipAddr is %S"), &displayIpAddr2);    
    CleanupStack::PopAndDestroy(lanRecord);
    CleanupStack::PopAndDestroy(iapRecord);
    CleanupStack::PopAndDestroy(cmdbSession);


    }

//Ensure that Close() effectively pulls down the interface
//when no other subsessions are associated with it.
void CTunDriverTestWrapper::DoRConnTest1_10(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_10             "));
    INFO_PRINTF1(_L("********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }
    if((iStatus = iConnection.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    if (iStatus.Int() != KErrNone)
				{
				ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				StopD3PApp();
				return;
				}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    //Enumerate the number of interfaces
    iConnection.EnumerateConnections(iInitNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces: %d!\n "), iInitNumOfInterfaces);

    //open socket on interface. 
    if((iStatus = iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Nokia sues Apple!";

    // Get destination address from config file  
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iDestAddr.SetPort(iDestPort);
    //bind local port to socket
    err = iSocket.SetLocalPort(LOCAL_PORT);
    if (err != KErrNone)
        {
        ERR_PRINTF1(_L("Error setting local port"));   
        SetError(err);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    //send data to server
    iSocket.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent"));
    else
    		{
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
				}
    
    iRecvBuf.Zero();
    iSocket.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed."));
        SetError(iStatus.Int());
        }  

    iSocket.Close();
    iConnection.Close();
    iSocketServ.Close();
    
    StopD3PApp();
    
    }

//Ensure that Close() does not pull down the interface when there are other 
//connections associated with it.
void CTunDriverTestWrapper::DoRConnTest1_11()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_11             "));
    INFO_PRINTF1(_L("*********************************************"));

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iConnection1.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection1 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iConnection2.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection2 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection1.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection1: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection1 %d.\n"), iStatus.Int());
    iConnection2.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection2: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection2 %d.\n"), iStatus.Int());

    //Enumerate the number of interfaces
    iConnection2.EnumerateConnections(iInitNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces: %d!\n "), iInitNumOfInterfaces);

    iConnection1.Close();
    User::After(30000);

    iConnection2.EnumerateConnections(iCurNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces after closing 1st Connection: %d!\n "), iCurNumOfInterfaces);
    if(iCurNumOfInterfaces == iInitNumOfInterfaces)
    		INFO_PRINTF1(_L("Number of interfaces before and after closing the connection is same"));
 		else
        {
        ERR_PRINTF1(_L("ERROR: More Number of Interfaces than expected."));    
        SetError(KErrGeneral);
        return;
        }
    iConnection2.Stop();
    User::After(30000);
    iSocketServ.Close();      
    }

//Ensure that Stop() pulls down the interface when there are no subsessions
//other than the connection associated with it.
void CTunDriverTestWrapper::DoRConnTest1_12()
    {

    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_12             "));
    INFO_PRINTF1(_L("*********************************************"));

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

		iCommDbPref1.SetIapId(REAL_IAP);             // Virtual Interface
    iCommDbPref1.SetBearerSet(KCommDbBearerLAN);
    iCommDbPref1.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    
    iConnection.Start(iCommDbPref1,iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    //Enumerate the number of interfaces
    iConnection.EnumerateConnections(iInitNumOfInterfaces);
    INFO_PRINTF2(_L("Number of initial Interfaces: %d!\n "), iInitNumOfInterfaces);

    TInt MyErr = iConnection.Stop();
    User::After(30000);
    iConnection.EnumerateConnections(iCurNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces after stopping the connection: %d!\n "), iCurNumOfInterfaces);
    if(iCurNumOfInterfaces>=iInitNumOfInterfaces)
        {
        ERR_PRINTF1(_L("ERROR: More Number of Interfaces than expected."));    
        SetError(KErrGeneral);
        return;
        }
    iSocketServ.Close();      
    }

//Ensure that Stop() pulls down the interface when 
//there are sockets associated with it.
void CTunDriverTestWrapper::DoRConnTest1_13(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_13             "));
    INFO_PRINTF1(_L("*********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				StopD3PApp();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    //open socket on interface. 
    if((iStatus = iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Sending Data through Socket!";

    // Get destination address from config file   
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iDestAddr.SetPort(iDestPort);
    //bind local port to socket
    err = iSocket.SetLocalPort(LOCAL_PORT);
    if (err != KErrNone)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(err);
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    //send data to server
    iSocket.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent"));
    else
    		{
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
        iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;        
      	}

    iRecvBuf.Zero();
    iSocket.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone)
        {
        INFO_PRINTF1(_L("\nData Received from Socket : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket"));
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;        
        }  


    //Enumerate the number of interfaces
    iConnection.EnumerateConnections(iInitNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Init Interfaces: %d!\n "), iInitNumOfInterfaces);

    iConnection.Stop();
    User::After(30000);
    iConnection.EnumerateConnections(iCurNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces after Stopping the interface: %d!\n "), iCurNumOfInterfaces);
    if(iCurNumOfInterfaces>=iInitNumOfInterfaces)
        {
        ERR_PRINTF1(_L("ERROR: More Number of Interfaces than expected."));    
        SetError(KErrGeneral);
        }
    iSocket.Close();
    iSocketServ.Close();    
    StopD3PApp();
    }

//Stop() a connection and ensure that other connections over 
//other interfaces are unaffected. 
void CTunDriverTestWrapper::DoRConnTest1_14(const TDesC& aSection)
    {

    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_14             "));
    INFO_PRINTF1(_L("*********************************************"));

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        
        return;
        }

    if((iStatus = iConnection1.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection1 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection1: %d"), iStatus.Int());
        SetError(iStatus.Int());
        
        return;
        }
    iCommDbPref1.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref1.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref1.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    iConnection1.Start(iCommDbPref1,iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection1: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection1 %d.\n"), iStatus.Int());

    if((iStatus = iConnection2.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection2 opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection2: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ.Close();
        return;
        }
    iCommDbPref2.SetIapId(REAL_IAP);             // Virtual Interface
    iCommDbPref2.SetBearerSet(KCommDbBearerLAN);
    iCommDbPref2.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    iConnection2.Start(iCommDbPref2, iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection2: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iConnection1.Close();
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection2 %d.\n"), iStatus.Int());
    //enumnerate the interfaces
    iConnection1.EnumerateConnections(iInitNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Initial Interfaces: %d!\n "), iInitNumOfInterfaces);

    //Shutdown the first connection.
    iConnection1.Stop();
 
    //enumnerate the interfaces again.
    iConnection2.EnumerateConnections(iCurNumOfInterfaces);
    INFO_PRINTF2(_L("Number of Interfaces after Stopping the VTun Interface: %d!\n "), iCurNumOfInterfaces);
    if(iCurNumOfInterfaces>iInitNumOfInterfaces)
        {
        ERR_PRINTF1(_L("ERROR: More Number of Interfaces than expected."));    
        SetError(KErrGeneral);
        iConnection2.Close();
        iSocketServ.Close();
        return;
        }
    //open udp socket over second connection and send data over it.

    if((iStatus = iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection2)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket: %d"), iStatus.Int());    
        SetError(iStatus.Int());
        iConnection2.Close();
        iSocketServ.Close();
        return;
        }

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Sending Data through Socket!";

    // Get destination address from config file     
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KRealDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
        iSocket.Close();
        iConnection2.Close();
        iSocketServ.Close();
        return;
        }
    //set destination IP ie. server
    TInt err = iRealDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
        iConnection2.Close();
        iSocketServ.Close();       
        return;
        }

    returnValue = GetIntFromConfig(aSection, KRealDestPort, iRealDestPort);
    iRealDestAddr.SetPort(iRealDestPort);
    //bind local port to socket
    err = iSocket.SetLocalPort(LOCAL_PORT);

    //send data to server
    iSocket.SendTo(iSendBuf, iRealDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent"));
    else
    		{
      	ERR_PRINTF2(_L("\nData sending failed: %d"), iStatus.Int());
      	SetError(iStatus.Int());
      	iSocket.Close();
      	iConnection2.Close();
      	iSocketServ.Close();
      	return;
      	}

    iRecvBuf.Zero();
    iSocket.RecvFrom(iRecvBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);
    TBuf<128> temp;
    if( iStatus == KErrNone )
        {
        INFO_PRINTF1(_L("\nData Received from Socket : "));
        temp.Copy(iRecvBuf);
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));
        }
    else
        {
        ERR_PRINTF1(_L("\nData receive failed on Socket"));
        SetError(iStatus.Int());
        }  

    iSocket.Close();
    iConnection2.Close();
    iSocketServ.Close();
        
    }

void CTunDriverTestWrapper::DoRConnTest1_15()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_15             "));
    INFO_PRINTF1(_L("********************************************"));

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }

    iSocket.Open(iSocketServ,_L("tun"));

    TUint localPort = 6789;
    TPckgC <TUint> portPckg (localPort);

    //Positive case

    if((iStatus = iSocket.SetOpt(KSolInetIp,KSoTunnelPort,portPckg)) == KErrNone )
        INFO_PRINTF1(_L("Socket SetOpt Positive scenario succesful Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error in Socket SetOpt: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocket.Close();
        iSocketServ.Close();
        return;
        }

    //Negative Case #1
    if((iStatus = iSocket.SetOpt(KSolInetUdp,KSoTunnelPort,portPckg)) == KErrNotSupported )
        INFO_PRINTF1(_L("Socket SetOpt Negative scenario #1 succesful Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error in Socket SetOpt: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocket.Close();
        iSocketServ.Close();
        return;
        }

    //Negative Case #2 
    if((iStatus = iSocket.SetOpt(KSolInetUdp,KSoNoSourceAddressSelect,portPckg)) == KErrNotSupported )
        INFO_PRINTF1(_L("Socket SetOpt Negative scenario #2 succesful Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error in Socket SetOpt: %d"), iStatus.Int());
        SetError(iStatus.Int());
        }

    iSocket.Close();
    iSocketServ.Close();
   }

void CTunDriverTestWrapper::DoRConnTest1_16(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_16            "));
    INFO_PRINTF1(_L("********************************************"));


    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        return;
        }
    if((iStatus = iConnection.Open(iSocketServ)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ.Close();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    iSocket.Open(iSocketServ,_L("tun"));

    TUint localPort = 6789;
    TPckgC <TUint> portPckg (localPort);

    //Setting the Port Number into the Stack
    if((iStatus = iSocket.SetOpt(KSolInetIp,KSoTunnelPort,portPckg)) == KErrNone )
        INFO_PRINTF1(_L("Socket SetOpt Positive scenario succesful Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error in Socket SetOpt: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        return;
        }

    // Get destination address from config file 
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }
    
    //set destination IP 
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }

    //set the destination port
    returnValue = GetIntFromConfig(aSection, KDestPort, iDestPort);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }
    
    iDestAddr.SetPort(iDestPort);

    //retrieve the local address
    TInetAddr iLocalAddr;
    returnValue = GetStringFromConfig(aSection, KLocalAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading LocalAddr"));   
        SetError(KErrUnknown);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }
    
    err = iLocalAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }

    iLocalAddr.SetPort(LOCAL_PORT);

    //open socket on interface.
    if((iStatus = iSocket1.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp,iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket1 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket1: %d"), iStatus.Int());    
        SetError(iStatus.Int());
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }

    //open socket on interface.
    if((iStatus = iSocket2.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetIcmp,iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket2 Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF2(_L("Error Opening Socket2: %d"), iStatus.Int());    
        SetError(iStatus.Int());
				iSocket.Close();
				iSocket1.Close();
        iConnection.Close();
        iSocketServ.Close();
        return;
        }

    err = iSocket1.Bind(iLocalAddr);
    if(err != KErrNone)
    		{
        ERR_PRINTF2(_L("Error Binding Socket1: %d"), err);    
        SetError(iStatus.Int());
 				iSocket.Close();
 				iSocket1.Close();
 				iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
				return;
				}

    TInt iLocalPort;
    returnValue = GetIntFromConfig(aSection, KLocalPort, iLocalPort);
    iLocalAddr.SetPort(iLocalPort);

    err = iSocket2.Bind(iLocalAddr);
    if(err != KErrNone)
				{
        ERR_PRINTF2(_L("Error Binding Socket2: %d"), err);
        SetError(iStatus.Int());
 				iSocket.Close();
 				iSocket1.Close();
 				iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
				return;
				}

    iSendBuf.Zero();
    iSendBuf = (TText8*)"Test Message";

    //send data to server using the local port = LOCAL_PORT (7777)
    iSocket1.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);

    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru socket1"));
    else
				{
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
 				iSocket.Close();
 				iSocket1.Close();
 				iSocket2.Close();
        iConnection.Close();
        iSocketServ.Close();
				return;
				}
				
    //send data to server using the local port configured in ini file 
    iSocket2.SendTo(iSendBuf, iDestAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);

    if( iStatus == KErrNone)
        INFO_PRINTF1(_L("\nData Sent thru socket2"));
    else
    		{
        ERR_PRINTF1(_L("\nData sending failed"));
        SetError(iStatus.Int());
      	}
		iSocket.Close();
    iSocket1.Close();
    iSocket2.Close();
    iConnection.Close();
    iSocketServ.Close();
    }

//Simple TCP data transfer.
void CTunDriverTestWrapper::DoRConnTest1_17(const TDesC& aSection)
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_17             "));
    INFO_PRINTF1(_L("*********************************************"));

    //start the dummy3rdpartyapp
    if ((iVTunError = StartD3PApp()) != KErrNone)
        {
        return;
        }

    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
        StopD3PApp();
        return;
        }

    if((iStatus = iConnection.Open(iSocketServ /*, KAfInet*/)) == KErrNone )
        INFO_PRINTF1(_L("Connection opened. \n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
		if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    //open socket on interface. 
    if((iStatus = iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, iConnection)) == KErrNone )
        INFO_PRINTF1(_L("Socket Opened!\n "));
    else   // (iStatus!=KErrNone)
        {
        ERR_PRINTF1(_L("Error Opening Socket!"));    
        SetError(iStatus.Int());
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    // Get destination address from config file    
    TPtrC ptrToReadFromConfig(KNameDefault);
    TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));   
        SetError(KErrUnknown);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    //set destination IP ie. server
    TInt err = iDestAddr.Input(ptrToReadFromConfig);   
    if(err != KErrNone)
        {
        INFO_PRINTF2(KInetInputFail, err);
        SetError(err);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    returnValue = GetIntFromConfig(aSection, KTcpDestPort, iDestPort);
    if (!returnValue)
        {
        ERR_PRINTF1(_L("Reading config file failed, while reading DestPort"));   
        SetError(KErrUnknown);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    iDestAddr.SetPort(iDestPort);
    //bind local port to socket
    err = iSocket.SetLocalPort(7776);
    if (err != KErrNone)
        {
        ERR_PRINTF1(_L("Error Setting local port"));   
        SetError(err);
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }

    iSocket.Connect(iDestAddr,iStatus);
    User::WaitForRequest(iStatus);
    if (iStatus != KErrNone)
        {
        ERR_PRINTF1(_L("Error Connecting to Server!"));    
        SetError(iStatus.Int());
				iSocket.Close();
        iConnection.Close();
        iSocketServ.Close();
        StopD3PApp();
        return;
        }
    else
        {
        INFO_PRINTF2(_L("tcp Start with = %d\n"), iStatus.Int());
        }
 	  TBuf8<128> receiveBuffer;
    TBuf<128>  temp;

    do
        {
        TSockXfrLength aLen = 0;
        iRecvBuf.FillZ();
        iSocket.RecvOneOrMore(receiveBuffer,0,iStatus,aLen);
        User::WaitForRequest(iStatus);
        temp.Copy(receiveBuffer);

        INFO_PRINTF1(_L("\nData Received from Socket : "));
        INFO_PRINTF1(temp);
        INFO_PRINTF1(_L("\n"));

        }while(iStatus == KErrNone);      

    //Close the Connection,Socket and socket server.
    iSocket.Close();
    iConnection.Stop();
    iSocketServ.Close();
    StopD3PApp();
    }


//For Code Coverage.

void CTunDriverTestWrapper::DoRConnTest1_20()
{
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_20             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_21()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_21             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_22()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_22             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_23()
    {

    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_23             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }


void CTunDriverTestWrapper::DoRConnTest1_24()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_24             "));
    INFO_PRINTF1(_L("********************************************"));

		StartConn(); 



      
        
    }

void CTunDriverTestWrapper::DoRConnTest1_25()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_25             "));
    INFO_PRINTF1(_L("********************************************"));


    StartConn();    
    }

void CTunDriverTestWrapper::DoRConnTest1_26()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_26             "));
    INFO_PRINTF1(_L("********************************************"));


    StartConn();
    }


void CTunDriverTestWrapper::DoRConnTest1_27()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_27             "));
    INFO_PRINTF1(_L("********************************************"));

    StartConn();
    }


void CTunDriverTestWrapper::DoRConnTest1_28()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_28             "));
    INFO_PRINTF1(_L("********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_29()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_29             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_30()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_30             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_31()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_31             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }


void CTunDriverTestWrapper::DoRConnTest1_32()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_32             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }

void CTunDriverTestWrapper::DoRConnTest1_33()
    {
    INFO_PRINTF1(_L("*********************************************"));
    INFO_PRINTF1(_L("           Test: DoRConnTest1_33             "));
    INFO_PRINTF1(_L("*********************************************"));

    StartConn();
    }


void CTunDriverTestWrapper::StartConn()
    {
    if((iStatus = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened.\n"));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), iStatus.Int());
        SetError(iStatus.Int());
             return;
        }

		if((iStatus = iConnection.Open(iSocketServ)) == KErrNone )
    		INFO_PRINTF1(_L("Connection opened. \n"));
		else
    		{
        ERR_PRINTF2(_L("Error Opening Connection: %d"), iStatus.Int());
        SetError(iStatus.Int());
        iSocketServ.Close();
        return;
        }
           
		iCommDbPref.SetIapId(TUNDRIVER_IAP);             // Virtual Interface
    iCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    iConnection.Start(iCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    if(iStatus.Int()!=KErrNone)
	    	{
	    	ERR_PRINTF2(_L("Error starting connection: %d"), iStatus.Int());	
				SetError(iStatus.Int());
				iSocketServ.Close();
				return;
	    	}
    INFO_PRINTF2(_L("Starting Connection %d.\n"), iStatus.Int());

    iConnection.Stop();
    iSocketServ.Close();
    }
