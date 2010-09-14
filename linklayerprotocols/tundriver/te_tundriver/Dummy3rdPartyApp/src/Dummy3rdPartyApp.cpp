/*
 ============================================================================
 Name		 : Dummy3rdPartyApp.cpp
 Author	     : Gurpreet Singh Nagi
 Copyright   : Your copyright notice
 Description : Exe source file
 ============================================================================
 */

//  Include Files  

#include "Dummy3rdPartyApp.h"
#include <e32base.h>
#include <e32std.h>
#include <e32cons.h>			// Console

//  Constants
_LIT(KTextConsoleTitle, "Dummy3rdPartyApp");
_LIT(KTextFailed, " failed, leave code = %d");
_LIT(KTextPressAnyKey, " [press any key]\n");

#define BUF_SIZE 2048
//  Global Variables
LOCAL_D CConsoleBase* console; // write all messages to this


//  Local Functions
void Dummy3rdPartyApp::InitConn()
    {
    if((iStatus = iVirtSocketServ.Connect()) == KErrNone )
        console->Printf(_L("Opening Socket Server on Virtual Interface.\n"));

    if((iStatus = iRealSocketServ.Connect()) == KErrNone )
        console->Printf(_L("Opening Socket Server on Real Interface.\n"));
    
    if((iStatus = iHookSocketServ.Connect()) == KErrNone )
            console->Printf(_L("Opening Socket Server for Hook.\n"));

    if((iStatus = iVirtConnection.Open(iVirtSocketServ, KAfInet)) == KErrNone )
        console->Printf(_L("\nOpening Virtual Connection."));

    if((iStatus = iRealConnection.Open(iRealSocketServ, KAfInet)) == KErrNone )
        console->Printf(_L("\nOpening Real Connection.\n"));
    
    iHookSocket.Open(iHookSocketServ,_L("tun"));
    
    }

void Dummy3rdPartyApp::StartConn()
    {
    iVirtCommDbPref.SetIapId(14);             // Dummy Interface
    iVirtCommDbPref.SetBearerSet(KCommDbBearerVirtual);
    iVirtCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    TInt err = KErrNone;
    iVirtConnection.Start(iVirtCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    err = iStatus.Int();
    if(iStatus == KErrNone)
        console->Printf(_L("Connection Started on Virtual Interface.\n"));
    User::LeaveIfError(err);
    err = KErrNone;
    //open socket on Virtual interface.
    iStatus = iVirtSocket.Open(iVirtSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iVirtConnection);
    //if((iStatus = iVirtSocket.Open(iVirtSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iVirtConnection)) == KErrNone )
    if(iStatus == KErrNone)
        console->Printf(_L("Socket Opened on Virtual Interface.\n"));

    iRealCommDbPref.SetIapId(9);            //Real Interface i.e. ethernet
    iRealCommDbPref.SetBearerSet(KCommDbBearerLAN);
    iRealCommDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    iRealConnection.Start(iRealCommDbPref, iStatus);
    User::WaitForRequest(iStatus);
    if(iStatus == KErrNone)
        console->Printf(_L("Connection Started on Real Interface.\n"));
    err = iStatus.Int();
    //open socket on real interface
    if((iStatus = iRealSocket.Open(iRealSocketServ, KAfInet, KSockDatagram, KProtocolInetUdp, iRealConnection)) == KErrNone )
        console->Printf(_L("Socket Opened on Real Interface.\n"));
    User::LeaveIfError(err);

    //set destination port and IP ie. OpenVPN gateway address
    iRealAddr.SetV4MappedAddress(INET_ADDR(10,225,171,71));        
    iRealAddr.SetPort(1194);
    err = KErrNone;
    //bind local port to socket
    // To Get any available port from the kernel
    
    TInetAddr inetAddrBind;
    inetAddrBind.SetPort(KInetPortAny);
    inetAddrBind.SetAddress(KInetAddrAny);
    iVirtSocket.Bind(inetAddrBind);
    TUint port1  = iVirtSocket.LocalPort();
      
    TPckgC <TUint> portPckg (port1);
    iHookSocket.SetOpt(KSolInetIp,KSoTunnelPort,portPckg);

    console->Printf(_L("Socket bound on Virtual Interface.\n"), err);

    err = iRealSocket.SetLocalPort(1194); 
    console->Printf(_L("Socket bound on Real Interface.\n"), err);
     
    }


void Dummy3rdPartyApp::ProcessData()
    {
    //Receive data from client on virtual interface
    iVirtSocket.RecvFrom(iSendBuf, iVirtAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);

    if(iStatus == KErrNone)
        {
        console->Printf(_L("\nData Received from Virtual Interface:  "));
        //copy received data to temp buffer and print it
        //TBuf<BUF_SIZE> buf;
        //buf.Copy(iSendBuf);
        //console->Printf(buf);
        }
    else
        console->Printf(_L("\nData receive failed on Virtual Interface."));     

    //Send data on real interface to OpenVPN Gateway.
    iRealSocket.SendTo(iSendBuf, iRealAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);  

    if( iStatus == KErrNone)
        {
        console->Printf(_L("\nData Sent on Real interface : "));

        //display sent data
        //TBuf<BUF_SIZE> buf;
        //buf.Copy(iSendBuf);
        //console->Printf(buf);
        }
    else
        console->Printf(_L("\nData send failed on Real Interface"));   

    //Receive data from from real interface
    iRealSocket.RecvFrom(iRecvBuf, iRealAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);

    if(iStatus == KErrNone)
        {
        console->Printf(_L("\nData Received from Real Interface:  "));
        //copy received data to temp buffer and print it
        //TBuf<BUF_SIZE> buf;
        //buf.Copy(iRecvBuf);
        //console->Printf(buf);
        }
    else
        console->Printf(_L("\nData receive failed on Real Interface"));     

    //Send data to virtual interface.
    iVirtSocket.SendTo(iRecvBuf, iVirtAddr, NULL, iStatus);
    User::WaitForRequest(iStatus);  

    if( iStatus == KErrNone)
        {
        console->Printf(_L("\nData Sent on Virtual interface : "));

        //display sent data
        //TBuf<BUF_SIZE> buf;
        //buf.Copy(iRecvBuf);
        //console->Printf(buf);
        }
    else
        console->Printf(_L("\nData send failed on Virtual Interface"));   
    }

void Dummy3rdPartyApp::CloseConn()
    {
    //Close all handles.
    iVirtSocket.Close();
    iRealSocket.Close();
    iHookSocket.Close();

    iVirtConnection.Close();
    iRealConnection.Close();
    iVirtSocketServ.Close();
    iRealSocketServ.Close();
    iHookSocketServ.Close();
    }

LOCAL_C void MainL()
    {
    Dummy3rdPartyApp obj;

    //intializing connection and socket
    obj.InitConn();
    //start the connection
    obj.StartConn();

    while(1)
        {
        obj.ProcessData();
        }
    //close connection, subconnection, socket
    //TODO: how to explicitly close the loop? 
    // One possible solution is running a timer.
    obj.CloseConn();
    }

LOCAL_C void DoStartL()
    {
    // Create active scheduler (to run active objects)
    CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
    CleanupStack::PushL(scheduler);
    CActiveScheduler::Install(scheduler);

    MainL();
    // Delete active scheduler
    CleanupStack::PopAndDestroy(scheduler);
    }

//  Global Functions

GLDEF_C TInt E32Main()
    {
    // Create cleanup stack
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();

    // Create output console
    TRAPD(createError, console = Console::NewL(KTextConsoleTitle, TSize(
            KConsFullScreen, KConsFullScreen)));
    if (createError)
        return createError;

    // Run application code inside TRAP harness, wait keypress when terminated
    TRAPD(mainError, DoStartL());
    if (mainError)
        console->Printf(KTextFailed, mainError);
    console->Printf(KTextPressAnyKey);
    console->Getch();

    delete console;
    delete cleanup;
    __UHEAP_MARKEND;
    return KErrNone;
    }
