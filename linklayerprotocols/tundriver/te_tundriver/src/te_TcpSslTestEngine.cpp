/*
 * te_TcpSslTestEngine.cpp
 *
 *  Created on: Jan 28, 2010
 *      Author: GUNAGI
 */


#include "te_TcpSslTestEngine.h"
#include <commdbconnpref.h>

// Send buffer size
const TInt KSendBufferSize = 512;
// Receive buffer size
const TInt KReceiveBufferSize = 256;
// Maximum number of connection attempts
const TInt KMaxAttempts = 200;
// KYes Size
const TInt KYesSize = 3;
// KMessage Size
const TInt KMessageSize = 35;
// KNullDesC Size
const TInt KNullDesCSize = 0;
// Key header
const TInt KeyHeader = 0x400;
//const TInt KeyHeader = 0x04;
// Delay
const TInt KDelay = 100000;

_LIT(KYes,"Yes");
_LIT(KInputFile, "C:\\t_secdlg_in.dat");
_LIT(KOutputFile, "C:\\t_secdlg_out.dat");
_LIT(KMessage, "Passphrase of the imported key file");
//_LIT(KMessage, "1234");

// HTTP messages
_LIT8(KSimpleGet, "GET ");
_LIT8(KNewLine, "\r\n"); 
_LIT8(KHeader, "Connection: close\r\nUser-Agent: SSL_TEST\r\nAccept-Encoding:\r\nAccept: */*");

// Progress messages
_LIT(KConnnectedMessage, "\nConnecting to %S:%d%S\n");
_LIT(KSecureConnnectingMessage, "\nMaking secure connection");
_LIT(KGettingPageMessage, "\nRequesting web page");
_LIT(KReceivingMessage,"\nReceiving server response");
_LIT(KCipherSuiteInUseMessage,"\nCipher suite in use: %S");
_LIT(KProtocolMessage, "\nProtocol used in connection: %S");
_LIT(KReceivedMessage,"\nReceived server response\n");
_LIT(KCompleteMessage,"\nTransaction complete: bytes recieved %d");
_LIT(KFileErrorMessage,"\nError in writing data to file");
_LIT(KCancelledMessage,"\nConnection closed");
_LIT(KFailSocketMessage,"\nFailed to open socket: %d");

// State reporting messages
_LIT( KStateErrESocketConnected1, "\nError in state connecting socket: %d\n" );
_LIT( KStateErrESocketConnected, "\nError in state ESocketConnected: %d\n" );
_LIT( KStateErrESettingCiphers, "\nError in state ESettingCiphers: %d\n" );
_LIT( KStateErrSecureConnected, "\nError in state ESecureConnected: %d\n" );
_LIT( KStateErrGetRequestSent, "\nError in state EGetRequestSent: %d\n" );
_LIT( KStateErrEDataReceived, "\nError in state EDataReceived: %d\n" );

// Panic code
_LIT( KTcpSslEnginePanic, "TCPSSL-ENGINE");

//
// CTcpSslTestEngine
//

CTcpSslTestEngine* CTcpSslTestEngine::NewL()
    {
    CTcpSslTestEngine* self = new(ELeave) CTcpSslTestEngine;
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop();        
    return self;
    }

// Constructor should also call the parent constructor to set the priority
// of the active object.
CTcpSslTestEngine::CTcpSslTestEngine() 
: CActive(0), iSndBuffer(0,0), iRcvBuffer(0,0)
            {
            }

CTcpSslTestEngine::~CTcpSslTestEngine()
    {
    // Cancel any outstanding request- this cleans up
    // resources created during a connection
    //Cancel(); 
    ConnectionClosed();
    // Clean up engine's permanent resources
    delete (void*)iSndBuffer.Ptr();
    delete (void*)iRcvBuffer.Ptr();
    iTimer.Close();
    iSocketServ.Close();
    }

void CTcpSslTestEngine::ConstructL()
    {
    iSndBuffer.Set((TUint8*)User::AllocL(KSendBufferSize),0,KSendBufferSize);
    iRcvBuffer.Set((TUint8*)User::AllocL(KReceiveBufferSize),0,KReceiveBufferSize);
    // Connect the socket server
    User::LeaveIfError( iSocketServ.Connect()); 
    // Create a local timer
    User::LeaveIfError( iTimer.CreateLocal());
    // Set initial state
    iRunState = ESocketConnected;
    iInUse = EFalse;

    CActiveScheduler::Add( this );

    }

void CTcpSslTestEngine::ConnectL(const TConnectSettings& aConnectSettings)
    {
    iConnectSettings = &aConnectSettings;

    // Set initial values for flags & buffers
    iSuccess = ETrue;
    iInUse = ETrue;
    iRunState = ESocketConnected;
    iSndBuffer.SetLength( 0 );
    iRcvBuffer.SetLength( 0 );
    iTotalBytesRead = 0;

    TRequestStatus status;
    RConnection rCon;
    TInt ret = rCon.Open(iSocketServ);
    iConsole->Printf(_L("Connection open with = %d\n"), ret);

    TCommDbConnPref commDbPref;
    commDbPref.SetIapId(12);
    commDbPref.SetBearerSet(KCommDbBearerVirtual);
    commDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    rCon.Start(commDbPref, status);
    User::WaitForRequest(status);

    // Interpret server address
    if (iInetAddr.Input(iConnectSettings->iAddress) != KErrNone)
        // Success if already in dotted-decimal format
        {
        // Connect to a host resolver (for DNS resolution) - happens sychronously
        //User::LeaveIfError( iHostResolver.Open( iSocketServ, KAfInet, KProtocolInetTcp, rCon ));
        // Try to resolve symbolic name
        //TNameEntry nameEntry;
        //User::LeaveIfError (iHostResolver.GetByName( iConnectSettings->iAddress, nameEntry ));
        //TSockAddr sockAddr = nameEntry().iAddr;
        //iInetAddr = iInetAddr.Cast( sockAddr );
        //iHostResolver.Close();
        }

    // Store other connection parameters
    iInetAddr.SetPort( iConnectSettings->iPortNum );

    /*   RSocketServ SockServ;
     SockServ.Connect();

     TInt err(0);
     TInt ret = rCon.Open(SockServ);
     iConsole->Printf(_L("Connection open with = %d\n"), ret);*/


    // Open a TCP socket
    ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, rCon );

    if(ret != KErrNone)
        {
        iConsole->Printf(KStateErrESocketConnected1, ret);  
        }
    else
        {

        User::LeaveIfError(ret);
        }


    // Connect to the server, asynchronously
    iSocket.Connect( iInetAddr, iStatus );  
    SetActive();
    CActiveScheduler::Start();

    // Print status
    iConsole->Printf(KConnnectedMessage, 
            &iConnectSettings->iAddress, 
            iConnectSettings->iPortNum, 
            &iConnectSettings->iPage ); 
    }

void CTcpSslTestEngine::SetConsole( CConsoleBase& aConsole )
    {
    iConsole = &aConsole;
    }

void CTcpSslTestEngine::SetOutputFile( RFile& aOutputFile )
    {
    iOutputFile = &aOutputFile;
    }

void CTcpSslTestEngine::Initialize()
    {

    }
TBool CTcpSslTestEngine::InUse()
    {
    return iInUse;
    }

void CTcpSslTestEngine::RunL()
    {
    //The Test App that uses the tls module should check the status prior to continuing.
    //Detailed error checking need to be added in the Test App.
    if (iStatus.Int() != KErrNone ) 
        {
        switch ( iStatus.Int() )
            {
            case KErrCouldNotConnect:
                {
                _LIT( errmsg, "Failed to connect. error code : %d" );
                iConsole->Printf(errmsg, iStatus.Int());
                //User::Panic(KTcpSslEnginePanic,iStatus.Int());
                break;
                }
            default:
                {
                iRunState =  EConnectionClosed;
                _LIT( errmsg, "Error encountered. error code : %d" );
                iConsole->Printf(errmsg, iStatus.Int());
                break;
                }
            }
        }

    switch ( iRunState )
        {
        case ESocketConnected:
            MakePageRequestL();
            break;

        case EGetRequestSent:
            GetServerResponseL();
            break;

        case EDataReceived:
            ReadServerResponseL();
            break;

        case EConnectionClosed:
            ConnectionClosed();
            break;

        default:
            break;
        } // end switch
    }

TInt CTcpSslTestEngine::RunError( TInt aError )
    {
    // Panic prevents looping
    __ASSERT_ALWAYS(iRunState != EConnectionClosed, 
            User::Panic(KTcpSslEnginePanic,0));

    // do a switch on the state to get the right err message
    switch (iRunState)
        {
        case ESocketConnected:
            iConsole->Printf(KStateErrESocketConnected, aError );
            break;
        case ESettingCiphers:
            iConsole->Printf(KStateErrESettingCiphers, aError );
            break;
        case ESecureConnected:
            iConsole->Printf(KStateErrSecureConnected, aError);
            break;
        case EGetRequestSent:
            iConsole->Printf(KStateErrGetRequestSent, aError);
            break;
        case EDataReceived:
            iConsole->Printf(KStateErrEDataReceived, aError);
            break;
        default:
            break;
        }

    iRunState = EConnectionClosed;
    iSuccess = EFalse;
    iTimer.After( iStatus, KDelay );
    SetActive();

    return KErrNone;
    }

void CTcpSslTestEngine::DoCancel()
    {
    iConsole->Printf(KCancelledMessage);
    ConnectionClosed();
    }

void CTcpSslTestEngine::MakePageRequestL()
    {

    // if connection failed, reconnect
    if(iStatus.Int() != KErrNone)
        {
        if(++iCounter >= KMaxAttempts)
            {
            User::Leave(iStatus.Int());
            }

        iRunState = ESocketConnected;
        iSocket.CancelAll();
        iSocket.Close();

        RConnection rCon;
        TInt ret = rCon.Open(iSocketServ);
        iConsole->Printf(_L("Connection open with = %d\n"), ret);

        TRequestStatus status;

        TCommDbConnPref commDbPref;
        commDbPref.SetIapId(12);
        commDbPref.SetBearerSet(KCommDbBearerVirtual);
        commDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

        rCon.Start(commDbPref, status);
        User::WaitForRequest(status);
        // Open a TCP socket
        ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, rCon );

        while(KErrNone != ret)
            {       
            iConsole->Printf(KFailSocketMessage, ret);          
            ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, rCon );
            }


        // Connect to the server, asynchronously
        iSocket.Connect( iInetAddr, iStatus );  
        SetActive();

        // Print status
        iConsole->Printf(KConnnectedMessage, 
                &iConnectSettings->iAddress, 
                iConnectSettings->iPortNum, 
                &iConnectSettings->iPage ); 

        return;
        }

    iCounter = 0;
    iConsole->Printf(KGettingPageMessage);

    // Create a GET request 
    _LIT8(KUrl1, "https://");
    _LIT8(KUrl2, ":443/index.html HTTP/1.0\r\n");
    iSndBuffer+=KSimpleGet;
    iSndBuffer+=KUrl1;
    iSndBuffer+=hostIpAddress;
    iSndBuffer+=KUrl2;
    iSndBuffer+=KNewLine;   
    iSndBuffer+=KHeader;

    // Send the request
    iRunState = EGetRequestSent;
    iSocket.Send( iSndBuffer, NULL, iStatus, iBytesSent );
    SetActive();
    }

void CTcpSslTestEngine::GetServerResponseL()       
    {
    // The get request has been sent, can now try and receive the data
    User::LeaveIfError(iStatus.Int());
    iConsole->Printf(KReceivingMessage);

    // Read asynchonously-returns when buffer full
    iRunState = EDataReceived;
    iSocket.RecvOneOrMore( iRcvBuffer, NULL, iStatus );
    SetActive();
    }

void CTcpSslTestEngine::ReadServerResponseL()
    {
    // Any error other than KErrEof means the test is a failure
    if (iStatus!=KErrEof) User::LeaveIfError(iStatus.Int());
    iConsole->Printf(KReceivedMessage);
    TBuf<512> temp;
    temp.Copy(iRcvBuffer);
    iConsole->Printf(temp);

    // Put the received data in the output file & reset the receive buffer
    iTotalBytesRead += iRcvBuffer.Length();

    // Case 1: error is KErrEof (message complete) or no data received, so stop
    if ( ( iStatus==KErrEof ) || ( iRcvBuffer.Length() == 0 ) )
        {
        iConsole->Printf(KCompleteMessage, iTotalBytesRead);
        // Close the socket neatly
        iRunState = EConnectionClosed;
        iTimer.After( iStatus, KDelay );
        SetActive();
        return; 
        }

    // Case 2: there's more data to get from the server
    iRcvBuffer.SetLength( 0 );
    iRunState = EDataReceived;
    iSocket.RecvOneOrMore( iRcvBuffer, NULL, iStatus );
    SetActive(); 
    }

void CTcpSslTestEngine::ConnectionClosed()
    {
    if (!iInUse) return;
    // Clean up
    //iHostResolver.Close();
    iSocket.CancelAll();
    iSocket.Close();

    // Close the output file
    //iOutputFile->Close();

    // Wait here for an unload of the ssl.dll to make sure that a session is not
    // reconnected next time. 
    User::After( KDelay );
    iInUse = EFalse;
    SetActive();
    CActiveScheduler::Stop();
    }
