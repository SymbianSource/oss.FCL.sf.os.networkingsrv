/*
 ============================================================================
 Name		 : Dummy3rdPartyApp.h
 Author	     : Gurpreet Singh Nagi
 Copyright   : Your copyright notice
 Description : Exe header file
 ============================================================================
 */

#ifndef __DUMMY3RDPARTYAPP_H__
#define __DUMMY3RDPARTYAPP_H__

//  Include Files

#include <e32base.h>
#include <es_sock.h>
#include <commdbconnpref.h>
#include <in_sock.h>

//  Function Prototypes

GLDEF_C TInt E32Main();

//functions:

class Dummy3rdPartyApp
    {
public:
    Dummy3rdPartyApp::Dummy3rdPartyApp()
        {

        }

    Dummy3rdPartyApp::~Dummy3rdPartyApp()
        {

        }
    void InitConn();
    void StartConn();
    void ProcessData();
    void CloseConn();

    TRequestStatus iStatus;
    TInetAddr iVirtAddr, iRealAddr;
    RSocketServ iVirtSocketServ, iRealSocketServ, iHookSocketServ;
    RSocket iVirtSocket, iRealSocket, iHookSocket;
    RConnection iVirtConnection, iRealConnection;
    TBuf8<2048> iSendBuf, iRecvBuf;

    TCommDbConnPref iVirtCommDbPref, iRealCommDbPref;
    };



#endif  // __DUMMY3RDPARTYAPP_H__

